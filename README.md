# R-Type

A networked multiplayer remake of the classic shoot'em up **R-Type**, built on a custom C++ game engine.
Up to four players fight waves of Bydos together; a multithreaded authoritative server runs the game and
graphical clients connect to it over UDP.

## Supported platforms

| Platform | Server | Client |
|----------|--------|--------|
| Linux    | ✅     | ✅     |
| Windows  | ✅     | ✅     |

## Requirements

- A C++20 compiler (GCC 11+, Clang 14+ or MSVC 2022)
- CMake 3.21+
- Ninja (Linux) or Visual Studio 2022 (Windows)
- Git

All third-party libraries are fetched automatically by CMake through [CPM](https://github.com/cpm-cmake/CPM.cmake).
Low-level platform development packages must be installed separately.

### Linux (Ubuntu/Debian)

The GUI uses raylib 5.5 with its desktop X11 backend. Install the build tools,
OpenGL and X11 development packages:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git libasound2-dev libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
```

A graphical session is needed to run the graphical client. Native Wayland
support is disabled; Wayland sessions need XWayland for the X11 backend.

### Windows

Install Visual Studio 2022 with the Desktop development with C++ workload
and a Windows SDK. CPM builds raylib from source; no separate raylib SDK
installation is required.

## Build

```sh
# Linux
cmake --preset release
cmake --build --preset release

# Windows (Developer PowerShell)
cmake --preset windows
cmake --build --preset windows
```

The binaries `r-type_server` and `r-type_client` are produced at the repository root.

## Run

```sh
./r-type_server
./r-type_client
```

## Tests

```sh
ctest --preset release
```

## Documentation

- [Developer documentation](docs/index.md)
- [Network client](docs/network-client.md)
- [Network protocol](docs/protocol.md)
- [Contributing](CONTRIBUTING.md)

## Authors

- TODO: name - role - contact
- Nathan Genet - lead developer - nathan.genet@epitech.eu
- Noah Auroy - lead developer - noah.auroy@epitech.eu
- Ilona Dolbec - lead developer - ilona.dolbec@epitech.eu
- Alexandre ADIKPETO - lead developer - alexandre.adikpeto@epitech.eu

## License

Released under the [MIT License](LICENSE).
