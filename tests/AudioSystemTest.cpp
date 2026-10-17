#include "AudioSystem.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <raylib.h>
#include <stdexcept>

struct rAudioBuffer
{
    bool alias = false;
    bool playing = false;
    float volume = 1.0F;
    rAudioBuffer* source = nullptr;
    int aliases = 0;
};

namespace {

bool ready = false;
bool allowDevice = false;
bool failSource = false;
int initCalls = 0;
int closeCalls = 0;
int sourceLoads = 0;
int aliasAttempts = 0;
int failAliasAt = -1;
int stopCalls = 0;
int playCalls = 0;
std::vector<rAudioBuffer*> buffers;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

Sound makeSound(rAudioBuffer* buffer)
{
    buffers.push_back(buffer);
    Sound sound{};
    sound.frameCount = 100;
    sound.stream = {buffer, nullptr, 22050, 16, 1};
    return sound;
}

void erase(rAudioBuffer* buffer)
{
    std::erase(buffers, buffer);
    delete buffer;
}

void checkVolume(float expected)
{
    for (const auto* buffer : buffers) {
        if (buffer->alias)
            require(buffer->volume == expected, "Voice volume differs from settings");
    }
}

struct TemporaryDirectory
{
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("rtype-audio-test-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    TemporaryDirectory()
    {
        std::filesystem::create_directories(path);
        std::ofstream(path / "shot.wav", std::ios::binary) << "good";
        std::ofstream(path / "hit.wav", std::ios::binary) << "good";
        std::ofstream(path / "bad.wav", std::ios::binary) << "bad";
        std::ofstream(path / "empty.wav", std::ios::binary);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

} // namespace

extern "C" {

void InitAudioDevice()
{
    ++initCalls;
    ready = allowDevice;
}

bool IsAudioDeviceReady()
{
    return ready;
}

void CloseAudioDevice()
{
    require(ready && buffers.empty(), "All sound resources must be released before device closure");
    ++closeCalls;
    ready = false;
}

Wave LoadWaveFromMemory(const char*, const unsigned char* data, int size)
{
    if (size != 4 || std::memcmp(data, "good", 4) != 0)
        return {};
    return {100, 22050, 16, 1, new unsigned char[1]};
}

bool IsWaveValid(Wave wave)
{
    return wave.data != nullptr;
}

void UnloadWave(Wave wave)
{
    delete[] static_cast<unsigned char*>(wave.data);
}

Sound LoadSoundFromWave(Wave)
{
    require(ready, "Sound creation requires a ready device");
    if (failSource)
        return {};
    ++sourceLoads;
    return makeSound(new rAudioBuffer);
}

Sound LoadSoundAlias(Sound source)
{
    ++aliasAttempts;
    if (aliasAttempts == failAliasAt)
        return {};
    auto* alias = new rAudioBuffer;
    alias->alias = true;
    alias->source = source.stream.buffer;
    ++alias->source->aliases;
    return makeSound(alias);
}

bool IsSoundValid(Sound sound)
{
    return sound.frameCount > 0 && sound.stream.buffer != nullptr;
}

void UnloadSoundAlias(Sound sound)
{
    auto* buffer = sound.stream.buffer;
    require(ready && buffer->alias, "Aliases must be released while the device is ready");
    require(std::find(buffers.begin(), buffers.end(), buffer->source) != buffers.end(),
            "Alias source was unloaded too early");
    --buffer->source->aliases;
    erase(buffer);
}

void UnloadSound(Sound sound)
{
    auto* buffer = sound.stream.buffer;
    require(ready && !buffer->alias && buffer->aliases == 0,
            "Source must be released after every alias and before device closure");
    erase(buffer);
}

void StopSound(Sound sound)
{
    ++stopCalls;
    sound.stream.buffer->playing = false;
}

bool IsSoundPlaying(Sound sound)
{
    return sound.stream.buffer->playing;
}

void PlaySound(Sound sound)
{
    require(ready && !sound.stream.buffer->playing, "Playback must use a free voice");
    ++playCalls;
    sound.stream.buffer->playing = true;
}

void SetSoundVolume(Sound sound, float volume)
{
    require(ready, "Volume updates require a ready device");
    sound.stream.buffer->volume = volume;
}

} // extern "C"

int main()
{
    using namespace rtype::gui;
    TemporaryDirectory directory;
    const std::array<SoundEvent, 1> shot{{{"shot"}}};
    AudioSystem audio(directory.path);
    audio.setSettings({0.25F, false});
    require(!audio.open() && !audio.isAvailable(), "Device failure must be non-fatal");
    require(!audio.loadSound("shot", "shot.wav") && audio.play(shot) == 0,
            "Unavailable audio must reject loads and skip playback");
    audio.close();
    require(closeCalls == 0, "Failed initialization must not close an unowned device");

    allowDevice = true;
    require(audio.open(), "Expected successful device initialization");
    const auto initializations = initCalls;
    require(audio.open() && initCalls == initializations, "Open must be idempotent");
    require(audio.loadSound("shot", "shot.wav"), "Expected sound load");
    require(audio.loadSound("shot", "./shot.wav") && sourceLoads == 1,
            "Repeated registrations must reuse their resources");
    require(!audio.loadSound("shot", "hit.wav"), "Conflicting IDs must be rejected");
    require(audio.loadSound("hit", "hit.wav"), "Expected second effect");
    require(buffers.size() == 18, "Expected two sources with eight voices each");
    checkVolume(0.25F);

    const std::array<SoundEvent, 3> simultaneous{{{"shot"}, {"shot"}, {"hit"}}};
    require(audio.play(simultaneous) == 3, "Effects and repeated instances must overlap");
    const std::vector<SoundEvent> burst(10, {"shot"});
    require(audio.play(burst) == 6 && stopCalls == 0,
            "Saturated effects must drop new instances without interrupting playback");
    const std::array<SoundEvent, 1> hit{{{"hit"}}};
    require(audio.play(hit) == 1, "Saturation must not affect unrelated sounds");
    for (auto* buffer : buffers)
        buffer->playing = false;
    require(audio.play(shot) == 1, "Finished voices must be reusable");

    audio.setSettings({0.4F, false});
    checkVolume(0.4F);
    audio.setSettings({0.4F, true});
    checkVolume(0.0F);
    const auto beforeMute = playCalls;
    require(audio.play(shot) == 0 && playCalls == beforeMute && stopCalls == 0,
            "Mute must discard new events without stopping active voices");
    audio.setSettings({0.4F, false});
    checkVolume(0.4F);
    for (const float volume : {-0.1F, 1.1F, std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected = false;
        try {
            audio.setSettings({volume, false});
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Invalid volume must be rejected");
        checkVolume(0.4F);
    }

    require(!audio.loadSound("missing", "missing.wav"), "Missing files must fail gracefully");
    require(audio.getLastError().find("missing.wav") != std::string::npos,
            "Loading errors must identify the asset");
    require(!audio.loadSound("bad", "bad.wav") && !audio.loadSound("empty", "empty.wav"),
            "Invalid and empty files must fail gracefully");
    const auto existingBuffers = buffers.size();
    failSource = true;
    require(!audio.loadSound("failed", "shot.wav"), "Source creation failure must be handled");
    failSource = false;
    failAliasAt = aliasAttempts + 3;
    require(!audio.loadSound("failed", "shot.wav") && buffers.size() == existingBuffers,
            "Partial voice creation must clean up all new resources");
    failAliasAt = -1;
    require(audio.loadSound("failed", "shot.wav"), "Failed loads must not poison the cache");
    const std::array<SoundEvent, 2> mixed{{{"unknown"}, {"hit"}}};
    require(audio.play(mixed) == 1, "Unknown IDs must not prevent valid effects");

    audio.close();
    audio.close();
    require(buffers.empty() && !audio.isAvailable() && closeCalls == 1,
            "Shutdown must release resources and close the device exactly once");
    require(audio.open() && audio.loadSound("shot", "shot.wav"), "Audio must support reopening");
    checkVolume(0.4F);
    audio.close();

    ready = true;
    const auto previousCloses = closeCalls;
    {
        AudioSystem other(directory.path);
        require(!other.open(), "An externally owned device must not be taken over");
    }
    require(ready && closeCalls == previousCloses, "An external device must not be closed");
    ready = false;
}
