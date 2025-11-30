# DevDash MCP Server

Model Context Protocol (MCP) server providing Claude Code with UI visibility and telemetry access for DevDash.

## Architecture

```
Claude Code CLI
      |
      | MCP Protocol (stdio)
      v
+---------------------------+
| devdash-mcp (Python)      |
| - devdash_get_state       |
| - devdash_screenshot      |
| - devdash_get_warnings    |
| - devdash_list_windows    |
+------------+--------------+
             | HTTP (localhost:18080)
             v
+---------------------------+
| DevDash Application       |
| DevToolsServer (C++)      |
| - GET /api/state          |
| - GET /api/warnings       |
| - GET /api/screenshot     |
| - GET /api/windows        |
+---------------------------+
```

## Installation

From the `tools/mcp` directory:

```bash
# Install in development mode
pip install -e .

# Or install normally
pip install .
```

## Configuration

Add to `.claude/mcp.json`:

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

## MCP Tools

### `devdash_get_state`

Get current telemetry state from DevDash.

**Returns**: JSON object with connection status, timestamp, and telemetry values (RPM, speed, temperatures, pressures, etc.)

**Example**:
```json
{
  "connected": true,
  "timestamp": 1732897200000,
  "telemetry": {
    "rpm": 3500,
    "vehicleSpeed": 120,
    "coolantTemperature": 92,
    "oilPressure": 350,
    "batteryVoltage": 13.8
  }
}
```

### `devdash_screenshot`

Capture PNG screenshot of a DevDash window.

**Parameters**:
- `window` (required): Window name to capture (e.g., "cluster", "headunit")

**Returns**: PNG image

**Example usage in Claude Code**:
```
Can you show me a screenshot of the cluster window?
```

### `devdash_get_warnings`

Get active warnings and critical alerts.

**Returns**: JSON object with warnings and criticals arrays

**Example**:
```json
{
  "warnings": [],
  "criticals": []
}
```

### `devdash_list_windows`

List available windows for screenshot capture.

**Returns**: JSON array of window information

**Example**:
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

## Usage Example

Once configured, Claude Code can directly query DevDash:

```
User: What's the current RPM?
Claude: [uses devdash_get_state tool] The engine is currently at 3500 RPM.

User: Show me the cluster display
Claude: [uses devdash_screenshot tool with window="cluster"] Here's the current cluster view...

User: Are there any warnings?
Claude: [uses devdash_get_warnings tool] No active warnings or critical alerts.
```

## Requirements

- DevDash must be running with DevToolsServer enabled (started automatically in main.cpp)
- DevToolsServer binds to `127.0.0.1:18080` (localhost only)
- Python 3.10+
- MCP SDK (`mcp>=0.9.0`)
- HTTP client (`httpx>=0.27.0`)

## Troubleshooting

**"Cannot connect to DevDash"**:
- Ensure DevDash is running
- Check that port 18080 is not blocked by firewall
- Verify DevToolsServer started successfully (check logs)

**"Window not found"**:
- Use `devdash_list_windows` to see available windows
- Window names are case-sensitive ("cluster", not "Cluster")

## Development

The MCP server is a simple HTTP-to-MCP bridge. It translates MCP tool calls into HTTP requests to DevDash's DevToolsServer.

To modify or extend:
1. Edit `devdash_mcp/server.py`
2. Add new tools in `list_tools()` and `call_tool()`
3. Corresponding endpoints must exist in DevToolsServer (C++)
