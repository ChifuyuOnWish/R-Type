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
void DrawRectangleRec(Rectangle, Color)
{
    calls.push_back("ui");
}
Shader LoadShaderFromMemory(const char*, const char*)
{
    return {1, nullptr};
}
int GetShaderLocation(Shader, const char*)
{
    return 0;
}
void UnloadShader(Shader) {}
RenderTexture2D LoadRenderTexture(int width, int height)
{
    return {1, {2, width, height, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8}, {}};
}
bool IsRenderTextureValid(RenderTexture2D target)
{
    return target.id != 0;
}
void UnloadRenderTexture(RenderTexture2D) {}
void BeginTextureMode(RenderTexture2D)
{
    calls.push_back("target");
}
void EndTextureMode()
{
    calls.push_back("target end");
}
void BeginShaderMode(Shader)
{
    calls.push_back("filter");
}
void EndShaderMode()
{
    calls.push_back("filter end");
}
void SetShaderValue(Shader, int, const void*, int) {}
void SetShaderValueV(Shader, int, const void*, int uniformType, int count)
{
    require(uniformType == SHADER_UNIFORM_VEC3 && count == 3, "Three RGB transform rows");
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
        scene.colorVision = ColorVisionMode::Protanopia;
        scene.colorVisionStrength = 1.0F;
        Renderer(resources).render(scene);
        require(calls == std::vector<std::string>{"begin", "target", "clear", "sprite",
                                                  "target end", "filter", "sprite", "filter end",
                                                  "world", "ui", "overlay", "end"},
                "Filter composites the scene before drawing text and UI");
        calls.clear();
        scene.colorVisionStrength = 0.0F;
        Renderer(resources).render(scene);
        require(calls == std::vector<std::string>{"begin", "clear", "sprite", "world", "ui",
                                                  "overlay", "end"},
                "Zero strength skips the filter pass");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
