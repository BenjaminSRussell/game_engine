# Issue #354: Nanite cluster DAG, quantization, LOD, and streaming.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles cluster_builder.m
# directly. Metal is Apple-only, so Linux does not register this target.
if(APPLE)
    add_executable(test_cluster_builder
        tests/cluster_builder_test.m
        src/engine/geometry/nanite/cluster_builder.m
    )
    target_include_directories(test_cluster_builder PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src
    )
    target_compile_options(test_cluster_builder PRIVATE -fno-objc-arc)
    find_library(CLUSTER_BUILDER_METAL_FRAMEWORK Metal REQUIRED)
    find_library(CLUSTER_BUILDER_FOUNDATION_FRAMEWORK Foundation REQUIRED)
    target_link_libraries(test_cluster_builder PRIVATE
        ${CLUSTER_BUILDER_FOUNDATION_FRAMEWORK}
        ${CLUSTER_BUILDER_METAL_FRAMEWORK}
    )
    add_test(NAME test_cluster_builder COMMAND test_cluster_builder)
endif()
