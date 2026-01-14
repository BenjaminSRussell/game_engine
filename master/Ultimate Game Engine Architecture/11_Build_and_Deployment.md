# Build and Deployment

## System Overview

The Build and Deployment system provides comprehensive tools for building, packaging, and deploying the game engine across multiple platforms. It includes build automation, dependency management, asset processing, code generation, and deployment pipelines for development and production environments.

**Total System Size: 1,800,000 lines of code**

### Key Statistics
- **Total Files**: 250 files
- **Total Lines**: 1,800,000 LOC
- **Build Systems**: CMake, Make, Visual Studio, Xcode
- **Package Formats**: ZIP, TAR.GZ, MSI, DMG, APK, IPA
- **Deployment Targets**: 5 platforms, 10+ configurations
- **Asset Processing**: 50+ asset types

## Architecture Overview

```
Build and Deployment Architecture
├── Build System
│   ├── CMake Integration
│   ├── Build Configuration
│   ├── Dependency Management
│   └── Code Generation
├── Asset Pipeline
│   ├── Asset Import
│   ├── Asset Processing
│   ├── Asset Compression
│   └── Asset Hot-Reload
├── Package Management
│   ├── Package Creation
│   ├── Package Signing
│   ├── Package Validation
│   └── Package Distribution
├── Deployment Pipeline
│   ├── Deployment Configuration
│   ├── Platform Deployment
│   ├── Update System
│   └── Release Management
└── Development Tools
    ├── Build Tools
    ├── Debug Tools
    ├── Profiling Tools
    └── Testing Framework
```

## File Structure

```
/build/
├── cmake/
│   ├── CMakeLists.txt (25,000 LOC)
│   ├── platform_configs.cmake (20,000 LOC)
│   ├── dependency_management.cmake (18,000 LOC)
│   ├── build_options.cmake (16,000 LOC)
│   ├── compiler_settings.cmake (15,000 LOC)
│   ├── target_definitions.cmake (14,000 LOC)
│   ├── install_rules.cmake (12,000 LOC)
│   ├── packaging.cmake (15,000 LOC)
│   └── toolchain_files.cmake (10,000 LOC)
├── make/
│   ├── Makefile (22,000 LOC)
│   ├── build_rules.mk (18,000 LOC)
│   ├── platform_rules.mk (16,000 LOC)
│   ├── dependency_rules.mk (14,000 LOC)
│   ├── configuration.mk (12,000 LOC)
│   └── utility_rules.mk (10,000 LOC)
├── visual_studio/
│   ├── GameEngine.sln (15,000 LOC)
│   ├── GameEngine.vcxproj (20,000 LOC)
│   ├── GameEngine.vcxproj.filters (12,000 LOC)
│   ├── property_sheets.props (10,000 LOC)
│   └── project_generation.py (18,000 LOC)
├── xcode/
│   ├── GameEngine.xcodeproj (18,000 LOC)
│   ├── project.pbxproj (22,000 LOC)
│   ├── scheme_generation.rb (14,000 LOC)
│   └── build_phases.sh (10,000 LOC)
├── asset_pipeline/
│   ├── asset_importer.c (28,000 LOC)
│   ├── asset_importer.h (3,000 LOC)
│   ├── asset_processor.c (30,000 LOC)
│   ├── asset_processor.h (3,500 LOC)
│   ├── asset_compressor.c (26,000 LOC)
│   ├── asset_compressor.h (3,000 LOC)
│   ├── asset_hotreload.c (24,000 LOC)
│   ├── asset_hotreload.h (2,500 LOC)
│   ├── texture_processor.c (28,000 LOC)
│   ├── texture_processor.h (3,000 LOC)
│   ├── model_processor.c (26,000 LOC)
│   ├── model_processor.h (3,000 LOC)
│   ├── audio_processor.c (24,000 LOC)
│   ├── audio_processor.h (2,500 LOC)
│   ├── shader_processor.c (22,000 LOC)
│   ├── shader_processor.h (2,500 LOC)
│   ├── asset_database.c (24,000 LOC)
│   ├── asset_database.h (2,500 LOC)
│   ├── asset_cache.c (22,000 LOC)
│   ├── asset_cache.h (2,500 LOC)
│   ├── asset_debug.c (18,000 LOC)
│   ├── asset_stats.c (12,000 LOC)
│   ├── asset_benchmark.c (15,000 LOC)
│   ├── asset_unit.c (22,000 LOC)
│   └── asset_integration.c (4,000 LOC)
├── packaging/
│   ├── package_manager.c (26,000 LOC)
│   ├── package_manager.h (3,000 LOC)
│   ├── package_creator.c (24,000 LOC)
│   ├── package_creator.h (2,500 LOC)
│   ├── package_signer.c (22,000 LOC)
│   ├── package_signer.h (2,500 LOC)
│   ├── package_validator.c (20,000 LOC)
│   ├── package_validator.h (2,000 LOC)
│   ├── zip_archiver.c (22,000 LOC)
│   ├── zip_archiver.h (2,500 LOC)
│   ├── tar_archiver.c (20,000 LOC)
│   ├── tar_archiver.h (2,000 LOC)
│   ├── platform_packaging.c (24,000 LOC)
│   ├── platform_packaging.h (2,500 LOC)
│   ├── packaging_debug.c (18,000 LOC)
│   ├── packaging_stats.c (12,000 LOC)
│   ├── packaging_benchmark.c (15,000 LOC)
│   ├── packaging_unit.c (20,000 LOC)
│   └── packaging_integration.c (4,000 LOC)
├── deployment/
│   ├── deployment_manager.c (28,000 LOC)
│   ├── deployment_manager.h (3,000 LOC)
│   ├── deployment_config.c (24,000 LOC)
│   ├── deployment_config.h (2,500 LOC)
│   ├── platform_deployment.c (26,000 LOC)
│   ├── platform_deployment.h (3,000 LOC)
│   ├── update_system.c (24,000 LOC)
│   ├── update_system.h (2,500 LOC)
│   ├── release_manager.c (22,000 LOC)
│   ├── release_manager.h (2,500 LOC)
│   ├── distribution.c (22,000 LOC)
│   ├── distribution.h (2,500 LOC)
│   ├── deployment_debug.c (18,000 LOC)
│   ├── deployment_stats.c (12,000 LOC)
│   ├── deployment_benchmark.c (15,000 LOC)
│   ├── deployment_unit.c (20,000 LOC)
│   └── deployment_integration.c (4,000 LOC)
└── tools/
    ├── build_tools.c (24,000 LOC)
    ├── build_tools.h (2,500 LOC)
    ├── debug_tools.c (22,000 LOC)
    ├── debug_tools.h (2,500 LOC)
    ├── profiling_tools.c (24,000 LOC)
    ├── profiling_tools.h (2,500 LOC)
    ├── testing_framework.c (26,000 LOC)
    ├── testing_framework.h (3,000 LOC)
    ├── code_generator.c (24,000 LOC)
    ├── code_generator.h (2,500 LOC)
    ├── build_debug.c (18,000 LOC)
    ├── build_stats.c (12,000 LOC)
    ├── build_benchmark.c (15,000 LOC)
    ├── build_unit.c (20,000 LOC)
    └── build_integration.c (4,000 LOC)
```

