# PLC Simulator

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE) [![Release](https://img.shields.io/github/v/release/LittleMiaoo/PLC_Simulator)](https://github.com/LittleMiaoo/PLC_Simulator/releases)

> 工业通信协议测试与仿真工具（PLC 通信模拟器）· 基于 Qt6 + CMake

**中文** · [English](README.en.md)

## 简介

PLC Simulator 以**服务端**身份模拟 PLC（可编程逻辑控制器），让上位机 / 客户端在**没有真实 PLC** 时也能联调、测试通信协议。

典型用途：
- 无需真实设备即可验证协议收发与寄存器读写逻辑；
- 通过内置 Lua 脚本引擎编写自动化测试；
- 借助可视化模拟平台测试运动控制类指令。

## 核心特性

- **协议仿真**：基恩士 PC-Link 上位链路协议、三菱 Q 系列 MC 二进制协议
- **图形界面**：协议选择、连接配置、寄存器实时查看 / 编辑、通信日志（ASCII / HEX 可切换）
- **网络通信**：TCP/IP（作为服务端监听）
- **Lua 脚本引擎**：编写自定义自动化测试脚本，支持语法高亮与代码提示
- **TypeScript 脚本**：可用 TypeScript 编写脚本，运行前自动编译为 Lua（需 Node.js）；详见 [docs/SCRIPTING.md](docs/SCRIPTING.md)
- **MCP 控制面**：内置 MCP 服务器，供 Cursor 等 AI 工具通过 localhost 控制寄存器/通信/脚本/平台；详见 [tools/mcp/README.md](tools/mcp/README.md)（[English](tools/mcp/README.en.md)）
- **可视化模拟平台**：图像加载与模拟运动控制，用于测试设备控制指令
- **深色 / 浅色主题**

## 界面预览

> 🚧 界面仍在开发中，待 UI 稳定后补充截图

<!-- 待补：主界面截图 -->
<!-- 待补：脚本编辑器截图 -->
<!-- 待补：模拟平台截图 -->

## 从源码构建

### 环境要求

- CMake 3.16 或更高版本
- Qt6（需含 Widgets、Network、SerialPort 等组件）
- 支持 C++17 的编译器（推荐 Visual Studio 2022 / MSVC）

> Lua 已内置于 `thirdparty/Lua`，无需单独安装。  
> 使用 **TypeScript** 脚本时需安装 [Node.js 18+](https://nodejs.org)，并在 `tools/tstl-transpile` 目录执行一次 `npm install`（见 [docs/SCRIPTING.md](docs/SCRIPTING.md)）。

### 构建

```bash
git clone https://github.com/LittleMiaoo/PLC_Simulator.git
cd PLC_Simulator

mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

构建完成后，Release 版可执行文件 `PLCSimulator.exe` 会输出到项目根目录的 `Bin/x64/`。

## 基本使用

1. 选择通信协议（基恩士 PC-Link / 三菱 MC）；
2. 配置 IP 与端口，点击「打开链接」——程序作为服务端开始监听；
3. 在寄存器表查看 / 编辑数据，在通信日志观察收发帧（可切换 ASCII / HEX）；
4. （可选）编写 Lua 或 TypeScript 脚本实现自动化测试逻辑（TS 说明见 [docs/SCRIPTING.md](docs/SCRIPTING.md)）；
5. （可选）打开模拟平台，测试运动控制相关指令。
6. （可选）在 **帮助 → MCP 控制** 中配置端口/token，复制 URL 或 Cursor JSON，启动 MCP 供 AI agent 联调。

## 路线图

- [ ] 串口通信
- [ ] 支持更多 PLC 通信协议

## 许可证

本项目采用 **MIT License**，详见 [LICENSE](LICENSE)。

本项目使用 Qt6（LGPL v3）与 Lua（MIT），完整第三方许可信息见 [THIRD_PARTY_LICENSES.txt](THIRD_PARTY_LICENSES.txt)。

## 免责声明

本工具用于 PLC 协议仿真与测试，所模拟的协议可能与真实 PLC 存在差异，不保证完全一致。软件的担保与责任条款以 [LICENSE](LICENSE) 为准。
