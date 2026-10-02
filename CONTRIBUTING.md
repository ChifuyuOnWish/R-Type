# Contributing

## Setup

```sh
git clone <repo-url>
cd rtype
cmake --preset debug
cmake --build --preset debug
```

If you never configure with CMake, enable the hooks with `./scripts/install-hooks.sh`.
You also need `clang-format` installed for the pre-commit check.

## Workflow

1. Every piece of work starts from an **issue**.
2. Create a branch from `main`: `<type>/<issue>-<short-description>`, e.g. `feat/12-udp-socket`.
   Types: `feat`, `fix`, `docs`, `refactor`, `test`, `ci`, `chore`, `build`, `perf`, `merge`.
3. Commit using [Conventional Commits](https://www.conventionalcommits.org):
   `<type>(<scope>): <description>`, e.g. `fix(client): stop starfield stutter on resize`.
4. Open a pull request into `main`, fill in the template and link the issue (`Closes #12`).
5. A PR needs **one approval** and **green CI** before it can be merged. PRs are **squash-merged**.
6. Delete your branch after merging.

`main` is protected: nobody pushes to it directly.

## Releases

Each milestone is tagged on `main` following [SemVer](https://semver.org):
`v0.1.0` for the first defense, `v1.0.0` for the final one.

## Coding conventions

- C++20, formatted with the repository's `.clang-format` (`./scripts/format.sh` formats everything).
- Code, comments, commits and documentation are written in **English**.
- Files: `PascalCase.hpp` / `PascalCase.cpp`, one main class per file.
- TODO: naming conventions for classes, members, functions and constants, agreed by the team.

## Documentation

Docs live in `docs/` as Markdown, with diagrams in [Mermaid](https://mermaid.js.org).
Update them in the same PR as the code they describe.