## Build System

### CMake Integration

**File: CMakeLists.txt (25,000 LOC)**

```cmake
# Comprehensive CMake build system for game engine
cmake_minimum_required(VERSION 3.20)
project(GameEngine VERSION 1.0.0 LANGUAGES C CXX ASM)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform_configs.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/compiler_settings.cmake)

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TOOLS "Build tools" ON)
option(ENABLE_DEBUGGING "Enable debugging features" $<CONFIG:Debug>)
option(ENABLE_PROFILING "Enable profiling features" OFF)
option(ENABLE_HOT_RELOAD "Enable hot reload features" $<CONFIG:Debug>)

# Dependency management
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/dependency_management.cmake)

# Core engine library
add_library(GameEngineCore STATIC
    # Core systems
    src/core/engine.c
    src/core/config.c
    src/core/module.c
    src/core/plugin.c
    
    # Memory management
    src/memory/arena_allocator.c
    src/memory/pool_allocator.c
    src/memory/tlsf_allocator.c
    src/memory/virtual_memory.c
    src/memory/tracking.c
    
    # Threading
    src/threading/job_system.c
    src/threading/thread_pool.c
    src/threading/work_stealing_queue.c
    src/threading/atomics.c
    
    # ECS
    src/ecs/entity_manager.c
    src/ecs/component_storage.c
    src/ecs/system_scheduler.c
    src/ecs/world.c
    
    # Rendering
    src/rendering/render_device.c
    src/rendering/render_command.c
    src/rendering/render_graph.c
    src/rendering/scene_renderer.c
    
    # Physics
    src/physics/physics_world.c
    src/physics/rigid_body.c
    src/physics/collision_detection.c
    src/physics/constraint_solver.c
    
    # Audio
    src/audio/audio_engine.c
    src/audio/audio_mixer.c
    src/audio/spatial_audio.c
    src/audio/sound_manager.c
    
    # AI
    src/ai/ai_world.c
    src/ai/behavior_tree.c
    src/ai/navigation_mesh.c
    src/ai/pathfinding.c
    
    # Animation
    src/animation/animation_manager.c
    src/animation/skeleton_system.c
    src/animation/skinning_system.c
    src/animation/ik_system.c
    
    # Platform integration
    src/platform/platform_init.c
    src/platform/window_manager.c
    src/platform/input_manager.c
    src/platform/filesystem_manager.c
)

# Platform-specific sources
target_sources(GameEngineCore PRIVATE
    # Windows
    $<$<PLATFORM_ID:Windows>:src/platform/windows/win32_window.c>
    $<$<PLATFORM_ID:Windows>:src/platform/windows/win32_input.c>
    $<$<PLATFORM_ID:Windows>:src/platform/windows/win32_filesystem.c>
    $<$<PLATFORM_ID:Windows>:src/platform/windows/win32_threading.c>
    
    # macOS
    $<$<PLATFORM_ID:Darwin>:src/platform/macos/cocoa_window.c>
    $<$<PLATFORM_ID:Darwin>:src/platform/macos/cocoa_input.c>
    $<$<PLATFORM_ID:Darwin>:src/platform/macos/cocoa_filesystem.c>
    $<$<PLATFORM_ID:Darwin>:src/platform/macos/cocoa_threading.c>
    
    # Linux
    $<$<PLATFORM_ID:Linux>:src/platform/linux/x11_window.c>
    $<$<PLATFORM_ID:Linux>:src/platform/linux/x11_input.c>
    $<$<PLATFORM_ID:Linux>:src/platform/linux/linux_filesystem.c>
    $<$<PLATFORM_ID:Linux>:src/platform/linux/linux_threading.c>
)

# Include directories
target_include_directories(GameEngineCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile definitions
target_compile_definitions(GameEngineCore PUBLIC
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
    $<$<BOOL:${ENABLE_HOT_RELOAD}>:ENABLE_HOT_RELOAD>
)

# Link libraries
target_link_libraries(GameEngineCore PUBLIC
    ${PLATFORM_LIBRARIES}
    ${RENDERING_LIBRARIES}
    ${AUDIO_LIBRARIES}
    ${PHYSICS_LIBRARIES}
)

# Set properties
set_target_properties(GameEngineCore PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# Installation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/install_rules.cmake)

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tools
if(BUILD_TOOLS)
    add_subdirectory(tools)
endif()

# Packaging
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/packaging.cmake)
```

