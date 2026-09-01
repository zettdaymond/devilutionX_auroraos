#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace launcher {

/// Группа на экране «Настройки»; порядок значений задаёт порядок групп.
enum class SettingGroup : uint8_t {
	Gameplay,
	Interface,
	Graphics,
	Audio,
};

/// Идентификатор одной движковой настройки из diablo.ini.
/// Порядок значений задаёт порядок строк внутри группы.
enum class SettingId : uint8_t {
	// Геймплей
	RunInTown,
	AutoGoldPickup,
	AutoElixirPickup,
	AutoPickupInTown,
	AutoRefillBelt,
	RandomizeQuests,
	TheoQuest,
	CowQuest,
	AdriaRefillsMana,
	DisableCripplingShrines,
	QuickCast,
	HealPotionPickup,
	ManaPotionPickup,
	// Интерфейс
	EnemyHealthBar,
	ShowHealthValues,
	ShowManaValues,
	ExperienceBar,
	ShowItemLabels,
	FloatingNumbers,
	ShowMonsterType,
	// Графика
	Zoom,
	ColorCycling,
	FpsLimiter,
	ShowFps,
	GammaCorrection,
	// Звук
	SoundVolume,
	MusicVolume,
	WalkingSound,

	Count
};

constexpr size_t kSettingCount = static_cast<size_t>(SettingId::Count);

/// Как рисовать настройку и в каких единицах хранить её значение.
enum class SettingKind : uint8_t {
	Toggle,        // bool, в ini пишется 1/0
	Slider,        // целое в диапазоне [minValue..maxValue], единицы как в ini
	PercentVolume, // громкость: состояние хранит проценты 0..100, в ini — шкала −1600..0
	Cycle,         // перебор вариантов: значение = optionValues[индекс]
};

constexpr size_t kMaxSettingOptions = 6;

/// Неизменное описание настройки: ключ diablo.ini, вид контрола,
/// значение по умолчанию и русские тексты. Для PercentVolume defaultInt
/// и minValue/maxValue заданы в процентах. Для Cycle optionCount > 0,
/// вариант i имеет имя optionNames[i] и значение optionValues[i]
/// (значения не обязаны совпадать с индексами: зелья — 0/1/2/4/8/16).
struct SettingSpec {
	SettingId id;
	SettingGroup group;
	SettingKind kind;
	std::string_view section;
	std::string_view key;
	int defaultInt;
	int minValue;
	int maxValue;
	std::array<std::string_view, kMaxSettingOptions> optionNames; // Cycle: первые optionCount имён
	std::array<int, kMaxSettingOptions> optionValues;             // Cycle: значения вариантов
	uint8_t optionCount;                                          // 0 = не Cycle
	std::string_view nameRu;
	std::string_view descriptionRu; // пустая строка = без описания
};

