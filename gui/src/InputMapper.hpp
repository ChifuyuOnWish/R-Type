#pragma once

#include "GuiTypes.hpp"

namespace rtype::gui {

struct KeyboardState
{
    bool focused = true;
    std::array<bool, guiKeyCount> heldKeys{};
};

class InputMapper
{
public:
    void setKeyBindings(const KeyBindings& bindings);
    [[nodiscard]] PlayerActions map(const KeyboardState& keyboard);
    void reset() noexcept;

private:
    KeyBindings _bindings;
    std::array<bool, playerActionCount> _heldActions{};
};

} // namespace rtype::gui
