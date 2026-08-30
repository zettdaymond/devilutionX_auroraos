#pragma once

#include "../widgets/Widgets.hpp"

namespace launcher::ui::dialogs {

using widgets::Dispatcher;

/// Modal confirmations and the download overlay. Each dialog is driven
/// entirely by LauncherState and only dispatches intents.
namespace confirm {

/// ConfirmDownloadDemo / ConfirmDownloadRu.
void Download(const LauncherState &state, const Dispatcher &dispatch, KnownFile file);

} // namespace confirm

namespace overlay {

/// Fullscreen download progress overlay with speed, ETA, cancel and
/// the failure panel (retry / close).
void Download(const LauncherState &state, const Dispatcher &dispatch);

} // namespace overlay

/// "Missing Hellfire files" warning.
void MissingFiles(const LauncherState &state, const Dispatcher &dispatch);

/// Simple error box (state.errorText).
void Error(const LauncherState &state, const Dispatcher &dispatch);

/// Bottom transient notification. The view tracks display time
/// locally and dispatches UiDismissToast after ~3 s.
void Toast(const LauncherState &state, const Dispatcher &dispatch);

/// Opens the ImGui popup matching the dialog stored in state.
/// The view calls this when state.dialog changes (ImGui popups must be
/// opened explicitly before BeginPopupModal renders them).
void OpenFor(Dialog dialog);

/// Popup id used for the download overlay (needed to close it
/// programmatically).
const char *DownloadOverlayId();

} // namespace launcher::ui::dialogs
