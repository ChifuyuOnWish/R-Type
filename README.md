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
Nothing has to be installed system-wide.

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
- [Network protocol](docs/protocol.md)
- [Contributing](CONTRIBUTING.md)

## Authors

- TODO: name — role — contact
- Noah Auroy — lead developer — noah.auroy@epitech.eu

## License

Released under the [MIT License](LICENSE).
