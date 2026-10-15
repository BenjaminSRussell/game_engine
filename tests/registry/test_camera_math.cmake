# Issue #188: yaw/pitch camera basis, view, and projection.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c:80). Compiles
# rendering/core/camera.c directly. The phantom cmake/sources.cmake paths
# src/engine/rendering/camera.c and camera_impl.c belong to issue #172.
add_executable(test_camera_math
    tests/camera_math_test.c
    src/engine/rendering/core/camera.c
    src/engine/math/linear_algebra/matrices/mat4/mat4_projection.c
)
target_include_directories(test_camera_math PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_camera_math PRIVATE m)
add_test(NAME test_camera_math COMMAND test_camera_math)
