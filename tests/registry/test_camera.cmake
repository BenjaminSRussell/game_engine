# Issue #263. camera_init, camera_get_forward and camera_get_projection_matrix
# must match include/rendering/camera.h. Not add_engine_test: that macro links
# Engine, which does not build on origin/main (missing GLFW, then gpu_profiler.c:80).
# Compiles camera.c directly.
add_executable(test_camera
    tests/camera_test.c
    src/engine/rendering/core/camera.c
)
target_include_directories(test_camera PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_camera PRIVATE m)
add_test(NAME test_camera COMMAND test_camera)
