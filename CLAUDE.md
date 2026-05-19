# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**Logic 2 (default):**
```bash
cmake -B build -DLOGIC2=ON
cmake --build build
cp build/Analyzers/libISO7816Analyzer.so /path/to/Logic2/plugins/
```

**Legacy Logic 1.x:**
```bash
cmake -B build -DLOGIC2=OFF
cmake --build build
```

**Debug build:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DLOGIC2=ON
cmake --build build
```

The SDK is fetched automatically via CMake `FetchContent` from `https://github.com/saleae/AnalyzerSDK` (master branch). No manual SDK setup needed.

Output lands in `build/Analyzers/libISO7816Analyzer.so` (Linux/macOS) or `build/Analyzers/Release/ISO7816Analyzer.dll` (Windows).

## CI/CD

Two GitHub Actions workflows:
- `.github/workflows/build.yml` — Logic 2 builds (`-DLOGIC2=ON`) for all platforms
- `.github/workflows/build_legacy.yml` — Legacy Logic 1.x builds (`-DLOGIC2=OFF`)

Both trigger on push/PR to `master` and on any tag. Tagged releases produce `analyzer.zip` / `analyzer_legacy.zip` as GitHub release assets.

## Debugging with Logic2

Find the renderer PID and attach gdb:
```bash
ps ax | grep Logic.*--type=renderer | head -n1 | cut -d " " -f1
lsof -p <PID> | grep libISO7816Analyzer.so   # verify analyzer loaded
gdb -p <PID>
(gdb) break ISO7816Analyzer::WorkerThread
```

## Architecture

This is a **Saleae Logic2 custom analyzer plugin** (shared library loaded by Logic2). It decodes ISO/IEC 7816 smart card protocol from captured waveforms.

**Signal inputs** (configured via `ISO7816AnalyzerSettings`):
- `IO` — bidirectional data line (required)
- `CLK` — clock (required)
- `RST` — reset signal (required)
- `VCC` — power (optional)

**Decode pipeline** (`WorkerThread` drives everything):

```
Raw samples → character layer → protocol layer → nodes → frames
```

1. `ISO7816Analyzer::WorkerThread` — loops on RST rising edge, reads raw CLK/IO samples, assembles 8-bit characters with parity, calls `ISO7816ProtocolChar::newData()`
2. `ISO7816ProtocolChar` — top-level protocol dispatcher; routes bytes to sub-protocols based on `ISO7816Context::mState`
3. Sub-protocols (each implements `ISO7816ProtocolLayer`):
   - `ISO7816ProtocolATR` — Answer-To-Reset parsing (TS, T0, interface bytes, historical bytes, TCK)
   - `ISO7816ProtocolPPS` — Protocol Parameter Selection negotiation
   - `ISO7816ProtocolTPDUT0` — T=0 TPDU command/response pairs; emits APDU node on completion
   - `ISO7816ProtocolTPDUT1` — T=1 TPDU blocks (I/R/S); tracks I-block chains and emits APDU node when M=0
4. Each decoded unit becomes an `ISO7816Node` (hierarchy: `NodeChar` → `NodeTPDU`/`NodePPS`/`NodeATR` → `NodeAPDU`)
5. `ISO7816Analyzer::newFrame()` registers each node with `mResults`, emitting `Frame` + `FrameV2` (Logic2)
6. `ISO7816AnalyzerResults::GenerateBubbleText()` produces zoom-aware bubble text per channel

**`ISO7816Context`** holds mutable decode state shared across all protocol layers:
- `mState` — current phase (`S_ATR`, `S_PPS`, `S_T0`, `S_T1`)
- `mISOParams` (`iso_params_t`) — negotiated F, D, convention, WI, IFSC, BWI, CWI, etc. (updated by ATR/PPS parsers, used by `AdvanceEtu()` for timing)
- `mCurrSender` — toggles between `sender_card` / `sender_reader`

**ETU timing**: `AdvanceEtu()` advances by `(F/D) * etu * 2` CLK edges, keeping IO/RST/VCC channels in sync via `SyncToSample()`.

**Convention**: Direct (LSB-first) vs. Inverse (MSB-first, bits inverted) is determined during ATR and stored in `mISOParams.convention`; `WorkerThread` applies it per-bit.

## Display / Bubble text

`GenerateBubbleText` emits multiple zoom levels per channel:

| Channel | Node level | Long | Medium | Short |
|---------|-----------|------|--------|-------|
| IO      | char      | `TS(0x3B) direct` | `TS(0x3B)` | `TS` |
| CLK     | atr/pps/tpdu | full params string | — | type name |
| RST     | apdu      | full description | — | `APDU` |

Medium and long forms for char nodes respect the user-selected `display_base` (hex/decimal/binary/ASCII).

## FrameV2 data (Logic2 data table)

`newFrame()` populates `FrameV2` keys per node level:

| Node level | Keys |
|-----------|------|
| `char`    | `value` (byte), `label` (string), `from` ("card"/"reader") |
| `tpdu` / `apdu` / `pps` / `atr` | `description` (string) |

## Key constraints

- C++11 (`CMAKE_CXX_STANDARD 11`)
- `LOGIC2` is a CMake option defaulting ON; `-DLOGIC2=OFF` builds legacy Logic 1.x (compiles, untested)
- No unit tests — verification is done by loading the `.so` into Logic2 and capturing real or simulated signals
- `ISO7816SimulationDataGenerator` generates synthetic waveforms for Logic2's built-in simulation mode
