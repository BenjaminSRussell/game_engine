// render_target_verification.c
//
// Purpose: Comprehensive render target setup verification system
// Implements TODO-0042: Render target setup verification

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

// Mock graphics API types
typedef enum {
    RENDER_TARGET_TYPE_COLOR,
    RENDER_TARGET_TYPE_DEPTH,
    RENDER_TARGET_TYPE_STENCIL,
    RENDER_TARGET_TYPE_DEPTH_STENCIL,
    RENDER_TARGET_TYPE_COUNT
} RenderTargetType;

typedef enum {
    PIXEL_FORMAT_R8G8B8A8_UNORM,
    PIXEL_FORMAT_R32G32B32A32_FLOAT,
    PIXEL_FORMAT_R16G16B16A16_FLOAT,
    PIXEL_FORMAT_D32_FLOAT,
    PIXEL_FORMAT_D24_UNORM_S8_UINT,
    PIXEL_FORMAT_R11G11B10_FLOAT,
    PIXEL_FORMAT_COUNT
} PixelFormat;

typedef enum {
    SAMPLE_COUNT_1,
    SAMPLE_COUNT_2,
    SAMPLE_COUNT_4,
    SAMPLE_COUNT_8,
    SAMPLE_COUNT_16,
    SAMPLE_COUNT_COUNT
} SampleCount;

typedef struct {
    void* handle;
    RenderTargetType type;
    PixelFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth; // For 3D textures
    uint32_t array_size; // For texture arrays
    SampleCount sample_count;
    bool is_multisampled;
    bool is_array;
    bool is_cube_map;
    uint32_t mip_levels;
    const char* name;
    uint64_t creation_id;
    bool is_bound;
    uint32_t bind_slot;
    size_t memory_size;
} RenderTarget;

typedef struct {
    RenderTarget* color_targets[8]; // Up to 8 color targets
    RenderTarget* depth_target;
    RenderTarget* stencil_target;
    uint32_t color_target_count;
    uint32_t width;
    uint32_t height;
    bool is_complete;
    uint64_t framebuffer_id;
    const char* name;
} RenderTargetSet;

typedef struct {
    RenderTarget* targets;
    uint32_t capacity;
    uint32_t count;
    RenderTargetSet* framebuffer_sets;
    uint32_t fb_capacity;
    uint32_t fb_count;
    uint64_t next_creation_id;
    uint64_t next_framebuffer_id;
    bool is_initialized;
    size_t total_memory_allocated;
} RenderTargetManager;

// Global render target manager
static RenderTargetManager g_rt_manager = {0};

// Format and type names
static const char* TARGET_TYPE_NAMES[RENDER_TARGET_TYPE_COUNT] = {
    "Color",
    "Depth", 
    "Stencil",
    "Depth-Stencil"
};

static const char* PIXEL_FORMAT_NAMES[PIXEL_FORMAT_COUNT] = {
    "R8G8B8A8_UNORM",
    "R32G32B32A32_FLOAT",
    "R16G16B16A16_FLOAT",
    "D32_FLOAT",
    "D24_UNORM_S8_UINT",
    "R11G11B10_FLOAT"
};

static const char* SAMPLE_COUNT_NAMES[SAMPLE_COUNT_COUNT] = {
    "1x", "2x", "4x", "8x", "16x"
};

// Format properties
typedef struct {
    uint8_t bytes_per_pixel;
    bool is_depth_format;
    bool is_stencil_format;
    bool supports_filtering;
    bool supports_rendering;
} PixelFormatProperties;

static const PixelFormatProperties FORMAT_PROPERTIES[PIXEL_FORMAT_COUNT] = {
    {4,  false, false, true,  true},  // R8G8B8A8_UNORM
    {16, false, false, true,  true},  // R32G32B32A32_FLOAT
    {8,  false, false, true,  true},  // R16G16B16A16_FLOAT
    {4,  true,  false, false, true},  // D32_FLOAT
    {4,  true,  true,  false, true},  // D24_UNORM_S8_UINT
    {4,  false, false, true,  true}   // R11G11B10_FLOAT
};

