# -*- coding: utf-8 -*-
"""Часть 3: ui/."""
import io

ROOT = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher"
R = []

def add(f, ln, marker, new):
    R.append((f, ln, marker, new))

add("ui/Icons.hpp", 3, "FontAwesome 4 codepoints", "/// Коды иконок FontAwesome 4 (байтовые последовательности UTF-8),")
add("ui/Icons.hpp", 4, "Render with a font", "/// используемые интерфейсом. Рисуются шрифтом с вшитым диапазоном иконок")

add("ui/LauncherView.cpp", 29, "Start browsing", "\t// Начинаем обзор с домашней папки пользователя — вероятнее всего")
add("ui/LauncherView.cpp", 30, "place for transferred", "\t// MPQ лежат именно там (~/Documents, ~/Downloads).")
add("ui/LauncherView.cpp", 80, "Drag-to-scroll gesture", "\t// Жест «потянул — прокрутил»: если палец увести дальше порога нажатия,")
add("ui/LauncherView.cpp", 81, "threshold while held", "\t// кадр считается прокруткой, и клики на время жеста")
add("ui/LauncherView.cpp", 82, "intents from clicks", "\t// игнорируются.")
add("ui/LauncherView.cpp", 101, "Keep a small gap", "\t// Небольшой отступ от края экрана, чтобы системные жесты телефона")
add("ui/LauncherView.cpp", 102, "clipped by system", "\t// не обрезали панель навигации.")
add("ui/LauncherView.cpp", 105, "Root window: covers", "\t// Корневое окно занимает весь экран, без рамок и заголовка.")
add("ui/LauncherView.cpp", 111, "The root is a transparent", "\t// Корень — прозрачный контейнер: флаг NoInputs не даёт ему перекрыть")
add("ui/LauncherView.cpp", 112, "covering the navbar", "\t// панель навигации и перехватывать клики при пересортировке окон ImGui")
add("ui/LauncherView.cpp", 113, "children of a NoInputs", "\t// (дочерние окна остаются кликабельными сами по себе).")
add("ui/LauncherView.cpp", 121, "Content area: the nav bar", "\t// Контентная область: панель навигации занимает место снизу (портрет)")
add("ui/LauncherView.cpp", 144, "Apply the drag delta", "\t\t\t// Пока жест активен и палец над контентом — применяем дельту")
add("ui/LauncherView.cpp", 145, "is active and the pointer", "\t\t\t// движения к позиции прокрутки.")
add("ui/LauncherView.cpp", 177, "The gesture ends", "\t// Жест заканчивается отпусканием кнопки — сброс ПОСЛЕ отрисовки,")
add("ui/LauncherView.cpp", 178, "a click fired", "\t// чтобы клик в кадре отпускания ещё подавлялся.")
add("ui/LauncherView.cpp", 381, "Slightly above", "\t// Чуть выше нижнего края экрана, чтобы системные жесты телефона")
add("ui/LauncherView.cpp", 382, "clip the buttons", "\t// не обрезали кнопки.")
add("ui/LauncherView.cpp", 506, "otherwise the block", "\t// иначе блок выше сразу же открыл бы его снова.")

add("ui/LauncherView.hpp", 17, "Top-level view", "/// Верхний уровень интерфейса: фон, панель навигации, роутер экранов,")
add("ui/LauncherView.hpp", 18, "dialogs and the MPQ", "/// модальные диалоги и файловый браузер MPQ.")
add("ui/LauncherView.hpp", 20, "MVI role", "/// Роль в MVI: чистая функция от LauncherState. Единственный способ")
add("ui/LauncherView.hpp", 21, "anything is by dispatching", "/// что-то изменить — отправить Intent; бизнес-состояния не хранит")
add("ui/LauncherView.hpp", 22, "(besides the FileBrowser", "/// (кроме экземпляра FileBrowser и учёта открытых попапов).")
add("ui/LauncherView.hpp", 30, "Render one frame", "\t/// Рисует один кадр. `dispatch` потокобезопасен (Store::Dispatch).")
add("ui/LauncherView.hpp", 39, "Attach a dedicated", "\t/// Подключает собственный арт hero-панели режима; пустая текстура —")
add("ui/LauncherView.hpp", 40, "keeps the bg.png", "\t/// остаётся обрезка общего фона.")
add("ui/LauncherView.hpp", 43, "Attach the golden", "\t/// Подключает уровни золотого силуэта иконки режима (от крупного);")
add("ui/LauncherView.hpp", 44, "zero count keeps", "\t/// нулевое число — глиф FontAwesome в плитках.")
add("ui/LauncherView.hpp", 68, "True while the user", "\t/// Истинно во время прокрутки перетаскиванием: клики на время жеста")
add("ui/LauncherView.hpp", 69, "dispatched during", "\t/// подавляются, чтобы провести пальцем по карточке — не значит")
add("ui/LauncherView.hpp", 70, 'card does not', "\t/// «нажать» на неё.")

