#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace launcher {

/// Downloads one file at a time. Callbacks may be invoked from a
/// background thread — implementations guarantee they are called,
/// but the receiver must marshal data to its own thread
/// (the Store does this via dispatch()).
class IDownloadService {
public:
	struct Listener {
		/// Progress report; bytesPerSec is a smoothed instantaneous speed.
		std::function<void(int64_t totalBytes, int64_t downloadedBytes, int64_t bytesPerSec)> onProgress;
		/// Terminal event; always called exactly once per Start().
		/// On cancel or failure `success` is false and `error` explains why.
		std::function<void(bool success, std::string error)> onFinished;
	};

	virtual ~IDownloadService() = default;

	/// Begin downloading `url` into `destination` (file path).
	/// Starting while another download is active is a programming error;
	/// implementations log and ignore such calls.
	virtual void Start(const std::string &url, const std::filesystem::path &destination, Listener listener) = 0;

	/// Abort the active download. Triggers onFinished(false, "cancelled")
	/// and removes the partial file.
	virtual void Cancel() = 0;

	[[nodiscard]] virtual bool IsActive() = 0;
};

} // namespace launcher
