# Task 9 Report: Watch-expression UI, persistence, i18n, docs

**Branch:** `feat/giaang`  
**Date:** 2026-09-18

## Summary

Replaced split-range / numeric start-address controls with a single **监视地址** watch-expression field (`edit_WatchExpr`). Expressions are parsed via `WatchList::parse`, applied through `RegisterTableController::setWatches`, and persisted as `watchExpr`. Legacy `startAddr` / `secondStartAddr` / `splitView` configs migrate once through `migrateWatchExpr`. Invalid input shows `label_WatchError` without clearing the last good watch list; valid edits debounce at 400 ms before apply+save.

## Changes

| File | Change |
| --- | --- |
| `src/Gui/MainWindow/MainWindow.ui` | Removed split checkbox and second address row; added `edit_WatchExpr`, `label_WatchError` |
| `src/Gui/MainWindow/MainWindow.h` / `.cpp` | `applyWatchExpression`, `migrateWatchExpr`, debounced `textChanged`, save `watchExpr` |
| `src/Gui/MainWindow/RegisterTable/RegisterTableController.*` | Removed Task 8 start/split adapters and `initTable(int,int)` |
| `src/translations/plc_simulator_en.ts` | `监视地址:` → `Watch:`; placeholder translation |
| `README.md`, `README.en.md` | M/D watch list usage |
| `ChangeLog.txt` | 2026-09-18 entry (v1.12.1) |

## Verification

```powershell
cmake --build build --config Debug --target PLCSimulatorTests PLC_Simulator
ctest --test-dir build -C Debug --output-on-failure
```

**Result:** Build OK; `ctest` 100% pass (PLCSimulatorTests).

## Commit

```
feat: replace range grid with a free-form M/D watch expression
```
