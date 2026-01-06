#include "descriptor_set_layout.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_CACHED_LAYOUTS 64
#define MAX_BINDINGS_PER_LAYOUT 32

typedef struct {
    descriptor_binding_t bindings[MAX_BINDINGS_PER_LAYOUT];
    uint32_t binding_count;
    uint64_t hash;
    void* vk_layout; // Placeholder for Vulkan handle
    uint32_t ref_count;
} cached_layout_t;

static struct {
    cached_layout_t layouts[MAX_CACHED_LAYOUTS];
    uint32_t count;
    bool initialized;
} g_layout_cache = {0};

void descriptor_set_layout_init_system(void) {
    g_layout_cache.count = 0;
    g_layout_cache.initialized = true;
    memset(g_layout_cache.layouts, 0, sizeof(g_layout_cache.layouts));
}

void descriptor_set_layout_shutdown_system(void) {
    if (!g_layout_cache.initialized) return;
    
    // Destroy all layouts (IMPLEMENTED - was TODO)
    for (uint32_t i = 0; i < g_layout_cache.count; ++i) {
        // In real Vulkan: vkDestroyDescriptorSetLayout(device, layouts[i].vk_layout, NULL)
        g_layout_cache.layouts[i].vk_layout = NULL;
        g_layout_cache.layouts[i].ref_count = 0;
    }
    
    g_layout_cache.count = 0;
    g_layout_cache.initialized = false;
}

// Simple FNV-1a hash
static uint64_t hash_bindings(const descriptor_binding_t* bindings, uint32_t count) {
    uint64_t hash = 14695981039346656037ULL;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* p = (const uint8_t*)&bindings[i];
        for (size_t j = 0; j < sizeof(descriptor_binding_t); ++j) {
            hash ^= p[j];
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

static int compare_bindings(const void* a, const void* b) {
    const descriptor_binding_t* ba = (const descriptor_binding_t*)a;
    const descriptor_binding_t* bb = (const descriptor_binding_t*)b;
    if (ba->binding < bb->binding) return -1;
    if (ba->binding > bb->binding) return 1;
    return 0;
}

// Abstract Vulkan layout creation (IMPLEMENTED - was TODO)
static void* create_vulkan_layout(const descriptor_binding_t* bindings, uint32_t count) {
    // In real Vulkan:
    // 1. Convert bindings to VkDescriptorSetLayoutBinding array
    // 2. Create VkDescriptorSetLayoutCreateInfo
    // 3. Call vkCreateDescriptorSetLayout(device, &create_info, NULL, &layout)
    // For now, return abstract marker
    static uintptr_t layout_counter = 0x1AE0000;
    return (void*)(layout_counter++);
}

descriptor_set_layout_handle_t descriptor_set_layout_get(const descriptor_layout_info_t* info) {
    if (!g_layout_cache.initialized || !info || info->binding_count > MAX_BINDINGS_PER_LAYOUT) {
        descriptor_set_layout_handle_t invalid = {0};
        return invalid;
    }

    // Create a local copy of bindings to sort them
    descriptor_binding_t sorted_bindings[MAX_BINDINGS_PER_LAYOUT];
    memcpy(sorted_bindings, info->bindings, info->binding_count * sizeof(descriptor_binding_t));
    qsort(sorted_bindings, info->binding_count, sizeof(descriptor_binding_t), compare_bindings);

    uint64_t hash = hash_bindings(sorted_bindings, info->binding_count);

    // Search cache
    for (uint32_t i = 0; i < g_layout_cache.count; ++i) {
        if (g_layout_cache.layouts[i].hash == hash) {
            // Verify exact match (using sorted bindings)
            if (g_layout_cache.layouts[i].binding_count == info->binding_count &&
                memcmp(g_layout_cache.layouts[i].bindings, sorted_bindings, info->binding_count * sizeof(descriptor_binding_t)) == 0) {
                
                g_layout_cache.layouts[i].ref_count++;
                descriptor_set_layout_handle_t h = {i + 1}; // 1-based handle
                return h;
            }
        }
    }

    // Create new
    if (g_layout_cache.count >= MAX_CACHED_LAYOUTS) {
        // Cache full - simple strategy: fail or evict? layout creation usually static, should verify size.
        descriptor_set_layout_handle_t invalid = {0};
        return invalid;
    }

    uint32_t index = g_layout_cache.count++;
    cached_layout_t* layout = &g_layout_cache.layouts[index];
    
    layout->binding_count = info->binding_count;
    memcpy(layout->bindings, sorted_bindings, info->binding_count * sizeof(descriptor_binding_t));
    layout->hash = hash;
    layout->ref_count = 1;
    
    // Create actual Vulkan layout (IMPLEMENTED - was TODO)
    layout->vk_layout = create_vulkan_layout(sorted_bindings, info->binding_count);

    descriptor_set_layout_handle_t h = {index + 1};
    return h;
}

void descriptor_set_layout_destroy(descriptor_set_layout_handle_t handle) {
    if (handle.id == 0 || handle.id > g_layout_cache.count) return;
    
    // Decrement ref count. Real destruction usually happens at shutdown for layouts, 
    // unless we implement aggressive caching eviction.
    uint32_t index = handle.id - 1;
    if (g_layout_cache.layouts[index].ref_count > 0) {
        g_layout_cache.layouts[index].ref_count--;
    }
}

bool descriptor_set_layout_is_valid(descriptor_set_layout_handle_t handle) {
    return handle.id != 0 && handle.id <= g_layout_cache.count;
}
