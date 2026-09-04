# -*- coding: utf-8 -*-
"""Тест качества локальной vision-модели (тот же backend, что у local-vision MCP)."""
import base64
import json
import sys
import urllib.request

opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
URL = "http://localhost:1234/v1/chat/completions"
MODEL = sys.argv[3] if len(sys.argv) > 3 else "qwen3.8-27b@q3_k_xl"


def ask(image_path, prompt):
    with open(image_path, "rb") as f:
        b64 = base64.b64encode(f.read()).decode("ascii")
    payload = {
        "model": MODEL,
        "messages": [
            {
                "role": "user",
                "content": [
                    {"type": "image_url", "image_url": {"url": "data:image/png;base64," + b64}},
                    {"type": "text", "text": prompt},
                ],
            }
        ],
        "temperature": 0.1,
        "max_tokens": 16384,
    }
    req = urllib.request.Request(URL, data=json.dumps(payload).encode("utf-8"),
                                 headers={"Content-Type": "application/json"})
    with opener.open(req, timeout=900) as resp:
        result = json.load(resp)
    with open(r"C:\Users\zett\AppData\Local\Temp\lv_raw.json", "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=1)
    msg = result["choices"][0]["message"]
    content = msg.get("content") or ""
    reasoning = msg.get("reasoning_content") or ""
    if not content and reasoning:
        content = "(reasoning) " + reasoning
    usage = result.get("usage", {})
    content += "\n[usage: prompt=%s completion=%s]" % (usage.get("prompt_tokens"), usage.get("completion_tokens"))
    return content


if __name__ == "__main__":
    image = sys.argv[1]
    prompt = sys.argv[2]
    answer = ask(image, prompt)
    with open(r"C:\Users\zett\AppData\Local\Temp\lv_answer.txt", "w", encoding="utf-8") as f:
        f.write(answer)
    print("written", len(answer))
