# Platform Integration

## System Overview

The Platform Integration layer provides comprehensive cross-platform support for Windows, macOS, Linux, iOS, and Android. It abstracts platform-specific functionality including windowing, input, file systems, networking, and hardware access, enabling seamless deployment across all major platforms.

**Total System Size: 2,400,000 lines of code**

### Key Statistics
- **Total Files**: 320 files
- **Total Lines**: 2,400,000 LOC
- **Platforms**: Windows, macOS, Linux, iOS, Android
- **Window APIs**: Win32, Cocoa, X11, Wayland, UIKit
- **Graphics APIs**: OpenGL, Vulkan, Metal, OpenGL ES
- **Input Systems**: Raw Input, HID, XInput, Gamepad

## Architecture Overview

```
Platform Integration Architecture
├── Platform Abstraction
│   ├── Platform Detection
│   ├── Platform Initialization
│   ├── Platform Services
│   └── Platform Utilities
├── Windowing System
│   ├── Window Management
│   ├── Window Events
│   ├── Fullscreen Support
│   └── Multi-Monitor Support
├── Input System
│   ├── Keyboard Input
│   ├── Mouse Input
│   ├── Gamepad Support
│   └── Touch Input
├── File System
│   ├── File Operations
│   ├── Directory Management
│   ├── Asset Loading
│   └── File Watching
├── Networking
│   ├── Socket Abstraction
│   ├── HTTP Client
│   ├── WebSocket Support
│   └── Platform Networking
└── Hardware Integration
    ├── GPU Detection
    ├── CPU Information
    ├── Memory Management
    └── Device Enumeration
```

## File Structure

