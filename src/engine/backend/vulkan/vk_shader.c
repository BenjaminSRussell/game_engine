#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Vulkan shader compilation pipeline
#define VK_MAX_SHADERS 256
#define VK_MAX_SHADER_STAGES 6
#define VK_SHADER_CACHE_SIZE 1024

typedef enum {
    VK_SHADER_STAGE_VERTEX = 0,
    VK_SHADER_STAGE_TESSELLATION_CONTROL,
    VK_SHADER_STAGE_TESSELLATION_EVALUATION,
    VK_SHADER_STAGE_GEOMETRY,
    VK_SHADER_STAGE_FRAGMENT,
    VK_SHADER_STAGE_COMPUTE,
    VK_SHADER_STAGE_COUNT
} vk_shader_stage_t;

typedef struct vk_shader_module {
    VkShaderModule handle;
    VkShaderStageFlagBits stage;
    char entry_point[64];
    char name[256];
    u32 hash;
    bool is_compiled;
    u64 last_modified;
} vk_shader_module_t;

typedef struct vk_shader_cache_entry {
    u32 hash;
    VkShaderModule module;
    VkShaderStageFlagBits stage;
    char entry_point[64];
    u64 last_used;
    u32 use_count;
} vk_shader_cache_entry_t;

typedef struct vk_shader_compiler {
    VkDevice device;
    VkPhysicalDevice physical_device;
    
    vk_shader_module_t shaders[VK_MAX_SHADERS];
    vk_shader_cache_entry_t cache[VK_SHADER_CACHE_SIZE];
    
    u32 shader_count;
    u32 cache_count;
    u32 next_shader_id;
    
    // Compilation statistics
    u32 total_compilations;
    u32 cache_hits;
    u32 cache_misses;
    u32 failed_compilations;
    
    // Hot reload support
    bool hot_reload_enabled;
    u64 last_check_time;
} vk_shader_compiler_t;

static vk_shader_compiler_t g_shader_compiler = {0};

