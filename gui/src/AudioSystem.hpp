#pragma once

#include "GuiTypes.hpp"

#include <filesystem>
#include <memory>
#include <span>

namespace rtype::gui {

class AudioSystem
{
public:
    explicit AudioSystem(std::filesystem::path assetRoot);
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) = delete;
    AudioSystem& operator=(AudioSystem&&) = delete;

    [[nodiscard]] bool open();
    void close() noexcept;
    [[nodiscard]] bool isAvailable() const noexcept;
    [[nodiscard]] bool loadSound(const std::string& id, const std::filesystem::path& path);
    [[nodiscard]] std::size_t play(std::span<const SoundEvent> events);
    void setSettings(const AudioSettings& settings);
    [[nodiscard]] const std::string& getLastError() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace rtype::gui
