# Issue #256: src/engine/backend/vulkan_backend.c sits at the backend root, not in
# backend/vulkan/, so the Apple-only exclude for backend/vulkan/*.c did not drop it.
# macOS then failed with "'vulkan.h' file not found" although Vulkan is disabled there.
# Evaluates cmake/sources.cmake with APPLE on and off and checks where the file lands.
# Script test, not add_engine_test: that macro links Engine, which does not build on
# origin/main. Run from the repo root, because the globs in sources.cmake are relative.
cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED SOURCE_CMAKE)
    set(SOURCE_CMAKE "${CMAKE_CURRENT_LIST_DIR}/../../cmake/sources.cmake")
endif()
if(NOT EXISTS "${SOURCE_CMAKE}")
    message(FATAL_ERROR "sources.cmake not found: ${SOURCE_CMAKE}")
endif()

# APPLE is a normal variable, so setting it inside this function overrides the host
# value for the include. Returns the ENGINE_SOURCES list that sources.cmake builds.
function(engine_sources_for apple out_var)
    set(APPLE ${apple})
    set(ENGINE_SOURCES "")
    include("${SOURCE_CMAKE}")
    set(${out_var} "${ENGINE_SOURCES}" PARENT_SCOPE)
endfunction()

# Entries of the list named by list_var that match regex. Filters a copy, because
# list(FILTER) edits its variable in place and the checks below reuse the lists.
function(matching_entries list_var regex out_var)
    set(_hits "${${list_var}}")
    list(FILTER _hits INCLUDE REGEX "${regex}")
    set(${out_var} "${_hits}" PARENT_SCOPE)
endfunction()

engine_sources_for(ON apple_sources)
engine_sources_for(OFF other_sources)

# Apple: Vulkan is disabled in favor of Metal, so neither the vulkan/ tree nor the
# backend-root vulkan_backend.c may be compiled.
matching_entries(apple_sources "/backend/vulkan/" apple_vulkan_tree)
if(apple_vulkan_tree)
    message(FATAL_ERROR "backend/vulkan/*.c is compiled when APPLE is true")
endif()

matching_entries(apple_sources "/backend/vulkan_backend\\.c$" apple_vulkan_root)
if(apple_vulkan_root)
    message(FATAL_ERROR "vulkan_backend.c is still compiled when APPLE is true (#256)")
endif()

# Other platforms: the Vulkan backend stays on, including the backend-root file.
matching_entries(other_sources "/backend/vulkan/" other_vulkan_tree)
if(NOT other_vulkan_tree)
    message(FATAL_ERROR "backend/vulkan/*.c is missing from non-Apple ENGINE_SOURCES")
endif()

matching_entries(other_sources "/backend/vulkan_backend\\.c$" other_vulkan_root)
if(NOT other_vulkan_root)
    message(FATAL_ERROR "vulkan_backend.c is missing from non-Apple ENGINE_SOURCES")
endif()

message(STATUS "Vulkan backend platform gate OK: Vulkan excluded on Apple, kept elsewhere")