// Simple hash function for shader source
static u32 hash_shader_source(const char* source) {
    u32 hash = 5381;
    int c;
    
    while ((c = *source++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

// Check if shader is supported by device
static bool is_shader_stage_supported(VkShaderStageFlagBits stage) {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(g_shader_compiler.physical_device, &features);
    
    switch (stage) {
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return features.tessellationShader;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return features.geometryShader;
        default:
            return true;
    }
}

// Compile shader from GLSL source
static VkShaderModule compile_shader_from_source(const char* source, VkShaderStageFlagBits stage) {
    if (!source || !g_shader_compiler.device) {
        return VK_NULL_HANDLE;
    }
    
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = strlen(source);
    create_info.pCode = (const uint32_t*)source;
    
    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(g_shader_compiler.device, &create_info, NULL, &shader_module);
    
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create shader module (stage: %d)\n", stage);
        return VK_NULL_HANDLE;
    }
    
    return shader_module;
}

// Initialize shader compiler
bool vk_shader_compiler_init(VkDevice device, VkPhysicalDevice physical_device, bool hot_reload) {
    if (!device || !physical_device) {
        printf("Error: Invalid device or physical device for shader compiler\n");
        return false;
    }
    
    g_shader_compiler.device = device;
    g_shader_compiler.physical_device = physical_device;
    g_shader_compiler.hot_reload_enabled = hot_reload;
    g_shader_compiler.last_check_time = 0;
    
    printf("Vulkan shader compiler initialized (hot reload: %s)\n", hot_reload ? "enabled" : "disabled");
    return true;
}

// Cleanup shader compiler
void vk_shader_compiler_cleanup(void) {
    if (!g_shader_compiler.device) {
        return;
    }
    
    // Destroy all shader modules
    for (u32 i = 0; i < g_shader_compiler.shader_count; i++) {
        if (g_shader_compiler.shaders[i].handle != VK_NULL_HANDLE) {
            vkDestroyShaderModule(g_shader_compiler.device, g_shader_compiler.shaders[i].handle, NULL);
        }
    }
    
    // Destroy cache entries
    for (u32 i = 0; i < g_shader_compiler.cache_count; i++) {
        if (g_shader_compiler.cache[i].module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(g_shader_compiler.device, g_shader_compiler.cache[i].module, NULL);
        }
    }
    
    memset(&g_shader_compiler, 0, sizeof(g_shader_compiler));
    
    printf("Vulkan shader compiler cleaned up\n");
}

// Load shader from source
u32 vk_shader_load_from_source(const char* name, const char* source, VkShaderStageFlagBits stage, const char* entry_point) {
    if (!name || !source || !g_shader_compiler.device) {
        return 0;
    }
    
    if (!is_shader_stage_supported(stage)) {
        printf("Error: Shader stage %d not supported by device\n", stage);
        return 0;
    }
    
    if (g_shader_compiler.shader_count >= VK_MAX_SHADERS) {
        printf("Error: Maximum shaders reached\n");
        return 0;
    }
    
    // Calculate hash
    u32 hash = hash_shader_source(source);
    
    // Check cache first
    for (u32 i = 0; i < g_shader_compiler.cache_count; i++) {
        if (g_shader_compiler.cache[i].hash == hash && g_shader_compiler.cache[i].stage == stage) {
            g_shader_compiler.cache[i].last_used = 0;
            g_shader_compiler.cache[i].use_count++;
            g_shader_compiler.cache_hits++;
            
            return g_shader_compiler.cache[i].module != VK_NULL_HANDLE ? i + 1 : 0;
        }
    }
    
    // Compile shader
    VkShaderModule module = compile_shader_from_source(source, stage);
    if (module == VK_NULL_HANDLE) {
        g_shader_compiler.failed_compilations++;
        return 0;
    }
    
    // Store shader
    u32 shader_index = g_shader_compiler.next_shader_id++;
    vk_shader_module_t* shader = &g_shader_compiler.shaders[shader_index];
    
    shader->handle = module;
    shader->stage = stage;
    strncpy(shader->entry_point, entry_point ? entry_point : "main", 63);
    shader->entry_point[63] = '\0';
    strncpy(shader->name, name, 255);
    shader->name[255] = '\0';
    shader->hash = hash;
    shader->is_compiled = true;
    shader->last_modified = 0;
    
    // Add to cache
    if (g_shader_compiler.cache_count < VK_SHADER_CACHE_SIZE) {
        vk_shader_cache_entry_t* cache_entry = &g_shader_compiler.cache[g_shader_compiler.cache_count];
        cache_entry->hash = hash;
        cache_entry->module = module;
        cache_entry->stage = stage;
        strncpy(cache_entry->entry_point, shader->entry_point, 63);
        cache_entry->entry_point[63] = '\0';
        cache_entry->last_used = 0;
        cache_entry->use_count = 1;
        
        g_shader_compiler.cache_count++;
    }
    
    g_shader_compiler.shader_count++;
    g_shader_compiler.total_compilations++;
    g_shader_compiler.cache_misses++;
    
    printf("Loaded shader '%s' (stage: %d, entry: %s)\n", name, stage, shader->entry_point);
    return shader_index + 1;
}

// Load shader from SPIR-V binary
u32 vk_shader_load_from_spirv(const char* name, const uint32_t* spirv_code, size_t code_size, VkShaderStageFlagBits stage, const char* entry_point) {
    if (!name || !spirv_code || code_size == 0 || !g_shader_compiler.device) {
        return 0;
    }
    
    if (!is_shader_stage_supported(stage)) {
        printf("Error: Shader stage %d not supported by device\n", stage);
        return 0;
    }
    
    if (g_shader_compiler.shader_count >= VK_MAX_SHADERS) {
        printf("Error: Maximum shaders reached\n");
        return 0;
    }
    
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code_size;
    create_info.pCode = spirv_code;
    
    VkShaderModule module;
    VkResult result = vkCreateShaderModule(g_shader_compiler.device, &create_info, NULL, &module);
    
    if (result != VK_SUCCESS) {
        printf("Error: Failed to create shader module from SPIR-V\n");
        g_shader_compiler.failed_compilations++;
        return 0;
    }
    
    // Store shader
    u32 shader_index = g_shader_compiler.next_shader_id++;
    vk_shader_module_t* shader = &g_shader_compiler.shaders[shader_index];
    
    shader->handle = module;
    shader->stage = stage;
    strncpy(shader->entry_point, entry_point ? entry_point : "main", 63);
    shader->entry_point[63] = '\0';
    strncpy(shader->name, name, 255);
    shader->name[255] = '\0';
    shader->hash = hash_shader_source((const char*)spirv_code);
    shader->is_compiled = true;
    shader->last_modified = 0;
    
    g_shader_compiler.shader_count++;
    g_shader_compiler.total_compilations++;
    
    printf("Loaded SPIR-V shader '%s' (stage: %d, entry: %s)\n", name, stage, shader->entry_point);
    return shader_index + 1;
}

// Unload shader
bool vk_shader_unload(u32 shader_id) {
    if (!g_shader_compiler.device || shader_id == 0) {
        return false;
    }
    
    if (shader_id > g_shader_compiler.next_shader_id) {
        printf("Error: Invalid shader ID %u\n", shader_id);
        return false;
    }
    
    vk_shader_module_t* shader = &g_shader_compiler.shaders[shader_id - 1];
    
    if (shader->handle == VK_NULL_HANDLE) {
        return false;
    }
    
    vkDestroyShaderModule(g_shader_compiler.device, shader->handle, NULL);
    shader->handle = VK_NULL_HANDLE;
    shader->is_compiled = false;
    
    printf("Unloaded shader '%s'\n", shader->name);
    return true;
}

// Get shader module handle
VkShaderModule vk_shader_get_module(u32 shader_id) {
    if (!g_shader_compiler.device || shader_id == 0) {
        return VK_NULL_HANDLE;
    }
    
    if (shader_id > g_shader_compiler.next_shader_id) {
        return VK_NULL_HANDLE;
    }
    
    return g_shader_compiler.shaders[shader_id - 1].handle;
}

// Get shader stage
VkShaderStageFlagBits vk_shader_get_stage(u32 shader_id) {
    if (!g_shader_compiler.device || shader_id == 0) {
        return 0;
    }
    
    if (shader_id > g_shader_compiler.next_shader_id) {
        return 0;
    }
    
    return g_shader_compiler.shaders[shader_id - 1].stage;
}

// Get shader entry point
const char* vk_shader_get_entry_point(u32 shader_id) {
    if (!g_shader_compiler.device || shader_id == 0) {
        return NULL;
    }
    
    if (shader_id > g_shader_compiler.next_shader_id) {
        return NULL;
    }
    
    return g_shader_compiler.shaders[shader_id - 1].entry_point;
}

// Create pipeline shader stage create info
VkPipelineShaderStageCreateInfo vk_shader_create_stage_info(u32 shader_id) {
    VkPipelineShaderStageCreateInfo stage_info = {0};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    
    if (shader_id > 0 && shader_id <= g_shader_compiler.next_shader_id) {
        vk_shader_module_t* shader = &g_shader_compiler.shaders[shader_id - 1];
        stage_info.stage = shader->stage;
        stage_info.module = shader->handle;
        stage_info.pName = shader->entry_point;
    }
    
    return stage_info;
}

// Hot reload shader
bool vk_shader_reload(u32 shader_id, const char* new_source) {
    if (!g_shader_compiler.hot_reload_enabled || !new_source || shader_id == 0) {
        return false;
    }
    
    if (shader_id > g_shader_compiler.next_shader_id) {
        printf("Error: Invalid shader ID %u for reload\n", shader_id);
        return false;
    }
    
    vk_shader_module_t* shader = &g_shader_compiler.shaders[shader_id - 1];
    
    if (shader->handle == VK_NULL_HANDLE) {
        return false;
    }
    
    // Check if source changed
    u32 new_hash = hash_shader_source(new_source);
    if (new_hash == shader->hash) {
        return true; // No change
    }
    
    // Recompile shader
    VkShaderModule new_module = compile_shader_from_source(new_source, shader->stage);
    if (new_module == VK_NULL_HANDLE) {
        printf("Error: Failed to recompile shader '%s'\n", shader->name);
        return false;
    }
    
    // Destroy old module
    vkDestroyShaderModule(g_shader_compiler.device, shader->handle, NULL);
    
    // Update shader
    shader->handle = new_module;
    shader->hash = new_hash;
    shader->last_modified = 0;
    
    printf("Reloaded shader '%s'\n", shader->name);
    return true;
}

// Get compilation statistics
void vk_shader_get_stats(u32* total_compilations, u32* cache_hits, u32* cache_misses, u32* failed_compilations) {
    if (total_compilations) *total_compilations = g_shader_compiler.total_compilations;
    if (cache_hits) *cache_hits = g_shader_compiler.cache_hits;
    if (cache_misses) *cache_misses = g_shader_compiler.cache_misses;
    if (failed_compilations) *failed_compilations = g_shader_compiler.failed_compilations;
}

// Clear cache
void vk_shader_clear_cache(void) {
    for (u32 i = 0; i < g_shader_compiler.cache_count; i++) {
        if (g_shader_compiler.cache[i].module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(g_shader_compiler.device, g_shader_compiler.cache[i].module, NULL);
        }
    }
    
    g_shader_compiler.cache_count = 0;
    printf("Cleared shader cache\n");
}

// Validate shader
bool vk_shader_validate(u32 shader_id) {
    if (!g_shader_compiler.device || shader_id == 0) {
        return false;
    }
    
    if (shader_id > g_shader_compiler.next_shader_id) {
        return false;
    }
    
    vk_shader_module_t* shader = &g_shader_compiler.shaders[shader_id - 1];
    
    return shader->handle != VK_NULL_HANDLE && shader->is_compiled;
}

// Get shader info
bool vk_shader_get_info(u32 shader_id, char* name, size_t name_size, VkShaderStageFlagBits* stage, const char** entry_point) {
    if (!g_shader_compiler.device || shader_id == 0) {
        return false;
    }
    
    if (shader_id > g_shader_compiler.next_shader_id) {
        return false;
    }
    
    vk_shader_module_t* shader = &g_shader_compiler.shaders[shader_id - 1];
    
    if (shader->handle == VK_NULL_HANDLE) {
        return false;
    }
    
    if (name && name_size > 0) {
        strncpy(name, shader->name, name_size - 1);
        name[name_size - 1] = '\0';
    }
    
    if (stage) *stage = shader->stage;
    if (entry_point) *entry_point = shader->entry_point;
    
    return true;
}
