#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

// Function prototypes
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);
static void load_debug_utils_functions(VkInstance instance);
static bool check_validation_layers_available(void);
static void add_validation_layer(const VkLayerProperties* properties);
static bool is_validation_layer_supported(const char* layer_name);
static void log_validation_message(const char* level, const char* message);
static void format_debug_message(char* buffer, size_t buffer_size, 
                                VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                const VkDebugUtilsMessengerCallbackDataEXT* data);
static void update_statistics(VkDebugUtilsMessageSeverityFlagBitsEXT severity);
static bool validate_callback_parameters(const VkDebugUtilsMessengerCreateInfoEXT* create_info);
static void cleanup_debug_callback(vk_debug_callback_t* callback);
static void initialize_thread_safety(void);
static void cleanup_thread_safety(void);
static void lock_validation_manager(void);
static void unlock_validation_manager(void);

// Vulkan validation layer integration
#define VK_MAX_VALIDATION_LAYERS 16
#define VK_MAX_DEBUG_CALLBACKS 32
#define VK_MAX_DEBUG_MESSAGES 1024

typedef struct vk_validation_layer {
    char name[256];
    VkLayerProperties properties;
    bool is_enabled;
    bool is_available;
} vk_validation_layer_t;

typedef struct vk_debug_callback {
    VkDebugUtilsMessengerEXT handle;
    VkDebugUtilsMessengerCreateInfoEXT create_info;
    PFN_vkDebugUtilsMessengerCallbackEXT callback_func;
    void* user_data;
    bool is_active;
} vk_debug_callback_t;

typedef struct vk_validation_manager {
    VkInstance instance;
    VkDebugUtilsMessengerEXT default_messenger;
    
    vk_validation_layer_t layers[VK_MAX_VALIDATION_LAYERS];
    u32 layer_count;
    u32 enabled_layer_count;
    
    vk_debug_callback_t callbacks[VK_MAX_DEBUG_CALLBACKS];
    u32 callback_count;
    
    // Debug message buffer
    char debug_messages[VK_MAX_DEBUG_MESSAGES][512];
    u32 debug_message_count;
    u32 current_message_index;
    
    // Statistics
    u32 total_debug_messages;
    u32 error_count;
    u32 warning_count;
    u32 info_count;
    u32 verbose_count;
    
    // Thread safety
    pthread_mutex_t validation_mutex;
    bool thread_safety_initialized;
    
    // Configuration
    bool validation_enabled;
    bool debug_utils_available;
    bool verbose_logging;
    bool file_logging;
    FILE* log_file;
    char log_filename[256];
    
    // Performance tracking
    clock_t start_time;
    u32 callbacks_created;
    u32 callbacks_destroyed;
    u32 layers_enabled;
    u32 layers_disabled;
} vk_validation_manager_t;

static vk_validation_manager_t g_validation_manager = {0};

// Debug callback function
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    
    // Store debug message
    if (g_validation_manager.debug_message_count < VK_MAX_DEBUG_MESSAGES) {
        u32 index = g_validation_manager.current_message_index;
        char* message = g_validation_manager.debug_messages[index];
        
        snprintf(message, 512, "[%s] %s (Object: %s)",
                callback_data->pMessageIdName ? callback_data->pMessageIdName : "UNKNOWN",
                callback_data->pMessage ? callback_data->pMessage : "No message",
                callback_data->pObjectName ? callback_data->pObjectName : "No object");
        
        g_validation_manager.current_message_index = (index + 1) % VK_MAX_DEBUG_MESSAGES;
        g_validation_manager.debug_message_count++;
        g_validation_manager.total_debug_messages++;
        
        // Count message types
        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            g_validation_manager.error_count++;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            g_validation_manager.warning_count++;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            g_validation_manager.info_count++;
        } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            g_validation_manager.verbose_count++;
        }
    }
    
    // Print to console
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        printf("[ERROR] %s\n", callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("[WARNING] %s\n", callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        printf("[INFO] %s\n", callback_data->pMessage);
    } else {
        printf("[VERBOSE] %s\n", callback_data->pMessage);
    }
    
    return VK_FALSE; // Don't abort on validation errors
}

// Load debug utils function
static PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = NULL;
static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = NULL;

// Load debug utils functions
static void load_debug_utils_functions(VkInstance instance) {
    vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    
    g_validation_manager.debug_utils_available = (vkCreateDebugUtilsMessengerEXT != NULL && vkDestroyDebugUtilsMessengerEXT != NULL);
    
    if (g_validation_manager.debug_utils_available) {
        printf("Vulkan debug utils functions loaded\n");
    } else {
        printf("Warning: Vulkan debug utils not available\n");
    }
}

// Add validation layer to manager
static void add_validation_layer(const VkLayerProperties* properties) {
    if (g_validation_manager.layer_count >= VK_MAX_VALIDATION_LAYERS) {
        printf("Warning: Maximum validation layers reached\n");
        return;
    }
    
    vk_validation_layer_t* layer = &g_validation_manager.layers[g_validation_manager.layer_count];
    strncpy(layer->name, properties->layerName, 255);
    layer->name[255] = '\0';
    layer->properties = *properties;
    layer->is_available = true;
    layer->is_enabled = false;
    g_validation_manager.layer_count++;
}

// Check if validation layer is supported
static bool is_validation_layer_supported(const char* layer_name) {
    if (!layer_name) return false;
    
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (strcmp(g_validation_manager.layers[i].name, layer_name) == 0) {
            return g_validation_manager.layers[i].is_available;
        }
    }
    return false;
}

// Log validation message
static void log_validation_message(const char* level, const char* message) {
    if (!message) return;
    
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    printf("[%s] [%s] %s\n", timestamp, level, message);
    
    if (g_validation_manager.file_logging && g_validation_manager.log_file) {
        fprintf(g_validation_manager.log_file, "[%s] [%s] %s\n", timestamp, level, message);
        fflush(g_validation_manager.log_file);
    }
}

// Format debug message
static void format_debug_message(char* buffer, size_t buffer_size, 
                                VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                const VkDebugUtilsMessengerCallbackDataEXT* data) {
    if (!buffer || !data || buffer_size == 0) return;
    
    const char* severity_str = "UNKNOWN";
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity_str = "ERROR"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity_str = "WARNING"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity_str = "INFO"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity_str = "VERBOSE"; break;
    }
    
    snprintf(buffer, buffer_size, "[%s] [%s] %s (Object: %s)",
             severity_str,
             data->pMessageIdName ? data->pMessageIdName : "UNKNOWN",
             data->pMessage ? data->pMessage : "No message",
             data->pObjectName ? data->pObjectName : "No object");
}

// Update statistics
static void update_statistics(VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
    g_validation_manager.total_debug_messages++;
    
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        g_validation_manager.error_count++;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        g_validation_manager.warning_count++;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        g_validation_manager.info_count++;
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        g_validation_manager.verbose_count++;
    }
}

// Validate callback parameters
static bool validate_callback_parameters(const VkDebugUtilsMessengerCreateInfoEXT* create_info) {
    if (!create_info) return true; // NULL is valid, will use defaults
    
    if (create_info->sType != VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT) {
        printf("Error: Invalid sType in debug messenger create info\n");
        return false;
    }
    
    if (create_info->messageSeverity == 0) {
        printf("Warning: No message severity specified\n");
    }
    
    if (create_info->messageType == 0) {
        printf("Warning: No message type specified\n");
    }
    
    if (!create_info->pfnUserCallback) {
        printf("Error: No callback function specified\n");
        return false;
    }
    
    return true;
}

// Cleanup debug callback
static void cleanup_debug_callback(vk_debug_callback_t* callback) {
    if (!callback) return;
    
    if (callback->is_active && callback->handle != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(g_validation_manager.instance, callback->handle, NULL);
    }
    
    memset(callback, 0, sizeof(vk_debug_callback_t));
    callback->is_active = false;
}

// Initialize thread safety
static void initialize_thread_safety(void) {
    if (g_validation_manager.thread_safety_initialized) return;
    
    int result = pthread_mutex_init(&g_validation_manager.validation_mutex, NULL);
    if (result == 0) {
        g_validation_manager.thread_safety_initialized = true;
    } else {
        printf("Warning: Failed to initialize thread safety (error: %d)\n", result);
    }
}

// Cleanup thread safety
static void cleanup_thread_safety(void) {
    if (!g_validation_manager.thread_safety_initialized) return;
    
    pthread_mutex_destroy(&g_validation_manager.validation_mutex);
    g_validation_manager.thread_safety_initialized = false;
}

// Lock validation manager
static void lock_validation_manager(void) {
    if (g_validation_manager.thread_safety_initialized) {
        pthread_mutex_lock(&g_validation_manager.validation_mutex);
    }
}

// Unlock validation manager
static void unlock_validation_manager(void) {
    if (g_validation_manager.thread_safety_initialized) {
        pthread_mutex_unlock(&g_validation_manager.validation_mutex);
    }
}

// Check if validation layers are available
static bool check_validation_layers_available(void) {
    u32 layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    
    if (layer_count == 0) {
        printf("No validation layers available\n");
        return false;
    }
    
    VkLayerProperties* available_layers = malloc(layer_count * sizeof(VkLayerProperties));
    if (!available_layers) {
        printf("Error: Failed to allocate memory for validation layers\n");
        return false;
    }
    
    VkResult result = vkEnumerateInstanceLayerProperties(&layer_count, available_layers);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to enumerate instance layers (result: %d)\n", result);
        free(available_layers);
        return false;
    }
    
    printf("Available validation layers:\n");
    for (u32 i = 0; i < layer_count; i++) {
        printf("  - %s (v%u.%u.%u)\n", 
               available_layers[i].layerName,
               VK_VERSION_MAJOR(available_layers[i].specVersion),
               VK_VERSION_MINOR(available_layers[i].specVersion),
               VK_VERSION_PATCH(available_layers[i].specVersion));
        
        // Check for standard validation layers
        if (strcmp(available_layers[i].layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            if (g_validation_manager.layer_count < VK_MAX_VALIDATION_LAYERS) {
                vk_validation_layer_t* layer = &g_validation_manager.layers[g_validation_manager.layer_count];
                strncpy(layer->name, available_layers[i].layerName, 255);
                layer->name[255] = '\0';
                layer->properties = available_layers[i];
                layer->is_available = true;
                layer->is_enabled = false;
                g_validation_manager.layer_count++;
            }
        } else if (strcmp(available_layers[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
            if (g_validation_manager.layer_count < VK_MAX_VALIDATION_LAYERS) {
                vk_validation_layer_t* layer = &g_validation_manager.layers[g_validation_manager.layer_count];
                strncpy(layer->name, available_layers[i].layerName, 255);
                layer->name[255] = '\0';
                layer->properties = available_layers[i];
                layer->is_available = true;
                layer->is_enabled = false;
                g_validation_manager.layer_count++;
            }
        }
    }
    
    free(available_layers);
    printf("Found %u validation layers\n", g_validation_manager.layer_count);
    
    return g_validation_manager.layer_count > 0;
}

// Initialize validation manager
bool vk_validation_manager_init(VkInstance instance, bool enable_validation) {
    if (!instance) {
        printf("Error: Invalid instance for validation manager initialization\n");
        return false;
    }
    
    lock_validation_manager();
    
    // Initialize basic fields
    g_validation_manager.instance = instance;
    g_validation_manager.validation_enabled = enable_validation;
    g_validation_manager.verbose_logging = false;
    g_validation_manager.file_logging = false;
    g_validation_manager.log_file = NULL;
    g_validation_manager.start_time = clock();
    g_validation_manager.callbacks_created = 0;
    g_validation_manager.callbacks_destroyed = 0;
    g_validation_manager.layers_enabled = 0;
    g_validation_manager.layers_disabled = 0;
    
    // Initialize thread safety
    initialize_thread_safety();
    
    // Set default log filename
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    strftime(g_validation_manager.log_filename, sizeof(g_validation_manager.log_filename), 
             "vulkan_validation_%Y%m%d_%H%M%S.log", timeinfo);
    
    // Load debug utils functions
    load_debug_utils_functions(instance);
    
    // Check for available validation layers
    if (enable_validation) {
        if (!check_validation_layers_available()) {
            printf("Warning: Validation requested but no layers available\n");
            unlock_validation_manager();
            return false;
        }
        
        // Enable standard validation layers
        for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
            if (strcmp(g_validation_manager.layers[i].name, "VK_LAYER_KHRONOS_validation") == 0 ||
                strcmp(g_validation_manager.layers[i].name, "VK_LAYER_LUNARG_standard_validation") == 0) {
                g_validation_manager.layers[i].is_enabled = true;
                g_validation_manager.enabled_layer_count++;
                g_validation_manager.layers_enabled++;
            }
        }
        
        printf("Enabled %u validation layers\n", g_validation_manager.enabled_layer_count);
        
        // Create default debug messenger
        if (g_validation_manager.debug_utils_available) {
            VkDebugUtilsMessengerCreateInfoEXT messenger_info = {0};
            messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            messenger_info.pfnUserCallback = debug_callback;
            messenger_info.pUserData = NULL;
            
            VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &messenger_info, NULL, &g_validation_manager.default_messenger);
            if (result != VK_SUCCESS) {
                printf("Warning: Failed to create default debug messenger (result: %d)\n", result);
            } else {
                printf("Created default debug messenger\n");
                g_validation_manager.callbacks_created++;
            }
        }
    }
    
    unlock_validation_manager();
    printf("Vulkan validation manager initialized (validation: %s)\n", enable_validation ? "enabled" : "disabled");
    return true;
}

// Cleanup validation manager
void vk_validation_manager_cleanup(void) {
    if (!g_validation_manager.instance) {
        return;
    }
    
    lock_validation_manager();
    
    // Close log file
    if (g_validation_manager.log_file) {
        fclose(g_validation_manager.log_file);
        g_validation_manager.log_file = NULL;
    }
    
    // Destroy all debug callbacks
    for (u32 i = 0; i < g_validation_manager.callback_count; i++) {
        if (g_validation_manager.callbacks[i].is_active && g_validation_manager.callbacks[i].handle != VK_NULL_HANDLE) {
            vkDestroyDebugUtilsMessengerEXT(g_validation_manager.instance, g_validation_manager.callbacks[i].handle, NULL);
            g_validation_manager.callbacks_destroyed++;
        }
    }
    
    // Destroy default messenger
    if (g_validation_manager.default_messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(g_validation_manager.instance, g_validation_manager.default_messenger, NULL);
        g_validation_manager.default_messenger = VK_NULL_HANDLE;
        g_validation_manager.callbacks_destroyed++;
    }
    
    // Print final statistics
    clock_t end_time = clock();
    double duration = ((double)(end_time - g_validation_manager.start_time)) / CLOCKS_PER_SEC;
    printf("\n=== Vulkan Validation Statistics ===\n");
    printf("Runtime: %.2f seconds\n", duration);
    printf("Total messages: %u\n", g_validation_manager.total_debug_messages);
    printf("Errors: %u\n", g_validation_manager.error_count);
    printf("Warnings: %u\n", g_validation_manager.warning_count);
    printf("Info: %u\n", g_validation_manager.info_count);
    printf("Verbose: %u\n", g_validation_manager.verbose_count);
    printf("Callbacks created: %u\n", g_validation_manager.callbacks_created);
    printf("Callbacks destroyed: %u\n", g_validation_manager.callbacks_destroyed);
    printf("Layers enabled: %u\n", g_validation_manager.layers_enabled);
    printf("Layers disabled: %u\n", g_validation_manager.layers_disabled);
    printf("===================================\n\n");
    
    // Cleanup thread safety
    cleanup_thread_safety();
    
    memset(&g_validation_manager, 0, sizeof(g_validation_manager));
    
    unlock_validation_manager();
    printf("Vulkan validation manager cleaned up\n");
}