// Initialize render target manager
bool render_target_manager_init(void) {
    printf("Initializing render target manager...\n");
    
    g_rt_manager.capacity = 64;
    g_rt_manager.targets = calloc(g_rt_manager.capacity, sizeof(RenderTarget));
    if (!g_rt_manager.targets) {
        printf("Error: Failed to allocate render target array\n");
        return false;
    }
    
    g_rt_manager.fb_capacity = 16;
    g_rt_manager.framebuffer_sets = calloc(g_rt_manager.fb_capacity, sizeof(RenderTargetSet));
    if (!g_rt_manager.framebuffer_sets) {
        free(g_rt_manager.targets);
        printf("Error: Failed to allocate framebuffer array\n");
        return false;
    }
    
    g_rt_manager.next_creation_id = 1;
    g_rt_manager.next_framebuffer_id = 1;
    g_rt_manager.is_initialized = true;
    
    printf("Render target manager initialized successfully\n");
    return true;
}

// Calculate memory size for render target
static size_t calculate_memory_size(uint32_t width, uint32_t height, uint32_t depth, 
                                   PixelFormat format, SampleCount sample_count, uint32_t mip_levels) {
    const PixelFormatProperties* props = &FORMAT_PROPERTIES[format];
    size_t base_size = (size_t)width * height * depth * props->bytes_per_pixel;
    
    // Account for multisampling
    uint32_t sample_multiplier = 1 << (int)sample_count;
    size_t total_size = base_size * sample_multiplier;
    
    // Account for mipmaps (full mip chain = 1.33x base size)
    if (mip_levels > 1) {
        total_size = (size_t)(total_size * 1.33);
    }
    
    return total_size;
}

// Validate render target parameters
static bool validate_render_target_params(RenderTargetType type, PixelFormat format,
                                         uint32_t width, uint32_t height, uint32_t depth,
                                         SampleCount sample_count, uint32_t mip_levels) {
    // Validate dimensions
    if (width == 0 || height == 0 || depth == 0) {
        printf("Error: Invalid dimensions %ux%ux%u\n", width, height, depth);
        return false;
    }
    
    // Maximum texture size check
    if (width > 16384 || height > 16384 || depth > 2048) {
        printf("Error: Dimensions exceed maximum size\n");
        return false;
    }
    
    // Power of two check for certain formats
    if (format == PIXEL_FORMAT_D24_UNORM_S8_UINT) {
        if ((width & (width - 1)) || (height & (height - 1))) {
            printf("Warning: Non-power-of-two dimensions for depth format\n");
        }
    }
    
    // Validate format compatibility with type
    const PixelFormatProperties* props = &FORMAT_PROPERTIES[format];
    
    if (type == RENDER_TARGET_TYPE_DEPTH || type == RENDER_TARGET_TYPE_DEPTH_STENCIL) {
        if (!props->is_depth_format) {
            printf("Error: Non-depth format used for depth target\n");
            return false;
        }
    }
    
    if (type == RENDER_TARGET_TYPE_STENCIL || type == RENDER_TARGET_TYPE_DEPTH_STENCIL) {
        if (!props->is_stencil_format && type != RENDER_TARGET_TYPE_DEPTH_STENCIL) {
            printf("Error: Non-stencil format used for stencil target\n");
            return false;
        }
    }
    
    // Validate mip levels
    uint32_t max_mips = 1 + (uint32_t)floor(log2(width > height ? width : height));
    if (mip_levels > max_mips) {
        printf("Error: Too many mip levels (%u, max %u)\n", mip_levels, max_mips);
        return false;
    }
    
    return true;
}

