#pragma once

#include "GuiTypes.hpp"

#include <string>

namespace rtype::gui {

class Window
{
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    void open(const GuiConfig& config);
    void close() noexcept;

    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] bool shouldClose() const;
    [[nodiscard]] GuiVector2 getSize() const;
    [[nodiscard]] bool wasResized() const;
    [[nodiscard]] float getFrameTime() const;

private:
    std::string _title;
    bool _open = false;
};

} // namespace rtype::gui
