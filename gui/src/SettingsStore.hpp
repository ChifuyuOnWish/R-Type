#pragma once

#include "GuiTypes.hpp"

#include <filesystem>

namespace rtype::gui {

class SettingsStore
{
public:
    [[nodiscard]] static std::filesystem::path defaultPath();
    static void validate(const GuiPreferences& preferences);
    [[nodiscard]] static bool load(const std::filesystem::path& path, GuiPreferences& output,
                                   std::string& error);
    [[nodiscard]] static bool save(const std::filesystem::path& path,
                                   const GuiPreferences& preferences, std::string& error);
};

} // namespace rtype::gui
