# Controller mappings

The GUI loads `gamecontrollerdb.txt` automatically after opening the window.
Players do not need to create or edit a controller file.

The database comes from
[mdqinc/SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB),
commit `ff333823a85ae896e73f4c53670932cf0f728adb`, with one marked project addition
at the end. Its zlib license is retained as `gamecontrollerdb-LICENSE.txt`.

The project addition proposes the wired Elite 2 revision 0517 GUID
`030000005e040000000b000017050000`, derived from the upstream wired 0408 layout.
It remains pending physical button/axis validation. Record that validation in
the Settings test matrix before marking the revision supported.

This text data normalizes hardware buttons and axes into standard gamepad inputs.
Player preferences then map those inputs to movement and fire. It uses raylib's
existing GLFW backend and adds no input library or driver.

To update the database, retain the upstream attribution and license, preserve or
review the marked addition, update the pinned revision and rerun tests. An unknown
layout is reported in Settings with its GUID in the terminal; the program does not
invent an association for an unknown controller.
