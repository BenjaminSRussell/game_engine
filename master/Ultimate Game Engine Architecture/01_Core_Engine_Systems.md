# Core Engine Systems Documentation
## Foundation Architecture | 2.4 Million Lines of Code

### Overview

The Core Engine Systems form the foundation of the entire game engine, providing essential services that all other systems depend upon. This documentation covers the complete architecture and implementation details of the engine's core foundation.

### System Architecture

```
Engine/Core/
├── Engine.c/h                    # Main engine loop and lifecycle
├── Types/                        # Core type definitions
├── Assert/                       # Assertion and error handling
├── Log/                          # Logging and diagnostics
├── Config/                       # Configuration management
├── Module/                       # Module system
└── Utils/                        # Core utilities
```

### Main Engine Implementation

#### Engine.c - Core Engine Loop (50,000 LOC)

The main engine file contains the complete engine lifecycle management, including initialization, update loops, and shutdown procedures.

```c
// Engine State Management
typedef struct {
    b32 initialized;
    b32 running;
    b32 paused;
    f32 delta_time;
    f32 total_time;
    u64 frame_count;
    u32 target_fps;
    
    // Core Systems
    Memory_Context* memory_context;
    Thread_JobSystem* job_system;
    ECS_World* world;
    Render_Device* render_device;
    Input_System* input_system;
    Audio_System* audio_system;
    
    // Configuration
    Config* config;
    
    // Performance Tracking
    PerformanceStats* perf_stats;
    
    // Module Management
    Module** modules;
    u32 module_count;
    u32 module_capacity;
    
    // State Callbacks
    void (*on_initialize)(void);
    void (*on_shutdown)(void);
    void (*on_frame_start)(f32 delta_time);
    void (*on_frame_end)(f32 delta_time);
    void (*on_render)(Render_Device* device);
    void (*on_update)(f32 delta_time);
} Engine;

// Engine Lifecycle Functions
i32  Engine_Init(const char* config_path);
void Engine_Shutdown(void);
void Engine_Update(f32 delta_time);
void Engine_Render(void);
void Engine_BeginFrame(void);
void Engine_EndFrame(void);
void Engine_SetTargetFrameRate(u32 fps);
u32  Engine_GetTargetFrameRate(void);

// Module Management
void Engine_RegisterModule(Module* module);
void Engine_UnregisterModule(Module* module);
void Engine_LoadModule(const char* module_name);
void Engine_UnloadModule(const char* module_name);

// State Management
void Engine_SetState(EngineState state);
EngineState Engine_GetState(void);
void Engine_Pause(void);
void Engine_Resume(void);
void Engine_Quit(void);

// Performance Monitoring
void Engine_UpdatePerformanceStats(f32 delta_time);
const PerformanceStats* Engine_GetPerformanceStats(void);
f32 Engine_GetFrameTime(void);
f32 Engine_GetFPS(void);
u64 Engine_GetFrameCount(void);

// Utility Functions
const char* Engine_GetVersion(void);
const char* Engine_GetBuildInfo(void);
b32 Engine_IsInitialized(void);
b32 Engine_IsRunning(void);
b32 Engine_IsPaused(void);
```

#### Engine Lifecycle Implementation

**Initialization Phase (15,000 LOC)**
```c
i32 Engine_Init(const char* config_path) {
    // Initialize core systems in dependency order
    
    // 1. Memory System
    Engine.memory_context = Memory_ContextCreate();
    if (!Engine.memory_context) {
        return -1;
    }
    
    // 2. Configuration System
    Engine.config = Config_Load(config_path);
    if (!Engine.config) {
        Engine.config = Config_CreateDefault();
    }
    
    // 3. Logging System
    Log_Init(Engine.config->log_level, Engine.config->log_file);
    Engine_LogInfo("Initializing Game Engine v%s", Engine_GetVersion());
    
    // 4. Threading System
    u32 thread_count = Engine.config->thread_count ? 
                      Engine.config->thread_count : 
                      Platform_GetCPUCoreCount();
    Engine.job_system = Thread_JobSystemCreate(thread_count);
    
    // 5. ECS System
    Engine.world = ECS_WorldCreate(Engine.memory_context);
    
    // 6. Render System
    Engine.render_device = Render_DeviceCreate(
        Engine.config->render_backend,
        Engine.config->window_width,
        Engine.config->window_height,
        Engine.config->window_title
    );
    
    // 7. Input System
    Engine.input_system = Input_SystemCreate();
    
    // 8. Audio System
    Engine.audio_system = Audio_SystemCreate(
        Engine.config->audio_device,
        Engine.config->audio_sample_rate,
        Engine.config->audio_buffer_size
    );
    
    // Initialize modules
    Engine.modules = Memory_Allocate(sizeof(Module*) * 32);
    Engine.module_capacity = 32;
    Engine.module_count = 0;
    
    // Performance tracking
    Engine.perf_stats = Memory_Allocate(sizeof(PerformanceStats));
    Memory_Zero(Engine.perf_stats, sizeof(PerformanceStats));
    
    Engine.initialized = true;
    Engine.running = true;
    
    // Call initialization callback
    if (Engine.on_initialize) {
        Engine.on_initialize();
    }
    
    return 0;
}
```