// Create render target
uint64_t create_render_target(RenderTargetType type, PixelFormat format,
                              uint32_t width, uint32_t height, uint32_t depth,
                              SampleCount sample_count, uint32_t mip_levels,
                              const char* name) {
    if (!g_rt_manager.is_initialized) {
        printf("Error: Render target manager not initialized\n");
        return 0;
    }
    
    // Validate parameters
    if (!validate_render_target_params(type, format, width, height, depth, sample_count, mip_levels)) {
        return 0;
    }
    
    // Expand array if needed
    if (g_rt_manager.count >= g_rt_manager.capacity) {
        uint32_t new_capacity = g_rt_manager.capacity * 2;
        RenderTarget* new_targets = realloc(g_rt_manager.targets, 
                                           new_capacity * sizeof(RenderTarget));
        if (!new_targets) {
            printf("Error: Failed to expand render target array\n");
            return 0;
        }
        g_rt_manager.targets = new_targets;
        g_rt_manager.capacity = new_capacity;
    }
    
    // Create render target
    RenderTarget* rt = &g_rt_manager.targets[g_rt_manager.count];
    memset(rt, 0, sizeof(RenderTarget));
    
    rt->handle = malloc(calculate_memory_size(width, height, depth, format, sample_count, mip_levels));
    if (!rt->handle) {
        printf("Error: Failed to allocate render target memory\n");
        return 0;
    }
    
    rt->type = type;
    rt->format = format;
    rt->width = width;
    rt->height = height;
    rt->depth = depth;
    rt->array_size = 1;
    rt->sample_count = sample_count;
    rt->is_multisampled = (sample_count > SAMPLE_COUNT_1);
    rt->is_array = false;
    rt->is_cube_map = false;
    rt->mip_levels = mip_levels;
    rt->name = name ? strdup(name) : NULL;
    rt->creation_id = g_rt_manager.next_creation_id++;
    rt->is_bound = false;
    rt->bind_slot = 0;
    rt->memory_size = calculate_memory_size(width, height, depth, format, sample_count, mip_levels);
    
    g_rt_manager.count++;
    g_rt_manager.total_memory_allocated += rt->memory_size;
    
    printf("Created render target [%zu] %s: %ux%ux%u %s %s (%.1f MB)\n",
           rt->creation_id, name ? name : "Unnamed", width, height, depth,
           TARGET_TYPE_NAMES[type], PIXEL_FORMAT_NAMES[format],
           rt->memory_size / (1024.0 * 1024.0));
    
    return rt->creation_id;
}

// Find render target by ID
static RenderTarget* find_render_target(uint64_t target_id) {
    for (uint32_t i = 0; i < g_rt_manager.count; i++) {
        if (g_rt_manager.targets[i].creation_id == target_id) {
            return &g_rt_manager.targets[i];
        }
    }
    return NULL;
}

// Create framebuffer set
uint64_t create_framebuffer_set(const char* name) {
    if (!g_rt_manager.is_initialized) {
        printf("Error: Render target manager not initialized\n");
        return 0;
    }
    
    // Expand array if needed
    if (g_rt_manager.fb_count >= g_rt_manager.fb_capacity) {
        uint32_t new_capacity = g_rt_manager.fb_capacity * 2;
        RenderTargetSet* new_fb = realloc(g_rt_manager.framebuffer_sets,
                                       new_capacity * sizeof(RenderTargetSet));
        if (!new_fb) {
            printf("Error: Failed to expand framebuffer array\n");
            return 0;
        }
        g_rt_manager.framebuffer_sets = new_fb;
        g_rt_manager.fb_capacity = new_capacity;
    }
    
    // Create framebuffer
    RenderTargetSet* fb = &g_rt_manager.framebuffer_sets[g_rt_manager.fb_count];
    memset(fb, 0, sizeof(RenderTargetSet));
    
    fb->name = name ? strdup(name) : NULL;
    fb->framebuffer_id = g_rt_manager.next_framebuffer_id++;
    fb->is_complete = false;
    
    g_rt_manager.fb_count++;
    
    printf("Created framebuffer set [%zu] %s\n", fb->framebuffer_id, name ? name : "Unnamed");
    return fb->framebuffer_id;
}

// Find framebuffer set by ID
static RenderTargetSet* find_framebuffer_set(uint64_t fb_id) {
    for (uint32_t i = 0; i < g_rt_manager.fb_count; i++) {
        if (g_rt_manager.framebuffer_sets[i].framebuffer_id == fb_id) {
            return &g_rt_manager.framebuffer_sets[i];
        }
    }
    return NULL;
}

