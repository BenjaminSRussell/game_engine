# tests/registry

Each unit test registers itself in its own file in this directory. The root `CMakeLists.txt` includes every `tests/registry/*.cmake` file, sorted by name, at the end of the unit-test section. A test PR adds a file here and does not edit the root `CMakeLists.txt`.

## Convention

- One file per test target, named `tests/registry/<test_target>.cmake`, where `<test_target>` is the executable name and the `add_test` name. For example, `test_widget_focus.cmake`.
- A file registers only its own target. It holds that target's `add_executable`, `target_*` and `add_test` calls, or one `add_engine_test` call.
- Paths are relative to the repo root, as they are in the root `CMakeLists.txt`.
- Use `add_engine_test(<test_target> <source>)` only once the `Engine` target builds on `main`. Until then the macro would link `Engine`, which does not build, so compile the unit under test directly, as the `test_widget_focus` block in the root `CMakeLists.txt` does: list the unit's sources in `add_executable`, add the include directories it needs, then call `add_test(NAME <test_target> COMMAND <test_target>)`.
- New files are picked up on the next configure or build, because the glob uses `CONFIGURE_DEPENDS`.
- If an older PR appended a test block to the root `CMakeLists.txt`, move that block into its own file here when you rebase it.