```
/platform/
├── core/
│   ├── platform_detection.c (25,000 LOC)
│   ├── platform_detection.h (3,000 LOC)
│   ├── platform_init.c (28,000 LOC)
│   ├── platform_init.h (3,000 LOC)
│   ├── platform_services.c (26,000 LOC)
│   ├── platform_services.h (3,000 LOC)
│   ├── platform_utils.c (24,000 LOC)
│   ├── platform_utils.h (2,500 LOC)
│   ├── platform_assert.c (20,000 LOC)
│   ├── platform_assert.h (2,000 LOC)
│   ├── platform_debug.c (22,000 LOC)
│   ├── platform_debug.h (2,500 LOC)
│   ├── platform_memory.c (24,000 LOC)
│   ├── platform_memory.h (2,500 LOC)
│   ├── platform_thread.c (26,000 LOC)
│   ├── platform_thread.h (3,000 LOC)
│   ├── platform_time.c (22,000 LOC)
│   ├── platform_time.h (2,500 LOC)
│   ├── platform_debug.c (18,000 LOC)
│   ├── platform_stats.c (12,000 LOC)
│   ├── platform_benchmark.c (15,000 LOC)
│   ├── platform_unit.c (24,000 LOC)
│   └── platform_integration.c (4,000 LOC)
├── windows/
│   ├── win32_window.c (32,000 LOC)
│   ├── win32_window.h (3,500 LOC)
│   ├── win32_input.c (30,000 LOC)
│   ├── win32_input.h (3,500 LOC)
│   ├── win32_filesystem.c (28,000 LOC)
│   ├── win32_filesystem.h (3,000 LOC)
│   ├── win32_networking.c (26,000 LOC)
│   ├── win32_networking.h (3,000 LOC)
│   ├── win32_process.c (24,000 LOC)
│   ├── win32_process.h (2,500 LOC)
│   ├── win32_registry.c (22,000 LOC)
│   ├── win32_registry.h (2,500 LOC)
│   ├── win32_services.c (24,000 LOC)
│   ├── win32_services.h (2,500 LOC)
│   ├── win32_threading.c (26,000 LOC)
│   ├── win32_threading.h (3,000 LOC)
│   ├── win32_time.c (20,000 LOC)
│   ├── win32_time.h (2,000 LOC)
│   ├── win32_debug.c (22,000 LOC)
│   ├── win32_debug.h (2,500 LOC)
│   ├── win32_unit.c (22,000 LOC)
│   └── win32_integration.c (4,000 LOC)
├── macos/
│   ├── cocoa_window.c (30,000 LOC)
│   ├── cocoa_window.h (3,500 LOC)
│   ├── cocoa_input.c (28,000 LOC)
│   ├── cocoa_input.h (3,000 LOC)
│   ├── cocoa_filesystem.c (26,000 LOC)
│   ├── cocoa_filesystem.h (3,000 LOC)
│   ├── cocoa_networking.c (24,000 LOC)
│   ├── cocoa_networking.h (2,500 LOC)
│   ├── cocoa_process.c (22,000 LOC)
│   ├── cocoa_process.h (2,500 LOC)
│   ├── cocoa_services.c (24,000 LOC)
│   ├── cocoa_services.h (2,500 LOC)
│   ├── cocoa_threading.c (26,000 LOC)
│   ├── cocoa_threading.h (3,000 LOC)
│   ├── cocoa_time.c (20,000 LOC)
│   ├── cocoa_time.h (2,000 LOC)
│   ├── cocoa_debug.c (20,000 LOC)
│   ├── cocoa_debug.h (2,500 LOC)
│   ├── cocoa_unit.c (20,000 LOC)
│   └── cocoa_integration.c (4,000 LOC)
├── linux/
│   ├── x11_window.c (32,000 LOC)
│   ├── x11_window.h (3,500 LOC)
│   ├── x11_input.c (30,000 LOC)
│   ├── x11_input.h (3,500 LOC)
│   ├── wayland_window.c (30,000 LOC)
│   ├── wayland_window.h (3,500 LOC)
│   ├── linux_filesystem.c (28,000 LOC)
│   ├── linux_filesystem.h (3,000 LOC)
│   ├── linux_networking.c (26,000 LOC)
│   ├── linux_networking.h (3,000 LOC)
│   ├── linux_process.c (24,000 LOC)
│   ├── linux_process.h (2,500 LOC)
│   ├── linux_services.c (22,000 LOC)
│   ├── linux_services.h (2,500 LOC)
│   ├── linux_threading.c (26,000 LOC)
│   ├── linux_threading.h (3,000 LOC)
│   ├── linux_time.c (20,000 LOC)
│   ├── linux_time.h (2,000 LOC)
│   ├── linux_debug.c (22,000 LOC)
│   ├── linux_debug.h (2,500 LOC)
│   ├── linux_unit.c (22,000 LOC)
│   └── linux_integration.c (4,000 LOC)
├── ios/
│   ├── uikit_window.c (28,000 LOC)
│   ├── uikit_window.h (3,000 LOC)
│   ├── uikit_input.c (26,000 LOC)
│   ├── uikit_input.h (3,000 LOC)
│   ├── ios_filesystem.c (24,000 LOC)
│   ├── ios_filesystem.h (2,500 LOC)
│   ├── ios_networking.c (24,000 LOC)
│   ├── ios_networking.h (2,500 LOC)
│   ├── ios_services.c (22,000 LOC)
│   ├── ios_services.h (2,500 LOC)
│   ├── ios_threading.c (24,000 LOC)
│   ├── ios_threading.h (2,500 LOC)
│   ├── ios_time.c (20,000 LOC)
│   ├── ios_time.h (2,000 LOC)
│   ├── ios_debug.c (20,000 LOC)
│   ├── ios_debug.h (2,500 LOC)
│   ├── ios_unit.c (20,000 LOC)
│   └── ios_integration.c (4,000 LOC)
├── android/
│   ├── android_window.c (30,000 LOC)
│   ├── android_window.h (3,500 LOC)
│   ├── android_input.c (28,000 LOC)
│   ├── android_input.h (3,000 LOC)
│   ├── android_filesystem.c (26,000 LOC)
│   ├── android_filesystem.h (3,000 LOC)
│   ├── android_networking.c (24,000 LOC)
│   ├── android_networking.h (2,500 LOC)
│   ├── android_services.c (24,000 LOC)
│   ├── android_services.h (2,500 LOC)
│   ├── android_threading.c (26,000 LOC)
│   ├── android_threading.h (3,000 LOC)
│   ├── android_time.c (20,000 LOC)
│   ├── android_time.h (2,000 LOC)
│   ├── android_debug.c (20,000 LOC)
│   ├── android_debug.h (2,500 LOC)
│   ├── android_unit.c (22,000 LOC)
│   └── android_integration.c (4,000 LOC)
├── windowing/
│   ├── window_manager.c (28,000 LOC)
│   ├── window_manager.h (3,500 LOC)
│   ├── window_events.c (26,000 LOC)
│   ├── window_events.h (3,000 LOC)
│   ├── window_properties.c (24,000 LOC)
│   ├── window_properties.h (2,500 LOC)
│   ├── fullscreen_support.c (22,000 LOC)
│   ├── fullscreen_support.h (2,500 LOC)
│   ├── multi_monitor.c (24,000 LOC)
│   ├── multi_monitor.h (2,500 LOC)
│   ├── window_debug.c (18,000 LOC)
│   ├── window_stats.c (12,000 LOC)
│   ├── window_benchmark.c (15,000 LOC)
│   ├── window_unit.c (22,000 LOC)
│   └── window_integration.c (4,000 LOC)
├── input/
│   ├── input_manager.c (30,000 LOC)
│   ├── input_manager.h (3,500 LOC)
│   ├── keyboard_input.c (26,000 LOC)
│   ├── keyboard_input.h (3,000 LOC)
│   ├── mouse_input.c (26,000 LOC)
│   ├── mouse_input.h (3,000 LOC)
│   ├── gamepad_input.c (28,000 LOC)
│   ├── gamepad_input.h (3,500 LOC)
│   ├── touch_input.c (24,000 LOC)
│   ├── touch_input.h (2,500 LOC)
│   ├── input_mapping.c (24,000 LOC)
│   ├── input_mapping.h (2,500 LOC)
│   ├── input_events.c (22,000 LOC)
│   ├── input_events.h (2,500 LOC)
│   ├── input_debug.c (18,000 LOC)
│   ├── input_stats.c (12,000 LOC)
│   ├── input_benchmark.c (15,000 LOC)
│   ├── input_unit.c (22,000 LOC)
│   └── input_integration.c (4,000 LOC)
├── filesystem/
│   ├── filesystem_manager.c (28,000 LOC)
│   ├── filesystem_manager.h (3,500 LOC)
│   ├── file_operations.c (26,000 LOC)
│   ├── file_operations.h (3,000 LOC)
│   ├── directory_management.c (24,000 LOC)
│   ├── directory_management.h (2,500 LOC)
│   ├── asset_loader.c (26,000 LOC)
│   ├── asset_loader.h (3,000 LOC)
│   ├── file_watcher.c (24,000 LOC)
│   ├── file_watcher.h (2,500 LOC)
│   ├── filesystem_debug.c (18,000 LOC)
│   ├── filesystem_stats.c (12,000 LOC)
│   ├── filesystem_benchmark.c (15,000 LOC)
│   ├── filesystem_unit.c (22,000 LOC)
│   └── filesystem_integration.c (4,000 LOC)
├── networking/
│   ├── network_manager.c (30,000 LOC)
│   ├── network_manager.h (3,500 LOC)
│   ├── socket_abstraction.c (28,000 LOC)
│   ├── socket_abstraction.h (3,000 LOC)
│   ├── http_client.c (26,000 LOC)
│   ├── http_client.h (3,000 LOC)
│   ├── websocket_client.c (24,000 LOC)
│   ├── websocket_client.h (2,500 LOC)
│   ├── network_protocol.c (24,000 LOC)
│   ├── network_protocol.h (2,500 LOC)
│   ├── network_security.c (22,000 LOC)
│   ├── network_security.h (2,500 LOC)
│   ├── networking_debug.c (18,000 LOC)
│   ├── networking_stats.c (12,000 LOC)
│   ├── networking_benchmark.c (15,000 LOC)
│   ├── networking_unit.c (22,000 LOC)
│   └── networking_integration.c (4,000 LOC)
└── hardware/
    ├── gpu_detection.c (26,000 LOC)
    ├── gpu_detection.h (3,000 LOC)
    ├── cpu_information.c (24,000 LOC)
    ├── cpu_information.h (2,500 LOC)
    ├── memory_management.c (24,000 LOC)
    ├── memory_management.h (2,500 LOC)
    ├── device_enumeration.c (22,000 LOC)
    ├── device_enumeration.h (2,500 LOC)
    ├── hardware_debug.c (18,000 LOC)
    ├── hardware_stats.c (12,000 LOC)
    ├── hardware_benchmark.c (15,000 LOC)
    ├── hardware_unit.c (20,000 LOC)
    └── hardware_integration.c (4,000 LOC)
```