add("ui/Theme.cpp", 20, "Palette — dark browns", "// Палитра: тёмно-коричневые, золото Diablo и глубокий красный.")
add("ui/Theme.cpp", 56, "ImGui owns the copied", "// Скопированным буфером владеет ImGui после Build.")
add("ui/Theme.cpp", 66, "FontAwesome 4 glyph", "// Диапазон глифов FontAwesome 4 для иконок лаунчера.")
add("ui/Theme.cpp", 79, "Discard any previously", "// Сбрасываем ранее загруженные шрифты (перезапуск на десктопе).")

add("ui/dialogs/Dialogs.cpp", 21, "Centered modal", "/// Помощник для модального окна по центру. Ширина задаётся в rem и")
add("ui/dialogs/Dialogs.cpp", 22, "always capped", "/// всегда ограничена экраном, чтобы диалог не вылез за телефон.")
add("ui/dialogs/Dialogs.cpp", 23, "phone screen. Returns", "/// Возвращает true, пока диалог открыт.")
add("ui/dialogs/Dialogs.cpp", 43, "Width for one of", "/// Ширина одной из пары кнопок диалога: желаемый размер в rem,")
add("ui/dialogs/Dialogs.cpp", 44, "never wider than", "/// но не больше половины доступного ряда.")

add("ui/dialogs/Dialogs.hpp", 9, "Modal confirmations", "/// Модальные подтверждения и оверлей загрузки. Каждый диалог целиком")
add("ui/dialogs/Dialogs.hpp", 10, "driven entirely", "/// управляется LauncherState и только отправляет интенты.")
add("ui/dialogs/Dialogs.hpp", 20, "Fullscreen download", "/// Оверлей загрузки на весь экран: скорость, оставшееся время,")
add("ui/dialogs/Dialogs.hpp", 21, "the failure panel", "/// отмена и панель ошибки (повторить / закрыть).")
add("ui/dialogs/Dialogs.hpp", 26, "Missing Hellfire files", "/// Предупреждение «не хватает файлов Hellfire».")
add("ui/dialogs/Dialogs.hpp", 29, "Simple error box", "/// Простое окно ошибки (state.errorText).")
add("ui/dialogs/Dialogs.hpp", 32, "Bottom transient", "/// Короткое уведомление внизу экрана. Время показа считает сам вид,")
add("ui/dialogs/Dialogs.hpp", 33, "locally and dispatches", "/// через ~3 с отправляет UiDismissToast.")
add("ui/dialogs/Dialogs.hpp", 36, "Opens the ImGui", "/// Открывает попап ImGui, соответствующий диалогу из состояния.")
add("ui/dialogs/Dialogs.hpp", 37, "The view calls this", "/// Вид зовёт это при смене state.dialog: попапы ImGui нужно")
add("ui/dialogs/Dialogs.hpp", 38, "opened explicitly", "/// открывать явно до отрисовки BeginPopupModal.")
add("ui/dialogs/Dialogs.hpp", 41, "Popup id used", "/// Идентификатор попапа оверлея загрузки (нужен для закрытия")

add("ui/screens/Screens.hpp", 10, "Per-mode artwork", "/// Арты режима: собственная картина hero-панели и набор золотых")
add("ui/screens/Screens.hpp", 11, "icon set for shelf", "/// силуэтов-иконок плиток. Оба необязательны (пустые текстуры):")
add("ui/screens/Screens.hpp", 12, "falls back to a crop", "/// без арта — обрезка общего фона, без иконки — глиф FontAwesome.")
add("ui/screens/Screens.hpp", 18, "All artwork", "/// Все арты, которыми рисуют экраны.")
add("ui/screens/Screens.hpp", 26, "Main screen: featured", "/// Главный экран: hero-панель выбранного режима (большая обложка с")
add("ui/screens/Screens.hpp", 27, "shelf of the remaining", "/// артом и кнопкой) плюс полка остальных режимов; при отсутствии")
add("ui/screens/Screens.hpp", 28, "first-run hero", "/// файлов — панель первого запуска.")

