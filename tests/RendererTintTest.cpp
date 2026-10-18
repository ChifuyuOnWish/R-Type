#include "Renderer.hpp"
#include "detail/ResourceManagerData.hpp"

#include <iostream>
#include <raylib.h>
#include <stdexcept>
#include <vector>

namespace {
std::vector<std::string> calls;
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
extern "C" {
bool IsWindowReady()
{
    return true;
}
int GetScreenWidth()
{
    return 640;
}
int GetScreenHeight()
{
    return 360;
}
Font GetFontDefault()
{
    return {};
}
bool IsFontValid(Font)
{
    return true;
}
bool IsTextureValid(Texture2D)
{
    return true;
}
void BeginDrawing()
{
    calls.push_back("begin");
}
void EndDrawing()
{
    calls.push_back("end");
}
void ClearBackground(Color)
{
    calls.push_back("clear");
}
void DrawTexturePro(Texture2D, Rectangle, Rectangle, Vector2, float, Color)
{
    calls.push_back("sprite");
}
void DrawTextEx(Font, const char* value, Vector2, float, float, Color)
{
    calls.emplace_back(value);
}
void DrawRectangleRec(Rectangle bounds, Color color)
{
    if (color.a == 64) {
        require(bounds.x == 0 && bounds.y == 0 && bounds.width == 640 && bounds.height == 360,
                "Tint covers the entire window");
        calls.push_back("tint");
    } else
        calls.push_back("ui");
}
void UnloadTexture(Texture2D) {}
void UnloadFont(Font) {}
}
namespace rtype::gui {
ResourceManager::ResourceManager(std::filesystem::path path) : _impl(std::make_unique<Impl>())
{
    _impl->root = std::move(path);
}
ResourceManager::~ResourceManager() = default;
void ResourceManager::loadTexture(const std::string& id, const std::filesystem::path&)
{
    auto owner = std::make_unique<detail::TextureOwner>();
    owner->value = {1, 32, 32, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    _impl->textures[id] = std::move(owner);
}
} // namespace rtype::gui
int main()
{
    using namespace rtype::gui;
    try {
        ResourceManager resources;
        resources.loadTexture("player", "unused");
        RenderScene scene;
        SpriteDraw player;
        player.textureId = "player";
        player.sourceRect = {{0, 0}, 32, 32};
        player.destinationRect = {{0, 0}, 32, 32};
        scene.sprites.push_back(player);
        TextDraw world;
        world.text = "world";
        scene.sceneTexts.push_back(world);
        TextDraw overlay;
        overlay.text = "overlay";
        scene.overlayTexts.push_back(overlay);
        scene.rectangles.push_back({{{20, 20}, 40, 40}, {}});
        scene.sceneTint = {255, 80, 80, 64};
        Renderer(resources).render(scene);
        require(calls == std::vector<std::string>{"begin", "clear", "sprite", "world", "tint", "ui",
                                                  "overlay", "end"},
                "Tint covers the scene but not UI overlays");
        calls.clear();
        scene.sceneTint.alpha = 0;
        Renderer(resources).render(scene);
        require(calls == std::vector<std::string>{"begin", "clear", "sprite", "world", "ui",
                                                  "overlay", "end"},
                "Disabled tint adds no overlay");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