// Attach render target to framebuffer
bool attach_render_target_to_framebuffer(uint64_t fb_id, uint64_t target_id, uint32_t slot) {
    RenderTargetSet* fb = find_framebuffer_set(fb_id);
    if (!fb) {
        printf("Error: Invalid framebuffer ID %zu\n", fb_id);
        return false;
    }
    
    RenderTarget* rt = find_render_target(target_id);
    if (!rt) {
        printf("Error: Invalid render target ID %zu\n", target_id);
        return false;
    }
    
    if (slot >= 8) {
        printf("Error: Invalid color attachment slot %u (max 7)\n", slot);
        return false;
    }
    
    if (rt->type == RENDER_TARGET_TYPE_COLOR) {
        fb->color_targets[slot] = rt;
        if (slot + 1 > fb->color_target_count) {
            fb->color_target_count = slot + 1;
        }
        printf("Attached color target [%zu] to framebuffer [%zu] at slot %u\n",
               target_id, fb_id, slot);
    } else if (rt->type == RENDER_TARGET_TYPE_DEPTH || rt->type == RENDER_TARGET_TYPE_DEPTH_STENCIL) {
        fb->depth_target = rt;
        printf("Attached depth target [%zu] to framebuffer [%zu]\n", target_id, fb_id);
    } else if (rt->type == RENDER_TARGET_TYPE_STENCIL) {
        fb->stencil_target = rt;
        printf("Attached stencil target [%zu] to framebuffer [%zu]\n", target_id, fb_id);
    }
    
    // Update framebuffer dimensions
    if (fb->color_target_count > 0) {
        fb->width = fb->color_targets[0]->width;
        fb->height = fb->color_targets[0]->height;
    } else if (fb->depth_target) {
        fb->width = fb->depth_target->width;
        fb->height = fb->depth_target->height;
    }
    
    return true;
}

