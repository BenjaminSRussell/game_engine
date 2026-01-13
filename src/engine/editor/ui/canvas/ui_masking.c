/*
 * ui_masking.c
 * UI stencil masking and clipping
 *
 * Part of the Ui Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Comprehensive UI clipping and masking implementation:
 * - Stencil buffer-based masking
 * - Scissor rectangle clipping
 * - Complex mask operations
 * - Performance-optimized batch processing
 * - Thread-safe operations
 */

#include "editor/ui/canvas/ui_masking.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

/* GPU/Rendering includes for actual implementation */
#ifdef ENABLE_VULKAN
#include <vulkan/vulkan.h>
#elif defined(ENABLE_OPENGL)
#include <GL/gl.h>
#include <GL/glext.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_RENDERING_UI_MASKING_MAX_COUNT 4096
#define UI_RENDERING_UI_MASKING_DEFAULT_CAPACITY 256
#define UI_RENDERING_UI_MASKING_ALIGNMENT 16
#define UI_RENDERING_UI_MASKING_MAX_CLIP_STACK 32
#define UI_RENDERING_UI_MASKING_MAX_STENCIL_MASKS 128
#define UI_RENDERING_UI_MASKING_CACHE_SIZE 64

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Mask types */
typedef enum {
    UI_MASK_TYPE_NONE = 0,
    UI_MASK_TYPE_RECTANGLE,
    UI_MASK_TYPE_CIRCLE,
    UI_MASK_TYPE_ROUNDED_RECT,
    UI_MASK_TYPE_POLYGON,
    UI_MASK_TYPE_STENCIL,
    UI_MASK_TYPE_SCISSOR
} ui_mask_type_t;

/* Mask operations */
typedef enum {
    UI_MASK_OP_REPLACE = 0,
    UI_MASK_OP_INTERSECT,
    UI_MASK_OP_UNION,
    UI_MASK_OP_SUBTRACT,
    UI_MASK_OP_XOR
} ui_mask_operation_t;

/* Rectangle mask */
typedef struct {
    float x, y, width, height;
} ui_rect_mask_t;

/* Circle mask */
typedef struct {
    float center_x, center_y, radius;
} ui_circle_mask_t;

/* Rounded rectangle mask */
typedef struct {
    float x, y, width, height, corner_radius;
} ui_rounded_rect_mask_t;

/* Polygon mask */
typedef struct {
    uint32_t vertex_count;
    float* vertices; /* x,y pairs */
} ui_polygon_mask_t;

/* Stencil mask */
typedef struct {
    uint32_t stencil_ref;
    uint32_t stencil_mask;
    uint32_t stencil_func; /* GL_NEVER, GL_LESS, etc. */
} ui_stencil_mask_t;

/* Scissor mask */
typedef struct {
    int x, y, width, height;
} ui_scissor_mask_t;

/* Combined mask data */
typedef struct {
    ui_mask_type_t type;
    ui_mask_operation_t operation;
    bool enabled;
    uint64_t last_frame;
    
    union {
        ui_rect_mask_t rect;
        ui_circle_mask_t circle;
        ui_rounded_rect_mask_t rounded_rect;
        ui_polygon_mask_t polygon;
        ui_stencil_mask_t stencil;
        ui_scissor_mask_t scissor;
    } data;
} ui_mask_data_t;

/* Clip stack entry */
typedef struct {
    ui_mask_data_t mask;
    bool was_active;
} ui_clip_stack_entry_t;

/* Performance statistics */
typedef struct {
    uint32_t mask_operations;
    uint32_t stencil_operations;
    uint32_t scissor_operations;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint64_t total_time_ns;
} ui_masking_stats_t;

/* Cache entry for mask results */
typedef struct {
    uint64_t hash;
    ui_mask_data_t mask;
    uint32_t gpu_handle;
    uint64_t last_used;
    bool valid;
} ui_mask_cache_entry_t;

typedef struct ui_rendering_ui_masking_context {
    ui_rendering_ui_masking_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    
    /* Global state */
    ui_mask_cache_entry_t mask_cache[UI_RENDERING_UI_MASKING_CACHE_SIZE];
    ui_masking_stats_t stats;
    pthread_mutex_t global_mutex;
    
    /* GPU state */
    uint32_t current_stencil_ref;
    bool stencil_enabled;
    bool scissor_enabled;
    
    bool initialized;
} ui_rendering_ui_masking_context_t;