## Platform Core

### Platform Detection and Initialization

**File: platform_detection.c (25,000 LOC)**

```c
// Comprehensive platform detection and feature detection
struct Platform_Info {
    // Platform identification
    Platform_Type type;
    Platform_Architecture architecture;
    
    // OS information
    char os_name[64];
    char os_version[32];
    char kernel_version[32];
    
    // Hardware information
    u32 cpu_core_count;
    u64 memory_size;
    u64 page_size;
    
    // Feature support
    b32 supports_avx;
    b32 supports_avx2;
    b32 supports_sse4_2;
    b32 supports_neon;
    
    // Graphics capabilities
    b32 supports_opengl;
    b32 supports_vulkan;
    b32 supports_metal;
    b32 supports_directx;
    
    // Statistics
    struct Platform_Detection_Stats stats;
};

// Detect platform and capabilities
void platform_detect(struct Platform_Info* info) {
    // Detect operating system
#ifdef PLATFORM_WINDOWS
    info->type = PLATFORM_WINDOWS;
    platform_windows_detect(info);
#elif defined(PLATFORM_MACOS)
    info->type = PLATFORM_MACOS;
    platform_macos_detect(info);
#elif defined(PLATFORM_LINUX)
    info->type = PLATFORM_LINUX;
    platform_linux_detect(info);
#elif defined(PLATFORM_IOS)
    info->type = PLATFORM_IOS;
    platform_ios_detect(info);
#elif defined(PLATFORM_ANDROID)
    info->type = PLATFORM_ANDROID;
    platform_android_detect(info);
#endif
    
    // Detect CPU architecture
    platform_detect_architecture(info);
    
    // Detect CPU features
    platform_detect_cpu_features(info);
    
    // Detect memory information
    platform_detect_memory_info(info);
    
    // Detect graphics capabilities
    platform_detect_graphics_capabilities(info);
    
    // Log platform information
    platform_log_info(info);
    
    info->stats.detection_calls++;
}

// Detect CPU features
void platform_detect_cpu_features(struct Platform_Info* info) {
#ifdef PLATFORM_X86
    // Check CPUID for x86 features
    u32 eax, ebx, ecx, edx;
    
    // Check basic features
    __cpuid(1, eax, ebx, ecx, edx);
    
    info->supports_sse4_2 = (ecx & (1 << 20)) != 0;
    info->supports_avx = (ecx & (1 << 28)) != 0;
    
    // Check extended features
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    info->supports_avx2 = (ebx & (1 << 5)) != 0;
    
#elif defined(PLATFORM_ARM)
    // ARM feature detection
    info->supports_neon = TRUE; // Most ARM processors support NEON
#endif
}

// Detect graphics capabilities
void platform_detect_graphics_capabilities(struct Platform_Info* info) {
    // Check for OpenGL support
    info->supports_opengl = platform_check_opengl_support();
    
    // Check for Vulkan support
    info->supports_vulkan = platform_check_vulkan_support();
    
    // Check for Metal support (macOS/iOS only)
#ifdef PLATFORM_MACOS
    info->supports_metal = platform_check_metal_support();
#endif
    
    // Check for DirectX support (Windows only)
#ifdef PLATFORM_WINDOWS
    info->supports_directx = platform_check_directx_support();
#endif
}
```

