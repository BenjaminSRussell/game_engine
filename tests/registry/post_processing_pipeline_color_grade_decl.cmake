# Issue #425: post_processing_pipeline.c calls color_grade_add_to_graph(), so color_grading.h
# must be included and must declare it. Syntax-only compile of the unit, so this does not link
# Engine (the library still does not build). It fails on an implicit declaration or on the
# int-to-RGResourceHandle assignment if the include or the declaration is removed.
add_test(
    NAME post_processing_pipeline_color_grade_decl
    COMMAND ${CMAKE_C_COMPILER} -fsyntax-only -std=c11
        -Werror=implicit-function-declaration -Werror=int-conversion
        -I${CMAKE_SOURCE_DIR}/src/engine/include
        -I${CMAKE_SOURCE_DIR}/src/engine
        -I${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/engine/rendering/post_processing/post_processing_pipeline.c
)
