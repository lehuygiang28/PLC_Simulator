# Task 8 Report: Watch-list register table model

**Branch:** `feat/giaang`  
**Date:** 2026-09-18

## Summary

Rewrote `RegisterTableModel` and `RegisterTableController` for a 2-column watch list driven by `QVector<DeviceAddress>`. Bit rows display and edit `0`/`1`; D-word rows keep existing type formatting and validation. Temporary `setStartAddr` adapter maps `D{start}..D{start+99}` so `MainWindow` still compiles before Task 9.

## Changes

| File | Change |
| --- | --- |
| `src/Gui/MainWindow/RegisterTable/RegisterTableModel.h` / `.cpp` | `setWatches`, 2 columns, per-row bit/D-word read/write, flash/editing preserved |
| `src/Gui/MainWindow/RegisterTable/RegisterTableController.h` / `.cpp` | `setWatches`, `initTable()` without viewport grid; legacy start/split adapters |
| `tests/tst_RegisterTableModel.h` / `.cpp` | Three model tests |
| `tests/main.cpp` | `QApplication` for `ThemeManager` |
| `tests/CMakeLists.txt` | Link Gui/Widgets, model + ThemeManager sources, `dwmapi` on WIN32 |

## Verification

```powershell
cmake --build build --config Debug --target PLCSimulatorTests PLC_Simulator
ctest --test-dir build -C Debug --output-on-failure
.\build\Temp\PLC_Simulator\bin\Debug\PLCSimulatorTests.exe tst_RegisterTableModel
```

**Result:** Build OK; `ctest` 100% pass; all `tst_RegisterTableModel::*` PASS.

## Commit

```
feat: render a discrete D/M watch list in the register table
```
