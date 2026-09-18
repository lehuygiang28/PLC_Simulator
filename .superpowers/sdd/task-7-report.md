# Task 7 Report: MCP / ControlService bit addresses

## Summary

Exposed M bits and D.n bits through MCP register tools (`get_register`, `set_register`, `dump_registers`) via shared `DeviceAddress` parsing and a new `parseValueView` helper.

## Changes

| Area | Detail |
|------|--------|
| `DeviceAddress` | Added `ValueView` enum and `parseValueView()` (auto: bit for M/D.n, int16 for D words). |
| `ControlService` | `getRegister` / `setRegister` / `dumpRegisters` use `DeviceAddress` + `parseValueView`; bit paths call `RegisterStore::GetBit` / `SetBit` / `bits`. Platform axis helpers still use D-word-only `parseRegisterAddress`. |
| `McpHttpServer` | Updated tool descriptions for M, D.n, and `type=bit`. |
| Docs | `tools/mcp/README.md` and `README.en.md` address line for D/M/D.n. |
| Tests | `tst_DeviceAddress`: `value_view_defaults_bit_for_m`, `value_view_rejects_int16_on_m`. |

## Verification

```powershell
cmake --build build --config Debug --target PLCSimulatorTests PLC_Simulator
ctest --test-dir build -C Debug --output-on-failure
```

- **PLCSimulatorTests**: PASS (includes new `parseValueView` tests).
- **PLC_Simulator**: Links successfully (`PLCSimulatord.exe`).

## Commit

```
feat: expose M and D.n bit access through MCP register tools
```
