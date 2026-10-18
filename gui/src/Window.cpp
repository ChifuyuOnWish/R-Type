#include "Window.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <raylib.h>
#include <stdexcept>

namespace rtype::gui {

Window::~Window()
{
    close();
}

void Window::open(const GuiConfig& config)
{
    if (_open || IsWindowReady())
        throw std::logic_error("A GUI window is already open");
    if (config.width <= 0 || config.height <= 0 || config.targetFps < 0)
        throw std::invalid_argument(
            "Window dimensions must be positive and target FPS non-negative");

    _title = config.title;
    const unsigned int flags =
        config.resizable ? static_cast<unsigned int>(FLAG_WINDOW_RESIZABLE) : 0U;
    SetConfigFlags(flags);
    InitWindow(config.width, config.height, _title.c_str());

    if (!IsWindowReady())
        throw std::runtime_error("Failed to initialize the GUI window");

    _open = true;
    SetExitKey(KEY_NULL);
    SetWindowMinSize(640, 360);
    if (config.resizable) {
        if (auto* window = glfwGetCurrentContext())
            glfwSetWindowAspectRatio(window, 16, 9);
        else {
            close();
            throw std::runtime_error("Could not access the window context to set its aspect ratio");
        }
    }
    SetTargetFPS(config.targetFps);
}

void Window::close() noexcept
{
    if (!_open)
        return;

    CloseWindow();
    _open = false;
}

bool Window::isOpen() const noexcept
{
    return _open;
}

bool Window::shouldClose() const
{
    return !_open || WindowShouldClose();
}

GuiVector2 Window::getSize() const
{
    if (!_open)
        return {};
    return {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())};
}

bool Window::wasResized() const
{
    return _open && IsWindowResized();
}

float Window::getFrameTime() const
{
    return _open ? GetFrameTime() : 0.0F;
}

} // namespace rtype::gui
