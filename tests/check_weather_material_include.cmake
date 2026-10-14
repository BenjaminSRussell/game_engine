# Issue #272: weather_rendering.c must include the live material system.
# The old path src/engine/rendering/material_system.h does not exist.
# rendering/materials/material_system.c is a disabled stub. The implementation
# that other materials code includes is src/engine/materials/material_system.h.
# This script does not compile the Engine target.

if(NOT DEFINED WEATHER_C OR NOT DEFINED CANONICAL_HEADER)
    message(FATAL_ERROR "WEATHER_C and CANONICAL_HEADER are required")
endif()

if(NOT EXISTS "${WEATHER_C}")
    message(FATAL_ERROR "weather source not found: ${WEATHER_C}")
endif()

if(NOT EXISTS "${CANONICAL_HEADER}")
    message(FATAL_ERROR "canonical header not found: ${CANONICAL_HEADER}")
endif()

file(STRINGS "${WEATHER_C}" _lines)
set(_inc "")
foreach(_line IN LISTS _lines)
    if(_line MATCHES "^#include[ \t]+\"([^\"]*material_system\\.h)\"")
        if(NOT _inc STREQUAL "")
            message(FATAL_ERROR "multiple material_system.h includes in ${WEATHER_C}")
        endif()
        set(_inc "${CMAKE_MATCH_1}")
    endif()
endforeach()

if(_inc STREQUAL "")
    message(FATAL_ERROR "weather_rendering.c does not include material_system.h")
endif()

if(_inc STREQUAL "../../../rendering/material_system.h")
    message(FATAL_ERROR "include still uses the missing rendering/material_system.h path")
endif()

get_filename_component(_dir "${WEATHER_C}" DIRECTORY)
get_filename_component(_resolved "${_dir}/${_inc}" ABSOLUTE)
if(NOT EXISTS "${_resolved}")
    message(FATAL_ERROR "material_system include does not exist: ${_inc} -> ${_resolved}")
endif()

get_filename_component(_resolved_real "${_resolved}" REALPATH)
get_filename_component(_canonical_real "${CANONICAL_HEADER}" REALPATH)
if(NOT _resolved_real STREQUAL _canonical_real)
    message(FATAL_ERROR
        "material_system include resolves to ${_resolved_real}, expected canonical ${_canonical_real}")
endif()

file(READ "${_canonical_real}" _header)
if(NOT _header MATCHES "material_system_init")
    message(FATAL_ERROR "canonical material_system.h does not declare material_system_init")
endif()

get_filename_component(_impl_dir "${_canonical_real}" DIRECTORY)
set(_impl_c "${_impl_dir}/material_system.c")
if(NOT EXISTS "${_impl_c}")
    message(FATAL_ERROR "canonical material system has no implementation: ${_impl_c}")
endif()

file(READ "${_impl_c}" _body)
string(STRIP "${_body}" _body_stripped)
if(_body_stripped STREQUAL "// Disabled")
    message(FATAL_ERROR "canonical material_system.c is a disabled stub")
endif()
if(NOT _body MATCHES "bool material_system_init[ \t]*\\(")
    message(FATAL_ERROR "material_system.c does not define material_system_init")
endif()

message(STATUS "weather_rendering.c includes canonical material_system.h")
