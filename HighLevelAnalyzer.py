"""
ISO/IEC 7816 High Level Analyzer
Processes char-level frames from the ISO7816 Low Level Analyzer and reconstructs
ATR, PPS, T=0 TPDU/APDU, and T=1 block/APDU frames.

Each input char frame carries:
  - data   (U8):  raw byte value
  - sender (str): "card" or "reader"

The HLA self-manages the protocol phase state machine.
"""

from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame, ChoicesSetting

# ── lookup tables ─────────────────────────────────────────────────────────────

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


def _fn(fi):
    v = _FN_TABLE[fi] if 0 <= fi < len(_FN_TABLE) else 0
    return v if v > 0 else 372


def _dn(di):
    v = _DN_TABLE[di] if 0 <= di < len(_DN_TABLE) else 0
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


# ── ATR parser ────────────────────────────────────────────────────────────────

class AtrParser:
    """
    Byte-by-byte ATR state machine.
    Feed bytes with push(); done() returns True when all bytes consumed.
    Call description() for the formatted summary.
    """

    # States
    _S_TS = 0
    _S_T0 = 1
    _S_TA = 2
    _S_TB = 3
    _S_TC = 4
    _S_TD = 5
    _S_HIST = 6
    _S_TCK = 7
    _S_DONE = 8

    def __init__(self):
        self._state = self._S_TS
        self._td_mask = 0       # presence bits from current TDi
        self._next_mask = 0     # presence bits for next group
        self._hist_remaining = 0
        self._tck_needed = False

        # Parsed fields
        self.convention = ''    # 'direct' or 'inverse'
        self.protocols = set()  # {0, 1, 14, ...}
        self.fi = 372
        self.di = 1
        self.wi = 10            # default WI (T=0 guard time extension)
        self.ifsc = 32          # default IFSC (T=1)
        self.bwi = 4            # default BWI (T=1)
        self.cwi = 13           # default CWI (T=1)
        self.edc = 'LRC'        # default EDC (T=1)
        self._ta_index = 0      # TA1, TA2, TA3... counter
        self._tb_index = 0
        self._tc_index = 0
        self._has_t1 = False
        self._has_t0 = False
        self._group = 1         # current interface byte group number

    def push(self, value):
        if self._state == self._S_TS:
            if value == 0x3B:
                self.convention = 'direct'
            elif value in (0x3F, 0x03):
                self.convention = 'inverse'
            self._state = self._S_T0

        elif self._state == self._S_T0:
            self._hist_remaining = value & 0x0F
            self._td_mask = value & 0xF0
            self._ta_index = 1
            self._tb_index = 1
            self._tc_index = 1
            self._group = 1
            self._state = self._next_state_from_mask(self._td_mask, 'TA')

        elif self._state == self._S_TA:
            self._process_ta(value)
            self._state = self._next_after('TA')

        elif self._state == self._S_TB:
            self._process_tb(value)
            self._state = self._next_after('TB')

        elif self._state == self._S_TC:
            self._process_tc(value)
            self._state = self._next_after('TC')

        elif self._state == self._S_TD:
            prot = value & 0x0F
            if prot == 0:
                self._has_t0 = True
                self.protocols.add(0)
            elif prot == 1:
                self._has_t1 = True
                self.protocols.add(1)
            else:
                self.protocols.add(prot)
            self._tck_needed = any(p != 0 for p in self.protocols)
            self._group += 1
            self._ta_index += 1
            self._tb_index += 1
            self._tc_index += 1
            self._td_mask = value & 0xF0
            self._state = self._next_state_from_mask(self._td_mask, 'TA')

        elif self._state == self._S_HIST:
            self._hist_remaining -= 1
            if self._hist_remaining <= 0:
                self._state = self._S_TCK if self._tck_needed else self._S_DONE

        elif self._state in (self._S_TCK, self._S_DONE):
            self._state = self._S_DONE

    def done(self):
        return self._state == self._S_DONE

    def _next_state_from_mask(self, mask, which):
        order = ['TA', 'TB', 'TC', 'TD']
        bits  = [0x10, 0x20, 0x40, 0x80]
        states = [self._S_TA, self._S_TB, self._S_TC, self._S_TD]
        idx = order.index(which)
        for i in range(idx, 4):
            if mask & bits[i]:
                return states[i]
        return self._S_HIST if self._hist_remaining > 0 else (
               self._S_TCK if self._tck_needed else self._S_DONE)

    def _next_after(self, which):
        return self._next_state_from_mask(self._td_mask, {
            'TA': 'TB', 'TB': 'TC', 'TC': 'TD', 'TD': 'TA'
        }[which])

    def _process_ta(self, value):
        if self._ta_index == 1:
            self.fi = _fn((value >> 4) & 0x0F)
            self.di = _dn(value & 0x0F)
        elif self._ta_index >= 3 and self._has_t1:
            self.ifsc = value

    def _process_tb(self, value):
        if self._tb_index >= 3 and self._has_t1:
            self.bwi = (value >> 4) & 0x0F
            self.cwi = value & 0x0F

    def _process_tc(self, value):
        if self._tc_index == 1:
            self.wi = value if value != 0 else 10
        elif self._tc_index >= 3 and self._has_t1:
            self.edc = 'CRC' if (value & 0x01) else 'LRC'

    def description(self):
        if self._has_t1:
            prot = 'T=1'
        elif self._has_t0 or not self.protocols:
            prot = 'T=0'
        else:
            prot = 'T=' + '+'.join(str(p) for p in sorted(self.protocols))

        etu = self.fi // self.di if self.di > 0 else self.fi
        parts = [prot, self.convention, f'Fi={self.fi}', f'Di={self.di}', f'ETU={etu}']

        if self._has_t0:
            parts.append(f'WI={self.wi}')
        if self._has_t1:
            parts += [f'IFSC={self.ifsc}', f'BWI={self.bwi}', f'CWI={self.cwi}', self.edc]

        return ' '.join(p for p in parts if p)