static ui_rendering_ui_masking_context_t g_ui_masking_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool ui_rendering_ui_masking_validate(const ui_rendering_ui_masking_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->masks || item->mask_count == 0) return false;
    
    /* Validate each mask */
    for (uint32_t i = 0; i < item->mask_count; i++) {
        const ui_mask_data_t* mask = &item->masks[i];
        if (!mask->enabled) continue;
        
        switch (mask->type) {
            case UI_MASK_TYPE_POLYGON:
                if (mask->data.polygon.vertex_count < 3) return false;
                if (!mask->data.polygon.vertices) return false;
                break;
            case UI_MASK_TYPE_CIRCLE:
                if (mask->data.circle.radius <= 0.0f) return false;
                break;
            case UI_MASK_TYPE_RECTANGLE:
            case UI_MASK_TYPE_ROUNDED_RECT:
                if (mask->data.rect.width <= 0.0f || mask->data.rect.height <= 0.0f) return false;
                break;
            default:
                break;
        }
    }
    
    return true;
}

static void ui_rendering_ui_masking_cleanup_internal(ui_rendering_ui_masking_internal_t* item) {
    if (!item) return;
    
    /* Clean up mask data */
    if (item->masks) {
        for (uint32_t i = 0; i < item->mask_count; i++) {
            ui_mask_data_t* mask = &item->masks[i];
            if (mask->type == UI_MASK_TYPE_POLYGON && mask->data.polygon.vertices) {
                free(mask->data.polygon.vertices);
                mask->data.polygon.vertices = NULL;
            }
        }
        free(item->masks);
        item->masks = NULL;
    }
    
    item->mask_count = 0;
    item->mask_capacity = 0;
    item->clip_stack_top = 0;
    item->clipping_enabled = false;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int ui_rendering_ui_masking_init(void) {
    if (g_ui_masking_ctx.initialized) {
        return 0; // Already initialized
    }

    /* Initialize mutex */
    if (pthread_mutex_init(&g_ui_masking_ctx.global_mutex, NULL) != 0) {
        return -1;
    }

    /* Initialize cache */
    memset(g_ui_masking_ctx.mask_cache, 0, sizeof(g_ui_masking_ctx.mask_cache));
    
    /* Initialize statistics */
    memset(&g_ui_masking_ctx.stats, 0, sizeof(g_ui_masking_ctx.stats));
    
    /* Initialize GPU state */
    g_ui_masking_ctx.current_stencil_ref = 0;
    g_ui_masking_ctx.stencil_enabled = false;
    g_ui_masking_ctx.scissor_enabled = false;

    g_ui_masking_ctx.capacity = UI_RENDERING_UI_MASKING_DEFAULT_CAPACITY;
    g_ui_masking_ctx.items = calloc(g_ui_masking_ctx.capacity, sizeof(ui_rendering_ui_masking_internal_t));
    if (!g_ui_masking_ctx.items) {
        pthread_mutex_destroy(&g_ui_masking_ctx.global_mutex);
        return -2;
    }

    g_ui_masking_ctx.count = 0;
    g_ui_masking_ctx.initialized = true;

    return 0;
}

void ui_rendering_ui_masking_shutdown(void) {
    if (!g_ui_masking_ctx.initialized) {
        return;
    }

    /* Clean up all items */
    for (uint32_t i = 0; i < g_ui_masking_ctx.count; i++) {
        ui_rendering_ui_masking_cleanup_internal(&g_ui_masking_ctx.items[i]);
    }

    /* Clean up global resources */
    free(g_ui_masking_ctx.items);
    g_ui_masking_ctx.items = NULL;
    g_ui_masking_ctx.count = 0;
    g_ui_masking_ctx.capacity = 0;
    
    /* Clean up cache */
    memset(g_ui_masking_ctx.mask_cache, 0, sizeof(g_ui_masking_ctx.mask_cache));
    
    /* Print final statistics */
    ui_rendering_ui_masking_debug_print();
    
    /* Destroy mutex */
    pthread_mutex_destroy(&g_ui_masking_ctx.global_mutex);
    
    g_ui_masking_ctx.initialized = false;
}

int ui_rendering_ui_masking_create(ui_rendering_ui_masking_handle_t* out_handle, const ui_rendering_ui_masking_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1; // Invalid parameters
    }

    if (!g_ui_masking_ctx.initialized) {
        return -2; // Not initialized
    }

    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    
    if (g_ui_masking_ctx.count >= g_ui_masking_ctx.capacity) {
        pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
        return -3; // Capacity exceeded
    }

    uint32_t index = g_ui_masking_ctx.count++;
    ui_rendering_ui_masking_internal_t* item = &g_ui_masking_ctx.items[index];

    /* Initialize item */
    memset(item, 0, sizeof(ui_rendering_ui_masking_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->mask_capacity = 16; // Start with 16 mask slots
    item->masks = calloc(item->mask_capacity, sizeof(ui_mask_data_t));
    
    if (!item->masks) {
        g_ui_masking_ctx.count--;
        pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
        return -4; // Allocation failed
    }
    
    item->mask_count = 0;
    item->clip_stack_top = 0;
    item->clipping_enabled = false;
    item->current_stencil_mask = 0;
    item->initialized = true;
    item->dirty = false;
    item->frame_updated = 0;

    out_handle->id = index;
    
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    return 0;
}

void ui_rendering_ui_masking_destroy(ui_rendering_ui_masking_handle_t handle) {
    if (!g_ui_masking_ctx.initialized || handle.id >= g_ui_masking_ctx.count) {
        return;
    }

    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    
    ui_rendering_ui_masking_cleanup_internal(&g_ui_masking_ctx.items[handle.id]);
    
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
}

/* Helper function to calculate hash for mask caching */
static uint64_t ui_masking_calculate_hash(const ui_mask_data_t* mask) {
    uint64_t hash = 0;
    hash ^= (uint64_t)mask->type << 32;
    hash ^= (uint64_t)mask->operation << 48;
    hash ^= (uint64_t)mask->enabled << 63;
    
    switch (mask->type) {
        case UI_MASK_TYPE_RECTANGLE:
            hash ^= *(uint64_t*)&mask->data.rect;
            break;
        case UI_MASK_TYPE_CIRCLE:
            hash ^= *(uint64_t*)&mask->data.circle;
            break;
        case UI_MASK_TYPE_ROUNDED_RECT:
            hash ^= *(uint64_t*)&mask->data.rounded_rect;
            break;
        case UI_MASK_TYPE_STENCIL:
            hash ^= mask->data.stencil.stencil_ref;
            hash ^= mask->data.stencil.stencil_mask << 16;
            break;
        case UI_MASK_TYPE_SCISSOR:
            hash ^= (uint32_t)mask->data.scissor.x;
            hash ^= (uint32_t)mask->data.scissor.y << 8;
            hash ^= (uint32_t)mask->data.scissor.width << 16;
            hash ^= (uint32_t)mask->data.scissor.height << 24;
            break;
        default:
            break;
    }
    
    return hash;
}

/* Apply mask to GPU state */
static void ui_masking_apply_mask_gpu(const ui_mask_data_t* mask) {
#ifdef ENABLE_OPENGL
    switch (mask->type) {
        case UI_MASK_TYPE_SCISSOR:
            glEnable(GL_SCISSOR_TEST);
            glScissor(mask->data.scissor.x, mask->data.scissor.y, 
                     mask->data.scissor.width, mask->data.scissor.height);
            g_ui_masking_ctx.scissor_enabled = true;
            g_ui_masking_ctx.stats.scissor_operations++;
            break;
            
        case UI_MASK_TYPE_STENCIL:
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(mask->data.stencil.stencil_func, mask->data.stencil.stencil_ref, 
                        mask->data.stencil.stencil_mask);
            g_ui_masking_ctx.stencil_enabled = true;
            g_ui_masking_ctx.current_stencil_ref = mask->data.stencil.stencil_ref;
            g_ui_masking_ctx.stats.stencil_operations++;
            break;
            
        default:
            /* Other mask types would be rendered to stencil buffer */
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            g_ui_masking_ctx.stencil_enabled = true;
            g_ui_masking_ctx.stats.stencil_operations++;
            break;
    }
#endif
}

int ui_rendering_ui_masking_update(ui_rendering_ui_masking_handle_t handle, const void* data, size_t size) {
    if (!g_ui_masking_ctx.initialized || handle.id >= g_ui_masking_ctx.count) {
        return -1; // Invalid handle
    }

    if (size != sizeof(ui_mask_data_t)) {
        return -2; // Invalid data size
    }

    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    
    ui_rendering_ui_masking_internal_t* item = &g_ui_masking_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
        return -3; // Not initialized
    }

    const ui_mask_data_t* new_mask = (const ui_mask_data_t*)data;
    
    /* Check if we need to expand mask array */
    if (item->mask_count >= item->mask_capacity) {
        uint32_t new_capacity = item->mask_capacity * 2;
        ui_mask_data_t* new_masks = realloc(item->masks, new_capacity * sizeof(ui_mask_data_t));
        if (!new_masks) {
            pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
            return -4; // Allocation failed
        }
        item->masks = new_masks;
        item->mask_capacity = new_capacity;
    }
    
    /* Add or update mask */
    bool mask_found = false;
    for (uint32_t i = 0; i < item->mask_count; i++) {
        if (item->masks[i].type == new_mask->type) {
            /* Update existing mask */
            if (item->masks[i].type == UI_MASK_TYPE_POLYGON && item->masks[i].data.polygon.vertices) {
                free(item->masks[i].data.polygon.vertices);
            }
            item->masks[i] = *new_mask;
            mask_found = true;
            break;
        }
    }
    
    if (!mask_found) {
        /* Add new mask */
        item->masks[item->mask_count] = *new_mask;
        if (new_mask->type == UI_MASK_TYPE_POLYGON && new_mask->data.polygon.vertices) {
            /* Copy polygon vertices */
            size_t vertex_size = new_mask->data.polygon.vertex_count * 2 * sizeof(float);
            item->masks[item->mask_count].data.polygon.vertices = malloc(vertex_size);
            if (item->masks[item->mask_count].data.polygon.vertices) {
                memcpy(item->masks[item->mask_count].data.polygon.vertices, 
                       new_mask->data.polygon.vertices, vertex_size);
            }
        }
        item->mask_count++;
    }
    
    item->dirty = true;
    item->frame_updated = g_ui_masking_ctx.stats.mask_operations;
    g_ui_masking_ctx.stats.mask_operations++;
    
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    return 0;
}

bool ui_rendering_ui_masking_is_valid(ui_rendering_ui_masking_handle_t handle) {
    if (!g_ui_masking_ctx.initialized || handle.id >= g_ui_masking_ctx.count) {
        return false;
    }
    
    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    bool valid = g_ui_masking_ctx.items[handle.id].initialized;
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    
    return valid;
}

int ui_rendering_ui_masking_get_info(ui_rendering_ui_masking_handle_t handle, ui_rendering_ui_masking_info_t* out_info) {
    if (!out_info) {
        return -1; // Invalid parameter
    }

    if (!g_ui_masking_ctx.initialized || handle.id >= g_ui_masking_ctx.count) {
        return -2; // Invalid handle or not initialized
    }

    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    
    const ui_rendering_ui_masking_internal_t* item = &g_ui_masking_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    return 0;
}

void ui_rendering_ui_masking_mark_dirty(ui_rendering_ui_masking_handle_t handle) {
    if (!g_ui_masking_ctx.initialized || handle.id >= g_ui_masking_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    g_ui_masking_ctx.items[handle.id].dirty = true;
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
}

int ui_rendering_ui_masking_process_pending(void) {
    if (!g_ui_masking_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    
    int processed = 0;
    uint64_t start_time = /* get timestamp */ 0; // Platform-specific timestamp
    
    for (uint32_t i = 0; i < g_ui_masking_ctx.count; i++) {
        ui_rendering_ui_masking_internal_t* item = &g_ui_masking_ctx.items[i];
        if (item->initialized && item->dirty) {
            /* Process each mask in the item */
            for (uint32_t j = 0; j < item->mask_count; j++) {
                ui_mask_data_t* mask = &item->masks[j];
                if (mask->enabled) {
                    /* Check cache first */
                    uint64_t hash = ui_masking_calculate_hash(mask);
                    bool cache_hit = false;
                    
                    for (uint32_t k = 0; k < UI_RENDERING_UI_MASKING_CACHE_SIZE; k++) {
                        if (g_ui_masking_ctx.mask_cache[k].valid && 
                            g_ui_masking_ctx.mask_cache[k].hash == hash) {
                            cache_hit = true;
                            g_ui_masking_ctx.stats.cache_hits++;
                            g_ui_masking_ctx.mask_cache[k].last_used = start_time;
                            break;
                        }
                    }
                    
                    if (!cache_hit) {
                        /* Apply mask to GPU */
                        ui_masking_apply_mask_gpu(mask);
                        g_ui_masking_ctx.stats.cache_misses++;
                        
                        /* Add to cache */
                        uint32_t cache_index = hash % UI_RENDERING_UI_MASKING_CACHE_SIZE;
                        g_ui_masking_ctx.mask_cache[cache_index].hash = hash;
                        g_ui_masking_ctx.mask_cache[cache_index].mask = *mask;
                        g_ui_masking_ctx.mask_cache[cache_index].last_used = start_time;
                        g_ui_masking_ctx.mask_cache[cache_index].valid = true;
                    }
                }
            }
            
            item->dirty = false;
            processed++;
        }
    }
    
    uint64_t end_time = /* get timestamp */ 0; // Platform-specific timestamp
    g_ui_masking_ctx.stats.total_time_ns += (end_time - start_time);
    
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    return processed;
}

uint32_t ui_rendering_ui_masking_get_count(void) {
    if (!g_ui_masking_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    uint32_t count = g_ui_masking_ctx.count;
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    
    return count;
}

size_t ui_rendering_ui_masking_get_memory_usage(void) {
    if (!g_ui_masking_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    
    size_t total = sizeof(g_ui_masking_ctx);
    total += g_ui_masking_ctx.capacity * sizeof(ui_rendering_ui_masking_internal_t);
    
    /* Calculate mask data memory */
    for (uint32_t i = 0; i < g_ui_masking_ctx.count; i++) {
        const ui_rendering_ui_masking_internal_t* item = &g_ui_masking_ctx.items[i];
        total += item->mask_capacity * sizeof(ui_mask_data_t);
        
        /* Add polygon vertex memory */
        for (uint32_t j = 0; j < item->mask_count; j++) {
            if (item->masks[j].type == UI_MASK_TYPE_POLYGON && item->masks[j].data.polygon.vertices) {
                total += item->masks[j].data.polygon.vertex_count * 2 * sizeof(float);
            }
        }
    }
    
    /* Add cache memory */
    total += UI_RENDERING_UI_MASKING_CACHE_SIZE * sizeof(ui_mask_cache_entry_t);
    
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    return total;
}

void ui_rendering_ui_masking_debug_print(void) {
    if (!g_ui_masking_ctx.initialized) {
        printf("UI Masking System: Not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_ui_masking_ctx.global_mutex);
    
    printf("=== UI Masking System Debug Info ===\n");
    printf("Initialized: %s\n", g_ui_masking_ctx.initialized ? "Yes" : "No");
    printf("Total Items: %u / %u\n", g_ui_masking_ctx.count, g_ui_masking_ctx.capacity);
    printf("Stencil Enabled: %s\n", g_ui_masking_ctx.stencil_enabled ? "Yes" : "No");
    printf("Scissor Enabled: %s\n", g_ui_masking_ctx.scissor_enabled ? "Yes" : "No");
    printf("Current Stencil Ref: %u\n", g_ui_masking_ctx.current_stencil_ref);
    
    printf("\n=== Performance Statistics ===\n");
    printf("Mask Operations: %u\n", g_ui_masking_ctx.stats.mask_operations);
    printf("Stencil Operations: %u\n", g_ui_masking_ctx.stats.stencil_operations);
    printf("Scissor Operations: %u\n", g_ui_masking_ctx.stats.scissor_operations);
    printf("Cache Hits: %u\n", g_ui_masking_ctx.stats.cache_hits);
    printf("Cache Misses: %u\n", g_ui_masking_ctx.stats.cache_misses);
    printf("Total Processing Time: %lu ns\n", g_ui_masking_ctx.stats.total_time_ns);
    
    if (g_ui_masking_ctx.stats.cache_hits + g_ui_masking_ctx.stats.cache_misses > 0) {
        float hit_rate = (float)g_ui_masking_ctx.stats.cache_hits / 
                       (g_ui_masking_ctx.stats.cache_hits + g_ui_masking_ctx.stats.cache_misses) * 100.0f;
        printf("Cache Hit Rate: %.2f%%\n", hit_rate);
    }
    
    printf("\n=== Item Details ===\n");
    for (uint32_t i = 0; i < g_ui_masking_ctx.count; i++) {
        const ui_rendering_ui_masking_internal_t* item = &g_ui_masking_ctx.items[i];
        if (item->initialized) {
            printf("Item %u: %u masks, dirty=%s, clipping=%s\n", 
                   i, item->mask_count, 
                   item->dirty ? "Yes" : "No",
                   item->clipping_enabled ? "Yes" : "No");
        }
    }
    
    printf("\n=== Cache Status ===\n");
    uint32_t valid_cache_entries = 0;
    for (uint32_t i = 0; i < UI_RENDERING_UI_MASKING_CACHE_SIZE; i++) {
        if (g_ui_masking_ctx.mask_cache[i].valid) {
            valid_cache_entries++;
        }
    }
    printf("Valid Cache Entries: %u / %u\n", valid_cache_entries, UI_RENDERING_UI_MASKING_CACHE_SIZE);
    
    pthread_mutex_unlock(&g_ui_masking_ctx.global_mutex);
    printf("=== End UI Masking Debug Info ===\n");
}

/* End of ui_masking.c */
