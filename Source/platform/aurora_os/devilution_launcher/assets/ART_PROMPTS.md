# Промпты для генерации hero-артов режимов

Цель: отдельные арты для hero-панелей Diablo / Hellfire / Demo.
Файлы кладём сюда: `hero_diablo.png`, `hero_hellfire.png`, `hero_demo.png`
(1216×832 для SDXL или 1568×1056 для GLM-Image / CogView-4; PNG, тёмный низ под текст).

**Сгенерировано 2026-08-31** через A1111 REST API (`--api`, http://127.0.0.1:7860;
запросы шли мимо прокси — `ProxyHandler({})` в urllib / `--noproxy "*"` в curl):

| Файл | Сид | Примечание |
|---|---|---|
| `hero_diablo.png` | 1307 | Тристрам, 8/10 по ревью |
| `hero_hellfire.png` | 2607 | сигил над монастырём, 7.5–8/10 |
| `hero_demo.png` | 5111 | склеп с факелом, 8/10, чистая (сид 3904 брак — псевдо-подпись в углу) |

Инфраструктура подключена: `ArtSet` (ui/screens/Screens.hpp) несёт per-mode
текстуры, `Application` грузит `assets/hero_*.png` с фолбэком на кроп `bg.png`.
Перегенерация: замените PNG и пересоберите — файлы вшиты через CMakeRC.

## Иконки плиток (золотые силуэты)

Сгенерированы там же (1024×1024, CFG 5 / 12 шагов, жёсткий промпт
«logo emblem, solid black flat silhouette, stencil»). Постобработка
(порог + флуд-филл от краёв + очистка до главной компоненты) превращает
драфт в сплошное золото #E8C77E с альфой; вырезы не используются —
только скважина Demo пробита в маске кодом. Финальные сиды:
`icon_hellfire.png` — монах в капюшоне, сид 4711; `icon_diablo.png` —
компактный рогатый череп, сид 4826 + вертикальное сжатие 0.84 (драфты
тянутся вниз); `icon_demo.png` — арка со скважиной, сид 4713. Выход
512×512 RGBA, LANCZOS-сглаживание маски. Фолбэк при отсутствии —
FontAwesome-глифы (Fire/Gamepad/Download).

Гладкость краёв: SDL2 создаёт текстуры с Nearest-скейлом (Linear бэкенд
ImGui ставит только своему шрифтовому атласу) — `LoadAssetTexture`
явно ставит `SDL_ScaleModeLinear` всем арт-текстурам. DreamShaper
Lightning при CFG 2 игнорирует «flat silhouette» — нужен CFG ~5;
глазницы/глаза он всё равно рисует инвертированно, надёжнее сплошной
силуэт без вырезов.

## Diablo (главный)

```
Dark fantasy digital painting, medieval gothic town of Tristram at night,
cathedral silhouette with single lit window, dying bonfire embers floating in air,
deep browns and dark reds palette (#1A0F0A, #8B1A10), golden torchlight accents (#C7A568),
moody atmospheric fog, no characters, no text, no logos,
composition: focal point in upper center, darker at bottom, oil painting style, 90s dark fantasy video game box art
```

## Hellfire

```
Dark fantasy digital painting, infernal demon sigil glowing above ruined monastery,
orange hellfire and ember storm (#B3621A, #8B1A10 accents), deep black-brown shadows,
ominous heat haze, no characters, no text, no logos,
composition: focal point upper right, darker bottom left for text overlay,
90s dark fantasy video game box art style
```

## Shareware/Demo

```
Dark fantasy digital painting, entrance to a dank dungeon crypt, single burning torch
on stone wall, warm amber glow against cold darkness (#6B5D48 stone, #E8C77E firelight),
subtle floating embers, inviting but ominous mood, no characters, no text, no logos,
composition: doorway in upper center framing darkness, 90s dark fantasy game art
```

## Параметры

**Stable Diffusion WebUI (A1111, REST API `http://127.0.0.1:7860`, запуск с `--api`):**
- Чекпоинт: SDXL (Juggernaut XL / DreamShaper XL), размер 1216×832, steps 30, CFG 5–7, DPM++ 2M Karras
- Negative: `text, watermark, signature, logo, blurry, modern, photorealistic, people, characters, bright colors, low quality, deformed`

**z-ai-image MCP (`z-ai-image-mcp`, .mcp.json в корне репо):**
- Модель `glm-image`, размер `1568x1056` (или `cogview-4-250304`, `1344x768`), инструмент `generate_and_download_image` с `file_output` прямо в assets/
