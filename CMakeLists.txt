# CMakeLists.txt for BlockGame - Refactored for Modularity
# TODO: High - Investigate dependency management (e.g., Conan, vcpkg) for third-party libraries.
# TODO: Medium - Add support for other platforms (Linux, Windows) by abstracting platform-specific code.

cmake_minimum_required(VERSION 3.20)
project(BlockGame C CXX OBJC)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# PHASE 1: Build System Restructuring - Runtime/Editor Separation
option(BUILD_EDITOR "Build editor executable with ImGui and editor tools" ON)
option(BUILD_RUNTIME_ONLY "Build lean runtime without any editor code" OFF)

if(BUILD_EDITOR)
    message(STATUS "========================================")
    message(STATUS "BUILD MODE: Editor (WITH_EDITOR=1)")
    message(STATUS "Building EditorApp with ImGui, tools, and full reflection")
    message(STATUS "========================================")
else()
    message(STATUS "========================================")
    message(STATUS "BUILD MODE: Runtime Only (Editor code excluded)")
    message(STATUS "Building lean game binary for shipping")
    message(STATUS "========================================")
endif()

# Find dependencies
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW3 not found via find_package, trying pkg-config...")
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW3 glfw3)
        if(GLFW3_FOUND)
            message(STATUS "GLFW3 found via pkg-config")
            include_directories(${GLFW3_INCLUDE_DIRS})
            link_directories(${GLFW3_LIBRARY_DIRS})
        endif()
    endif()
endif()

if(NOT glfw3_FOUND AND NOT GLFW3_FOUND)
    message(WARNING "GLFW3 not found - window functionality will be limited")
else()
    message(STATUS "GLFW3 found and will be linked")
    message(STATUS "  glfw3_FOUND: ${glfw3_FOUND}")
    message(STATUS "  GLFW3_FOUND: ${GLFW3_FOUND}")
    message(STATUS "  glfw3_LIBRARIES: ${glfw3_LIBRARIES}")
    message(STATUS "  GLFW3_LIBRARIES: ${GLFW3_LIBRARIES}")
    message(STATUS "  GLWF3_INCLUDE_DIRS: ${GLFW3_INCLUDE_DIRS}")
    message(STATUS "  glfw3_INCLUDE_DIRS: ${glfw3_INCLUDE_DIRS}")
endif()

include_directories(
    include 
    include_private 
    include/vendor 
    include/vendor/lua
    src/engine/include
    src/game/blockgame/include
    src/engine
    src
    src/frontend/Core/Bridges/include
    # New unified systems
    src/engine/include/core/math
    src/engine/include/core/memory
    src/engine/include/core/logging
)


# --- Custom Targets ---
# TODO: High - Create a separate target for running tests.
add_custom_target(validate_assets
    COMMAND python3 ${CMAKE_SOURCE_DIR}/src/engine/scripts/validate_assets.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating assets (PNG sizes, atlas constraints)"
)

# Include new Unreal Engine parity systems
include(cmake/new_systems.cmake)

add_custom_target(compile_shaders
    COMMAND ${CMAKE_SOURCE_DIR}/src/engine/scripts/compile_shaders.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling Vulkan shaders to SPIR-V"
)

# Metal shader compilation for instancing system
if(APPLE)
    add_custom_target(compile_instancing_shaders
        COMMAND xcrun -sdk macosx metal -c ${CMAKE_SOURCE_DIR}/assets/shaders/compute/instance_culling.metal -o ${CMAKE_BINARY_DIR}/instance_culling.air
        COMMAND xcrun -sdk macosx metal -c ${CMAKE_SOURCE_DIR}/assets/shaders/compute/batch_generation.metal -o ${CMAKE_BINARY_DIR}/batch_generation.air
        COMMAND xcrun -sdk macosx metallib ${CMAKE_BINARY_DIR}/instance_culling.air ${CMAKE_BINARY_DIR}/batch_generation.air -o ${CMAKE_BINARY_DIR}/instancing_shaders.metallib
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Compiling Metal compute shaders for instancing system"
    )
    message(STATUS "Metal shader compilation target configured: compile_instancing_shaders")