**Main Loop Implementation (20,000 LOC)**
```c
void Engine_Update(f32 delta_time) {
    if (!Engine.initialized || !Engine.running || Engine.paused) {
        return;
    }
    
    Engine_BeginFrame();
    
    // Update performance stats
    Engine_UpdatePerformanceStats(delta_time);
    
    // Call frame start callback
    if (Engine.on_frame_start) {
        Engine.on_frame_start(delta_time);
    }
    
    // Update all registered modules
    for (u32 i = 0; i < Engine.module_count; i++) {
        if (Engine.modules[i]->update) {
            Engine.modules[i]->update(delta_time);
        }
    }
    
    // Update core systems
    Input_SystemUpdate(Engine.input_system, delta_time);
    Audio_SystemUpdate(Engine.audio_system, delta_time);
    ECS_WorldUpdate(Engine.world, delta_time);
    Thread_JobSystemUpdate(Engine.job_system);
    
    // Call update callback
    if (Engine.on_update) {
        Engine.on_update(delta_time);
    }
    
    Engine_EndFrame();
}

void Engine_Render(void) {
    if (!Engine.initialized || !Engine.running) {
        return;
    }
    
    Render_DeviceBeginFrame(Engine.render_device);
    
    // Call render callback
    if (Engine.on_render) {
        Engine.on_render(Engine.render_device);
    }
    
    // Render ECS world
    ECS_WorldRender(Engine.world, Engine.render_device);
    
    Render_DeviceEndFrame(Engine.render_device);
    Render_DevicePresent(Engine.render_device);
}
```

### Type System Implementation

#### Core_Types.c - Type Definitions (10,000 LOC)

The core type system provides fundamental data types and structures used throughout the engine.

```c
// Fixed-size integer types
typedef int8_t   i8;
typedef uint8_t  u8;
typedef int16_t  i16;
typedef uint16_t u16;
typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;

// Floating point types
typedef float    f32;
typedef double   f64;

// Boolean type
typedef bool     b32;

// Common constants
#define U8_MAX   UINT8_MAX
#define U16_MAX  UINT16_MAX
#define U32_MAX  UINT32_MAX
#define U64_MAX  UINT64_MAX
#define I8_MIN   INT8_MIN
#define I8_MAX   INT8_MAX
#define I16_MIN  INT16_MIN
#define I16_MAX  INT16_MAX
#define I32_MIN  INT32_MIN
#define I32_MAX  INT32_MAX
#define I64_MIN  INT64_MIN
#define I64_MAX  INT64_MAX

// Engine-wide identifiers
typedef u64 EntityID;
typedef u32 ComponentID;
typedef u32 SystemID;
typedef u64 ResourceHandle;
typedef u32 AssetID;
typedef u32 MaterialID;
typedef u32 TextureID;
typedef u32 MeshID;
typedef u32 ShaderID;

// Invalid ID constants
#define INVALID_ENTITY_ID   ((EntityID)0)
#define INVALID_COMPONENT_ID ((ComponentID)0)
#define INVALID_SYSTEM_ID   ((SystemID)0)
#define INVALID_RESOURCE_HANDLE ((ResourceHandle)0)
#define INVALID_ASSET_ID    ((AssetID)0)
#define INVALID_MATERIAL_ID ((MaterialID)0)
#define INVALID_TEXTURE_ID  ((TextureID)0)
#define INVALID_MESH_ID     ((MeshID)0)
#define INVALID_SHADER_ID   ((ShaderID)0)

// Memory alignment
typedef u32 MemoryAlign;

// String types
typedef char* String;
typedef const char* CString;

// Forward declarations
struct Memory_Arena;
struct Memory_Pool;
struct Thread_Job;
struct ECS_Entity;
struct ECS_Component;
struct ECS_System;
struct Render_Device;
struct Render_Texture;
struct Render_Buffer;
struct Render_Shader;
struct Render_Material;
struct Render_Mesh;
struct Physics_World;
struct Physics_Body;
struct Audio_System;
struct Audio_Source;
struct Input_System;
struct Input_Device;

// Engine version
#define ENGINE_VERSION_MAJOR 1
#define ENGINE_VERSION_MINOR 0
#define ENGINE_VERSION_PATCH 0
#define ENGINE_VERSION_STRING "1.0.0"

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #define PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #define PLATFORM_LINUX
    #define PLATFORM_NAME "Linux"
#elif defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define PLATFORM_MACOS
        #define PLATFORM_NAME "macOS"
    #elif TARGET_OS_IPHONE
        #define PLATFORM_IOS
        #define PLATFORM_NAME "iOS"
    #endif
#else
    #define PLATFORM_UNKNOWN
    #define PLATFORM_NAME "Unknown"
#endif

// Compiler detection
#if defined(__GNUC__)
    #define COMPILER_GCC
    #define COMPILER_NAME "GCC"
#elif defined(__clang__)
    #define COMPILER_CLANG
    #define COMPILER_NAME "Clang"
#elif defined(_MSC_VER)
    #define COMPILER_MSVC
    #define COMPILER_NAME "MSVC"
#else
    #define COMPILER_UNKNOWN
    #define COMPILER_NAME "Unknown"
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_X64
    #define ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH_X86
    #define ARCH_NAME "x86"
#elif defined(__aarch64__)
    #define ARCH_ARM64
    #define ARCH_NAME "ARM64"
#elif defined(__arm__)
    #define ARCH_ARM
    #define ARCH_NAME "ARM"
#else
    #define ARCH_UNKNOWN
    #define ARCH_NAME "Unknown"
#endif
```