/// Каталог всех настраиваемых движковых опций. Держать синхронно
/// с SettingId: kSettingCatalog[i].id == SettingId(i).
inline constexpr std::array<SettingSpec, kSettingCount> kSettingCatalog { {
	{ SettingId::RunInTown, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Run in Town",
		0, 0, 1, {}, {}, 0, "Бег в городе",
		"Герои бегают по городу без удержания кнопки бега (функция Hellfire, действует и в Diablo)." },
	{ SettingId::AutoGoldPickup, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Auto Gold Pickup",
		0, 0, 1, {}, {}, 0, "Автоподбор золота",
		"Золото подбирается само, когда герой проходит рядом." },
	{ SettingId::AutoElixirPickup, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Auto Elixir Pickup",
		0, 0, 1, {}, {}, 0, "Автоподбор эликсиров",
		"Эликсиры подбираются сами при прохождении рядом." },
	{ SettingId::AutoPickupInTown, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Auto Pickup in Town",
		0, 0, 1, {}, {}, 0, "Автоподбор в городе",
		"Автоподбор золота и эликсиров работает и в городе." },
	{ SettingId::AutoRefillBelt, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Auto Refill Belt",
		0, 0, 1, {}, {}, 0, "Автопополнение пояса",
		"Выпитое зелье автоматически заменяется таким же из сумки." },
	{ SettingId::RandomizeQuests, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Randomize Quests",
		1, 0, 1, {}, {}, 0, "Случайные квесты",
		"Каждая новая игра выбирает случайный набор квестов, как заведено в оригинале." },
	{ SettingId::TheoQuest, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Theo Quest",
		0, 0, 1, {}, {}, 0, "Квест Тео (Hellfire)",
		"Добавляет квест маленькой девочки о пропавшем мишке Тео." },
	{ SettingId::CowQuest, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Cow Quest",
		0, 0, 1, {}, {}, 0, "Коровий квест (Hellfire)",
		"Фермер Лестер превращается в Совершенного Психа с коровьим костюмом." },
	{ SettingId::AdriaRefillsMana, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Adria Refills Mana",
		0, 0, 1, {}, {}, 0, "Адрия восполняет ману",
		"В лавке Адрии мана восстанавливается бесплатно, как здоровье у Пепла." },
	{ SettingId::DisableCripplingShrines, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Disable Crippling Shrines",
		0, 0, 1, {}, {}, 0, "Запрет коварных святилищ",
		"Святилища с губительными эффектами (котлы, Fascinating и подобные) нельзя активировать." },
	{ SettingId::QuickCast, SettingGroup::Gameplay, SettingKind::Toggle, "Game", "Quick Cast",
		0, 0, 1, {}, {}, 0, "Быстрое заклинание",
		"Клавиши заклинаний сразу произносят их, а не готовят в слот." },
	{ SettingId::HealPotionPickup, SettingGroup::Gameplay, SettingKind::Cycle, "Game", "Heal Potion Pickup",
		0, 0, 16,
		{ "Выкл", "1", "2", "4", "8", "16" }, { 0, 1, 2, 4, 8, 16 }, 6,
		"Подбор зелий лечения",
		"Сколько зелий лечения подбирается автоматически." },
	{ SettingId::ManaPotionPickup, SettingGroup::Gameplay, SettingKind::Cycle, "Game", "Mana Potion Pickup",
		0, 0, 16,
		{ "Выкл", "1", "2", "4", "8", "16" }, { 0, 1, 2, 4, 8, 16 }, 6,
		"Подбор зелий маны",
		"Сколько зелий маны подбирается автоматически." },
	{ SettingId::EnemyHealthBar, SettingGroup::Interface, SettingKind::Toggle, "Game", "Enemy Health Bar",
		0, 0, 1, {}, {}, 0, "Полоса здоровья врага",
		"Здоровье врага, по которому идёт бой, показывается полосой вверху экрана." },
	{ SettingId::ShowHealthValues, SettingGroup::Interface, SettingKind::Toggle, "Game", "Show health values",
		0, 0, 1, {}, {}, 0, "Точное здоровье",
		"Текущее и максимальное здоровье цифрами на глобусе жизни." },
	{ SettingId::ShowManaValues, SettingGroup::Interface, SettingKind::Toggle, "Game", "Show mana values",
		0, 0, 1, {}, {}, 0, "Точная мана",
		"Текущая и максимальная мана цифрами на глобусе маны." },
	{ SettingId::ExperienceBar, SettingGroup::Interface, SettingKind::Toggle, "Game", "Experience Bar",
		0, 0, 1, {}, {}, 0, "Полоса опыта",
		"Полоса набора опыта внизу экрана." },
	{ SettingId::ShowItemLabels, SettingGroup::Interface, SettingKind::Toggle, "Game", "Show Item Labels",
		0, 0, 1, {}, {}, 0, "Названия предметов",
		"Подписывать предметы, лежащие на земле." },
	{ SettingId::FloatingNumbers, SettingGroup::Interface, SettingKind::Cycle, "Game", "Enable floating numbers",
		0, 0, 2,
		{ "Выкл", "Случайные углы", "Вертикально", "", "", "" }, { 0, 1, 2, 0, 0, 0 }, 3,
		"Всплывающие числа",
		"Числа урона и опыта всплывают над персонажами." },
	{ SettingId::ShowMonsterType, SettingGroup::Interface, SettingKind::Toggle, "Game", "Show Monster Type",
		0, 0, 1, {}, {}, 0, "Тип монстра",
		"В описании монстра показывается его тип (зверь/демон/нежить)." },
	{ SettingId::Zoom, SettingGroup::Graphics, SettingKind::Toggle, "Graphics", "Zoom",
		0, 0, 1, {}, {}, 0, "Приближение",
		"Разрешает приближать и отдалять камеру в игре." },
	{ SettingId::ColorCycling, SettingGroup::Graphics, SettingKind::Toggle, "Graphics", "Color Cycling",
		1, 0, 1, {}, {}, 0, "Анимация палитры",
		"Живая анимация воды, лавы и кислоты (циклическая палитра)." },
	{ SettingId::FpsLimiter, SettingGroup::Graphics, SettingKind::Toggle, "Graphics", "FPS Limiter",
		1, 0, 1, {}, {}, 0, "Ограничение FPS",
		"Ограничивает частоту кадров — меньше нагрев и расход заряда." },
	{ SettingId::ShowFps, SettingGroup::Graphics, SettingKind::Toggle, "Graphics", "Show FPS",
		0, 0, 1, {}, {}, 0, "Счётчик FPS",
		"Показывает частоту кадров в углу экрана." },
	{ SettingId::GammaCorrection, SettingGroup::Graphics, SettingKind::Slider, "Graphics", "Gamma Correction",
		100, 75, 125, {}, {}, 0, "Яркость",
		"Гамма-коррекция картинки; 100 — стандартная яркость." },
	{ SettingId::SoundVolume, SettingGroup::Audio, SettingKind::PercentVolume, "Audio", "Sound Volume",
		100, 0, 100, {}, {}, 0, "Громкость звуков",
		"Звуки эффектов, интерфейса и голоса." },
	{ SettingId::MusicVolume, SettingGroup::Audio, SettingKind::PercentVolume, "Audio", "Music Volume",
		100, 0, 100, {}, {}, 0, "Громкость музыки",
		"Музыкальное сопровождение." },
	{ SettingId::WalkingSound, SettingGroup::Audio, SettingKind::Toggle, "Audio", "Walking Sound",
		1, 0, 1, {}, {}, 0, "Звук шагов",
		"Персонаж слышен при ходьбе." },
} };

