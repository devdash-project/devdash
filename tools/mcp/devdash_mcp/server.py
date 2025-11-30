"""DevDash MCP Server - HTTP bridge to DevTools API."""

import base64
import json
from typing import Any

import httpx
from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp.types import Tool, TextContent, ImageContent


# DevTools HTTP API endpoint
DEVTOOLS_BASE_URL = "http://127.0.0.1:18080"

# Initialize MCP server
app = Server("devdash")


@app.list_tools()
async def list_tools() -> list[Tool]:
    """List available DevDash tools."""
    return [
        Tool(
            name="devdash_get_state",
            description="Get current telemetry state from DevDash (RPM, speed, temperatures, etc.)",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": [],
            },
        ),
        Tool(
            name="devdash_get_warnings",
            description="Get active warnings and critical alerts from DevDash",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": [],
            },
        ),
        Tool(
            name="devdash_screenshot",
            description="Capture PNG screenshot of a DevDash window",
            inputSchema={
                "type": "object",
                "properties": {
                    "window": {
                        "type": "string",
                        "description": "Window name to capture (e.g., 'cluster', 'headunit')",
                    }
                },
                "required": ["window"],
            },
        ),
        Tool(
            name="devdash_list_windows",
            description="List available DevDash windows for screenshot capture",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": [],
            },
        ),
        Tool(
            name="devdash_get_logs",
            description="Retrieve recent application logs from DevDash with filtering by level and category",
            inputSchema={
                "type": "object",
                "properties": {
                    "count": {
                        "type": "integer",
                        "description": "Number of log entries to retrieve (max 1000, default 100)",
                        "default": 100,
                    },
                    "level": {
                        "type": "string",
                        "description": "Minimum log level to include (debug, info, warning, critical)",
                        "enum": ["debug", "info", "warning", "critical"],
                        "default": "info",
                    },
                    "category": {
                        "type": "string",
                        "description": "Filter by category (e.g., 'devdash.broker', 'devdash.adapter')",
                        "default": "",
                    },
                },
                "required": [],
            },
        ),
    ]


@app.call_tool()
async def call_tool(name: str, arguments: Any) -> list[TextContent | ImageContent]:
    """Handle tool execution requests."""
    try:
        async with httpx.AsyncClient(timeout=5.0) as client:
            if name == "devdash_get_state":
                response = await client.get(f"{DEVTOOLS_BASE_URL}/api/state")
                response.raise_for_status()
                data = response.json()

                return [
                    TextContent(
                        type="text",
                        text=json.dumps(data, indent=2),
                    )
                ]

            elif name == "devdash_get_warnings":
                response = await client.get(f"{DEVTOOLS_BASE_URL}/api/warnings")
                response.raise_for_status()
                data = response.json()

                return [
                    TextContent(
                        type="text",
                        text=json.dumps(data, indent=2),
                    )
                ]

            elif name == "devdash_list_windows":
                response = await client.get(f"{DEVTOOLS_BASE_URL}/api/windows")
                response.raise_for_status()
                data = response.json()

                return [
                    TextContent(
                        type="text",
                        text=json.dumps(data, indent=2),
                    )
                ]

            elif name == "devdash_get_logs":
                # Build query parameters
                params = {}
                if "count" in arguments:
                    params["count"] = min(arguments["count"], 1000)
                if "level" in arguments:
                    params["level"] = arguments["level"]
                if "category" in arguments and arguments["category"]:
                    params["category"] = arguments["category"]

                response = await client.get(
                    f"{DEVTOOLS_BASE_URL}/api/logs",
                    params=params,
                )
                response.raise_for_status()
                data = response.json()

                return [
                    TextContent(
                        type="text",
                        text=json.dumps(data, indent=2),
                    )
                ]

            elif name == "devdash_screenshot":
                window = arguments.get("window")
                if not window:
                    return [
                        TextContent(
                            type="text",
                            text="Error: 'window' parameter is required",
                        )
                    ]

                response = await client.get(
                    f"{DEVTOOLS_BASE_URL}/api/screenshot",
                    params={"window": window},
                )
                response.raise_for_status()

                # Return image as base64-encoded PNG
                image_data = response.content
                image_b64 = base64.b64encode(image_data).decode("utf-8")

                return [
                    ImageContent(
                        type="image",
                        data=image_b64,
                        mimeType="image/png",
                    )
                ]

            else:
                return [
                    TextContent(
                        type="text",
                        text=f"Unknown tool: {name}",
                    )
                ]

    except httpx.ConnectError:
        return [
            TextContent(
                type="text",
                text="Error: Cannot connect to DevDash. Make sure DevDash is running with DevTools server enabled.",
            )
        ]
    except httpx.HTTPStatusError as e:
        return [
            TextContent(
                type="text",
                text=f"Error: HTTP {e.response.status_code} - {e.response.text}",
            )
        ]
    except Exception as e:
        return [
            TextContent(
                type="text",
                text=f"Error: {str(e)}",
            )
        ]


def main():
    """Run the DevDash MCP server."""
    import asyncio

    async def run():
        async with stdio_server() as (read_stream, write_stream):
            await app.run(read_stream, write_stream, app.create_initialization_options())

    asyncio.run(run())


if __name__ == "__main__":
    main()