# ── HLA class ─────────────────────────────────────────────────────────────────

class Hla(HighLevelAnalyzer):

    display_level = ChoicesSetting(choices=('APDU', 'TPDU'))

    result_types = {
        'atr':  {'format': 'ATR: {{data.description}}'},
        'pps':  {'format': 'PPS: {{data.description}}'},
        'tpdu': {'format': 'TPDU: {{data.description}}'},
        'apdu': {'format': 'APDU: {{data.description}}'},
    }

    def __init__(self):
        self._reset_session()

    def _reset_session(self):
        # Phase: 'atr', 'pps', 't0', 't1'
        self._phase = 'atr'

        # ATR
        self._atr = AtrParser()
        self._atr_start = None
        self._atr_end = None

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
        self._t0_header = []
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
        self._t1_edc_len = 1
        self._t1_edc_count = 0
        self._t1_apdu_active = False
        self._t1_apdu_start = None
        self._t1_apdu_data = []

    # ── ATR ──────────────────────────────────────────────────────────────────

    def _process_atr(self, frame, value):
        if self._atr_start is None:
            self._atr_start = frame.start_time

        self._atr.push(value)
        self._atr_end = frame.end_time

        if not self._atr.done():
            return None

        desc = self._atr.description()
        f = AnalyzerFrame('atr', self._atr_start, self._atr_end, {'description': desc, 'sender': 'card'})

        # Determine next phase from ATR
        if self._atr._has_t1:
            self._t1_edc_len = 2 if self._atr.edc == 'CRC' else 1
        # Phase after ATR is PPS (reader may or may not send; first byte 0xFF = PPSS)
        # We optimistically move to PPS; if the next byte is not 0xFF we switch to T0/T1
        self._phase = 'pps_or_data'
        self._pps_state = 'PPSS'
        self._pps_exchange = 0
        return f

    # ── PPS ──────────────────────────────────────────────────────────────────

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
            if value & 0x10:
                self._pps_state = 'PPS1'
            elif value & 0x20:
                self._pps_state = 'PPS2'
            elif value & 0x40:
                self._pps_state = 'PPS3'
            else:
                self._pps_state = 'PCK'
            return []

        if self._pps_state == 'PPS1':
            self._pps_fi = _fn((value >> 4) & 0x0F)
            self._pps_di = _dn(value & 0x0F)
            if self._pps_pps0 & 0x20:
                self._pps_state = 'PPS2'
            elif self._pps_pps0 & 0x40:
                self._pps_state = 'PPS3'
            else:
                self._pps_state = 'PCK'
            return []

        if self._pps_state == 'PPS2':
            self._pps_state = 'PPS3' if self._pps_pps0 & 0x40 else 'PCK'
            return []

        if self._pps_state == 'PPS3':
            self._pps_state = 'PCK'
            return []

        if self._pps_state == 'PCK':
            prot = self._pps_pps0 & 0x0F
            direction = 'reader' if self._pps_exchange == 0 else 'card'
            parts = [f'T={prot}']
            if self._pps_fi is not None:
                parts.append(f'Fi={self._pps_fi}')
            if self._pps_di is not None:
                parts.append(f'Di={self._pps_di}')
                if self._pps_di > 0 and self._pps_fi:
                    parts.append(f'ETU={self._pps_fi // self._pps_di}')
            f = AnalyzerFrame('pps', self._pps_start, frame.end_time,
                              {'description': ' '.join(parts), 'sender': direction})
            self._pps_exchange += 1
            self._pps_state = 'PPSS'
            if self._pps_exchange >= 2:
                # Both sides exchanged; move to data phase
                if self._atr._has_t1:
                    self._phase = 't1'
                else:
                    self._phase = 't0'
            return [f]

        return []

    # ── public entry point ────────────────────────────────────────────────────

    def decode(self, frame: AnalyzerFrame):
        if frame.type != 'char':
            return None

        raw = frame.data.get('data', 0)
        value = raw[0] if isinstance(raw, (bytes, bytearray)) else int(raw)
        sender = frame.data.get('sender', 'reader')

        # New ATR detection: card sends TS byte while in data phase
        if self._phase in ('t0', 't1') and sender == 'card' and value in (0x3B, 0x3F, 0x03):
            self._reset_session()

        results = []

        if self._phase == 'atr':
            f = self._process_atr(frame, value)
            if f:
                results.append(f)

        elif self._phase == 'pps_or_data':
            if value == 0xFF:
                self._phase = 'pps'
                results.extend(self._process_pps(frame, value))
            else:
                # No PPS — go straight to data phase
                if self._atr._has_t1:
                    self._phase = 't1'
                else:
                    self._phase = 't0'
                if self._phase == 't0':
                    results.extend(self._process_t0(frame, value))
                else:
                    results.extend(self._process_t1(frame, value))

        elif self._phase == 'pps':
            results.extend(self._process_pps(frame, value))

        elif self._phase == 't0':
            results.extend(self._process_t0(frame, value))

        elif self._phase == 't1':
            results.extend(self._process_t1(frame, value))

        return results if results else None

    # ── T=0 ──────────────────────────────────────────────────────────────────

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
                pass
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
                return [AnalyzerFrame('tpdu', start, frame.end_time, {'description': desc, 'sender': 'reader', 'protocol': 'T0'})]
            else:
                return [AnalyzerFrame('apdu', start, frame.end_time,
                                      {'description': f'{ins_name} {sw_str}', 'sender': 'reader', 'protocol': 'T0'})]

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

    # ── T=1 ──────────────────────────────────────────────────────────────────
    # TPDU mode : t1_block for every block (I/R/S)
    # APDU mode : ZERO t1_block frames — only t1_apdu when I-block chain ends (M=0)

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
                m  = (pcb >> 5) & 1
                if self.display_level == 'TPDU':
                    desc = f'I(NS={ns},M={m}) len={self._t1_len}'
                    results.append(AnalyzerFrame('tpdu', self._t1_start, frame.end_time,
                                                 {'description': desc, 'sender': sender, 'protocol': 'T1'}))
                if m == 0 and self._t1_apdu_active:
                    if self.display_level == 'APDU':
                        results.append(AnalyzerFrame('apdu',
                                                     self._t1_apdu_start, frame.end_time,
                                                     {'description': self._build_t1_apdu_desc(),
                                                      'sender': sender, 'protocol': 'T1'}))
                    self._t1_apdu_active = False
                    self._t1_apdu_data = []

            elif is_r:
                if self.display_level == 'TPDU':
                    nr  = (pcb >> 4) & 1
                    err = pcb & 0x03
                    desc = f'R(NR={nr})' if err == 0 else f'R(NR={nr},err={err})'
                    results.append(AnalyzerFrame('tpdu', self._t1_start, frame.end_time,
                                                 {'description': desc, 'sender': sender, 'protocol': 'T1'}))

            else:  # S-block — emitted in TPDU mode only
                if self.display_level == 'TPDU':
                    resp = bool(pcb & 0x20)
                    code = pcb & 0x1F
                    names = {0x00: 'RESYNC', 0x01: 'IFS', 0x02: 'ABORT', 0x03: 'WTX'}
                    desc = f'S({names.get(code, f"{code:#04x}")},{"resp" if resp else "req"})'
                    if self._t1_len > 0 and self._t1_apdu_data:
                        desc += f' val={self._t1_apdu_data[0]:#04x}'
                    results.append(AnalyzerFrame('tpdu', self._t1_start, frame.end_time,
                                                 {'description': desc, 'sender': sender, 'protocol': 'T1'}))

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
