"""
ISO/IEC 7816 High Level Analyzer
Processes char-level frames from the ISO7816 Low Level Analyzer and reconstructs
ATR, PPS, T=0 TPDU/APDU, and T=1 block/APDU frames.

Each input char frame carries (in order):
  - data        (U8):  raw byte value
  - sender      (str): "card" or "reader" (reliable for ATR/PPS; HLA determines
                        direction for T0/T1 from protocol position)
  - protocol    (str): "atr", "pps", "t0", or "t1"
  - description (str): field name assigned by C++ LLA (e.g. "TS(direct)", "TA1(Fi=372,Di=1)")

Display level setting:
  - APDU : emit atr + pps + apdu (T=0) + t1_apdu (T=1) + t1_block for R/S only
  - TPDU : emit atr + pps + tpdu (T=0) + t1_block for every block
"""

import re
from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, ChoicesSetting

# ── lookup tables ────────────────────────────────────────────────────────────

_FN_TABLE = [372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048, 0, 0]
_DN_TABLE = [0, 1, 2, 4, 8, 16, 32, 64, 12, 20, 0, 0, 0, 0, 0, 0]

_INS_NAMES = {
    0x04: 'DEACTIVATE FILE',  0x0C: 'ERASE RECORD',       0x0E: 'ERASE BINARY',
    0x10: 'PERFORM SCQL',     0x12: 'PERFORM TRANSACTION', 0x14: 'PERFORM USER',
    0x20: 'VERIFY',           0x22: 'MANAGE SE',           0x24: 'CHANGE REF DATA',
    0x26: 'DISABLE REF DATA', 0x28: 'ENABLE REF DATA',     0x2A: 'PERFORM CRYPTO',
    0x2C: 'RESET RETRY CTR',  0x32: 'INCREASE',            0x44: 'ACTIVATE FILE',
    0x46: 'GEN ASYM KEY',     0x70: 'MANAGE CHANNEL',      0x82: 'EXT AUTH',
    0x84: 'GET CHALLENGE',    0x86: 'GEN AUTHENTICATE',    0x88: 'INT AUTH',
    0xA0: 'SEARCH BINARY',    0xA2: 'SEARCH RECORD',       0xA4: 'SELECT',
    0xB0: 'READ BINARY',      0xB2: 'READ RECORD',         0xC0: 'GET RESPONSE',
    0xC2: 'ENVELOPE',         0xCA: 'GET DATA',            0xD0: 'WRITE BINARY',
    0xD2: 'WRITE RECORD',     0xD6: 'UPDATE BINARY',       0xDA: 'PUT DATA',
    0xDC: 'UPDATE RECORD',    0xE0: 'CREATE FILE',         0xE2: 'APPEND RECORD',
    0xE4: 'DELETE FILE',      0xE6: 'TERMINATE DF',        0xE8: 'TERMINATE EF',
    0xFE: 'TERMINATE CARD',
}


def _get_fn(fi_idx):
    v = _FN_TABLE[fi_idx] if 0 <= fi_idx < len(_FN_TABLE) else 0
    return v if v > 0 else 372


def _get_dn(di_idx):
    v = _DN_TABLE[di_idx] if 0 <= di_idx < len(_DN_TABLE) else 0
    return v if v > 0 else 1


def _ins_name(ins):
    return _INS_NAMES.get(ins & 0xFE, f'INS({ins:#04x})')


def _sw_string(sw1, sw2):
    if sw1 == 0x61:
        return f'OK ({sw2} more bytes)'
    if sw1 == 0x62:
        info = {0x00: 'no info', 0x81: 'part corrupt', 0x82: 'EOF',
                0x83: 'deactivated', 0x84: 'FCI corrupt'}
        return f'Warning ({info.get(sw2, f"{sw2:#04x}")})'
    if sw1 == 0x63:
        if (sw2 & 0xF0) == 0xC0:
            return f'Auth fail ({sw2 & 0x0F} retries left)'
        return f'Warning ({sw2:#04x})'
    if sw1 == 0x64:
        return 'Exec error (no change)'
    if sw1 == 0x65:
        return 'Exec error (changed)'
    if sw1 == 0x67:
        return 'Wrong length'
    if sw1 == 0x68:
        return f'CLA not supported ({sw2:#04x})'
    if sw1 == 0x69:
        info = {0x82: 'security status', 0x83: 'auth blocked',
                0x84: 'ref data invalid', 0x85: 'conditions not satisfied',
                0x86: 'no current EF', 0x87: 'expected SM', 0x88: 'SM incorrect'}
        return f'Not allowed ({info.get(sw2, f"{sw2:#04x}")})'
    if sw1 == 0x6A:
        info = {0x80: 'wrong data', 0x81: 'func not supported',
                0x82: 'file not found', 0x83: 'record not found',
                0x84: 'no memory', 0x86: 'incorrect P1P2', 0x88: 'ref not found'}
        return f'Wrong P1P2 ({info.get(sw2, f"{sw2:#04x}")})'
    if sw1 == 0x6C:
        return f'Wrong length (use {sw2:#04x})'
    if sw1 == 0x6D:
        return 'INS not supported'
    if sw1 == 0x6E:
        return 'CLA not supported'
    if sw1 == 0x6F:
        return 'Unknown error'
    if sw1 == 0x90 and sw2 == 0x00:
        return 'OK'
    if sw1 == 0x9F:
        return f'OK ({sw2} more bytes)'
    if 0x90 <= sw1 <= 0x9F:
        return f'OK ({sw1:#04x} {sw2:#04x})'
    return f'SW={sw1:#04x}{sw2:#04x}'


