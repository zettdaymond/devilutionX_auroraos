#include "core/EngineOptions.hpp"

#include "services/EngineOptionsService.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace launcher {
namespace {

class EngineOptionsTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		m_dir = std::filesystem::temp_directory_path()
		    / ("devilutionx-engine-options-test-" + std::to_string(++s_counter));
		std::filesystem::create_directories(m_dir);
		m_iniPath = m_dir / "diablo.ini";
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(m_dir, ec);
	}

	/// Пишет файл ini как его мог бы оставить движок.
	void WriteRawIni(const std::string &content)
	{
		std::ofstream stream(m_iniPath, std::ios::trunc);
		stream << content;
	}

	[[nodiscard]] std::string ReadRawIni() const
	{
		std::ifstream stream(m_iniPath);
		return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	}

	std::filesystem::path m_dir;
	std::filesystem::path m_iniPath;

	static int s_counter;
};

int EngineOptionsTest::s_counter = 0;

TEST(CatalogTest, CatalogMatchesEnumAndKeysUnique)
{
	EXPECT_EQ(kSettingCatalog.size(), kSettingCount);
	for (size_t i = 0; i < kSettingCatalog.size(); ++i) {
		EXPECT_EQ(kSettingCatalog[i].id, static_cast<SettingId>(i)) << "index " << i;
	}
	for (size_t i = 0; i < kSettingCatalog.size(); ++i) {
		for (size_t j = i + 1; j < kSettingCatalog.size(); ++j) {
			const bool sameKey = kSettingCatalog[i].section == kSettingCatalog[j].section
			    && kSettingCatalog[i].key == kSettingCatalog[j].key;
			EXPECT_FALSE(sameKey) << "duplicate key at " << i << " and " << j;
		}
	}
}

TEST(CatalogTest, CatalogOrderedByGroups)
{
	// Порядок строк на экране задаёт каталог: группы идут подряд.
	int lastGroup = -1;
	for (const SettingSpec &spec : kSettingCatalog) {
		const int group = static_cast<int>(spec.group);
		EXPECT_GE(group, lastGroup) << "group order broken at " << spec.key.data();
		lastGroup = group;
	}
}

TEST(CatalogTest, CycleOptionsAreWellFormed)
{
	for (const SettingSpec &spec : kSettingCatalog) {
		if (spec.kind != SettingKind::Cycle) {
			continue;
		}
		// Разрешение — цикл с внешним списком (secondaryKey непуст):
		// статические варианты пусты, список даёт состояние.
		if (!spec.secondaryKey.empty()) {
			EXPECT_EQ(spec.optionCount, 0) << spec.key.data();
			continue;
		}
		EXPECT_GE(spec.optionCount, 2) << spec.key.data();
		EXPECT_LE(spec.optionCount, kMaxSettingOptions) << spec.key.data();
		for (size_t i = 0; i < spec.optionCount; ++i) {
			EXPECT_FALSE(spec.optionNames[i].empty()) << spec.key.data() << " option " << i;
			for (size_t j = i + 1; j < spec.optionCount; ++j) {
				EXPECT_NE(spec.optionValues[i], spec.optionValues[j])
				    << spec.key.data() << " duplicate option value";
			}
		}
		// Степпер шире индексов не выходит.
		EXPECT_EQ(SettingCycleIndex(spec, spec.optionValues[0]), 0) << spec.key.data();
	}
}

TEST(CatalogTest, VolumeMapping)
{
	EXPECT_EQ(VolumePctToIni(0), -1600);
	EXPECT_EQ(VolumePctToIni(100), 0);
	EXPECT_EQ(VolumePctToIni(50), -800);
	EXPECT_EQ(VolumeIniToPct(-1600), 0);
	EXPECT_EQ(VolumeIniToPct(0), 100);
	EXPECT_EQ(VolumeIniToPct(-800), 50);
	// Движок пишет шагами по 25 — округление к ближайшему проценту.
	EXPECT_EQ(VolumeIniToPct(-775), 52);
	EXPECT_EQ(VolumeIniToPct(-2000), 0);
	EXPECT_EQ(VolumeIniToPct(100), 100);
}

TEST_F(EngineOptionsTest, MissingFileGivesDefaults)
{
	EngineOptionsService service(m_iniPath);
	const auto values = service.Load();
	EXPECT_EQ(values, DefaultSettingValues());
}