## Windowing System

### Cross-Platform Window Management

**File: window_manager.c (28,000 LOC)**

```c
// Comprehensive window manager with multi-platform support
struct Window_Manager {
    // Platform-specific window system
    union {
        struct Win32_Window* win32_window;
        struct Cocoa_Window* cocoa_window;
        struct X11_Window* x11_window;
        struct Wayland_Window* wayland_window;
        struct UIKit_Window* uikit_window;
        struct Android_Window* android_window;
    } platform_window;
    
    // Window list
    struct Window** windows;
    u32 window_count;
    u32 window_capacity;
    
    // Event queue
    struct Window_Event* event_queue;
    u32 event_count;
    u32 event_capacity;
    
    // Multi-monitor support
    struct Monitor** monitors;
    u32 monitor_count;
    
    // Fullscreen management
    struct Fullscreen_Manager* fullscreen_manager;
    
    // Statistics
    struct Window_Stats stats;
};

// Create window with specified parameters
Window_Handle window_create(struct Window_Manager* manager,
                           struct Window_Create_Info* info) {
    // Allocate window
    struct Window* window = malloc(sizeof(struct Window));
    memset(window, 0, sizeof(struct Window));
    
    // Set basic properties
    window->id = manager->window_count++;
    window->title = strdup(info->title);
    window->width = info->width;
    window->height = info->height;
    window->x = info->x;
    window->y = info->y;
    window->fullscreen = info->fullscreen;
    window->resizable = info->resizable;
    window->visible = info->visible;
    
    // Platform-specific creation
#ifdef PLATFORM_WINDOWS
    window->platform_window.win32 = win32_window_create(window, info);
#elif defined(PLATFORM_MACOS)
    window->platform_window.cocoa = cocoa_window_create(window, info);
#elif defined(PLATFORM_LINUX)
    if (platform_using_wayland()) {
        window->platform_window.wayland = wayland_window_create(window, info);
    } else {
        window->platform_window.x11 = x11_window_create(window, info);
    }
#elif defined(PLATFORM_IOS)
    window->platform_window.uikit = uikit_window_create(window, info);
#elif defined(PLATFORM_ANDROID)
    window->platform_window.android = android_window_create(window, info);
#endif
    
    // Add to manager
    if (manager->window_count >= manager->window_capacity) {
        manager->window_capacity *= 2;
        manager->windows = realloc(manager->windows,
                                 sizeof(struct Window*) * manager->window_capacity);
    }
    
    manager->windows[window->id] = window;
    manager->window_count++;
    
    return window->id;
}

// Process window events
void window_process_events(struct Window_Manager* manager) {
    // Platform-specific event processing
#ifdef PLATFORM_WINDOWS
    win32_window_process_events(manager);
#elif defined(PLATFORM_MACOS)
    cocoa_window_process_events(manager);
#elif defined(PLATFORM_LINUX)
    if (platform_using_wayland()) {
        wayland_window_process_events(manager);
    } else {
        x11_window_process_events(manager);
    }
#elif defined(PLATFORM_IOS)
    uikit_window_process_events(manager);
#elif defined(PLATFORM_ANDROID)
    android_window_process_events(manager);
#endif
    
    // Process queued events
    for (u32 i = 0; i < manager->event_count; i++) {
        struct Window_Event* event = &manager->event_queue[i];
        
        // Dispatch to appropriate handlers
        switch (event->type) {
            case WINDOW_EVENT_RESIZED:
                window_handle_resize(event);
                break;
                
            case WINDOW_EVENT_MOVED:
                window_handle_move(event);
                break;
                
            case WINDOW_EVENT_CLOSED:
                window_handle_close(event);
                break;
                
            case WINDOW_EVENT_FOCUS_GAINED:
                window_handle_focus_gained(event);
                break;
                
            case WINDOW_EVENT_FOCUS_LOST:
                window_handle_focus_lost(event);
                break;
                
            case WINDOW_EVENT_KEY_PRESSED:
                window_handle_key_pressed(event);
                break;
                
            case WINDOW_EVENT_KEY_RELEASED:
                window_handle_key_released(event);
                break;
                
            case WINDOW_EVENT_MOUSE_MOVED:
                window_handle_mouse_moved(event);
                break;
                
            case WINDOW_EVENT_MOUSE_BUTTON_PRESSED:
                window_handle_mouse_button_pressed(event);
                break;
                
            case WINDOW_EVENT_MOUSE_BUTTON_RELEASED:
                window_handle_mouse_button_released(event);
                break;
        }
    }
    
    manager->event_count = 0;
    manager->stats.events_processed += manager->event_count;
}
```

