# Saleae ISO/IEC 7816 Analyzer

A custom analyzer plugin for [Saleae Logic 2](https://www.saleae.com/downloads/) that decodes ISO/IEC 7816-3 smart card protocol from captured waveforms.

## Features

- Decodes **ATR** (Answer To Reset) with full interface byte parsing (TA/TB/TC/TD, historical bytes, TCK)
- Decodes **PPS** (Protocol Parameter Selection) negotiation
- Decodes **T=0 TPDU** command/response pairs with INS and SW1/SW2 interpretation
- Decodes **T=1 TPDU** blocks (I-block, R-block, S-block)
- Reconstructs **APDU** layer from T=0 and T=1 exchanges
- Supports **direct** and **inverse** convention
- Bubble text with multiple zoom levels; respects Logic 2 display base (hex/decimal/binary/ASCII)
- Logic 2 data table integration (value, label, sender per byte)

## Signals

| Signal | Required | Description |
|--------|----------|-------------|
| IO     | yes      | Bidirectional data line |
| CLK    | yes      | Clock |
| RST    | yes      | Reset |
| VCC    | no       | Power (optional, for reference) |

## Installation

Download the latest release from the [Releases page](../../releases) and extract the `.zip`. Copy the `.so`/`.dll` file for your platform into the Logic 2 custom analyzers directory:

| Platform | Path |
|----------|------|
| Linux    | `~/.config/Logic/plugins/` |
| macOS    | `~/Library/Application Support/Logic/plugins/` |
| Windows  | `%APPDATA%\Logic\plugins\` |

Then reload Logic 2.

## Build from source

### Prerequisites

- CMake ≥ 3.13
- C++11 compiler (GCC, Clang, MSVC)
- Git (CMake fetches the Saleae SDK automatically)

### Logic 2

```bash
cmake -B build -DLOGIC2=ON
cmake --build build
```

Output: `build/Analyzers/libISO7816Analyzer.so` (Linux/macOS) or `build/Analyzers/Release/ISO7816Analyzer.dll` (Windows).

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

## Debugging with Logic 2 (Linux/macOS)

```bash
# Find the renderer PID
ps ax | grep Logic.*--type=renderer | head -n1 | cut -d " " -f1

# Verify the analyzer is loaded
lsof -p <PID> | grep libISO7816Analyzer.so

# Attach gdb
gdb -p <PID>
(gdb) break ISO7816Analyzer::WorkerThread
```