add("ui/widgets/Widgets.cpp", 20, "Largest font size", "/// Наибольший кегль (<= startSize), при котором текст влезает в ширину.")
add("ui/widgets/Widgets.cpp", 33, "Truncates text", "/// Обрезает текст с «…», чтобы он влезал в ширину.")
add("ui/widgets/Widgets.cpp", 34, "Removes whole UTF-8", "/// Удаляет целые кодовые точки UTF-8 — кириллица не режется пополам.")
add("ui/widgets/Widgets.cpp", 74, "Reserve layout space", "\t// Занимаем место в раскладке; клики — только по кнопкам.")
add("ui/widgets/Widgets.cpp", 93, "Legibility: full-canvas", "\t// Читаемость: лёгкая цветная подкраска на всю панель плюс нижний")
add("ui/widgets/Widgets.cpp", 94, "bottom gradient", "\t// градиент (от прозрачного к тёмному).")
add("ui/widgets/Widgets.cpp", 101, "Text block, anchored", "\t// Текстовый блок прижат к левому нижнему углу и не заходит")
add("ui/widgets/Widgets.cpp", 102, "kept clear of", "\t// на ряд кнопок.")
add("ui/widgets/Widgets.cpp", 118, "The text block sits", "\t\t// Блок текста целиком НАД рядом кнопок, поэтому он всегда")
add("ui/widgets/Widgets.cpp", 119, "the panel width", "\t// пользуется всей шириной панели (кнопки прижаты к низу).")
add("ui/widgets/Widgets.cpp", 122, "Measure the text block", "\t// Сначала измеряем блок текста — он не нарастёт на кнопки.")
add("ui/widgets/Widgets.cpp", 138, "Draw status line", "\t\t\t// Строки статуса рисуем по одной, чтобы переносы '\\n'")
add("ui/widgets/Widgets.cpp", 139, "The font first shrinks", "\t\t\t// выравнивались. Шрифт сначала уменьшается доfit (плотные экраны),")
add("ui/widgets/Widgets.cpp", 140, "ellipsis is the", "\t\t\t// многоточие — последнее средство.")
add("ui/widgets/Widgets.cpp", 156, "CTA buttons", "\t\t// Кнопки действий прижаты к правому нижнему углу (первая — правее всех).")
add("ui/widgets/Widgets.cpp", 203, "Icon chip with the game", "\t// Плашка иконки в цвете режима: золотой силуэт из ассетов")
add("ui/widgets/Widgets.cpp", 204, "bundled (nearest", "\t// (ближайший по размеру уровень), иначе глиф FontAwesome.")
add("ui/widgets/Widgets.cpp", 255, "Trailing play/lock", "\t// Индикатор справа: воспроизведение или замок.")
add("ui/widgets/Widgets.cpp", 277, "Drop shadow, then", "\t// Сначала тень, затем вертикальный золотой градиент.")
add("ui/widgets/Widgets.cpp", 303, "Opaque fill", "\t// Непрозрачная заливка, чтобы арт не просвечивал сквозь надпись.")

add("ui/widgets/Widgets.hpp", 31, "A call-to-action", "/// Кнопка действия для hero-панелей.")
add("ui/widgets/Widgets.hpp", 38, "Featured-game hero", "/// Hero-панель главного режима — большая «обложка» игры: арт с нижним")
add("ui/widgets/Widgets.hpp", 39, "key art", "/// градиентом, надзаголовок, крупное название, строка статуса и до двух")
add("ui/widgets/Widgets.hpp", 40, "two CTA buttons", "/// кнопок действий. Панель нажимается только своими кнопками.")
add("ui/widgets/Widgets.hpp", 42, "@param uv0/uv1", "/// @param uv0/uv1  видимая часть арта (в долях единицы)")
add("ui/widgets/Widgets.hpp", 43, "@param tint", "/// @param tint     цветовой оттенок поверх арта; его альфа — сила")
add("ui/widgets/Widgets.hpp", 44, "the mix strength", "///                 подкраски (обрезки фона ~0.16, свои арты ~0.08)")
add("ui/widgets/Widgets.hpp", 49, "Compact shelf tile", "/// Компактная плитка полки: иконка в плашке, название и статус, справа —")
add("ui/widgets/Widgets.hpp", 50, "play/lock indicator", "/// значок воспроизведения или замка. В плашке — золотой силуэт")
add("ui/widgets/Widgets.hpp", 51, "(nearest suitable", "/// (ближайший по размеру уровень), а без него — глиф FontAwesome.")
add("ui/widgets/Widgets.hpp", 55, "Gold gradient button", "/// Золотая градиентная кнопка с тёмным жирным текстом — главное действие.")
add("ui/widgets/Widgets.hpp", 58, "Outlined button", "/// Контурная кнопка с золотым текстом — второстепенное действие.")
add("ui/widgets/Widgets.hpp", 62, "Secondary button with", "/// Второстепенная кнопка с иконкой (для экранов и диалогов).")
add("ui/widgets/Widgets.hpp", 66, "Text with a given", "/// Текст цветом роли, по центру текущей ширины контента.")
add("ui/widgets/Widgets.hpp", 69, "Status bullet", "/// Маркер статуса: зелёная галка или тусклый крест + текст + пояснение.")

def main():
    by_file = {}
    for f, ln, marker, new in R:
        by_file.setdefault(f, []).append((ln, marker, new))
    ok, bad = 0, []
    for f, items in by_file.items():
        path = f"{ROOT}\\{f}"
        lines = io.open(path, encoding="utf-8").read().splitlines()
        for ln, marker, new in items:
            cur = lines[ln - 1]
            if marker not in cur:
                bad.append((f, ln, cur.strip()[:70]))
                continue
            lines[ln - 1] = new
            ok += 1
        io.open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    print("replaced:", ok, "missed:", len(bad))
    for b in bad:
        print("MISS", b)

main()
