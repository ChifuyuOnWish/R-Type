# Audio

## Ownership and lifecycle

`Gui::open()` attempts audio initialization unless `GuiConfig::audioEnabled` is
false. An unavailable device leaves rendering and input operational.
`IGui::isAudioAvailable()` reports whether this GUI owns a ready audio device.

`AudioSystem` exclusively owns the device it opens. It refuses to take over an
already initialized external device. Other code must not close its device.
On shutdown, playback voices are stopped and unloaded before their shared source
sounds; the audio device is closed last. Repeated shutdown calls are safe.

## Load effects

Load effects after opening the GUI:

```cpp
if (gui.isAudioAvailable() &&
    !gui.audio().loadSound("shot", "audio/demo-shot.wav")) {
    std::cerr << gui.audio().getLastError() << '\n';
}
```

Paths use the same asset root as visual resources, independent of the working
directory. Sound IDs have their own namespace. Repeating an ID and normalized
path reuses the sound; assigning that ID another path fails.

Missing, empty, invalid or unsupported files and native buffer creation failures
return `false` and populate `getLastError()`. Failed loads are not cached.
The caller can report the problem and continue without that effect.

## Trigger effects

```cpp
const std::array<rtype::gui::SoundEvent, 1> events{{{"shot"}}};
gui.playAudio(events);
```

The client supplies events when they occur. The GUI does not infer gameplay events
from rendering data. Unknown sound IDs are skipped and recorded in the audio
manager's last error. Playback is a no-op when audio is unavailable.

Each loaded effect has eight playback voices sharing its sample data. Repeated
events can overlap, including multiple instances of the same effect. If all eight
voices for an effect are playing, a new instance is dropped. Existing voices and
other effects continue; no voice is stolen.

## Volume and mute

```cpp
gui.setAudioSettings({0.5F, false}); // Half volume
gui.setAudioSettings({0.5F, true});  // Mute
```

Settings can be applied before opening the GUI or during playback. Volume must
be finite and between 0 and 1; invalid settings throw without changing the
previous settings. Updates affect existing and future voices.

Muting sets existing voices to zero volume while their playback continues.
New events are discarded while muted. Unmuting restores the configured volume.
Settings survive closing and reopening the GUI.

## Local checks and accessibility

The demo uses two generated test tones: Space triggers `demo-shot`, and the
temporary enemy removal triggers `demo-hit`. Visual text accompanies both
events and remains available when sound is muted, missing or unavailable.

```sh
./r-type_client
./r-type_client --volume 0.25
./r-type_client --mute
./r-type_client --no-audio
```

Press Space repeatedly to check overlapping effects. Temporarily rename an
audio asset to check the missing-file path. The application must keep rendering
and showing the visual cues.

The automated audio test replaces native audio calls with a fake backend to
check initialization failures, loading, overlapping voices, volume, mute and
cleanup ordering without a physical device. Listening checks on a real device
are still required.
