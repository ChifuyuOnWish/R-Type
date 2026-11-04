#include "ResourceManager.hpp"

#include "detail/ResourceManagerData.hpp"

#include <fstream>
#include <limits>
#include <raylib.h>
#include <stdexcept>
#include <utility>

namespace rtype::gui {
GuiVector2 ResourceManager::measureText(const std::string& fontId, const std::string& text,
                                        float size, float spacing) const
{
    Font font = GetFontDefault();
    if (!fontId.empty()) {
        const auto found = _impl->fonts.find(fontId);
        if (found == _impl->fonts.end())
            throw std::out_of_range("Unknown font ID: " + fontId);
        font = found->second->value;
    }
    const auto measured = MeasureTextEx(font, text.c_str(), size, spacing);
    return {measured.x, measured.y};
}
namespace {

std::string pathText(const std::filesystem::path& path)
{
    const auto utf8 = path.generic_u8string();
    return {utf8.begin(), utf8.end()};
}

std::runtime_error loadError(const std::string& id, const std::filesystem::path& path,
                             const std::string& reason)
{
    return std::runtime_error("Resource '" + id + "' (" + pathText(path) + "): " + reason);
}

std::vector<unsigned char> readBytes(const std::string& id, const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw loadError(id, path, "cannot open file");

    const auto length = file.tellg();
    if (length <= 0 || length > std::numeric_limits<int>::max())
        throw loadError(id, path, "file is empty or exceeds the loader size limit");

    std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size())))
        throw loadError(id, path, "cannot read complete file");
    return bytes;
}

struct ImageOwner
{
    Image value{};
    ~ImageOwner()
    {
        if (value.data != nullptr)
            UnloadImage(value);
    }
};

} // namespace

ResourceManager::ResourceManager(std::filesystem::path assetRoot) : _impl(std::make_unique<Impl>())
{
    if (assetRoot.empty())
        throw std::invalid_argument("Asset root must not be empty");
    if (assetRoot.is_relative()) {
        const char* directory = GetApplicationDirectory();
        if (directory == nullptr || directory[0] == '\0')
            throw std::runtime_error("Cannot determine the executable directory");
        const std::string directoryText(directory);
        assetRoot =
            std::filesystem::path(std::u8string(directoryText.begin(), directoryText.end())) /
            assetRoot;
    }
    _impl->root = std::filesystem::absolute(assetRoot).lexically_normal();
}

ResourceManager::~ResourceManager()
{
    clear();
}

void ResourceManager::loadTexture(const std::string& id, const std::filesystem::path& path)
{
    if (id.empty())
        throw std::invalid_argument("Texture ID must not be empty");
    const auto resolved = _impl->resolve(path);
    if (const auto found = _impl->textures.find(id); found != _impl->textures.end()) {
        if (found->second->path != resolved)
            throw loadError(id, resolved, "texture ID is already assigned to another path");
        return;
    }

    const auto bytes = readBytes(id, resolved);
    const auto extension = resolved.extension().string();
    ImageOwner image{
        LoadImageFromMemory(extension.c_str(), bytes.data(), static_cast<int>(bytes.size()))};
    if (!IsImageValid(image.value))
        throw loadError(id, resolved, "invalid or unsupported image");
    if (!IsWindowReady())
        throw loadError(id, resolved, "an open window is required");

    auto texture = std::make_unique<detail::TextureOwner>();
    texture->path = resolved;
    texture->value = LoadTextureFromImage(image.value);
    if (!IsTextureValid(texture->value))
        throw loadError(id, resolved, "texture upload failed");
    _impl->textures.emplace(id, std::move(texture));
}

void ResourceManager::loadFont(const std::string& id, const std::filesystem::path& path, int size)
{
    if (id.empty() || size <= 0)
        throw std::invalid_argument("Font ID must not be empty and font size must be positive");
    const auto resolved = _impl->resolve(path);
    if (const auto found = _impl->fonts.find(id); found != _impl->fonts.end()) {
        if (found->second->path != resolved || found->second->requestedSize != size)
            throw loadError(id, resolved, "font ID is already assigned to another path or size");
        return;
    }

    const auto bytes = readBytes(id, resolved);
    if (!IsWindowReady())
        throw loadError(id, resolved, "an open window is required");

    auto font = std::make_unique<detail::FontOwner>();
    font->path = resolved;
    font->requestedSize = size;
    const auto extension = resolved.extension().string();
    font->value = LoadFontFromMemory(extension.c_str(), bytes.data(),
                                     static_cast<int>(bytes.size()), size, nullptr, 0);
    const auto defaultFont = GetFontDefault();
    font->owned = font->value.texture.id != 0 && font->value.texture.id != defaultFont.texture.id;
    if (!font->owned || !IsFontValid(font->value) || !IsTextureValid(font->value.texture))
        throw loadError(id, resolved, "invalid or unsupported font");
    _impl->fonts.emplace(id, std::move(font));
}

bool ResourceManager::hasTexture(const std::string& id) const
{
    return _impl->textures.contains(id);
}

bool ResourceManager::hasFont(const std::string& id) const
{
    return _impl->fonts.contains(id);
}

GuiVector2 ResourceManager::getTextureSize(const std::string& id) const
{
    const auto found = _impl->textures.find(id);
    if (found == _impl->textures.end())
        throw std::out_of_range("Unknown texture ID: " + id);
    return {static_cast<float>(found->second->value.width),
            static_cast<float>(found->second->value.height)};
}

int ResourceManager::getFontBaseSize(const std::string& id) const
{
    const auto found = _impl->fonts.find(id);
    if (found == _impl->fonts.end())
        throw std::out_of_range("Unknown font ID: " + id);
    return found->second->value.baseSize;
}

const std::filesystem::path& ResourceManager::getAssetRoot() const noexcept
{
    return _impl->root;
}

void ResourceManager::clear() noexcept
{
    _impl->fonts.clear();
    _impl->textures.clear();
}

} // namespace rtype::gui
