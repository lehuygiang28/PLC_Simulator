# PLC Simulator MCP

Native MCP control plane built into `PLCSimulator.exe`.

[中文](README.md) · **English**

## Start the Simulator

GUI (default):

```powershell
.\Bin\x64\PLCSimulator.exe
```

Headless:

```powershell
.\Bin\x64\PLCSimulator.exe --headless
```

Optional CLI flags:

| Flag | Description |
|------|-------------|
| `--mcp-port=8765` | MCP listen port (default `8765`) |
| `--mcp-token=secret` | Optional bearer token |
| `--no-mcp` | Do not start MCP |

Environment variable fallback for token:

```powershell
$env:PLC_SIM_MCP_TOKEN = "your-secret"
```

CLI flags override saved settings when provided.

## GUI Control Panel

Open **Help → MCP Control** in the simulator window.

From the dialog you can:

- Change MCP port (saved to `Config/app_config.json`)
- Generate and copy a token
- Copy the MCP URL
- Copy ready-to-paste Cursor MCP JSON
- **Start / Stop / Restart** the MCP server
- Enable or disable auto-start when the app launches

If the default port is in use, pick another port and click **Restart**.

## Cursor Configuration

Manual example:

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

Use **Copy Cursor JSON** in the MCP dialog to generate this block automatically.

Omit `headers` when no token is configured.

## Available Tools

| Category | Tools |
|----------|-------|
| Status | `get_status`, `get_logs` |
| Registers | `get_register`, `set_register`, `dump_registers`, `reset_registers` |
| Communication | `get_comm_status`, `set_comm_config`, `set_protocol`, `open_comm`, `close_comm` |
| Scripts | `list_scripts`, `read_script`, `write_script`, `run_script`, `stop_script`, `list_script_functions` |
| Platform | `get_platform_params`, `set_platform_params`, `get_platform_pose`, `move_platform` |

## Resources

- `plc-simulator://script-api` — TypeScript-style declarations for script globals

## Live Notifications

Subscribe with SSE:

```http
GET http://127.0.0.1:8765/mcp
Accept: text/event-stream
```

The server pushes MCP `notifications/message` events for communication, script, and app logs. Agents can also poll with `get_logs`.

## Security & Limits

- MCP binds to **`127.0.0.1` only** (localhost)
- Optional bearer token via UI, `--mcp-token`, or `PLC_SIM_MCP_TOKEN`
- Only **one** simulator instance may run at a time
- **Headless mode**: registers, communication, and scripts work fully; visual platform tools require GUI mode

## Typical Agent Workflow

1. Start PLC Simulator (GUI or `--headless`)
2. Configure MCP in **Help → MCP Control** (or use defaults)
3. Add the MCP URL to Cursor
4. Call `get_status` → `set_protocol` / `open_comm` → `set_register` or `run_script`
5. Watch traffic with `get_logs` or SSE notifications while the host app talks to the simulated PLC