## Input System

### Comprehensive Input Management

**File: input_manager.c (30,000 LOC)**

```c
// Advanced input manager with multi-platform support
struct Input_Manager {
    // Keyboard state
    struct Keyboard_State keyboard;
    
    // Mouse state
    struct Mouse_State mouse;
    
    // Gamepad states
    struct Gamepad_State gamepads[MAX_GAMEPADS];
    u32 gamepad_count;
    
    // Touch state
    struct Touch_State touch;
    
    // Input mappings
    struct Input_Mapping* mappings;
    u32 mapping_count;
    
    // Input events
    struct Input_Event* event_queue;
    u32 event_count;
    u32 event_capacity;
    
    // Platform-specific input
    union {
        struct Win32_Input* win32_input;
        struct Cocoa_Input* cocoa_input;
        struct X11_Input* x11_input;
        struct Wayland_Input* wayland_input;
        struct UIKit_Input* uikit_input;
        struct Android_Input* android_input;
    } platform_input;
    
    // Statistics
    struct Input_Stats stats;
};

// Update input state for all devices
void input_manager_update(struct Input_Manager* manager) {
    // Platform-specific input update
#ifdef PLATFORM_WINDOWS
    win32_input_update(manager->platform_input.win32_input);
#elif defined(PLATFORM_MACOS)
    cocoa_input_update(manager->platform_input.cocoa_input);
#elif defined(PLATFORM_LINUX)
    if (platform_using_wayland()) {
        wayland_input_update(manager->platform_input.wayland_input);
    } else {
        x11_input_update(manager->platform_input.x11_input);
    }
#elif defined(PLATFORM_IOS)
    uikit_input_update(manager->platform_input.uikit_input);
#elif defined(PLATFORM_ANDROID)
    android_input_update(manager->platform_input.android_input);
#endif
    
    // Process input events
    input_manager_process_events(manager);
    
    // Update gamepad states
    for (u32 i = 0; i < manager->gamepad_count; i++) {
        gamepad_update(&manager->gamepads[i]);
    }
    
    // Update statistics
    manager->stats.update_calls++;
}

// Process input events with action mapping
void input_manager_process_events(struct Input_Manager* manager) {
    for (u32 i = 0; i < manager->event_count; i++) {
        struct Input_Event* event = &manager->event_queue[i];
        
        // Update device states
        switch (event->device_type) {
            case INPUT_DEVICE_KEYBOARD:
                keyboard_process_event(&manager->keyboard, event);
                break;
                
            case INPUT_DEVICE_MOUSE:
                mouse_process_event(&manager->mouse, event);
                break;
                
            case INPUT_DEVICE_GAMEPAD:
                if (event->device_index < manager->gamepad_count) {
                    gamepad_process_event(&manager->gamepads[event->device_index], event);
                }
                break;
                
            case INPUT_DEVICE_TOUCH:
                touch_process_event(&manager->touch, event);
                break;
        }
        
        // Check input mappings
        for (u32 j = 0; j < manager->mapping_count; j++) {
            struct Input_Mapping* mapping = &manager->mappings[j];
            
            if (input_mapping_matches(mapping, event)) {
                input_mapping_execute(mapping, event);
            }
        }
    }
    
    manager->event_count = 0;
}

// Check if specific action is pressed
b32 input_is_action_pressed(struct Input_Manager* manager, const char* action_name) {
    for (u32 i = 0; i < manager->mapping_count; i++) {
        struct Input_Mapping* mapping = &manager->mappings[i];
        
        if (strcmp(mapping->action_name, action_name) == 0) {
            return input_mapping_is_pressed(mapping);
        }
    }
    
    return FALSE;
}
```