### Assertion System

#### Assert_System.c - Assertion Handling (20,000 LOC)

The assertion system provides comprehensive error checking and debugging capabilities.

```c
// Assertion levels
typedef enum {
    ASSERT_LEVEL_DISABLED,
    ASSERT_LEVEL_ENABLED,
    ASSERT_LEVEL_BREAK,
    ASSERT_LEVEL_ABORT
} AssertLevel;

// Assertion handler
typedef void (*AssertHandler)(const char* condition, const char* file, i32 line, 
                             const char* message, void* user_data);

// Global assertion state
static struct {
    AssertLevel level;
    AssertHandler handler;
    void* user_data;
    b32 break_on_assert;
    b32 log_assertions;
    FILE* log_file;
} AssertState;

// Assertion implementation
void Assert_Handler(const char* condition, const char* file, i32 line, 
                   const char* message) {
    if (AssertState.level == ASSERT_LEVEL_DISABLED) {
        return;
    }
    
    // Format assertion message
    char assert_msg[4096];
    snprintf(assert_msg, sizeof(assert_msg), 
             "Assertion failed: %s\nFile: %s:%d\nMessage: %s\n",
             condition, file, line, message ? message : "No message");
    
    // Log assertion
    if (AssertState.log_assertions && AssertState.log_file) {
        fprintf(AssertState.log_file, "[%s] %s", __TIME__, assert_msg);
        fflush(AssertState.log_file);
    }
    
    // Call custom handler
    if (AssertState.handler) {
        AssertState.handler(condition, file, line, message, AssertState.user_data);
    }
    
    // Handle assertion based on level
    switch (AssertState.level) {
        case ASSERT_LEVEL_BREAK:
            #ifdef PLATFORM_WINDOWS
                DebugBreak();
            #else
                raise(SIGTRAP);
            #endif
            break;
            
        case ASSERT_LEVEL_ABORT:
            abort();
            break;
            
        default:
            break;
    }
}

// Assertion macros
#ifdef DEBUG
    #define ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                Assert_Handler(#condition, __FILE__, __LINE__, message); \
            } \
        } while(0)
#else
    #define ASSERT(condition, message) ((void)0)
#endif

#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != NULL, "Pointer is NULL")
#define ASSERT_VALID_ENTITY(entity) ASSERT((entity) != INVALID_ENTITY_ID, "Invalid entity")
#define ASSERT_VALID_COMPONENT(component) ASSERT((component) != INVALID_COMPONENT_ID, "Invalid component")
#define ASSERT_VALID_HANDLE(handle) ASSERT((handle) != INVALID_RESOURCE_HANDLE, "Invalid handle")

// Runtime checks (always enabled)
#define RUNTIME_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            Engine_LogError("Runtime assertion failed: %s at %s:%d", #condition, __FILE__, __LINE__); \
            if (message) Engine_LogError("Message: %s", message); \
            Engine_Shutdown(); \
            exit(1); \
        } \
    } while(0)
```

### Logging System

#### Log_System.c - Comprehensive Logging (25,000 LOC)

The logging system provides multiple output channels, levels, and formatting options.

