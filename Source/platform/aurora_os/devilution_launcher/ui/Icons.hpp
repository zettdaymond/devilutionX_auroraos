#pragma once

/// FontAwesome 4 codepoints (UTF-8 byte sequences) used by the launcher
/// UI. Render with a font that has the icon range merged in
/// (see Theme::init).
namespace launcher::ui::icons {

inline constexpr const char *Home = "\xEF\x80\x95";          // U+F015
inline constexpr const char *Folder = "\xEF\x81\xBB";        // U+F07B
inline constexpr const char *Info = "\xEF\x84\xA9";          // U+F129
inline constexpr const char *Download = "\xEF\x80\x99";      // U+F019
inline constexpr const char *Play = "\xEF\x81\x8B";          // U+F04B
inline constexpr const char *Lock = "\xEF\x80\xA3";          // U+F023
inline constexpr const char *Check = "\xEF\x80\x8C";         // U+F00C
inline constexpr const char *Times = "\xEF\x80\x8D";         // U+F00D
inline constexpr const char *Trash = "\xEF\x87\xB8";         // U+F1F8
inline constexpr const char *Hdd = "\xEF\x82\xA0";           // U+F0A0
inline constexpr const char *Gamepad = "\xEF\x84\x9B";       // U+F11B
inline constexpr const char *Fire = "\xEF\x81\xAD";          // U+F06D
inline constexpr const char *Globe = "\xEF\x82\xAC";         // U+F0AC
inline constexpr const char *Refresh = "\xEF\x80\xA1";       // U+F021
inline constexpr const char *Exclamation = "\xEF\x84\xAA";   // U+F12A
inline constexpr const char *Music = "\xEF\x80\x81";         // U+F001
inline constexpr const char *Book = "\xEF\x80\xAD";          // U+F02D

} // namespace launcher::ui::icons
