#pragma once

#include <memory>

namespace devilution {

class DisplayBlankerController
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    static void SetPreventDisplayBlanking(bool value);

private:
    struct Impl;

    static std::unique_ptr<Impl> m_impl;
};

}
