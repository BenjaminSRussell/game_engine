# Unit tests for src/engine/rendering/animation/gpu_skinning.c (issue #211).
# Not add_engine_test: that macro links Engine, which does not build on main.
# gpu_skinning.c does not compile against the current headers: it uses
# BufferDesc, TextureDesc and shader_* (not declared anywhere), calls
# mat4_identity and clamp without declaring them, and passes a category to
# LOG_*. It is therefore compiled against test-only stand-ins in
# tests/gpu_skinning/stubs. The compat header is force-included into every
# source of this target.
add_executable(test_gpu_skinning
    tests/gpu_skinning/gpu_skinning_test.c
    src/engine/rendering/animation/gpu_skinning.c
)
# BEFORE: the root's directory-level include dirs (src/engine/include, src/engine,
# src) would otherwise come first and shadow the stand-ins with the real headers.
target_include_directories(test_gpu_skinning BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/gpu_skinning/stubs
)
target_include_directories(test_gpu_skinning PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_compile_options(test_gpu_skinning PRIVATE
    -include ${CMAKE_SOURCE_DIR}/tests/gpu_skinning/stubs/gpu_skinning_compat.h
)
target_link_libraries(test_gpu_skinning PRIVATE m)
add_test(NAME test_gpu_skinning COMMAND test_gpu_skinning)
