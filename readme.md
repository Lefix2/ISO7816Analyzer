# Saleae ISO/IEC 7816 Analyzer

A custom analyzer for [Saleae Logic](https://www.saleae.com/downloads/) that fully decodes the ISO/IEC 7816-3 smart card protocol from captured waveforms.

Two editions are provided:

| Edition | Target | What it does |
|---------|--------|--------------|
| **Logic 2** (LLA + HLA) | Logic 2 | Low Level Analyzer decodes individual characters; High Level Analyzer reconstructs ATR, PPS, TPDU, and APDU frames |
| **Legacy** | Logic 1.x | Single analyzer, all levels decoded in one pass |

---

## Signals

| Signal | Required | Description |
|--------|----------|-------------|
| IO     | yes | Bidirectional data line (half-duplex) |
| CLK    | yes | Card clock |
| RST    | yes | Reset |
| VCC    | no  | Power rail (optional, used for reference only) |

---

## Logic 2 — Two-layer architecture

Logic 2 uses a two-component pipeline:

```
Captured waveform
       │
       ▼
┌──────────────────────────────┐
│  Low Level Analyzer (C++)    │  one frame per byte
│  • bit sampling & ETU timing │  fields: data, description, from, phase
│  • parity check              │
│  • ATR parsing (convention,  │
│    Fi/Di, protocol)          │
│  • PPS parsing               │
└──────────────────────────────┘
       │  char frames
       ▼
┌──────────────────────────────┐
│  High Level Analyzer (Python)│  one frame per protocol unit
│  • ATR summary frame         │
│  • PPS exchange frames       │
│  • T=0 TPDU + APDU frames    │
│  • T=1 block + APDU frames   │
└──────────────────────────────┘
```

### LLA frame fields (Logic 2 data table)

Each byte emitted by the LLA carries:

| Field | Type | Description |
|-------|------|-------------|
| `data` | byte | Raw byte value |
| `description` | string | Field name (e.g. `TS(direct)`, `TA1(Fi=372,Di=1)`, `CLA`, `INS`, `SW1`) |
| `from` | string | `"card"` or `"reader"` |
| `phase` | string | `"atr"`, `"pps"`, `"t0"`, or `"t1"` |

> **Note:** `from` and `phase` are set by the ATR/PPS parsers. For T=0/T=1 bytes the HLA determines direction from protocol position.

### HLA frame types

| Type | Format | Description |
|------|--------|-------------|
| `atr` | `ATR: <params>` | Full ATR parsed (convention, Fi/Di) |
| `pps` | `PPS [reader/card]: <params>` | One side of PPS exchange |
| `tpdu` | `TPDU [reader]: <INS> P1 P2 → SW` | Complete T=0 command/response |
| `apdu` | `APDU: <INS> <SW>` | Logical APDU (T=0 and T=1) |
| `t1_block` | `T1 [sender]: I/R/S block` | Individual T=1 block |
| `t1_apdu` | `T1 APDU: <description>` | Reconstructed APDU from I-block chain |

---

## Installation (Logic 2)

### Step 1 — Low Level Analyzer

Download `logic2_<platform>.zip` from the [Releases page](../../releases) and copy the `.so` / `.dll` for your platform into the Logic 2 custom analyzers directory:

| Platform | Path |
|----------|------|
| Linux    | `~/.config/Logic/plugins/` |
| macOS    | `~/Library/Application Support/Logic/plugins/` |
| Windows  | `%APPDATA%\Logic\plugins\` |

Restart Logic 2, then add the **ISO/IEC-7816** analyzer to your session and assign the IO, CLK, and RST channels.

### Step 2 — High Level Analyzer

Download `logic2_hla.zip` from the [Releases page](../../releases) and extract `extension.json` and `HighLevelAnalyzer.py` into a dedicated folder (e.g. `ISO7816_HLA/`). In Logic 2:

1. Open **Extensions** → **Load existing extension…**
2. Select the folder containing `extension.json`.
3. Add the **ISO7816 HLA** analyzer to your session and set its input to the **ISO/IEC-7816** LLA.

---

## Installation (Legacy Logic 1.x)

Download `analyzer_legacy.zip` from the [Releases page](../../releases). Copy the `.so` / `.dll` into the Logic custom analyzers directory and restart Logic.

---

## Features

### All editions
- **ATR** decoding: TS, T0, TA/TB/TC/TD interface bytes, historical bytes, TCK checksum
- **PPS** (Protocol Parameter Selection) negotiation
- **Direct** and **inverse** convention
- **T=0 TPDU** command/response with INS and SW1/SW2 interpretation
- **T=1 TPDU** block framing (I-block, R-block, S-block) with LRC/CRC EDC
- **APDU** reconstruction from T=0 and T=1 exchanges

### Logic 2 extras
- Logic 2 data table with per-byte `data`, `description`, `from`, `phase` fields
- HLA overlay with ATR/PPS/TPDU/APDU frames above the character stream
- Bubble text on the IO channel at multiple zoom levels (long / medium / short), respects the display base setting (hex / decimal / binary / ASCII)

---

## Build from source

### Prerequisites

- CMake ≥ 3.13
- C++11 compiler (GCC, Clang, or MSVC)
- Git — the Saleae Analyzer SDK is fetched automatically via CMake `FetchContent`

### Logic 2

```bash
cmake -B build -DLOGIC2=ON
cmake --build build
# Output: build/Analyzers/libISO7816Analyzer.so  (Linux/macOS)
#         build/Analyzers/Release/ISO7816Analyzer.dll  (Windows)
```

### Legacy Logic 1.x

```bash
cmake -B build -DLOGIC2=OFF
cmake --build build
```

### Debug build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DLOGIC2=ON
cmake --build build
```

---

## Debugging with Logic 2 (Linux/macOS)

```bash
# Find the renderer PID
ps ax | grep Logic.*--type=renderer | head -n1 | cut -d " " -f1

# Verify the analyzer is loaded
lsof -p <PID> | grep libISO7816Analyzer.so

# Attach gdb and set a breakpoint
gdb -p <PID>
(gdb) break ISO7816Analyzer::WorkerThread
```
