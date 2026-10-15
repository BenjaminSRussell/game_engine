# Issue #438: quest_ui.h must include the live quest header.
# The old path ../assets/assets/code/todo_quests.c is not next to this file.
# assets/assets/code/todo_quests.c is a prototype .c, not a header.
# This script does not compile the Engine target.

if(NOT DEFINED QUEST_UI_H OR NOT DEFINED CANONICAL_HEADER)
    message(FATAL_ERROR "QUEST_UI_H and CANONICAL_HEADER are required")
endif()

if(NOT EXISTS "${QUEST_UI_H}")
    message(FATAL_ERROR "quest_ui.h not found: ${QUEST_UI_H}")
endif()

if(NOT EXISTS "${CANONICAL_HEADER}")
    message(FATAL_ERROR "canonical quest header not found: ${CANONICAL_HEADER}")
endif()

file(STRINGS "${QUEST_UI_H}" _lines)
set(_inc "")
foreach(_line IN LISTS _lines)
    if(_line MATCHES "^#include[ \t]+\"([^\"]+)\"")
        set(_path "${CMAKE_MATCH_1}")
        if(_path MATCHES "todo_quests\\.c")
            message(FATAL_ERROR "quest_ui.h still includes phantom todo_quests.c: ${_path}")
        endif()
        if(_path MATCHES "quest_system\\.h$")
            if(NOT _inc STREQUAL "")
                message(FATAL_ERROR "multiple quest_system.h includes in ${QUEST_UI_H}")
            endif()
            set(_inc "${_path}")
        endif()
    endif()
endforeach()

if(_inc STREQUAL "")
    message(FATAL_ERROR "quest_ui.h does not include quest_system.h")
endif()

get_filename_component(_dir "${QUEST_UI_H}" DIRECTORY)
get_filename_component(_resolved "${_dir}/${_inc}" ABSOLUTE)
if(NOT EXISTS "${_resolved}")
    message(FATAL_ERROR "quest_system include does not exist: ${_inc} -> ${_resolved}")
endif()

get_filename_component(_resolved_real "${_resolved}" REALPATH)
get_filename_component(_canonical_real "${CANONICAL_HEADER}" REALPATH)
if(NOT _resolved_real STREQUAL _canonical_real)
    message(FATAL_ERROR
        "quest include resolves to ${_resolved_real}, expected canonical ${_canonical_real}")
endif()

file(READ "${_canonical_real}" _header)
if(NOT _header MATCHES "QuestManager")
    message(FATAL_ERROR "canonical quest header does not declare QuestManager")
endif()

message(STATUS "quest_ui.h includes canonical quest_system.h")