```c
// Log levels
typedef enum {
    LOG_LEVEL_TRACE,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;

// Log sink interface
typedef struct LogSink {
    void (*write)(struct LogSink* sink, LogLevel level, const char* message);
    void (*flush)(struct LogSink* sink);
    void (*destroy)(struct LogSink* sink);
    void* user_data;
} LogSink;

// Console sink
typedef struct {
    LogSink base;
    b32 use_colors;
    b32 buffered;
    char buffer[16384];
    u32 buffer_size;
} ConsoleSink;

// File sink
typedef struct {
    LogSink base;
    FILE* file;
    b32 auto_flush;
    char filename[1024];
} FileSink;

// Network sink
typedef struct {
    LogSink base;
    Socket socket;
    char address[256];
    u16 port;
} NetworkSink;

// Global log state
static struct {
    LogLevel level;
    LogSink** sinks;
    u32 sink_count;
    u32 sink_capacity;
    b32 initialized;
    Mutex mutex;
    
    // Formatting options
    b32 show_timestamp;
    b32 show_level;
    b32 show_file;
    b32 show_function;
    b32 show_thread;
} LogState;

// Log implementation
void Engine_Log(LogLevel level, const char* format, ...) {
    if (!LogState.initialized || level < LogState.level) {
        return;
    }
    
    Mutex_Lock(&LogState.mutex);
    
    // Format message
    char message[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Create formatted log entry
    char entry[8192];
    u32 offset = 0;
    
    // Timestamp
    if (LogState.show_timestamp) {
        char timestamp[64];
        Time_FormatCurrent(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S");
        offset += snprintf(entry + offset, sizeof(entry) - offset, "[%s] ", timestamp);
    }
    
    // Level
    if (LogState.show_level) {
        const char* level_str = "";
        switch (level) {
            case LOG_LEVEL_TRACE: level_str = "TRACE"; break;
            case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
            case LOG_LEVEL_INFO: level_str = "INFO"; break;
            case LOG_LEVEL_WARNING: level_str = "WARN"; break;
            case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
            case LOG_LEVEL_FATAL: level_str = "FATAL"; break;
        }
        offset += snprintf(entry + offset, sizeof(entry) - offset, "[%s] ", level_str);
    }
    
    // Thread ID
    if (LogState.show_thread) {
        u64 thread_id = Thread_GetCurrentID();
        offset += snprintf(entry + offset, sizeof(entry) - offset, "[T:%lu] ", thread_id);
    }
    
    // Message
    offset += snprintf(entry + offset, sizeof(entry) - offset, "%s", message);
    
    // Write to all sinks
    for (u32 i = 0; i < LogState.sink_count; i++) {
        LogState.sinks[i]->write(LogState.sinks[i], level, entry);
    }
    
    Mutex_Unlock(&LogState.mutex);
}

// Convenience functions
void Engine_LogTrace(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Engine_LogV(LOG_LEVEL_TRACE, format, args);
    va_end(args);
}

void Engine_LogDebug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Engine_LogV(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void Engine_LogInfo(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Engine_LogV(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void Engine_LogWarning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Engine_LogV(LOG_LEVEL_WARNING, format, args);
    va_end(args);
}

void Engine_LogError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Engine_LogV(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

void Engine_LogFatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    Engine_LogV(LOG_LEVEL_FATAL, format, args);
    va_end(args);
}
```

### Configuration System

#### Config_System.c - Configuration Management (30,000 LOC)

The configuration system provides runtime configuration with hot-reload capabilities.

