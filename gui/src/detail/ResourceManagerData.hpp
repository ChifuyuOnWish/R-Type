#pragma once

#include "../ResourceManager.hpp"

#include <raylib.h>
#include <stdexcept>
#include <unordered_map>

namespace rtype::gui {
namespace detail {

struct TextureOwner
{
    Texture2D value{};
    std::filesystem::path path;

    ~TextureOwner()
    {
        if (value.id != 0)
            UnloadTexture(value);
    }
};

struct FontOwner
{
    Font value{};
    std::filesystem::path path;
    int requestedSize = 0;
    bool owned = false;

    ~FontOwner()
    {
        if (owned)
            UnloadFont(value);
    }
};

} // namespace detail

struct ResourceManager::Impl
{
    std::filesystem::path root;
    std::unordered_map<std::string, std::unique_ptr<detail::TextureOwner>> textures;
    std::unordered_map<std::string, std::unique_ptr<detail::FontOwner>> fonts;

    [[nodiscard]] std::filesystem::path resolve(const std::filesystem::path& path) const
    {
        if (path.empty())
            throw std::invalid_argument("Resource path must not be empty");
        return (path.is_absolute() ? path : root / path).lexically_normal();
    }
};

} // namespace rtype::gui
