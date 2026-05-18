# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

**Linux (preferred):**
```bash
mkdir build && cd build
cmake ../
make
cp Analyzers/libISO7816Analyzer.so /path/to/Logic2/plugins/
```

**Debug build:**
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ../
make
```

The SDK is fetched automatically via CMake `FetchContent` from `https://github.com/saleae/AnalyzerSDK` (alpha branch). No manual SDK setup needed.

Output lands in `build/Analyzers/libISO7816Analyzer.so`.

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
   - `ISO7816ProtocolTPDUT0` — T=0 TPDU command/response pairs
4. Each decoded unit becomes an `ISO7816Node` (hierarchy: `NodeChar` → `NodeTPDU`/`NodePPS`/`NodeATR` → `NodeAPDU`)
5. `ISO7816Analyzer::newFrame()` registers each node with `mResults`, emitting both `Frame` (legacy) and `FrameV2` (Logic2, guarded by `#ifdef LOGIC2`)
6. `ISO7816AnalyzerResults` generates bubble text and tabular output from the node tree via `GetNodeByFrameId()`

**`ISO7816Context`** holds mutable decode state shared across all protocol layers:
- `mState` — current phase (`S_ATR`, `S_PPS`, `S_T0`, `S_T1`)
- `mISOParams` (`iso_params_t`) — negotiated F, D, convention, WI, etc. (updated by ATR/PPS parsers, used by `AdvanceEtu()` for timing)
- `mCurrSender` — toggles between `sender_card` / `sender_reader`

**ETU timing**: `AdvanceEtu()` advances by `(F/D) * etu * 2` CLK edges, keeping IO/RST/VCC channels in sync via `SyncToSample()`.

**Convention**: Direct (LSB-first) vs. Inverse (MSB-first, bits inverted) is determined during ATR and stored in `mISOParams.convention`; `WorkerThread` applies it per-bit.

## Key constraints

- C++11 (`CMAKE_CXX_STANDARD 11`)
- `LOGIC2` preprocessor macro is always defined (see `CMakeLists.txt`); legacy Logic 1.x paths still compile but are untested
- No unit tests — verification is done by loading the `.so` into Logic2 and capturing real or simulated signals
- `ISO7816SimulationDataGenerator` generates synthetic waveforms for Logic2's built-in simulation mode