# ── HLA class ────────────────────────────────────────────────────────────────

class Hla(HighLevelAnalyzer):

    display_level = ChoicesSetting(choices=('APDU', 'TPDU'))

    result_types = {
        'atr':      {'format': 'ATR: {{data.description}}'},
        'pps':      {'format': 'PPS [{{data.sender}}]: {{data.description}}'},
        'tpdu':     {'format': 'TPDU [{{data.sender}}]: {{data.description}}'},
        'apdu':     {'format': 'APDU: {{data.description}}'},
        't1_block': {'format': 'T1 [{{data.sender}}]: {{data.description}}'},
        't1_apdu':  {'format': 'T1 APDU: {{data.description}}'},
    }

    def __init__(self):
        self._phase = None

        # ATR accumulation
        self._atr_start = None
        self._atr_end = None
        self._atr_chars = []  # list of (value, label)

        # PPS state machine
        self._pps_state = 'PPSS'
        self._pps_start = None
        self._pps_pps0 = 0
        self._pps_fi = None
        self._pps_di = None
        self._pps_exchange = 0  # 0 = reader→card, 1 = card echo

        # T=0 state machine
        self._t0_state = 'HEADER'
        self._t0_start = None
        self._t0_header = []   # [CLA, INS, P1, P2, P3]
        self._t0_data_count = 0
        self._t0_ins = 0
        self._t0_p3 = 0
        self._t0_sw1 = 0

        # T=1 state machine
        self._t1_state = 'NAD'
        self._t1_start = None
        self._t1_block_sender = 'reader'
        self._t1_pcb = 0
        self._t1_len = 0
        self._t1_data_count = 0
        self._t1_edc_len = 1   # LRC default
        self._t1_edc_count = 0
        # APDU chain accumulation
        self._t1_apdu_active = False
        self._t1_apdu_start = None
        self._t1_apdu_data = []

    # ── public entry point ───────────────────────────────────────────────────

    def decode(self, frame: AnalyzerFrame):
        if frame.type != 'char':
            return None

        raw = frame.data.get('data', 0)
        value = raw[0] if isinstance(raw, (bytes, bytearray)) else int(raw)
        label = frame.data.get('description', '')
        phase = frame.data.get('protocol', 'unknown')

        results = []

        if phase != self._phase:
            if self._phase == 'atr' and self._atr_chars:
                f = self._emit_atr()
                if f:
                    results.append(f)
            self._phase = phase

        if phase == 'atr':
            self._collect_atr(frame, value, label)
        elif phase == 'pps':
            results.extend(self._process_pps(frame, value))
        elif phase == 't0':
            results.extend(self._process_t0(frame, value))
        elif phase == 't1':
            results.extend(self._process_t1(frame, value))

        return results if results else None

    # ── ATR ─────────────────────────────────────────────────────────────────

    def _collect_atr(self, frame, value, label):
        if not self._atr_chars:
            self._atr_start = frame.start_time
            # Reset sub-protocol states for a fresh session
            self._t1_state = 'NAD'
            self._t1_block_sender = 'reader'
            self._t1_apdu_active = False
            self._t0_state = 'HEADER'
            self._t0_header = []
        self._atr_chars.append((value, label))
        self._atr_end = frame.end_time

    def _emit_atr(self):
        if not self._atr_chars or self._atr_start is None:
            return None

        convention = ''
        fi_di = ''
        for _v, lbl in self._atr_chars:
            if 'TS' in lbl:
                convention = 'direct' if 'direct' in lbl else 'inverse' if 'inverse' in lbl else ''
            if 'TA1' in lbl:
                m = re.search(r'Fi=(\d+),Di=(\d+)', lbl)
                if m:
                    fi, di = int(m.group(1)), int(m.group(2))
                    fi_di = f'Fi={fi} Di={di}'
                    if di > 0:
                        fi_di += f' ETU={fi // di}'

        parts = [p for p in [convention, fi_di] if p]
        if not parts:
            parts = [f'{len(self._atr_chars)} bytes']

        f = AnalyzerFrame('atr', self._atr_start, self._atr_end,
                          {'description': ' '.join(parts)})
        self._atr_chars = []
        self._atr_start = None
        self._atr_end = None
        self._pps_state = 'PPSS'
        self._pps_exchange = 0
        return f

    # ── PPS ─────────────────────────────────────────────────────────────────

    def _process_pps(self, frame, value):
        if self._pps_state == 'PPSS':
            self._pps_start = frame.start_time
            self._pps_pps0 = 0
            self._pps_fi = None
            self._pps_di = None
            self._pps_state = 'PPS0'
            return []

        if self._pps_state == 'PPS0':
            self._pps_pps0 = value
            self._pps_state = ('PPS1' if value & 0x10 else
                               'PPS2' if value & 0x20 else
                               'PPS3' if value & 0x40 else 'PCK')
            return []

        if self._pps_state == 'PPS1':
            self._pps_fi = _get_fn((value >> 4) & 0x0F)
            self._pps_di = _get_dn(value & 0x0F)
            self._pps_state = ('PPS2' if self._pps_pps0 & 0x20 else
                               'PPS3' if self._pps_pps0 & 0x40 else 'PCK')
            return []

        if self._pps_state == 'PPS2':
            self._pps_state = 'PPS3' if self._pps_pps0 & 0x40 else 'PCK'
            return []

        if self._pps_state == 'PPS3':
            self._pps_state = 'PCK'
            return []

        if self._pps_state == 'PCK':
            prot = self._pps_pps0 & 0x0F
            sender = 'reader' if self._pps_exchange == 0 else 'card'
            parts = [f'T={prot}']
            if self._pps_fi is not None:
                parts.append(f'Fi={self._pps_fi}')
            if self._pps_di is not None:
                parts.append(f'Di={self._pps_di}')
                if self._pps_di > 0 and self._pps_fi:
                    parts.append(f'ETU={self._pps_fi // self._pps_di}')
            f = AnalyzerFrame('pps', self._pps_start, frame.end_time,
                              {'description': ' '.join(parts), 'sender': sender})
            self._pps_exchange = 1 - self._pps_exchange
            self._pps_state = 'PPSS'
            return [f]

        return []

    # ── T=0 ─────────────────────────────────────────────────────────────────
    # Sender determined by protocol position: reader sends header + command
    # data; card sends procedure bytes, response data, and SW.

    def _process_t0(self, frame, value):
        state = self._t0_state

        if state == 'HEADER':
            if not self._t0_header:
                self._t0_start = frame.start_time
                self._t0_data_count = 0
            self._t0_header.append(value)
            if len(self._t0_header) == 5:
                self._t0_ins = self._t0_header[1]
                self._t0_p3 = self._t0_header[4]
                self._t0_state = 'procedure'
            return []

        if state == 'procedure':
            proc = value
            ins = self._t0_ins
            if proc == 0x60:
                pass  # NULL — wait, stay in procedure
            elif (proc & 0xF0) in (0x60, 0x90):
                self._t0_sw1 = proc
                self._t0_state = 'SW2'
            elif proc == ins or proc == (ins ^ 0x01):
                self._t0_data_count = 0
                self._t0_state = 'Bytes'
            elif proc == (ins ^ 0xFF) or proc == (ins ^ 0xFE):
                self._t0_state = 'Byte'
            return []

        if state == 'SW2':
            sw_str = _sw_string(self._t0_sw1, value)
            ins_name = _ins_name(self._t0_ins)
            cla, p1, p2 = self._t0_header[0], self._t0_header[2], self._t0_header[3]
            start = self._t0_start
            self._t0_state = 'HEADER'
            self._t0_header = []
            if self.display_level == 'TPDU':
                desc = f'{ins_name} CLA={cla:#04x} P1={p1:#04x} P2={p2:#04x} → {sw_str}'
                return [AnalyzerFrame('tpdu', start, frame.end_time,
                                      {'description': desc, 'sender': 'reader'})]
            else:
                return [AnalyzerFrame('apdu', start, frame.end_time,
                                      {'description': f'{ins_name} {sw_str}'})]

        if state == 'Bytes':
            self._t0_data_count += 1
            p3 = self._t0_p3 if self._t0_p3 != 0 else 256
            if self._t0_data_count >= p3:
                self._t0_data_count = 0
                self._t0_state = 'procedure'
            return []

        if state == 'Byte':
            self._t0_state = 'procedure'
            return []

        return []

    # ── T=1 ─────────────────────────────────────────────────────────────────
    # Frames emitted depend on display_level:
    #   TPDU : t1_block for every block (I/R/S); no t1_apdu
    #   APDU : t1_apdu when I-block chain completes (M=0); t1_block for R/S only
    # This ensures no time-overlapping frames in either mode.

    def _process_t1(self, frame, value):
        state = self._t1_state

        if state == 'NAD':
            self._t1_start = frame.start_time
            self._t1_state = 'PCB'
            return []

        if state == 'PCB':
            self._t1_pcb = value
            self._t1_state = 'LEN'
            return []

        if state == 'LEN':
            self._t1_len = value
            self._t1_data_count = 0
            self._t1_edc_count = 0
            if (self._t1_pcb & 0x80) == 0:  # I-block
                if not self._t1_apdu_active:
                    self._t1_apdu_active = True
                    self._t1_apdu_start = self._t1_start
                    self._t1_apdu_data = []
            self._t1_state = 'DATA' if value > 0 else 'EDC'
            return []

        if state == 'DATA':
            if (self._t1_pcb & 0x80) == 0 and self._t1_apdu_active:
                self._t1_apdu_data.append(value)
            self._t1_data_count += 1
            if self._t1_data_count >= self._t1_len:
                self._t1_edc_count = 0
                self._t1_state = 'EDC'
            return []

        if state == 'EDC':
            self._t1_edc_count += 1
            if self._t1_edc_count < self._t1_edc_len:
                return []

            results = []
            pcb = self._t1_pcb
            sender = self._t1_block_sender
            is_i = (pcb & 0x80) == 0
            is_r = (pcb & 0xC0) == 0x80

            if is_i:
                ns = (pcb >> 6) & 1
                m = (pcb >> 5) & 1
                if self.display_level == 'TPDU':
                    desc = f'I(NS={ns},M={m}) len={self._t1_len}'
                    results.append(AnalyzerFrame('t1_block', self._t1_start, frame.end_time,
                                                 {'description': desc, 'sender': sender}))
                if m == 0 and self._t1_apdu_active:
                    if self.display_level == 'APDU':
                        results.append(AnalyzerFrame('t1_apdu',
                                                     self._t1_apdu_start, frame.end_time,
                                                     {'description': self._build_t1_apdu_desc()}))
                    self._t1_apdu_active = False
                    self._t1_apdu_data = []

            elif is_r:
                nr = (pcb >> 4) & 1
                err = pcb & 0x03
                desc = f'R(NR={nr})' if err == 0 else f'R(NR={nr},err={err})'
                results.append(AnalyzerFrame('t1_block', self._t1_start, frame.end_time,
                                             {'description': desc, 'sender': sender}))
            else:
                # S-block — emitted in both modes
                resp = bool(pcb & 0x20)
                code = pcb & 0x1F
                names = {0x00: 'RESYNC', 0x01: 'IFS', 0x02: 'ABORT', 0x03: 'WTX'}
                desc = f'S({names.get(code, f"{code:#04x}")},{"resp" if resp else "req"})'
                if self._t1_len > 0 and self._t1_apdu_data:
                    desc += f' val={self._t1_apdu_data[0]:#04x}'
                results.append(AnalyzerFrame('t1_block', self._t1_start, frame.end_time,
                                             {'description': desc, 'sender': sender}))

            self._t1_block_sender = 'card' if sender == 'reader' else 'reader'
            self._t1_state = 'NAD'
            return results

        return []

    def _build_t1_apdu_desc(self):
        data = self._t1_apdu_data
        if len(data) == 2:
            return _sw_string(data[0], data[1])
        if len(data) >= 4:
            sw_str = _sw_string(data[-2], data[-1])
            if not (0x60 <= data[0] <= 0x6F or data[0] == 0x90):
                ins_name = _ins_name(data[1]) if len(data) > 1 else '?'
                return f'{ins_name} {sw_str}'
            return f'RSP ({len(data)} bytes) {sw_str}'
        return f'APDU ({len(data)} bytes)'
