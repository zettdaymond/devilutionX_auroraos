# -*- coding: utf-8 -*-
"""FileRow: имя+размер всегда на одной строке, кнопка — под размером."""
import io

p = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\widgets\Widgets.cpp"
t = io.open(p, encoding="utf-8").read()
BS = chr(92)

start = t.index("void FileRow(bool present")
end = t.index("void ScreenHeader")
new_impl = '''void FileRow(bool present, const char *name, const char *status, const char *path,
    const std::function<void()> &onDownload, const std::function<void()> &onDelete)
{
	// У каждого файла свои кнопки с одинаковыми метками («Скачать»,
	// корзинка) — без собственного пространства id ImGui посчитал бы их
	// одним и тем же виджетом.
	ImGui::PushID(name);

	const float startX = ImGui::GetCursorPosX();
	const float rowWidth = ImGui::GetContentRegionAvail().x;
	const float buttonSize = Scale::Px(2.2F);
	const float gap = Scale::Px(0.5F);
	const float rightEdge = startX + rowWidth;

	// Первая строка одинакова у всех файлов: маркер, имя и прижатый
	// вправо размер (или статус). Кнопка действий стоит под размером,
	// а не рядом с именем — тогда длинные имена ничего не вытесняют.
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(present ? ColorRole::Success : ColorRole::TextDim));
	ImGui::TextUnformatted(present ? icons::Check : icons::Times);
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(present ? ColorRole::TextBody : ColorRole::TextDim));
	ImGui::TextUnformatted(name);
	ImGui::PopStyleColor();

	const ImVec2 statusSize = ImGui::CalcTextSize(status);
	ImGui::SameLine(rightEdge - statusSize.x);
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextUnformatted(status);
	ImGui::PopStyleColor();

	const float textBottom = ImGui::GetCursorPosY();

	float actionWidth = 0.0F;
	if (onDelete) {
		actionWidth = buttonSize;
	} else if (onDownload) {
		actionWidth = ImGui::CalcTextSize("Скачать").x + Scale::Px(1.2F);
	}
	if (actionWidth > 0.0F) {
		ImGui::SetCursorPosX(rightEdge - actionWidth);
		Theme::PushButtonStyle(false);
		if (onDelete) {
			if (ImGui::Button(icons::Trash, ImVec2(buttonSize, buttonSize))) {
				onDelete();
			}
		} else {
			if (ImGui::Button("Скачать", ImVec2(actionWidth, buttonSize))) {
				onDownload();
			}
		}
		Theme::PopButtonStyle();
	}
	const float actionBottom = textBottom + buttonSize + gap;

	if (present && path != nullptr && path[0] != 'NULCHECK') {
		ImGui::SetCursorPosY(textBottom);
		const float indent = Scale::Px(1.6F);
		// Строки пути не заходят под кнопку, висящую справа.
		const float pathWidth = rowWidth - indent - (actionWidth > 0.0F ? actionWidth + gap : 0.0F);
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
		for (const std::string &line : WrapPathAtSlashes(ImGui::GetFont(), ImGui::GetFontSize(), path,
		     pathWidth)) {
			ImGui::SetCursorPosX(startX + indent);
			ImGui::TextUnformatted(line.c_str());
		}
		if (onDelete) {
			ImGui::SetCursorPosX(startX + indent);
			ImGui::Text("%s скачан", icons::Download);
		}
		ImGui::PopStyleColor();
	}

	// Кнопка может висеть ниже последней строки пути — не дадим
	// следующей строке наехать на неё.
	if (ImGui::GetCursorPosY() < actionBottom) {
		ImGui::SetCursorPosY(actionBottom);
	}
	ImGui::Dummy(ImVec2(0, Scale::Px(0.3F)));
	ImGui::PopID();
}

'''
new_impl = new_impl.replace("'NULCHECK'", "'" + BS + "0'")
t = t[:start] + new_impl + t[end:]
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("row ok")

# «О порте» -> «Инфо»
p2 = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\LauncherView.cpp"
t2 = io.open(p2, encoding="utf-8").read()
assert '{ Screen::About, icons::Info, "О порте" }' in t2
t2 = t2.replace('{ Screen::About, icons::Info, "О порте" }', '{ Screen::About, icons::Info, "Инфо" }')
io.open(p2, "w", encoding="utf-8", newline="\n").write(t2)
print("quick action ok")

p3 = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\screens\Screens.cpp"
t3 = io.open(p3, encoding="utf-8").read()
assert 'widgets::ScreenHeader("О порте"' in t3
t3 = t3.replace('widgets::ScreenHeader("О порте"', 'widgets::ScreenHeader("Инфо"')
io.open(p3, "w", encoding="utf-8", newline="\n").write(t3)
print("header ok")