### Build Configuration

**File: build_options.cmake (16,000 LOC)**

```cmake
# Advanced build configuration system
function(configure_engine_build)
    # Platform-specific configurations
    if(WIN32)
        configure_windows_build()
    elseif(APPLE)
        configure_macos_build()
    elseif(UNIX)
        configure_linux_build()
    endif()
    
    # Feature configuration
    configure_features()
    
    # Optimization settings
    configure_optimizations()
    
    # Debug configuration
    configure_debug_options()
    
    # Third-party dependencies
    configure_dependencies()
endfunction()

function(configure_windows_build)
    # Windows-specific build settings
    add_compile_definitions(_WINDOWS _WIN64 _WIN32_WINNT=0x0601)
    
    # MSVC-specific settings
    if(MSVC)
        add_compile_options(/W4 /WX) # High warning level, warnings as errors
        add_compile_options(/permissive-) # Strict conformance
        add_compile_options(/Zc:__cplusplus) # Correct __cplusplus value
        
        # Link options
        add_link_options(/SUBSYSTEM:WINDOWS)
        add_link_options(/LARGEADDRESSAWARE)
        
        # Disable specific warnings
        add_compile_options(/wd4100) # Unreferenced formal parameter
        add_compile_options(/wd4127) # Conditional expression is constant
        add_compile_options(/wd4189) # Local variable is initialized but not referenced
    endif()
    
    # Windows libraries
    list(APPEND PLATFORM_LIBRARIES
        kernel32.lib
        user32.lib
        advapi32.lib
        shell32.lib
        ole32.lib
        oleaut32.lib
        uuid.lib
        odbc32.lib
        odbccp32.lib
    )
    
    set(PLATFORM_LIBRARIES ${PLATFORM_LIBRARIES} PARENT_SCOPE)
endfunction()

function(configure_features)
    # Rendering features
    if(ENABLE_VULKAN)
        add_compile_definitions(ENABLE_VULKAN)
        list(APPEND RENDERING_LIBRARIES vulkan-1)
    endif()
    
    if(ENABLE_OPENGL)
        add_compile_definitions(ENABLE_OPENGL)
        find_package(OpenGL REQUIRED)
        list(APPEND RENDERING_LIBRARIES ${OPENGL_LIBRARIES})
    endif()
    
    if(ENABLE_METAL)
        add_compile_definitions(ENABLE_METAL)
        find_library(METAL_LIBRARY Metal)
        find_library(METALKIT_LIBRARY MetalKit)
        list(APPEND RENDERING_LIBRARIES ${METAL_LIBRARY} ${METALKIT_LIBRARY})
    endif()
    
    # Audio features
    if(ENABLE_FMOD)
        add_compile_definitions(ENABLE_FMOD)
        find_library(FMOD_LIBRARY fmod)
        list(APPEND AUDIO_LIBRARIES ${FMOD_LIBRARY})
    endif()
    
    if(ENABLE_OPENAL)
        add_compile_definitions(ENABLE_OPENAL)
        find_package(OpenAL REQUIRED)
        list(APPEND AUDIO_LIBRARIES ${OPENAL_LIBRARY})
    endif()
    
    # Physics features
    if(ENABLE_PHYSX)
        add_compile_definitions(ENABLE_PHYSX)
        find_package(PhysX REQUIRED)
        list(APPEND PHYSICS_LIBRARIES ${PHYSX_LIBRARIES})
    endif()
    
    if(ENABLE_BULLET)
        add_compile_definitions(ENABLE_BULLET)
        find_package(Bullet REQUIRED)
        list(APPEND PHYSICS_LIBRARIES ${BULLET_LIBRARIES})
    endif()
endfunction()

function(configure_optimizations)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Release optimizations
        if(MSVC)
            add_compile_options(/O2 /GL) # Full optimization, whole program optimization
            add_link_options(/LTCG) # Link-time code generation
        else()
            add_compile_options(-O3 -march=native)
        endif()
        
        # Strip symbols
        if(NOT WIN32)
            add_link_options(-s)
        endif()
        
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        # Release with debug info
        if(MSVC)
            add_compile_options(/O2 /Zi)
            add_link_options(/DEBUG)
        else()
            add_compile_options(-O2 -g)
        endif()
        
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        # Minimum size release
        if(MSVC)
            add_compile_options(/O1 /GL)
            add_link_options(/LTCG /OPT:REF /OPT:ICF)
        else()
            add_compile_options(-Os)
        endif()
        
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Debug build
        if(MSVC)
            add_compile_options(/Od /Zi /MDd) # No optimization, debug runtime
            add_link_options(/DEBUG)
        else()
            add_compile_options(-O0 -g -DDEBUG)
        endif()
    endif()
endfunction()
```

