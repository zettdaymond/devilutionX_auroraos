#include "core/Store.hpp"

#include "services/DesktopPathProvider.hpp"
#include "services/mocks/MockDownloadService.hpp"
#include "services/mocks/MockServices.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <thread>

namespace launcher {
namespace {

class StoreTest : public ::testing::Test {
protected:
	void construct(MockDownloadBehavior behavior = MockDownloadBehavior::InstantSuccess)
	{
		m_world = std::make_shared<MockWorld>();
		m_config = std::make_unique<MockConfigService>();
		m_files = std::make_unique<MockGameFilesService>(m_world);
		m_downloads = std::make_unique<MockDownloadService>(m_world, behavior);
		m_paths = std::make_unique<DesktopPathProvider>(
		    std::filesystem::temp_directory_path() / "devilutionx-launcher-test");
		m_engineOptions = std::make_unique<MockEngineOptionsService>();
		m_store = std::make_unique<Store>(*m_config, *m_files, *m_downloads, *m_paths, *m_engineOptions);
		m_store->Init();
	}

	void pump()
	{
		m_store->Poll();
	}

	/// Wait until the mock download thread finished delivering events.
	void waitForIdleDownload()
	{
		for (int i = 0; i < 500 && m_downloads->IsActive(); ++i) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		pump();
		pump();
	}

	[[nodiscard]] const LauncherState &state() const { return m_store->State(); }

