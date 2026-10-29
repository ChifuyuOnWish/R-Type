#include "SettingsMenu.hpp"
#include "SettingsStore.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace rtype::gui;
namespace {
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void menuTest()
{
    SettingsMenu menu;
    menu.setTextMeasure([](const std::string& value, float size) {
        return static_cast<float>(value.size()) * (size * 0.5F + 1.0F);
    });
    GuiPreferences original;
    menu.open(original);
    MenuInput activate;
    activate.activate = true;
    menu.select(1);
    (void)menu.handle(activate);
    require(menu.draft().audio.muted && !original.audio.muted, "Draft isolation");
    menu.select(4);
    (void)menu.handle(activate);
    require(menu.draft().visuals.tint == TintMode::Red, "Tint selection");
    menu.select(5);
    for (int index = 0; index < 20; ++index)
        (void)menu.handle(activate);
    require(menu.draft().visuals.tintOpacity == 0.5F, "Opacity is bounded");
    menu.select(6);
    (void)menu.handle(activate);
    require(menu.isCapturing(), "Binding capture");
    MenuInput binding;
    binding.key = GuiKey::Down;
    (void)menu.handle(binding);
    require(menu.draft().keyboard.keys[0] == GuiKey::Down &&
                menu.draft().keyboard.keys[1] == GuiKey::Up,
            "Conflicting bindings swap");
    menu.select(7);
    (void)menu.handle(activate);
    MenuInput cancel;
    cancel.cancelCapture = true;
    (void)menu.handle(cancel);
    require(menu.isOpen() && !menu.isCapturing(), "Capture cancellation keeps menu open");
    menu.select(12);
    require(menu.handle(activate) == MenuResult::Apply, "Apply request");
    menu.select(11);
    (void)menu.handle(activate);
    require(menu.draft().visuals.tint == TintMode::Off && !menu.draft().audio.muted,
            "Defaults restored in draft");
    menu.select(3);
    (void)menu.handle(activate);
    (void)menu.handle(activate);
    require(menu.draft().visuals.textScale == 1.5F, "Text size");
    for (std::size_t row = 0; row < 15; ++row) {
        menu.select(row);
        bool selectedVisible = false;
        for (const auto& item : menu.rows({640, 360}))
            selectedVisible |= item.index == row;
        require(selectedVisible, "Selected row remains visible");
        for (const auto& draw : menu.makeScene({640, 360}).overlayTexts)
            require(draw.position.y >= 0 && draw.position.y + draw.fontSize <= 360,
                    "Menu text fits minimum height");
    }
    menu.setNotice("Unsaved changes. Apply and save to keep them.");
    bool wholeNotice = false;
    for (const auto& draw : menu.makeScene({640, 360}).overlayTexts)
        wholeNotice |= draw.text == "Unsaved changes. Apply and save to keep them.";
    require(wholeNotice, "Fitting notice is not truncated");
    const std::string longNotice = "A longer message uses several lines without losing any of its "
                                   "words when it does not fit on one line.";
    menu.setNotice(longNotice);
    std::string joined;
    for (const auto& draw : menu.makeScene({640, 360}).overlayTexts) {
        if (draw.text.starts_with("A longer") || !joined.empty()) {
            if (!joined.empty())
                joined += ' ';
            joined += draw.text;
        }
    }
    require(joined == longNotice, "Wrapped notice preserves words");
    for (const auto viewport :
         {GuiVector2{640, 360}, GuiVector2{1280, 720}, GuiVector2{1920, 1080}}) {
        for (std::size_t category = 16; category < 20; ++category) {
            menu.select(category);
            for (const auto action : {11U, 12U, 13U, 14U}) {
                bool visible = false;
                for (const auto& row : menu.rows(viewport))
                    if (row.index == action)
                        visible = row.bounds.pos.y >= 0 &&
                                  row.bounds.pos.y + row.bounds.height <= viewport.y;
                require(visible, "Footer actions always visible in every category");
            }
            for (const auto& row : menu.rows(viewport))
                require(row.bounds.pos.x >= 0 && row.bounds.pos.y >= 0 &&
                            row.bounds.pos.x + row.bounds.width <= viewport.x &&
                            row.bounds.pos.y + row.bounds.height <= viewport.y,
                        "Settings controls fit supported window");
        }
    }
    menu.select(15);
    const bool fps = menu.draft().visuals.showFps;
    (void)menu.handle(activate);
    require(menu.draft().visuals.showFps != fps, "FPS toggle belongs to Display");
    MenuInput nextCategory;
    nextCategory.category = 1;
    (void)menu.handle(nextCategory);
    require(menu.category() == 1, "Tab selects Audio category");
    menu.setArtwork("settings-background", {1280, 720});
    require(menu.makeScene({1280, 720}).sprites.front().textureId == "ui-settings-background",
            "Notebook sprite replaces drawn paper");
    MenuInput back;
    back.back = true;
    require(menu.handle(back) == MenuResult::Closed && !menu.isOpen(), "Back cancels draft");
}
void persistenceTest(const std::filesystem::path& root)
{
    const auto path = root / "folder with spaces" / "settings.conf";
    std::string error;
    GuiPreferences preferences;
    require(!SettingsStore::load(path, preferences, error) && error.empty(),
            "Missing file uses defaults");
    preferences.keyboard.keys[0] = GuiKey::W;
    preferences.audio.muted = true;
    preferences.visuals.backgroundMotion = false;
    preferences.visuals.showFps = false;
    preferences.visuals.textScale = 1.5F;
    preferences.visuals.tint = TintMode::Amber;
    preferences.visuals.tintOpacity = 0.4F;
    require(SettingsStore::save(path, preferences, error), "Save preferences");
    GuiPreferences loaded;
    require(SettingsStore::load(path, loaded, error) && loaded.keyboard.keys[0] == GuiKey::W &&
                loaded.audio.muted && !loaded.visuals.backgroundMotion && !loaded.visuals.showFps &&
                loaded.visuals.textScale == 1.5F && loaded.visuals.tint == TintMode::Amber &&
                loaded.visuals.tintOpacity == 0.4F,
            "Preference round trip");
    std::ofstream(path) << "version=1\nvolume=0.5\n";
    require(SettingsStore::load(path, loaded, error) && loaded.visuals.showFps,
            "Old files retain default FPS preference");
    preferences.audio.volume = 0.2F;
    require(SettingsStore::save(path, preferences, error), "Replace existing file");
    require(SettingsStore::load(path, loaded, error) && loaded.audio.volume == 0.2F,
            "Replacement loaded");
    for (const auto& invalid :
         {std::string("version=9\n"), std::string("version=1\nvolume=nan\n"),
          std::string("version=1\nkey.0=999\n"), std::string("version=1\ntint=9\n"),
          std::string("version=1\ntint-opacity=0.6\n"), std::string("version=1\nscale=9\n"),
          std::string("version=1\nshow-fps=2\n"), std::string("version=1\nmuted=0\nmuted=1\n"),
          std::string(17000, 'x')}) {
        std::ofstream(path) << invalid;
        loaded.audio.volume = 0.7F;
        require(!SettingsStore::load(path, loaded, error) && !error.empty() &&
                    loaded.audio.volume == 0.7F,
                "Invalid file does not partially change preferences");
    }
    const auto blocked = root / "ordinary-file";
    std::ofstream(blocked) << "keep";
    require(!SettingsStore::save(blocked / "settings.conf", preferences, error),
            "Write error reported");
    std::string retained;
    std::ifstream(blocked) >> retained;
    require(retained == "keep", "Write error preserves existing file");
}
} // namespace
int main()
{
    const auto root = std::filesystem::temp_directory_path() /
                      ("rtype-simple-settings-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        std::filesystem::create_directories(root);
        menuTest();
        persistenceTest(root);
        std::filesystem::remove_all(root);
        return 0;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(root);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
