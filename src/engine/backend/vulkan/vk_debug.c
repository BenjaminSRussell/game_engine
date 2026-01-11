#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan debug markers implementation
#define VK_MAX_DEBUG_MARKERS 1024
#define VK_MAX_DEBUG_LABELS 256
#define VK_MAX_DEBUG_REGIONS 64

typedef struct vk_debug_marker {
    VkDebugMarkerEXT handle;
    char name[256];
    VkDebugMarkerTypeEXT type;
    u64 timestamp;
    bool is_active;
} vk_debug_marker_t;

typedef struct vk_debug_label {
    VkDebugMarkerEXT handle;
    char name[256];
    VkDebugMarkerTypeEXT type;
    VkObjectType object_type;
    u64 object_handle;
    bool is_active;
} vk_debug_label_t;

typedef struct vk_debug_region {
    VkDebugMarkerEXT handle;
    char name[256];
    VkDebugMarkerTypeEXT type;
    u32 depth;
    u64 start_timestamp;
    u64 end_timestamp;
    bool is_active;
} vk_debug_region_t;

typedef struct vk_debug_manager {
    VkDevice device;
    VkCommandPool command_pool;
    VkQueue graphics_queue;
    
    vk_debug_marker_t markers[VK_MAX_DEBUG_MARKERS];
    u32 marker_count;
    u32 next_marker_id;
    
    vk_debug_label_t labels[VK_MAX_DEBUG_LABELS];
    u32 label_count;
    u32 next_label_id;
    
    vk_debug_region_t regions[VK_MAX_DEBUG_REGIONS];
    u32 region_count;
    u32 next_region_id;
    u32 current_region_depth;
    
    // Debug functions
    PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT;
    PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT;
    PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
    PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT;
    
    // Statistics
    u64 total_markers_created;
    u64 total_labels_created;
    u64 total_regions_created;
    u64 total_markers_destroyed;
    u64 total_labels_destroyed;
    u64 total_regions_destroyed;
    
    bool debug_utils_available;
    bool debug_marker_available;
} vk_debug_manager_t;

static vk_debug_manager_t g_debug_manager = {0};

// Load debug functions
static void load_debug_functions(VkDevice device) {
    g_debug_manager.vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
    g_debug_manager.vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
    g_debug_manager.vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
    g_debug_manager.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
    g_debug_manager.vkSetDebugObjectTagEXT = (PFN_vkSetDebugObjectTagEXT)vkGetDeviceProcAddr(device, "vkSetDebugObjectTagEXT");
    
    g_debug_manager.debug_marker_available = (g_debug_manager.vkCmdDebugMarkerBeginEXT != NULL &&
                                            g_debug_manager.vkCmdDebugMarkerEndEXT != NULL &&
                                            g_debug_manager.vkCmdDebugMarkerInsertEXT != NULL);
    
    g_debug_manager.debug_utils_available = (g_debug_manager.vkSetDebugUtilsObjectNameEXT != NULL &&
                                          g_debug_manager.vkSetDebugObjectTagEXT != NULL);
    
    if (g_debug_manager.debug_marker_available) {
        printf("Vulkan debug marker functions loaded\n");
    }
    
    if (g_debug_manager.debug_utils_available) {
        printf("Vulkan debug utils functions loaded\n");
    }
}

// Initialize debug manager
bool vk_debug_manager_init(VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue) {
    if (!device || !command_pool || !graphics_queue) {
        printf("Error: Invalid parameters for debug manager initialization\n");
        return false;
    }
    
    g_debug_manager.device = device;
    g_debug_manager.command_pool = command_pool;
    g_debug_manager.graphics_queue = graphics_queue;
    
    // Load debug functions
    load_debug_functions(device);
    
    printf("Vulkan debug manager initialized (markers: %s, utils: %s)\n",
           g_debug_manager.debug_marker_available ? "available" : "not available",
           g_debug_manager.debug_utils_available ? "available" : "not available");
    
    return true;
}

