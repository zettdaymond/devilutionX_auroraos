# -*- coding: utf-8 -*-
import io

p = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\widgets\Widgets.cpp"
t = io.open(p, encoding="utf-8").read()
BS = chr(92)

# 1) захватить низ текстовой строки до кнопки
old = """	if (onDelete || onDownload) {
		ImGui::SameLine(rightEdge - actionWidth);
		Theme::PushButtonStyle(false);"""
new = """	// Низ текстовой строки: путь начнётся сразу под именем, а не под
	// высокой кнопкой — иначе у строк с кнопкой между именем и путём
	// зияла пустота размером с кнопку.
	const float textBottom = ImGui::GetCursorPosY();

	if (onDelete || onDownload) {
		ImGui::SameLine(rightEdge - actionWidth);
		Theme::PushButtonStyle(false);"""
assert old in t, "button block not found"
t = t.replace(old, new)

# 2) путь: старт у textBottom, ширина без зоны кнопки
old2 = ("	if (present && path != nullptr && path[0] != '" + BS + "0') {\n"
        "		// Путь у всех строк начинается на одной глубине — по высокой\n"
        "		// первой строке (высота кнопки), а не по тому, есть ли кнопка\n"
        "		// в конкретной строке. Иначе путь «проседал» у файлов с\n"
        "		// корзинкой и плясал у остальных.\n"
        "		const float firstLineBottom = rowTopY + std::max(buttonSize, ImGui::GetTextLineHeight())\n"
        "		    + ImGui::GetStyle().ItemSpacing.y;\n"
        "		ImGui::SetCursorPosY(firstLineBottom);\n"
        "		const float indent = Scale::Px(1.6F);\n")
new2 = ("	if (present && path != nullptr && path[0] != '" + BS + "0') {\n"
        "		ImGui::SetCursorPosY(textBottom);\n"
        "		const float indent = Scale::Px(1.6F);\n"
        "		// Строки пути не заходят в зону кнопки: кнопка занимает\n"
        "		// правый край первых ~2.2rem высоты строки.\n"
        "		const float pathWidth = rowWidth - indent - (actionWidth > 0.0F ? actionWidth + gap : 0.0F);\n")
assert old2 in t, "path block not found"
t = t.replace(old2, new2)

# 3) maxWidth у WrapPathAtSlashes -> pathWidth
old3 = """		for (const std::string &line : WrapPathAtSlashes(ImGui::GetFont(), ImGui::GetFontSize(), path,
		     rowWidth - indent)) {"""
new3 = """		for (const std::string &line : WrapPathAtSlashes(ImGui::GetFont(), ImGui::GetFontSize(), path,
		     pathWidth)) {"""
assert old3 in t, "wrap call not found"
t = t.replace(old3, new3)

# 4) rowTopY больше не используется
t = t.replace("""	const float startX = ImGui::GetCursorPosX();
	const float rowTopY = ImGui::GetCursorPosY();""", """	const float startX = ImGui::GetCursorPosX();""")

io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
