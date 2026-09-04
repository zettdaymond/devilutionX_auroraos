# -*- coding: utf-8 -*-
"""Клиентский тест local-vision MCP по stdio (полный протокол MCP)."""
import asyncio
import sys

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

SERVER = r"C:\Users\zett\.claude\mcp\local_vision_mcp.py"
PYTHON = r"C:\Users\zett\.claude\mcp\venv\Scripts\python.exe"
IMAGE = r"D:\pr\Aurora\devilutionX_auroraos\.screenshots\settings\v3_portrait.png"


async def main():
    params = StdioServerParameters(
        command=PYTHON,
        args=[SERVER],
        env={
            "LM_STUDIO_URL": "http://localhost:1234",
            "VISION_MODEL": "qwen3.8-27b@q3_k_xl",
            "VISION_MAX_TOKENS": "16384",
        },
    )
    async with stdio_client(params) as (read, write):
        async with ClientSession(read, write) as session:
            info = await session.initialize()
            print("connected:", info.server_info.name)

            tools = await session.list_tools()
            print("tools:", [t.name for t in tools.tools])

            result = await session.call_tool(
                "analyze_image",
                {
                    "image_path": IMAGE,
                    "prompt": "Перечисли видимые строки настроек и состояние тумблера каждой: "
                              "ВКЛ (золотой трек) или ВЫКЛ (тёмный трек). Кратко.",
                },
            )
            for item in result.content:
                print(item.text if hasattr(item, "text") else item)


asyncio.run(main())
sys.exit(0)