/// Спецификация настройки по её идентификатору.
[[nodiscard]] constexpr const SettingSpec &SettingSpecOf(SettingId id)
{
	return kSettingCatalog[static_cast<size_t>(id)];
}

/// Индекс варианта цикла для текущего значения; 0, если значение
/// не совпадает ни с одним (нестандартное значение из правленого ini).
[[nodiscard]] constexpr int SettingCycleIndex(const SettingSpec &spec, int value)
{
	for (int i = 0; i < static_cast<int>(spec.optionCount); ++i) {
		if (spec.optionValues[static_cast<size_t>(i)] == value) {
			return i;
		}
	}
	return 0;
}

/// Группы по порядку вывода на экране.
struct SettingGroupSpec {
	SettingGroup group;
	std::string_view titleRu;
};

inline constexpr std::array<SettingGroupSpec, 4> kSettingGroups { {
	{ SettingGroup::Gameplay, "Геймплей" },
	{ SettingGroup::Interface, "Интерфейс" },
	{ SettingGroup::Graphics, "Графика" },
	{ SettingGroup::Audio, "Звук" },
} };

/// Значения по умолчанию для всех настроек, индекс — SettingId.
[[nodiscard]] constexpr std::array<int, kSettingCount> DefaultSettingValues()
{
	std::array<int, kSettingCount> values {};
	for (size_t i = 0; i < kSettingCount; ++i) {
		values[i] = kSettingCatalog[i].defaultInt;
	}
	return values;
}

/// Шкала громкости движка: логарифмическая, от тишины до максимума.
constexpr int kVolumeIniMin = -1600;
constexpr int kVolumeIniMax = 0;

/// Проценты (0..100) → единицы ini (−1600..0).
[[nodiscard]] constexpr int VolumePctToIni(int pct)
{
	return kVolumeIniMin + 16 * pct;
}

/// Единицы ini (−1600..0) → проценты (0..100), с округлением к ближайшему.
[[nodiscard]] constexpr int VolumeIniToPct(int ini)
{
	if (ini <= kVolumeIniMin) {
		return 0;
	}
	if (ini >= kVolumeIniMax) {
		return 100;
	}
	return (ini - kVolumeIniMin + 8) / 16;
}

} // namespace launcher
