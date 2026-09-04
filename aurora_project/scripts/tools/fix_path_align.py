# -*- coding: utf-8 -*-
import io

p = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\widgets\Widgets.cpp"
t = io.open(p, encoding="utf-8").read()
BS = chr(92)
old = ("	if (present && path != nullptr && path[0] != '" + BS + "0') {\n"
       "		const float indent = Scale::Px(1.6F);\n")
new = ("	if (present && path != nullptr && path[0] != '" + BS + "0') {\n"
       "		// Путь у всех строк начинается на одной глубине — по высокой\n"
       "		// первой строке (высота кнопки), а не по тому, есть ли кнопка\n"
       "		// в конкретной строке. Иначе путь «проседал» у файлов с\n"
       "		// корзинкой и плясал у остальных.\n"
       "		const float firstLineBottom = rowTopY + std::max(buttonSize, ImGui::GetTextLineHeight())\n"
       "		    + ImGui::GetStyle().ItemSpacing.y;\n"
       "		ImGui::SetCursorPosY(firstLineBottom);\n"
       "		const float indent = Scale::Px(1.6F);\n")
assert old in t, "path block not found"
t = t.replace(old, new)
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