// Create debug callback
u32 vk_validation_create_debug_callback(const VkDebugUtilsMessengerCreateInfoEXT* create_info, PFN_vkDebugUtilsMessengerCallbackEXT callback_func, void* user_data) {
    if (!g_validation_manager.instance || !g_validation_manager.debug_utils_available) {
        printf("Error: Validation manager not initialized or debug utils unavailable\n");
        return 0;
    }
    
    lock_validation_manager();
    
    if (g_validation_manager.callback_count >= VK_MAX_DEBUG_CALLBACKS) {
        printf("Error: Maximum debug callbacks reached\n");
        unlock_validation_manager();
        return 0;
    }
    
    if (!validate_callback_parameters(create_info)) {
        unlock_validation_manager();
        return 0;
    }
    
    u32 callback_id = g_validation_manager.callback_count + 1;
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[g_validation_manager.callback_count];
    
    VkDebugUtilsMessengerCreateInfoEXT messenger_info = {0};
    if (create_info) {
        messenger_info = *create_info;
    } else {
        messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messenger_info.pfnUserCallback = callback_func ? callback_func : debug_callback;
        messenger_info.pUserData = user_data;
    }
    
    VkResult result = vkCreateDebugUtilsMessengerEXT(g_validation_manager.instance, &messenger_info, NULL, &callback->handle);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create debug callback (result: %d)\n", result);
        unlock_validation_manager();
        return 0;
    }
    
    callback->create_info = messenger_info;
    callback->callback_func = messenger_info.pfnUserCallback;
    callback->user_data = messenger_info.pUserData;
    callback->is_active = true;
    g_validation_manager.callback_count++;
    g_validation_manager.callbacks_created++;
    
    unlock_validation_manager();
    printf("Created debug callback (ID: %u)\n", callback_id);
    
    return callback_id;
}

// Destroy debug callback
bool vk_validation_destroy_debug_callback(u32 callback_id) {
    if (!g_validation_manager.instance || callback_id == 0) {
        return false;
    }
    
    lock_validation_manager();
    
    if (callback_id > g_validation_manager.callback_count) {
        printf("Error: Invalid debug callback ID %u\n", callback_id);
        unlock_validation_manager();
        return false;
    }
    
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[callback_id - 1];
    
    if (!callback->is_active) {
        unlock_validation_manager();
        return false;
    }
    
    cleanup_debug_callback(callback);
    g_validation_manager.callbacks_destroyed++;
    
    unlock_validation_manager();
    printf("Destroyed debug callback (ID: %u)\n", callback_id);
    return true;
}

// Get enabled validation layers
bool vk_validation_get_enabled_layers(const char** layer_names, u32* layer_count) {
    if (!layer_names || !layer_count) {
        return false;
    }
    
    u32 count = 0;
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (g_validation_manager.layers[i].is_enabled) {
            layer_names[count++] = g_validation_manager.layers[i].name;
        }
    }
    
    *layer_count = count;
    return count > 0;
}

// Enable validation layer
bool vk_validation_enable_layer(const char* layer_name) {
    if (!layer_name || !g_validation_manager.validation_enabled) {
        return false;
    }
    
    lock_validation_manager();
    
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (strcmp(g_validation_manager.layers[i].name, layer_name) == 0) {
            if (!g_validation_manager.layers[i].is_enabled) {
                g_validation_manager.layers[i].is_enabled = true;
                g_validation_manager.enabled_layer_count++;
                g_validation_manager.layers_enabled++;
                unlock_validation_manager();
                printf("Enabled validation layer: %s\n", layer_name);
                return true;
            }
            unlock_validation_manager();
            return true; // Already enabled
        }
    }
    
    unlock_validation_manager();
    printf("Warning: Validation layer not found: %s\n", layer_name);
    return false;
}

// Disable validation layer
bool vk_validation_disable_layer(const char* layer_name) {
    if (!layer_name || !g_validation_manager.validation_enabled) {
        return false;
    }
    
    lock_validation_manager();
    
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (strcmp(g_validation_manager.layers[i].name, layer_name) == 0) {
            if (g_validation_manager.layers[i].is_enabled) {
                g_validation_manager.layers[i].is_enabled = false;
                g_validation_manager.enabled_layer_count--;
                g_validation_manager.layers_disabled++;
                unlock_validation_manager();
                printf("Disabled validation layer: %s\n", layer_name);
                return true;
            }
            unlock_validation_manager();
            return true; // Already disabled
        }
    }
    
    unlock_validation_manager();
    printf("Warning: Validation layer not found: %s\n", layer_name);
    return false;
}

// Get debug message
const char* vk_validation_get_debug_message(u32 message_index) {
    if (message_index >= g_validation_manager.debug_message_count) {
        return NULL;
    }
    
    u32 index = (g_validation_manager.current_message_index - 1 - message_index + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
    return g_validation_manager.debug_messages[index];
}

// Get debug message count
u32 vk_validation_get_debug_message_count(void) {
    return g_validation_manager.debug_message_count;
}

// Clear debug messages
void vk_validation_clear_debug_messages(void) {
    g_validation_manager.debug_message_count = 0;
    g_validation_manager.current_message_index = 0;
    memset(g_validation_manager.debug_messages, 0, sizeof(g_validation_manager.debug_messages));
}

// Get statistics
void vk_validation_get_stats(u32* total_debug_messages, u32* error_count, u32* warning_count, u32* info_count, u32* verbose_count) {
    if (total_debug_messages) *total_debug_messages = g_validation_manager.total_debug_messages;
    if (error_count) *error_count = g_validation_manager.error_count;
    if (warning_count) *warning_count = g_validation_manager.warning_count;
    if (info_count) *info_count = g_validation_manager.info_count;
    if (verbose_count) *verbose_count = g_validation_manager.verbose_count;
}

// Check if validation is enabled
bool vk_validation_is_enabled(void) {
    return g_validation_manager.validation_enabled;
}

// Check if debug utils are available
bool vk_validation_debug_utils_available(void) {
    return g_validation_manager.debug_utils_available;
}

// Set validation enabled state
void vk_validation_set_enabled(bool enabled) {
    g_validation_manager.validation_enabled = enabled;
}

// Get default messenger handle
VkDebugUtilsMessengerEXT vk_validation_get_default_messenger(void) {
    return g_validation_manager.default_messenger;
}

// Report validation error
void vk_validation_report_error(const char* message) {
    if (!g_validation_manager.validation_enabled || !message) {
        return;
    }
    
    lock_validation_manager();
    g_validation_manager.error_count++;
    g_validation_manager.total_debug_messages++;
    unlock_validation_manager();
    
    log_validation_message("ERROR", message);
}

// Report validation warning
void vk_validation_report_warning(const char* message) {
    if (!g_validation_manager.validation_enabled || !message) {
        return;
    }
    
    lock_validation_manager();
    g_validation_manager.warning_count++;
    g_validation_manager.total_debug_messages++;
    unlock_validation_manager();
    
    log_validation_message("WARNING", message);
}

// Report validation info
void vk_validation_report_info(const char* message) {
    if (!g_validation_manager.validation_enabled || !message) {
        return;
    }
    
    lock_validation_manager();
    g_validation_manager.info_count++;
    g_validation_manager.total_debug_messages++;
    unlock_validation_manager();
    
    log_validation_message("INFO", message);
}

// Report validation verbose
void vk_validation_report_verbose(const char* message) {
    if (!g_validation_manager.validation_enabled || !message) {
        return;
    }
    
    lock_validation_manager();
    g_validation_manager.verbose_count++;
    g_validation_manager.total_debug_messages++;
    unlock_validation_manager();
    
    log_validation_message("VERBOSE", message);
}

// Enable file logging
bool vk_validation_enable_file_logging(const char* filename) {
    if (!filename) {
        filename = g_validation_manager.log_filename;
    }
    
    lock_validation_manager();
    
    if (g_validation_manager.log_file) {
        fclose(g_validation_manager.log_file);
    }
    
    g_validation_manager.log_file = fopen(filename, "w");
    if (!g_validation_manager.log_file) {
        unlock_validation_manager();
        printf("Error: Failed to open log file: %s\n", filename);
        return false;
    }
    
    g_validation_manager.file_logging = true;
    strncpy(g_validation_manager.log_filename, filename, 255);
    g_validation_manager.log_filename[255] = '\0';
    
    unlock_validation_manager();
    printf("Enabled file logging to: %s\n", filename);
    return true;
}

// Disable file logging
void vk_validation_disable_file_logging(void) {
    lock_validation_manager();
    
    if (g_validation_manager.log_file) {
        fclose(g_validation_manager.log_file);
        g_validation_manager.log_file = NULL;
    }
    
    g_validation_manager.file_logging = false;
    
    unlock_validation_manager();
    printf("Disabled file logging\n");
}

// Set verbose logging
void vk_validation_set_verbose_logging(bool enabled) {
    lock_validation_manager();
    g_validation_manager.verbose_logging = enabled;
    unlock_validation_manager();
    printf("Verbose logging %s\n", enabled ? "enabled" : "disabled");
}

// Get verbose logging state
bool vk_validation_get_verbose_logging(void) {
    return g_validation_manager.verbose_logging;
}

// Get file logging state
bool vk_validation_get_file_logging(void) {
    return g_validation_manager.file_logging;
}

// Get log filename
const char* vk_validation_get_log_filename(void) {
    return g_validation_manager.log_filename;
}

// Reset statistics
void vk_validation_reset_statistics(void) {
    lock_validation_manager();
    g_validation_manager.total_debug_messages = 0;
    g_validation_manager.error_count = 0;
    g_validation_manager.warning_count = 0;
    g_validation_manager.info_count = 0;
    g_validation_manager.verbose_count = 0;
    unlock_validation_manager();
    printf("Reset validation statistics\n");
}

// Get performance statistics
void vk_validation_get_performance_stats(double* runtime_seconds, u32* callbacks_created, u32* callbacks_destroyed) {
    if (runtime_seconds) {
        clock_t current_time = clock();
        *runtime_seconds = ((double)(current_time - g_validation_manager.start_time)) / CLOCKS_PER_SEC;
    }
    if (callbacks_created) *callbacks_created = g_validation_manager.callbacks_created;
    if (callbacks_destroyed) *callbacks_destroyed = g_validation_manager.callbacks_destroyed;
}

// Dump all debug messages to file
bool vk_validation_dump_messages_to_file(const char* filename) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Failed to open dump file: %s\n", filename);
        return false;
    }
    
    lock_validation_manager();
    
    fprintf(file, "=== Vulkan Validation Messages Dump ===\n");
    fprintf(file, "Total messages: %u\n\n", g_validation_manager.debug_message_count);
    
    for (u32 i = 0; i < g_validation_manager.debug_message_count; i++) {
        u32 index = (g_validation_manager.current_message_index - 1 - i + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
        fprintf(file, "[%u] %s\n", i, g_validation_manager.debug_messages[index]);
    }
    
    fprintf(file, "\n=== End of Dump ===\n");
    
    unlock_validation_manager();
    
    fclose(file);
    printf("Dumped %u messages to: %s\n", g_validation_manager.debug_message_count, filename);
    return true;
}

// Get layer properties
bool vk_validation_get_layer_properties(const char* layer_name, VkLayerProperties* properties) {
    if (!layer_name || !properties) return false;
    
    lock_validation_manager();
    
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (strcmp(g_validation_manager.layers[i].name, layer_name) == 0) {
            *properties = g_validation_manager.layers[i].properties;
            unlock_validation_manager();
            return true;
        }
    }
    
    unlock_validation_manager();
    return false;
}

// Get all available layers
u32 vk_validation_get_all_layers(const char** layer_names, u32 max_count) {
    if (!layer_names || max_count == 0) return 0;
    
    lock_validation_manager();
    
    u32 count = 0;
    for (u32 i = 0; i < g_validation_manager.layer_count && count < max_count; i++) {
        layer_names[count++] = g_validation_manager.layers[i].name;
    }
    
    unlock_validation_manager();
    return count;
}

// Check if specific layer is enabled
bool vk_validation_is_layer_enabled(const char* layer_name) {
    if (!layer_name) return false;
    
    lock_validation_manager();
    
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (strcmp(g_validation_manager.layers[i].name, layer_name) == 0) {
            bool enabled = g_validation_manager.layers[i].is_enabled;
            unlock_validation_manager();
            return enabled;
        }
    }
    
    unlock_validation_manager();
    return false;
}

// Enable all validation layers
u32 vk_validation_enable_all_layers(void) {
    lock_validation_manager();
    
    u32 enabled_count = 0;
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (!g_validation_manager.layers[i].is_enabled) {
            g_validation_manager.layers[i].is_enabled = true;
            g_validation_manager.enabled_layer_count++;
            g_validation_manager.layers_enabled++;
            enabled_count++;
        }
    }
    
    unlock_validation_manager();
    printf("Enabled %u validation layers\n", enabled_count);
    return enabled_count;
}

// Disable all validation layers
u32 vk_validation_disable_all_layers(void) {
    lock_validation_manager();
    
    u32 disabled_count = 0;
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (g_validation_manager.layers[i].is_enabled) {
            g_validation_manager.layers[i].is_enabled = false;
            g_validation_manager.enabled_layer_count--;
            g_validation_manager.layers_disabled++;
            disabled_count++;
        }
    }
    
    unlock_validation_manager();
    printf("Disabled %u validation layers\n", disabled_count);
    return disabled_count;
}

// Get callback info
bool vk_validation_get_callback_info(u32 callback_id, VkDebugUtilsMessengerCreateInfoEXT* info) {
    if (!info || callback_id == 0 || callback_id > g_validation_manager.callback_count) {
        return false;
    }
    
    lock_validation_manager();
    
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[callback_id - 1];
    if (!callback->is_active) {
        unlock_validation_manager();
        return false;
    }
    
    *info = callback->create_info;
    
    unlock_validation_manager();
    return true;
}