endif()

# --- Source File Lists ---

# Engine Sources
include(cmake/sources.cmake)


# --- Vulkan ---
find_package(Vulkan)
if(Vulkan_FOUND)
    file(GLOB_RECURSE VULKAN_SOURCES "src/engine/backend/vulkan/*.c")
    list(APPEND ENGINE_SOURCES ${VULKAN_SOURCES})
    add_definitions(-DVULKAN_BUILD)
    include_directories(${Vulkan_INCLUDE_DIRS})
    set(VULKAN_LIBS ${Vulkan_LIBRARIES})
    message(STATUS "Vulkan found and enabled")
else()
    message(WARNING "Vulkan not found, building without Vulkan renderer")
endif()

# --- Libraries ---
# TODO: Medium - Implement a more robust build options system (e.g., for enabling/disabling features like Vulkan, debugging, etc.).
add_library(Engine STATIC ${ENGINE_SOURCES})
target_include_directories(Engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include_private>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/engine/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/engine>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/engine/geometry/instancing>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/game/blockgame/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(Engine PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)

if(APPLE)
    add_definitions(-DMETAL_BUILD)
    file(GLOB ML_OBJC_SOURCES src/engine/ai/ml/*.c)
    set_source_files_properties(${ML_OBJC_SOURCES} PROPERTIES LANGUAGE OBJC)

    # Metal backend .m files (Objective-C)
    file(GLOB METAL_OBJC_SOURCES src/engine/backend/metal/*.m)
    set_source_files_properties(${METAL_OBJC_SOURCES} PROPERTIES LANGUAGE OBJC COMPILE_FLAGS "-fobjc-arc")
    
    # Metal backend .c files also use Objective-C syntax (#import, @available, etc.)
    file(GLOB METAL_C_SOURCES src/engine/backend/metal/*.c)
    set_source_files_properties(${METAL_C_SOURCES} PROPERTIES LANGUAGE OBJC COMPILE_FLAGS "-fobjc-arc")
    
    # Add Metal sources to engine sources
    list(APPEND ENGINE_SOURCES ${METAL_OBJC_SOURCES} ${METAL_C_SOURCES})

    # Core integration files using Metal/ObjC
    set(CORE_OBJC_SOURCES 
        src/engine/core/integration/render_world.m
        src/engine/core/integration/scene_renderer.m
        src/engine/platform/macos/macos_optimizations.m
    )
    set_source_files_properties(${CORE_OBJC_SOURCES} PROPERTIES LANGUAGE OBJC COMPILE_FLAGS "-fobjc-arc")
    
    # Link Metal frameworks on macOS
    find_library(METAL_FRAMEWORK Metal)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(IOKIT_FRAMEWORK IOKit)
    
    if(METAL_FRAMEWORK AND QUARTZCORE_FRAMEWORK)
        target_link_libraries(Engine PRIVATE ${METAL_FRAMEWORK} ${QUARTZCORE_FRAMEWORK} ${COREVIDEO_FRAMEWORK} ${IOKIT_FRAMEWORK})
        message(STATUS "Metal frameworks linked successfully")
    else()
        message(WARNING "Metal frameworks not found - Metal rendering may not work")
    endif()
endif()

# SIMD Math Library Platform-Specific Sources
# Detect platform and add appropriate SIMD implementation
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    # x86/x86_64 platform
    message(STATUS "Detected x86/x86_64 platform - enabling SSE2/AVX2 SIMD")
    
    # Check for AVX2 support
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    
    if(COMPILER_SUPPORTS_AVX2)
        message(STATUS "AVX2 support detected, compiling with AVX2 optimizations")
        set(SIMD_SOURCE src/engine/core/simd/simd_math_x86.c)
        set_source_files_properties(${SIMD_SOURCE} PROPERTIES COMPILE_FLAGS "-mavx2 -mfma")
        target_compile_definitions(Engine PRIVATE SIMD_AVX2)
    else()
        message(STATUS "Compiling with SSE2 optimizations")
        set(SIMD_SOURCE src/engine/core/simd/simd_math_x86.c)
        set_source_files_properties(${SIMD_SOURCE} PROPERTIES COMPILE_FLAGS "-msse2")
        target_compile_definitions(Engine PRIVATE SIMD_SSE2)
    endif()
    
    target_sources(Engine PRIVATE ${SIMD_SOURCE})
    
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    # ARM platform
    message(STATUS "Detected ARM platform - enabling NEON SIMD")
    set(SIMD_SOURCE src/engine/core/simd/simd_math_neon.c)
    
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR APPLE)
        # AArch64/Apple Silicon has NEON by default, no -mfpu flag needed
        message(STATUS "ARM64/Apple Silicon detected, NEON enabled by default")
    else()
        # ARMv7 need explicit flag
        message(STATUS "ARMv7 detected, enabling -mfpu=neon")
        set_source_files_properties(${SIMD_SOURCE} PROPERTIES COMPILE_FLAGS "-mfpu=neon")
    endif()
    
    target_compile_definitions(Engine PRIVATE SIMD_NEON)
    target_sources(Engine PRIVATE ${SIMD_SOURCE})
    
else()
    message(STATUS "Unknown platform, using scalar SIMD fallback")
endif()


# Include Swift bridge wrapper
if(APPLE)
    # Swift bridge wrapper for C-Swift integration
    target_sources(Engine PRIVATE 
        src/engine/core/swift_bridge_wrapper.c
    )
    
    # Add Swift frontend target - DISABLED FOR NOW
    # find_program(SWIFT_EXECUTABLE swift)
    # if(SWIFT_EXECUTABLE)
    #     message(STATUS "Swift compiler found, adding Swift frontend")
    #     
    #     # Create Swift package executable
    #     add_executable(VoxelForgeStudio
    #         src/frontend/App.swift
    #         src/frontend/ContentView.swift
    #         src/frontend/Sources/EngineBridge.swift
    #         src/frontend/Core/CommandManager.swift
    #         src/frontend/Core/DesignSystem.swift
    #         src/frontend/Core/Profiler.swift
    #         src/frontend/Core/SelectionManager.swift
    #         src/frontend/Views/Panels/SceneHierarchyPanel.swift
    #         src/frontend/Views/Panels/InspectorPanel.swift
    #         src/frontend/Views/Panels/ViewportPanel.swift
    #         src/frontend/Views/Panels/ContentBrowserPanel.swift
    #     )
    #     
    #     # Explicitly set the language for Swift
    #     set_source_files_properties(src/frontend/App.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/ContentView.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Sources/EngineBridge.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Core/CommandManager.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Core/DesignSystem.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Core/Profiler.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Core/SelectionManager.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Views/Panels/SceneHierarchyPanel.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Views/Panels/InspectorPanel.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Views/Panels/ViewportPanel.swift PROPERTIES LANGUAGE Swift)
    #     set_source_files_properties(src/frontend/Views/Panels/ContentBrowserPanel.swift PROPERTIES LANGUAGE Swift)
    #     
    #     set_target_properties(VoxelForgeStudio PROPERTIES
    #         SWIFT_MODULE_NAME VoxelForgeStudio
    #         SWIFT_MODULE_INSTALL YES
    #     )
    #     
    #     target_link_libraries(VoxelForgeStudio PRIVATE 
    #         Engine 
    #         "-framework Foundation" 
    #         "-framework SwiftUI" 
    #         "-framework Metal" 
    #         "-framework MetalKit" 
    #         "-framework Cocoa" 
    #         "-framework CoreGraphics"
    #     )
    #     
    #     target_include_directories(VoxelForgeStudio PRIVATE
    #         ${CMAKE_SOURCE_DIR}/include
    #         ${CMAKE_SOURCE_DIR}/include/platform
    #     )
    #     
    #     # Add Swift bridge compilation
    #     target_compile_definitions(VoxelForgeStudio PRIVATE SWIFT_BUILD)
    # else()
    #     message(WARNING "Swift compiler not found, Swift frontend will not be built")
    # endif()
endif()

# Link GLFW if found
if(GLFW3_FOUND)
    target_link_libraries(Engine PUBLIC ${GLFW3_LIBRARIES})
elseif(glfw3_FOUND)
    if(TARGET glfw3::glfw)
        target_link_libraries(Engine PUBLIC glfw3::glfw)
    elseif(TARGET glfw)
        target_link_libraries(Engine PUBLIC glfw)
    else()
        target_link_libraries(Engine PUBLIC glfw3)
    endif()
endif()

if(VULKAN_LIBS)
    target_link_libraries(Engine PUBLIC ${VULKAN_LIBS})
endif()

# Link OpenGL framework on macOS
if(APPLE)
    list(APPEND ENGINE_SOURCES
        src/engine/rendering/3d_rendering/geometry/bvh/bvh_metal_culling.m
    )
    target_link_libraries(Engine PUBLIC "-framework OpenGL" "-framework CoreFoundation" "-framework Foundation" "-framework CoreServices" "-framework Metal" "-framework MetalPerformanceShaders" "-framework CoreML")
endif()

# Find ZLIB and Threads before they're needed
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

add_library(Game STATIC ${GAME_SOURCES})
target_link_libraries(Game PUBLIC Engine ZLIB::ZLIB)
target_include_directories(Game PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/game/blockgame/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/engine/include>
)

# --- App Executable ---
if(APPLE AND IOS)
    set(APP_SOURCES
        App/Sources/AppDelegate.swift
        App/Sources/SceneDelegate.swift
        App/Sources/GameViewController.swift
        App/Sources/MainMenuViewController.swift
        App/Sources/SettingsViewController.swift
        App/Sources/AssetEditorViewController.swift
        App/Sources/BlockGame-Bridging-Header.h
    )
    add_executable(BlockGameApp MACOSX_BUNDLE ${APP_SOURCES})
    set_target_properties(BlockGameApp PROPERTIES
        BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/App/Resources/Info.plist
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.benjaminrussell.BlockGame"
        XCODE_ATTRIBUTE_INFOPLIST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/App/Resources/Info.plist
        XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/App/Sources/BlockGame-Bridging-Header.h
        XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone, iPad
    )

if(APPLE)
    target_link_libraries(BlockGameApp PRIVATE Game ZLIB::ZLIB Threads::Threads "-framework UIKit" "-framework SwiftUI" "-framework Metal" "-framework MetalKit")
else()
    target_link_libraries(BlockGameApp PRIVATE Game Engine ZLIB::ZLIB Threads::Threads)
endif()
endif()

# Placeholder for other platforms (e.g. macOS Desktop)
message(STATUS "Checking BlockGameApp target conditions: IOS=${IOS}, APPLE=${APPLE}")
if(NOT IOS)
    message(STATUS "Adding BlockGameApp executable target")
    add_executable(BlockGameApp src/engine/core/monolithic_main.c)
    target_link_libraries(BlockGameApp PRIVATE Game Engine ZLIB::ZLIB Threads::Threads "-framework Metal" "-framework MetalKit" "-framework Cocoa" "-framework IOKit" "-framework QuartzCore")
endif()

# ===========================================
# Demo Applications (Added 2026-01-04)
# ===========================================
# Demos temporarily disabled following engine restructuring
# Physics Sandbox Demo
# add_executable(demo_sandbox src/game/demo_physics_sandbox.c
#     src/engine/core/logging/unified_logger.c
#     src/engine/core/memory.c
#     # Engine sources are linked via the Engine library
# )
# if(APPLE)
#     target_link_libraries(demo_sandbox PRIVATE Engine glfw "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
# else()
#     target_link_libraries(demo_sandbox PRIVATE Engine glfw)
# endif()
# # Simple Terrain Demo
# add_executable(demo_terrain src/game/demo_simple_terrain.c
#     src/engine/core/logging/unified_logger.c
#     src/engine/core/memory.c
#     src/engine/core/window.c
#     src/engine/core/utils.c
#     src/engine/core/string_utils.c
# )
# 
# if(APPLE)
#     target_link_libraries(demo_terrain PRIVATE glfw "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
# endif()
# 
# message(STATUS "Demo applications configured: demo_sandbox, demo_terrain")

# Unit Tests
# add_subdirectory(src/engine/test)

# Unit Tests with Catch2 (Phase 1)
# TODO: Resolve CXX compiler feature detection issue
# if(EXISTS "${CMAKE_SOURCE_DIR}/tests/unit/CMakeLists.txt")
#     add_subdirectory(tests/unit)
# endif()

# add_executable(test_asset_instances test/test_asset_instances.c)

# target_link_libraries(test_asset_instances Engine)
# target_include_directories(test_asset_instances PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )

# ===========================================
# Unit Tests (Temporarily disabled due to missing files)
# ===========================================
enable_testing()

# PHASE 1: Integrate Catch2 testing framework
if(EXISTS "${CMAKE_SOURCE_DIR}/Vendor/Catch2/CMakeLists.txt")
    message(STATUS "Catch2 found, enabling modern unit tests")
    add_subdirectory(Vendor/Catch2)
    
    # Phase 1 Build System Tests
    # add_executable(test_phase1_build_system tests/unit/test_phase1_build_system.cpp)
    # target_link_libraries(test_phase1_build_system PRIVATE Engine Catch2::Catch2WithMain)
    # target_include_directories(test_phase1_build_system PRIVATE
    #     ${CMAKE_SOURCE_DIR}/src/engine/include
    #     ${CMAKE_SOURCE_DIR}/src/engine
    # )
    # add_test(NAME Phase1_BuildSystem COMMAND test_phase1_build_system)
else()
    message(WARNING "Catch2 not found in Vendor/Catch2")
endif()

# Helper macro to add test
macro(add_engine_test name source)
    add_executable(${name} ${source} ${CMAKE_SOURCE_DIR}/tests/unit/test_framework_unified.c)
    target_link_libraries(${name} PRIVATE Engine)
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/tests
    )
    add_test(NAME ${name} COMMAND ${name})
endmacro()

# Physics Unit Tests
# add_engine_test(test_physics_ids src/engine/physics/tests/test_physics_ids.c)

# Network Unit Tests
# add_engine_test(test_network_stability src/engine/network/tests/network_stability_tests.c)

# Physics Integration Tests & Benchmark (Physics DOD - Phase 2)
# add_engine_test(test_physics_integration tests/test_physics_integration.c)

# AABB Tree Tests (Physics DOD - Phase 3)
# add_engine_test(test_aabb_tree tests/test_aabb_tree.c)

# GJK/EPA Tests (Physics DOD - Phase 4)
# add_engine_test(test_collision_gjk_epa tests/test_collision_gjk_epa.c)

# Material System Tests
# add_engine_test(test_material_system test/test_material_system.c)

# Post-Processing Tests
# add_engine_test(test_post_processing test/test_post_processing.c)

# ===========================================
# Unit Tests - AI Subsystem
# ===========================================
# add_engine_test(test_ai_command_processor tests/unit/ai/test_ai_command_processor.c)
# add_engine_test(test_content_manager tests/unit/ai/test_content_manager.c)
# add_engine_test(test_perception_system tests/unit/ai/test_perception_system.c)
# add_engine_test(test_utility_ai tests/unit/ai/test_utility_ai.c)
# add_engine_test(test_goap_enhanced tests/unit/ai/test_goap_enhanced.c)
# add_engine_test(test_memory_system tests/unit/ai/test_memory_system.c)

# ===========================================
# Unit Tests - Audio Subsystem
# ===========================================
# add_engine_test(test_audio_system tests/unit/audio/test_audio_system.c)
# add_engine_test(test_audio_manager tests/unit/audio/test_audio_manager.c)

# ===========================================
# Unit Tests - Core Subsystem
# ===========================================
# add_engine_test(test_logger tests/unit/core/test_logger.c)
# add_engine_test(test_memory tests/unit/core/test_memory.c)
# add_engine_test(test_string_utils tests/unit/core/test_string_utils.c)
# add_engine_test(test_cache tests/unit/core/test_cache.c)
# add_engine_test(test_hashmap tests/unit/core/test_hashmap.c)

# Verification Test for New Core Architecture
add_engine_test(test_core_init tests/unit/core/test_core_init.c)

# ===========================================
# Unit Tests - ECS Subsystem
# ===========================================
# add_engine_test(test_ecs tests/unit/ecs/test_ecs.c)

# ===========================================
# Unit Tests - Math Subsystem
# ===========================================
# add_engine_test(test_vec3 tests/unit/math/test_vec3.c)
# add_engine_test(test_mat4 tests/unit/math/test_mat4.c)
# add_engine_test(test_quat tests/unit/math/test_quat.c)

# ===========================================
# Physics Verification Protocols
# AAA-Grade "Proof of Life" Tests
# ===========================================
# add_engine_test(test_protocol1_energy tests/physics_verification/test_protocol1_energy.c)
# add_engine_test(test_protocol2_stacking tests/physics_verification/test_protocol2_stacking.c)
# add_engine_test(test_protocol3_friction tests/physics_verification/test_protocol3_friction.c)
# add_engine_test(test_protocol4_ccd tests/physics_verification/test_protocol4_ccd.c)

# add_engine_test(test_protocol5_determinism tests/physics_verification/test_protocol5_determinism.c)

# ===========================================
# Modern Architecture Tests (Phase 6)
# ===========================================
# add_engine_test(test_vulkan_memory tests/unit/core/test_vulkan_memory.c)
# add_engine_test(test_dynamic_rendering tests/unit/renderer/test_dynamic_rendering.c)
# add_engine_test(test_moltenvk tests/unit/renderer/test_moltenvk.c)
# add_engine_test(test_async_compute tests/unit/renderer/test_async_compute.c)






# ===========================================
# G-Buffer & Deferred Rendering Tests
# ===========================================
# add_executable(test_normal_encoding tests/test_normal_encoding.c)
# target_link_libraries(test_normal_encoding PRIVATE Engine)
# target_include_directories(test_normal_encoding PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )
# 
# add_executable(test_framebuffer tests/test_framebuffer.c)
# target_link_libraries(test_framebuffer PRIVATE Engine)
# target_include_directories(test_framebuffer PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )
# 
# add_executable(test_gbuffer_resize tests/test_gbuffer_resize.c)
# target_link_libraries(test_gbuffer_resize PRIVATE Engine)
# target_include_directories(test_gbuffer_resize PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
#     ${CMAKE_SOURCE_DIR}/src/engine/rendering/gbuffer
# )
# 
# message(STATUS "G-Buffer tests configured: test_normal_encoding, test_framebuffer, test_gbuffer_resize")
# 
# ===========================================
# Render Graph System Tests
# ===========================================
# add_executable(test_frame_graph tests/test_frame_graph.c)
# target_link_libraries(test_frame_graph PRIVATE Engine)
# target_include_directories(test_frame_graph PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )
# 
# message(STATUS "Render Graph tests configured: test_frame_graph")

# ===========================================
# Physics Demos - Visual Physics Simulations
# ===========================================
add_executable(physics_demos
    src/engine/physics/demos/main.c
    src/engine/physics/demos/physics_demo_framework.c
    src/engine/physics/demos/demo_stack_collapse.c
    src/engine/physics/demos/demo_newton_cradle.c
    src/engine/physics/demos/demo_domino_chain.c
    src/engine/physics/demos/demo_ragdoll.c
    src/engine/physics/demos/demo_vehicle_physics.c
    src/engine/physics/demos/demo_fluid_simulation.c
)
target_link_libraries(physics_demos PRIVATE Engine Game ZLIB::ZLIB Threads::Threads "-framework Foundation" "-framework CoreVideo" "-framework CoreGraphics" "-framework QuartzCore" "-framework AppKit")
target_include_directories(physics_demos PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/physics/demos
)
message(STATUS "Physics demos configured: physics_demos")

# ===========================================
# Full Integration Verification
# ===========================================
# add_executable(full_integration_test src/engine/core/integration/full_engine_test.c)
# target_link_libraries(full_integration_test PRIVATE Engine Game ZLIB::ZLIB Threads::Threads "-framework Foundation" "-framework CoreVideo" "-framework CoreGraphics" "-framework QuartzCore" "-framework AppKit")
# target_include_directories(full_integration_test PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )
# message(STATUS "Verification target added: full_integration_test")

# Test Post-Processing Effects
# add_executable(test_post_processing
#     src/engine/rendering/post_processing/post_processing_test.c
# )
# target_link_libraries(test_post_processing PRIVATE Engine Game ZLIB::ZLIB Threads::Threads "-framework Foundation" "-framework CoreVideo" "-framework CoreGraphics" "-framework QuartzCore" "-framework AppKit")
# target_include_directories(test_post_processing PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )
# message(STATUS "Verification target added: test_post_processing")

# Test Player Food
# add_executable(test_player_food
#     tests/integration/test_player_food.c
#     src/game/blockgame/player/player_food.c
#     src/game/blockgame/player/status_effects.c
#     src/game/blockgame/inventory/inventory.c
#     src/game/blockgame/inventory/item_registry.c
#     src/engine/core/logging/unified_logger.c
#     src/engine/core/containers/hashmap.c
#     src/engine/core/memory/memory.c
#     src/engine/core/memory/buddy_allocator.c
#     src/engine/core/memory/linear_allocator.c
#     src/engine/core/string_utils.c
# )

# Test Unreal Engine Quality Features
# add_executable(test_unreal_features
#     test_unreal_features.c
#     src/engine/rendering/lighting/lumen_gi.m
#     src/engine/rendering/advanced/ray_tracing.c
#     src/engine/geometry/nanite/nanite_cluster.c
#     src/engine/rendering/texturing/virtual_texture.c
#     src/engine/rendering/hybrid_renderer.c
#     src/engine/rendering/occlusion/hzb/hzb_temporal.c
#     src/engine/core/logging/unified_logger.c
#     src/engine/core/memory/memory.c
#     src/engine/math/vec3.c
#     src/engine/math/mat4.c
# )
# target_include_directories(test_player_food PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/game/blockgame/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )
# message(STATUS "Verification target added: test_player_food")

# Test Crafting
# add_executable(test_crafting
#     tests/unit/test_crafting.c
#     src/game/blockgame/crafting/furnace.c
#     src/game/blockgame/inventory/inventory.c
#     src/game/blockgame/inventory/item_registry.c
#     src/engine/core/logging/unified_logger.c
#     src/engine/core/containers/hashmap.c
#     src/engine/core/memory/memory.c
#     src/engine/core/memory/buddy_allocator.c
#     src/engine/core/memory/linear_allocator.c
#     src/engine/core/string_utils.c
# )
# target_include_directories(test_crafting PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/game/blockgame/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )
# message(STATUS "Verification target added: test_crafting")
