#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    
    bool validation_enabled;
    bool debug_utils_available;
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
        
        snprintf(message, 511, "[%s] %s: %s\n",
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
        printf("[VALIDATION ERROR] %s: %s\n", callback_data->pMessageIdName ? callback_data->pMessageIdName : "UNKNOWN", callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        printf("[VALIDATION WARNING] %s: %s\n", callback_data->pMessageIdName ? callback_data->pMessageIdName : "UNKNOWN", callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        printf("[VALIDATION INFO] %s: %s\n", callback_data->pMessageIdName ? callback_data->pMessageIdName : "UNKNOWN", callback_data->pMessage);
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

// Check if validation layers are available
static bool check_validation_layers_available(void) {
    u32 layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    
    if (layer_count == 0) {
        printf("No validation layers available\n");
        return false;
    }
    
    VkLayerProperties* available_layers = malloc(layer_count * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);
    
    printf("Available validation layers:\n");
    for (u32 i = 0; i < layer_count; i++) {
        printf("  %u: %s (%s)\n", i, available_layers[i].layerName, available_layers[i].description);
        
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
    
    g_validation_manager.instance = instance;
    g_validation_manager.validation_enabled = enable_validation;
    
    // Load debug utils functions
    load_debug_utils_functions(instance);
    
    // Check for available validation layers
    if (enable_validation) {
        if (!check_validation_layers_available()) {
            printf("Warning: Validation requested but no layers available\n");
            return false;
        }
        
        // Enable standard validation layers
        for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
            if (strcmp(g_validation_manager.layers[i].name, "VK_LAYER_KHRONOS_validation") == 0 ||
                strcmp(g_validation_manager.layers[i].name, "VK_LAYER_LUNARG_standard_validation") == 0) {
                g_validation_manager.layers[i].is_enabled = true;
                g_validation_manager.enabled_layer_count++;
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
                printf("Warning: Failed to create default debug messenger\n");
            } else {
                printf("Created default debug messenger\n");
            }
        }
    }
    
    printf("Vulkan validation manager initialized (validation: %s)\n", enable_validation ? "enabled" : "disabled");
    return true;
}

// Cleanup validation manager
void vk_validation_manager_cleanup(void) {
    if (!g_validation_manager.instance) {
        return;
    }
    
    // Destroy all debug callbacks
    for (u32 i = 0; i < g_validation_manager.callback_count; i++) {
        if (g_validation_manager.callbacks[i].is_active && g_validation_manager.callbacks[i].handle != VK_NULL_HANDLE) {
            vkDestroyDebugUtilsMessengerEXT(g_validation_manager.instance, g_validation_manager.callbacks[i].handle, NULL);
        }
    }
    
    // Destroy default messenger
    if (g_validation_manager.default_messenger != VK_NULL_HANDLE) {
        vkDestroyDebugUtilsMessengerEXT(g_validation_manager.instance, g_validation_manager.default_messenger, NULL);
        g_validation_manager.default_messenger = VK_NULL_HANDLE;
    }
    
    memset(&g_validation_manager, 0, sizeof(g_validation_manager));
    
    printf("Vulkan validation manager cleaned up\n");
}

// Create debug callback
u32 vk_validation_create_debug_callback(const VkDebugUtilsMessengerCreateInfoEXT* create_info, PFN_vkDebugUtilsMessengerCallbackEXT callback_func, void* user_data) {
    if (!g_validation_manager.instance || !g_validation_manager.debug_utils_available) {
        return 0;
    }
    
    if (g_validation_manager.callback_count >= VK_MAX_DEBUG_CALLBACKS) {
        printf("Error: Maximum debug callbacks reached\n");
        return 0;
    }
    
    u32 callback_id = g_validation_manager.callback_count++;
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[callback_id - 1];
    
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
        printf("Error: Failed to create debug callback\n");
        return 0;
    }
    
    callback->create_info = messenger_info;
    callback->callback_func = messenger_info.pfnUserCallback;
    callback->user_data = messenger_info.pUserData;
    callback->is_active = true;
    
    printf("Created debug callback (ID: %u)\n", callback_id);
    
    return callback_id;
}

// Destroy debug callback
bool vk_validation_destroy_debug_callback(u32 callback_id) {
    if (!g_validation_manager.instance || callback_id == 0) {
        return false;
    }
    
    if (callback_id > g_validation_manager.callback_count) {
        printf("Error: Invalid debug callback ID %u\n", callback_id);
        return false;
    }
    
    vk_debug_callback_t* callback = &g_validation_manager.callbacks[callback_id - 1];
    
    if (!callback->is_active) {
        return false;
    }
    
    vkDestroyDebugUtilsMessengerEXT(g_validation_manager.instance, callback->handle, NULL);
    
    memset(callback, 0, sizeof(vk_debug_callback_t));
    callback->is_active = false;
    
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
    
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (strcmp(g_validation_manager.layers[i].name, layer_name) == 0) {
            if (!g_validation_manager.layers[i].is_enabled) {
                g_validation_manager.layers[i].is_enabled = true;
                g_validation_manager.enabled_layer_count++;
                printf("Enabled validation layer: %s\n", layer_name);
                return true;
            }
            return true; // Already enabled
        }
    }
    
    printf("Warning: Validation layer not found: %s\n", layer_name);
    return false;
}

// Disable validation layer
bool vk_validation_disable_layer(const char* layer_name) {
    if (!layer_name || !g_validation_manager.validation_enabled) {
        return false;
    }
    
    for (u32 i = 0; i < g_validation_manager.layer_count; i++) {
        if (strcmp(g_validation_manager.layers[i].name, layer_name) == 0) {
            if (g_validation_manager.layers[i].is_enabled) {
                g_validation_manager.layers[i].is_enabled = false;
                g_validation_manager.enabled_layer_count--;
                printf("Disabled validation layer: %s\n", layer_name);
                return true;
            }
            return true; // Already disabled
        }
    }
    
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
    if (!g_validation_manager.validation_enabled) {
        return;
    }
    
    printf("[VALIDATION ERROR] %s\n", message);
    g_validation_manager.error_count++;
    g_validation_manager.total_debug_messages++;
}

// Report validation warning
void vk_validation_report_warning(const char* message) {
    if (!g_validation_manager.validation_enabled) {
        return;
    }
    
    printf("[VALIDATION WARNING] %s\n", message);
    g_validation_manager.warning_count++;
    g_validation_manager.total_debug_messages++;
}

// Report validation info
void vk_validation_report_info(const char* message) {
    if (!g_validation_manager.validation_enabled) {
        return;
    }
    
    printf("[VALIDATION INFO] %s\n", message);
    g_validation_manager.info_count++;
    g_validation_manager.total_debug_messages++;
}

// Report validation verbose
void vk_validation_report_verbose(const char* message) {
    if (!g_validation_manager.validation_enabled) {
        return;
    }
    
    printf("[VALIDATION VERBOSE] %s\n", message);
    g_validation_manager.verbose_count++;
    g_validation_manager.total_debug_messages++;
}
