/*
 * lut_system.c
 * 3D LUT color grading system implementation
 */

#include "postprocess/color_grading/lut_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <include/math/math.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

#define MAX_LUTS 16

typedef struct lut_entry {
    lut_3d_t* lut;
    char name[64];
    bool in_use;
} lut_entry_t;

struct lut_system {
    lut_entry_t luts[MAX_LUTS];
    uint32_t lut_count;
    
    lut_3d_t* active_lut;
    lut_3d_t* blend_source;
    lut_3d_t* blend_target;
    float blend_progress;         // 0.0 to 1.0
    float blend_speed;            // Units per second    
    bool is_blending;
};

static bool g_initialized = false;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

// Trilinear interpolation in 3D LUT
static void sample_lut_3d(const lut_3d_t* lut, float r, float g, float b,
                         float* out_r, float* out_g, float* out_b) {
    if (!lut || !lut->data) {
        *out_r = r;
        *out_g = g;
        *out_b = b;
        return;
    }
    
    // Clamp input to [0, 1]
    r = fmaxf(0.0f, fminf(1.0f, r));
    g = fmaxf(0.0f, fminf(1.0f, g));
    b = fmaxf(0.0f, fminf(1.0f, b));
    
    // Scale to LUT coordinates
    float fr = r * (float)(lut->size - 1);
    float fg = g * (float)(lut->size - 1);
    float fb = b * (float)(lut->size - 1);
    
    // Get integer indices
    uint32_t r0 = (uint32_t)floorf(fr);
    uint32_t g0 = (uint32_t)floorf(fg);
    uint32_t b0 = (uint32_t)floorf(fb);
    
    uint32_t r1 = fminf(r0 + 1, lut->size - 1);
    uint32_t g1 = fminf(g0 + 1, lut->size - 1);
    uint32_t b1 = fminf(b0 + 1, lut->size - 1);
    
    // Get fractional parts
    float dr = fr - (float)r0;
    float dg = fg - (float)g0;
    float db = fb - (float)b0;
    
    // Sample 8 corners of the cube
    #define LUT_INDEX(R, G, B) ((B) * lut->size * lut->size + (G) * lut->size + (R))
    
    const float* c000 = &lut->data[LUT_INDEX(r0, g0, b0) * 3];
    const float* c100 = &lut->data[LUT_INDEX(r1, g0, b0) * 3];
    const float* c010 = &lut->data[LUT_INDEX(r0, g1, b0) * 3];
    const float* c110 = &lut->data[LUT_INDEX(r1, g1, b0) * 3];
    const float* c001 = &lut->data[LUT_INDEX(r0, g0, b1) * 3];
    const float* c101 = &lut->data[LUT_INDEX(r1, g0, b1) * 3];
    const float* c011 = &lut->data[LUT_INDEX(r0, g1, b1) * 3];
    const float* c111 = &lut->data[LUT_INDEX(r1, g1, b1) * 3];
    
    #undef LUT_INDEX
    
    // Trilinear interpolation
    for (int i = 0; i < 3; i++) {
        float c00 = c000[i] * (1.0f - dr) + c100[i] * dr;
        float c01 = c001[i] * (1.0f - dr) + c101[i] * dr;
        float c10 = c010[i] * (1.0f - dr) + c110[i] * dr;
        float c11 = c011[i] * (1.0f - dr) + c111[i] * dr;
        
        float c0 = c00 * (1.0f - dg) + c10 * dg;
        float c1 = c01 * (1.0f - dg) + c11 * dg;
        
        float result = c0 * (1.0f - db) + c1 * db;
        
        if (i == 0) *out_r = result;
        else if (i == 1) *out_g = result;
        else *out_b = result;
    }
}

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int lut_system_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    g_initialized = true;
    return 0;
}

void lut_system_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    g_initialized = false;
}

/* ============================================================================
 * LIFECYCLE
 * ============================================================================ */