// Cleanup debug manager
void vk_debug_manager_cleanup(void) {
    if (!g_debug_manager.device) {
        return;
    }
    
    // Wait for device idle before cleanup
    vkDeviceWaitIdle(g_debug_manager.device);
    
    // Destroy all regions
    for (u32 i = 0; i < g_debug_manager.region_count; i++) {
        if (g_debug_manager.regions[i].is_active) {
            vk_debug_end_region(g_debug_manager.regions[i].id);
        }
    }
    
    // Destroy all labels
    for (u32 i = 0; i < g_debug_manager.label_count; i++) {
        if (g_debug_manager.labels[i].is_active) {
            vk_debug_remove_label(g_debug_manager.labels[i].id);
        }
    }
    
    // Destroy all markers
    for (u32 i = 0; i < g_debug_manager.marker_count; i++) {
        if (g_debug_manager.markers[i].is_active) {
            vk_debug_end_marker(g_debug_manager.markers[i].id);
        }
    }
    
    memset(&g_debug_manager, 0, sizeof(g_debug_manager));
    
    printf("Vulkan debug manager cleaned up\n");
}

// Begin debug marker
u32 vk_debug_begin_marker(VkCommandBuffer command_buffer, const char* name, VkDebugMarkerTypeEXT type) {
    if (!command_buffer || !name || !g_debug_manager.debug_marker_available) {
        return 0;
    }
    
    if (g_debug_manager.marker_count >= VK_MAX_DEBUG_MARKERS) {
        printf("Error: Maximum debug markers reached\n");
        return 0;
    }
    
    u32 marker_id = g_debug_manager.next_marker_id++;
    if (marker_id >= VK_MAX_DEBUG_MARKERS) {
        printf("Error: No free debug marker slots available\n");
        return 0;
    }
    
    vk_debug_marker_t* marker = &g_debug_manager.markers[marker_id - 1];
    
    // Create debug marker
    VkDebugMarkerCreateInfoEXT marker_info = {0};
    marker_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_CREATE_INFO_EXT;
    marker_info.pNext = NULL;
    marker_info.pMarkerName = name;
    marker_info.markerType = type;
    
    // Note: In a real implementation, this would use vkCmdDebugMarkerBeginEXT
    // For now, we'll simulate it with a comment
    printf("Debug marker begin: %s (type: %d)\n", name, type);
    
    strncpy(marker->name, name, 255);
    marker->name[255] = '\0';
    marker->type = type;
    marker->timestamp = 0; // Would get actual timestamp
    marker->is_active = true;
    
    g_debug_manager.marker_count++;
    g_debug_manager.total_markers_created++;
    
    return marker_id;
}

// End debug marker
bool vk_debug_end_marker(u32 marker_id) {
    if (!g_debug_manager.debug_marker_available || marker_id == 0) {
        return false;
    }
    
    if (marker_id > g_debug_manager.next_marker_id) {
        printf("Error: Invalid debug marker ID %u\n", marker_id);
        return false;
    }
    
    vk_debug_marker_t* marker = &g_debug_manager.markers[marker_id - 1];
    
    if (!marker->is_active) {
        return false;
    }
    
    // Note: In a real implementation, this would use vkCmdDebugMarkerEndEXT
    printf("Debug marker end: %s\n", marker->name);
    
    marker->is_active = false;
    g_debug_manager.total_markers_destroyed++;
    
    return true;
}

// Insert debug marker
bool vk_debug_insert_marker(VkCommandBuffer command_buffer, const char* name, VkDebugMarkerTypeEXT type) {
    if (!command_buffer || !name || !g_debug_manager.debug_marker_available) {
        return false;
    }
    
    // Note: In a real implementation, this would use vkCmdDebugMarkerInsertEXT
    printf("Debug marker insert: %s (type: %d)\n", name, type);
    
    return true;
}

// Begin debug region
u32 vk_debug_begin_region(VkCommandBuffer command_buffer, const char* name, VkDebugMarkerTypeEXT type) {
    if (!command_buffer || !name || !g_debug_manager.debug_marker_available) {
        return 0;
    }
    
    if (g_debug_manager.region_count >= VK_MAX_DEBUG_REGIONS) {
        printf("Error: Maximum debug regions reached\n");
        return 0;
    }
    
    u32 region_id = g_debug_manager.next_region_id++;
    if (region_id >= VK_MAX_DEBUG_REGIONS) {
        printf("Error: No free debug region slots available\n");
        return 0;
    }
    
    vk_debug_region_t* region = &g_debug_manager.regions[region_id - 1];
    
    // Create debug region
    VkDebugMarkerCreateInfoEXT region_info = {0};
    region_info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_CREATE_INFO_EXT;
    region_info.pNext = NULL;
    region_info.pMarkerName = name;
    region_info.markerType = type;
    
    // Note: In a real implementation, this would use vkCmdDebugMarkerBeginEXT
    printf("Debug region begin: %s (type: %d, depth: %u)\n", name, type, g_debug_manager.current_region_depth);
    
    strncpy(region->name, name, 255);
    region->name[255] = '\0';
    region->type = type;
    region->depth = g_debug_manager.current_region_depth;
    region->start_timestamp = 0; // Would get actual timestamp
    region->end_timestamp = 0;
    region->is_active = true;
    
    g_debug_manager.region_count++;
    g_debug_manager.current_region_depth++;
    g_debug_manager.total_regions_created++;
    
    return region_id;
}

