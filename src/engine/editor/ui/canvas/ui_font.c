/*
 * ui_font.c
 * Font rendering
 *
 * Part of the Ui Rendering subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement UI batching
 * TODO: Add SDF text rendering
 * TODO: Implement UI effects
 * TODO: Add 9-patch sprites
 * TODO: Implement UI gradients
 * TODO: Add UI animations
 * TODO: Implement UI clipping
 * TODO: Add UI render targets
 * TODO: Implement ui font initialization
 * TODO: Add ui font cleanup/shutdown
 * TODO: Implement ui font validation
 * TODO: Add ui font error handling
 * TODO: Implement ui font serialization
 * TODO: Add ui font debug output
 * TODO: Implement ui font unit tests
 * TODO: Add ui font performance counters
 * TODO: Implement ui font hot-reload
 * TODO: Add ui font thread safety
 * TODO: Implement ui font memory pooling
 * TODO: Add ui font caching layer
 * TODO: Implement ui font async operations
 * TODO: Add ui font GPU integration
 * TODO: Implement ui font SIMD optimization
 * TODO: Add ui font batch processing
 * TODO: Implement ui font streaming support
 * TODO: Add ui font LOD support
 * TODO: Implement ui font culling integration
 * TODO: Add ui font render graph node
 */

#include "editor/ui/canvas/ui_font.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <ft2build.h>
#include FT_FREETYPE_H

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define UI_RENDERING_UI_FONT_MAX_COUNT 4096
#define UI_RENDERING_UI_FONT_DEFAULT_CAPACITY 256
#define UI_RENDERING_UI_FONT_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ui_rendering_ui_font_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    ui_font_face_t* font_faces;
    uint32_t font_count;
    uint32_t font_capacity;
    glyph_cache_t* glyph_cache;
    uint32_t cache_size;
    uint32_t cache_capacity;
    glyph_storage_t* glyphs;
    uint32_t glyph_count;
    uint32_t glyph_capacity;
    uint32_t atlas_width;
    uint32_t atlas_height;
    void* atlas_data;
} ui_rendering_ui_font_internal_t;

typedef struct ui_rendering_ui_font_context {
    ui_rendering_ui_font_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    pthread_mutex_t global_mutex;
    FT_Library ft_library;
    bool ft_initialized;
    uint32_t current_texture_unit;
    bool texture_upload_enabled;
    ui_rendering_ui_font_stats_t stats;
} ui_rendering_ui_font_context_t;

static ui_rendering_ui_font_context_t g_ui_font_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool ui_rendering_ui_font_validate(const ui_rendering_ui_font_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->font_faces || item->font_count == 0) return false;
    
    /* Validate glyph cache */
    if (!item->glyph_cache || item->cache_size == 0) return false;
    
    /* Validate atlas texture */
    if (item->atlas_width == 0 || item->atlas_height == 0) return false;
    
    return true;
}