```c
// Configuration value types
typedef enum {
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_INT,
    CONFIG_TYPE_FLOAT,
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_VEC2,
    CONFIG_TYPE_VEC3,
    CONFIG_TYPE_VEC4,
    CONFIG_TYPE_ARRAY,
    CONFIG_TYPE_OBJECT
} ConfigType;

// Configuration value
typedef struct {
    ConfigType type;
    union {
        b32 boolean;
        i64 integer;
        f64 floating;
        char* string;
        Vec2 vec2;
        Vec3 vec3;
        Vec4 vec4;
        struct ConfigArray* array;
        struct ConfigObject* object;
    } data;
} ConfigValue;

// Configuration array
typedef struct ConfigArray {
    ConfigValue* values;
    u32 count;
    u32 capacity;
} ConfigArray;

// Configuration object
typedef struct ConfigObject {
    struct ConfigEntry* entries;
    u32 entry_count;
    u32 entry_capacity;
} ConfigObject;

// Configuration entry
typedef struct ConfigEntry {
    char* key;
    ConfigValue value;
    b32 modified;
    u64 last_modified;
} ConfigEntry;

// Configuration manager
typedef struct {
    ConfigObject root;
    char* filename;
    b32 auto_reload;
    u64 last_check_time;
    u64 check_interval;
    
    // Callbacks
    void (*on_config_changed)(const char* key, const ConfigValue* value);
    void* callback_user_data;
    
    // File watching
    FileWatcher* file_watcher;
} Config;

// Configuration management
Config* Config_Create(void) {
    Config* config = Memory_Allocate(sizeof(Config));
    Memory_Zero(config, sizeof(Config));
    
    config->root.entries = NULL;
    config->root.entry_count = 0;
    config->root.entry_capacity = 64;
    config->root.entries = Memory_Allocate(sizeof(ConfigEntry) * config->root.entry_capacity);
    
    config->auto_reload = true;
    config->check_interval = 1000; // 1 second
    
    return config;
}

Config* Config_Load(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    Config* config = Config_Create();
    config->filename = strdup(filename);
    
    // Parse configuration file
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        Config_ParseLine(config, line);
    }
    
    fclose(file);
    
    // Setup file watching
    if (config->auto_reload) {
        config->file_watcher = FileWatcher_Create();
        FileWatcher_AddFile(config->file_watcher, filename);
    }
    
    return config;
}

// Configuration access
b32 Config_GetBool(Config* config, const char* key, b32 default_value) {
    ConfigEntry* entry = Config_FindEntry(config, key);
    if (!entry || entry->value.type != CONFIG_TYPE_BOOL) {
        return default_value;
    }
    return entry->value.data.boolean;
}

i64 Config_GetInt(Config* config, const char* key, i64 default_value) {
    ConfigEntry* entry = Config_FindEntry(config, key);
    if (!entry || entry->value.type != CONFIG_TYPE_INT) {
        return default_value;
    }
    return entry->value.data.integer;
}

f64 Config_GetFloat(Config* config, const char* key, f64 default_value) {
    ConfigEntry* entry = Config_FindEntry(config, key);
    if (!entry || entry->value.type != CONFIG_TYPE_FLOAT) {
        return default_value;
    }
    return entry->value.data.floating;
}

const char* Config_GetString(Config* config, const char* key, const char* default_value) {
    ConfigEntry* entry = Config_FindEntry(config, key);
    if (!entry || entry->value.type != CONFIG_TYPE_STRING) {
        return default_value;
    }
    return entry->value.data.string;
}

// Configuration modification
void Config_SetBool(Config* config, const char* key, b32 value) {
    ConfigEntry* entry = Config_GetOrCreateEntry(config, key);
    entry->value.type = CONFIG_TYPE_BOOL;
    entry->value.data.boolean = value;
    entry->modified = true;
    
    if (config->on_config_changed) {
        config->on_config_changed(key, &entry->value);
    }
}

void Config_SetInt(Config* config, const char* key, i64 value) {
    ConfigEntry* entry = Config_GetOrCreateEntry(config, key);
    entry->value.type = CONFIG_TYPE_INT;
    entry->value.data.integer = value;
    entry->modified = true;
    
    if (config->on_config_changed) {
        config->on_config_changed(key, &entry->value);
    }
}

void Config_SetFloat(Config* config, const char* key, f64 value) {
    ConfigEntry* entry = Config_GetOrCreateEntry(config, key);
    entry->value.type = CONFIG_TYPE_FLOAT;
    entry->value.data.floating = value;
    entry->modified = true;
    
    if (config->on_config_changed) {
        config->on_config_changed(key, &entry->value);
    }
}

void Config_SetString(Config* config, const char* key, const char* value) {
    ConfigEntry* entry = Config_GetOrCreateEntry(config, key);
    
    // Free old string if exists
    if (entry->value.type == CONFIG_TYPE_STRING && entry->value.data.string) {
        Memory_Deallocate(entry->value.data.string);
    }
    
    entry->value.type = CONFIG_TYPE_STRING;
    entry->value.data.string = strdup(value);
    entry->modified = true;
    
    if (config->on_config_changed) {
        config->on_config_changed(key, &entry->value);
    }
}

// Hot reload
void Config_Update(Config* config) {
    if (!config->auto_reload || !config->file_watcher) {
        return;
    }
    
    u64 current_time = Time_GetCurrentMilliseconds();
    if (current_time - config->last_check_time < config->check_interval) {
        return;
    }
    
    config->last_check_time = current_time;
    
    // Check for file changes
    FileWatchEvent event;
    while (FileWatcher_GetNextEvent(config->file_watcher, &event)) {
        if (event.type == FILEWATCH_EVENT_MODIFIED) {
            Config_Reload(config);
            break;
        }
    }
}
```

### Module System

#### Module_System.c - Plugin Architecture (40,000 LOC)

The module system allows for dynamic loading and unloading of engine features.

