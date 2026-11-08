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
    !gui.audio().loadSound("player_shot", "audio/player_shot.wav")) {
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
const std::array<rtype::gui::SoundEvent, 1> events{{{"player_shot"}}};
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

Players can adjust and save these values through [Settings](settings.md).
The GUI defaults to 50% volume, unmuted.


```cpp
gui.setAudioSettings({0.5F, false}); // Half volume
gui.setAudioSettings({0.5F, true});  // Mute
```

Settings can be applied before opening the GUI or during playback. Volume must
be finite and between 0 and 1; invalid settings throw without changing the
previous settings. Updates affect existing and future voices.

Muting sets existing voices to zero volume while their playback continues.
New events are discarded while muted. Unmuting restores the configured volume.
Saved preferences are loaded on opening. Explicit API audio settings override
the saved audio values; use Apply and save to persist session changes.

## Current client integration

After opening the GUI, the client loads three recorded shooting effects and a death effect. Recordings are WAV PCM signed 16-bit, mono,
48 kHz. Leading and trailing background noise are trimmed. Short 3 ms / 8 ms fades
avoid hard cut boundaries; volume is not normalized.

The client resolves each Shoot event's source entity before choosing an effect:

| Source / server event | Sound ID | Asset |
|---|---|---|
| Player Shoot | player_shot | audio/player_shot.wav |
| Bat or boss Shoot | bat_shot | audio/bat_shot.wav |
| Floor or ceiling slug Shoot | wall_enemy_shot | audio/wall_enemy_shot.wav |
| Kill or PlayerDeath | death | audio/death.wav |
| Hit or shooters without a matching recording | None | None |

Unknown or already removed shooters are ignored rather than assigned an unrelated
sound. Projectile spawns do not trigger additional audio. A silent enemy only
produces a shot sound if the server emits a Shoot event for it.

Sounds are triggered by received server events, including other players and
enemies, rather than local keyboard presses. The client does not display event
notices. Muting or disabling audio currently has no equivalent event text.

Events received while Settings or a connection status page is displayed are
not played or queued for later. Network and world updates continue. Voices
already playing may finish. Volume, mute and overlapping voices follow the
AudioSystem rules above. Missing sound assets are logged and skipped without
blocking gameplay or rendering.

## Verification

Run a server and a connected client with an audio device available. Hold Fire
and check that server-confirmed shots produce the player recording. Check bats,
floor and ceiling slugs, and deaths against the table above. Change volume, mute and
unmute in Settings; apply each change. Check that mute and --no-audio suppress sound without affecting gameplay. Missing assets must not prevent movement or rendering.

The automated audio test replaces native audio calls with a fake backend to
check initialization failures, loading, overlapping voices, volume, mute and
cleanup ordering without a physical device. These checks do not confirm the
new server-event playback on real hardware; that requires a manual check.

Client-world tests cover all four player slots, bats, floor and ceiling slugs,
basic enemies, tanks and the tank boss, plus unknown, projectile and removed
source IDs. The client-session loopback test verifies that a received bat Shoot
event produces both its mouth animation and the `bat_shot` sound event.

## Recording edits

| Supplied recording | Output | Source interval (seconds) |
|---|---|---|
| plane_shot.m4a | player_shot.wav | 1.065 to 1.640 |
| ceuxquisontsurlesmurs.m4a | bat_shot.wav | 0.735 to 1.480 |
| lesautreslà.m4a | wall_enemy_shot.wav | 0.663 to 1.090 |
| ceuxquivole.m4a | death.wav | 0.770 to 0.930 |

The old generated tones are removed. Death audio uses Kill and PlayerDeath
events only, so a preceding Hit or EntityDestroy does not play it twice.