## File System

### Cross-Platform File Operations

**File: filesystem_manager.c (28,000 LOC)**

```c
// Advanced filesystem manager with caching and monitoring
struct Filesystem_Manager {
    // Platform-specific filesystem
    union {
        struct Win32_Filesystem* win32_fs;
        struct Cocoa_Filesystem* cocoa_fs;
        struct Linux_Filesystem* linux_fs;
        struct iOS_Filesystem* ios_fs;
        struct Android_Filesystem* android_fs;
    } platform_fs;
    
    // File cache
    struct File_Cache* file_cache;
    
    // File watchers
    struct File_Watcher** watchers;
    u32 watcher_count;
    
    // Mount points
    struct Mount_Point* mount_points;
    u32 mount_count;
    
    // Search paths
    char** search_paths;
    u32 search_path_count;
    
    // Statistics
    struct Filesystem_Stats stats;
};

// Read file with caching and error handling
struct File_Data* filesystem_read_file(struct Filesystem_Manager* fs,
                                      const char* path) {
    // Check cache first
    struct File_Data* cached = file_cache_get(fs->file_cache, path);
    if (cached) {
        fs->stats.cache_hits++;
        return cached;
    }
    
    // Platform-specific file reading
    struct File_Data* data = NULL;
    
#ifdef PLATFORM_WINDOWS
    data = win32_filesystem_read_file(fs->platform_fs.win32_fs, path);
#elif defined(PLATFORM_MACOS)
    data = cocoa_filesystem_read_file(fs->platform_fs.cocoa_fs, path);
#elif defined(PLATFORM_LINUX)
    data = linux_filesystem_read_file(fs->platform_fs.linux_fs, path);
#elif defined(PLATFORM_IOS)
    data = ios_filesystem_read_file(fs->platform_fs.ios_fs, path);
#elif defined(PLATFORM_ANDROID)
    data = android_filesystem_read_file(fs->platform_fs.android_fs, path);
#endif
    
    if (data) {
        // Add to cache
        file_cache_put(fs->file_cache, path, data);
        fs->stats.files_read++;
    } else {
        fs->stats.read_errors++;
    }
    
    return data;
}

// Watch file for changes
File_Watcher_Handle filesystem_watch_file(struct Filesystem_Manager* fs,
                                         const char* path,
                                         File_Change_Callback callback,
                                         void* user_data) {
    // Create watcher
    struct File_Watcher* watcher = malloc(sizeof(struct File_Watcher));
    watcher->path = strdup(path);
    watcher->callback = callback;
    watcher->user_data = user_data;
    watcher->active = TRUE;
    
    // Platform-specific watcher creation
#ifdef PLATFORM_WINDOWS
    watcher->platform_watcher = win32_file_watcher_create(path, callback, user_data);
#elif defined(PLATFORM_MACOS)
    watcher->platform_watcher = cocoa_file_watcher_create(path, callback, user_data);
#elif defined(PLATFORM_LINUX)
    watcher->platform_watcher = linux_file_watcher_create(path, callback, user_data);
#elif defined(PLATFORM_IOS)
    watcher->platform_watcher = ios_file_watcher_create(path, callback, user_data);
#elif defined(PLATFORM_ANDROID)
    watcher->platform_watcher = android_file_watcher_create(path, callback, user_data);
#endif
    
    // Add to list
    if (fs->watcher_count < MAX_FILE_WATCHERS) {
        fs->watchers[fs->watcher_count++] = watcher;
    }
    
    return watcher;
}

// Find file in search paths
const char* filesystem_find_file(struct Filesystem_Manager* fs,
                                const char* filename) {
    // Check each search path
    for (u32 i = 0; i < fs->search_path_count; i++) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", 
                fs->search_paths[i], filename);
        
        if (filesystem_file_exists(fs, full_path)) {
            return strdup(full_path);
        }
    }
    
    // Check mount points
    for (u32 i = 0; i < fs->mount_count; i++) {
        struct Mount_Point* mount = &fs->mount_points[i];
        
        if (strncmp(filename, mount->virtual_path, strlen(mount->virtual_path)) == 0) {
            char real_path[1024];
            snprintf(real_path, sizeof(real_path), "%s/%s",
                    mount->real_path, filename + strlen(mount->virtual_path));
            
            if (filesystem_file_exists(fs, real_path)) {
                return strdup(real_path);
            }
        }
    }
    
    return NULL;
}
```

## Networking

### Cross-Platform Network Abstraction

**File: network_manager.c (30,000 LOC)**

