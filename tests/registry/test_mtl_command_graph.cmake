# Issue #287. Unit test for backend/metal/mtl_command_graph.c.
# The root CMakeLists compiles backend/metal/*.c as Objective-C with -fobjc-arc on Apple,
# so the test is Apple-only and compiles the unit the same way, which takes the __OBJC__
# branch of metal_command_dependency_wait(). It compiles the unit directly rather than
# linking Engine, which does not build on main.
if(APPLE)
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/src/engine/backend/metal/mtl_command_graph.c
        ${CMAKE_SOURCE_DIR}/tests/unit/backend/metal/mtl_command_graph_test.c
        PROPERTIES LANGUAGE OBJC COMPILE_FLAGS "-fobjc-arc")

    add_executable(test_mtl_command_graph
        tests/unit/backend/metal/mtl_command_graph_test.c
        src/engine/backend/metal/mtl_command_graph.c)
    target_include_directories(test_mtl_command_graph PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_mtl_command_graph PRIVATE
        "-framework Foundation" "-framework Metal")
    add_test(NAME test_mtl_command_graph COMMAND test_mtl_command_graph)
endif()
