# -*- coding: utf-8 -*-
"""Блочные строки чек-листа: путь целиком со переносом по разделителям,
кнопки полного размера, грамматика групп, «свободно» в Прочее."""
import io

base = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher"
BS = chr(92)  # backslash

def cpp_char_backslash():
    # C++ char literal for backslash: '\''
    return "'" + BS + BS + "'"

# ---------- Widgets.hpp ----------
p = base + r"\ui\widgets\Widgets.hpp"
t = io.open(p, encoding="utf-8").read()
old = """/// Маркер статуса файла: галка/крест, имя и статус (размер либо
/// «не найден»). Для найденного файла вторыми строками идут путь
/// (обрезанный посередине многоточием до ширины колонки) и пометка
/// «скачан лаунчером».
void FileStatusLine(bool present, const char *name, const char *status, const char *path, bool downloaded);"""
new = """/// Строка файла в чек-листе. Первая строка: маркер, имя, справа размер
/// (или «можно скачать»/«не найден») и действие — корзинка для
/// скачанного, кнопка «Скачать» для отсутствующего скачиваемого.
/// Ниже — путь целиком на всю ширину (перенос только по разделителям
/// «/» и «\\\\») и пометка «скачан».
void FileRow(bool present, const char *name, const char *status, const char *path,
    const std::function<void()> &onDownload, const std::function<void()> &onDelete);"""
assert old in t, "hpp decl not found"
t = t.replace(old, new)
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("hpp ok")

# ---------- Widgets.cpp ----------
p = base + r"\ui\widgets\Widgets.cpp"
t = io.open(p, encoding="utf-8").read()

start_marker = "/// Обрезает текст посередине"
end_marker = "\tImGui::PopStyleColor();\n}\n\nvoid ScreenHeader"
si = t.index(start_marker)
ei = t.index(end_marker)

q = chr(34)  # double quote
nul_check = "path[0] != '" + BS + "0'"
backslash_char = cpp_char_backslash()

impl = """/// Разбивает путь на строки по разделителям «/» и «\\»: перенос никогда
/// не рвёт имя каталога посередине, путь виден целиком.
std::vector<std::string> WrapPathAtSlashes(ImFont *font, float size, const std::string &path, float maxWidth)
{
	std::vector<std::string> parts;
	std::string current;
	for (char c : path) {
		current += c;
		if ((c == '/' || c == BACKSLASH_CHAR) && current.size() > 1) {
			parts.push_back(current);
			current.clear();
		}
	}
	if (!current.empty()) {
		parts.push_back(current);
	}

	std::vector<std::string> lines;
	std::string line;
	auto lineWidth = [&](const std::string &s) {
		return font != nullptr ? font->CalcTextSizeA(size, FLT_MAX, 0.0F, s.c_str()).x : 0.0F;
	};
	for (const std::string &part : parts) {
		if (!line.empty() && lineWidth(line + part) > maxWidth) {
			lines.push_back(line);
			line.clear();
		}
		line += part;
		if (lineWidth(line) > maxWidth) {
			lines.push_back(line); // даже один сегмент шире строки — оставляем его целиком
			line.clear();
		}
	}
	if (!line.empty()) {
		lines.push_back(line);
	}
	return lines;
}

void FileRow(bool present, const char *name, const char *status, const char *path,
    const std::function<void()> &onDownload, const std::function<void()> &onDelete)
{
	const float startX = ImGui::GetCursorPosX();
	const float rowWidth = ImGui::GetContentRegionAvail().x;
	const float buttonSize = Scale::Px(2.2F);
	const float gap = Scale::Px(0.5F);
	const float rightEdge = startX + rowWidth;

	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(present ? ColorRole::Success : ColorRole::TextDim));
	ImGui::TextUnformatted(present ? icons::Check : icons::Times);
	ImGui::PopStyleColor();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(present ? ColorRole::TextBody : ColorRole::TextDim));
	ImGui::TextUnformatted(name);
	ImGui::PopStyleColor();

	float actionWidth = 0.0F;
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

	if (onDelete || onDownload) {
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

	if (present && path != nullptr && NUL_CHECK) {
		const float indent = Scale::Px(1.6F);
		ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
		for (const std::string &line : WrapPathAtSlashes(ImGui::GetFont(), ImGui::GetFontSize(), path,
		     rowWidth - indent)) {
			ImGui::SetCursorPosX(startX + indent);
			ImGui::TextUnformatted(line.c_str());
		}
		if (onDelete) {
			ImGui::SetCursorPosX(startX + indent);
			ImGui::Text("%s скачан", icons::Download);
		}
		ImGui::PopStyleColor();
	}
	ImGui::Dummy(ImVec2(0, Scale::Px(0.3F)));
}

"""
impl = impl.replace("BACKSLASH_CHAR", backslash_char)
impl = impl.replace("NUL_CHECK", nul_check)
# в докомментарии — C-строка с обратным слешем «\\» уже корректна в шаблоне