static void ui_rendering_ui_font_cleanup_internal(ui_rendering_ui_font_internal_t* item) {
    if (!item) return;
    
    /* Clean up font faces */
    if (item->font_faces) {
        for (uint32_t i = 0; i < item->font_count; i++) {
            ui_font_face_t* face = &item->font_faces[i];
#ifdef ENABLE_FREETYPE
            if (face->ft_face) {
                FT_Done_Face(face->ft_face);
            }
#endif
        }
        free(item->font_faces);
        item->font_faces = NULL;
    }
    
    /* Clean up glyph cache */
    if (item->glyph_cache) {
        for (uint32_t i = 0; i < item->cache_size; i++) {
            if (item->glyph_cache[i].glyph.bitmap_data) {
                free(item->glyph_cache[i].glyph.bitmap_data);
            }
        }
        free(item->glyph_cache);
        item->glyph_cache = NULL;
    }
    
    /* Clean up glyph storage */
    if (item->glyphs) {
        for (uint32_t i = 0; i < item->glyph_count; i++) {
            if (item->glyphs[i].bitmap_data) {
                free(item->glyphs[i].bitmap_data);
            }
        }
        free(item->glyphs);
        item->glyphs = NULL;
    }
    
    /* Clean up atlas data */
    if (item->atlas_data) {
        free(item->atlas_data);
        item->atlas_data = NULL;
    }
    
    /* Clean up legacy data */
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    item->font_count = 0;
    item->font_capacity = 0;
    item->cache_size = 0;
    item->cache_capacity = 0;
    item->glyph_count = 0;
    item->glyph_capacity = 0;
    item->atlas_width = 0;
    item->atlas_height = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int ui_rendering_ui_font_init(void) {
    if (g_ui_font_ctx.initialized) {
        return 0; // Already initialized
    }

    /* Initialize FreeType library */
#ifdef ENABLE_FREETYPE
    if (FT_Init_FreeType(&g_ui_font_ctx.ft_library) != 0) {
        return -1; // FreeType initialization failed
    }
    g_ui_font_ctx.ft_initialized = true;
#endif

    /* Initialize mutex */
    if (pthread_mutex_init(&g_ui_font_ctx.global_mutex, NULL) != 0) {
#ifdef ENABLE_FREETYPE
        if (g_ui_font_ctx.ft_initialized) {
            FT_Done_FreeType(g_ui_font_ctx.ft_library);
        }
#endif
        return -2;
    }

    /* Initialize statistics */
    memset(&g_ui_font_ctx.stats, 0, sizeof(g_ui_font_ctx.stats));
    
    /* Initialize GPU state */
    g_ui_font_ctx.current_texture_unit = 0;
    g_ui_font_ctx.texture_upload_enabled = true;

    g_ui_font_ctx.capacity = UI_RENDERING_UI_FONT_DEFAULT_CAPACITY;
    g_ui_font_ctx.items = calloc(g_ui_font_ctx.capacity, sizeof(ui_rendering_ui_font_internal_t));
    if (!g_ui_font_ctx.items) {
        pthread_mutex_destroy(&g_ui_font_ctx.global_mutex);
#ifdef ENABLE_FREETYPE
        if (g_ui_font_ctx.ft_initialized) {
            FT_Done_FreeType(g_ui_font_ctx.ft_library);
        }
#endif
        return -3;
    }

    g_ui_font_ctx.count = 0;
    g_ui_font_ctx.initialized = true;

    return 0;
}

void ui_rendering_ui_font_shutdown(void) {
    // TODO: Implement UI clipping
    // TODO: Add UI render targets
    // TODO: Implement ui font initialization
    // TODO: Add ui font cleanup/shutdown

    if (!g_ui_font_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ui_font_ctx.count; i++) {
        ui_rendering_ui_font_cleanup_internal(&g_ui_font_ctx.items[i]);
    }

    free(g_ui_font_ctx.items);
    g_ui_font_ctx.items = NULL;
    g_ui_font_ctx.count = 0;
    g_ui_font_ctx.capacity = 0;
    g_ui_font_ctx.initialized = false;
}

int ui_rendering_ui_font_create(ui_rendering_ui_font_handle_t* out_handle, const ui_rendering_ui_font_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ui_font_ctx.initialized) {
        return -2;
    }

    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);

    /* Find free slot */
    uint32_t index = 0;
    bool found = false;
    for (uint32_t i = 0; i < g_ui_font_ctx.count; i++) {
        if (!g_ui_font_ctx.items[i].initialized) {
            index = i;
            found = true;
            break;
        }
    }

    /* Allocate new slot if needed */
    if (!found) {
        if (g_ui_font_ctx.count >= g_ui_font_ctx.capacity) {
            uint32_t new_capacity = g_ui_font_ctx.capacity * 2;
            ui_rendering_ui_font_internal_t* new_items = realloc(g_ui_font_ctx.items, 
                new_capacity * sizeof(ui_rendering_ui_font_internal_t));
            if (!new_items) {
                pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
                return -3;
            }
            g_ui_font_ctx.items = new_items;
            g_ui_font_ctx.capacity = new_capacity;
        }
        index = g_ui_font_ctx.count++;
    }

    ui_rendering_ui_font_internal_t* item = &g_ui_font_ctx.items[index];
    memset(item, 0, sizeof(ui_rendering_ui_font_internal_t));

    /* Initialize font system */
    item->id = desc->id;
    item->flags = desc->flags;
    item->sdf_enabled = (desc->flags & 0x01) != 0; // SDF flag
    item->current_font_size = UI_RENDERING_UI_FONT_MIN_FONT_SIZE;
    item->padding = UI_RENDERING_UI_FONT_SDF_PADDING;
    
    /* Initialize font faces */
    item->font_capacity = 4;
    item->font_faces = calloc(item->font_capacity, sizeof(ui_font_face_t));
    if (!item->font_faces) {
        pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
        return -4;
    }
    
    /* Initialize glyph cache */
    item->cache_capacity = UI_RENDERING_UI_FONT_CACHE_SIZE;
    item->glyph_cache = calloc(item->cache_capacity, sizeof(ui_glyph_cache_entry_t));
    if (!item->glyph_cache) {
        free(item->font_faces);
        pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
        return -5;
    }
    
    /* Initialize glyph storage */
    item->glyph_capacity = UI_RENDERING_UI_FONT_MAX_GLYPHS;
    item->glyphs = calloc(item->glyph_capacity, sizeof(ui_glyph_t));
    if (!item->glyphs) {
        free(item->font_faces);
        free(item->glyph_cache);
        pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
        return -6;
    }
    
    /* Initialize atlas texture */
    item->atlas_width = 1024;
    item->atlas_height = 1024;
    item->atlas_data = calloc(item->atlas_width * item->atlas_height, 4); // RGBA
    if (!item->atlas_data) {
        free(item->font_faces);
        free(item->glyph_cache);
        free(item->glyphs);
        pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
        return -7;
    }
    
    item->font_count = 0;
    item->cache_size = 0;
    item->glyph_count = 0;
    
    item->initialized = true;
    item->dirty = false;
    item->frame_updated = 0;

    out_handle->id = index;
    
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
    return 0;
}

