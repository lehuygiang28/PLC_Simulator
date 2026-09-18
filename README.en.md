# PLC Simulator

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE) [![Release](https://img.shields.io/github/v/release/LittleMiaoo/PLC_Simulator)](https://github.com/LittleMiaoo/PLC_Simulator/releases)

> Industrial communication protocol testing and simulation tool (PLC communication simulator) · Built with Qt6 + CMake

[中文](README.md) · **English**

## Overview

PLC Simulator acts as a **server-side** PLC (programmable logic controller) emulator so host applications and clients can develop and test communication protocols **without real hardware**.

Typical use cases:

- Verify protocol send/receive and register read/write logic without physical devices
- Write automated tests with the built-in Lua scripting engine
- Exercise motion-control style commands through the visual simulation platform

## Key Features

- **Protocol simulation**: Keyence PC-Link host link protocol, Mitsubishi Q-series MC binary protocol
- **Graphical UI**: Protocol selection, connection setup, free-form D-word / M-bit watch list (e.g. `M1500-1559, M1564, D2024`), live register view/edit, communication log (ASCII / HEX toggle)
- **Network communication**: TCP/IP server mode
- **Lua scripting**: Custom automation scripts with syntax highlighting and completion
- **TypeScript scripting**: Write scripts in TypeScript, auto-transpiled to Lua before execution (requires Node.js); see [docs/SCRIPTING.md](docs/SCRIPTING.md)
- **MCP control plane**: Built-in MCP server for Cursor and other AI tools to control registers, communication, scripts, and platform over localhost; see [tools/mcp/README.en.md](tools/mcp/README.en.md)
- **Visual simulation platform**: Image loading and simulated motion control for device command testing
- **Dark / light themes**

## UI Preview

> 🚧 UI is still evolving; screenshots will be added once the layout stabilizes

<!-- TODO: main window screenshot -->
<!-- TODO: script editor screenshot -->
<!-- TODO: simulation platform screenshot -->

## Build from Source

### Requirements

- CMake 3.16 or later
- Qt6 (Widgets, Network, SerialPort, etc.)
- C++17 compiler (Visual Studio 2022 / MSVC recommended)

> Lua is bundled under `thirdparty/Lua` — no separate install needed.  
> For **TypeScript** scripts, install [Node.js 18+](https://nodejs.org) and run `npm install` once in `tools/tstl-transpile` (see [docs/SCRIPTING.md](docs/SCRIPTING.md)).

### Build Steps

```bash
git clone https://github.com/LittleMiaoo/PLC_Simulator.git
cd PLC_Simulator

mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

After a successful Release build, `PLCSimulator.exe` is copied to `Bin/x64/` at the project root.

## Basic Usage

1. Select a communication protocol (Keyence PC-Link / Mitsubishi MC)
2. Configure IP and port, then click **Open connection** — the app listens as a TCP server
3. Enter devices to watch in the **Watch** field (comma-separated; ranges on the same device, e.g. `M1500-1559, M1564, D2024`); view and edit values in the register table; watch frames in the communication log (ASCII / HEX)
4. *(Optional)* Write Lua or TypeScript automation scripts (TypeScript details: [docs/SCRIPTING.md](docs/SCRIPTING.md))
5. *(Optional)* Open the simulation platform to test motion-control commands
6. *(Optional)* Use **Help → MCP Control** to set port/token, copy URL or Cursor JSON, and start MCP for AI agent integration

## Roadmap

- [ ] Serial port communication
- [ ] Additional PLC communication protocols

## License

This project is released under the **MIT License**. See [LICENSE](LICENSE).

It uses Qt6 (LGPL v3) and Lua (MIT). Full third-party license notices: [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt).

## Disclaimer

This tool is intended for PLC protocol simulation and testing. Emulated behavior may differ from real PLCs and is not guaranteed to be identical. Warranty and liability terms are defined in [LICENSE](LICENSE).
