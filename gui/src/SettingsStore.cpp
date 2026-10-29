#include "SettingsStore.hpp"

#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace rtype::gui {
namespace {

template <class Value> Value number(const std::string& text)
{
    Value result{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        throw std::invalid_argument("Invalid settings number");
    return result;
}

bool boolean(const std::string& text)
{
    if (text != "0" && text != "1")
        throw std::invalid_argument("Invalid settings boolean");
    return text == "1";
}

void replaceFile(const std::filesystem::path& temporary, const std::filesystem::path& path)
{
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::system_error(static_cast<int>(GetLastError()), std::system_category());
#else
    std::filesystem::rename(temporary, path);
#endif
}

} // namespace

std::filesystem::path SettingsStore::defaultPath()
{
#ifdef _WIN32
    if (const char* directory = std::getenv("APPDATA"); directory && directory[0])
        return std::filesystem::path(directory) / "RType" / "settings.conf";
#else
    if (const char* directory = std::getenv("XDG_CONFIG_HOME"); directory && directory[0]) {
        const std::filesystem::path root(directory);
        if (root.is_absolute())
            return root / "rtype" / "settings.conf";
    }
    if (const char* directory = std::getenv("HOME"); directory && directory[0])
        return std::filesystem::path(directory) / ".config" / "rtype" / "settings.conf";
#endif
    return {};
}

void SettingsStore::validate(const GuiPreferences& preferences)
{
    if (!std::isfinite(preferences.audio.volume) || preferences.audio.volume < 0.0F ||
        preferences.audio.volume > 1.0F)
        throw std::invalid_argument("Volume must be between 0 and 1");
    if (!std::isfinite(preferences.visuals.textScale) || preferences.visuals.textScale < 1.0F ||
        preferences.visuals.textScale > 1.5F)
        throw std::invalid_argument("UI text scale must be between 1 and 1.5");
    if (preferences.visuals.tint < TintMode::Off || preferences.visuals.tint > TintMode::Amber ||
        !std::isfinite(preferences.visuals.tintOpacity) || preferences.visuals.tintOpacity < 0.0F ||
        preferences.visuals.tintOpacity > 0.5F)
        throw std::invalid_argument("Invalid scene tint settings");
    for (const auto key : preferences.keyboard.keys) {
        if (static_cast<std::size_t>(key) >= guiKeyCount)
            throw std::invalid_argument("Invalid keyboard binding");
    }
}

bool SettingsStore::load(const std::filesystem::path& path, GuiPreferences& output,
                         std::string& error)
{
    error.clear();
    try {
        if (path.empty() || !std::filesystem::exists(path))
            return false;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        const auto length = file.tellg();
        if (!file || length < 0 || length > 16384)
            throw std::runtime_error("Settings file cannot be read or exceeds 16 KiB");
        file.seekg(0);
        std::string bytes(static_cast<std::size_t>(length), '\0');
        if (!file.read(bytes.data(), static_cast<std::streamsize>(bytes.size())))
            throw std::runtime_error("Settings file could not be read completely");
        std::unordered_map<std::string, std::string> values;
        std::istringstream lines(bytes);
        for (std::string line; std::getline(lines, line);) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty() || line.front() == '#')
                continue;
            const auto separator = line.find('=');
            if (separator == std::string::npos ||
                !values.emplace(line.substr(0, separator), line.substr(separator + 1)).second)
                throw std::runtime_error("Malformed or duplicate settings entry");
        }
        if (!values.contains("version") || number<int>(values.at("version")) != 1)
            throw std::runtime_error("Unsupported settings version");
        GuiPreferences next;
        if (values.contains("volume"))
            next.audio.volume = number<float>(values.at("volume"));
        if (values.contains("muted"))
            next.audio.muted = boolean(values.at("muted"));
        if (values.contains("show-fps"))
            next.visuals.showFps = boolean(values.at("show-fps"));
        if (values.contains("motion"))
            next.visuals.backgroundMotion = boolean(values.at("motion"));
        if (values.contains("scale"))
            next.visuals.textScale = number<float>(values.at("scale"));
        if (values.contains("tint"))
            next.visuals.tint = static_cast<TintMode>(number<int>(values.at("tint")));
        if (values.contains("tint-opacity"))
            next.visuals.tintOpacity = number<float>(values.at("tint-opacity"));
        for (std::size_t index = 0; index < playerActionCount; ++index) {
            const auto key = "key." + std::to_string(index);
            if (values.contains(key))
                next.keyboard.keys[index] =
                    static_cast<GuiKey>(number<std::size_t>(values.at(key)));
        }
        validate(next);
        output = next;
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

bool SettingsStore::save(const std::filesystem::path& path, const GuiPreferences& preferences,
                         std::string& error)
{
    error.clear();
    std::filesystem::path temporary;
    try {
        validate(preferences);
        if (path.empty())
            throw std::runtime_error("No settings location is available");
        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        temporary = path;
        temporary +=
            ".tmp-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file.imbue(std::locale::classic());
        file << std::setprecision(9) << "version=1\nvolume=" << preferences.audio.volume
             << "\nmuted=" << preferences.audio.muted
             << "\nshow-fps=" << preferences.visuals.showFps
             << "\nmotion=" << preferences.visuals.backgroundMotion
             << "\ntint=" << static_cast<int>(preferences.visuals.tint)
             << "\nscale=" << preferences.visuals.textScale
             << "\ntint-opacity=" << preferences.visuals.tintOpacity << '\n';
        for (std::size_t index = 0; index < playerActionCount; ++index)
            file << "key." << index << '='
                 << static_cast<std::size_t>(preferences.keyboard.keys[index]) << '\n';
        file.flush();
        const bool written = static_cast<bool>(file);
        file.close();
        if (!written || file.fail())
            throw std::runtime_error("Settings could not be written");
        replaceFile(temporary, path);
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        std::error_code ignored;
        if (!temporary.empty())
            std::filesystem::remove(temporary, ignored);
        return false;
    }
}

} // namespace rtype::gui