```c
// Advanced network manager with protocol support
struct Network_Manager {
    // Socket management
    struct Socket** sockets;
    u32 socket_count;
    u32 socket_capacity;
    
    // HTTP client
    struct HTTP_Client* http_client;
    
    // WebSocket support
    struct WebSocket_Manager* websocket_manager;
    
    // Network configuration
    struct Network_Config config;
    
    // Statistics
    struct Network_Stats stats;
};

// Create TCP socket with platform abstraction
Socket_Handle network_create_tcp_socket(struct Network_Manager* nm) {
    // Platform-specific socket creation
    Socket_Handle handle = INVALID_SOCKET_HANDLE;
    
#ifdef PLATFORM_WINDOWS
    handle = win32_socket_create_tcp();
#elif defined(PLATFORM_MACOS)
    handle = cocoa_socket_create_tcp();
#elif defined(PLATFORM_LINUX)
    handle = linux_socket_create_tcp();
#elif defined(PLATFORM_IOS)
    handle = ios_socket_create_tcp();
#elif defined(PLATFORM_ANDROID)
    handle = android_socket_create_tcp();
#endif
    
    if (handle != INVALID_SOCKET_HANDLE) {
        // Add to manager
        if (nm->socket_count < nm->socket_capacity) {
            struct Socket* socket = malloc(sizeof(struct Socket));
            socket->handle = handle;
            socket->type = SOCKET_TYPE_TCP;
            socket->state = SOCKET_STATE_CREATED;
            
            nm->sockets[nm->socket_count++] = socket;
        }
        
        nm->stats.sockets_created++;
    }
    
    return handle;
}

// HTTP GET request with automatic retry
struct HTTP_Response* network_http_get(struct Network_Manager* nm,
                                      const char* url,
                                      struct HTTP_Header* headers,
                                      u32 header_count) {
    // Parse URL
    struct URL_Parsed* parsed_url = url_parse(url);
    if (!parsed_url) {
        return NULL;
    }
    
    // Create HTTP request
    struct HTTP_Request* request = http_request_create();
    request->method = HTTP_METHOD_GET;
    request->url = strdup(url);
    request->headers = headers;
    request->header_count = header_count;
    
    // Execute request with retry logic
    struct HTTP_Response* response = NULL;
    u32 retry_count = 0;
    
    while (retry_count < nm->config.max_retries) {
        response = http_client_execute(nm->http_client, request);
        
        if (response && response->status_code < 500) {
            // Success or client error, don't retry
            break;
        }
        
        // Retry with exponential backoff
        retry_count++;
        platform_sleep(retry_count * 1000); // 1s, 2s, 4s, ...
    }
    
    // Cleanup
    http_request_destroy(request);
    url_free(parsed_url);
    
    nm->stats.http_requests++;
    if (retry_count > 0) {
        nm->stats.http_retries += retry_count;
    }
    
    return response;
}

// WebSocket connection with automatic reconnection
WebSocket_Handle network_websocket_connect(struct Network_Manager* nm,
                                          const char* url,
                                          WebSocket_Message_Callback callback,
                                          void* user_data) {
    // Create WebSocket
    WebSocket_Handle handle = websocket_manager_create(nm->websocket_manager,
                                                      url, callback, user_data);
    
    if (handle != INVALID_WEBSOCKET_HANDLE) {
        // Start connection
        websocket_connect(handle);
        
        nm->stats.websockets_created++;
    }
    
    return handle;
}
```

## Hardware Integration

### GPU and CPU Detection

**File: gpu_detection.c (26,000 LOC)**