lut_system_t* lut_system_create(void) {
    if (!g_initialized) {
        return NULL;
    }
    
    lut_system_t* system = (lut_system_t*)calloc(1, sizeof(lut_system_t));
    if (!system) {
        return NULL;
    }
    
    // Create default identity LUT
    system->active_lut = lut_3d_create_identity(LUT_DEFAULT_SIZE);
    if (!system->active_lut) {
        free(system);
        return NULL;
    }
    
    return system;
}

void lut_system_destroy(lut_system_t* system) {
    if (!system) {
        return;
    }
    
    // Free all loaded LUTs
    for (uint32_t i = 0; i < MAX_LUTS; i++) {
        if (system->luts[i].in_use && system->luts[i].lut) {
            lut_3d_destroy(system->luts[i].lut);
        }
    }
    
    // Free blend temporaries
    if (system->blend_source && system->blend_source != system->active_lut) {
        lut_3d_destroy(system->blend_source);
    }
    if (system->blend_target) {
        lut_3d_destroy(system->blend_target);
    }
    
    // Free active LUT
    if (system->active_lut) {
        lut_3d_destroy(system->active_lut);
    }
    
    free(system);
}

/* ============================================================================
 * LUT CREATION
 * ============================================================================ */

lut_3d_t* lut_3d_create_identity(uint32_t size) {
    lut_3d_t* lut = (lut_3d_t*)calloc(1, sizeof(lut_3d_t));
    if (!lut) {
        return NULL;
    }
    
    lut->size = size;
    snprintf(lut->name, sizeof(lut->name), "Identity_%u", size);
    
    size_t data_size = size * size * size * 3 * sizeof(float);
    lut->data = (float*)malloc(data_size);
    if (!lut->data) {
        free(lut);
        return NULL;
    }
    
    // Fill with identity mapping
    for (uint32_t b = 0; b < size; b++) {
        for (uint32_t g = 0; g < size; g++) {
            for (uint32_t r = 0; r < size; r++) {
                uint32_t idx = (b * size * size + g * size + r) * 3;
                lut->data[idx + 0] = (float)r / (float)(size - 1);
                lut->data[idx + 1] = (float)g / (float)(size - 1);
                lut->data[idx + 2] = (float)b / (float)(size - 1);
            }
        }
    }
    
    return lut;
}

lut_3d_t* lut_3d_create_from_file(const char* filepath) {
    if (!filepath) {
        return NULL;
    }
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        return NULL;
    }
    
    lut_3d_t* lut = (lut_3d_t*)calloc(1, sizeof(lut_3d_t));
    if (!lut) {
        fclose(file);
        return NULL;
    }
    
    char line[256];
    uint32_t size = 0;
    float min_range[3] = {0, 0, 0};
    float max_range[3] = {1, 1, 1};
    uint32_t current_pixel = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        if (strncmp(line, "TITLE", 5) == 0) {
            sscanf(line, "TITLE \"%[^\"]\"", lut->name);
        } else if (strncmp(line, "LUT_3D_SIZE", 11) == 0) {
            sscanf(line, "LUT_3D_SIZE %u", &size);
            if (size > 0 && size <= 128) { // Reasonable limit
                lut->size = size;
                lut->data = (float*)malloc(size * size * size * 3 * sizeof(float));
            }
        } else if (strncmp(line, "DOMAIN_MIN", 10) == 0) {
            sscanf(line, "DOMAIN_MIN %f %f %f", &min_range[0], &min_range[1], &min_range[2]);
        } else if (strncmp(line, "DOMAIN_MAX", 10) == 0) {
            sscanf(line, "DOMAIN_MAX %f %f %f", &max_range[0], &max_range[1], &max_range[2]);
        } else {
            // Data line
            float r, g, b;
            if (sscanf(line, "%f %f %f", &r, &g, &b) == 3) {
                if (lut->data && current_pixel < lut->size * lut->size * lut->size) {
                    uint32_t idx = current_pixel * 3;
                    lut->data[idx + 0] = r;
                    lut->data[idx + 1] = g;
                    lut->data[idx + 2] = b;
                    current_pixel++;
                }
            }
        }
    }
    
    fclose(file);
    
    if (size == 0 || !lut->data || current_pixel < size * size * size) {
        lut_3d_destroy(lut);
        return NULL;
    }
    
    return lut;
}

