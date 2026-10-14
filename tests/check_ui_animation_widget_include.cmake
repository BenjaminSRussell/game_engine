# Issue #280: ui_animation.c must quote the local widget header.
# Angle brackets do not search the including file's directory, so
# <widgets/widget.h> is not found. "widgets/widget.h" resolves to
# widgets/widget.h beside ui_animation.c.
if(NOT DEFINED UI_ANIMATION_C OR UI_ANIMATION_C STREQUAL "")
    message(FATAL_ERROR "UI_ANIMATION_C is required")
endif()

if(NOT EXISTS "${UI_ANIMATION_C}")
    message(FATAL_ERROR "ui_animation.c not found: ${UI_ANIMATION_C}")
endif()

get_filename_component(_ui_dir "${UI_ANIMATION_C}" DIRECTORY)
set(_widget_h "${_ui_dir}/widgets/widget.h")
if(NOT EXISTS "${_widget_h}")
    message(FATAL_ERROR "quoted include would not resolve: ${_widget_h}")
endif()

file(STRINGS "${UI_ANIMATION_C}" _lines)
set(_quoted 0)
set(_angled 0)
foreach(_line IN LISTS _lines)
    string(STRIP "${_line}" _stripped)
    if(_stripped STREQUAL "#include \"widgets/widget.h\"")
        math(EXPR _quoted "${_quoted} + 1")
    endif()
    if(_stripped STREQUAL "#include <widgets/widget.h>")
        math(EXPR _angled "${_angled} + 1")
    endif()
endforeach()

if(_angled GREATER 0)
    message(FATAL_ERROR
        "${UI_ANIMATION_C} uses #include <widgets/widget.h>; use quotes")
endif()

if(NOT _quoted EQUAL 1)
    message(FATAL_ERROR
        "expected one #include \"widgets/widget.h\" in ${UI_ANIMATION_C}; found ${_quoted}")
endif()

message(STATUS "ui_animation.c quotes widgets/widget.h and the header is beside it")
