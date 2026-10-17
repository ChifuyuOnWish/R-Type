#pragma once

#include "GuiTypes.hpp"

#include <span>

namespace rtype::gui {

class ResourceManager;

class Renderer
{
public:
    explicit Renderer(const ResourceManager& resources);
    void render(const RenderScene& scene, std::span<const GuiPosition> backgroundStars = {}) const;

private:
    const ResourceManager& _resources;
};

} // namespace rtype::gui