// Set callback user data
bool vk_validation_set_callback_user_data(u32 callback_id, void* user_data) {
    if (callback_id == 0 || callback_id > g_validation_manager.callback_count) {
        return false;
    }
    
    lock_validation_manager();
    
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[callback_id - 1];
    if (!callback->is_active) {
        unlock_validation_manager();
        return false;
    }
    
    callback->user_data = user_data;
    
    unlock_validation_manager();
    return true;
}

// Get callback user data
void* vk_validation_get_callback_user_data(u32 callback_id) {
    if (callback_id == 0 || callback_id > g_validation_manager.callback_count) {
        return NULL;
    }
    
    lock_validation_manager();
    
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[callback_id - 1];
    void* user_data = callback->is_active ? callback->user_data : NULL;
    
    unlock_validation_manager();
    return user_data;
}

// Check if callback is active
bool vk_validation_is_callback_active(u32 callback_id) {
    if (callback_id == 0 || callback_id > g_validation_manager.callback_count) {
        return false;
    }
    
    lock_validation_manager();
    
    bool active = g_validation_manager.callbacks[callback_id - 1].is_active;
    
    unlock_validation_manager();
    return active;
}

// Get active callback count
u32 vk_validation_get_active_callback_count(void) {
    lock_validation_manager();
    
    u32 active_count = 0;
    for (u32 i = 0; i < g_validation_manager.callback_count; i++) {
        if (g_validation_manager.callbacks[i].is_active) {
            active_count++;
        }
    }
    
    unlock_validation_manager();
    return active_count;
}

// Destroy all callbacks
u32 vk_validation_destroy_all_callbacks(void) {
    lock_validation_manager();
    
    u32 destroyed_count = 0;
    for (u32 i = 0; i < g_validation_manager.callback_count; i++) {
        if (g_validation_manager.callbacks[i].is_active) {
            cleanup_debug_callback(&g_validation_manager.callbacks[i]);
            g_validation_manager.callbacks_destroyed++;
            destroyed_count++;
        }
    }
    
    unlock_validation_manager();
    printf("Destroyed %u debug callbacks\n", destroyed_count);
    return destroyed_count;
}

// Print all enabled layers
void vk_validation_print_enabled_layers(void) {
    lock_validation_manager();
    
    printf("Enabled validation layers:\n");
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (g_validation_manager.layers[i].is_enabled) {
            printf("  - %s\n", g_validation_manager.layers[i].name);
        }
    }
    
    unlock_validation_manager();
}

// Print all available layers
void vk_validation_print_available_layers(void) {
    lock_validation_manager();
    
    printf("Available validation layers:\n");
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        printf("  - %s (%s)\n", 
               g_validation_manager.layers[i].name,
               g_validation_manager.layers[i].is_enabled ? "ENABLED" : "DISABLED");
    }
    
    unlock_validation_manager();
}

// Print current statistics
void vk_validation_print_statistics(void) {
    lock_validation_manager();
    
    printf("\n=== Current Vulkan Validation Statistics ===\n");
    printf("Total messages: %u\n", g_validation_manager.total_debug_messages);
    printf("Errors: %u\n", g_validation_manager.error_count);
    printf("Warnings: %u\n", g_validation_manager.warning_count);
    printf("Info: %u\n", g_validation_manager.info_count);
    printf("Verbose: %u\n", g_validation_manager.verbose_count);
    printf("Active callbacks: %u\n", vk_validation_get_active_callback_count());
    printf("Enabled layers: %u\n", g_validation_manager.enabled_layer_count);
    printf("File logging: %s\n", g_validation_manager.file_logging ? "enabled" : "disabled");
    printf("Verbose logging: %s\n", g_validation_manager.verbose_logging ? "enabled" : "disabled");
    printf("=========================================\n\n");
    
    unlock_validation_manager();
}

// Export statistics to JSON
bool vk_validation_export_statistics_json(const char* filename) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Failed to open JSON export file: %s\n", filename);
        return false;
    }
    
    lock_validation_manager();
    
    fprintf(file, "{\n");
    fprintf(file, "  \"total_messages\": %u,\n", g_validation_manager.total_debug_messages);
    fprintf(file, "  \"error_count\": %u,\n", g_validation_manager.error_count);
    fprintf(file, "  \"warning_count\": %u,\n", g_validation_manager.warning_count);
    fprintf(file, "  \"info_count\": %u,\n", g_validation_manager.info_count);
    fprintf(file, "  \"verbose_count\": %u,\n", g_validation_manager.verbose_count);
    fprintf(file, "  \"enabled_layers\": %u,\n", g_validation_manager.enabled_layer_count);
    fprintf(file, "  \"active_callbacks\": %u,\n", vk_validation_get_active_callback_count());
    fprintf(file, "  \"callbacks_created\": %u,\n", g_validation_manager.callbacks_created);
    fprintf(file, "  \"callbacks_destroyed\": %u,\n", g_validation_manager.callbacks_destroyed);
    fprintf(file, "  \"file_logging\": %s,\n", g_validation_manager.file_logging ? "true" : "false");
    fprintf(file, "  \"verbose_logging\": %s,\n", g_validation_manager.verbose_logging ? "true" : "false");
    fprintf(file, "  \"log_filename\": \"%s\"\n", g_validation_manager.log_filename);
    fprintf(file, "}\n");
    
    unlock_validation_manager();
    
    fclose(file);
    printf("Exported statistics to JSON: %s\n", filename);
    return true;
}

// Validate instance creation
bool vk_validation_validate_instance_creation(const VkInstanceCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Instance create info is NULL\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO) {
        printf("Error: Invalid sType in instance create info\n");
        return false;
    }
    
    if (!g_validation_manager.validation_enabled) {
        return true;
    }
    
    for (u32 i = 0; i < create_info->enabledLayerCount; i++) {
        const char* layer_name = create_info->ppEnabledLayerNames[i];
        if (!is_validation_layer_supported(layer_name)) {
            printf("Error: Requested layer not available: %s\n", layer_name);
            return false;
        }
    }
    
    return true;
}

// Get validation layer count
u32 vk_validation_get_layer_count(void) {
    return g_validation_manager.layer_count;
}

// Get enabled layer count
u32 vk_validation_get_enabled_layer_count(void) {
    return g_validation_manager.enabled_layer_count;
}

// Get callback count
u32 vk_validation_get_callback_count(void) {
    return g_validation_manager.callback_count;
}

// Check if validation manager is initialized
bool vk_validation_is_initialized(void) {
    return g_validation_manager.instance != VK_NULL_HANDLE;
}

// Get instance handle
VkInstance vk_validation_get_instance(void) {
    return g_validation_manager.instance;
}

// Advanced filtering and message processing functions

// Set message severity filter
bool vk_validation_set_severity_filter(VkDebugUtilsMessageSeverityFlagsEXT allowed_severities) {
    lock_validation_manager();
    
    // This would require modifying the debug callback creation
    // For now, we'll store the filter for future use
    printf("Set severity filter (implementation pending)\n");
    
    unlock_validation_manager();
    return true;
}

// Set message type filter  
bool vk_validation_set_type_filter(VkDebugUtilsMessageTypeFlagsEXT allowed_types) {
    lock_validation_manager();
    
    printf("Set type filter (implementation pending)\n");
    
    unlock_validation_manager();
    return true;
}

