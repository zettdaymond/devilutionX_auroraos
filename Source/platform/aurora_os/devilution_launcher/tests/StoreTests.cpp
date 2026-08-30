#include "core/Store.hpp"

#include "services/DesktopPathProvider.hpp"
#include "services/mocks/MockDownloadService.hpp"
#include "services/mocks/MockServices.hpp"

#include <gtest/gtest.h>

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
		m_store = std::make_unique<Store>(*m_config, *m_files, *m_downloads, *m_paths);
		m_store->init();
	}

	void pump()
	{
		m_store->poll();
	}

	/// Wait until the mock download thread finished delivering events.
	void waitForIdleDownload()
	{
		for (int i = 0; i < 500 && m_downloads->isActive(); ++i) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		pump();
		pump();
	}

	[[nodiscard]] const LauncherState &state() const { return m_store->state(); }

	std::shared_ptr<MockWorld> m_world;
	std::unique_ptr<MockConfigService> m_config;
	std::unique_ptr<MockGameFilesService> m_files;
	std::unique_ptr<MockDownloadService> m_downloads;
	std::unique_ptr<DesktopPathProvider> m_paths;
	std::unique_ptr<Store> m_store;
};

TEST_F(StoreTest, InitialStateIsEmpty)
{
	construct();

	EXPECT_EQ(state().screen, Screen::Home);
	EXPECT_FALSE(state().hasAnyFiles());
	EXPECT_FALSE(state().diablo.available);
	EXPECT_FALSE(state().hellfire.available);
	EXPECT_FALSE(state().demo.available);
	EXPECT_FALSE(state().download.has_value());
}

TEST_F(StoreTest, FolderSelectedScansAndEnablesDiablo)
{
	construct();

	m_world->setPresent(KnownFile::Diabdat, 500);
	m_store->dispatch(intent::DataFolderSelected { std::filesystem::temp_directory_path() });
	pump();

	EXPECT_TRUE(state().diablo.available);
	ASSERT_TRUE(m_config->lastSaved().dataFolder.has_value());
}

TEST_F(StoreTest, LaunchDiabloSetsPendingLaunch)
{
	construct();
	m_world->setPresent(KnownFile::Diabdat, 500);
	m_store->dispatch(intent::RescanFiles {});
	pump();

	m_store->dispatch(intent::LaunchGame { ExitAction::LaunchDiablo });
	pump();

	ASSERT_TRUE(state().pendingLaunch.has_value());
	EXPECT_EQ(*state().pendingLaunch, ExitAction::LaunchDiablo);
}

TEST_F(StoreTest, HellfirePartialOpensMissingFilesDialog)
{
	construct();
	m_world->applyPreset("hellfire-partial");
	m_store->dispatch(intent::RescanFiles {});
	pump();

	m_store->dispatch(intent::LaunchGame { ExitAction::LaunchHellfire });
	pump();

	EXPECT_EQ(state().dialog, Dialog::HellfireMissingFiles);
	EXPECT_FALSE(state().pendingLaunch.has_value());
	ASSERT_EQ(state().hellfireMissing.size(), 2U);
}

TEST_F(StoreTest, DemoWithoutFileOpensConfirmDialog)
{
	construct();

	m_store->dispatch(intent::LaunchGame { ExitAction::LaunchDemo });
	pump();

	EXPECT_EQ(state().dialog, Dialog::ConfirmDownloadDemo);
	EXPECT_FALSE(state().pendingLaunch.has_value());
}

TEST_F(StoreTest, DemoWithFileLaunches)
{
	construct();
	m_world->setPresent(KnownFile::Spawn, 52 * 1024 * 1024);
	m_store->dispatch(intent::RescanFiles {});
	pump();

	m_store->dispatch(intent::LaunchGame { ExitAction::LaunchDemo });
	pump();

	ASSERT_TRUE(state().pendingLaunch.has_value());
	EXPECT_EQ(*state().pendingLaunch, ExitAction::LaunchDemo);
}

TEST_F(StoreTest, DownloadSuccessMakesDemoAvailable)
{
	construct();

	m_store->dispatch(intent::StartDownload { KnownFile::Spawn });
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

	m_store->dispatch(intent::StartDownload { KnownFile::Spawn });
	pump();
	ASSERT_TRUE(state().download.has_value());

	m_store->dispatch(intent::EvDownloadProgress { 1000, 250, 100 });
	pump();

	EXPECT_FLOAT_EQ(state().download->fraction, 0.25F);
	EXPECT_EQ(state().download->downloadedBytes, 250);

	m_store->dispatch(intent::CancelDownload {});
	pump();
	waitForIdleDownload();

	EXPECT_FALSE(state().download.has_value());
	EXPECT_FALSE(state().demo.available) << "cancelled download must not create the file";
}

TEST_F(StoreTest, DownloadFailureShowsErrorInOverlay)
{
	construct(MockDownloadBehavior::FailAtHalf);

	m_store->dispatch(intent::StartDownload { KnownFile::Spawn });
	pump();
	waitForIdleDownload();

	// The overlay stays open with the failure; closing it clears everything.
	ASSERT_TRUE(state().download.has_value());
	EXPECT_FALSE(state().download->active);
	EXPECT_FALSE(state().download->error.empty());

	m_store->dispatch(intent::UiCloseDialog {});
	pump();
	EXPECT_FALSE(state().download.has_value());
}

TEST_F(StoreTest, InsufficientDiskSpaceRefusesDownload)
{
	construct();
	m_world->setFreeSpace(1024);

	m_store->dispatch(intent::StartDownload { KnownFile::Spawn });
	pump();

	EXPECT_EQ(state().dialog, Dialog::Error);
	EXPECT_FALSE(state().download.has_value());
}

TEST_F(StoreTest, DeleteDownloadedFileRescans)
{
	construct();
	m_world->setPresent(KnownFile::RuVoice, 150 * 1024 * 1024);
	m_store->dispatch(intent::RescanFiles {});
	pump();
	EXPECT_TRUE(state().russianVoiceInstalled);

	m_store->dispatch(intent::DeleteDownloadedFile { KnownFile::RuVoice });
	pump();

	EXPECT_FALSE(state().russianVoiceInstalled);
	EXPECT_TRUE(state().toast.has_value());
}

} // namespace
} // namespace launcher
