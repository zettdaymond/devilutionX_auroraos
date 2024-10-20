#pragma once

#include <string>
#include <unordered_map>

#include <imgui.h>

#include <cmrc/cmrc.hpp>
CMRC_DECLARE(assets);

class FontStorage {
public:
    static void loadFonts()
    {
        static const ImWchar kAwesomeIconsRanges[] = {
           0xf000,
           0xf3ff,
           0,
        };

        auto fs = cmrc::assets::get_filesystem();
        ImGuiIO& io{ImGui::GetIO()};

        const float default_font_size = 22.0f;

        auto font = fs.open("assets/Beaufort-Regular.ttf");
        auto defaultFont = io.Fonts->AddFontFromMemoryTTF((void*)font.begin(), font.size(), default_font_size);

        ImFontConfig config;
        config.MergeMode = true;

        io.Fonts->AddFontFromMemoryTTF((void*)font.begin(),
                                       font.size(),
                                       default_font_size,
                                       &config,
                                       io.Fonts->GetGlyphRangesCyrillic());

        auto font_awesome_data = fs.open("assets/fontawesome-webfont.ttf");
        io.Fonts->AddFontFromMemoryTTF((void*)font_awesome_data.begin(),
                                       font_awesome_data.size(),
                                       default_font_size,
                                       &config,
                                       kAwesomeIconsRanges);

        // Шрифт для иконок
        ImFontConfig config2;
        config2.MergeMode = false;
        config2.GlyphMinAdvanceX = 48.0f; // Минимальный размер иконок
        auto largeIconFont = io.Fonts->AddFontFromMemoryTTF((void*)font_awesome_data.begin(),
                                                            font_awesome_data.size(),
                                                            48.0f,
                                                            &config2,
                                                            kAwesomeIconsRanges);

        // Оффициальный шрифт Diablo
        auto diabloFontAsset = fs.open("assets/exocet2.ttf");

        ImFontConfig config3;
        config2.MergeMode = false;
        auto diabloFont = io.Fonts->AddFontFromMemoryTTF((void*)diabloFontAsset.begin(),
                                                         diabloFontAsset.size(),
                                                         48.0f,
                                                         &config3);

        fonts["defaultFont"] = defaultFont;
        fonts["largeIconFont"] = largeIconFont;
        fonts["diabloFont"] = diabloFont;
    }

    static ImFont* getFont(const std::string& fontName)
    {
        if (fonts.find(fontName) != fonts.end()) {
            return fonts[fontName];
        }
        return nullptr;
    }

private:
    static inline std::unordered_map<std::string, ImFont*> fonts =
       {}; // Maps font names to their corresponding sf::Font objects
};
