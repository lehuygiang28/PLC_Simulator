# Task 6 Report: Keyence MR bits

**Branch:** `feat/giaang`  
**Date:** 2026-09-18

## Summary

Implemented Keyence PC-Link `MR` relay bit read/write alongside unchanged `DM.H` word access. MR commands map to simulator `M` bits via `PlcAccess` (`DeviceKind::M`, `PlcUnit::Bit`).

## Changes

| File | Change |
| --- | --- |
| `src/Comm/Protocol/CommProtocolKeyencePCLink.h` | Private `parsePcLink()` shared by read/write |
| `src/Comm/Protocol/CommProtocolKeyencePCLink.cpp` | `RDS`/`WRS` for `DM` + `.H` (hex words) and `MR` (0/1 tokens); bit read replies as space-separated `0`/`1` |
| `tests/tst_KeyenceDevices.h` / `.cpp` | Four tests: DM unchanged, MR read/write parse, MR pack |
| `tests/main.cpp` | Register `tst_KeyenceDevices` suite |
| `tests/CMakeLists.txt` | Add Keyence test sources |

## Protocol rules (this task)

- `RDS DM#####.H ####` → D words (unchanged)
- `RDS MR##### ####` → M bits, count = 4-digit decimal
- `WRS MR##### #### 0 1 …` → write bits as single-space `0`/`1` tokens
- Read reply for MR: `1 0 1` (no trailing space)

## Verification

```powershell
cmake --build build --config Debug --target PLCSimulatorTests PLC_Simulator
ctest --test-dir build -C Debug --output-on-failure
.\build\Temp\PLC_Simulator\bin\Debug\PLCSimulatorTests.exe tst_KeyenceDevices
```

**Result:** Build OK; `ctest` 100% pass; all `tst_KeyenceDevices::*` PASS.

## Commit

```
feat: map Keyence MR bit devices onto simulator M bits
```