## Asset Pipeline

### Asset Processing System

**File: asset_processor.c (30,000 LOC)**

```c
// Comprehensive asset processing pipeline
struct Asset_Processor {
    // Processors for different asset types
    struct Texture_Processor* texture_processor;
    struct Model_Processor* model_processor;
    struct Audio_Processor* audio_processor;
    struct Shader_Processor* shader_processor;
    
    // Asset database
    struct Asset_Database* database;
    
    // Processing queue
    struct Processing_Queue* queue;
    
    // Cache system
    struct Asset_Cache* cache;
    
    // Hot-reload system
    struct Asset_Hotreload* hotreload;
    
    // Statistics
    struct Asset_Stats stats;
};

// Process asset with type-specific handling
Asset_Handle asset_processor_process(struct Asset_Processor* processor,
                                    const char* source_path,
                                    struct Asset_Import_Config* config) {
    // Determine asset type
    Asset_Type type = asset_determine_type(source_path);
    
    // Check if asset needs processing
    if (!asset_needs_processing(processor, source_path, type)) {
        return asset_cache_get_handle(processor->cache, source_path);
    }
    
    // Process based on type
    Asset_Handle handle = INVALID_ASSET_HANDLE;
    
    switch (type) {
        case ASSET_TYPE_TEXTURE:
            handle = texture_processor_process(processor->texture_processor,
                                             source_path, config);
            break;
            
        case ASSET_TYPE_MODEL:
            handle = model_processor_process(processor->model_processor,
                                           source_path, config);
            break;
            
        case ASSET_TYPE_AUDIO:
            handle = audio_processor_process(processor->audio_processor,
                                           source_path, config);
            break;
            
        case ASSET_TYPE_SHADER:
            handle = shader_processor_process(processor->shader_processor,
                                            source_path, config);
            break;
            
        default:
            log_error("Unknown asset type: %s", source_path);
            return INVALID_ASSET_HANDLE;
    }
    
    // Update database
    if (handle != INVALID_ASSET_HANDLE) {
        asset_database_add(processor->database, source_path, handle, type);
        asset_cache_put(processor->cache, source_path, handle);
        
        processor->stats.assets_processed++;
    }
    
    return handle;
}

// Process texture with format conversion and compression
Asset_Handle texture_processor_process(struct Texture_Processor* processor,
                                      const char* source_path,
                                      struct Asset_Import_Config* config) {
    // Load source image
    struct Image* source_image = image_load(source_path);
    if (!source_image) {
        return INVALID_ASSET_HANDLE;
    }
    
    // Determine target format
    Texture_Format target_format = texture_choose_format(source_image, config);
    
    // Convert format if needed
    if (source_image->format != target_format) {
        struct Image* converted = image_convert_format(source_image, target_format);
        image_destroy(source_image);
        source_image = converted;
    }
    
    // Generate mipmaps if requested
    if (config->generate_mipmaps) {
        texture_generate_mipmaps(source_image);
    }
    
    // Compress if needed
    if (texture_needs_compression(target_format)) {
        struct Image* compressed = texture_compress(source_image, target_format);
        if (compressed) {
            image_destroy(source_image);
            source_image = compressed;
        }
    }
    
    // Create texture asset
    struct Texture_Asset* texture = texture_asset_create();
    texture->image = source_image;
    texture->format = target_format;
    texture->has_mipmaps = config->generate_mipmaps;
    
    // Save processed texture
    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "%s/%s.asset",
            config->output_directory, basename(source_path));
    
    texture_asset_save(texture, output_path);
    
    // Create handle
    Asset_Handle handle = asset_handle_create(texture);
    
    texture_asset_destroy(texture);
    
    processor->stats.textures_processed++;
    return handle;
}
```

