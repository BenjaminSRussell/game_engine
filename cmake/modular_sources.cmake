# cmake/modular_sources.cmake
# Phase 4 Refactored Modular Sources

set(MODULAR_RUNTIME_SOURCES
    # Platform Bootstrap
    Source/Runtime/Core/Platform/platform_bootstrap.c
    


    # Memory System (Phase 2)
    Source/Runtime/Memory/Memory.c
    Source/Runtime/Memory/Allocators/Arena/Arena_Allocator.c
    
    # Phase 2: Core Memory Allocators
    Source/Runtime/Core/Memory/allocators/arena/arena_allocator.c
    Source/Runtime/Core/Memory/allocators/pool/pool_allocator.c
    Source/Runtime/Core/Memory/allocators/stack/stack_allocator.c
    Source/Runtime/Core/Memory/tracking/memory_tracker.c
    Source/Runtime/Core/Memory/profiling/memory_profiler.c
    
    # Threading System (Phase 3)
    Source/Runtime/Threading/Jobs/Job_System.c
    Source/Runtime/Core/Threading/sync/mutex.c
    Source/Runtime/Core/Threading/sync/semaphore.c
    Source/Runtime/Core/Threading/queues/work_stealing_queue.c
    Source/Runtime/Core/Threading/workers/worker_thread.c
    
    # ECS System (Phase 4)
    Source/Runtime/ECS/ECS.c
    Source/Runtime/ECS/Archetypes/ECS_Archetype.c
    Source/Runtime/ECS/Components/ECS_Component.c
    Source/Runtime/ECS/systems/ECS_System.c
    Source/Runtime/ECS/Queries/ECS_Query.c
    Source/Runtime/ECS/components/Components_Registration.c
    Source/Runtime/ECS/systems/Physics_System.c
    Source/Runtime/ECS/systems/Particle_System.c
    Source/Runtime/ECS/systems/Audio_System.c
    
    # Render System (Phase 5)
    Source/Runtime/Render/Render.c
    
    # Physics System (Phase 6)
    Source/Runtime/Physics/Physics.c
    
    # Audio System (Phase 7)
    Source/Runtime/Audio/Audio.c

    # AI System (Phase 8)
    Source/Runtime/AI/AI.c
    
    # Animation System (Phase 9)
    Source/Runtime/Animation/Animation.c
    
    # Scripting System (Phase 10)
    Source/Runtime/Scripting/Scripting.c
    
    # Networking System (Phase 11)
    Source/Runtime/Networking/Networking.c
    
    # UI System (Phase 12)
    Source/Runtime/UI/UI.c
    
    # Platform System (Phase 13)
    Source/Runtime/Platform/Platform.c
    
    # Core Log System (Phase 14)
    Source/Runtime/Core/Log/Log.c
    
    # Asset System (Phase 14)
    Source/Runtime/Asset/Asset.c
    
    # Shader System (Phase 14)
    Source/Runtime/Shaders/Shader_System.c
    
    # Input System
    Source/Runtime/Input/Input.c
    
    # Profiler System (Phase 16)
    Source/Runtime/Profiling/Profiler.c
    

    
    # Config

    
    # Camera

    
    # World Generation

    
    # Engine Loop

    Source/Runtime/Core/Engine.c
    
    # Launch
    Source/Runtime/Launch/main.c
)

message(STATUS "Modular runtime sources configured (${CMAKE_CURRENT_LIST_FILE})")
