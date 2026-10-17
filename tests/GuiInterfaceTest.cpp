#include "IGui.hpp"

#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

class FakeGui final : public rtype::gui::IGui
{
public:
    void open(const rtype::gui::GuiConfig& config) override
    {
        require(config.width > 0 && config.height > 0, "Invalid window size");
        _closed = false;
    }

    void close() noexcept override { _closed = true; }

    [[nodiscard]] bool shouldClose() const override { return _closed; }

    void setKeyBindings(const rtype::gui::KeyBindings& bindings) override
    {
        fireKey = bindings.keys[static_cast<std::size_t>(rtype::gui::PlayerAction::Fire)];
    }

    [[nodiscard]] rtype::gui::PlayerActions pollActions() override
    {
        rtype::gui::PlayerActions actions;
        actions.heldActions[static_cast<std::size_t>(rtype::gui::PlayerAction::Fire)] = true;
        return actions;
    }

    void render(const rtype::gui::RenderScene& scene) override
    {
        require(!_closed, "Rendering requires an open GUI");
        require(scene.sprites.size() == 1, "Expected one sprite");
        renderedEntity = scene.sprites.front().entityId;
        renderedTexture = scene.sprites.front().textureId;
        renderedX = scene.sprites.front().destinationRect.pos.x;
        renderedOverlay = scene.overlayTexts.front().text;
    }

    void setAudioSettings(const rtype::gui::AudioSettings& settings) override
    {
        audioSettings = settings;
    }

    [[nodiscard]] bool isAudioAvailable() const noexcept override { return !_closed; }

    void playAudio(std::span<const rtype::gui::SoundEvent> events) override
    {
        require(events.size() == 1, "Expected one audio event");
        playedSound = events.front().soundId;
    }

    [[nodiscard]] float getFrameTime() const override { return 1.0F / 60.0F; }

    rtype::gui::GuiEntityId renderedEntity;
    std::string renderedTexture;
    float renderedX = 0.0F;
    std::string renderedOverlay;
    std::string playedSound;
    rtype::gui::AudioSettings audioSettings;
    rtype::gui::GuiKey fireKey = rtype::gui::GuiKey::Space;

private:
    bool _closed = true;
};

// A client-facing path depending only on the GUI contract, not raylib or ECS.
void presentFrame(rtype::gui::IGui& gui)
{
    using namespace rtype::gui;
    KeyBindings bindings;
    bindings.keys[static_cast<std::size_t>(PlayerAction::Fire)] = GuiKey::F;
    gui.setKeyBindings(bindings);
    const PlayerActions actions = gui.pollActions();
    require(actions.heldActions[static_cast<std::size_t>(PlayerAction::Fire)],
            "Expected the injected fire action");

    RenderScene scene;
    SpriteDraw player;
    player.entityId = {12, 3};
    player.textureId = "player";
    player.sourceRect = {{0.0F, 0.0F}, 32.0F, 16.0F};
    player.destinationRect = {{100.0F, 200.0F}, 64.0F, 32.0F};
    scene.sprites.push_back(player);

    TextDraw status;
    status.fontId = "hud";
    status.text = "Connected";
    scene.overlayTexts.push_back(status);
    gui.render(scene);

    gui.setAudioSettings({0.25F, true});
    require(gui.isAudioAvailable(), "Audio status must be accessible through IGui");
    const std::array<SoundEvent, 1> events{{{"shot"}}};
    gui.playAudio(events);
}

} // namespace

int main()
{
    FakeGui gui;
    require(gui.shouldClose(), "GUI should initially be closed");
    gui.open({});
    require(!gui.shouldClose(), "GUI should be open");

    presentFrame(gui);
    require(gui.renderedEntity == rtype::gui::GuiEntityId{12, 3},
            "Entity identity must preserve its generation");
    require(gui.renderedTexture == "player", "Texture ID must reach the GUI");
    require(gui.renderedX == 100.0F, "Position must reach the GUI");
    require(gui.renderedOverlay == "Connected", "Overlay must reach the GUI");
    require(gui.fireKey == rtype::gui::GuiKey::F, "Bindings must reach the GUI through IGui");
    require(gui.playedSound == "shot", "Sound event must reach the GUI");
    require(gui.audioSettings.volume == 0.25F && gui.audioSettings.muted,
            "Audio settings must reach the GUI");
    require(gui.getFrameTime() > 0.0F, "Frame time must be positive");

    gui.close();
    gui.close();
    require(gui.shouldClose(), "GUI should be closed");
    require(!gui.isAudioAvailable(), "Closed GUI must report unavailable audio");
}
