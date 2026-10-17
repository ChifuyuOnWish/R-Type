#include "Gui.hpp"

#include "Renderer.hpp"

#include <stdexcept>
#include <utility>

namespace rtype::gui {

Gui::Gui(std::filesystem::path assetRoot) : _resources(std::move(assetRoot)) {}

Gui::~Gui()
{
    close();
}

void Gui::open(const GuiConfig& config)
{
    _window.open(config);
}

void Gui::close() noexcept
{
    _resources.clear();
    _window.close();
}

bool Gui::shouldClose() const
{
    return _window.shouldClose();
}

PlayerActions Gui::pollActions()
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot poll actions before opening the GUI");

    // raylib polls window events at EndDrawing; action mapping is added separately.
    return {};
}

void Gui::render(const RenderScene& scene)
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot render before opening the GUI");
    Renderer(_resources).render(scene);
}

void Gui::playAudio(std::span<const SoundEvent> events)
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot play audio before opening the GUI");
    if (!events.empty())
        throw std::logic_error("Audio playback is not implemented yet");
}

float Gui::getFrameTime() const
{
    return _window.getFrameTime();
}

ResourceManager& Gui::resources() noexcept
{
    return _resources;
}

const ResourceManager& Gui::resources() const noexcept
{
    return _resources;
}

} // namespace rtype::gui
