# Task 5 Report: PlcAccess + Mitsubishi D/M + workflow dispatch

**Branch:** `feat/giaang`  
**Status:** DONE

## Scope

- Added `PlcAccess` (`DeviceKind`, `PlcUnit`, start/count, word/bit payloads).
- Switched `CommProtocolBase` read/write virtuals to `PlcAccess`.
- Mitsubishi MC 3E binary: device codes `A8` (D) / `90` (M), subcommands `0000` (word) / `0001` (bit); M word read/write rejects unaligned start.
- Keyence PC-Link: signatures only; DM `.H` word path unchanged (MR deferred to Task 6).
- `MainWorkflow::ProcessRequest` dispatches to `RegisterStore::bits/words/setBits/setWords`.
- Qt tests: `tst_MitsubishiDevices` (6 cases).

## TDD evidence

### Step 1 — tests added before implementation

Created `tests/tst_MitsubishiDevices.h/.cpp` with `mcRequest()` helper and six slots per plan (`read_d_word_still_works`, `read_m_bit`, `write_m_bit_payload`, `pack_bit_read_pads_odd_count`, `read_d_bit_unit`, `m_word_rejects_unaligned`).

### Step 2 — expected compile failure (not re-run after green build)

Prior to `PlcAccess.h` and signature changes, build would fail on missing `PlcAccess` / old `long&` overloads (per plan).

### Step 3–5 — implementation + green build

```text
cmake --build build --config Debug --target PLCSimulatorTests PLC_Simulator
# Exit code: 0
```

```text
ctest --test-dir build -C Debug --output-on-failure
# 1/1 Test #1: PLCSimulatorTests ................   Passed    0.46 sec
# 100% tests passed out of 1
```

Mitsubishi suite runs inside aggregated `PLCSimulatorTests` (DeviceAddress, WatchList, RegisterStoreBits, MitsubishiDevices).

### Assertions covered

| Test | Behavior verified |
| --- | --- |
| `read_d_word_still_works` | `0401/0000`, dev `A8`, D word @100 |
| `read_m_bit` | `0401/0001`, dev `90`, M bits @1500 ×2 |
| `write_m_bit_payload` | `1401/0001`, bit payload `01 00` |
| `pack_bit_read_pads_odd_count` | Reply hex contains `0100` (value + pad) |
| `read_d_bit_unit` | D device, bit unit, 16 points @2024 |
| `m_word_rejects_unaligned` | M word @1500 rejected |

## Files touched

- `src/Comm/Protocol/PlcAccess.h` (new)
- `src/Comm/Protocol/CommProtocolBase.h`
- `src/Comm/Protocol/CommProtocolMitsubishiQBinary.h/.cpp`
- `src/Comm/Protocol/CommProtocolKeyencePCLink.h/.cpp`
- `src/MainFlow/MainWorkflow.cpp`
- `src/CMakeLists.txt`
- `tests/tst_MitsubishiDevices.h/.cpp`, `tests/main.cpp`, `tests/CMakeLists.txt`

## Commit

Message: `feat: handle Mitsubishi M/D bit and word units in the simulator`

SHA: `a7864745844a7b1cce1710c5245c2c4f699c9550`
