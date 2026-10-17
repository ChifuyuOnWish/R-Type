#include "ResourceManager.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

template <typename Exception, typename Function> void expectError(Function function)
{
    bool caught = false;
    try {
        function();
    } catch (const Exception&) {
        caught = true;
    }
    require(caught, "Expected a controlled resource error");
}

struct TemporaryDirectory
{
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("rtype-resources-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    TemporaryDirectory() { std::filesystem::create_directories(path); }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

} // namespace

int main()
{
    using rtype::gui::ResourceManager;
    TemporaryDirectory temporary;
    ResourceManager resources(temporary.path);
    require(resources.getAssetRoot().is_absolute(), "Asset root must be absolute");
    require(!resources.hasTexture("missing"), "Unknown texture must not exist");
    require(!resources.hasFont("missing"), "Unknown font must not exist");

    expectError<std::invalid_argument>([&] { ResourceManager invalid(""); });
    expectError<std::invalid_argument>([&] { resources.loadTexture("", "sprite.gif"); });
    expectError<std::invalid_argument>([&] { resources.loadTexture("sprite", ""); });
    expectError<std::invalid_argument>([&] { resources.loadFont("font", "font.ttf", 0); });
    expectError<std::out_of_range>([&] { (void)resources.getTextureSize("missing"); });
    expectError<std::out_of_range>([&] { (void)resources.getFontBaseSize("missing"); });

    bool descriptiveError = false;
    try {
        resources.loadTexture("player", "missing.gif");
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        descriptiveError = message.find("player") != std::string::npos &&
                           message.find("missing.gif") != std::string::npos;
    }
    require(descriptiveError, "Loading errors must identify the resource and file");
    expectError<std::runtime_error>([&] { resources.loadFont("font", "missing.ttf", 20); });

    {
        std::ofstream invalidImage(temporary.path / "invalid.gif", std::ios::binary);
        invalidImage << "This is not an image";
    }
    expectError<std::runtime_error>([&] { resources.loadTexture("broken", "invalid.gif"); });
    require(!resources.hasTexture("broken"), "Failed loads must not enter the cache");
    require(!resources.hasTexture("player"), "Missing files must not enter the cache");
    require(!resources.hasFont("font"), "Missing fonts must not enter the cache");
    resources.clear();
    resources.clear();
    return 0;
}
