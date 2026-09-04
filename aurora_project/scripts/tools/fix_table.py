# -*- coding: utf-8 -*-
import io

base = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher"

# 1) Таблица: имя — по контенту, остальное — растяжение
p = base + r"\ui\screens\Screens.cpp"
t = io.open(p, encoding="utf-8").read()
old = """	if (!ImGui::BeginTable(tableName.c_str(), columnCount, ImGuiTableFlags_SizingStretchProp)) {
		return;
	}
	ImGui::TableSetupColumn("status", ImGuiTableColumnFlags_WidthFixed, Scale::Px(1.6F));
	ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("detail", ImGuiTableColumnFlags_WidthStretch);"""
new = """	// FixedFit: колонка имени сжимается ровно под самое длинное имя файла,
	// а всё свободное место достаётся размеру и пути.
	if (!ImGui::BeginTable(tableName.c_str(), columnCount, ImGuiTableFlags_SizingFixedFit)) {
		return;
	}
	ImGui::TableSetupColumn("status", ImGuiTableColumnFlags_WidthFixed, Scale::Px(1.6F));
	ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("detail", ImGuiTableColumnFlags_WidthStretch);"""
assert old in t
t = t.replace(old, new)
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("table ok")

# 2) MiddleEllipsis: замер тем же шрифтом и кеглем, которыми рисуем
p = base + r"\ui\widgets\Widgets.cpp"
t = io.open(p, encoding="utf-8").read()
old = """	if (present && path != nullptr && path[0] != '\\0') {
		const std::string fitted = MiddleEllipsis(Theme::Font(FontRole::Body), Scale::Px(0.85F), path,
		    ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(fitted.c_str());"""
new = """	if (present && path != nullptr && path[0] != '\\0') {
		const std::string fitted = MiddleEllipsis(ImGui::GetFont(), ImGui::GetFontSize(), path,
		    ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(fitted.c_str());"""
assert old in t
t = t.replace(old, new)
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("measure ok")
