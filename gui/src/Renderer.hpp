#pragma once

#include "GuiTypes.hpp"

namespace rtype::gui {

class ResourceManager;

class Renderer
{
public:
    explicit Renderer(const ResourceManager& resources);
    void render(const RenderScene& scene) const;

private:
    const ResourceManager& _resources;
};

} // namespace rtype::gui