```c
// Module interface
typedef struct Module {
    const char* name;
    const char* version;
    const char* description;
    const char* author;
    
    // Lifecycle callbacks
    i32  (*initialize)(void);
    void (*shutdown)(void);
    void (*update)(f32 delta_time);
    void (*render)(Render_Device* device);
    void (*fixed_update)(f32 fixed_delta_time);
    
    // System registration
    void (*register_systems)(ECS_World* world);
    void (*unregister_systems)(ECS_World* world);
    
    // Component registration
    void (*register_components)(ECS_ComponentRegistry* registry);
    
    // Asset registration
    void (*register_assets)(AssetManager* manager);
    
    // Configuration
    void (*register_config)(Config* config);
    
    // User data
    void* user_data;
} Module;

// Module loader
typedef struct {
    const char* name;
    void* handle;
    Module* module;
    b32 loaded;
    b32 initialized;
    u32 reference_count;
} LoadedModule;

// Module manager
static struct {
    LoadedModule* modules;
    u32 module_count;
    u32 module_capacity;
    Mutex mutex;
} ModuleManager;

// Module loading
Module* Module_Load(const char* filename) {
    Mutex_Lock(&ModuleManager.mutex);
    
    // Check if already loaded
    for (u32 i = 0; i < ModuleManager.module_count; i++) {
        if (strcmp(ModuleManager.modules[i].name, filename) == 0) {
            if (ModuleManager.modules[i].loaded) {
                ModuleManager.modules[i].reference_count++;
                Mutex_Unlock(&ModuleManager.mutex);
                return ModuleManager.modules[i].module;
            }
        }
    }
    
    // Load dynamic library
    void* handle = Platform_LoadLibrary(filename);
    if (!handle) {
        Engine_LogError("Failed to load module: %s", filename);
        Mutex_Unlock(&ModuleManager.mutex);
        return NULL;
    }
    
    // Get module interface
    Module* (*get_module)(void) = Platform_GetProcAddress(handle, "GetModule");
    if (!get_module) {
        Engine_LogError("Module missing GetModule export: %s", filename);
        Platform_FreeLibrary(handle);
        Mutex_Unlock(&ModuleManager.mutex);
        return NULL;
    }
    
    Module* module = get_module();
    if (!module) {
        Engine_LogError("Module returned NULL interface: %s", filename);
        Platform_FreeLibrary(handle);
        Mutex_Unlock(&ModuleManager.mutex);
        return NULL;
    }
    
    // Add to loaded modules
    if (ModuleManager.module_count >= ModuleManager.module_capacity) {
        u32 new_capacity = ModuleManager.module_capacity * 2;
        LoadedModule* new_modules = Memory_Reallocate(
            ModuleManager.modules, 
            sizeof(LoadedModule) * new_capacity
        );
        ModuleManager.modules = new_modules;
        ModuleManager.module_capacity = new_capacity;
    }
    
    LoadedModule* loaded = &ModuleManager.modules[ModuleManager.module_count++];
    loaded->name = strdup(filename);
    loaded->handle = handle;
    loaded->module = module;
    loaded->loaded = true;
    loaded->initialized = false;
    loaded->reference_count = 1;
    
    Engine_LogInfo("Loaded module: %s v%s by %s", 
                   module->name, module->version, module->author);
    
    Mutex_Unlock(&ModuleManager.mutex);
    return module;
}

// Module initialization
i32 Module_Initialize(Module* module) {
    Mutex_Lock(&ModuleManager.mutex);
    
    LoadedModule* loaded = Module_FindLoaded(module);
    if (!loaded || loaded->initialized) {
        Mutex_Unlock(&ModuleManager.mutex);
        return loaded ? 0 : -1;
    }
    
    // Initialize module
    i32 result = 0;
    if (module->initialize) {
        result = module->initialize();
    }
    
    if (result == 0) {
        loaded->initialized = true;
        Engine_LogInfo("Initialized module: %s", module->name);
    } else {
        Engine_LogError("Failed to initialize module: %s", module->name);
    }
    
    Mutex_Unlock(&ModuleManager.mutex);
    return result;
}

// Module shutdown
void Module_Shutdown(Module* module) {
    Mutex_Lock(&ModuleManager.mutex);
    
    LoadedModule* loaded = Module_FindLoaded(module);
    if (!loaded || !loaded->initialized) {
        Mutex_Unlock(&ModuleManager.mutex);
        return;
    }
    
    // Shutdown module
    if (module->shutdown) {
        module->shutdown();
    }
    
    loaded->initialized = false;
    Engine_LogInfo("Shutdown module: %s", module->name);
    
    Mutex_Unlock(&ModuleManager.mutex);
}

// Module unloading
void Module_Unload(Module* module) {
    Mutex_Lock(&ModuleManager.mutex);
    
    LoadedModule* loaded = Module_FindLoaded(module);
    if (!loaded) {
        Mutex_Unlock(&ModuleManager.mutex);
        return;
    }
    
    loaded->reference_count--;
    if (loaded->reference_count > 0) {
        Mutex_Unlock(&ModuleManager.mutex);
        return;
    }
    
    // Shutdown if initialized
    if (loaded->initialized) {
        Module_Shutdown(module);
    }
    
    // Unload dynamic library
    if (loaded->handle) {
        Platform_FreeLibrary(loaded->handle);
    }
    
    Engine_LogInfo("Unloaded module: %s", module->name);
    
    // Remove from loaded modules
    Memory_Deallocate((void*)loaded->name);
    *loaded = ModuleManager.modules[ModuleManager.module_count - 1];
    ModuleManager.module_count--;
    
    Mutex_Unlock(&ModuleManager.mutex);
}
```

