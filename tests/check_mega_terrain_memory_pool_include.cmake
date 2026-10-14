# Issue #271: mega_terrain_advanced.c must include the real memory_pool.h.
# The old relative path ../../../core/memory_pool.h does not exist.
if(NOT DEFINED TERRAIN_SOURCE)
    message(FATAL_ERROR "TERRAIN_SOURCE is required")
endif()

if(NOT EXISTS "${TERRAIN_SOURCE}")
    message(FATAL_ERROR "terrain source not found: ${TERRAIN_SOURCE}")
endif()

get_filename_component(_dir "${TERRAIN_SOURCE}" DIRECTORY)
file(STRINGS "${TERRAIN_SOURCE}" _lines)
set(_found 0)

foreach(_line IN LISTS _lines)
    string(STRIP "${_line}" _stripped)
    if(_stripped MATCHES "^#include[ \t]+\"([^\"]*memory_pool\\.h)\"")
        set(_inc "${CMAKE_MATCH_1}")
        math(EXPR _found "${_found} + 1")
        if(_inc STREQUAL "../../../core/memory_pool.h")
            message(FATAL_ERROR
                "mega_terrain_advanced.c still includes missing path ${_inc}")
        endif()
        get_filename_component(_resolved "${_dir}/${_inc}" ABSOLUTE)
        if(NOT EXISTS "${_resolved}")
            message(FATAL_ERROR
                "memory_pool.h include \"${_inc}\" does not exist (resolved ${_resolved})")
        endif()
        message(STATUS "memory_pool.h include \"${_inc}\" -> ${_resolved}")
    endif()
endforeach()

if(NOT _found EQUAL 1)
    message(FATAL_ERROR
        "expected exactly one memory_pool.h include in ${TERRAIN_SOURCE}; found ${_found}")
endif()
