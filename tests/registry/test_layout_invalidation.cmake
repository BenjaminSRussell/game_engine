# Issue #432. layout_invalidation.c must not assign a const LayoutCacheLevel* to a
# mutable one. Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c:80). The unit has no Engine
# dependencies, so it is compiled directly. The discarded-qualifiers diagnostic is
# promoted to an error, so a regression fails the build.
add_executable(test_layout_invalidation
    tests/unit/ui/layout_invalidation_test.c
    src/engine/ui/layout/layout_invalidation.c
)
target_include_directories(test_layout_invalidation PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
)
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(test_layout_invalidation PRIVATE
        -Werror=incompatible-pointer-types-discards-qualifiers
    )
else()
    target_compile_options(test_layout_invalidation PRIVATE
        -Werror=discarded-qualifiers
    )
endif()
add_test(NAME test_layout_invalidation COMMAND test_layout_invalidation)