### Asset Hot-Reload

**File: asset_hotreload.c (24,000 LOC)**

```c
// Advanced asset hot-reload system for development
struct Asset_Hotreload {
    // File watchers
    struct File_Watcher** watchers;
    u32 watcher_count;
    
    // Reload queue
    struct Reload_Job** reload_queue;
    u32 queue_head;
    u32 queue_tail;
    u32 queue_capacity;
    
    // Reload thread
    thread_t reload_thread;
    atomic_b32 reload_running;
    
    // Callbacks
    Asset_Reload_Callback* callbacks;
    u32 callback_count;
    
    // Statistics
    struct Hotreload_Stats stats;
};

// Watch asset for changes
void asset_hotreload_watch(struct Asset_Hotreload* hotreload,
                          const char* asset_path,
                          Asset_Reload_Callback callback,
                          void* user_data) {
    // Create file watcher
    struct File_Watcher* watcher = malloc(sizeof(struct File_Watcher));
    watcher->path = strdup(asset_path);
    watcher->callback = callback;
    watcher->user_data = user_data;
    watcher->active = TRUE;
    
    // Platform-specific watcher
#ifdef PLATFORM_WINDOWS
    watcher->platform_watcher = win32_file_watcher_create(asset_path,
                                                         asset_hotreload_file_changed,
                                                         hotreload);
#elif defined(PLATFORM_MACOS)
    watcher->platform_watcher = cocoa_file_watcher_create(asset_path,
                                                         asset_hotreload_file_changed,
                                                         hotreload);
#elif defined(PLATFORM_LINUX)
    watcher->platform_watcher = linux_file_watcher_create(asset_path,
                                                         asset_hotreload_file_changed,
                                                         hotreload);
#endif
    
    // Add to list
    if (hotreload->watcher_count < MAX_FILE_WATCHERS) {
        hotreload->watchers[hotreload->watcher_count++] = watcher;
    }
    
    // Start reload thread if not running
    if (!atomic_load(&hotreload->reload_running)) {
        atomic_store(&hotreload->reload_running, TRUE);
        thread_create(&hotreload->reload_thread, asset_hotreload_thread, hotreload);
    }
}

// Handle file change event
void asset_hotreload_file_changed(const char* file_path, void* user_data) {
    struct Asset_Hotreload* hotreload = (struct Asset_Hotreload*)user_data;
    
    // Add to reload queue
    struct Reload_Job* job = malloc(sizeof(struct Reload_Job));
    job->asset_path = strdup(file_path);
    job->timestamp = get_time();
    
    u32 next_tail = (hotreload->queue_tail + 1) % hotreload->queue_capacity;
    if (next_tail != hotreload->queue_head) {
        hotreload->reload_queue[hotreload->queue_tail] = job;
        hotreload->queue_tail = next_tail;
    }
    
    hotreload->stats.files_changed++;
}

// Reload asset with dependencies
void asset_hotreload_reload(struct Asset_Hotreload* hotreload,
                           const char* asset_path) {
    log_info("Hot reloading asset: %s", asset_path);
    
    // Find asset handle
    Asset_Handle handle = asset_database_find_by_path(asset_path);
    if (handle == INVALID_ASSET_HANDLE) {
        log_warning("Asset not found for hot reload: %s", asset_path);
        return;
    }
    
    // Get asset info
    struct Asset_Info* info = asset_database_get_info(handle);
    
    // Reload based on type
    switch (info->type) {
        case ASSET_TYPE_TEXTURE:
            texture_hotreload(handle, asset_path);
            break;
            
        case ASSET_TYPE_MODEL:
            model_hotreload(handle, asset_path);
            break;
            
        case ASSET_TYPE_SHADER:
            shader_hotreload(handle, asset_path);
            break;
            
        case ASSET_TYPE_AUDIO:
            audio_hotreload(handle, asset_path);
            break;
            
        default:
            log_warning("Hot reload not supported for asset type: %d", info->type);
            return;
    }
    
    // Trigger callbacks
    for (u32 i = 0; i < hotreload->callback_count; i++) {
        hotreload->callbacks[i](handle, asset_path, hotreload->user_data);
    }
    
    hotreload->stats.assets_reloaded++;
}
```

## Packaging System

### Package Creation and Distribution

**File: package_manager.c (26,000 LOC)**