TEST_F(EngineOptionsTest, RoundTripThroughFile)
{
	std::array<int, kSettingCount> values = DefaultSettingValues();
	values[static_cast<size_t>(SettingId::RunInTown)] = 1;
	values[static_cast<size_t>(SettingId::GammaCorrection)] = 110;
	values[static_cast<size_t>(SettingId::MusicVolume)] = 25;

	EngineOptionsService writer(m_iniPath);
	writer.SaveAll(values);

	EngineOptionsService reader(m_iniPath);
	const auto loaded = reader.Load();
	EXPECT_EQ(loaded, values);
}

TEST_F(EngineOptionsTest, PreservesForeignKeysAndSections)
{
	WriteRawIni("[Network]\nbindip=0.0.0.0\n[Keymapper]\nquickSpell1=F1\n[Game]\nRun in Town=1\n");

	std::array<int, kSettingCount> values = DefaultSettingValues();
	values[static_cast<size_t>(SettingId::RunInTown)] = 0;
	values[static_cast<size_t>(SettingId::CowQuest)] = 1;

	EngineOptionsService service(m_iniPath);
	service.SaveAll(values);

	const std::string content = ReadRawIni();
	EXPECT_NE(content.find("bindip=0.0.0.0"), std::string::npos);
	EXPECT_NE(content.find("quickSpell1=F1"), std::string::npos);
	EXPECT_NE(content.find("Run in Town=0"), std::string::npos);
	EXPECT_NE(content.find("Cow Quest=1"), std::string::npos);
}

TEST_F(EngineOptionsTest, BoolSerializedLikeEngine)
{
	// Движок пишет булевы как 1/0 через SetLongValue, без пробелов
	// вокруг '=' и без true/false.
	std::array<int, kSettingCount> values = DefaultSettingValues();
	values[static_cast<size_t>(SettingId::AutoGoldPickup)] = 1;

	EngineOptionsService service(m_iniPath);
	service.SaveAll(values);

	const std::string content = ReadRawIni();
	EXPECT_NE(content.find("Auto Gold Pickup=1"), std::string::npos);
	EXPECT_EQ(content.find("Auto Gold Pickup ="), std::string::npos);
	EXPECT_EQ(content.find("=true"), std::string::npos);
	EXPECT_EQ(content.find("=false"), std::string::npos);
}

TEST_F(EngineOptionsTest, VolumeWrittenOnEngineScale)
{
	std::array<int, kSettingCount> values = DefaultSettingValues();
	values[static_cast<size_t>(SettingId::SoundVolume)] = 50;
	values[static_cast<size_t>(SettingId::MusicVolume)] = 0;

	EngineOptionsService service(m_iniPath);
	service.SaveAll(values);

	const std::string content = ReadRawIni();
	EXPECT_NE(content.find("Sound Volume=-800"), std::string::npos);
	EXPECT_NE(content.find("Music Volume=-1600"), std::string::npos);
}

TEST_F(EngineOptionsTest, EngineWrittenVolumesReadBack)
{
	// Значения, записанные движком шагами по 25, читаются процентами.
	WriteRawIni("[Audio]\nSound Volume=-775\n");

	EngineOptionsService service(m_iniPath);
	const auto values = service.Load();
	EXPECT_EQ(values[static_cast<size_t>(SettingId::SoundVolume)], 52);
}

TEST_F(EngineOptionsTest, CorruptFileFallsBackToDefaults)
{
	WriteRawIni("\xFF\xFE not an ini [[[");

	EngineOptionsService service(m_iniPath);
	const auto values = service.Load();
	EXPECT_EQ(values, DefaultSettingValues());
}

TEST_F(EngineOptionsTest, NoTmpFileLeftAfterSave)
{
	EngineOptionsService service(m_iniPath);
	service.SaveAll(DefaultSettingValues());

	EXPECT_TRUE(std::filesystem::exists(m_iniPath));
	EXPECT_FALSE(std::filesystem::exists(m_dir / "diablo.ini.tmp"));
}