void ui_rendering_ui_font_destroy(ui_rendering_ui_font_handle_t handle) {
    if (!g_ui_font_ctx.initialized || handle.id >= g_ui_font_ctx.count) {
        return;
    }

    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    ui_rendering_ui_font_cleanup_internal(&g_ui_font_ctx.items[handle.id]);
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
}

int ui_rendering_ui_font_update(ui_rendering_ui_font_handle_t handle, const void* data, size_t size) {
    if (!g_ui_font_ctx.initialized || handle.id >= g_ui_font_ctx.count) {
        return -1;
    }

    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    
    ui_rendering_ui_font_internal_t* item = &g_ui_font_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
        return -2;
    }
    
    /* Update legacy data if provided */
    if (data && size > 0) {
        if (item->data) {
            free(item->data);
        }
        item->data = malloc(size);
        if (!item->data) {
            pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
            return -3;
        }
        memcpy(item->data, data, size);
        item->data_size = size;
    }
    
    item->dirty = true;
    item->frame_updated = g_ui_font_ctx.stats.glyph_rasterizations;
    
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
    return 0;
}

bool ui_rendering_ui_font_is_valid(ui_rendering_ui_font_handle_t handle) {
    // TODO: Add ui font batch processing
    if (handle.id >= g_ui_font_ctx.count) {
        return false;
    }
    return g_ui_font_ctx.items[handle.id].initialized;
}

int ui_rendering_ui_font_get_info(ui_rendering_ui_font_handle_t handle, ui_rendering_ui_font_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (!g_ui_font_ctx.initialized || handle.id >= g_ui_font_ctx.count) {
        return -2;
    }

    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    
    const ui_rendering_ui_font_internal_t* item = &g_ui_font_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
    return 0;
}

