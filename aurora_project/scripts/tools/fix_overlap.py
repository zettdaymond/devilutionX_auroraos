# -*- coding: utf-8 -*-
import io

p = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\widgets\Widgets.cpp"
t = io.open(p, encoding="utf-8").read()

old = """	float actionWidth = 0.0F;
	if (onDelete) {
		actionWidth = buttonSize;
	} else if (onDownload) {
		actionWidth = ImGui::CalcTextSize("Скачать").x + Scale::Px(1.2F);
	}
	const ImVec2 statusSize = ImGui::CalcTextSize(status);
	ImGui::SameLine(rightEdge - actionWidth - (actionWidth > 0.0F ? gap : 0.0F) - statusSize.x);
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::TextUnformatted(status);
	ImGui::PopStyleColor();
"""
new = """	float actionWidth = 0.0F;
	if (onDelete) {
		actionWidth = buttonSize;
	} else if (onDownload) {
		actionWidth = ImGui::CalcTextSize("Скачать").x + Scale::Px(1.2F);
	}

	// Статус справа: у отсутствующего скачиваемого файла кнопка «Скачать»
	// сама говорит всё — статус не дублируем. Если статус не влезает до
	// действия, уводим его строкой ниже (с отступом, как путь), а не
	// печатаем поверх имени.
	const ImVec2 statusSize = ImGui::CalcTextSize(status);
	const float nameWidth = ImGui::CalcTextSize(present ? icons::Check : icons::Times).x
	    + ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(name).x;
	const float statusX = rightEdge - actionWidth - (actionWidth > 0.0F ? gap : 0.0F) - statusSize.x;
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	if (!onDownload && startX + nameWidth + gap <= statusX) {
		ImGui::SameLine(statusX);
		ImGui::TextUnformatted(status);
		ImGui::PopStyleColor();
	} else if (!onDownload) {
		ImGui::PopStyleColor();
		ImGui::SetCursorPosX(startX + Scale::Px(1.6F));
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
		ImGui::TextUnformatted(status);
		ImGui::PopStyleColor();
	} else {
		ImGui::PopStyleColor();
	}
"""
assert old in t, "status block not found"
t = t.replace(old, new)
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
