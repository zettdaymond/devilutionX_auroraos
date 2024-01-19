#include "DisplayBlankerController.hpp"

#include <assert.h>
#include <optional>

#include <SDL2/SDL.h>

#include "DBus.hpp"

namespace devilution
{

constexpr static inline auto kMceService = "com.nokia.mce";
constexpr static inline auto kMceRequestPath = "/com/nokia/mce/request";
constexpr static inline auto kMceRequestIf = "com.nokia.mce.request";

constexpr static auto kMcePreventBlankReq = "req_display_blanking_pause";
constexpr static auto kMceCancelPreventBlankReq = "req_display_cancel_blanking_pause";

constexpr static auto kDbusTypeInvalid = ((int) '\0');

constexpr static auto kRequestBlankingPauseIntervalMs = 60 * 1000;

static std::mutex sync_mutex;

static SDL_TimerCallback kSDLTimerCallback = [](Uint32 interval, void *param) -> Uint32
{
    // SDL timer invoke callback from another thread
    std::lock_guard lock(sync_mutex);

    DBus::CallVoidMethod(kMceService, kMceRequestPath, kMceRequestIf, kMcePreventBlankReq, kDbusTypeInvalid);
    return interval;
};

struct DisplayBlankerController::Impl
{

    std::optional<SDL_TimerID> m_timer;
};

std::unique_ptr<DisplayBlankerController::Impl> DisplayBlankerController::m_impl = nullptr;

void DisplayBlankerController::Init()
{
    if(!m_impl) {
        DBus::Init();
        m_impl = std::make_unique<DisplayBlankerController::Impl>();
    }
}

void DisplayBlankerController::Shutdown()
{
    if(m_impl) {
        if(m_impl->m_timer) {
            SDL_RemoveTimer(m_impl->m_timer.value());
        }
        DBus::Shutdown();
    }
}

bool DisplayBlankerController::IsInitialized()
{
    return m_impl != nullptr;
}

void DisplayBlankerController::SetPreventDisplayBlanking(bool value)
{
    // SDL timer invoke callback from another thread
    std::lock_guard lock(sync_mutex);

    assert(m_impl != nullptr);

    if(value && !m_impl->m_timer) {
        auto result = DBus::CallVoidMethod(kMceService, kMceRequestPath, kMceRequestIf, kMcePreventBlankReq, kDbusTypeInvalid);

        if(!result) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not send DBus MCE Request for preventing screen blank");
        }
        else {
            m_impl->m_timer = SDL_AddTimer(kRequestBlankingPauseIntervalMs, kSDLTimerCallback, nullptr);
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Send DBus MCE Request for preventing screen blank and scedule periodic requests");
        }
    }
    else if(!value && m_impl->m_timer) {
        SDL_RemoveTimer(m_impl->m_timer.value());
        m_impl->m_timer = std::nullopt;

        auto result = DBus::CallVoidMethod(kMceService, kMceRequestPath, kMceRequestIf, kMceCancelPreventBlankReq, kDbusTypeInvalid);
        if(!result) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not send Dbbus MCE Request for cancel preventing screen blank");
        }
        else {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Send DBus MCE Request for cancel preventing screen blank and disable periodic requests");
        }
    }
}

}
