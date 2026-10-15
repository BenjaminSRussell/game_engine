# Issue #422: advanced FontAtlas must not clash with core/localization.h.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_font_atlas_coexist
    tests/font_atlas_coexist_test.c
    src/engine/rendering/text_rendering_advanced.c
)
target_include_directories(test_font_atlas_coexist PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_font_atlas_coexist PRIVATE m)
add_test(NAME test_font_atlas_coexist COMMAND test_font_atlas_coexist)
