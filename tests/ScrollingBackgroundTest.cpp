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

} // namespace

int main()
{
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