lut_3d_t* lut_3d_create_from_data(const float* data, uint32_t size) {
    if (!data) {
        return NULL;
    }
    
    lut_3d_t* lut = (lut_3d_t*)calloc(1, sizeof(lut_3d_t));
    if (!lut) {
        return NULL;
    }
    
    lut->size = size;
    snprintf(lut->name, sizeof(lut->name), "Custom_%u", size);
    
    size_t data_size = size * size * size * 3 * sizeof(float);
    lut->data = (float*)malloc(data_size);
    if (!lut->data) {
        free(lut);
        return NULL;
    }
    
    memcpy(lut->data, data, data_size);
    
    return lut;
}

void lut_3d_destroy(lut_3d_t* lut) {
    if (!lut) {
        return;
    }
    
    free(lut->data);
    free(lut);
}

/* ============================================================================
 * LUT OPERATIONS
 * ============================================================================ */

int lut_3d_save_to_file(const lut_3d_t* lut, const char* filepath) {
    if (!lut || !filepath || !lut->data) {
        return -1;
    }
    
    FILE* file = fopen(filepath, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "# Created by Antigravity Post-Processing System\n");
    fprintf(file, "TITLE \"%s\"\n", lut->name);
    fprintf(file, "LUT_3D_SIZE %u\n", lut->size);
    fprintf(file, "DOMAIN_MIN 0.0 0.0 0.0\n");
    fprintf(file, "DOMAIN_MAX 1.0 1.0 1.0\n");
    
    uint32_t pixel_count = lut->size * lut->size * lut->size;
    for (uint32_t i = 0; i < pixel_count; i++) {
        fprintf(file, "%.6f %.6f %.6f\n", lut->data[i*3], lut->data[i*3+1], lut->data[i*3+2]);
    }
    
    fclose(file);
    return 0;
}

lut_3d_t* lut_3d_blend(const lut_3d_t* lut_a, const lut_3d_t* lut_b, float blend_factor) {
    if (!lut_a || !lut_b || lut_a->size != lut_b->size) {
        return NULL;
    }
    
    lut_3d_t* result = (lut_3d_t*)calloc(1, sizeof(lut_3d_t));
    if (!result) {
        return NULL;
    }
    
    result->size = lut_a->size;
    snprintf(result->name, sizeof(result->name), "Blended");
    
    size_t data_size = result->size * result->size * result->size * 3 * sizeof(float);
    result->data = (float*)malloc(data_size);
    if (!result->data) {
        free(result);
        return NULL;
    }
    
    // Linear blend
    blend_factor = fmaxf(0.0f, fminf(1.0f, blend_factor));
    size_t element_count = data_size / sizeof(float);
    
    for (size_t i = 0; i < element_count; i++) {
        result->data[i] = lut_a->data[i] * (1.0f - blend_factor) + 
                         lut_b->data[i] * blend_factor;
    }
    
    return result;
}

/* ============================================================================
 * COLOR APPLICATION
 * ============================================================================ */

void lut_3d_apply_color(const lut_3d_t* lut, float r, float g, float b,
                       float* out_r, float* out_g, float* out_b) {
    sample_lut_3d(lut, r, g, b, out_r, out_g, out_b);
}

void lut_3d_apply_buffer(const lut_3d_t* lut, const float* input_rgb,
                        float* output_rgb, uint32_t pixel_count) {
    if (!lut || !input_rgb || !output_rgb) {
        return;
    }
    
    for (uint32_t i = 0; i < pixel_count; i++) {
        float r = input_rgb[i * 3 + 0];
        float g = input_rgb[i * 3 + 1];
        float b = input_rgb[i * 3 + 2];
        
        sample_lut_3d(lut, r, g, b,
                     &output_rgb[i * 3 + 0],
                     &output_rgb[i * 3 + 1],
                     &output_rgb[i * 3 + 2]);
    }
}