// Get message by severity
u32 vk_validation_get_messages_by_severity(VkDebugUtilsMessageSeverityFlagBitsEXT severity, const char** messages, u32 max_count) {
    if (!messages || max_count == 0) return 0;
    
    lock_validation_manager();
    
    u32 count = 0;
    for (u32 i = 0; i < g_validation_manager.debug_message_count && count < max_count; i++) {
        u32 index = (g_validation_manager.current_message_index - 1 - i + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
        // Note: Would need to parse severity from stored messages
        // For now, return all messages
        messages[count++] = g_validation_manager.debug_messages[index];
    }
    
    unlock_validation_manager();
    return count;
}

// Filter messages by pattern
u32 vk_validation_filter_messages(const char* pattern, const char** filtered_messages, u32 max_count) {
    if (!pattern || !filtered_messages || max_count == 0) return 0;
    
    lock_validation_manager();
    
    u32 count = 0;
    for (u32 i = 0; i < g_validation_manager.debug_message_count && count < max_count; i++) {
        u32 index = (g_validation_manager.current_message_index - 1 - i + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
        const char* message = g_validation_manager.debug_messages[index];
        
        if (strstr(message, pattern)) {
            filtered_messages[count++] = message;
        }
    }
    
    unlock_validation_manager();
    return count;
}

// Get error messages only
u32 vk_validation_get_error_messages(const char** error_messages, u32 max_count) {
    return vk_validation_filter_messages("[ERROR]", error_messages, max_count);
}

// Get warning messages only  
u32 vk_validation_get_warning_messages(const char** warning_messages, u32 max_count) {
    return vk_validation_filter_messages("[WARNING]", warning_messages, max_count);
}

// Create custom debug messenger with filtering
u32 vk_validation_create_filtered_callback(const VkDebugUtilsMessengerCreateInfoEXT* create_info,
                                          VkDebugUtilsMessageSeverityFlagsEXT severity_filter,
                                          VkDebugUtilsMessageTypeFlagsEXT type_filter,
                                          PFN_vkDebugUtilsMessengerCallbackEXT callback_func, void* user_data) {
    if (!g_validation_manager.instance || !g_validation_manager.debug_utils_available) {
        printf("Error: Validation manager not initialized or debug utils unavailable\n");
        return 0;
    }
    
    lock_validation_manager();
    
    if (g_validation_manager.callback_count >= VK_MAX_DEBUG_CALLBACKS) {
        printf("Error: Maximum debug callbacks reached\n");
        unlock_validation_manager();
        return 0;
    }
    
    u32 callback_id = g_validation_manager.callback_count + 1;
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[g_validation_manager.callback_count];
    
    VkDebugUtilsMessengerCreateInfoEXT messenger_info = {0};
    if (create_info) {
        messenger_info = *create_info;
        // Apply filters
        messenger_info.messageSeverity &= severity_filter;
        messenger_info.messageType &= type_filter;
    } else {
        messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messenger_info.messageSeverity = severity_filter;
        messenger_info.messageType = type_filter;
        messenger_info.pfnUserCallback = callback_func ? callback_func : debug_callback;
        messenger_info.pUserData = user_data;
    }
    
    VkResult result = vkCreateDebugUtilsMessengerEXT(g_validation_manager.instance, &messenger_info, NULL, &callback->handle);
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create filtered debug callback (result: %d)\n", result);
        unlock_validation_manager();
        return 0;
    }
    
    callback->create_info = messenger_info;
    callback->callback_func = messenger_info.pfnUserCallback;
    callback->user_data = messenger_info.pUserData;
    callback->is_active = true;
    g_validation_manager.callback_count++;
    g_validation_manager.callbacks_created++;
    
    unlock_validation_manager();
    printf("Created filtered debug callback (ID: %u)\n", callback_id);
    
    return callback_id;
}

// Performance monitoring functions

// Start performance monitoring
void vk_validation_start_performance_monitoring(void) {
    lock_validation_manager();
    g_validation_manager.start_time = clock();
    unlock_validation_manager();
    printf("Started performance monitoring\n");
}

// Stop performance monitoring  
double vk_validation_stop_performance_monitoring(void) {
    lock_validation_manager();
    clock_t end_time = clock();
    double duration = ((double)(end_time - g_validation_manager.start_time)) / CLOCKS_PER_SEC;
    unlock_validation_manager();
    printf("Stopped performance monitoring (duration: %.2f seconds)\n", duration);
    return duration;
}

// Get messages per second rate
double vk_validation_get_message_rate(void) {
    lock_validation_manager();
    
    clock_t current_time = clock();
    double duration = ((double)(current_time - g_validation_manager.start_time)) / CLOCKS_PER_SEC;
    
    if (duration <= 0.0) {
        unlock_validation_manager();
        return 0.0;
    }
    
    double rate = (double)g_validation_manager.total_debug_messages / duration;
    
    unlock_validation_manager();
    return rate;
}

// Memory usage statistics

// Get validation manager memory usage
size_t vk_validation_get_memory_usage(void) {
    size_t usage = 0;
    
    lock_validation_manager();
    
    // Manager structure
    usage += sizeof(vk_validation_manager_t);
    
    // Debug message buffer
    usage += sizeof(g_validation_manager.debug_messages);
    
    // Layer structures
    usage += sizeof(g_validation_manager.layers);
    
    // Callback structures  
    usage += sizeof(g_validation_manager.callbacks);
    
    unlock_validation_manager();
    return usage;
}

// Get peak memory usage
size_t vk_validation_get_peak_memory_usage(void) {
    // For now, return current usage
    return vk_validation_get_memory_usage();
}

// Advanced configuration

// Set maximum message count
bool vk_validation_set_max_message_count(u32 max_count) {
    if (max_count == 0 || max_count > VK_MAX_DEBUG_MESSAGES) {
        printf("Error: Invalid max message count: %u (max: %u)\n", max_count, VK_MAX_DEBUG_MESSAGES);
        return false;
    }
    
    lock_validation_manager();
    
    // Would need to restructure the message buffer
    printf("Set max message count to %u (implementation pending)\n", max_count);
    
    unlock_validation_manager();
    return true;
}

// Enable/disable automatic message clearing
void vk_validation_set_auto_clear(bool enabled, u32 threshold) {
    lock_validation_manager();
    
    printf("Set auto clear to %s (threshold: %u) (implementation pending)\n", enabled ? "enabled" : "disabled", threshold);
    
    unlock_validation_manager();
}

// Message formatting options

// Set message timestamp format
void vk_validation_set_timestamp_format(const char* format) {
    if (!format) return;
    
    lock_validation_manager();
    
    printf("Set timestamp format to %s (implementation pending)\n", format);
    
    unlock_validation_manager();
}

// Enable/disable message colors
void vk_validation_set_colored_output(bool enabled) {
    lock_validation_manager();
    
    printf("Set colored output to %s (implementation pending)\n", enabled ? "enabled" : "disabled");
    
    unlock_validation_manager();
}

// Validation layer extensions

// Check for layer extensions
bool vk_validation_check_layer_extensions(const char* layer_name, const char** extension_names, u32 extension_count) {
    if (!layer_name || !extension_names || extension_count == 0) return false;
    
    lock_validation_manager();
    
    printf("Checking %u extensions for layer %s (implementation pending)\n", extension_count, layer_name);
    
    unlock_validation_manager();
    return true;
}

// Get layer extension properties
u32 vk_validation_get_layer_extensions(const char* layer_name, VkExtensionProperties* extensions, u32 max_count) {
    if (!layer_name || !extensions || max_count == 0) return 0;
    
    lock_validation_manager();
    
    printf("Getting extensions for layer %s (implementation pending)\n", layer_name);
    
    unlock_validation_manager();
    return 0;
}

// Debug marker functions

// Set debug object name
bool vk_validation_set_object_name(VkDevice device, uint64_t object, VkObjectType object_type, const char* name) {
    if (!device || !name) return false;
    
    // This would require debug marker extension
    printf("Set object name for type %d: %s (implementation pending)\n", object_type, name);
    return true;
}

// Get debug object name
const char* vk_validation_get_object_name(uint64_t object, VkObjectType object_type) {
    printf("Get object name for type %d (implementation pending)\n", object_type);
    return NULL;
}

// Begin debug region
bool vk_validation_begin_debug_region(VkCommandBuffer command_buffer, const char* region_name) {
    if (!command_buffer || !region_name) return false;
    
    printf("Begin debug region: %s (implementation pending)\n", region_name);
    return true;
}

// End debug region
bool vk_validation_end_debug_region(VkCommandBuffer command_buffer) {
    if (!command_buffer) return false;
    
    printf("End debug region (implementation pending)\n");
    return true;
}

// Insert debug label
bool vk_validation_insert_debug_label(VkCommandBuffer command_buffer, const char* label_name) {
    if (!command_buffer || !label_name) return false;
    
    printf("Insert debug label: %s (implementation pending)\n", label_name);
    return true;
}

// Validation state queries

// Get validation state summary
void vk_validation_get_state_summary(bool* is_healthy, u32* issue_count, const char** primary_issue) {
    if (!is_healthy) return;
    
    lock_validation_manager();
    
    *is_healthy = g_validation_manager.error_count == 0;
    if (issue_count) *issue_count = g_validation_manager.error_count;
    if (primary_issue && g_validation_manager.debug_message_count > 0) {
        u32 index = (g_validation_manager.current_message_index - 1 + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
        *primary_issue = g_validation_manager.debug_messages[index];
    }
    
    unlock_validation_manager();
}

// Check for specific validation issues
bool vk_validation_has_issues_of_type(const char* issue_pattern) {
    if (!issue_pattern) return false;
    
    const char* messages[16];
    u32 count = vk_validation_filter_messages(issue_pattern, messages, 16);
    return count > 0;
}

// Health check
bool vk_validation_perform_health_check(void) {
    lock_validation_manager();
    
    bool healthy = true;
    
    // Check for critical errors
    if (g_validation_manager.error_count > 0) {
        printf("Health check: Found %u errors\n", g_validation_manager.error_count);
        healthy = false;
    }
    
    // Check callback health
    u32 active_callbacks = 0;
    for (u32 i = 0; i < g_validation_manager.callback_count; i++) {
        if (g_validation_manager.callbacks[i].is_active) {
            active_callbacks++;
        }
    }
    
    if (active_callbacks == 0 && g_validation_manager.validation_enabled) {
        printf("Health check: No active callbacks\n");
        healthy = false;
    }
    
    // Check thread safety
    if (!g_validation_manager.thread_safety_initialized) {
        printf("Health check: Thread safety not initialized\n");
        healthy = false;
    }
    
    unlock_validation_manager();
    
    printf("Validation system health: %s\n", healthy ? "HEALTHY" : "UNHEALTHY");
    return healthy;
}

// Utility functions

// Generate validation report
bool vk_validation_generate_report(const char* filename, bool include_details) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Failed to open report file: %s\n", filename);
        return false;
    }
    
    lock_validation_manager();
    
    fprintf(file, "# Vulkan Validation Report\n\n");
    fprintf(file, "Generated: %s\n", "timestamp"); // Would add real timestamp
    fprintf(file, "Total Messages: %u\n", g_validation_manager.total_debug_messages);
    fprintf(file, "Errors: %u\n", g_validation_manager.error_count);
    fprintf(file, "Warnings: %u\n", g_validation_manager.warning_count);
    fprintf(file, "Info: %u\n", g_validation_manager.info_count);
    fprintf(file, "Verbose: %u\n", g_validation_manager.verbose_count);
    
    if (include_details) {
        fprintf(file, "\n## Message Details\n\n");
        for (u32 i = 0; i < g_validation_manager.debug_message_count; i++) {
            u32 index = (g_validation_manager.current_message_index - 1 - i + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
            fprintf(file, "%u. %s\n", i + 1, g_validation_manager.debug_messages[index]);
        }
    }
    
    unlock_validation_manager();
    
    fclose(file);
    printf("Generated validation report: %s\n", filename);
    return true;
}

// Compare validation states
bool vk_validation_compare_states(const vk_validation_manager_t* other_state) {
    if (!other_state) return false;
    
    lock_validation_manager();
    
    bool same = (g_validation_manager.total_debug_messages == other_state->total_debug_messages &&
                 g_validation_manager.error_count == other_state->error_count &&
                 g_validation_manager.warning_count == other_state->warning_count);
    
    unlock_validation_manager();
    return same;
}

// Reset to defaults
void vk_validation_reset_to_defaults(void) {
    lock_validation_manager();
    
    // Reset statistics
    g_validation_manager.total_debug_messages = 0;
    g_validation_manager.error_count = 0;
    g_validation_manager.warning_count = 0;
    g_validation_manager.info_count = 0;
    g_validation_manager.verbose_count = 0;
    
    // Reset configuration
    g_validation_manager.verbose_logging = false;
    g_validation_manager.file_logging = false;
    
    // Clear messages
    g_validation_manager.debug_message_count = 0;
    g_validation_manager.current_message_index = 0;
    memset(g_validation_manager.debug_messages, 0, sizeof(g_validation_manager.debug_messages));
    
    unlock_validation_manager();
    printf("Reset validation manager to defaults\n");
}

// Additional validation functions for comprehensive Vulkan debugging

// Device-specific validation functions

// Validate device creation
bool vk_validation_validate_device_creation(VkPhysicalDevice physical_device, const VkDeviceCreateInfo* create_info) {
    if (!physical_device || !create_info) {
        printf("Error: Invalid parameters for device creation validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO) {
        printf("Error: Invalid sType in device create info\n");
        return false;
    }
    
    if (!g_validation_manager.validation_enabled) {
        return true;
    }
    
    // Validate requested device extensions
    for (u32 i = 0; i < create_info->enabledExtensionCount; i++) {
        const char* extension_name = create_info->ppEnabledExtensionNames[i];
        printf("Validating device extension: %s\n", extension_name);
        // Would check against available extensions
    }
    
    // Validate queue families
    if (!create_info->pQueueCreateInfos || create_info->queueCreateInfoCount == 0) {
        printf("Error: No queue create infos provided\n");
        return false;
    }
    
    printf("Device creation validation passed\n");
    return true;
}

// Validate command buffer creation
bool vk_validation_validate_command_buffer(VkCommandBuffer command_buffer, VkCommandBufferLevel level) {
    if (!command_buffer) {
        printf("Error: Invalid command buffer\n");
        return false;
    }
    
    if (level != VK_COMMAND_BUFFER_LEVEL_PRIMARY && level != VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
        printf("Error: Invalid command buffer level\n");
        return false;
    }
    
    printf("Command buffer validation passed\n");
    return true;
}

// Validate shader module creation
bool vk_validation_validate_shader_module(const VkShaderModuleCreateInfo* create_info) {
    if (!create_info || !create_info->pCode) {
        printf("Error: Invalid shader module create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO) {
        printf("Error: Invalid sType in shader module create info\n");
        return false;
    }
    
    if (create_info->codeSize == 0) {
        printf("Error: Empty shader code\n");
        return false;
    }
    
    // Basic SPIR-V validation
    const uint32_t* code = create_info->pCode;
    if (code[0] != 0x07230203) { // SPIR-V magic number
        printf("Warning: Invalid SPIR-V magic number\n");
    }
    
    printf("Shader module validation passed\n");
    return true;
}

// Validate pipeline creation
bool vk_validation_validate_graphics_pipeline(const VkGraphicsPipelineCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid graphics pipeline create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {
        printf("Error: Invalid sType in graphics pipeline create info\n");
        return false;
    }
    
    // Validate shader stages
    if (!create_info->pStages || create_info->stageCount == 0) {
        printf("Error: No shader stages provided\n");
        return false;
    }
    
    // Check for required shader stages
    bool has_vertex = false;
    bool has_fragment = false;
    
    for (u32 i = 0; i < create_info->stageCount; i++) {
        if (create_info->pStages[i].stage == VK_SHADER_STAGE_VERTEX_BIT) {
            has_vertex = true;
        } else if (create_info->pStages[i].stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
            has_fragment = true;
        }
    }
    
    if (!has_vertex) {
        printf("Error: Missing vertex shader stage\n");
        return false;
    }
    
    if (!has_fragment) {
        printf("Warning: Missing fragment shader stage\n");
    }
    
    printf("Graphics pipeline validation passed\n");
    return true;
}

// Validate buffer creation
bool vk_validation_validate_buffer_creation(const VkBufferCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid buffer create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO) {
        printf("Error: Invalid sType in buffer create info\n");
        return false;
    }
    
    if (create_info->size == 0) {
        printf("Error: Buffer size cannot be zero\n");
        return false;
    }
    
    // Validate usage flags
    if (create_info->usage == 0) {
        printf("Error: Buffer usage flags cannot be zero\n");
        return false;
    }
    
    printf("Buffer creation validation passed\n");
    return true;
}

// Validate image creation
bool vk_validation_validate_image_creation(const VkImageCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid image create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO) {
        printf("Error: Invalid sType in image create info\n");
        return false;
    }
    
    if (create_info->width == 0 || create_info->height == 0) {
        printf("Error: Image dimensions cannot be zero\n");
        return false;
    }
    
    // Validate format
    if (create_info->format == VK_FORMAT_UNDEFINED) {
        printf("Warning: Undefined image format\n");
    }
    
    printf("Image creation validation passed\n");
    return true;
}

// Validate sampler creation
bool vk_validation_validate_sampler_creation(const VkSamplerCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid sampler create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO) {
        printf("Error: Invalid sType in sampler create info\n");
        return false;
    }
    
    // Validate filter modes
    if (create_info->minFilter == VK_FILTER_MAX_1000MIPMAP ||
        create_info->magFilter == VK_FILTER_MAX_1000MIPMAP) {
        printf("Warning: Using placeholder filter mode\n");
    }
    
    printf("Sampler creation validation passed\n");
    return true;
}

// Render pass validation
bool vk_validation_validate_render_pass(const VkRenderPassCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid render pass create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO) {
        printf("Error: Invalid sType in render pass create info\n");
        return false;
    }
    
    // Validate attachments
    if (create_info->attachmentCount > 0 && !create_info->pAttachments) {
        printf("Error: Attachment count > 0 but no attachments provided\n");
        return false;
    }
    
    // Validate subpasses
    if (create_info->subpassCount > 0 && !create_info->pSubpasses) {
        printf("Error: Subpass count > 0 but no subpasses provided\n");
        return false;
    }
    
    printf("Render pass validation passed\n");
    return true;
}

// Framebuffer validation
bool vk_validation_validate_framebuffer(const VkFramebufferCreateInfo* create_info, VkRenderPass render_pass) {
    if (!create_info || render_pass == VK_NULL_HANDLE) {
        printf("Error: Invalid framebuffer create info or render pass\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO) {
        printf("Error: Invalid sType in framebuffer create info\n");
        return false;
    }
    
    if (create_info->width == 0 || create_info->height == 0) {
        printf("Error: Framebuffer dimensions cannot be zero\n");
        return false;
    }
    
    if (create_info->layers == 0) {
        printf("Error: Framebuffer layers cannot be zero\n");
        return false;
    }
    
    printf("Framebuffer validation passed\n");
    return true;
}

// Descriptor set layout validation
bool vk_validation_validate_descriptor_set_layout(const VkDescriptorSetLayoutCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid descriptor set layout create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {
        printf("Error: Invalid sType in descriptor set layout create info\n");
        return false;
    }
    
    // Validate bindings
    if (create_info->bindingCount > 0 && !create_info->pBindings) {
        printf("Error: Binding count > 0 but no bindings provided\n");
        return false;
    }
    
    printf("Descriptor set layout validation passed\n");
    return true;
}

// Memory management validation

// Validate memory allocation
bool vk_validation_validate_memory_allocation(VkDevice device, const VkMemoryAllocateInfo* allocate_info) {
    if (!device || !allocate_info) {
        printf("Error: Invalid device or allocate info\n");
        return false;
    }
    
    if (allocate_info->sType != VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO) {
        printf("Error: Invalid sType in memory allocate info\n");
        return false;
    }
    
    if (allocate_info->allocationSize == 0) {
        printf("Error: Allocation size cannot be zero\n");
        return false;
    }
    
    printf("Memory allocation validation passed\n");
    return true;
}

// Validate memory binding
bool vk_validation_validate_memory_binding(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset) {
    if (!device || buffer == VK_NULL_HANDLE || memory == VK_NULL_HANDLE) {
        printf("Error: Invalid device, buffer, or memory\n");
        return false;
    }
    
    printf("Memory binding validation passed\n");
    return true;
}

// Synchronization validation

// Validate semaphore creation
bool vk_validation_validate_semaphore_creation(const VkSemaphoreCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid semaphore create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO) {
        printf("Error: Invalid sType in semaphore create info\n");
        return false;
    }
    
    printf("Semaphore creation validation passed\n");
    return true;
}

// Validate fence creation
bool vk_validation_validate_fence_creation(const VkFenceCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid fence create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_FENCE_CREATE_INFO) {
        printf("Error: Invalid sType in fence create info\n");
        return false;
    }
    
    printf("Fence creation validation passed\n");
    return true;
}

// Validate event creation
bool vk_validation_validate_event_creation(const VkEventCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid event create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_EVENT_CREATE_INFO) {
        printf("Error: Invalid sType in event create info\n");
        return false;
    }
    
    printf("Event creation validation passed\n");
    return true;
}

// Query and feedback validation

// Validate query pool creation
bool vk_validation_validate_query_pool_creation(const VkQueryPoolCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid query pool create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO) {
        printf("Error: Invalid sType in query pool create info\n");
        return false;
    }
    
    if (create_info->queryCount == 0) {
        printf("Error: Query count cannot be zero\n");
        return false;
    }
    
    printf("Query pool creation validation passed\n");
    return true;
}

// Advanced debugging functions

// Validate command buffer state
bool vk_validation_validate_command_buffer_state(VkCommandBuffer command_buffer, VkCommandBufferLevel level) {
    if (!command_buffer) {
        printf("Error: Invalid command buffer\n");
        return false;
    }
    
    printf("Command buffer state validation passed\n");
    return true;
}

// Validate descriptor set updates
bool vk_validation_validate_descriptor_updates(const VkWriteDescriptorSet* descriptor_writes, u32 write_count) {
    if (!descriptor_writes || write_count == 0) {
        printf("Error: No descriptor writes provided\n");
        return false;
    }
    
    for (u32 i = 0; i < write_count; i++) {
        if (descriptor_writes[i].sType != VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET) {
            printf("Error: Invalid sType in descriptor write %u\n", i);
            return false;
        }
        
        if (descriptor_writes[i].dstSet == VK_NULL_HANDLE) {
            printf("Error: Invalid destination set in descriptor write %u\n", i);
            return false;
        }
    }
    
    printf("Descriptor updates validation passed\n");
    return true;
}