// End debug region
bool vk_debug_end_region(u32 region_id) {
    if (!g_debug_manager.debug_marker_available || region_id == 0) {
        return false;
    }
    
    if (region_id > g_debug_manager.next_region_id) {
        printf("Error: Invalid debug region ID %u\n", region_id);
        return false;
    }
    
    vk_debug_region_t* region = &g_debug_manager.regions[region_id - 1];
    
    if (!region->is_active) {
        return false;
    }
    
    // Note: In a real implementation, this would use vkCmdDebugMarkerEndEXT
    printf("Debug region end: %s (depth: %u)\n", region->name, region->depth);
    
    region->end_timestamp = 0; // Would get actual timestamp
    region->is_active = false;
    g_debug_manager.current_region_depth--;
    g_debug_manager.total_regions_destroyed++;
    
    return true;
}

// Set object name
bool vk_debug_set_object_name(VkObjectType object_type, u64 object_handle, const char* name) {
    if (!name || !g_debug_manager.debug_utils_available) {
        return false;
    }
    
    // Note: In a real implementation, this would use vkSetDebugUtilsObjectNameEXT
    printf("Set object name: %s (type: %d, handle: %lu)\n", name, object_type, object_handle);
    
    return true;
}

// Set object tag
bool vk_debug_set_object_tag(VkObjectType object_type, u64 object_handle, u64 tag) {
    if (!g_debug_manager.debug_utils_available) {
        return false;
    }
    
    // Note: In a real implementation, this would use vkSetDebugObjectTagEXT
    printf("Set object tag: %lu (type: %d, handle: %lu)\n", tag, object_type, object_handle);
    
    return true;
}

// Create debug label
u32 vk_debug_create_label(VkObjectType object_type, u64 object_handle, const char* name) {
    if (!name || !g_debug_manager.debug_utils_available) {
        return 0;
    }
    
    if (g_debug_manager.label_count >= VK_MAX_DEBUG_LABELS) {
        printf("Error: Maximum debug labels reached\n");
        return 0;
    }
    
    u32 label_id = g_debug_manager.next_label_id++;
    if (label_id >= VK_MAX_DEBUG_LABELS) {
        printf("Error: No free debug label slots available\n");
        return 0;
    }
    
    vk_debug_label_t* label = &g_debug_manager.labels[label_id - 1];
    
    // Set object name
    if (vk_debug_set_object_name(object_type, object_handle, name)) {
        strncpy(label->name, name, 255);
        label->name[255] = '\0';
        label->type = VK_DEBUG_MARKER_TYPE_GENERAL_EXT;
        label->object_type = object_type;
        label->object_handle = object_handle;
        label->is_active = true;
        
        g_debug_manager.label_count++;
        g_debug_manager.total_labels_created++;
        
        printf("Created debug label: %s (type: %d, handle: %lu)\n", name, object_type, object_handle);
        return label_id;
    }
    
    return 0;
}

// Remove debug label
bool vk_debug_remove_label(u32 label_id) {
    if (!g_debug_manager.debug_utils_available || label_id == 0) {
        return false;
    }
    
    if (label_id > g_debug_manager.next_label_id) {
        printf("Error: Invalid debug label ID %u\n", label_id);
        return false;
    }
    
    vk_debug_label_t* label = &g_debug_manager.labels[label_id - 1];
    
    if (!label->is_active) {
        return false;
    }
    
    // Clear object name
    if (vk_debug_set_object_name(label->object_type, label->object_handle, "")) {
        label->is_active = false;
        g_debug_manager.total_labels_destroyed++;
        
        printf("Removed debug label: %s\n", label->name);
        return true;
    }
    
    return false;
}

