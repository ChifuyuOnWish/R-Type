# Setting up the R-Type repository

Delete this file once everything below is done.

## 1. Create the repository

1. Create an **empty** repository on GitHub (the team's dev repo): no README, no .gitignore, no license. The kit already has them, and an empty repo lets the first push go through without conflicts.
2. Copy everything from this starter kit into it (including the hidden files: `.github/`, `.githooks/`,
   `.clang-format`, `.editorconfig`, `.gitattributes`, `.gitignore`).
3. Make sure the hooks are executable, then make the first commit:

   ```sh
   git init -b main
   chmod +x .githooks/* scripts/*.sh
   git add .
   git update-index --chmod=+x .githooks/* scripts/*.sh   # keeps them executable for everyone, Windows included
   git commit -m "chore: initial repository setup"
   git remote add origin git@github.com:<org>/<repo>.git
   git push -u origin main
   ```

4. Add every team member as a collaborator.

## 2. Mirror to the Epitech repository

The Epitech repository is the one graded, so this must work before the first deadline.

1. Generate a key pair just for the mirror (no passphrase):

   ```sh
   ssh-keygen -t ed25519 -C "rtype-mirror" -f mirror_key -N ""
   ```

2. On the **Epitech repository**, add `mirror_key.pub` as a deploy key **with write access**
   (or to an account that has push rights, depending on what your Epitech Git host allows).
3. On the **GitHub repository**, go to Settings › Secrets and variables › Actions:
   - Secret `MIRROR_SSH_KEY` = the content of `mirror_key` (the private key)
   - Variable `MIRROR_URL` = the SSH URL of the Epitech repository, e.g. `git@github.com:EpitechPromo2028/B-CPP-500-XXX-5-1-rtype-yourlogin.git`
4. Delete both key files from your machine.
5. Push something to `main` and check that the **Mirror to Epitech** workflow is green and the
   commit shows up on the Epitech repository.

## 3. Protect `main`

GitHub › Settings › Branches (or Rules › Rulesets) › add a rule for `main`:

- [x] Require a pull request before merging, with **1 approval**
- [x] Dismiss stale approvals when new commits are pushed
- [x] Require status checks to pass: `Format check`, `Commit messages`, `Build & test (Linux)`
      (and `Build & test (Windows)` if you keep it). They appear in the list after CI has run once.
- [x] Require branches to be up to date before merging
- [x] Block force pushes and deletions

Settings › General › Pull Requests:

- Allow **squash merging** only, with "Default to pull request title"
- Enable "Automatically delete head branches"

## 4. Each team member

```sh
git clone git@github.com:<org>/<repo>.git
cd <repo>
cmake --preset debug          # downloads dependencies and enables the Git hooks
cmake --build --preset debug
ctest --preset debug
```

Everyone needs CMake 3.21+, Ninja (Linux) or Visual Studio 2022 (Windows), and clang-format.
Configure your editor to format on save with the repository's `.clang-format`.

## 5. Decide on Windows

The CI builds on Windows from day one. If the team drops Windows, remove the Windows entry in
`.github/workflows/ci.yml` and the `windows` presets in `CMakePresets.json`.

## 6. Fill in the TODOs

- `README.md`: authors, license
- `LICENSE`: put the team members' names on the copyright line
- `CONTRIBUTING.md`: naming conventions
