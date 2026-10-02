# Add a third-party dependency

1. Add a `CPMAddPackage` call in `cmake/Dependencies.cmake`, pinned to a tag:

   ```cmake
   CPMAddPackage(
       NAME fmt
       GITHUB_REPOSITORY fmtlib/fmt
       GIT_TAG 11.0.2
   )
   ```

2. Link it to the target that needs it:

   ```cmake
   target_link_libraries(r-type_server PRIVATE fmt::fmt)
   ```

3. Reconfigure (`cmake --preset debug`). The sources land in `.cpm-cache/` and are reused by every
   build afterwards.

4. Add the library and the reason for choosing it to `docs/comparative-study.md`.

Never copy a library's source into the repository, and never rely on a system-installed library
(except low-level platform ones such as OpenGL or X11).
