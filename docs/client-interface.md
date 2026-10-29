# Client interface

Run `./r-type_client` to open the connection form. Enter a player name, server IPv4
address and port, then click Connect or press Enter. The default endpoint is
`127.0.0.1:4242`; it can be edited before connecting. Name, address and port fields
use click-to-focus, character entry and Backspace. Tab, Shift+Tab and arrow keys
move between controls. Player names currently accept printable ASCII, up to
32 bytes; the form accepts IPv4 addresses and ports from 1 to 65535.

`--connect host:port --name NAME` remains available for directly starting a
connection from the command line. Command-line hostname resolution retains the
existing networking behavior; the graphical form only accepts numeric IPv4.

## Connection and lobby

Connecting shows a loading screen. Cancel or Escape returns to the form. A failed
connection shows the reason, Retry and Edit address. Retry uses the last submitted
form values. A successful handshake waits for the server's lobby state before
opening the lobby or the running game. If that state is missing for ten seconds,
the connection is closed and the client offers retry. Disconnecting clears the
local world and player names; reconnecting creates a new network client so events
from a previous connection cannot enter the new session.

Campaign maps to the protocol's Level mode. Infinite maps to Infinite. Duel is
visible but disabled while PVP is unavailable. The GUI reflects the server's mode,
level names, unlocked stages and leader. Only the leader may change the selection
and start the session. Previous/next select unlocked stages. Start remains disabled
while a changed selection awaits confirmation; if the server does not confirm it
within three seconds, the player can retry. No gameplay starts locally: the
server's Playing phase opens the game, including when joining a running session.
A level clear, campaign clear or game over returns to the lobby with feedback.

A non-leader can view the selection, open Settings or disconnect. The GUI releases
movement and fire outside gameplay and while Settings is open. Gameplay input
keeps the existing serialized InputMessage and increasing client tick.

## Presentation

The title, Settings gear, fields, action buttons and three modes have optional
image slots. Missing artwork is shown as rectangles. See
[menu artwork](../assets/ui/README.md) for the filenames and fallback rules and
[menu font](../assets/fonts/README.md) for Holidays Homework setup and licensing.
Settings is reachable through the top-right control or F1. Its categories and fixed
bottom actions are documented in [Settings](settings.md). The connection/lobby
background can be replaced by `assets/ui/background.png`. Escape closes Settings;
on a connection/loading error page it returns to the connection form. During
play it retains the existing Settings shortcut.

Frontend layout is designed for the existing 16:9 window, down to 640 x 360, with
100%, 125% and 150% text settings. Text is measured with its actual loaded font;
long labels shrink to a minimum readable size and use an ellipsis if necessary.
Keyboard focus is shown above and below the control; hover changes its tint.

`gui/include/FrontendTypes.hpp` holds backend-independent state and commands.
`gui/src/FrontendMenu` handles editing, navigation and drawing requests.
`gui/src/InputReader` translates native input once per frame. `Gui` owns artwork,
font loading and Settings. `client/src/ClientSession` owns connections, protocol
messages and the local world. The GUI does not decode packets or decide game rules.

## Verification

`frontend_menu` checks editing, port validation, unavailable modes, leader controls,
selection acknowledgement gating, artwork fallback and supported layouts.
`gui_settings_integration` checks frontend input suppression, submission and
Settings access with an injected native backend. `client_session` connects to a
real UDP loopback server and exercises lobby state, serialized selection/start and
input, server-driven gameplay, leader changes, disconnection and reconnect.

Manual checks still required:

- Linux and Windows: connect to an available server, cancel, retry after failure,
  reconnect, and join a running game.
- Two clients: leader selection, follower view, leader departure and takeover.
- All supported text scales and window sizes, with the final font and artwork.
- Record screenshots after the final artwork is available.

Gamepad integration belongs to #108 and is not included in this change based on
main. Narration and color-vision assistance remain separate accessibility work.
