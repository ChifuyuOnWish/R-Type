#include "Gui.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

rtype::gui::SpriteDraw sprite(const std::string& texture, rtype::gui::GuiRectangle source,
                              rtype::gui::GuiRectangle destination, std::uint32_t id, int layer)
{
    rtype::gui::SpriteDraw result;
    result.entityId = {id, 0};
    result.textureId = texture;
    result.sourceRect = source;
    result.destinationRect = destination;
    result.layer = layer;
    return result;
}

rtype::gui::TextDraw label(const std::string& text, float x, float y)
{
    rtype::gui::TextDraw result;
    result.text = text;
    result.position = {x, y};
    return result;
}

rtype::gui::RenderScene makeDemoScene(bool showEnemy)
{
    rtype::gui::RenderScene scene;
    scene.clearColor = {20, 24, 32, 255};
    scene.background.enabled = true;
    scene.background.scrollSpeed = 40.0F;

    for (std::uint32_t index = 0; index < 4; ++index) {
        const float y = 100.0F + static_cast<float>(index) * 130.0F;
        const float sourceY = 2.0F + static_cast<float>(index) * 17.0F;
        scene.sprites.push_back(sprite("players", {{67.0F, sourceY}, 32.0F, 15.0F},
                                       {{100.0F, y}, 96.0F, 45.0F}, index + 1, 10));
        scene.sceneTexts.push_back(label("P" + std::to_string(index + 1), 50.0F, y + 10.0F));
    }

    scene.sprites.push_back(sprite("player-weapons", {{215.0F, 85.0F}, 16.0F, 12.0F},
                                   {{350.0F, 115.0F}, 48.0F, 36.0F}, 10, 20));
    scene.sprites.push_back(sprite("enemy-weapons", {{136.0F, 6.0F}, 7.0F, 6.0F},
                                   {{700.0F, 315.0F}, 21.0F, 18.0F}, 11, 20));
    if (showEnemy)
        scene.sprites.push_back(sprite("enemy", {{5.0F, 6.0F}, 21.0F, 24.0F},
                                       {{850.0F, 300.0F}, 63.0F, 72.0F}, 12, 10));

    scene.overlayTexts.push_back(label("R-Type", 20.0F, 20.0F));
    return scene;
}

} // namespace

int main()
{
    try {
        rtype::gui::Gui gui;
        gui.open({});
        auto& resources = gui.resources();
        resources.loadTexture("players", "player/ship-variants.gif");
        resources.loadTexture("player-weapons", "player/ship-and-effects.gif");
        resources.loadTexture("enemy", "enemy/red-fighter.gif");
        resources.loadTexture("enemy-weapons", "projectiles/red-beam-and-effects.gif");

        // Temporary local rendering fixture, replaced by client game state at integration.
        float elapsed = 0.0F;
        while (!gui.shouldClose()) {
            const auto actions = gui.pollActions();
            (void)actions;
            gui.render(makeDemoScene(elapsed < 5.0F));
            elapsed += gui.getFrameTime();
        }
        gui.close();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "r-type_client: " << error.what() << '\n';
        return 1;
    }
}