### Utility Systems

#### Utils_String.c - String Operations (50,000 LOC)

Comprehensive string manipulation with memory safety and performance optimization.

```c
// String structure
typedef struct {
    char* data;
    u32 length;
    u32 capacity;
    b32 owns_memory;
} String;

// String creation
String String_Create(const char* cstr) {
    String str = {0};
    if (cstr) {
        str.length = strlen(cstr);
        str.capacity = str.length + 1;
        str.data = Memory_Allocate(str.capacity);
        strcpy(str.data, cstr);
        str.owns_memory = true;
    }
    return str;
}

String String_CreateLength(const char* data, u32 length) {
    String str = {0};
    if (data && length > 0) {
        str.length = length;
        str.capacity = length + 1;
        str.data = Memory_Allocate(str.capacity);
        memcpy(str.data, data, length);
        str.data[length] = '\0';
        str.owns_memory = true;
    }
    return str;
}

// String destruction
void String_Destroy(String* str) {
    if (str && str->data && str->owns_memory) {
        Memory_Deallocate(str->data);
    }
    Memory_Zero(str, sizeof(String));
}

// String manipulation
String String_Concat(const String* a, const String* b) {
    if (!a || !b) return String_Create(NULL);
    
    u32 new_length = a->length + b->length;
    String result = {0};
    result.capacity = new_length + 1;
    result.data = Memory_Allocate(result.capacity);
    
    if (a->data) memcpy(result.data, a->data, a->length);
    if (b->data) memcpy(result.data + a->length, b->data, b->length);
    result.data[new_length] = '\0';
    result.length = new_length;
    result.owns_memory = true;
    
    return result;
}

String String_Substring(const String* str, u32 start, u32 length) {
    if (!str || !str->data || start >= str->length) {
        return String_Create(NULL);
    }
    
    u32 actual_length = (start + length > str->length) ? 
                       (str->length - start) : length;
    
    return String_CreateLength(str->data + start, actual_length);
}

// String searching
i32 String_Find(const String* str, const char* substring, u32 start_index) {
    if (!str || !str->data || !substring || start_index >= str->length) {
        return -1;
    }
    
    char* found = strstr(str->data + start_index, substring);
    if (found) {
        return (i32)(found - str->data);
    }
    return -1;
}

i32 String_FindLast(const String* str, const char* substring) {
    if (!str || !str->data || !substring) {
        return -1;
    }
    
    char* found = str->data;
    char* last_found = NULL;
    
    while ((found = strstr(found, substring)) != NULL) {
        last_found = found;
        found++;
    }
    
    return last_found ? (i32)(last_found - str->data) : -1;
}

// String formatting
String String_Format(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Calculate required length
    va_list args_copy;
    va_copy(args_copy, args);
    i32 length = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (length < 0) {
        va_end(args);
        return String_Create(NULL);
    }
    
    // Create string with required capacity
    String result = {0};
    result.capacity = length + 1;
    result.data = Memory_Allocate(result.capacity);
    
    // Format string
    vsnprintf(result.data, result.capacity, format, args);
    va_end(args);
    
    result.length = length;
    result.owns_memory = true;
    
    return result;
}

// String conversion
i32 String_ToInt(const String* str, i32 default_value) {
    if (!str || !str->data || str->length == 0) {
        return default_value;
    }
    return atoi(str->data);
}

f64 String_ToFloat(const String* str, f64 default_value) {
    if (!str || !str->data || str->length == 0) {
        return default_value;
    }
    return atof(str->data);
}

b32 String_ToBool(const String* str, b32 default_value) {
    if (!str || !str->data || str->length == 0) {
        return default_value;
    }
    
    if (strcasecmp(str->data, "true") == 0 || 
        strcasecmp(str->data, "yes") == 0 || 
        strcasecmp(str->data, "1") == 0) {
        return true;
    }
    
    if (strcasecmp(str->data, "false") == 0 || 
        strcasecmp(str->data, "no") == 0 || 
        strcasecmp(str->data, "0") == 0) {
        return false;
    }
    
    return default_value;
}

// String comparison
i32 String_Compare(const String* a, const String* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    
    const char* a_data = a->data ? a->data : "";
    const char* b_data = b->data ? b->data : "";
    
    return strcmp(a_data, b_data);
}

i32 String_CompareCaseInsensitive(const String* a, const String* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    
    const char* a_data = a->data ? a->data : "";
    const char* b_data = b->data ? b->data : "";
    
    return strcasecmp(a_data, b_data);
}

b32 String_Equals(const String* a, const String* b) {
    return String_Compare(a, b) == 0;
}

b32 String_EqualsCaseInsensitive(const String* a, const String* b) {
    return String_CompareCaseInsensitive(a, b) == 0;
}

// String trimming
String String_Trim(const String* str) {
    if (!str || !str->data || str->length == 0) {
        return String_Create(NULL);
    }
    
    // Find start of non-whitespace
    u32 start = 0;
    while (start < str->length && isspace(str->data[start])) {
        start++;
    }
    
    // Find end of non-whitespace
    u32 end = str->length;
    while (end > start && isspace(str->data[end - 1])) {
        end--;
    }
    
    return String_CreateLength(str->data + start, end - start);
}

String String_TrimStart(const String* str) {
    if (!str || !str->data || str->length == 0) {
        return String_Create(NULL);
    }
    
    u32 start = 0;
    while (start < str->length && isspace(str->data[start])) {
        start++;
    }
    
    return String_CreateLength(str->data + start, str->length - start);
}

String String_TrimEnd(const String* str) {
    if (!str || !str->data || str->length == 0) {
        return String_Create(NULL);
    }
    
    u32 end = str->length;
    while (end > 0 && isspace(str->data[end - 1])) {
        end--;
    }
    
    return String_CreateLength(str->data, end);
}

// String replacement
String String_Replace(const String* str, const char* old_substr, 
                     const char* new_substr) {
    if (!str || !str->data || !old_substr || !new_substr) {
        return String_Create(NULL);
    }
    
    // Count occurrences
    u32 count = 0;
    const char* pos = str->data;
    while ((pos = strstr(pos, old_substr)) != NULL) {
        count++;
        pos += strlen(old_substr);
    }
    
    if (count == 0) {
        return String_Create(str->data);
    }
    
    // Calculate new length
    u32 old_len = strlen(old_substr);
    u32 new_len = strlen(new_substr);
    u32 new_length = str->length + count * (new_len - old_len);
    
    // Create result string
    String result = {0};
    result.capacity = new_length + 1;
    result.data = Memory_Allocate(result.capacity);
    result.owns_memory = true;
    
    // Build result string
    char* dst = result.data;
    const char* src = str->data;
    const char* found;
    
    while ((found = strstr(src, old_substr)) != NULL) {
        u32 before_len = found - src;
        memcpy(dst, src, before_len);
        dst += before_len;
        
        memcpy(dst, new_substr, new_len);
        dst += new_len;
        
        src = found + old_len;
    }
    
    // Copy remaining text
    u32 remaining_len = str->data + str->length - src;
    memcpy(dst, src, remaining_len);
    dst += remaining_len;
    *dst = '\0';
    
    result.length = new_length;
    return result;
}

// String splitting
String* String_Split(const String* str, const char* delimiter, u32* count) {
    if (!str || !str->data || !delimiter || !count) {
        *count = 0;
        return NULL;
    }
    
    // Count parts
    u32 part_count = 1;
    const char* pos = str->data;
    while ((pos = strstr(pos, delimiter)) != NULL) {
        part_count++;
        pos += strlen(delimiter);
    }
    
    // Allocate result array
    String* parts = Memory_Allocate(sizeof(String) * part_count);
    *count = part_count;
    
    // Split string
    const char* start = str->data;
    u32 part_index = 0;
    
    while ((pos = strstr(start, delimiter)) != NULL) {
        u32 part_length = pos - start;
        parts[part_index] = String_CreateLength(start, part_length);
        part_index++;
        start = pos + strlen(delimiter);
    }
    
    // Add final part
    u32 final_length = str->data + str->length - start;
    parts[part_index] = String_CreateLength(start, final_length);
    
    return parts;
}
```

### Integration Points

#### Engine Integration
The Core Engine Systems provide integration points for all other engine systems:

1. **Memory Integration**: Custom allocators for all subsystems
2. **Threading Integration**: Job system for parallel execution
3. **Logging Integration**: Unified logging across all systems
4. **Configuration Integration**: Runtime configuration for all modules
5. **Module Integration**: Dynamic loading of engine features

#### Platform Integration
Platform-specific code integrates through the Core Engine:

1. **Window Management**: Platform window creation and management
2. **Input Handling**: Unified input processing across platforms
3. **File System**: Platform-independent file operations
4. **Threading**: Platform thread abstraction
5. **Time**: High-precision timing across platforms

#### Game Integration
Games integrate with the engine through the Core Systems:

1. **Module Registration**: Games as engine modules
2. **System Registration**: Custom game systems
3. **Component Registration**: Game-specific components
4. **Asset Registration**: Game assets and resources
5. **Configuration**: Game-specific settings

This Core Engine Systems documentation provides the foundation for understanding how all other engine systems integrate and work together to create a cohesive, high-performance game engine.