t = t[:si] + impl + t[ei + len("\tImGui::PopStyleColor();\n}\n\n"):]
if "#include <vector>" not in t:
    t = t.replace("#include <string>", "#include <string>\n#include <vector>", 1)
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("widgets ok")

# ---------- Screens.cpp: RenderFileGroup без таблицы ----------
p = base + r"\ui\screens\Screens.cpp"
t = io.open(p, encoding="utf-8").read()
si = t.index("/// Рисует одну группу чек-листа")
ei = t.index("/// Чек-лист файлов по группам")

group_impl = '''/// Рисует одну группу чек-листа: заголовок со сводкой и строки файлов.
void RenderFileGroup(const LauncherState &state, const Dispatcher &dispatch, const FileGroup &group)
{
	size_t missing = 0;
	for (KnownFile file : group.files) {
		if (state.fileSizes[static_cast<size_t>(file)] < 0) {
			++missing;
		}
	}

	ImGui::Dummy(ImVec2(0, Scale::Px(0.2F)));
	ImGui::PushFont(Theme::Font(FontRole::BodyBold));
	ImGui::TextUnformatted(group.title);
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::PushFont(Theme::Font(FontRole::Body));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(missing == 0 ? ColorRole::Success : ColorRole::TextDim));
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scale::Px(0.15F));
	if (strcmp(group.title, "Прочее") == 0) {
		ImGui::Text("— не обязательны для запуска · свободно %s", FormatBytes(state.freeDiskBytes).c_str());
	} else if (missing == 0) {
		ImGui::TextUnformatted("— всё на месте");
	} else if (group.files.size() == 1) {
		ImGui::TextUnformatted("— файл не найден");
	} else {
		ImGui::Text("— не хватает %zu из %zu", missing, group.files.size());
	}
	ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, Scale::Px(0.2F)));

	for (KnownFile id : group.files) {
		const size_t i = static_cast<size_t>(id);
		const FileSpec &spec = kFileCatalog[i];
		const bool present = state.fileSizes[i] >= 0;
		std::string status;
		std::string path;
		if (present) {
			status = FormatBytes(state.fileSizes[i]);
			path = state.fileFolders[i].string();
		} else {
			status = spec.downloadable ? "можно скачать" : "не найден";
		}

		std::function<void()> onDownload;
		if (!present && spec.downloadable) {
			const Dialog dialog
			    = (id == KnownFile::Spawn) ? Dialog::ConfirmDownloadDemo : Dialog::ConfirmDownloadRu;
			onDownload = [&dispatch, dialog] { dispatch(intent::UiOpenDialog { dialog }); };
		}
		std::function<void()> onDelete;
		if (present && spec.downloadable) {
			onDelete = [&dispatch, id] { dispatch(intent::DeleteDownloadedFile { id }); };
		}

		widgets::FileRow(present, spec.displayName.data(), status.c_str(),
		    path.empty() ? nullptr : path.c_str(), onDownload, onDelete);
	}
}

'''
t = t[:si] + group_impl + t[ei:]

old_loop = """	for (int i = 0; i < 3; ++i) {
		RenderFileGroup(state, dispatch, groups[i], i);
	}"""
new_loop = """	for (const FileGroup &group : groups) {
		RenderFileGroup(state, dispatch, group);
	}"""
assert old_loop in t, "loop not found"
t = t.replace(old_loop, new_loop)

old_free = """	ImGui::Dummy(ImVec2(0, Scale::Px(0.6F)));
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Color(ColorRole::TextDim));
	ImGui::Text("%s Свободно: %s", icons::Hdd, FormatBytes(state.freeDiskBytes).c_str());
	ImGui::PopStyleColor();
}"""
assert old_free in t, "free-space tail not found"
t = t.replace(old_free, "}")
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("screens ok")
