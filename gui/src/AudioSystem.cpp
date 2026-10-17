#include "AudioSystem.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <raylib.h>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace rtype::gui {
namespace {

constexpr std::size_t voicesPerSound = 8;

std::string pathText(const std::filesystem::path& path)
{
    const auto utf8 = path.generic_u8string();
    return {utf8.begin(), utf8.end()};
}

struct WaveOwner
{
    Wave value{};
    ~WaveOwner()
    {
        if (value.data != nullptr)
            UnloadWave(value);
    }
};

struct SoundOwner
{
    std::filesystem::path path;
    Sound source{};
    std::array<Sound, voicesPerSound> voices{};

    ~SoundOwner()
    {
        for (const auto voice : voices) {
            if (voice.stream.buffer != nullptr) {
                StopSound(voice);
                UnloadSoundAlias(voice);
            }
        }
        if (source.stream.buffer != nullptr)
            UnloadSound(source);
    }
};

} // namespace

struct AudioSystem::Impl
{
    std::filesystem::path root;
    bool ownsDevice = false;
    AudioSettings settings;
    std::string error;
    std::unordered_map<std::string, std::unique_ptr<SoundOwner>> sounds;

    bool fail(const std::string& id, const std::filesystem::path& path, const std::string& reason)
    {
        error = "Sound '" + id + "' (" + pathText(path) + "): " + reason;
        return false;
    }

    [[nodiscard]] float effectiveVolume() const noexcept
    {
        return settings.muted ? 0.0F : settings.volume;
    }
};

AudioSystem::AudioSystem(std::filesystem::path assetRoot) : _impl(std::make_unique<Impl>())
{
    if (assetRoot.empty() || !assetRoot.is_absolute())
        throw std::invalid_argument("Audio asset root must be an absolute non-empty path");
    _impl->root = std::move(assetRoot).lexically_normal();
}

AudioSystem::~AudioSystem()
{
    close();
}

bool AudioSystem::open()
{
    if (isAvailable())
        return true;
    if (IsAudioDeviceReady()) {
        _impl->error = "Audio device is already managed outside this AudioSystem";
        return false;
    }
    InitAudioDevice();
    _impl->ownsDevice = IsAudioDeviceReady();
    if (!_impl->ownsDevice) {
        _impl->error = "Audio device is unavailable; continuing without sound";
        return false;
    }
    _impl->error.clear();
    return true;
}

void AudioSystem::close() noexcept
{
    _impl->sounds.clear();
    if (_impl->ownsDevice) {
        CloseAudioDevice();
        _impl->ownsDevice = false;
    }
}

bool AudioSystem::isAvailable() const noexcept
{
    return _impl->ownsDevice && IsAudioDeviceReady();
}

bool AudioSystem::loadSound(const std::string& id, const std::filesystem::path& path)
{
    const auto resolved = (path.is_absolute() ? path : _impl->root / path).lexically_normal();
    if (id.empty() || path.empty())
        return _impl->fail(id, resolved, "ID and path must not be empty");
    if (!isAvailable())
        return _impl->fail(id, resolved, "audio device is unavailable");
    if (const auto found = _impl->sounds.find(id); found != _impl->sounds.end()) {
        if (found->second->path != resolved)
            return _impl->fail(id, resolved, "ID is already assigned to another path");
        _impl->error.clear();
        return true;
    }

    std::ifstream file(resolved, std::ios::binary | std::ios::ate);
    if (!file)
        return _impl->fail(id, resolved, "cannot open file");
    const auto length = file.tellg();
    if (length <= 0 || length > std::numeric_limits<int>::max())
        return _impl->fail(id, resolved, "file is empty or exceeds the loader size limit");
    std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size())))
        return _impl->fail(id, resolved, "cannot read complete file");

    const auto extension = resolved.extension().string();
    WaveOwner wave{
        LoadWaveFromMemory(extension.c_str(), bytes.data(), static_cast<int>(bytes.size()))};
    if (!IsWaveValid(wave.value))
        return _impl->fail(id, resolved, "invalid or unsupported audio file");

    auto sound = std::make_unique<SoundOwner>();
    sound->path = resolved;
    sound->source = LoadSoundFromWave(wave.value);
    if (!IsSoundValid(sound->source))
        return _impl->fail(id, resolved, "sound buffer creation failed");
    for (auto& voice : sound->voices) {
        voice = LoadSoundAlias(sound->source);
        if (!IsSoundValid(voice))
            return _impl->fail(id, resolved, "sound voice creation failed");
        SetSoundVolume(voice, _impl->effectiveVolume());
    }
    _impl->sounds.emplace(id, std::move(sound));
    _impl->error.clear();
    return true;
}

std::size_t AudioSystem::play(std::span<const SoundEvent> events)
{
    if (!isAvailable() || _impl->settings.muted)
        return 0;
    std::size_t played = 0;
    for (const auto& event : events) {
        const auto found = _impl->sounds.find(event.soundId);
        if (found == _impl->sounds.end()) {
            _impl->error = "Unknown sound ID: " + event.soundId;
            continue;
        }
        for (const auto voice : found->second->voices) {
            if (IsSoundPlaying(voice))
                continue;
            PlaySound(voice);
            ++played;
            break;
        }
    }
    return played;
}

void AudioSystem::setSettings(const AudioSettings& settings)
{
    if (!std::isfinite(settings.volume) || settings.volume < 0.0F || settings.volume > 1.0F)
        throw std::invalid_argument("Audio volume must be finite and between 0 and 1");
    _impl->settings = settings;
    for (const auto& [id, sound] : _impl->sounds) {
        (void)id;
        for (const auto voice : sound->voices)
            SetSoundVolume(voice, _impl->effectiveVolume());
    }
}

const std::string& AudioSystem::getLastError() const noexcept
{
    return _impl->error;
}

} // namespace rtype::gui