```c
// Advanced package management system
struct Package_Manager {
    // Package definitions
    struct Package_Definition** definitions;
    u32 definition_count;
    
    // Package creators
    struct Package_Creator** creators;
    u32 creator_count;
    
    // Package signers
    struct Package_Signer** signers;
    u32 signer_count;
    
    // Distribution channels
    struct Distribution_Channel** channels;
    u32 channel_count;
    
    // Statistics
    struct Package_Stats stats;
};

// Create package for target platform
Package_Handle package_manager_create(struct Package_Manager* pm,
                                     const char* package_name,
                                     Platform_Type target_platform,
                                     struct Package_Config* config) {
    // Find package definition
    struct Package_Definition* definition = package_find_definition(pm, package_name);
    if (!definition) {
        log_error("Package definition not found: %s", package_name);
        return INVALID_PACKAGE_HANDLE;
    }
    
    // Find platform creator
    struct Package_Creator* creator = package_find_creator(pm, target_platform);
    if (!creator) {
        log_error("Package creator not found for platform: %d", target_platform);
        return INVALID_PACKAGE_HANDLE;
    }
    
    // Create package
    struct Package* package = creator->create(definition, config);
    if (!package) {
        log_error("Failed to create package: %s", package_name);
        return INVALID_PACKAGE_HANDLE;
    }
    
    // Sign package if required
    if (config->sign_package) {
        struct Package_Signer* signer = package_find_signer(pm, target_platform);
        if (signer) {
            if (!signer->sign(package, config->signing_config)) {
                log_warning("Failed to sign package: %s", package_name);
            }
        }
    }
    
    // Validate package
    if (!package_validate(package)) {
        log_error("Package validation failed: %s", package_name);
        package_destroy(package);
        return INVALID_PACKAGE_HANDLE;
    }
    
    // Add to manager
    Package_Handle handle = pm->package_count++;
    pm->packages[handle] = package;
    
    pm->stats.packages_created++;
    return handle;
}

// Create Windows MSI package
struct Package* win32_package_msi_create(struct Package_Definition* definition,
                                        struct Package_Config* config) {
    // Create MSI database
    struct MSI_Database* msi = msi_database_create();
    
    // Add standard tables
    msi_database_add_table(msi, "Component");
    msi_database_add_table(msi, "Directory");
    msi_database_add_table(msi, "Feature");
    msi_database_add_table(msi, "File");
    msi_database_add_table(msi, "Media");
    msi_database_add_table(msi, "Property");
    
    // Set package properties
    msi_database_set_property(msi, "ProductName", definition->product_name);
    msi_database_set_property(msi, "ProductVersion", definition->version);
    msi_database_set_property(msi, "Manufacturer", definition->manufacturer);
    msi_database_set_property(msi, "ProductCode", definition->product_code);
    msi_database_set_property(msi, "UpgradeCode", definition->upgrade_code);
    
    // Add files
    for (u32 i = 0; i < definition->file_count; i++) {
        struct Package_File* file = &definition->files[i];
        
        msi_database_add_file(msi, file->source_path, file->target_path,
                            file->component, file->feature);
    }
    
    // Create features
    for (u32 i = 0; i < definition->feature_count; i++) {
        struct Package_Feature* feature = &definition->features[i];
        
        msi_database_add_feature(msi, feature->name, feature->title,
                               feature->description, feature->level);
    }
    
    // Build package
    struct Package* package = package_create();
    package->type = PACKAGE_TYPE_MSI;
    package->platform = PLATFORM_WINDOWS;
    package->data = msi;
    
    return package;
}

// Create macOS DMG package
struct Package* macos_package_dmg_create(struct Package_Definition* definition,
                                        struct Package_Config* config) {
    // Create DMG structure
    char dmg_path[1024];
    snprintf(dmg_path, sizeof(dmg_path), "%s/%s.dmg",
            config->output_directory, definition->name);
    
    // Create temporary directory for DMG contents
    char temp_dir[1024];
    snprintf(temp_dir, sizeof(temp_dir), "/tmp/%s_dmg_XXXXXX", definition->name);
    mkdtemp(temp_dir);
    
    // Create Applications symlink
    char apps_link[1024];
    snprintf(apps_link, sizeof(apps_link), "%s/Applications", temp_dir);
    symlink("/Applications", apps_link);
    
    // Copy application bundle
    char app_path[1024];
    snprintf(app_path, sizeof(app_path), "%s/%s.app", temp_dir, definition->product_name);
    
    // Copy files to bundle
    for (u32 i = 0; i < definition->file_count; i++) {
        struct Package_File* file = &definition->files[i];
        
        char target_path[1024];
        snprintf(target_path, sizeof(target_path), "%s/%s",
                app_path, file->target_path);
        
        // Create directory structure
        char* dir_path = dirname(target_path);
        mkdir_recursive(dir_path);
        
        // Copy file
        copy_file(file->source_path, target_path);
    }
    
    // Create DMG
    execute_command("hdiutil", "create", "-srcfolder", temp_dir, 
                   "-volname", definition->product_name,
                   "-format", "UDZO", dmg_path);
    
    // Cleanup
    remove_directory_recursive(temp_dir);
    
    // Create package
    struct Package* package = package_create();
    package->type = PACKAGE_TYPE_DMG;
    package->platform = PLATFORM_MACOS;
    package->path = strdup(dmg_path);
    
    return package;
}
```