// Validate framebuffer completeness
bool validate_framebuffer_completeness(uint64_t fb_id) {
    RenderTargetSet* fb = find_framebuffer_set(fb_id);
    if (!fb) {
        printf("Error: Invalid framebuffer ID %zu\n", fb_id);
        return false;
    }
    
    printf("Validating framebuffer [%zu] %s...\n", fb_id, fb->name ? fb->name : "Unnamed");
    
    bool is_complete = true;
    
    // Check if we have at least one attachment
    if (fb->color_target_count == 0 && !fb->depth_target && !fb->stencil_target) {
        printf("  Error: No attachments found\n");
        is_complete = false;
    }
    
    // Check color attachments
    uint32_t fb_width = 0, fb_height = 0;
    if (fb->color_target_count > 0) {
        fb_width = fb->color_targets[0]->width;
        fb_height = fb->color_targets[0]->height;
        
        for (uint32_t i = 0; i < fb->color_target_count; i++) {
            if (!fb->color_targets[i]) {
                printf("  Error: Null color attachment at slot %u\n", i);
                is_complete = false;
                continue;
            }
            
            if (fb->color_targets[i]->width != fb_width || 
                fb->color_targets[i]->height != fb_height) {
                printf("  Error: Color target %u size mismatch (%ux%u vs %ux%u)\n",
                       i, fb->color_targets[i]->width, fb->color_targets[i]->height,
                       fb_width, fb_height);
                is_complete = false;
            }
            
            if (fb->color_targets[i]->type != RENDER_TARGET_TYPE_COLOR) {
                printf("  Error: Non-color target in color slot %u\n", i);
                is_complete = false;
            }
        }
    }
    
    // Check depth attachment
    if (fb->depth_target) {
        if (fb_width == 0) {
            fb_width = fb->depth_target->width;
            fb_height = fb->depth_target->height;
        } else if (fb->depth_target->width != fb_width || 
                   fb->depth_target->height != fb_height) {
            printf("  Error: Depth target size mismatch (%ux%u vs %ux%u)\n",
                   fb->depth_target->width, fb->depth_target->height,
                   fb_width, fb_height);
            is_complete = false;
        }
        
        if (fb->depth_target->type != RENDER_TARGET_TYPE_DEPTH &&
            fb->depth_target->type != RENDER_TARGET_TYPE_DEPTH_STENCIL) {
            printf("  Error: Non-depth target in depth slot\n");
            is_complete = false;
        }
    }
    
    // Check stencil attachment
    if (fb->stencil_target) {
        if (fb_width == 0) {
            fb_width = fb->stencil_target->width;
            fb_height = fb->stencil_target->height;
        } else if (fb->stencil_target->width != fb_width || 
                   fb->stencil_target->height != fb_height) {
            printf("  Error: Stencil target size mismatch (%ux%u vs %ux%u)\n",
                   fb->stencil_target->width, fb->stencil_target->height,
                   fb_width, fb_height);
            is_complete = false;
        }
        
        if (fb->stencil_target->type != RENDER_TARGET_TYPE_STENCIL &&
            fb->stencil_target->type != RENDER_TARGET_TYPE_DEPTH_STENCIL) {
            printf("  Error: Non-stencil target in stencil slot\n");
            is_complete = false;
        }
    }
    
    // Check for consistent multisampling
    SampleCount fb_sample_count = SAMPLE_COUNT_1;
    if (fb->color_target_count > 0) {
        fb_sample_count = fb->color_targets[0]->sample_count;
    } else if (fb->depth_target) {
        fb_sample_count = fb->depth_target->sample_count;
    }
    
    for (uint32_t i = 0; i < fb->color_target_count; i++) {
        if (fb->color_targets[i]->sample_count != fb_sample_count) {
            printf("  Error: Inconsistent sample count for color target %u\n", i);
            is_complete = false;
        }
    }
    
    if (fb->depth_target && fb->depth_target->sample_count != fb_sample_count) {
        printf("  Error: Inconsistent sample count for depth target\n");
        is_complete = false;
    }
    
    if (fb->stencil_target && fb->stencil_target->sample_count != fb_sample_count) {
        printf("  Error: Inconsistent sample count for stencil target\n");
        is_complete = false;
    }
    
    fb->is_complete = is_complete;
    fb->width = fb_width;
    fb->height = fb_height;
    
    printf("Framebuffer [%zu] validation: %s (%ux%u, %s)\n",
           fb_id, is_complete ? "COMPLETE" : "INCOMPLETE",
           fb_width, fb_height, SAMPLE_COUNT_NAMES[fb_sample_count]);
    
    return is_complete;
}

// Test render target creation and binding
bool test_render_target_creation(void) {
    printf("\n=== Render Target Creation Test ===\n");
    
    bool test_passed = true;
    
    // Test basic color target
    uint64_t color_rt = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R8G8B8A8_UNORM,
                                            1920, 1080, 1, SAMPLE_COUNT_1, 1, "MainColor");
    if (color_rt == 0) {
        printf("Failed to create color render target\n");
        test_passed = false;
    }
    
    // Test depth target
    uint64_t depth_rt = create_render_target(RENDER_TARGET_TYPE_DEPTH, PIXEL_FORMAT_D32_FLOAT,
                                            1920, 1080, 1, SAMPLE_COUNT_1, 1, "MainDepth");
    if (depth_rt == 0) {
        printf("Failed to create depth render target\n");
        test_passed = false;
    }
    
    // Test multisampled target
    uint64_t msaa_rt = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R16G16B16A16_FLOAT,
                                           1920, 1080, 1, SAMPLE_COUNT_4, 1, "MSAAColor");
    if (msaa_rt == 0) {
        printf("Failed to create MSAA render target\n");
        test_passed = false;
    }
    
    // Test invalid parameters
    uint64_t invalid_rt = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R8G8B8A8_UNORM,
                                             0, 1080, 1, SAMPLE_COUNT_1, 1, "Invalid");
    if (invalid_rt != 0) {
        printf("Error: Created render target with invalid dimensions\n");
        test_passed = false;
    }
    
    return test_passed;
}

