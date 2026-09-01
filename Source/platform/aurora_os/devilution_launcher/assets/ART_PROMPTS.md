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

Кодирование (2026-09-01): арты хранятся JPEG quality 90 (subsampling off):
bg 2048×1200 — 178 КБ, heroes 960×657 — 185–204 КБ (изначальные PNG были
по ~1,3 МБ). Декодирует вендоренный `thirdparty/stb_image.h` (v2.30) —
сборка SDL_image для Aurora не содержит JPEG-загрузчика (проверено nm по
libSDL2_image.a), поэтому лаунчер не зависит от SDL_image вовсе: JPEG →
RGB24, PNG-иконки → RGBA32,Linear-скейл ставится на каждую текстуру.
Heroes перегенерируются теми же сидами (детерминированы), demo — 5111.

## Фон лаунчера (bg.jpg) — перегенерация 2026-09-01

Прежний фон (2048×1200) хорош в портрете, но в ландшафте капюшонная фигура
слева (0–43.5% кадра) перетягивала внимание с плиток режимов. Новая версия —
img2img от старого фона: наследует композицию и «почти чёрный» характер
(оригинал: vis 13.7% / yellow 88.6% / sat 0.39 / mean 9.7), фигуру убирает.

**Чекпоинт:** `dreamshaperXL10.safetensors` — полный DreamShaper XL 1.0
Alpha2 (Lykon; CivitAI 126688, SHA256 `0F1B80CF…B1FA4`; HF-зеркало
`jayparmr/DreamShaper_XL1_0_Alpha2`). НЕ Lightning-дистиллят: 30 шагов,
CFG 5.5, DPM++ 2M Karras. Общий режим: img2img, init = прежний bg.jpg,
1344×768. Negative (общий для всех): `text, watermark, signature, logo,
blurry, modern, photorealistic, people, person, characters, figure, hooded
figure, faces, creature, monster, skull, statue, bright colors, low quality,
deformed, close-up, busy composition, bright, vivid, high contrast, daylight`
(для fog-серий ещё `clear sky, sharp details`).

**Denoise-шкала вклада оригинала:** 0.42 — голова фигуры выживает
(«череп в шлеме», брак); 0.50 — баланс; 0.62–0.68 — сцена всё свободней.

### 1. bg.jpg — деревня в сильном тумане (финалист)

Сид 8801, denoise 0.68 (минимум оригинала). vis 18.1 / mean 13.6.

```
Dark fantasy digital painting, abandoned mysterious village at night swallowed by extremely dense fog,
thick heavy mist, only vague crooked silhouettes of wooden houses barely visible through the fog,
one faint dim lit window glowing through the mist, ravens circling above the rooftops,
dead gnarled trees, deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818),
desaturated, muted, faint dim amber glow, no characters, no text, no focal subject,
extremely dark, dim ambient, very low contrast, subdued,
oil painting style, 90s dark fantasy video game background art
```

### 2. bg_alt_cemetery_d62.jpg — кладбище в сильном тумане

Сид 8901, denoise 0.62. vis 18.9 / mean 13.4; у генерации уже есть своя
пара воронов на (47%, 37%).

```
Dark fantasy digital painting, abandoned cemetery in a mysterious ruined village at night
swallowed by extremely dense fog, thick rolling mist,
leaning gravestones and old crosses barely visible through the heavy fog,
vague crooked house silhouettes dissolving in the mist, ravens perched on gravestones,
dead gnarled tree, deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818),
desaturated, muted, faint dim amber glow, no characters, no text, no focal subject,
extremely dark, dim ambient, very low contrast, subdued,
oil painting style, 90s dark fantasy video game background art
```

### 3. bg_alt_cemetery_8702.jpg — кладбище (первый финалист)

Сид 8702, denoise 0.50. vis 14.2 / yellow 98.2 / sat 0.29 / mean 11.6.
Вариант с рисованными воронами (см. заметку ниже).

```
Dark fantasy digital painting, abandoned cemetery in a mysterious ruined village at night,
leaning gravestones and old weathered crosses, crooked wooden house silhouettes in the fog behind,
heavy fog rolling low over the graves, ravens perched on gravestones,
dead gnarled tree, deep charcoal black and dark yellow-olive palette (#181800, #303018, #301818),
desaturated, muted, faint dim amber glow, no characters, no text, no focal subject,
extremely dark, dim ambient, low contrast, subdued, oil painting style,
90s dark fantasy video game background art
```

**Общий конвейер постобработки** (все три): Color 0.90 (PIL) → градиент
затемнения к низу (×0.70 от y=55%) → апскейл ×2 нейросетевым DAT x2 через
A1111 `/sdapi/v1/extra-single-image` (1344×768 → 2688×1536) → JPEG q90.
Blur убран сознательно: DAT-апскейл сам сохраняет живописную фактуру,
а размытие делало мелкие детали невидимыми.

**Опыт с рисованными воронами** (для будущих итераций): inpaint по маске
модель заливает туманом; рисованные полигоны (крылья — Безье, туманный
ореол за каждой птицей) работают только в светлых полосах тумана — небо
темнее 20/255, тёмная птица вне светлой зоны не видна (дельта < 5).
Скрипты сессии: `%TEMP%\draw_ravens_up.py`.

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
ImGui ставит только своему шрифтовому атласу) — текстуры артов получают
`SDL_ScaleModeLinear` явно. Этого мало при сильной минификации: билинейка
без мипмапов (SDL_Renderer их не генерирует) рассыпает края, поэтому
иконки строятся цепочкой уровней 256/128/64 (stb_image_resize2, BOX) и
плитка рисует наименьший уровень, покрывающий чип (минификация ≤2x).
DreamShaper Lightning при CFG 2 игнорирует «flat silhouette» — нужен
CFG ~5; глазницы/глаза он всё равно рисует инвертированно, надёжнее
сплошной силуэт без вырезов.

**Апгрейд 2026-09-01 (убрана «резиновость» Lightning):** исходные арты
(960×657) прогнаны через DAT x2 (1920×1312) + img2img-дожарку на полном
dreamshaperXL10 (20 шагов, CFG 4, DPM++ 2M Karras, промпт исходный +
«oil painting style with visible brushwork», негатив + «smooth, plastic»).
Уровни дожарки выбраны по проверке дрейфа содержимого:
- `hero_diablo` — 0.40 (на 0.48 появляются новые фигуры/костёр, теряется
  шар между шпилями; на 0.40 шар лишь мягче, птицы в небе потеряны);
- `hero_hellfire` — 0.48, но область сигила-кольца и розеточного окна
  вклеена из чистого DAT-апскейла (на дожарке крест в кольце превращался
  в шипастое солнце), маска — растушёванные эллипсы, швов нет;
- `hero_demo` — 0.48 (чисто, ~90–93% совпадения, грейд чуть светлей).

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