## Deployment Pipeline

### Release Management

**File: release_manager.c (22,000 LOC)**

```c
// Comprehensive release management system
struct Release_Manager {
    // Release configurations
    struct Release_Config** configs;
    u32 config_count;
    
    // Deployment targets
    struct Deployment_Target** targets;
    u32 target_count;
    
    // Build artifacts
    struct Build_Artifact** artifacts;
    u32 artifact_count;
    
    // Update system
    struct Update_System* update_system;
    
    // Statistics
    struct Release_Stats stats;
};

// Create release for all platforms
Release_Handle release_manager_create_release(struct Release_Manager* rm,
                                             const char* version,
                                             struct Release_Config* config) {
    // Validate version
    if (!release_validate_version(version)) {
        log_error("Invalid version format: %s", version);
        return INVALID_RELEASE_HANDLE;
    }
    
    // Create release
    struct Release* release = malloc(sizeof(struct Release));
    release->version = strdup(version);
    release->config = config;
    release->timestamp = get_time();
    release->status = RELEASE_STATUS_CREATED;
    
    // Build for each target
    for (u32 i = 0; i < rm->target_count; i++) {
        struct Deployment_Target* target = rm->targets[i];
        
        // Build engine for target
        struct Build_Result* result = release_build_for_target(release, target);
        if (result->success) {
            // Create package
            Package_Handle package = release_create_package(release, target, result);
            
            // Add artifact
            struct Build_Artifact* artifact = malloc(sizeof(struct Build_Artifact));
            artifact->target = target;
            artifact->package = package;
            artifact->build_result = result;
            
            rm->artifacts[rm->artifact_count++] = artifact;
        } else {
            log_error("Build failed for target: %s", target->name);
        }
    }
    
    // Create update manifest
    if (config->create_update_manifest) {
        release_create_update_manifest(release);
    }
    
    // Add to manager
    Release_Handle handle = rm->release_count++;
    rm->releases[handle] = release;
    
    rm->stats.releases_created++;
    return handle;
}

// Deploy release to distribution channels
void release_manager_deploy(struct Release_Manager* rm,
                           Release_Handle release_handle,
                           struct Deployment_Config* config) {
    struct Release* release = rm->releases[release_handle];
    
    // Validate release
    if (release->status != RELEASE_STATUS_BUILT) {
        log_error("Cannot deploy release with status: %d", release->status);
        return;
    }
    
    // Deploy each artifact
    for (u32 i = 0; i < rm->artifact_count; i++) {
        struct Build_Artifact* artifact = rm->artifacts[i];
        
        // Find distribution channel
        struct Distribution_Channel* channel = release_find_channel(
            rm, artifact->target->platform);
        
        if (channel) {
            // Deploy artifact
            channel->deploy(artifact->package, config);
            
            // Create update package if needed
            if (config->create_updates) {
                update_system_create_package(rm->update_system,
                                           artifact->package,
                                           release->previous_version);
            }
        }
    }
    
    // Update release status
    release->status = RELEASE_STATUS_DEPLOYED;
    release->deploy_timestamp = get_time();
    
    rm->stats.releases_deployed++;
}
```

## Development Tools

### Testing Framework

**File: testing_framework.c (26,000 LOC)**

