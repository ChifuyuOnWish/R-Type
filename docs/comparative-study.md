# Technical & comparative study

For each choice: the alternatives considered, the comparison, and why we picked what we picked.

## Build system & package manager

| | CPM | vcpkg | Conan |
|-|-----|-------|-------|
| Extra tool to install | None (pure CMake) | vcpkg bootstrap | Python + Conan |
| Cross-platform setup | Same on all OSes | Triplets | Profiles |
| Prebuilt binaries | No (source builds, cached) | Binary caching | Yes (ConanCenter) |

**Choice:** CPM.

## Graphics library

## Networking

## Algorithms & data structures

## Storage

## Security