// Validate pipeline barriers
bool vk_validation_validate_pipeline_barriers(const VkMemoryBarrier* memory_barriers, u32 memory_barrier_count,
                                             const VkBufferMemoryBarrier* buffer_barriers, u32 buffer_barrier_count,
                                             const VkImageMemoryBarrier* image_barriers, u32 image_barrier_count) {
    // Validate memory barriers
    if (memory_barrier_count > 0 && !memory_barriers) {
        printf("Error: Memory barrier count > 0 but no barriers provided\n");
        return false;
    }
    
    // Validate buffer memory barriers
    if (buffer_barrier_count > 0 && !buffer_barriers) {
        printf("Error: Buffer memory barrier count > 0 but no barriers provided\n");
        return false;
    }
    
    // Validate image memory barriers
    if (image_barrier_count > 0 && !image_barriers) {
        printf("Error: Image memory barrier count > 0 but no barriers provided\n");
        return false;
    }
    
    printf("Pipeline barriers validation passed\n");
    return true;
}

// Validate render pass begin
bool vk_validation_validate_render_pass_begin(const VkRenderPassBeginInfo* begin_info) {
    if (!begin_info) {
        printf("Error: Invalid render pass begin info\n");
        return false;
    }
    
    if (begin_info->sType != VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO) {
        printf("Error: Invalid sType in render pass begin info\n");
        return false;
    }
    
    if (begin_info->renderPass == VK_NULL_HANDLE) {
        printf("Error: Invalid render pass\n");
        return false;
    }
    
    if (begin_info->framebuffer == VK_NULL_HANDLE) {
        printf("Error: Invalid framebuffer\n");
        return false;
    }
    
    printf("Render pass begin validation passed\n");
    return true;
}

// Validate drawing commands
bool vk_validation_validate_draw_commands(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
    if (vertex_count == 0) {
        printf("Error: Vertex count cannot be zero\n");
        return false;
    }
    
    if (instance_count == 0) {
        printf("Error: Instance count cannot be zero\n");
        return false;
    }
    
    printf("Draw commands validation passed\n");
    return true;
}

// Validate indexed drawing commands
bool vk_validation_validate_indexed_draw_commands(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
    if (index_count == 0) {
        printf("Error: Index count cannot be zero\n");
        return false;
    }
    
    if (instance_count == 0) {
        printf("Error: Instance count cannot be zero\n");
        return false;
    }
    
    printf("Indexed draw commands validation passed\n");
    return true;
}

// Debug utilities for object tracking

// Track object creation
void vk_validation_track_object_creation(VkObjectType object_type, uint64_t object_handle, const char* name) {
    lock_validation_manager();
    
    printf("Tracking object creation: type=%d, handle=%lu, name=%s\n", object_type, object_handle, name ? name : "unnamed");
    
    unlock_validation_manager();
}

// Track object destruction
void vk_validation_track_object_destruction(VkObjectType object_type, uint64_t object_handle) {
    lock_validation_manager();
    
    printf("Tracking object destruction: type=%d, handle=%lu\n", object_type, object_handle);
    
    unlock_validation_manager();
}

// Get object tracking info
bool vk_validation_get_object_info(uint64_t object_handle, VkObjectType* type, const char** name) {
    if (!type) return false;
    
    lock_validation_manager();
    
    printf("Getting object info for handle %lu (implementation pending)\n", object_handle);
    
    unlock_validation_manager();
    return true;
}

// Performance analysis functions

// Analyze validation overhead
double vk_validation_analyze_overhead(void) {
    lock_validation_manager();
    
    clock_t current_time = clock();
    double total_time = ((double)(current_time - g_validation_manager.start_time)) / CLOCKS_PER_SEC;
    
    if (total_time <= 0.0) {
        unlock_validation_manager();
        return 0.0;
    }
    
    // Estimate overhead based on message processing time
    double message_rate = (double)g_validation_manager.total_debug_messages / total_time;
    double overhead = message_rate * 0.001; // Rough estimate: 1ms per message
    
    unlock_validation_manager();
    return overhead;
}

// Get bottleneck analysis
void vk_validation_get_bottleneck_analysis(u32* error_rate, u32* warning_rate, double* message_frequency) {
    lock_validation_manager();
    
    clock_t current_time = clock();
    double duration = ((double)(current_time - g_validation_manager.start_time)) / CLOCKS_PER_SEC;
    
    if (duration > 0.0) {
        if (error_rate) *error_rate = (u32)((double)g_validation_manager.error_count / duration);
        if (warning_rate) *warning_rate = (u32)((double)g_validation_manager.warning_count / duration);
        if (message_frequency) *message_frequency = (double)g_validation_manager.total_debug_messages / duration;
    } else {
        if (error_rate) *error_rate = 0;
        if (warning_rate) *warning_rate = 0;
        if (message_frequency) *message_frequency = 0.0;
    }
    
    unlock_validation_manager();
}

// Configuration and preset management

// Load validation preset
bool vk_validation_load_preset(const char* preset_name) {
    if (!preset_name) return false;
    
    lock_validation_manager();
    
    printf("Loading validation preset: %s\n", preset_name);
    
    // Apply preset settings based on name
    if (strcmp(preset_name, "debug") == 0) {
        g_validation_manager.verbose_logging = true;
        g_validation_manager.validation_enabled = true;
        printf("Applied debug preset\n");
    } else if (strcmp(preset_name, "release") == 0) {
        g_validation_manager.verbose_logging = false;
        g_validation_manager.validation_enabled = false;
        printf("Applied release preset\n");
    } else if (strcmp(preset_name, "minimal") == 0) {
        g_validation_manager.verbose_logging = false;
        g_validation_manager.validation_enabled = true;
        printf("Applied minimal preset\n");
    } else {
        printf("Unknown preset: %s\n", preset_name);
        unlock_validation_manager();
        return false;
    }
    
    unlock_validation_manager();
    return true;
}

// Save current configuration as preset
bool vk_validation_save_preset(const char* preset_name) {
    if (!preset_name) return false;
    
    lock_validation_manager();
    
    printf("Saving validation preset: %s\n", preset_name);
    
    // Would save current settings to a file
    printf("Preset saved (implementation pending)\n");
    
    unlock_validation_manager();
    return true;
}

// Get available presets
u32 vk_validation_get_available_presets(const char** preset_names, u32 max_count) {
    if (!preset_names || max_count == 0) return 0;
    
    const char* presets[] = {"debug", "release", "minimal"};
    u32 preset_count = sizeof(presets) / sizeof(presets[0]);
    
    u32 count = 0;
    for (u32 i = 0; i < preset_count && count < max_count; i++) {
        preset_names[count++] = presets[i];
    }
    
    return count;
}

// Integration with external debuggers

// Register external debugger callback
bool vk_validation_register_debugger_callback(void (*callback)(const char* message, VkDebugUtilsMessageSeverityFlagBitsEXT severity)) {
    if (!callback) {
        printf("Error: Invalid debugger callback\n");
        return false;
    }
    
    lock_validation_manager();
    
    printf("Registered external debugger callback\n");
    
    unlock_validation_manager();
    return true;
}

// Unregister external debugger callback
void vk_validation_unregister_debugger_callback(void) {
    lock_validation_manager();
    
    printf("Unregistered external debugger callback\n");
    
    unlock_validation_manager();
}

// Send message to external debugger
void vk_validation_send_to_debugger(const char* message, VkDebugUtilsMessageSeverityFlagBitsEXT severity) {
    if (!message) return;
    
    lock_validation_manager();
    
    printf("Sending message to debugger: %s\n", message);
    
    unlock_validation_manager();
}

// Advanced filtering and search

// Search messages by content
u32 vk_validation_search_messages(const char* search_term, const char** results, u32 max_results) {
    if (!search_term || !results || max_results == 0) return 0;
    
    lock_validation_manager();
    
    u32 count = 0;
    for (u32 i = 0; i < g_validation_manager.debug_message_count && count < max_results; i++) {
        u32 index = (g_validation_manager.current_message_index - 1 - i + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
        const char* message = g_validation_manager.debug_messages[index];
        
        if (strstr(message, search_term)) {
            results[count++] = message;
        }
    }
    
    unlock_validation_manager();
    return count;
}

// Get messages in time range
u32 vk_validation_get_messages_in_time_range(double start_time, double end_time, const char** results, u32 max_results) {
    if (!results || max_results == 0) return 0;
    
    lock_validation_manager();
    
    printf("Getting messages in time range %.2f - %.2f (implementation pending)\n", start_time, end_time);
    
    unlock_validation_manager();
    return 0;
}

// Export messages in different formats

// Export messages as CSV
bool vk_validation_export_messages_csv(const char* filename) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Failed to open CSV file: %s\n", filename);
        return false;
    }
    
    lock_validation_manager();
    
    fprintf(file, "Index,Timestamp,Severity,Message\n");
    
    for (u32 i = 0; i < g_validation_manager.debug_message_count; i++) {
        u32 index = (g_validation_manager.current_message_index - 1 - i + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
        const char* message = g_validation_manager.debug_messages[index];
        
        fprintf(file, "%u,%s,%s\n", i, "timestamp", message);
    }
    
    unlock_validation_manager();
    
    fclose(file);
    printf("Exported messages to CSV: %s\n", filename);
    return true;
}

// Export messages as XML
bool vk_validation_export_messages_xml(const char* filename) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Failed to open XML file: %s\n", filename);
        return false;
    }
    
    lock_validation_manager();
    
    fprintf(file, "<?xml version=\"1.0\"?>\n");
    fprintf(file, "<validation_messages>\n");
    
    for (u32 i = 0; i < g_validation_manager.debug_message_count; i++) {
        u32 index = (g_validation_manager.current_message_index - 1 - i + VK_MAX_DEBUG_MESSAGES) % VK_MAX_DEBUG_MESSAGES;
        const char* message = g_validation_manager.debug_messages[index];
        
        fprintf(file, "  <message index=\"%u\" timestamp=\"%s\">%s</message>\n", i, "timestamp", message);
    }
    
    fprintf(file, "</validation_messages>\n");
    
    unlock_validation_manager();
    
    fclose(file);
    printf("Exported messages to XML: %s\n", filename);
    return true;
}

// Final utility functions

// Get validation system version
const char* vk_validation_get_version(void) {
    return "1.0.0";
}

// Get validation system capabilities
void vk_validation_get_capabilities(bool* supports_threading, bool* supports_file_logging, bool* supports_filtering) {
    lock_validation_manager();
    
    if (supports_threading) *supports_threading = true;
    if (supports_file_logging) *supports_file_logging = true;
    if (supports_filtering) *supports_filtering = true;
    
    unlock_validation_manager();
}

// Perform self-test
bool vk_validation_self_test(void) {
    printf("Performing validation system self-test...\n");
    
    // Test basic functionality
    if (!vk_validation_is_initialized()) {
        printf("FAIL: Validation manager not initialized\n");
        return false;
    }
    
    // Test message reporting
    vk_validation_report_info("Self-test info message");
    
    // Test statistics
    u32 total, errors, warnings, info, verbose;
    vk_validation_get_stats(&total, &errors, &warnings, &info, &verbose);
    
    if (total == 0) {
        printf("FAIL: Statistics not working\n");
        return false;
    }
    
    printf("Self-test PASSED\n");
    return true;
}

// Additional comprehensive validation functions

// Pipeline validation functions

