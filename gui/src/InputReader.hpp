#pragma once

#include "InputMapper.hpp"
#include "SettingsMenu.hpp"

#include <filesystem>

namespace rtype::gui {

struct InputFrame
{
    KeyboardState keyboard;
    GamepadState gamepad;
    bool gamepadDetected = false;
    std::string gamepadName;
    MenuInput menu;
    bool toggleSettings = false;
    bool mouseClick = false;
    GuiPosition mouse;
    float wheel = 0.0F;
};

class InputReader
{
public:
    void loadMappings(const std::filesystem::path& path);
    void reset() noexcept;
    [[nodiscard]] InputFrame poll(const GamepadSettings& settings = {});

private:
    int _vertical = 0;
    int _horizontal = 0;
    float _verticalRepeat = 0.0F;
    float _horizontalRepeat = 0.0F;
    int _device = -1;
    bool _menuReady = false;
    bool _connected = false;
    std::string _lastUnmappedGuid;
};

} // namespace rtype::gui
