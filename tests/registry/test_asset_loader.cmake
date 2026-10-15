# Issue #155. asset_loader.c is compiled directly, with a logger stub and a
# thread pool stub (see tests/unit/assets/asset_loader_test.c). add_engine_test
# would link Engine, which does not build on origin/main (missing GLFW, then
# gpu_profiler.c).
add_executable(test_asset_loader
    tests/unit/assets/asset_loader_test.c
    src/engine/assets/system/asset_system/loading/asset_loader.c)
target_include_directories(test_asset_loader PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_asset_loader PRIVATE Threads::Threads)
add_test(NAME test_asset_loader COMMAND test_asset_loader)
