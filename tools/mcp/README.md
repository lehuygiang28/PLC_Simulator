# PLC Simulator MCP

内置在 `PLCSimulator.exe` 中的原生 MCP 控制面。

**中文** · [English](README.en.md)

## 启动模拟器

GUI（默认）：

```powershell
.\Bin\x64\PLCSimulator.exe
```

无界面模式：

```powershell
.\Bin\x64\PLCSimulator.exe --headless
```

可选命令行参数：

| 参数 | 说明 |
|------|------|
| `--mcp-port=8765` | MCP 监听端口（默认 `8765`） |
| `--mcp-token=secret` | 可选 Bearer Token |
| `--no-mcp` | 不启动 MCP |

环境变量 Token 回退：

```powershell
$env:PLC_SIM_MCP_TOKEN = "your-secret"
```

命令行参数会覆盖已保存的配置。

## GUI 控制面板

在模拟器窗口打开 **帮助 → MCP 控制**。

可在对话框中：

- 修改 MCP 端口（保存到 `Config/app_config.json`）
- 生成 / 复制 Token
- 复制 MCP URL
- 复制可直接粘贴到 Cursor 的 MCP JSON
- **启动 / 停止 / 重启** MCP 服务器
- 开启 / 关闭程序启动时自动开启 MCP

若默认端口被占用，请更换端口后点击 **重启**。

## Cursor 配置

手动示例：

```json
{
  "mcpServers": {
    "plc-simulator": {
      "url": "http://127.0.0.1:8765/mcp",
      "headers": {
        "Authorization": "Bearer your-secret"
      }
    }
  }
}
```

在 MCP 对话框中点击 **复制 Cursor JSON** 可自动生成上述配置。

未配置 Token 时可省略 `headers` 字段。

## 可用 Tools

| 类别 | Tools |
|------|-------|
| 状态 | `get_status`, `get_logs` |
| 寄存器 | `get_register`, `set_register`, `dump_registers`, `reset_registers` |
| 通信 | `get_comm_status`, `set_comm_config`, `set_protocol`, `open_comm`, `close_comm` |
| 脚本 | `list_scripts`, `read_script`, `write_script`, `run_script`, `stop_script`, `list_script_functions` |
| 平台 | `get_platform_params`, `set_platform_params`, `get_platform_pose`, `move_platform` |

## Resources

- `plc-simulator://script-api` — 脚本全局 API 的 TypeScript 风格声明

## 实时通知

通过 SSE 订阅：

```http
GET http://127.0.0.1:8765/mcp
Accept: text/event-stream
```

服务器会推送通信、脚本与应用日志的 MCP `notifications/message` 事件。Agent 也可通过 `get_logs` 轮询。

## 安全与限制

- MCP 仅绑定 **`127.0.0.1`**（本机）
- Token 可通过 UI、`--mcp-token` 或 `PLC_SIM_MCP_TOKEN` 配置（可选）
- 同一时间只能运行 **一个** 模拟器实例
- **无界面模式**：寄存器、通信、脚本功能完整；可视化平台相关 Tool 需要 GUI 模式

## 典型 Agent 工作流

1. 启动 PLC Simulator（GUI 或 `--headless`）
2. 在 **帮助 → MCP 控制** 中配置 MCP（或使用默认值）
3. 将 MCP URL 添加到 Cursor
4. 调用 `get_status` → `set_protocol` / `open_comm` → `set_register` 或 `run_script`
5. 在上位机与模拟 PLC 通信时，通过 `get_logs` 或 SSE 观察报文
