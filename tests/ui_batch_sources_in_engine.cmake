# Prove issue #177: the six UI batch sources are compiled into the Engine target
# after the editor/*.c exclusion in cmake/sources.cmake.
if(NOT DEFINED ENGINE_BUILD_MAKE OR ENGINE_BUILD_MAKE STREQUAL "")
    message(FATAL_ERROR "ENGINE_BUILD_MAKE is not set")
endif()
if(NOT EXISTS "${ENGINE_BUILD_MAKE}")
    message(FATAL_ERROR "Engine build file not found: ${ENGINE_BUILD_MAKE}")
endif()

file(READ "${ENGINE_BUILD_MAKE}" _engine_build_make)

set(_ui_batch_sources
    "src/engine/editor/ui/canvas/ui_batch.c"
    "src/engine/editor/ui/canvas/ui_batch_effects.c"
    "src/engine/editor/ui/canvas/ui_batch_gpu.c"
    "src/engine/editor/ui/canvas/ui_batch_optimize.c"
    "src/engine/editor/ui/canvas/ui_batch_renderer.c"
    "src/engine/editor/ui/canvas/ui_batch_text.c"
)

foreach(_ui_batch_src IN LISTS _ui_batch_sources)
    string(FIND "${_engine_build_make}" "${_ui_batch_src}" _ui_batch_pos)
    if(_ui_batch_pos EQUAL -1)
        message(FATAL_ERROR "UI batch source not in Engine target: ${_ui_batch_src}")
    endif()
endforeach()

message(STATUS "UI batch sources are part of the Engine target")
