# 4x4 multiply, transpose, determinant, vector transforms, lerp, NaN/Inf.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c). Compiles
# mat4_core.c directly.
add_executable(test_mat4_core
    tests/mat4_core_test.c
    src/engine/math/linear_algebra/matrices/mat4/mat4_core.c
)
target_include_directories(test_mat4_core PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_mat4_core COMMAND test_mat4_core)

