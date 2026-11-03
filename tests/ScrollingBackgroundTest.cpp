#include "ScrollingBackground.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

using namespace rtype::gui;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool near(float left, float right)
{
    return std::abs(left - right) < 0.002F;
}

bool contains(const std::vector<GuiPosition>& stars, GuiPosition position)
{
    return std::any_of(stars.begin(), stars.end(), [position](const auto& star) {
        return near(star.x, position.x) && near(star.y, position.y);
    });
}

void samePositions(const std::vector<GuiPosition>& left, const std::vector<GuiPosition>& right)
{
    require(left.size() == right.size(), "Star counts differ");
    for (std::size_t index = 0; index < left.size(); ++index)
        require(near(left[index].x, right[index].x) && near(left[index].y, right[index].y),
                "Star positions differ");
}

template <class Function> void rejects(Function function)
{
    try {
        function();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("Invalid background data was accepted");
}

std::vector<GuiPosition> simulate(int fps)
{
    ScrollingBackground background;
    BackgroundConfig config;
    config.enabled = true;
    for (int frame = 0; frame < fps * 3; ++frame)
        background.update(config, 1.0F / static_cast<float>(fps));
    return background.getStars({1280.0F, 720.0F});
}

std::vector<SpriteDraw> simulatePaper(int fps)
{
    ScrollingBackground background;
    BackgroundConfig config;
    config.enabled = true;
    for (int frame = 0; frame < fps * 3; ++frame)
        background.update(config, 1.0F / static_cast<float>(fps));
    return background.getTiles({1280, 720}, {1280, 360}, "paper");
}

void paperTest()
{
    const auto thirty = simulatePaper(30);
    const auto sixty = simulatePaper(60);
    const auto fast = simulatePaper(144);
    require(thirty.size() == sixty.size() && sixty.size() == fast.size(),
            "Paper tile counts differ");
    for (std::size_t index = 0; index < thirty.size(); ++index)
        require(near(thirty[index].destinationRect.pos.x, sixty[index].destinationRect.pos.x) &&
                    near(sixty[index].destinationRect.pos.x, fast[index].destinationRect.pos.x),
                "Paper speed must not depend on FPS");
    ScrollingBackground background;
    BackgroundConfig config;
    config.enabled = true;
    config.scrollSpeed = 64;
    background.update(config, 0.5F);
    auto tiles = background.getTiles({512, 512}, {512, 512}, "paper");
    require(tiles.size() == 2 && near(tiles[0].destinationRect.pos.x, -32) &&
                near(tiles[1].destinationRect.pos.x, 480),
            "Paper moves left and covers both edges");
    require(tiles[0].sourceRect.width == 512 && tiles[0].sourceRect.height == 512 &&
                tiles[0].rotation == 0 && tiles[0].layer < 0,
            "Whole paper tiles stay behind entities");
    background.update(config, 7.5F);
    tiles = background.getTiles({512, 512}, {512, 512}, "paper");
    require(near(tiles[0].destinationRect.pos.x, 0), "Paper wraps after a full tile");
    config.enabled = false;
    background.update(config, 1);
    require(near(background.getTiles({512, 512}, {512, 512}, "paper")[0].destinationRect.pos.x, 0),
            "Disabled scrolling keeps its offset");
    tiles = background.getTiles({1600, 1000}, {1280, 360}, "paper");
    require(
        near(tiles[0].destinationRect.width / tiles[0].destinationRect.height, 1280.F / 360.F) &&
            tiles[0].destinationRect.height == 1000,
        "Resizing preserves paper proportions");
    require(background.getTiles({0, 0}, {512, 512}, "paper").empty(),
            "Empty viewport has no tiles");
    rejects([&] { (void)background.getTiles({512, 512}, {0, 512}, "paper"); });
    rejects([&] {
        (void)background.getTiles({512, 512}, {512, std::numeric_limits<float>::infinity()},
                                  "paper");
    });
}

} // namespace

int main()
{
    paperTest();
    samePositions(simulate(30), simulate(60));
    samePositions(simulate(60), simulate(144));

    ScrollingBackground background;
    BackgroundConfig config;
    config.enabled = true;
    config.scrollSpeed = 64.0F;
    const auto original = background.getStars({512.0F, 512.0F});
    require(original.size() == 64, "Expected one complete star tile");

    background.update(config, 0.5F);
    const auto moved = background.getStars({512.0F, 512.0F});
    for (const auto& star : original) {
        const float x = star.x >= 32.0F ? star.x - 32.0F : star.x + 480.0F;
        require(contains(moved, {x, star.y}), "Expected a 32-pixel move to the left with wrapping");
    }
    background.update(config, 7.5F);
    samePositions(original, background.getStars({512.0F, 512.0F}));
    background.update(config, 8000.0F);
    samePositions(original, background.getStars({512.0F, 512.0F}));

    const auto expanded = background.getStars({1024.0F, 1024.0F});
    require(expanded.size() == 256, "Resizing must repeat tiles across the entire viewport");
    for (const auto& star : original) {
        require(contains(expanded, star), "Resizing must preserve existing star coordinates");
        require(contains(expanded, {star.x + 512.0F, star.y}), "Missing horizontal tile");
        require(contains(expanded, {star.x, star.y + 512.0F}), "Missing vertical tile");
        require(contains(expanded, {star.x + 512.0F, star.y + 512.0F}), "Missing corner tile");
    }
    require(background.getStars({0.0F, 0.0F}).empty(), "An empty viewport must have no stars");

    config.enabled = false;
    background.update(config, 1.0F);
    samePositions(original, background.getStars({512.0F, 512.0F}));
    config.enabled = true;
    config.scrollSpeed = 0.0F;
    background.update(config, 1.0F);
    samePositions(original, background.getStars({512.0F, 512.0F}));

    config.scrollSpeed = -1.0F;
    rejects([&] { background.update(config, 1.0F); });
    config.scrollSpeed = std::numeric_limits<float>::infinity();
    rejects([&] { background.update(config, 1.0F); });
    config.scrollSpeed = 64.0F;
    rejects([&] { background.update(config, -1.0F); });
    rejects([&] { background.update(config, std::numeric_limits<float>::quiet_NaN()); });
    rejects([&] { (void)background.getStars({std::numeric_limits<float>::infinity(), 720.0F}); });
}