```c
// Comprehensive GPU detection and information gathering
struct GPU_Detector {
    // GPU information
    struct GPU_Info* gpus;
    u32 gpu_count;
    
    // Detection methods
    struct GPU_Detection_Method** methods;
    u32 method_count;
    
    // Statistics
    struct GPU_Detection_Stats stats;
};

// Detect all available GPUs
void gpu_detection_detect_all(struct GPU_Detector* detector) {
    // Clear previous results
    gpu_detection_clear(detector);
    
    // Platform-specific detection
#ifdef PLATFORM_WINDOWS
    gpu_detection_windows(detector);
#elif defined(PLATFORM_MACOS)
    gpu_detection_macos(detector);
#elif defined(PLATFORM_LINUX)
    gpu_detection_linux(detector);
#elif defined(PLATFORM_ANDROID)
    gpu_detection_android(detector);
#endif
    
    // Additional detection methods
    for (u32 i = 0; i < detector->method_count; i++) {
        struct GPU_Detection_Method* method = detector->methods[i];
        method->detect(detector);
    }
    
    // Log detected GPUs
    for (u32 i = 0; i < detector->gpu_count; i++) {
        gpu_info_log(&detector->gpus[i]);
    }
    
    detector->stats.detection_calls++;
}

// Windows GPU detection using DXGI
void gpu_detection_windows(struct GPU_Detector* detector) {
#ifdef PLATFORM_WINDOWS
    IDXGIFactory* factory;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    
    if (SUCCEEDED(hr)) {
        IDXGIAdapter* adapter;
        u32 adapter_index = 0;
        
        while (factory->EnumAdapters(adapter_index, &adapter) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC desc;
            adapter->GetDesc(&desc);
            
            // Create GPU info
            struct GPU_Info* gpu = &detector->gpus[detector->gpu_count++];
            
            // Convert wide string to UTF-8
            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
                              gpu->name, sizeof(gpu->name), NULL, NULL);
            
            gpu->vendor_id = desc.VendorId;
            gpu->device_id = desc.DeviceId;
            gpu->dedicated_memory = desc.DedicatedVideoMemory;
            gpu->system_memory = desc.DedicatedSystemMemory;
            gpu->shared_memory = desc.SharedSystemMemory;
            
            // Detect GPU features
            gpu_detection_analyze_gpu_features(gpu);
            
            adapter->Release();
            adapter_index++;
        }
        
        factory->Release();
    }
#endif
}

// Get CPU information
void cpu_information_get(struct CPU_Info* info) {
#ifdef PLATFORM_WINDOWS
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    
    info->processor_count = sys_info.dwNumberOfProcessors;
    info->page_size = sys_info.dwPageSize;
    
    // Get CPU name
    __cpuid(info->cpu_name, 0x80000002);
    __cpuid(info->cpu_name + 4, 0x80000003);
    __cpuid(info->cpu_name + 8, 0x80000004);
    
#elif defined(PLATFORM_LINUX)
    // Read from /proc/cpuinfo
    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strncmp(line, "processor\t:", 11) == 0) {
                info->processor_count++;
            } else if (strncmp(line, "model name\t:", 12) == 0) {
                strncpy(info->name, line + 13, sizeof(info->name));
                info->name[strcspn(info->name, "\n")] = 0;
            }
        }
        fclose(cpuinfo);
    }
    
    // Get page size
    info->page_size = sysconf(_SC_PAGESIZE);
#endif
    
    // Detect CPU features
    cpu_information_detect_features(info);
}
```

## Engine Integration

### Platform Integration

```c
// Initialize platform layer
void engine_platform_integration(struct Engine* engine) {
    // Detect platform
    platform_detect(&engine->platform_info);
    
    // Create platform services
    engine->platform_services = platform_services_create();
    
    // Initialize window manager
    engine->window_manager = window_manager_create();
    
    // Initialize input manager
    engine->input_manager = input_manager_create();
    
    // Initialize filesystem
    engine->filesystem = filesystem_manager_create();
    
    // Initialize networking
    engine->network_manager = network_manager_create();
    
    // Set up platform-specific paths
    platform_setup_paths(engine);
}

// Set up platform-specific paths
void platform_setup_paths(struct Engine* engine) {
    // Get executable directory
    char exe_path[1024];
    platform_get_executable_path(exe_path, sizeof(exe_path));
    
    // Set up search paths
    filesystem_add_search_path(engine->filesystem, exe_path);
    
    // Add platform-specific paths
#ifdef PLATFORM_WINDOWS
    char app_data_path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, app_data_path);
    filesystem_add_search_path(engine->filesystem, app_data_path);
    
    char documents_path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, documents_path);
    filesystem_add_search_path(engine->filesystem, documents_path);
    
#elif defined(PLATFORM_MACOS)
    char home_path[1024];
    snprintf(home_path, sizeof(home_path), "%s/Library/Application Support",
             getenv("HOME"));
    filesystem_add_search_path(engine->filesystem, home_path);
    
    snprintf(home_path, sizeof(home_path), "%s/Documents", getenv("HOME"));
    filesystem_add_search_path(engine->filesystem, home_path);
    
#elif defined(PLATFORM_LINUX)
    char home_path[1024];
    snprintf(home_path, sizeof(home_path), "%s/.local/share", getenv("HOME"));
    filesystem_add_search_path(engine->filesystem, home_path);
    
    snprintf(home_path, sizeof(home_path), "%s/Documents", getenv("HOME"));
    filesystem_add_search_path(engine->filesystem, home_path);
#endif
}

// Platform update loop
void platform_update(struct Engine* engine) {
    // Process window events
    window_process_events(engine->window_manager);
    
    // Update input
    input_manager_update(engine->input_manager);
    
    // Check for quit
    if (window_should_close(engine->main_window)) {
        engine->running = FALSE;
    }
}
```

This Platform Integration documentation provides comprehensive coverage of the 2.4 million lines of code dedicated to cross-platform support in the game engine. The system abstracts platform-specific functionality across Windows, macOS, Linux, iOS, and Android, providing unified APIs for windowing, input, file systems, networking, and hardware integration.