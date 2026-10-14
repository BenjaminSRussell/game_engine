# Issue #156: uncommented "src/engine/geometry/mesh.c" must appear exactly once.
if(NOT DEFINED SOURCES_CMAKE)
    message(FATAL_ERROR "SOURCES_CMAKE is required")
endif()

if(NOT EXISTS "${SOURCES_CMAKE}")
    message(FATAL_ERROR "sources list not found: ${SOURCES_CMAKE}")
endif()

file(STRINGS "${SOURCES_CMAKE}" _lines)
set(_count 0)
foreach(_line IN LISTS _lines)
    string(STRIP "${_line}" _stripped)
    if(_stripped MATCHES "^#")
        continue()
    endif()
    if(_stripped MATCHES "\"src/engine/geometry/mesh\\.c\"")
        math(EXPR _count "${_count} + 1")
    endif()
endforeach()

if(NOT _count EQUAL 1)
    message(FATAL_ERROR
        "src/engine/geometry/mesh.c is listed ${_count} times in ${SOURCES_CMAKE}; expected 1")
endif()

message(STATUS "src/engine/geometry/mesh.c listed once")