void ui_rendering_ui_font_mark_dirty(ui_rendering_ui_font_handle_t handle) {
    if (!g_ui_font_ctx.initialized || handle.id >= g_ui_font_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    g_ui_font_ctx.items[handle.id].dirty = true;
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
}

int ui_rendering_ui_font_process_pending(void) {
    if (!g_ui_font_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    
    int processed = 0;
    uint64_t start_time = 0; // Platform-specific timestamp
    
    for (uint32_t i = 0; i < g_ui_font_ctx.count; i++) {
        ui_rendering_ui_font_internal_t* item = &g_ui_font_ctx.items[i];
        if (item->initialized && item->dirty) {
            /* Process texture uploads if needed */
            if (item->atlas_data && g_ui_font_ctx.texture_upload_enabled) {
                /* Upload atlas to GPU */
#ifdef ENABLE_OPENGL
                if (item->glyph_texture_id == 0) {
                    glGenTextures(1, &item->glyph_texture_id);
                }
                glBindTexture(GL_TEXTURE_2D, item->glyph_texture_id);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                           item->atlas_width, item->atlas_height, 0, 
                           GL_RGBA, GL_UNSIGNED_BYTE, item->atlas_data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
                g_ui_font_ctx.stats.texture_uploads++;
            }
            
            item->dirty = false;
            processed++;
        }
    }
    
    uint64_t end_time = 0; // Platform-specific timestamp
    g_ui_font_ctx.stats.total_rasterization_time_ns += (end_time - start_time);
    
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
    return processed;
}

uint32_t ui_rendering_ui_font_get_count(void) {
    if (!g_ui_font_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    uint32_t count = g_ui_font_ctx.count;
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
    
    return count;
}

size_t ui_rendering_ui_font_get_memory_usage(void) {
    if (!g_ui_font_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    
    size_t total = sizeof(g_ui_font_ctx);
    total += g_ui_font_ctx.capacity * sizeof(ui_rendering_ui_font_internal_t);
    
    /* Calculate font data memory */
    for (uint32_t i = 0; i < g_ui_font_ctx.count; i++) {
        const ui_rendering_ui_font_internal_t* item = &g_ui_font_ctx.items[i];
        total += item->font_capacity * sizeof(ui_font_face_t);
        total += item->cache_capacity * sizeof(ui_glyph_cache_entry_t);
        total += item->glyph_capacity * sizeof(ui_glyph_t);
        
        /* Add glyph bitmap memory */
        for (uint32_t j = 0; j < item->glyph_count; j++) {
            total += item->glyphs[j].bitmap_size;
        }
        
        /* Add atlas memory */
        total += item->atlas_width * item->atlas_height * 4; // RGBA
        
        /* Add legacy data memory */
        total += item->data_size;
    }
    
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
    return total;
}

void ui_rendering_ui_font_debug_print(void) {
    if (!g_ui_font_ctx.initialized) {
        printf("UI Font System: Not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_ui_font_ctx.global_mutex);
    
    printf("=== UI Font System Debug Info ===\n");
    printf("Initialized: %s\n", g_ui_font_ctx.initialized ? "Yes" : "No");
    printf("Total Items: %u / %u\n", g_ui_font_ctx.count, g_ui_font_ctx.capacity);
    printf("Texture Upload Enabled: %s\n", g_ui_font_ctx.texture_upload_enabled ? "Yes" : "No");
    printf("Current Texture Unit: %u\n", g_ui_font_ctx.current_texture_unit);
    printf("FreeType Initialized: %s\n", g_ui_font_ctx.ft_initialized ? "Yes" : "No");
    
    printf("\n=== Performance Statistics ===\n");
    printf("Glyph Rasterizations: %u\n", g_ui_font_ctx.stats.glyph_rasterizations);
    printf("SDF Generations: %u\n", g_ui_font_ctx.stats.sdf_generations);
    printf("Cache Hits: %u\n", g_ui_font_ctx.stats.cache_hits);
    printf("Cache Misses: %u\n", g_ui_font_ctx.stats.cache_misses);
    printf("Font Loads: %u\n", g_ui_font_ctx.stats.font_loads);
    printf("Texture Uploads: %u\n", g_ui_font_ctx.stats.texture_uploads);
    printf("Total Rasterization Time: %lu ns\n", g_ui_font_ctx.stats.total_rasterization_time_ns);
    printf("Cache Hit Rate: %.2f%%\n", g_ui_font_ctx.stats.cache_hit_rate * 100.0f);
    printf("Active Glyphs: %u\n", g_ui_font_ctx.stats.active_glyphs);
    printf("Glyph Memory Usage: %zu bytes\n", g_ui_font_ctx.stats.glyph_memory_usage);
    
    printf("\n=== Item Details ===\n");
    for (uint32_t i = 0; i < g_ui_font_ctx.count; i++) {
        const ui_rendering_ui_font_internal_t* item = &g_ui_font_ctx.items[i];
        if (item->initialized) {
            printf("Item %u: %u fonts, %u glyphs, %u cached, dirty=%s, atlas=%ux%u, SDF=%s\n", 
                   i, item->font_count, item->glyph_count, item->cache_size,
                   item->dirty ? "Yes" : "No",
                   item->atlas_width, item->atlas_height,
                   item->sdf_enabled ? "Yes" : "No");
        }
    }
    
    pthread_mutex_unlock(&g_ui_font_ctx.global_mutex);
    printf("=== End UI Font Debug Info ===\n");
}

/* End of ui_font.c */