// Test framebuffer setup
bool test_framebuffer_setup(void) {
    printf("\n=== Framebuffer Setup Test ===\n");
    
    bool test_passed = true;
    
    // Create render targets
    uint64_t color1 = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R8G8B8A8_UNORM,
                                         1920, 1080, 1, SAMPLE_COUNT_1, 1, "Color1");
    uint64_t color2 = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R32G32B32A32_FLOAT,
                                         1920, 1080, 1, SAMPLE_COUNT_1, 1, "Color2");
    uint64_t depth = create_render_target(RENDER_TARGET_TYPE_DEPTH, PIXEL_FORMAT_D32_FLOAT,
                                        1920, 1080, 1, SAMPLE_COUNT_1, 1, "Depth");
    
    // Create framebuffer
    uint64_t fb = create_framebuffer_set("MainFramebuffer");
    if (fb == 0) {
        printf("Failed to create framebuffer\n");
        return false;
    }
    
    // Attach targets
    if (!attach_render_target_to_framebuffer(fb, color1, 0)) test_passed = false;
    if (!attach_render_target_to_framebuffer(fb, color2, 1)) test_passed = false;
    if (!attach_render_target_to_framebuffer(fb, depth, 0)) test_passed = false;
    
    // Validate framebuffer
    if (!validate_framebuffer_completeness(fb)) {
        printf("Framebuffer validation failed\n");
        test_passed = false;
    }
    
    // Test mismatched dimensions
    uint64_t small_color = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R8G8B8A8_UNORM,
                                              512, 512, 1, SAMPLE_COUNT_1, 1, "SmallColor");
    uint64_t fb2 = create_framebuffer_set("MismatchedFramebuffer");
    attach_render_target_to_framebuffer(fb2, color1, 0);
    attach_render_target_to_framebuffer(fb2, small_color, 1);
    
    if (validate_framebuffer_completeness(fb2)) {
        printf("Error: Framebuffer with mismatched dimensions should be incomplete\n");
        test_passed = false;
    }
    
    return test_passed;
}

// Test edge cases
bool test_edge_cases(void) {
    printf("\n=== Edge Cases Test ===\n");
    
    bool test_passed = true;
    
    // Test maximum dimensions
    uint64_t max_rt = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R8G8B8A8_UNORM,
                                          16384, 16384, 1, SAMPLE_COUNT_1, 1, "MaxSize");
    if (max_rt == 0) {
        printf("Failed to create maximum size render target\n");
        test_passed = false;
    }
    
    // Test multiple mip levels
    uint64_t mip_rt = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R8G8B8A8_UNORM,
                                         1024, 1024, 1, SAMPLE_COUNT_1, 10, "Mipmapped");
    if (mip_rt == 0) {
        printf("Failed to create mipmapped render target\n");
        test_passed = false;
    }
    
    // Test too many mip levels
    uint64_t invalid_mip = create_render_target(RENDER_TARGET_TYPE_COLOR, PIXEL_FORMAT_R8G8B8A8_UNORM,
                                              256, 256, 1, SAMPLE_COUNT_1, 20, "InvalidMip");
    if (invalid_mip != 0) {
        printf("Error: Created render target with too many mip levels\n");
        test_passed = false;
    }
    
    // Test depth-stencil format
    uint64_t ds_rt = create_render_target(RENDER_TARGET_TYPE_DEPTH_STENCIL, PIXEL_FORMAT_D24_UNORM_S8_UINT,
                                        1920, 1080, 1, SAMPLE_COUNT_1, 1, "DepthStencil");
    if (ds_rt == 0) {
        printf("Failed to create depth-stencil render target\n");
        test_passed = false;
    }
    
    return test_passed;
}

