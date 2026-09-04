# -*- coding: utf-8 -*-
import io

p = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\widgets\Widgets.cpp"
t = io.open(p, encoding="utf-8").read()
bad = "path[0] != '" + chr(0) + "')"
good = "path[0] != '" + chr(92) + "0')"
assert bad in t, "NUL pattern not found"
t = t.replace(bad, good)
io.open(p, "w", encoding="utf-8", newline="\n").write(t)
print("nul fixed")

# Заголовок: новая сигнатура FileStatusLine
p2 = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\widgets\Widgets.hpp"
t = io.open(p2, encoding="utf-8").read()
old = """/// Маркер статуса: зелёная галка или тусклый крест + текст + пояснение.
void FileStatusLine(bool present, const char *text, const char *detail);"""
new = """/// Маркер статуса файла: галка/крест, имя и статус (размер либо
/// «не найден»). Для найденного файла вторыми строками идут путь
/// (обрезанный посередине многоточием до ширины колонки) и пометка
/// «скачан лаунчером».
void FileStatusLine(bool present, const char *name, const char *status, const char *path, bool downloaded);"""
assert old in t, "header decl not found"
t = t.replace(old, new)
io.open(p2, "w", encoding="utf-8", newline="\n").write(t)
print("header ok")

# Вызов в RenderFileGroup
p3 = r"D:\pr\Aurora\devilutionX_auroraos\Source\platform\aurora_os\devilution_launcher\ui\screens\Screens.cpp"
t = io.open(p3, encoding="utf-8").read()
old = """		const bool present = state.fileSizes[i] >= 0;
		std::string detail;
		if (present) {
			detail = FormatBytes(state.fileSizes[i]);
			if (!state.fileFolders[i].empty()) {
				detail += "\\n" + state.fileFolders[i].string();
			}
			if (spec.downloadable) {
				detail += "\\nскачан лаунчером";
			}
		} else {
			detail = spec.downloadable ? "можно скачать" : "не найден";
		}
		widgets::FileStatusLine(present, spec.displayName.data(), detail.c_str());"""
new = """		const bool present = state.fileSizes[i] >= 0;
		std::string status;
		std::string path;
		if (present) {
			status = FormatBytes(state.fileSizes[i]);
			path = state.fileFolders[i].string();
		} else {
			status = spec.downloadable ? "можно скачать" : "не найден";
		}
		widgets::FileStatusLine(present, spec.displayName.data(), status.c_str(),
		    path.empty() ? nullptr : path.c_str(), present && spec.downloadable);"""
assert old in t, "call site not found"
t = t.replace(old, new)
io.open(p3, "w", encoding="utf-8", newline="\n").write(t)
print("screens ok")
