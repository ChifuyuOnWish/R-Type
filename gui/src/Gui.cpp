#include "Gui.hpp"

#include "Renderer.hpp"

#include <raylib.h>
#include <stdexcept>
#include <utility>

namespace rtype::gui {
namespace {

KeyboardState readKeyboard()
{
    constexpr std::array keyCodes{
        KEY_NULL,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_SPACE,
        KEY_A,
        KEY_B,
        KEY_C,
        KEY_D,
        KEY_E,
        KEY_F,
        KEY_G,
        KEY_H,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_M,
        KEY_N,
        KEY_O,
        KEY_P,
        KEY_Q,
        KEY_R,
        KEY_S,
        KEY_T,
        KEY_U,
        KEY_V,
        KEY_W,
        KEY_X,
        KEY_Y,
        KEY_Z,
        KEY_ENTER,
        KEY_LEFT_CONTROL,
        KEY_RIGHT_CONTROL,
        KEY_LEFT_SHIFT,
        KEY_RIGHT_SHIFT,
    };
    static_assert(keyCodes.size() == guiKeyCount);

    KeyboardState state;
    state.focused = IsWindowFocused();
    if (!state.focused)
        return state;

    for (std::size_t index = 1; index < keyCodes.size(); ++index) {
        state.heldKeys[index] = IsKeyDown(keyCodes[index]);
    }
    return state;
}

} // namespace

Gui::Gui(std::filesystem::path assetRoot)
    : _resources(std::move(assetRoot)), _audio(_resources.getAssetRoot())
{
}

Gui::~Gui()
{
    close();
}

void Gui::open(const GuiConfig& config)
{
    _window.open(config);
    _background = {};
    _input.reset();
    if (config.audioEnabled)
        (void)_audio.open();
}

void Gui::close() noexcept
{
    _input.reset();
    _audio.close();
    _resources.clear();
    _window.close();
}

bool Gui::shouldClose() const
{
    return _window.shouldClose();
}

void Gui::setKeyBindings(const KeyBindings& bindings)
{
    _input.setKeyBindings(bindings);
}

PlayerActions Gui::pollActions()
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot poll actions before opening the GUI");

    return _input.map(readKeyboard());
}

void Gui::render(const RenderScene& scene)
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot render before opening the GUI");
    _background.update(scene.background, _window.getFrameTime());
    const auto stars = scene.background.enabled ? _background.getStars(_window.getSize())
                                                : std::vector<GuiPosition>{};
    Renderer(_resources).render(scene, stars);
}

void Gui::setAudioSettings(const AudioSettings& settings)
{
    _audio.setSettings(settings);
}

bool Gui::isAudioAvailable() const noexcept
{
    return _audio.isAvailable();
}

void Gui::playAudio(std::span<const SoundEvent> events)
{
    if (!_window.isOpen())
        throw std::logic_error("Cannot play audio before opening the GUI");
    (void)_audio.play(events);
}

float Gui::getFrameTime() const
{
    return _window.getFrameTime();
}

AudioSystem& Gui::audio() noexcept
{
    return _audio;
}

const AudioSystem& Gui::audio() const noexcept
{
    return _audio;
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