/* ============================================================================
 * SYSTEM-LEVEL LUT MANAGEMENT
 * ============================================================================ */

int lut_system_load_lut(lut_system_t* system, const char* name, const char* filepath) {
    if (!system || !name || !filepath) {
        return -1;
    }
    
    // Find empty slot
    int slot = -1;
    for (uint32_t i = 0; i < MAX_LUTS; i++) {
        if (!system->luts[i].in_use) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        return -1; // No slots available
    }
    
    // Load LUT
    lut_3d_t* lut = lut_3d_create_from_file(filepath);
    if (!lut) {
        return -1;
    }
    
    system->luts[slot].lut = lut;
    strncpy(system->luts[slot].name, name, sizeof(system->luts[slot].name) - 1);
    system->luts[slot].in_use = true;
    system->lut_count++;
    
    return 0;
}

int lut_system_set_active_lut(lut_system_t* system, const char* name) {
    if (!system || !name) {
        return -1;
    }
    
    // Find LUT by name
    for (uint32_t i = 0; i < MAX_LUTS; i++) {
        if (system->luts[i].in_use && strcmp(system->luts[i].name, name) == 0) {
            system->active_lut = system->luts[i].lut;
            return 0;
        }
    }
    
    return -1; // Not found
}

const lut_3d_t* lut_system_get_active_lut(const lut_system_t* system) {
    if (!system) {
        return NULL;
    }
    
    return system->active_lut;
}

void lut_system_clear_luts(lut_system_t* system) {
    if (!system) {
        return;
    }
    
    for (uint32_t i = 0; i < MAX_LUTS; i++) {
        if (system->luts[i].in_use) {
            lut_3d_destroy(system->luts[i].lut);
            system->luts[i].in_use = false;
            system->luts[i].lut = NULL;
        }
    }
    
    system->lut_count = 0;
}

/* ============================================================================
 * LUT BLENDING
 * ============================================================================ */

void lut_system_blend_to_lut(lut_system_t* system, const char* target_name,
                            float blend_duration_seconds) {
    if (!system || !target_name || blend_duration_seconds <= 0.0f) {
        return;
    }
    
    // Find target LUT
    lut_3d_t* target = NULL;
    for (uint32_t i = 0; i < MAX_LUTS; i++) {
        if (system->luts[i].in_use && strcmp(system->luts[i].name, target_name) == 0) {
            target = system->luts[i].lut;
            break;
        }
    }
    
    if (!target) {
        return;
    }
    
    // Set up blend
    system->blend_source = system->active_lut;
    system->blend_target = target;
    system->blend_progress = 0.0f;
    system->blend_speed = 1.0f / blend_duration_seconds;
    system->is_blending = true;
}

float lut_system_update_blend(lut_system_t* system, float delta_time) {
    if (!system || !system->is_blending) {
        return 1.0f; // Not blending
    }
    
    system->blend_progress += system->blend_speed * delta_time;
    
    if (system->blend_progress >= 1.0f) {
        // Blend complete
        system->blend_progress = 1.0f;
        system->active_lut = system->blend_target;
        system->is_blending = false;
        system->blend_source = NULL;
        system->blend_target = NULL;
    }
    
    return system->blend_progress;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

uint32_t lut_system_get_lut_count(const lut_system_t* system) {
    if (!system) {
        return 0;
    }
    
    return system->lut_count;
}

size_t lut_system_get_memory_usage(const lut_system_t* system) {
    if (!system) {
        return 0;
    }
    
    size_t total = sizeof(lut_system_t);
    
    for (uint32_t i = 0; i < MAX_LUTS; i++) {
        if (system->luts[i].in_use && system->luts[i].lut) {
            total += sizeof(lut_3d_t);
            total += system->luts[i].lut->size * system->luts[i].lut->size * 
                    system->luts[i].lut->size * 3 * sizeof(float);
        }
    }
    
    return total;
}
