# DevTools MCP Integration

## Overview

DevDash now includes an MCP (Model Context Protocol) server that provides Claude Code with UI visibility and telemetry access through HTTP endpoints.

## Architecture

```
Claude Code
    ↓ (MCP protocol via stdio)
devdash-mcp (Python bridge)
    ↓ (HTTP: localhost:18080)
DevToolsServer (C++ embedded in DevDash)
    ↓
DataBroker (telemetry data)
QQuickWindow (screenshots)
```

## Components

### 1. DevToolsServer (C++)

Located in `src/core/devtools/`, this HTTP server is embedded directly in the DevDash application and automatically starts on port 18080.

**HTTP Endpoints:**

- `GET /api/state` - Current telemetry values (JSON)
- `GET /api/warnings` - Active warnings and critical alerts (JSON)
- `GET /api/screenshot?window=<name>` - PNG screenshot of specified window
- `GET /api/windows` - List of registered windows (JSON)

**Integration:** Automatically started in `main.cpp` when DevDash runs.

### 2. devdash-mcp (Python MCP Server)

Located in `tools/mcp/`, this Python package bridges MCP protocol to HTTP requests.

**MCP Tools:**

- `devdash_get_state` - Get current telemetry
- `devdash_get_warnings` - Get active warnings
- `devdash_screenshot` - Capture window screenshot
- `devdash_list_windows` - List available windows

**Installation:**

```bash
cd tools/mcp
pip install -e .
```

### 3. Claude Code Configuration

The `.claude/mcp.json` file configures Claude Code to use the MCP server:

```json
{
  "mcpServers": {
    "devdash": {
      "command": "python",
      "args": ["-m", "devdash_mcp"],
      "cwd": "/home/dev/Projects/devdash/tools/mcp"
    }
  }
}
```

## Usage

### Starting DevDash

The DevToolsServer starts automatically:

```bash
./scripts/run-with-mock idle
```

You'll see:
```
DevToolsServer: Registered window "cluster"
DevToolsServer: Registered window "headunit"
DevToolsServer: Listening on http://127.0.0.1: 18080
```

### Testing HTTP API Directly

```bash
# Get telemetry state
curl http://127.0.0.1:18080/api/state

# List windows
curl http://127.0.0.1:18080/api/windows

# Get cluster screenshot (save to file)
curl http://127.0.0.1:18080/api/screenshot?window=cluster > cluster.png

# Get warnings
curl http://127.0.0.1:18080/api/warnings
```

### Using MCP Tools in Claude Code

Once configured, you can ask Claude Code questions like:

```
"What's the current RPM?"
```

Claude will use `devdash_get_state` to query telemetry and respond with current values.

```
"Show me a screenshot of the cluster"
```

Claude will use `devdash_screenshot` with `window="cluster"` to capture and display the UI.

## API Response Examples

### GET /api/state

```json
{
  "connected": true,
  "timestamp": 1732897200000,
  "telemetry": {
    "rpm": 3500,
    "vehicleSpeed": 120,
    "coolantTemperature": 92,
    "oilPressure": 350,
    "oilTemperature": 105,
    "batteryVoltage": 13.8,
    "throttlePosition": 45,
    "manifoldPressure": 250,
    "gear": "4",
    "fuelPressure": 300,
    "intakeAirTemperature": 35,
    "airFuelRatio": 14.7
  }
}
```

### GET /api/windows

```json
{
  "windows": [
    {
      "name": "cluster",
      "width": 1920,
      "height": 720,
      "visible": true
    },
    {
      "name": "headunit",
      "width": 1024,
      "height": 600,
      "visible": true
    }
  ]
}
```

### GET /api/warnings

```json
{
  "warnings": [],
  "criticals": []
}
```

*(Warning detection not yet implemented - returns empty arrays)*

### GET /api/screenshot?window=cluster

Returns PNG image binary data with `Content-Type: image/png`.

## Troubleshooting

**"Cannot connect to DevDash"**

- Ensure DevDash is running (`./scripts/run-with-mock idle`)
- Check logs for "DevToolsServer: Listening on http://127.0.0.1: 18080"
- Verify port 18080 isn't blocked

**"Window not found"**

- Use `/api/windows` to see available windows
- Window names are case-sensitive: "cluster", "headunit"

**MCP tools not available in Claude Code**

- Verify `.claude/mcp.json` is configured correctly
- Ensure `devdash-mcp` is installed: `pip list | grep devdash`
- Restart Claude Code after configuration changes

## Security

- DevToolsServer binds to `127.0.0.1` only (localhost - not network accessible)
- Read-only API - no state modification endpoints
- No authentication required (local-only access assumed safe)

## Future Extensions

The HTTP API design supports future additions:

- `GET /api/logs` - Server-Sent Events (SSE) for real-time log streaming
- `POST /api/simulate` - Inject test data for UI testing
- WebSocket endpoint for bidirectional communication
- Performance metrics and profiling data

## Files Modified/Created

### Created:
- `src/core/devtools/DevToolsServer.h`
- `src/core/devtools/DevToolsServer.cpp`
- `tools/mcp/devdash_mcp/__init__.py`
- `tools/mcp/devdash_mcp/__main__.py`
- `tools/mcp/devdash_mcp/server.py`
- `tools/mcp/pyproject.toml`
- `tools/mcp/README.md`
- `.claude/mcp.json`

### Modified:
- `src/core/CMakeLists.txt` - Added DevToolsServer sources and Qt6::Network
- `src/main.cpp` - Integrated DevToolsServer initialization

## Development

To modify or extend the MCP tools:

1. **Add HTTP endpoint** in `src/core/devtools/DevToolsServer.cpp`
2. **Add MCP tool** in `tools/mcp/devdash_mcp/server.py`
3. **Update documentation** in `tools/mcp/README.md`

Example adding a new endpoint:

```cpp
// In DevToolsServer.cpp
void DevToolsServer::handleRequest(...) {
    if (urlPath == "/api/myendpoint") {
        handleMyEndpoint(socket);
    }
}

void DevToolsServer::handleMyEndpoint(QTcpSocket* socket) {
    QJsonObject response;
    response["data"] = "my data";
    sendJsonResponse(socket, response);
}
```

Then add corresponding MCP tool in Python server.