TEST_F(EngineOptionsTest, PotionCycleValuesRoundTrip)
{
	// Степпер зелий отправляет значения варианта (0/1/2/4/8/16) —
	// они должны переживать запись в ini как есть.
	std::array<int, kSettingCount> values = DefaultSettingValues();
	values[static_cast<size_t>(SettingId::HealPotionPickup)] = 4;
	values[static_cast<size_t>(SettingId::ManaPotionPickup)] = 16;

	EngineOptionsService service(m_iniPath);
	service.SaveAll(values);

	const std::string content = ReadRawIni();
	EXPECT_NE(content.find("Heal Potion Pickup=4"), std::string::npos);
	EXPECT_NE(content.find("Mana Potion Pickup=16"), std::string::npos);

	const auto loaded = service.Load();
	EXPECT_EQ(loaded[static_cast<size_t>(SettingId::HealPotionPickup)], 4);
	EXPECT_EQ(loaded[static_cast<size_t>(SettingId::ManaPotionPickup)], 16);
}

TEST_F(EngineOptionsTest, OutOfRangeValuesClampedOnSave)
{
	std::array<int, kSettingCount> values = DefaultSettingValues();
	values[static_cast<size_t>(SettingId::RunInTown)] = 7;    // bool
	values[static_cast<size_t>(SettingId::GammaCorrection)] = 400; // slider 75..125

	EngineOptionsService service(m_iniPath);
	service.SaveAll(values);

	const auto loaded = service.Load();
	EXPECT_EQ(loaded[static_cast<size_t>(SettingId::RunInTown)], 1);
	EXPECT_EQ(loaded[static_cast<size_t>(SettingId::GammaCorrection)], 125);
}

TEST_F(EngineOptionsTest, ResolutionWritesAspectCorrectedWidth)
{
	std::array<int, kSettingCount> values = DefaultSettingValues();
	values[static_cast<size_t>(SettingId::Resolution)] = 540;

	EngineOptionsService service(m_iniPath);
	service.SetResolutionAspect(1440, 720); // телефон 2:1
	service.SaveAll(values);

	const std::string content = ReadRawIni();
	EXPECT_NE(content.find("Height=540"), std::string::npos);
	EXPECT_NE(content.find("Width=1080"), std::string::npos);

	const auto loaded = service.Load();
	EXPECT_EQ(loaded[static_cast<size_t>(SettingId::Resolution)], 540);
}

TEST_F(EngineOptionsTest, ResolutionSurvivesSaveOfOtherSettings)
{
	// Выбранное в игре 2160p (или экзотика) обязано пережить запись
	// ЛЮБЫХ настроек лаунчера: ни снапа, ни clamp-а.
	WriteRawIni("[Graphics]\nWidth=3840\nHeight=2160\n");

	std::array<int, kSettingCount> values;
	{
		EngineOptionsService reader(m_iniPath);
		values = reader.Load();
	}
	EXPECT_EQ(values[static_cast<size_t>(SettingId::Resolution)], 2160);
	values[static_cast<size_t>(SettingId::RunInTown)] = 1;

	EngineOptionsService writer(m_iniPath);
	writer.SetResolutionAspect(16, 9);
	writer.SaveAll(values);

	EngineOptionsService recheck(m_iniPath);
	EXPECT_EQ(recheck.Load()[static_cast<size_t>(SettingId::Resolution)], 2160);
}

TEST(ResolutionOptionsTest, MirrorsEngineList)
{
	// Телефон (один режим 720): общие ступени до экрана + экран + 480.
	const auto phone = BuildResolutionOptions({ 720 }, 480);
	EXPECT_EQ(phone, (std::vector<int> { 480, 540, 720 }));

	// Планшет 2000x1200 (один режим): до 1080 + нативная 1200.
	const auto tablet = BuildResolutionOptions({ 1200 }, 480);
	EXPECT_EQ(tablet, (std::vector<int> { 480, 540, 720, 960, 1080, 1200 }));

	// Десктоп со многими режимами: общих ступеней НЕТ (как в движке),
	// только режимы + сырое значение ini + вечные 480.
	const auto desktop = BuildResolutionOptions({ 1234, 664, 617, 480 }, 1234);
	EXPECT_EQ(desktop, (std::vector<int> { 480, 617, 664, 1234 }));

	// Экзотическое значение из ini входит в список (движок гарантирует
	// присутствие текущего выбора); список — по возрастанию.
	const auto exotic = BuildResolutionOptions({ 720 }, 2034);
	EXPECT_EQ(exotic.back(), 2034);
	EXPECT_EQ(exotic, (std::vector<int> { 480, 540, 720, 2034 }));
}

} // namespace
} // namespace launcher
