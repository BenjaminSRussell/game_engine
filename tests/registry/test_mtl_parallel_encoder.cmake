# Issue #295. Unit test for backend/metal/mtl_parallel_encoder.c.
# The unit is Objective-C on Apple (the root CMakeLists sets LANGUAGE OBJC on
# backend/metal/*.c), so the test is Apple-only. It compiles the unit directly
# rather than linking Engine, which does not build on main.
if(APPLE)
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/src/engine/backend/metal/mtl_parallel_encoder.c
        ${CMAKE_SOURCE_DIR}/tests/unit/backend/metal/mtl_parallel_encoder_test.c
        PROPERTIES LANGUAGE OBJC COMPILE_FLAGS "-fobjc-arc")

    add_executable(test_mtl_parallel_encoder
        tests/unit/backend/metal/mtl_parallel_encoder_test.c
        src/engine/backend/metal/mtl_parallel_encoder.c)
    target_include_directories(test_mtl_parallel_encoder PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_mtl_parallel_encoder PRIVATE
        "-framework Foundation" "-framework Metal")
    add_test(NAME test_mtl_parallel_encoder COMMAND test_mtl_parallel_encoder)
endif()