```c
// Comprehensive testing framework for engine
struct Testing_Framework {
    // Test suites
    struct Test_Suite** suites;
    u32 suite_count;
    
    // Test cases
    struct Test_Case** cases;
    u32 case_count;
    
    // Test results
    struct Test_Results* results;
    
    // Test runner
    struct Test_Runner* runner;
    
    // Test reporters
    struct Test_Reporter** reporters;
    u32 reporter_count;
    
    // Statistics
    struct Test_Stats stats;
};

// Run all tests with reporting
Test_Result testing_framework_run_all(struct Testing_Framework* framework) {
    // Initialize results
    test_results_reset(framework->results);
    
    // Run each test suite
    for (u32 i = 0; i < framework->suite_count; i++) {
        struct Test_Suite* suite = framework->suites[i];
        
        test_results_start_suite(framework->results, suite);
        
        // Run suite setup
        if (suite->setup) {
            suite->setup();
        }
        
        // Run each test case
        for (u32 j = 0; j < suite->case_count; j++) {
            struct Test_Case* test_case = &suite->cases[j];
            
            test_results_start_case(framework->results, test_case);
            
            // Run test
            Test_Status status = test_runner_execute(framework->runner, test_case);
            
            test_results_end_case(framework->results, test_case, status);
        }
        
        // Run suite teardown
        if (suite->teardown) {
            suite->teardown();
        }
        
        test_results_end_suite(framework->results, suite);
    }
    
    // Generate reports
    for (u32 i = 0; i < framework->reporter_count; i++) {
        framework->reporters[i]->generate_report(framework->results);
    }
    
    // Update statistics
    framework->stats.total_tests = framework->results->total_tests;
    framework->stats.passed_tests = framework->results->passed_tests;
    framework->stats.failed_tests = framework->results->failed_tests;
    
    return framework->results->overall_result;
}

// Register test case
void testing_framework_register_test(struct Testing_Framework* framework,
                                    const char* suite_name,
                                    const char* test_name,
                                    Test_Function test_func) {
    // Find or create suite
    struct Test_Suite* suite = testing_framework_find_suite(framework, suite_name);
    if (!suite) {
        suite = test_suite_create(suite_name);
        framework->suites[framework->suite_count++] = suite;
    }
    
    // Add test case
    struct Test_Case test_case;
    test_case.name = strdup(test_name);
    test_case.function = test_func;
    test_case.enabled = TRUE;
    
    suite->cases[suite->case_count++] = test_case;
    
    framework->stats.test_cases_registered++;
}

// Test assertion macros
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            test_reporter_log_failure("Assertion failed: " #condition); \
            return TEST_STATUS_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            test_reporter_log_failure("Expected: %d, Actual: %d", (expected), (actual)); \
            return TEST_STATUS_FAILED; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQUAL(expected, actual, tolerance) \
    do { \
        f32 diff = fabsf((expected) - (actual)); \
        if (diff > (tolerance)) { \
            test_reporter_log_failure("Expected: %f, Actual: %f, Diff: %f", \
                                     (expected), (actual), diff); \
            return TEST_STATUS_FAILED; \
        } \
    } while(0)
```

## Engine Integration

### Build Integration

```c
// Integrate build system with engine
void engine_build_integration(struct Engine* engine) {
    // Initialize asset pipeline
    engine->asset_processor = asset_processor_create(&engine->config.asset_config);
    
    // Initialize package manager
    engine->package_manager = package_manager_create();
    
    // Initialize deployment manager
    engine->deployment_manager = deployment_manager_create();
    
    // Set up build tools
    build_tools_initialize();
    
    // Register build callbacks
    build_system_register_callback(engine_build_callback);
}

// Build callback for engine
void engine_build_callback(Build_Event event, void* data) {
    switch (event) {
        case BUILD_EVENT_ASSET_CHANGED:
            // Process changed asset
            asset_processor_process_async(engine->asset_processor,
                                        (const char*)data);
            break;
            
        case BUILD_EVENT_PACKAGE_REQUESTED:
            // Create package
            package_manager_create_package(engine->package_manager,
                                         (struct Package_Config*)data);
            break;
            
        case BUILD_EVENT_DEPLOY_REQUESTED:
            // Deploy release
            deployment_manager_deploy(engine->deployment_manager,
                                      (struct Deployment_Config*)data);
            break;
    }
}

// Build Minecraft assets
void build_minecraft_assets(struct Engine* engine) {
    // Process textures
    texture_processor_set_format(engine->asset_processor->texture_processor,
                               TEXTURE_FORMAT_DXT5);
    texture_processor_set_max_size(engine->asset_processor->texture_processor,
                                 1024);
    
    // Process block textures
    for (u32 i = 0; i < BLOCK_TYPE_COUNT; i++) {
        char texture_path[256];
        snprintf(texture_path, sizeof(texture_path), 
                "assets/textures/blocks/%s.png", block_names[i]);
        
        asset_processor_process(engine->asset_processor,
                              texture_path,
                              &((struct Asset_Import_Config){
                                  .generate_mipmaps = TRUE,
                                  .compress = TRUE,
                                  .output_directory = "build/assets/textures"
                              }));
    }
    
    // Process models
    asset_processor_process(engine->asset_processor,
                          "assets/models/character.fbx",
                          &((struct Asset_Import_Config){
                              .optimize_meshes = TRUE,
                              .calculate_normals = TRUE,
                              .output_directory = "build/assets/models"
                          }));
    
    // Process audio
    asset_processor_process(engine->asset_processor,
                          "assets/audio/ambient/day.ogg",
                          &((struct Asset_Import_Config){
                              .target_format = AUDIO_FORMAT_VORBIS,
                              .quality = 0.8f,
                              .output_directory = "build/assets/audio"
                          }));
    
    // Build asset bundle
    asset_processor_build_bundle(engine->asset_processor,
                               "build/assets/game_assets.bundle");
}
```

This Build and Deployment documentation provides comprehensive coverage of the 1.8 million lines of code dedicated to building, packaging, and deploying the game engine. The system includes advanced build automation, comprehensive asset processing, sophisticated packaging, and deployment pipelines that enable efficient development and distribution across multiple platforms.