	std::shared_ptr<MockWorld> m_world;
	std::unique_ptr<MockConfigService> m_config;
	std::unique_ptr<MockGameFilesService> m_files;
	std::unique_ptr<MockDownloadService> m_downloads;
	std::unique_ptr<DesktopPathProvider> m_paths;
	std::unique_ptr<MockEngineOptionsService> m_engineOptions;
	std::unique_ptr<Store> m_store;
};

TEST_F(StoreTest, InitialStateIsEmpty)
{
	construct();

	EXPECT_EQ(state().screen, Screen::Home);
	EXPECT_FALSE(state().HasAnyFiles());
	EXPECT_FALSE(state().diablo.available);
	EXPECT_FALSE(state().hellfire.available);
	EXPECT_FALSE(state().demo.available);
	EXPECT_FALSE(state().download.has_value());
}

TEST_F(StoreTest, WakeCallbackFiresOnlyForBackgroundDispatch)
{
	construct();

	std::atomic<int> wakes { 0 };
	m_store->SetWakeCallback([&wakes] { ++wakes; });

	// Интент с главного потока не будит: свой ввод цикл и так применит
	// в ближайшем кадре.
	m_store->Dispatch(intent::UiNavigate { Screen::About });
	pump();
	EXPECT_EQ(wakes.load(), 0);
	EXPECT_EQ(state().screen, Screen::About);

	// Интент из фонового потока (как прогресс загрузки из zoe) будит —
	// свёрнутый цикл спит в блокирующем ожидании и без пинка проспал бы
	// изменение состояния.
	std::thread background([&store = *m_store] {
		store.Dispatch(intent::UiNavigate { Screen::Data });
	});
	background.join();
	EXPECT_EQ(wakes.load(), 1);

	pump();
	EXPECT_EQ(state().screen, Screen::Data);
}

TEST_F(StoreTest, FolderSelectedScansAndEnablesDiablo)
{
	construct();

	m_world->SetPresent(KnownFile::Diabdat, 500);
	m_store->Dispatch(intent::DataFolderSelected { std::filesystem::temp_directory_path() });
	pump();

	EXPECT_TRUE(state().diablo.available);
	ASSERT_TRUE(m_config->LastSaved().dataFolder.has_value());
}

TEST_F(StoreTest, LaunchDiabloSetsPendingLaunch)
{
	construct();
	m_world->SetPresent(KnownFile::Diabdat, 500);
	m_store->Dispatch(intent::RescanFiles {});
	pump();

	m_store->Dispatch(intent::LaunchGame { ExitAction::LaunchDiablo });
	pump();

	ASSERT_TRUE(state().pendingLaunch.has_value());
	EXPECT_EQ(*state().pendingLaunch, ExitAction::LaunchDiablo);
}

TEST_F(StoreTest, HellfirePartialOpensMissingFilesDialog)
{
	construct();
	m_world->ApplyPreset("hellfire-partial");
	m_store->Dispatch(intent::RescanFiles {});
	pump();

	m_store->Dispatch(intent::LaunchGame { ExitAction::LaunchHellfire });
	pump();

	EXPECT_EQ(state().dialog, Dialog::HellfireMissingFiles);
	EXPECT_FALSE(state().pendingLaunch.has_value());
	ASSERT_EQ(state().hellfireMissing.size(), 2U);
}

TEST_F(StoreTest, DemoWithoutFileOpensConfirmDialog)
{
	construct();

	m_store->Dispatch(intent::LaunchGame { ExitAction::LaunchDemo });
	pump();

	EXPECT_EQ(state().dialog, Dialog::ConfirmDownloadDemo);
	EXPECT_FALSE(state().pendingLaunch.has_value());
}

TEST_F(StoreTest, DemoWithFileLaunches)
{
	construct();
	m_world->SetPresent(KnownFile::Spawn, 52 * 1024 * 1024);
	m_store->Dispatch(intent::RescanFiles {});
	pump();

	m_store->Dispatch(intent::LaunchGame { ExitAction::LaunchDemo });
	pump();

	ASSERT_TRUE(state().pendingLaunch.has_value());
	EXPECT_EQ(*state().pendingLaunch, ExitAction::LaunchDemo);
}

TEST_F(StoreTest, DownloadSuccessMakesDemoAvailable)
{
	construct();

	m_store->Dispatch(intent::StartDownload { KnownFile::Spawn });
	pump();
	EXPECT_EQ(state().dialog, Dialog::DownloadProgress);
	ASSERT_TRUE(state().download.has_value());
	EXPECT_TRUE(state().download->active);

	waitForIdleDownload();

	EXPECT_FALSE(state().download.has_value()) << "download state must be cleared";
	EXPECT_EQ(state().dialog, Dialog::None);
	EXPECT_TRUE(state().demo.available);
	EXPECT_TRUE(state().toast.has_value());
}

TEST_F(StoreTest, DownloadProgressUpdatesFraction)
{
	construct(MockDownloadBehavior::SlowSuccess);

	m_store->Dispatch(intent::StartDownload { KnownFile::Spawn });
	pump();
	ASSERT_TRUE(state().download.has_value());

	m_store->Dispatch(intent::EvDownloadProgress { 1000, 250, 100 });
	pump();

	EXPECT_FLOAT_EQ(state().download->fraction, 0.25F);
	EXPECT_EQ(state().download->downloadedBytes, 250);

	m_store->Dispatch(intent::CancelDownload {});
	pump();
	waitForIdleDownload();

	EXPECT_FALSE(state().download.has_value());
	EXPECT_FALSE(state().demo.available) << "cancelled download must not create the file";
}

TEST_F(StoreTest, DownloadFailureShowsErrorInOverlay)
{
	construct(MockDownloadBehavior::FailAtHalf);

	m_store->Dispatch(intent::StartDownload { KnownFile::Spawn });
	pump();
	waitForIdleDownload();

	// The overlay stays open with the failure; closing it clears everything.
	ASSERT_TRUE(state().download.has_value());
	EXPECT_FALSE(state().download->active);
	EXPECT_FALSE(state().download->error.empty());

	m_store->Dispatch(intent::UiCloseDialog {});
	pump();
	EXPECT_FALSE(state().download.has_value());
}

TEST_F(StoreTest, InsufficientDiskSpaceRefusesDownload)
{
	construct();
	m_world->SetFreeSpace(1024);

	m_store->Dispatch(intent::StartDownload { KnownFile::Spawn });
	pump();

	EXPECT_EQ(state().dialog, Dialog::Error);
	EXPECT_FALSE(state().download.has_value());
}

TEST_F(StoreTest, DeleteDownloadedFileRescans)
{
	construct();
	m_world->SetPresent(KnownFile::RuVoice, 150 * 1024 * 1024);
	m_store->Dispatch(intent::RescanFiles {});
	pump();
	EXPECT_TRUE(state().russianVoiceInstalled);

	m_store->Dispatch(intent::DeleteDownloadedFile { KnownFile::RuVoice });
	pump();

	EXPECT_FALSE(state().russianVoiceInstalled);
	EXPECT_TRUE(state().toast.has_value());
}

// ---- Настройки движка ----

TEST_F(StoreTest, InitLoadsEngineSettings)
{
	construct();

	EXPECT_TRUE(state().settingsLoaded);
	EXPECT_EQ(state().settingValues, DefaultSettingValues());
}

TEST_F(StoreTest, NavigateToSettingsScreen)
{
	construct();

	m_store->Dispatch(intent::UiNavigate { Screen::Settings });
	pump();

	EXPECT_EQ(state().screen, Screen::Settings);
}

TEST_F(StoreTest, SettingChangedUpdatesStateAndPersists)
{
	construct();

	m_store->Dispatch(intent::SettingChanged { SettingId::RunInTown, 1 });
	pump();

	EXPECT_EQ(state().settingValues[static_cast<size_t>(SettingId::RunInTown)], 1);
	ASSERT_EQ(m_engineOptions->Saves().size(), 1U);
	EXPECT_EQ(m_engineOptions->Saves()[0][static_cast<size_t>(SettingId::RunInTown)], 1);
}

TEST_F(StoreTest, VolumeSettingStoredAsPercent)
{
	construct();

	m_store->Dispatch(intent::SettingChanged { SettingId::SoundVolume, 30 });
	pump();

	EXPECT_EQ(state().settingValues[static_cast<size_t>(SettingId::SoundVolume)], 30);
	EXPECT_EQ(m_engineOptions->Saves().size(), 1U);
}

TEST_F(StoreTest, SettingsResetRestoresDefaults)
{
	construct();
	m_store->Dispatch(intent::SettingChanged { SettingId::RunInTown, 1 });
	m_store->Dispatch(intent::SettingChanged { SettingId::GammaCorrection, 90 });
	pump();
	ASSERT_EQ(m_engineOptions->Saves().size(), 2U);

	m_store->Dispatch(intent::SettingsReset {});
	pump();

	EXPECT_EQ(state().settingValues, DefaultSettingValues());
	EXPECT_EQ(m_engineOptions->Saves().size(), 3U);
	EXPECT_EQ(m_engineOptions->Saves()[2], DefaultSettingValues());
	ASSERT_TRUE(state().toast.has_value());
	EXPECT_EQ(state().dialog, Dialog::None);
}

TEST_F(StoreTest, ResetDialogFlow)
{
	construct();

	m_store->Dispatch(intent::UiOpenDialog { Dialog::ConfirmResetSettings });
	pump();
	EXPECT_EQ(state().dialog, Dialog::ConfirmResetSettings);

	m_store->Dispatch(intent::UiCloseDialog {});
	pump();
	EXPECT_EQ(state().dialog, Dialog::None);
	EXPECT_EQ(m_engineOptions->Saves().size(), 0U);
}

} // namespace
} // namespace launcher
