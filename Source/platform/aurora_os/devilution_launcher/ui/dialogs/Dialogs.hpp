#pragma once

#include "../widgets/Widgets.hpp"

namespace launcher::ui::dialogs {

using widgets::Dispatcher;

/// Модальные подтверждения и оверлей загрузки. Каждый диалог целиком
/// управляется LauncherState и только отправляет интенты.
namespace confirm {

/// ConfirmDownloadDemo / ConfirmDownloadRu.
void Download(const LauncherState &state, const Dispatcher &dispatch, KnownFile file);

/// ConfirmResetSettings — сброс настроек к значениям по умолчанию.
void ResetSettings(const LauncherState &state, const Dispatcher &dispatch);

} // namespace confirm

namespace overlay {

/// Оверлей загрузки на весь экран: скорость, оставшееся время,
/// отмена и панель ошибки (повторить / закрыть).
void Download(const LauncherState &state, const Dispatcher &dispatch);

} // namespace overlay

/// Предупреждение «не хватает файлов Hellfire».
void MissingFiles(const LauncherState &state, const Dispatcher &dispatch);

/// Простое окно ошибки (state.errorText).
void Error(const LauncherState &state, const Dispatcher &dispatch);

/// Короткое уведомление внизу экрана. Время показа считает сам вид,
/// через ~3 с отправляет UiDismissToast.
void Toast(const LauncherState &state, const Dispatcher &dispatch);

/// Открывает попап ImGui, соответствующий диалогу из состояния.
/// Вид зовёт это при смене state.dialog: попапы ImGui нужно
/// открывать явно до отрисовки BeginPopupModal.
void OpenFor(Dialog dialog);

/// Идентификатор попапа оверлея загрузки (нужен для закрытия
/// программно).
const char *DownloadOverlayId();

} // namespace launcher::ui::dialogs
