#pragma once

#include "GuiTypes.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace rtype::gui {

class ResourceManager
{
public:
    explicit ResourceManager(std::filesystem::path assetRoot = "assets");
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    void loadTexture(const std::string& id, const std::filesystem::path& path);
    void loadFont(const std::string& id, const std::filesystem::path& path, int size);

    [[nodiscard]] bool hasTexture(const std::string& id) const;
    [[nodiscard]] bool hasFont(const std::string& id) const;
    [[nodiscard]] GuiVector2 getTextureSize(const std::string& id) const;
    [[nodiscard]] int getFontBaseSize(const std::string& id) const;
    [[nodiscard]] const std::filesystem::path& getAssetRoot() const noexcept;

    void clear() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace rtype::gui