// Generate verification report
void generate_render_target_report(void) {
    printf("\n=== Render Target Verification Report ===\n");
    
    printf("Total Render Targets: %u\n", g_rt_manager.count);
    printf("Total Framebuffers: %u\n", g_rt_manager.fb_count);
    printf("Total Memory Allocated: %.1f MB\n", g_rt_manager.total_memory_allocated / (1024.0 * 1024.0));
    
    // Render target breakdown
    printf("\n--- Render Target Breakdown ---\n");
    uint32_t type_counts[RENDER_TARGET_TYPE_COUNT] = {0};
    uint32_t format_counts[PIXEL_FORMAT_COUNT] = {0};
    uint32_t sample_counts[SAMPLE_COUNT_COUNT] = {0};
    
    for (uint32_t i = 0; i < g_rt_manager.count; i++) {
        RenderTarget* rt = &g_rt_manager.targets[i];
        type_counts[rt->type]++;
        format_counts[rt->format]++;
        sample_counts[rt->sample_count]++;
    }
    
    for (int i = 0; i < RENDER_TARGET_TYPE_COUNT; i++) {
        if (type_counts[i] > 0) {
            printf("%s targets: %u\n", TARGET_TYPE_NAMES[i], type_counts[i]);
        }
    }
    
    for (int i = 0; i < PIXEL_FORMAT_COUNT; i++) {
        if (format_counts[i] > 0) {
            printf("%s format: %u\n", PIXEL_FORMAT_NAMES[i], format_counts[i]);
        }
    }
    
    for (int i = 0; i < SAMPLE_COUNT_COUNT; i++) {
        if (sample_counts[i] > 0) {
            printf("%s samples: %u\n", SAMPLE_COUNT_NAMES[i], sample_counts[i]);
        }
    }
    
    // Framebuffer status
    printf("\n--- Framebuffer Status ---\n");
    uint32_t complete_fbs = 0;
    for (uint32_t i = 0; i < g_rt_manager.fb_count; i++) {
        RenderTargetSet* fb = &g_rt_manager.framebuffer_sets[i];
        if (fb->is_complete) complete_fbs++;
        printf("Framebuffer [%zu] %s: %s (%ux%u, %u color targets)\n",
               fb->framebuffer_id, fb->name ? fb->name : "Unnamed",
               fb->is_complete ? "COMPLETE" : "INCOMPLETE",
               fb->width, fb->height, fb->color_target_count);
    }
    
    printf("Complete framebuffers: %u/%u (%.1f%%)\n",
           complete_fbs, g_rt_manager.fb_count,
           (float)complete_fbs / g_rt_manager.fb_count * 100.0f);
}

// Cleanup render target manager
void render_target_manager_cleanup(void) {
    if (!g_rt_manager.is_initialized) return;
    
    printf("Cleaning up render target manager...\n");
    
    // Free render targets
    for (uint32_t i = 0; i < g_rt_manager.count; i++) {
        RenderTarget* rt = &g_rt_manager.targets[i];
        free(rt->handle);
        if (rt->name) free((void*)rt->name);
    }
    
    // Free framebuffers
    for (uint32_t i = 0; i < g_rt_manager.fb_count; i++) {
        RenderTargetSet* fb = &g_rt_manager.framebuffer_sets[i];
        if (fb->name) free((void*)fb->name);
    }
    
    free(g_rt_manager.targets);
    free(g_rt_manager.framebuffer_sets);
    memset(&g_rt_manager, 0, sizeof(g_rt_manager));
    
    printf("Render target manager cleaned up\n");
}

// Main verification function
int main(void) {
    printf("=== Render Target Setup Verification ===\n");
    printf("Implementing TODO-0042: Render target setup verification\n\n");
    
    // Initialize manager
    if (!render_target_manager_init()) {
        printf("Failed to initialize render target manager\n");
        return 1;
    }
    
    // Run tests
    bool creation_passed = test_render_target_creation();
    bool framebuffer_passed = test_framebuffer_setup();
    bool edge_cases_passed = test_edge_cases();
    
    // Generate report
    generate_render_target_report();
    
    // Cleanup
    render_target_manager_cleanup();
    
    printf("\n=== Verification Complete ===\n");
    bool all_passed = creation_passed && framebuffer_passed && edge_cases_passed;
    printf("Result: %s\n", all_passed ? "PASSED" : "FAILED");
    
    return all_passed ? 0 : 1;
}
