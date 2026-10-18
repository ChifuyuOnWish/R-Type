# Technical & comparative study

For each choice: the alternatives considered, the comparison, and why we picked what we picked.

## Build system & package manager

| | CPM | vcpkg | Conan |
|-|-----|-------|-------|
| Extra tool to install | None (pure CMake) | vcpkg bootstrap | Python + Conan |
| Cross-platform setup | Same on all OSes | Triplets | Profiles |
| Prebuilt binaries | No (source builds, cached) | Binary caching | Yes (ConanCenter) |

**Choice:** CPM.

## Graphics, audio and input

**Choice:** raylib.

See the [graphics library study](graphics-library-study.md) for the
requirements, comparison, evaluation results and selection rationale.

## Networking

| Option | Strengths | Trade-offs |
|---|---|---|
| Asio | Cross-platform async I/O, stable event loop, header-only via CPM, no Boost dependency | Adds a networking library to the build |
| Raw sockets | No external library, direct control of the stack | Requires per-platform socket setup and more custom error handling |
| TCP channel | Simple stream API and built-in congestion control | Head-of-line blocking causes input and snapshot delays under loss |
| UDP + custom reliability | Keeps gameplay traffic low latency and lets us choose which messages are reliable | Requires an ack, resend and timeout layer to be implemented and tested |

**Choice:** standalone Asio for the transport layer, with our own acknowledgement and timeout rules on top of UDP. TCP would hide loss behind head-of-line blocking and make every input update wait behind later packets, while raw sockets would require OS-specific socket initialization and a custom event loop. The game uses a single UDP socket per side and only marks the critical messages as reliable.

## Algorithms & data structures

**Choice:** ECS (Entity Component System).

See the [engine architecture and algorithms study](engine-and-algorithms.md)

## Storage

## Security