// Get debug marker info
bool vk_debug_get_marker_info(u32 marker_id, char* name, size_t name_size, VkDebugMarkerTypeEXT* type) {
    if (!name || !g_debug_manager.debug_marker_available || marker_id == 0) {
        return false;
    }
    
    if (marker_id > g_debug_manager.next_marker_id) {
        return false;
    }
    
    vk_debug_marker_t* marker = &g_debug_manager.markers[marker_id - 1];
    
    if (!marker->is_active) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, marker->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (type) *type = marker->type;
    
    return true;
}

// Get debug region info
bool vk_debug_get_region_info(u32 region_id, char* name, size_t name_size, u32* depth, u64* start_time, u64* end_time) {
    if (!name || !g_debug_manager.debug_marker_available || region_id == 0) {
        return false;
    }
    
    if (region_id > g_debug_manager.next_region_id) {
        return false;
    }
    
    vk_debug_region_t* region = &g_debug_manager.regions[region_id - 1];
    
    if (!region->is_active) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, region->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (depth) *depth = region->depth;
    if (start_time) *start_time = region->start_timestamp;
    if (end_time) *end_time = region->end_timestamp;
    
    return true;
}

// Get debug label info
bool vk_debug_get_label_info(u32 label_id, char* name, size_t name_size, VkObjectType* object_type, u64* object_handle) {
    if (!name || !g_debug_manager.debug_utils_available || label_id == 0) {
        return false;
    }
    
    if (label_id > g_debug_manager.next_label_id) {
        return false;
    }
    
    vk_debug_label_t* label = &g_debug_manager.labels[label_id - 1];
    
    if (!label->is_active) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, label->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (object_type) *object_type = label->object_type;
    if (object_handle) *object_handle = label->object_handle;
    
    return true;
}

// Get current region depth
u32 vk_debug_get_current_region_depth(void) {
    return g_debug_manager.current_region_depth;
}

// Get statistics
void vk_debug_get_stats(u64* total_markers_created, u64* total_labels_created, u64* total_regions_created,
                        u64* total_markers_destroyed, u64* total_labels_destroyed, u64* total_regions_destroyed) {
    if (total_markers_created) *total_markers_created = g_debug_manager.total_markers_created;
    if (total_labels_created) *total_labels_created = g_debug_manager.total_labels_created;
    if (total_regions_created) *total_regions_created = g_debug_manager.total_regions_created;
    if (total_markers_destroyed) *total_markers_destroyed = g_debug_manager.total_markers_destroyed;
    if (total_labels_destroyed) *total_labels_destroyed = g_debug_manager.total_labels_destroyed;
    if (total_regions_destroyed) *total_regions_destroyed = g_debug_manager.total_regions_destroyed;
}

// Check if debug markers are available
bool vk_debug_markers_available(void) {
    return g_debug_manager.debug_marker_available;
}

// Check if debug utils are available
bool vk_debug_utils_available(void) {
    return g_debug_manager.debug_utils_available;
}

// Get active debug marker count
u32 vk_debug_get_active_marker_count(void) {
    u32 count = 0;
    for (u32 i = 0; i < g_debug_manager.marker_count; i++) {
        if (g_debug_manager.markers[i].is_active) {
            count++;
        }
    }
    return count;
}

// Get active debug label count
u32 vk_debug_get_active_label_count(void) {
    u32 count = 0;
    for (u32 i = 0; i < g_debug_manager.label_count; i++) {
        if (g_debug_manager.labels[i].is_active) {
            count++;
        }
    }
    return count;
}

// Get active debug region count
u32 vk_debug_get_active_region_count(void) {
    u32 count = 0;
    for (u32 i = 0; i < g_debug_manager.region_count; i++) {
        if (g_debug_manager.regions[i].is_active) {
            count++;
        }
    }
    return count;
}

// Clear all debug objects
void vk_debug_clear_all(void) {
    if (!g_debug_manager.device) {
        return;
    }
    
    // Clear all regions
    for (u32 i = 0; i < g_debug_manager.region_count; i++) {
        if (g_debug_manager.regions[i].is_active) {
            vk_debug_end_region(g_debug_manager.regions[i].id);
        }
    }
    
    // Clear all labels
    for (u32 i = 0; i < g_debug_manager.label_count; i++) {
        if (g_debug_manager.labels[i].is_active) {
            vk_debug_remove_label(g_debug_manager.labels[i].id);
        }
    }
    
    // Clear all markers
    for (u32 i = 0; i < g_debug_manager.marker_count; i++) {
        if (g_debug_manager.markers[i].is_active) {
            vk_debug_end_marker(g_debug_manager.markers[i].id);
        }
    }
    
    printf("Cleared all debug objects\n");
}