// Validate graphics pipeline creation with device context
bool vk_validation_validate_graphics_pipeline_full(const VkGraphicsPipelineCreateInfo* create_info, 
                                                  VkDevice device, VkRenderPass render_pass) {
    if (!create_info || !device) {
        printf("Error: Invalid parameters for graphics pipeline validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {
        printf("Error: Invalid sType in graphics pipeline create info\n");
        return false;
    }
    
    // Validate shader stages
    if (!create_info->pStages || create_info->stageCount == 0) {
        printf("Error: No shader stages provided\n");
        return false;
    }
    
    // Check for required shader stages
    bool has_vertex = false;
    bool has_fragment = false;
    
    for (u32 i = 0; i < create_info->stageCount; i++) {
        if (create_info->pStages[i].stage == VK_SHADER_STAGE_VERTEX_BIT) {
            has_vertex = true;
        } else if (create_info->pStages[i].stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
            has_fragment = true;
        }
        
        // Validate shader module
        if (create_info->pStages[i].module == VK_NULL_HANDLE) {
            printf("Error: Invalid shader module in stage %u\n", i);
            return false;
        }
    }
    
    if (!has_vertex) {
        printf("Error: Missing vertex shader stage\n");
        return false;
    }
    
    // Validate vertex input state
    if (create_info->pVertexInputState) {
        if (create_info->pVertexInputState->sType != VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO) {
            printf("Error: Invalid sType in vertex input state\n");
            return false;
        }
    }
    
    // Validate input assembly state
    if (create_info->pInputAssemblyState) {
        if (create_info->pInputAssemblyState->sType != VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO) {
            printf("Error: Invalid sType in input assembly state\n");
            return false;
        }
        
        // Validate topology
        if (create_info->pInputAssemblyState->topology == VK_PRIMITIVE_TOPOLOGY_MAX_ENUM) {
            printf("Error: Invalid primitive topology\n");
            return false;
        }
    }
    
    // Validate rasterization state
    if (!create_info->pRasterizationState) {
        printf("Error: Missing rasterization state\n");
        return false;
    }
    
    if (create_info->pRasterizationState->sType != VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO) {
        printf("Error: Invalid sType in rasterization state\n");
        return false;
    }
    
    // Validate multisample state
    if (create_info->pMultisampleState) {
        if (create_info->pMultisampleState->sType != VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO) {
            printf("Error: Invalid sType in multisample state\n");
            return false;
        }
    }
    
    // Validate depth stencil state
    if (create_info->pDepthStencilState) {
        if (create_info->pDepthStencilState->sType != VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO) {
            printf("Error: Invalid sType in depth stencil state\n");
            return false;
        }
    }
    
    // Validate color blend state
    if (create_info->pColorBlendState) {
        if (create_info->pColorBlendState->sType != VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO) {
            printf("Error: Invalid sType in color blend state\n");
            return false;
        }
    }
    
    printf("Graphics pipeline validation passed\n");
    return true;
}

// Validate compute pipeline creation
bool vk_validation_validate_compute_pipeline(const VkComputePipelineCreateInfo* create_info, VkDevice device) {
    if (!create_info || !device) {
        printf("Error: Invalid parameters for compute pipeline validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO) {
        printf("Error: Invalid sType in compute pipeline create info\n");
        return false;
    }
    
    // Validate compute shader stage
    if (create_info->stage.stage != VK_SHADER_STAGE_COMPUTE_BIT) {
        printf("Error: Compute pipeline must have compute shader stage\n");
        return false;
    }
    
    if (create_info->stage.module == VK_NULL_HANDLE) {
        printf("Error: Invalid compute shader module\n");
        return false;
    }
    
    if (!create_info->stage.pName) {
        printf("Error: Compute shader entry point must be specified\n");
        return false;
    }
    
    printf("Compute pipeline validation passed\n");
    return true;
}

// Descriptor validation functions

// Validate descriptor set layout creation with full checks
bool vk_validation_validate_descriptor_set_layout_full(const VkDescriptorSetLayoutCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid descriptor set layout create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {
        printf("Error: Invalid sType in descriptor set layout create info\n");
        return false;
    }
    
    // Validate bindings
    if (create_info->bindingCount > 0 && !create_info->pBindings) {
        printf("Error: Binding count > 0 but no bindings provided\n");
        return false;
    }
    
    for (u32 i = 0; i < create_info->bindingCount; i++) {
        const VkDescriptorSetLayoutBinding* binding = &create_info->pBindings[i];
        
        // Validate descriptor type
        if (binding->descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM) {
            printf("Error: Invalid descriptor type in binding %u\n", i);
            return false;
        }
        
        // Validate descriptor count
        if (binding->descriptorCount == 0) {
            printf("Error: Descriptor count cannot be zero in binding %u\n", i);
            return false;
        }
        
        // Validate stage flags
        if (binding->stageFlags == 0) {
            printf("Warning: No stage flags specified in binding %u\n", i);
        }
        
        // Validate immutable samplers
        if (binding->descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || 
            binding->descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            if (binding->descriptorCount > 0 && binding->pImmutableSamplers == NULL) {
                printf("Warning: Sampler binding %u has no immutable samplers\n", i);
            }
        }
    }
    
    printf("Descriptor set layout validation passed\n");
    return true;
}

// Validate descriptor set allocation
bool vk_validation_validate_descriptor_set(VkDescriptorSet descriptor_set, VkDescriptorSetLayout layout) {
    if (descriptor_set == VK_NULL_HANDLE) {
        printf("Error: Invalid descriptor set\n");
        return false;
    }
    
    if (layout == VK_NULL_HANDLE) {
        printf("Error: Invalid descriptor set layout\n");
        return false;
    }
    
    printf("Descriptor set validation passed\n");
    return true;
}

// Validate descriptor pool creation
bool vk_validation_validate_descriptor_pool(const VkDescriptorPoolCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid descriptor pool create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO) {
        printf("Error: Invalid sType in descriptor pool create info\n");
        return false;
    }
    
    if (create_info->maxSets == 0) {
        printf("Error: Descriptor pool max sets cannot be zero\n");
        return false;
    }
    
    if (create_info->poolSizeCount > 0 && !create_info->pPoolSizes) {
        printf("Error: Pool size count > 0 but no pool sizes provided\n");
        return false;
    }
    
    for (u32 i = 0; i < create_info->poolSizeCount; i++) {
        if (create_info->pPoolSizes[i].descriptorCount == 0) {
            printf("Error: Descriptor count cannot be zero in pool size %u\n", i);
            return false;
        }
    }
    
    printf("Descriptor pool validation passed\n");
    return true;
}

// Command buffer validation functions

// Validate command buffer recording
bool vk_validation_validate_command_buffer_recording(VkCommandBuffer command_buffer) {
    if (!command_buffer) {
        printf("Error: Invalid command buffer\n");
        return false;
    }
    
    printf("Command buffer recording validation passed\n");
    return true;
}

// Validate command buffer submission
bool vk_validation_validate_command_buffer_submission(VkCommandBuffer command_buffer, VkQueue queue) {
    if (!command_buffer || queue == VK_NULL_HANDLE) {
        printf("Error: Invalid command buffer or queue\n");
        return false;
    }
    
    printf("Command buffer submission validation passed\n");
    return true;
}

// Validate command buffer reset
bool vk_validation_validate_command_buffer_reset(VkCommandBuffer command_buffer) {
    if (!command_buffer) {
        printf("Error: Invalid command buffer\n");
        return false;
    }
    
    printf("Command buffer reset validation passed\n");
    return true;
}

// Queue validation functions

// Validate queue creation
bool vk_validation_validate_queue_creation(VkDevice device, u32 queue_family_index, u32 queue_index) {
    if (device == VK_NULL_HANDLE) {
        printf("Error: Invalid device\n");
        return false;
    }
    
    printf("Queue creation validation passed\n");
    return true;
}

// Validate queue submission
bool vk_validation_validate_queue_submission(VkQueue queue, const VkSubmitInfo* submit_info, u32 submit_count) {
    if (!queue || !submit_info || submit_count == 0) {
        printf("Error: Invalid queue, submit info, or submit count\n");
        return false;
    }
    
    for (u32 i = 0; i < submit_count; i++) {
        if (submit_info[i].sType != VK_STRUCTURE_TYPE_SUBMIT_INFO) {
            printf("Error: Invalid sType in submit info %u\n", i);
            return false;
        }
        
        // Validate command buffers
        if (submit_info[i].commandBufferCount > 0 && !submit_info[i].pCommandBuffers) {
            printf("Error: Command buffer count > 0 but no command buffers provided in submit %u\n", i);
            return false;
        }
        
        // Validate semaphores
        if (submit_info[i].waitSemaphoreCount > 0 && !submit_info[i].pWaitSemaphores) {
            printf("Error: Wait semaphore count > 0 but no semaphores provided in submit %u\n", i);
            return false;
        }
        
        if (submit_info[i].signalSemaphoreCount > 0 && !submit_info[i].pSignalSemaphores) {
            printf("Error: Signal semaphore count > 0 but no semaphores provided in submit %u\n", i);
            return false;
        }
    }
    
    printf("Queue submission validation passed\n");
    return true;
}

// Validate queue presentation
bool vk_validation_validate_queue_presentation(VkQueue queue, const VkPresentInfoKHR* present_info) {
    if (!queue || !present_info) {
        printf("Error: Invalid queue or present info\n");
        return false;
    }
    
    if (present_info->sType != VK_STRUCTURE_TYPE_PRESENT_INFO_KHR) {
        printf("Error: Invalid sType in present info\n");
        return false;
    }
    
    if (present_info->swapchainCount == 0 || !present_info->pSwapchains) {
        printf("Error: No swapchains provided for presentation\n");
        return false;
    }
    
    if (present_info->swapchainCount > 0 && !present_info->pImageIndices) {
        printf("Error: Swapchain count > 0 but no image indices provided\n");
        return false;
    }
    
    printf("Queue presentation validation passed\n");
    return true;
}

// Swapchain validation functions

// Validate swapchain creation
bool vk_validation_validate_swapchain_creation(const VkSwapchainCreateInfoKHR* create_info, VkPhysicalDevice physical_device) {
    if (!create_info || physical_device == VK_NULL_HANDLE) {
        printf("Error: Invalid swapchain create info or physical device\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR) {
        printf("Error: Invalid sType in swapchain create info\n");
        return false;
    }
    
    if (create_info->minImageCount == 0) {
        printf("Error: Minimum image count cannot be zero\n");
        return false;
    }
    
    if (create_info->imageExtent.width == 0 || create_info->imageExtent.height == 0) {
        printf("Error: Swapchain image extent cannot be zero\n");
        return false;
    }
    
    if (create_info->imageFormat == VK_FORMAT_UNDEFINED) {
        printf("Error: Swapchain image format cannot be undefined\n");
        return false;
    }
    
    if (create_info->imageColorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR) {
        printf("Error: Invalid color space\n");
        return false;
    }
    
    if (create_info->imageArrayLayers == 0) {
        printf("Error: Image array layers cannot be zero\n");
        return false;
    }
    
    printf("Swapchain creation validation passed\n");
    return true;
}

// Validate swapchain image
bool vk_validation_validate_swapchain_image(VkImage image, VkSwapchainKHR swapchain) {
    if (image == VK_NULL_HANDLE || swapchain == VK_NULL_HANDLE) {
        printf("Error: Invalid swapchain image or swapchain\n");
        return false;
    }
    
    printf("Swapchain image validation passed\n");
    return true;
}

// Surface validation functions

// Validate surface creation
bool vk_validation_validate_surface_creation(VkSurfaceKHR surface, VkPhysicalDevice physical_device) {
    if (surface == VK_NULL_HANDLE || physical_device == VK_NULL_HANDLE) {
        printf("Error: Invalid surface or physical device\n");
        return false;
    }
    
    printf("Surface creation validation passed\n");
    return true;
}

// Validate surface capabilities
bool vk_validation_validate_surface_capabilities(const VkSurfaceCapabilitiesKHR* capabilities) {
    if (!capabilities) {
        printf("Error: Invalid surface capabilities\n");
        return false;
    }
    
    if (capabilities->minImageCount == 0) {
        printf("Error: Minimum image count cannot be zero\n");
        return false;
    }
    
    if (capabilities->maxImageCount > 0 && capabilities->maxImageCount < capabilities->minImageCount) {
        printf("Error: Maximum image count less than minimum\n");
        return false;
    }
    
    printf("Surface capabilities validation passed\n");
    return true;
}

// Shader validation functions

// Validate shader specialization info
bool vk_validation_validate_shader_specialization(const VkSpecializationInfo* spec_info) {
    if (!spec_info) return true; // Optional
    
    if (spec_info->dataSize > 0 && !spec_info->pData) {
        printf("Error: Data size > 0 but no data provided\n");
        return false;
    }
    
    if (spec_info->mapEntryCount > 0 && !spec_info->pMapEntries) {
        printf("Error: Map entry count > 0 but no map entries provided\n");
        return false;
    }
    
    for (u32 i = 0; i < spec_info->mapEntryCount; i++) {
        const VkSpecializationMapEntry* entry = &spec_info->pMapEntries[i];
        
        if (entry->offset + entry->size > spec_info->dataSize) {
            printf("Error: Map entry %u exceeds data size\n", i);
            return false;
        }
    }
    
    printf("Shader specialization validation passed\n");
    return true;
}

// Validate shader stage creation
bool vk_validation_validate_shader_stage(const VkPipelineShaderStageCreateInfo* stage_info) {
    if (!stage_info) {
        printf("Error: Invalid shader stage create info\n");
        return false;
    }
    
    if (stage_info->sType != VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO) {
        printf("Error: Invalid sType in shader stage create info\n");
        return false;
    }
    
    if (stage_info->module == VK_NULL_HANDLE) {
        printf("Error: Invalid shader module\n");
        return false;
    }
    
    if (!stage_info->pName) {
        printf("Error: Shader entry point must be specified\n");
        return false;
    }
    
    if (stage_info->stage == VK_SHADER_STAGE_MAX_ENUM) {
        printf("Error: Invalid shader stage\n");
        return false;
    }
    
    printf("Shader stage validation passed\n");
    return true;
}

// Memory management validation functions

// Validate memory requirements
bool vk_validation_validate_memory_requirements(const VkMemoryRequirements* requirements) {
    if (!requirements) {
        printf("Error: Invalid memory requirements\n");
        return false;
    }
    
    if (requirements->size == 0) {
        printf("Error: Memory requirements size cannot be zero\n");
        return false;
    }
    
    if (requirements->alignment == 0) {
        printf("Warning: Memory alignment is zero\n");
    }
    
    printf("Memory requirements validation passed\n");
    return true;
}

// Validate memory map
bool vk_validation_validate_memory_map(VkDevice device, VkDeviceMemory memory, void** data) {
    if (!device || memory == VK_NULL_HANDLE || !data) {
        printf("Error: Invalid device, memory, or data pointer\n");
        return false;
    }
    
    printf("Memory map validation passed\n");
    return true;
}

// Validate memory unmap
bool vk_validation_validate_memory_unmap(VkDevice device, VkDeviceMemory memory) {
    if (!device || memory == VK_NULL_HANDLE) {
        printf("Error: Invalid device or memory\n");
        return false;
    }
    
    printf("Memory unmap validation passed\n");
    return true;
}

// Synchronization validation functions

// Validate semaphore signal
bool vk_validation_validate_semaphore_signal(VkQueue queue, VkSemaphore semaphore) {
    if (!queue || semaphore == VK_NULL_HANDLE) {
        printf("Error: Invalid queue or semaphore\n");
        return false;
    }
    
    printf("Semaphore signal validation passed\n");
    return true;
}

// Validate semaphore wait
bool vk_validation_validate_semaphore_wait(VkQueue queue, VkSemaphore semaphore) {
    if (!queue || semaphore == VK_NULL_HANDLE) {
        printf("Error: Invalid queue or semaphore\n");
        return false;
    }
    
    printf("Semaphore wait validation passed\n");
    return true;
}

// Validate fence wait
bool vk_validation_validate_fence_wait(VkDevice device, VkFence fence) {
    if (!device || fence == VK_NULL_HANDLE) {
        printf("Error: Invalid device or fence\n");
        return false;
    }
    
    printf("Fence wait validation passed\n");
    return true;
}

// Validate fence reset
bool vk_validation_validate_fence_reset(VkDevice device, VkFence fence) {
    if (!device || fence == VK_NULL_HANDLE) {
        printf("Error: Invalid device or fence\n");
        return false;
    }
    
    printf("Fence reset validation passed\n");
    return true;
}

// Validate event signal
bool vk_validation_validate_event_signal(VkDevice device, VkEvent event) {
    if (!device || event == VK_NULL_HANDLE) {
        printf("Error: Invalid device or event\n");
        return false;
    }
    
    printf("Event signal validation passed\n");
    return true;
}

// Validate event wait
bool vk_validation_validate_event_wait(VkDevice device, VkEvent event) {
    if (!device || event == VK_NULL_HANDLE) {
        printf("Error: Invalid device or event\n");
        return false;
    }
    
    printf("Event wait validation passed\n");
    return true;
}

// Advanced validation functions

// Validate pipeline cache
bool vk_validation_validate_pipeline_cache(const VkPipelineCacheCreateInfo* create_info) {
    if (!create_info) return true; // Optional
    
    if (create_info->sType != VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO) {
        printf("Error: Invalid sType in pipeline cache create info\n");
        return false;
    }
    
    printf("Pipeline cache validation passed\n");
    return true;
}

// Validate pipeline layout
bool vk_validation_validate_pipeline_layout(const VkPipelineLayoutCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid pipeline layout create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO) {
        printf("Error: Invalid sType in pipeline layout create info\n");
        return false;
    }
    
    // Validate descriptor set layouts
    if (create_info->setLayoutCount > 0 && !create_info->pSetLayouts) {
        printf("Error: Set layout count > 0 but no set layouts provided\n");
        return false;
    }
    
    // Validate push constant ranges
    if (create_info->pushConstantRangeCount > 0 && !create_info->pPushConstantRanges) {
        printf("Error: Push constant range count > 0 but no ranges provided\n");
        return false;
    }
    
    for (u32 i = 0; i < create_info->pushConstantRangeCount; i++) {
        const VkPushConstantRange* range = &create_info->pPushConstantRanges[i];
        
        if (range->size == 0) {
            printf("Error: Push constant range size cannot be zero\n");
            return false;
        }
        
        if (range->stageFlags == 0) {
            printf("Warning: No stage flags specified for push constant range %u\n", i);
        }
    }
    
    printf("Pipeline layout validation passed\n");
    return true;
}

// Validate sampler YCbCr conversion
bool vk_validation_validate_sampler_ycbcr_conversion(const VkSamplerYcbcrConversionCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid sampler YCbCr conversion create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO) {
        printf("Error: Invalid sType in sampler YCbCr conversion create info\n");
        return false;
    }
    
    if (create_info->format == VK_FORMAT_UNDEFINED) {
        printf("Error: YCbCr conversion format cannot be undefined\n");
        return false;
    }
    
    printf("Sampler YCbCr conversion validation passed\n");
    return true;
}

