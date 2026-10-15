# Issue #363: sky light create defaults and rotation/contrast setters.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles sky_light.c directly.
# sky_light.h includes <simd/simd.h>. The non-Apple shim pulls
# include/math/types.h, which is not in the tree, so this target is Apple-only.
if(APPLE)
    add_executable(test_sky_light
        tests/sky_light_test.c
        src/engine/lighting/sources/sky_light.c
    )
    target_include_directories(test_sky_light PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src
    )
    add_test(NAME test_sky_light COMMAND test_sky_light)
endif()

