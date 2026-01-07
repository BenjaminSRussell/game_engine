/*
 * texture_defaults.c
 * Default textures (white, normal, black)
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "materials/textures/texture_defaults.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_texture_defaults_context {
    uint32_t white_texture;
    uint32_t black_texture;
    uint32_t grey_texture;
    uint32_t normal_texture;
    uint32_t magenta_texture; // Error texture
    bool initialized;
} materials_texture_defaults_context_t;

static materials_texture_defaults_context_t g_texture_defaults_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Mock texture creation helper
static uint32_t create_1x1_texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    // In real engine: texture_create(...)
    // Here we just return non-zero IDs
    static uint32_t next_id = 900000; 
    return ++next_id;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_texture_defaults_init(void) {
    if (g_texture_defaults_ctx.initialized) return 0;
    
    // Create 1x1 default textures
    g_texture_defaults_ctx.white_texture = create_1x1_texture(255, 255, 255, 255);
    g_texture_defaults_ctx.black_texture = create_1x1_texture(0, 0, 0, 255);
    g_texture_defaults_ctx.grey_texture = create_1x1_texture(128, 128, 128, 255);
    g_texture_defaults_ctx.normal_texture = create_1x1_texture(128, 128, 255, 255); // Flat normal in tangent space
    g_texture_defaults_ctx.magenta_texture = create_1x1_texture(255, 0, 255, 255);
    
    g_texture_defaults_ctx.initialized = true;
    return 0;
}

void materials_texture_defaults_shutdown(void) {
    if (!g_texture_defaults_ctx.initialized) return;
    
    // Destroy textures
    // texture_destroy(...)
    
    g_texture_defaults_ctx.initialized = false;
}

int materials_texture_defaults_create(materials_texture_defaults_handle_t* out_handle, const materials_texture_defaults_desc_t* desc) {
    // Defaults are singleton, usually handle creation here isn't needed unless extending defaults
    if (out_handle) out_handle->id = 0;
    return 0;
}

void materials_texture_defaults_destroy(materials_texture_defaults_handle_t handle) {
}

int materials_texture_defaults_update(materials_texture_defaults_handle_t handle, const void* data, size_t size) {
    return 0;
}

bool materials_texture_defaults_is_valid(materials_texture_defaults_handle_t handle) {
    return g_texture_defaults_ctx.initialized;
}

int materials_texture_defaults_get_info(materials_texture_defaults_handle_t handle, materials_texture_defaults_info_t* out_info) {
    if (out_info) {
        out_info->id = 0;
        out_info->flags = 0;
        out_info->initialized = g_texture_defaults_ctx.initialized;
    }
    return 0;
}

void materials_texture_defaults_mark_dirty(materials_texture_defaults_handle_t handle) {
}

int materials_texture_defaults_process_pending(void) {
    return 0;
}

uint32_t materials_texture_defaults_get_count(void) {
    return 5; // Fixed set
}

size_t materials_texture_defaults_get_memory_usage(void) {
    return 4 * 5; // 4 bytes per 1x1 pixel * 5 textures
}

void materials_texture_defaults_debug_print(void) {
    printf("Texture Defaults: initialized\n");
}

/* Accessors */
uint32_t materials_texture_defaults_get_white(void) {
    return g_texture_defaults_ctx.initialized ? g_texture_defaults_ctx.white_texture : 0;
}

uint32_t materials_texture_defaults_get_black(void) {
    return g_texture_defaults_ctx.initialized ? g_texture_defaults_ctx.black_texture : 0;
}

uint32_t materials_texture_defaults_get_normal(void) {
    return g_texture_defaults_ctx.initialized ? g_texture_defaults_ctx.normal_texture : 0;
}