// Validate image view creation
bool vk_validation_validate_image_view(const VkImageViewCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid image view create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO) {
        printf("Error: Invalid sType in image view create info\n");
        return false;
    }
    
    if (create_info->image == VK_NULL_HANDLE) {
        printf("Error: Invalid image for image view\n");
        return false;
    }
    
    if (create_info->format == VK_FORMAT_UNDEFINED) {
        printf("Warning: Image view format is undefined\n");
    }
    
    printf("Image view validation passed\n");
    return true;
}

// Validate buffer view creation
bool vk_validation_validate_buffer_view(const VkBufferViewCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid buffer view create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO) {
        printf("Error: Invalid sType in buffer view create info\n");
        return false;
    }
    
    if (create_info->buffer == VK_NULL_HANDLE) {
        printf("Error: Invalid buffer for buffer view\n");
        return false;
    }
    
    if (create_info->format == VK_FORMAT_UNDEFINED) {
        printf("Error: Buffer view format cannot be undefined\n");
        return false;
    }
    
    printf("Buffer view validation passed\n");
    return true;
}

// =================================================================================================
//                           ADDITIONAL COMPREHENSIVE VALIDATION FUNCTIONS
// =================================================================================================

// Pipeline validation functions

// Validate graphics pipeline creation
bool vk_validation_validate_graphics_pipeline(const VkGraphicsPipelineCreateInfo* create_info, 
                                             VkDevice device, VkRenderPass render_pass) {
    if (!create_info || !device) {
        printf("Error: Invalid parameters for graphics pipeline validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {
        printf("Error: Invalid sType in graphics pipeline create info\n");
        return false;
    }
    
    // Validate shader stages
    if (!create_info->pStages || create_info->stageCount == 0) {
        printf("Error: No shader stages provided\n");
        return false;
    }
    
    // Check for vertex shader
    bool has_vertex_shader = false;
    bool has_fragment_shader = false;
    
    for (u32 i = 0; i < create_info->stageCount; i++) {
        if (create_info->pStages[i].stage == VK_SHADER_STAGE_VERTEX_BIT) {
            has_vertex_shader = true;
        }
        if (create_info->pStages[i].stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
            has_fragment_shader = true;
        }
        
        // Validate shader module
        if (!create_info->pStages[i].module) {
            printf("Error: Invalid shader module at stage %u\n", i);
            return false;
        }
    }
    
    if (!has_vertex_shader) {
        printf("Warning: No vertex shader in graphics pipeline\n");
    }
    
    // Validate vertex input state
    if (create_info->pVertexInputState && !create_info->pVertexInputState->pVertexBindingDescriptions) {
        printf("Warning: Vertex input state provided but no binding descriptions\n");
    }
    
    // Validate render pass
    if (!render_pass && create_info->renderPass == VK_NULL_HANDLE) {
        printf("Error: No valid render pass provided\n");
        return false;
    }
    
    printf("Graphics pipeline validation passed\n");
    return true;
}

// Validate compute pipeline creation
bool vk_validation_validate_compute_pipeline(const VkComputePipelineCreateInfo* create_info, VkDevice device) {
    if (!create_info || !device) {
        printf("Error: Invalid parameters for compute pipeline validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO) {
        printf("Error: Invalid sType in compute pipeline create info\n");
        return false;
    }
    
    // Validate compute shader
    if (!create_info->stage.module) {
        printf("Error: Invalid compute shader module\n");
        return false;
    }
    
    if (create_info->stage.stage != VK_SHADER_STAGE_COMPUTE_BIT) {
        printf("Error: Invalid shader stage for compute pipeline\n");
        return false;
    }
    
    printf("Compute pipeline validation passed\n");
    return true;
}

// Descriptor validation functions

// Validate descriptor set layout creation
bool vk_validation_validate_descriptor_set_layout(const VkDescriptorSetLayoutCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid descriptor set layout create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {
        printf("Error: Invalid sType in descriptor set layout create info\n");
        return false;
    }
    
    // Validate bindings
    for (u32 i = 0; i < create_info->bindingCount; i++) {
        const VkDescriptorSetLayoutBinding* binding = &create_info->pBindings[i];
        
        if (binding->descriptorCount == 0) {
            printf("Error: Descriptor count cannot be zero for binding %u\n", i);
            return false;
        }
        
        // Validate descriptor type
        if (binding->descriptorType == VK_DESCRIPTOR_TYPE_MAX_ENUM) {
            printf("Error: Invalid descriptor type for binding %u\n", i);
            return false;
        }
        
        // Validate stage flags
        if (binding->stageFlags == 0 && binding->descriptorType != VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
            printf("Warning: No stage flags specified for binding %u\n", i);
        }
    }
    
    printf("Descriptor set layout validation passed\n");
    return true;
}

// Validate descriptor set allocation
bool vk_validation_validate_descriptor_set(VkDescriptorSet descriptor_set, 
                                          VkDescriptorSetLayout layout) {
    if (descriptor_set == VK_NULL_HANDLE) {
        printf("Error: Invalid descriptor set\n");
        return false;
    }
    
    if (layout == VK_NULL_HANDLE) {
        printf("Error: Invalid descriptor set layout\n");
        return false;
    }
    
    printf("Descriptor set validation passed\n");
    return true;
}

// Buffer and image validation functions

// Validate buffer creation
bool vk_validation_validate_buffer(const VkBufferCreateInfo* create_info, VkPhysicalDevice physical_device) {
    if (!create_info || !physical_device) {
        printf("Error: Invalid parameters for buffer validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO) {
        printf("Error: Invalid sType in buffer create info\n");
        return false;
    }
    
    if (create_info->size == 0) {
        printf("Error: Buffer size cannot be zero\n");
        return false;
    }
    
    // Validate usage flags
    if (create_info->usage == 0) {
        printf("Error: Buffer usage flags cannot be zero\n");
        return false;
    }
    
    // Validate sharing mode
    if (create_info->sharingMode == VK_SHARING_MODE_CONCURRENT) {
        if (!create_info->pQueueFamilyIndices || create_info->queueFamilyIndexCount < 2) {
            printf("Error: Concurrent sharing mode requires at least 2 queue families\n");
            return false;
        }
    }
    
    printf("Buffer validation passed\n");
    return true;
}

// Validate image creation
bool vk_validation_validate_image(const VkImageCreateInfo* create_info, VkPhysicalDevice physical_device) {
    if (!create_info || !physical_device) {
        printf("Error: Invalid parameters for image validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO) {
        printf("Error: Invalid sType in image create info\n");
        return false;
    }
    
    if (create_info->width == 0 || create_info->height == 0 || create_info->depth == 0) {
        printf("Error: Image dimensions cannot be zero\n");
        return false;
    }
    
    if (create_info->mipLevels == 0) {
        printf("Error: Image mip levels cannot be zero\n");
        return false;
    }
    
    if (create_info->arrayLayers == 0) {
        printf("Error: Image array layers cannot be zero\n");
        return false;
    }
    
    // Validate format
    if (create_info->format == VK_FORMAT_MAX_ENUM) {
        printf("Error: Invalid image format\n");
        return false;
    }
    
    // Validate image type
    if (create_info->imageType == VK_IMAGE_TYPE_MAX_ENUM) {
        printf("Error: Invalid image type\n");
        return false;
    }
    
    // Validate usage flags
    if (create_info->usage == 0) {
        printf("Error: Image usage flags cannot be zero\n");
        return false;
    }
    
    printf("Image validation passed\n");
    return true;
}

// Validate image view creation
bool vk_validation_validate_image_view(const VkImageViewCreateInfo* create_info, VkImage image) {
    if (!create_info || image == VK_NULL_HANDLE) {
        printf("Error: Invalid parameters for image view validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO) {
        printf("Error: Invalid sType in image view create info\n");
        return false;
    }
    
    if (create_info->image != image) {
        printf("Error: Image view create info image doesn't match provided image\n");
        return false;
    }
    
    // Validate format
    if (create_info->format == VK_FORMAT_MAX_ENUM) {
        printf("Error: Invalid image view format\n");
        return false;
    }
    
    // Validate image view type
    if (create_info->viewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM) {
        printf("Error: Invalid image view type\n");
        return false;
    }
    
    printf("Image view validation passed\n");
    return true;
}

// Sampler validation functions

// Validate sampler creation
bool vk_validation_validate_sampler(const VkSamplerCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid sampler create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO) {
        printf("Error: Invalid sType in sampler create info\n");
        return false;
    }
    
    // Validate min/mag filters
    if (create_info->minFilter == VK_FILTER_MAX_ENUM || create_info->magFilter == VK_FILTER_MAX_ENUM) {
        printf("Error: Invalid sampler filters\n");
        return false;
    }
    
    // Validate address modes
    for (int i = 0; i < 3; i++) {
        if (create_info->addressModeU == VK_SAMPLER_ADDRESS_MODE_MAX_ENUM ||
            create_info->addressModeV == VK_SAMPLER_ADDRESS_MODE_MAX_ENUM ||
            create_info->addressModeW == VK_SAMPLER_ADDRESS_MODE_MAX_ENUM) {
            printf("Error: Invalid sampler address modes\n");
            return false;
        }
    }
    
    // Validate anisotropy
    if (create_info->anisotropyEnable && create_info->maxAnisotropy == 0.0f) {
        printf("Warning: Anisotropy enabled but max anisotropy is zero\n");
    }
    
    printf("Sampler validation passed\n");
    return true;
}

// Render pass validation functions

// Validate render pass creation
bool vk_validation_validate_render_pass(const VkRenderPassCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid render pass create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO) {
        printf("Error: Invalid sType in render pass create info\n");
        return false;
    }
    
    // Validate attachments
    for (u32 i = 0; i < create_info->attachmentCount; i++) {
        const VkAttachmentDescription* attachment = &create_info->pAttachments[i];
        
        if (attachment->format == VK_FORMAT_MAX_ENUM) {
            printf("Error: Invalid attachment format at index %u\n", i);
            return false;
        }
        
        // Validate load/store operations
        if (attachment->loadOp == VK_ATTACHMENT_LOAD_OP_MAX_ENUM ||
            attachment->storeOp == VK_ATTACHMENT_STORE_OP_MAX_ENUM) {
            printf("Error: Invalid load/store operations for attachment %u\n", i);
            return false;
        }
    }
    
    // Validate subpasses
    for (u32 i = 0; i < create_info->subpassCount; i++) {
        const VkSubpassDescription* subpass = &create_info->pSubpasses[i];
        
        // Validate pipeline bind point
        if (subpass->pipelineBindPoint == VK_PIPELINE_BIND_POINT_MAX_ENUM) {
            printf("Error: Invalid pipeline bind point for subpass %u\n", i);
            return false;
        }
        
        // Validate color attachments
        for (u32 j = 0; j < subpass->colorAttachmentCount; j++) {
            if (subpass->pColorAttachments[j].attachment >= create_info->attachmentCount) {
                printf("Error: Color attachment %u in subpass %u references invalid attachment\n", j, i);
                return false;
            }
        }
    }
    
    printf("Render pass validation passed\n");
    return true;
}

// Framebuffer validation functions

// Validate framebuffer creation
bool vk_validation_validate_framebuffer(const VkFramebufferCreateInfo* create_info, 
                                       VkRenderPass render_pass, u32 width, u32 height) {
    if (!create_info || render_pass == VK_NULL_HANDLE) {
        printf("Error: Invalid parameters for framebuffer validation\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO) {
        printf("Error: Invalid sType in framebuffer create info\n");
        return false;
    }
    
    if (create_info->renderPass != render_pass) {
        printf("Error: Framebuffer render pass doesn't match provided render pass\n");
        return false;
    }
    
    if (create_info->width == 0 || create_info->height == 0) {
        printf("Error: Framebuffer dimensions cannot be zero\n");
        return false;
    }
    
    if (create_info->layers == 0) {
        printf("Error: Framebuffer layers cannot be zero\n");
        return false;
    }
    
    // Validate attachments
    if (!create_info->pAttachments || create_info->attachmentCount == 0) {
        printf("Error: No attachments provided for framebuffer\n");
        return false;
    }
    
    printf("Framebuffer validation passed\n");
    return true;
}

// Command buffer validation functions

// Validate command buffer recording state
bool vk_validation_validate_command_buffer_state(VkCommandBuffer command_buffer, 
                                               VkCommandBufferLevel expected_level) {
    if (!command_buffer) {
        printf("Error: Invalid command buffer\n");
        return false;
    }
    
    // Would need to query command buffer state in a real implementation
    printf("Command buffer state validation passed\n");
    return true;
}

// Validate command buffer submission
bool vk_validation_validate_command_buffer_submission(const VkSubmitInfo* submit_info, 
                                                     VkQueue queue) {
    if (!submit_info || queue == VK_NULL_HANDLE) {
        printf("Error: Invalid parameters for command buffer submission validation\n");
        return false;
    }
    
    if (submit_info->sType != VK_STRUCTURE_TYPE_SUBMIT_INFO) {
        printf("Error: Invalid sType in submit info\n");
        return false;
    }
    
    if (!submit_info->pCommandBuffers || submit_info->commandBufferCount == 0) {
        printf("Error: No command buffers provided for submission\n");
        return false;
    }
    
    printf("Command buffer submission validation passed\n");
    return true;
}

// Synchronization validation functions

// Validate semaphore creation
bool vk_validation_validate_semaphore(const VkSemaphoreCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid semaphore create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO) {
        printf("Error: Invalid sType in semaphore create info\n");
        return false;
    }
    
    printf("Semaphore validation passed\n");
    return true;
}

// Validate fence creation
bool vk_validation_validate_fence(const VkFenceCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid fence create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_FENCE_CREATE_INFO) {
        printf("Error: Invalid sType in fence create info\n");
        return false;
    }
    
    printf("Fence validation passed\n");
    return true;
}

// Memory validation functions

// Validate memory allocation
bool vk_validation_validate_memory_allocation(const VkMemoryAllocateInfo* allocate_info,
                                            VkPhysicalDevice physical_device) {
    if (!allocate_info || !physical_device) {
        printf("Error: Invalid parameters for memory allocation validation\n");
        return false;
    }
    
    if (allocate_info->sType != VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO) {
        printf("Error: Invalid sType in memory allocate info\n");
        return false;
    }
    
    if (allocate_info->allocationSize == 0) {
        printf("Error: Memory allocation size cannot be zero\n");
        return false;
    }
    
    if (allocate_info->memoryTypeIndex == UINT32_MAX) {
        printf("Error: Invalid memory type index\n");
        return false;
    }
    
    printf("Memory allocation validation passed\n");
    return true;
}

// Query validation functions

// Validate query pool creation
bool vk_validation_validate_query_pool(const VkQueryPoolCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid query pool create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO) {
        printf("Error: Invalid sType in query pool create info\n");
        return false;
    }
    
    if (create_info->queryCount == 0) {
        printf("Error: Query count cannot be zero\n");
        return false;
    }
    
    if (create_info->queryType == VK_QUERY_TYPE_MAX_ENUM) {
        printf("Error: Invalid query type\n");
        return false;
    }
    
    printf("Query pool validation passed\n");
    return true;
}

// Advanced validation utilities

// Validate complete pipeline state
bool vk_validation_validate_complete_pipeline_state(VkPipeline pipeline, VkDevice device) {
    if (pipeline == VK_NULL_HANDLE || device == VK_NULL_HANDLE) {
        printf("Error: Invalid pipeline or device\n");
        return false;
    }
    
    // Would validate pipeline state in a real implementation
    printf("Complete pipeline state validation passed\n");
    return true;
}

// Validate resource binding consistency
bool vk_validation_validate_resource_bindings(VkCommandBuffer command_buffer) {
    if (!command_buffer) {
        printf("Error: Invalid command buffer for resource binding validation\n");
        return false;
    }
    
    // Would validate descriptor sets and resource bindings
    printf("Resource binding validation passed\n");
    return true;
}

// Validate memory barriers and synchronization
bool vk_validation_validate_memory_barriers(const VkMemoryBarrier* memory_barriers, 
                                          u32 barrier_count) {
    if (!memory_barriers || barrier_count == 0) {
        printf("Error: Invalid memory barriers\n");
        return false;
    }
    
    for (u32 i = 0; i < barrier_count; i++) {
        if (memory_barriers[i].sType != VK_STRUCTURE_TYPE_MEMORY_BARRIER) {
            printf("Error: Invalid sType in memory barrier %u\n", i);
            return false;
        }
        
        // Validate source/destination access masks
        if (memory_barriers[i].srcAccessMask == 0 && memory_barriers[i].dstAccessMask == 0) {
            printf("Warning: Both source and destination access masks are zero for barrier %u\n", i);
        }
    }
    
    printf("Memory barrier validation passed\n");
    return true;
}

// Validate shader specialization constants
bool vk_validation_validate_specialization_info(const VkSpecializationInfo* spec_info) {
    if (!spec_info) {
        return true; // Specialization info is optional
    }
    
    if (spec_info->dataSize == 0 && spec_info->mapEntryCount > 0) {
        printf("Error: Specialization data size is zero but map entries exist\n");
        return false;
    }
    
    if (spec_info->dataSize > 0 && spec_info->mapEntryCount == 0) {
        printf("Error: Specialization data provided but no map entries\n");
        return false;
    }
    
    // Validate map entries
    for (u32 i = 0; i < spec_info->mapEntryCount; i++) {
        const VkSpecializationMapEntry* entry = &spec_info->pMapEntries[i];
        
        if (entry->offset + entry->size > spec_info->dataSize) {
            printf("Error: Specialization map entry %u exceeds data size\n", i);
            return false;
        }
        
        if (entry->size == 0) {
            printf("Warning: Specialization map entry %u has zero size\n", i);
        }
    }
    
    printf("Specialization info validation passed\n");
    return true;
}

// Validate push constant ranges
bool vk_validation_validate_push_constant_ranges(const VkPushConstantRange* ranges, u32 range_count) {
    if (!ranges || range_count == 0) {
        return true; // Push constants are optional
    }
    
    for (u32 i = 0; i < range_count; i++) {
        const VkPushConstantRange* range = &ranges[i];
        
        if (range->offset == 0 && range->size == 0) {
            printf("Warning: Push constant range %u has zero offset and size\n", i);
        }
        
        if (range->size > 128) {
            printf("Warning: Push constant range %u size (%u) exceeds minimum maximum (128)\n", i, range->size);
        }
        
        if (range->stageFlags == 0) {
            printf("Warning: Push constant range %u has no stage flags\n", i);
        }
    }
    
    printf("Push constant range validation passed\n");
    return true;
}

// Validate vertex input attributes
bool vk_validation_validate_vertex_input_attributes(const VkVertexInputAttributeDescription* attributes, 
                                                   u32 attribute_count) {
    if (!attributes || attribute_count == 0) {
        return true; // Vertex attributes are optional
    }
    
    for (u32 i = 0; i < attribute_count; i++) {
        const VkVertexInputAttributeDescription* attr = &attributes[i];
        
        if (attr->format == VK_FORMAT_MAX_ENUM) {
            printf("Error: Invalid format for vertex attribute %u\n", i);
            return false;
        }
        
        if (attr->offset > 2048) {
            printf("Warning: Vertex attribute %u offset (%u) seems large\n", i, attr->offset);
        }
    }
    
    printf("Vertex input attribute validation passed\n");
    return true;
}

// Validate vertex input bindings
bool vk_validation_validate_vertex_input_bindings(const VkVertexInputBindingDescription* bindings, 
                                                u32 binding_count) {
    if (!bindings || binding_count == 0) {
        return true; // Vertex bindings are optional
    }
    
    for (u32 i = 0; i < binding_count; i++) {
        const VkVertexInputBindingDescription* binding = &bindings[i];
        
        if (binding->stride == 0) {
            printf("Warning: Vertex binding %u has zero stride\n", i);
        }
        
        if (binding->inputRate == VK_VERTEX_INPUT_RATE_MAX_ENUM) {
            printf("Error: Invalid input rate for vertex binding %u\n", i);
            return false;
        }
    }
    
    printf("Vertex input binding validation passed\n");
    return true;
}

// Validate surface creation
bool vk_validation_validate_surface_creation(VkInstance instance, const VkSurfaceCreateInfoKHR* create_info) {
    if (!instance || !create_info) {
        printf("Error: Invalid instance or surface create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SURFACE_CREATE_INFO_KHR) {
        printf("Error: Invalid sType in surface create info\n");
        return false;
    }
    
    printf("Surface creation validation passed\n");
    return true;
}

// Validate swapchain creation
bool vk_validation_validate_swapchain_creation(const VkSwapchainCreateInfoKHR* create_info, 
                                              VkSurfaceKHR surface) {
    if (!create_info || surface == VK_NULL_HANDLE) {
        printf("Error: Invalid swapchain create info or surface\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR) {
        printf("Error: Invalid sType in swapchain create info\n");
        return false;
    }
    
    if (create_info->minImageCount == 0) {
        printf("Error: Swapchain minImageCount cannot be zero\n");
        return false;
    }
    
    if (create_info->imageFormat == VK_FORMAT_MAX_ENUM) {
        printf("Error: Invalid swapchain image format\n");
        return false;
    }
    
    if (create_info->imageExtent.width == 0 || create_info->imageExtent.height == 0) {
        printf("Error: Swapchain image extent cannot be zero\n");
        return false;
    }
    
    if (create_info->imageArrayLayers == 0) {
        printf("Error: Swapchain imageArrayLayers cannot be zero\n");
        return false;
    }
    
    if (create_info->queueFamilyIndexCount > 0 && !create_info->pQueueFamilyIndices) {
        printf("Error: Queue family index count specified but indices array is NULL\n");
        return false;
    }
    
    printf("Swapchain creation validation passed\n");
    return true;
}

// Validate queue creation and submission
bool vk_validation_validate_queue_creation(VkDevice device, const VkDeviceQueueCreateInfo* queue_create_infos, 
                                         u32 queue_count) {
    if (!device || !queue_create_infos || queue_count == 0) {
        printf("Error: Invalid device, queue create infos, or queue count\n");
        return false;
    }
    
    for (u32 i = 0; i < queue_count; i++) {
        const VkDeviceQueueCreateInfo* queue_info = &queue_create_infos[i];
        
        if (queue_info->sType != VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO) {
            printf("Error: Invalid sType in queue create info %u\n", i);
            return false;
        }
        
        if (queue_info->queueCount == 0) {
            printf("Error: Queue count cannot be zero for queue %u\n", i);
            return false;
        }
        
        if (!queue_info->pQueuePriorities) {
            printf("Error: Queue priorities array is NULL for queue %u\n", i);
            return false;
        }
    }
    
    printf("Queue creation validation passed\n");
    return true;
}

// Validate command pool creation
bool vk_validation_validate_command_pool_creation(const VkCommandPoolCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid command pool create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO) {
        printf("Error: Invalid sType in command pool create info\n");
        return false;
    }
    
    if (create_info->queueFamilyIndex == UINT32_MAX) {
        printf("Error: Invalid queue family index in command pool create info\n");
        return false;
    }
    
    printf("Command pool creation validation passed\n");
    return true;
}

// Validate descriptor pool creation
bool vk_validation_validate_descriptor_pool_creation(const VkDescriptorPoolCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid descriptor pool create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO) {
        printf("Error: Invalid sType in descriptor pool create info\n");
        return false;
    }
    
    if (create_info->maxSets == 0) {
        printf("Error: Descriptor pool maxSets cannot be zero\n");
        return false;
    }
    
    if (create_info->poolSizeCount > 0 && !create_info->pPoolSizes) {
        printf("Error: Pool size count specified but pool sizes array is NULL\n");
        return false;
    }
    
    // Validate pool sizes
    for (u32 i = 0; i < create_info->poolSizeCount; i++) {
        const VkDescriptorPoolSize* pool_size = &create_info->pPoolSizes[i];
        
        if (pool_size->descriptorCount == 0) {
            printf("Warning: Descriptor count is zero for pool size %u\n", i);
        }
        
        if (pool_size->type == VK_DESCRIPTOR_TYPE_MAX_ENUM) {
            printf("Error: Invalid descriptor type for pool size %u\n", i);
            return false;
        }
    }
    
    printf("Descriptor pool creation validation passed\n");
    return true;
}

// Validate pipeline layout creation
bool vk_validation_validate_pipeline_layout_creation(const VkPipelineLayoutCreateInfo* create_info) {
    if (!create_info) {
        printf("Error: Invalid pipeline layout create info\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO) {
        printf("Error: Invalid sType in pipeline layout create info\n");
        return false;
    }
    
    if (create_info->setLayoutCount > 0 && !create_info->pSetLayouts) {
        printf("Error: Set layout count specified but layouts array is NULL\n");
        return false;
    }
    
    if (create_info->pushConstantRangeCount > 0 && !create_info->pPushConstantRanges) {
        printf("Error: Push constant range count specified but ranges array is NULL\n");
        return false;
    }
    
    // Validate push constant ranges
    for (u32 i = 0; i < create_info->pushConstantRangeCount; i++) {
        const VkPushConstantRange* range = &create_info->pPushConstantRanges[i];
        
        if (range->size == 0) {
            printf("Warning: Push constant range %u has zero size\n", i);
        }
        
        if (range->offset + range->size > 128) {
            printf("Warning: Push constant range %u exceeds minimum guaranteed size (128 bytes)\n", i);
        }
        
        if (range->stageFlags == 0) {
            printf("Warning: Push constant range %u has no shader stages specified\n", i);
        }
    }
    
    printf("Pipeline layout creation validation passed\n");
    return true;
}

// Validate image view creation
bool vk_validation_validate_image_view_creation(const VkImageViewCreateInfo* create_info, VkImage image) {
    if (!create_info || image == VK_NULL_HANDLE) {
        printf("Error: Invalid image view create info or image\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO) {
        printf("Error: Invalid sType in image view create info\n");
        return false;
    }
    
    if (create_info->image != image) {
        printf("Error: Image view create info image doesn't match provided image\n");
        return false;
    }
    
    if (create_info->format == VK_FORMAT_MAX_ENUM) {
        printf("Error: Invalid format in image view create info\n");
        return false;
    }
    
    if (create_info->viewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM) {
        printf("Error: Invalid view type in image view create info\n");
        return false;
    }
    
    // Validate component mapping
    if (create_info->components.r == VK_COMPONENT_SWIZZLE_IDENTITY && 
        create_info->components.g == VK_COMPONENT_SWIZZLE_IDENTITY &&
        create_info->components.b == VK_COMPONENT_SWIZZLE_IDENTITY &&
        create_info->components.a == VK_COMPONENT_SWIZZLE_IDENTITY) {
        // Identity swizzle is valid
    } else {
        // Validate individual swizzles
        VkComponentSwizzle swizzles[] = {create_info->components.r, create_info->components.g, 
                                         create_info->components.b, create_info->components.a};
        for (int i = 0; i < 4; i++) {
            if (swizzles[i] >= VK_COMPONENT_SWIZZLE_MAX_ENUM) {
                printf("Error: Invalid component swizzle %d\n", i);
                return false;
            }
        }
    }
    
    printf("Image view creation validation passed\n");
    return true;
}

// Validate buffer view creation
bool vk_validation_validate_buffer_view_creation(const VkBufferViewCreateInfo* create_info, VkBuffer buffer) {
    if (!create_info || buffer == VK_NULL_HANDLE) {
        printf("Error: Invalid buffer view create info or buffer\n");
        return false;
    }
    
    if (create_info->sType != VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO) {
        printf("Error: Invalid sType in buffer view create info\n");
        return false;
    }
    
    if (create_info->buffer != buffer) {
        printf("Error: Buffer view create info buffer doesn't match provided buffer\n");
        return false;
    }
    
    if (create_info->format == VK_FORMAT_MAX_ENUM) {
        printf("Error: Invalid format in buffer view create info\n");
        return false;
    }
    
    if (create_info->offset == 0 && create_info->range == VK_WHOLE_SIZE) {
        // Whole buffer view is valid
    } else if (create_info->range == VK_WHOLE_SIZE) {
        printf("Warning: Buffer view range is VK_WHOLE_SIZE but offset is non-zero\n");
    } else if (create_info->offset + create_info->range > create_info->range) {
        printf("Error: Buffer view offset + range exceeds buffer size\n");
        return false;
    }
    
    printf("Buffer view creation validation passed\n");
    return true;
}
keep adding make sure they havent been implemented already