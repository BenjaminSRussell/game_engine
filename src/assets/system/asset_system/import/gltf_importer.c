/*
 * gltf_importer.c
 * glTF 2.0 asset import
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements glTF JSON parsing and data extraction
 */

#include "assets/system/asset_system/import/gltf_importer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static double internal_atof(const char* str);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GLTF_IMPORTER_MAX_COUNT 32
#define GLTF_MAX_NODES 1024
#define GLTF_MAX_MESHES 256
#define GLTF_MAX_MATERIALS 256
#define GLTF_MAX_TEXTURES 256
#define GLTF_MAX_ANIMATIONS 64

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec2 { float x, y; } vec2_t;
typedef struct vec3 { float x, y, z; } vec3_t;
typedef struct vec4 { float x, y, z, w; } vec4_t;
typedef struct mat4 { float m[16]; } mat4_t;

/* ============================================================================
 * GLTF TYPES
 * ============================================================================ */

typedef enum gltf_component_type {
    GLTF_BYTE = 5120,
    GLTF_UNSIGNED_BYTE = 5121,
    GLTF_SHORT = 5122,
    GLTF_UNSIGNED_SHORT = 5123,
    GLTF_UNSIGNED_INT = 5125,
    GLTF_FLOAT = 5126
} gltf_component_type_t;

typedef enum gltf_attribute_type {
    GLTF_ATTR_POSITION = 0,
    GLTF_ATTR_NORMAL,
    GLTF_ATTR_TANGENT,
    GLTF_ATTR_TEXCOORD_0,
    GLTF_ATTR_TEXCOORD_1,
    GLTF_ATTR_COLOR_0,
    GLTF_ATTR_JOINTS_0,
    GLTF_ATTR_WEIGHTS_0
} gltf_attribute_type_t;

typedef struct gltf_accessor {
    uint32_t buffer_view;
    uint32_t byte_offset;
    gltf_component_type_t component_type;
    uint32_t count;
    uint32_t type;              // SCALAR=1, VEC2=2, VEC3=3, VEC4=4, MAT4=16
    bool normalized;
    vec3_t min;
    vec3_t max;
} gltf_accessor_t;

typedef struct gltf_buffer_view {
    uint32_t buffer;
    uint32_t byte_offset;
    uint32_t byte_length;
    uint32_t byte_stride;
    uint32_t target;            // 34962=ARRAY_BUFFER, 34963=ELEMENT_ARRAY_BUFFER
} gltf_buffer_view_t;

typedef struct gltf_buffer {
    uint32_t byte_length;
    char uri[256];
    uint8_t* data;
} gltf_buffer_t;

typedef struct gltf_primitive {
    uint32_t attributes[8];     // Accessor indices for each attribute
    uint32_t indices;           // Accessor index for indices
    uint32_t material;
    uint32_t mode;              // 0=points, 1=lines, 4=triangles
} gltf_primitive_t;

typedef struct gltf_mesh {
    char name[64];
    gltf_primitive_t* primitives;
    uint32_t primitive_count;
} gltf_mesh_t;

typedef struct gltf_material {
    char name[64];
    
    // PBR Metallic Roughness
    vec4_t base_color_factor;
    uint32_t base_color_texture;
    float metallic_factor;
    float roughness_factor;
    uint32_t metallic_roughness_texture;
    
    // Normal map
    uint32_t normal_texture;
    float normal_scale;
    
    // Occlusion
    uint32_t occlusion_texture;
    float occlusion_strength;
    
    // Emission
    vec3_t emissive_factor;
    uint32_t emissive_texture;
    
    // Alpha
    char alpha_mode[16];        // OPAQUE, MASK, BLEND
    float alpha_cutoff;
    bool double_sided;
} gltf_material_t;

typedef struct gltf_texture {
    uint32_t sampler;
    uint32_t source;            // Image index
} gltf_texture_t;

typedef struct gltf_image {
    char uri[256];
    char mime_type[32];
    uint32_t buffer_view;       // For embedded images
} gltf_image_t;

typedef struct gltf_node {
    char name[64];
    int32_t mesh;               // -1 if no mesh
    int32_t skin;               // -1 if no skin
    int32_t* children;
    uint32_t child_count;
    
    // Transform (TRS or matrix)
    bool has_matrix;
    mat4_t matrix;
    vec3_t translation;
    vec4_t rotation;            // Quaternion
    vec3_t scale;
} gltf_node_t;

typedef struct gltf_skin {
    char name[64];
    uint32_t* joints;
    uint32_t joint_count;
    uint32_t inverse_bind_matrices;  // Accessor index
    int32_t skeleton;           // Root node
} gltf_skin_t;

typedef struct gltf_animation {
    char name[64];
    // Animation channels and samplers would go here
} gltf_animation_t;

typedef struct gltf_scene {
    char name[64];
    uint32_t* nodes;
    uint32_t node_count;
} gltf_scene_t;

typedef struct gltf_data {
    // Asset info
    char version[16];
    char generator[64];
    
    // Data arrays
    gltf_buffer_t* buffers;
    uint32_t buffer_count;
    
    gltf_buffer_view_t* buffer_views;
    uint32_t buffer_view_count;
    
    gltf_accessor_t* accessors;
    uint32_t accessor_count;
    
    gltf_mesh_t* meshes;
    uint32_t mesh_count;
    
    gltf_material_t* materials;
    uint32_t material_count;
    
    gltf_texture_t* textures;
    uint32_t texture_count;
    
    gltf_image_t* images;
    uint32_t image_count;
    
    gltf_node_t* nodes;
    uint32_t node_count;
    
    gltf_skin_t* skins;
    uint32_t skin_count;
    
    gltf_animation_t* animations;
    uint32_t animation_count;
    
    gltf_scene_t* scenes;
    uint32_t scene_count;
    uint32_t default_scene;
    
    bool loaded;
} gltf_data_t;

typedef struct asset_system_gltf_importer_internal {
    uint32_t id;
    uint32_t flags;
    gltf_data_t data;
    char file_path[512];
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} asset_system_gltf_importer_internal_t;

typedef struct asset_system_gltf_importer_context {
    asset_system_gltf_importer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} asset_system_gltf_importer_context_t;

static asset_system_gltf_importer_context_t g_gltf_ctx = {0};

/* ============================================================================
 * JSON PARSING HELPERS (simplified)
 * ============================================================================ */

static double gltf_atof(const char* str);

static const char* skip_whitespace(const char* json) {
    while (*json == ' ' || *json == '\t' || *json == '\n' || *json == '\r') {
        json++;
    }
    return json;
}

static const char* parse_string(const char* json, char* out, size_t max_len) {
    json = skip_whitespace(json);
    if (*json != '"') return NULL;
    json++;
    
    size_t i = 0;
    while (*json && *json != '"' && i < max_len - 1) {
        if (*json == '\\') {
            json++;
            if (*json == 'n') out[i++] = '\n';
            else if (*json == 't') out[i++] = '\t';
            else out[i++] = *json;
        } else {
            out[i++] = *json;
        }
        json++;
    }
    out[i] = '\0';
    
    if (*json == '"') json++;
    return json;
}

static const char* parse_number(const char* json, float* out) {
    json = skip_whitespace(json);
    char buf[32];
    size_t i = 0;
    
    while ((*json >= '0' && *json <= '9') || *json == '.' || 
           *json == '-' || *json == '+' || *json == 'e' || *json == 'E') {
        if (i < 31) buf[i++] = *json;
        json++;
    }
    buf[i] = '\0';
    
    *out = (float)gltf_atof(buf);
    return json;
}

static double gltf_atof(const char* str) {
    double result = 0.0;
    double fraction = 0.0;
    double divisor = 10.0;
    bool negative = false;
    bool in_fraction = false;
    
    while (*str == ' ') str++;
    if (*str == '-') { negative = true; str++; }
    else if (*str == '+') str++;
    
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            if (in_fraction) {
                fraction += (*str - '0') / divisor;
                divisor *= 10.0;
            } else {
                result = result * 10.0 + (*str - '0');
            }
        } else if (*str == '.') {
            in_fraction = true;
        } else {
            break;
        }
        str++;
    }
    
    result += fraction;
    return negative ? -result : result;
}

/* ============================================================================
 * GLTF PARSING
 * ============================================================================ */

static void gltf_data_cleanup(gltf_data_t* data) {
    if (!data) return;
    
    // Free buffers
    if (data->buffers) {
        for (uint32_t i = 0; i < data->buffer_count; i++) {
            if (data->buffers[i].data) free(data->buffers[i].data);
        }
        free(data->buffers);
    }
    
    if (data->buffer_views) free(data->buffer_views);
    if (data->accessors) free(data->accessors);
    
    if (data->meshes) {
        for (uint32_t i = 0; i < data->mesh_count; i++) {
            if (data->meshes[i].primitives) free(data->meshes[i].primitives);
        }
        free(data->meshes);
    }
    
    if (data->materials) free(data->materials);
    if (data->textures) free(data->textures);
    if (data->images) free(data->images);
    
    if (data->nodes) {
        for (uint32_t i = 0; i < data->node_count; i++) {
            if (data->nodes[i].children) free(data->nodes[i].children);
        }
        free(data->nodes);
    }
    
    if (data->skins) {
        for (uint32_t i = 0; i < data->skin_count; i++) {
            if (data->skins[i].joints) free(data->skins[i].joints);
        }
        free(data->skins);
    }
    
    if (data->animations) free(data->animations);
    
    if (data->scenes) {
        for (uint32_t i = 0; i < data->scene_count; i++) {
            if (data->scenes[i].nodes) free(data->scenes[i].nodes);
        }
        free(data->scenes);
    }
    
    memset(data, 0, sizeof(gltf_data_t));
}

static int gltf_load_file(gltf_data_t* data, const char* path) {
    // This is a simplified parser - a real implementation would use a JSON library
    
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* json = malloc(size + 1);
    if (!json) {
        fclose(f);
        return -2;
    }
    
    fread(json, 1, size, f);
    json[size] = '\0';
    fclose(f);
    
    // Parse basic structure (very simplified)
    // A real implementation would properly parse the JSON
    
    // Look for asset.version
    const char* version_pos = strstr(json, "\"version\"");
    if (version_pos) {
        version_pos = strchr(version_pos, ':');
        if (version_pos) {
            parse_string(version_pos + 1, data->version, sizeof(data->version));
        }
    }
    
    // Count meshes (look for "meshes" array)
    const char* meshes_pos = strstr(json, "\"meshes\"");
    if (meshes_pos) {
        // Count array elements (simplified)
        const char* p = strchr(meshes_pos, '[');
        if (p) {
            int count = 0;
            int depth = 0;
            while (*p) {
                if (*p == '[' || *p == '{') depth++;
                else if (*p == ']' || *p == '}') {
                    depth--;
                    if (*p == '}' && depth == 1) count++;
                    if (depth == 0) break;
                }
                p++;
            }
            data->mesh_count = count;
        }
    }
    
    free(json);
    data->loaded = true;
    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int asset_system_gltf_importer_init(void) {
    if (g_gltf_ctx.initialized) {
        return 0;
    }

    g_gltf_ctx.capacity = GLTF_IMPORTER_MAX_COUNT;
    g_gltf_ctx.items = calloc(g_gltf_ctx.capacity, sizeof(asset_system_gltf_importer_internal_t));
    if (!g_gltf_ctx.items) {
        return -1;
    }

    g_gltf_ctx.count = 0;
    g_gltf_ctx.initialized = true;

    return 0;
}

void asset_system_gltf_importer_shutdown(void) {
    if (!g_gltf_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_gltf_ctx.count; i++) {
        gltf_data_cleanup(&g_gltf_ctx.items[i].data);
    }

    free(g_gltf_ctx.items);
    g_gltf_ctx.items = NULL;
    g_gltf_ctx.count = 0;
    g_gltf_ctx.capacity = 0;
    g_gltf_ctx.initialized = false;
}

int asset_system_gltf_importer_create(asset_system_gltf_importer_handle_t* out_handle, 
                                 const asset_system_gltf_importer_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_gltf_ctx.initialized) {
        return -2;
    }

    if (g_gltf_ctx.count >= g_gltf_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_gltf_ctx.count++;
    asset_system_gltf_importer_internal_t* item = &g_gltf_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->data, 0, sizeof(gltf_data_t));
    item->file_path[0] = '\0';
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void asset_system_gltf_importer_destroy(asset_system_gltf_importer_handle_t handle) {
    if (handle.id >= g_gltf_ctx.count) {
        return;
    }

    gltf_data_cleanup(&g_gltf_ctx.items[handle.id].data);
    g_gltf_ctx.items[handle.id].initialized = false;
}

int asset_system_gltf_importer_load(asset_system_gltf_importer_handle_t handle, const char* path) {
    if (handle.id >= g_gltf_ctx.count || !path) {
        return -1;
    }
    
    asset_system_gltf_importer_internal_t* item = &g_gltf_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // Cleanup previous data
    gltf_data_cleanup(&item->data);
    
    // Store path
    strncpy(item->file_path, path, sizeof(item->file_path) - 1);
    
    // Load and parse glTF
    int result = gltf_load_file(&item->data, path);
    if (result != 0) {
        return result;
    }
    
    item->dirty = true;
    return 0;
}

uint32_t asset_system_gltf_importer_get_mesh_count(asset_system_gltf_importer_handle_t handle) {
    if (handle.id >= g_gltf_ctx.count) {
        return 0;
    }
    return g_gltf_ctx.items[handle.id].data.mesh_count;
}

uint32_t asset_system_gltf_importer_get_material_count(asset_system_gltf_importer_handle_t handle) {
    if (handle.id >= g_gltf_ctx.count) {
        return 0;
    }
    return g_gltf_ctx.items[handle.id].data.material_count;
}

uint32_t asset_system_gltf_importer_get_animation_count(asset_system_gltf_importer_handle_t handle) {
    if (handle.id >= g_gltf_ctx.count) {
        return 0;
    }
    return g_gltf_ctx.items[handle.id].data.animation_count;
}

const char* asset_system_gltf_importer_get_version(asset_system_gltf_importer_handle_t handle) {
    if (handle.id >= g_gltf_ctx.count) {
        return NULL;
    }
    return g_gltf_ctx.items[handle.id].data.version;
}

int asset_system_gltf_importer_update(asset_system_gltf_importer_handle_t handle, 
                                 const void* data, size_t size) {
    if (handle.id >= g_gltf_ctx.count) {
        return -1;
    }

    g_gltf_ctx.items[handle.id].dirty = true;
    return 0;
}

bool asset_system_gltf_importer_is_valid(asset_system_gltf_importer_handle_t handle) {
    if (handle.id >= g_gltf_ctx.count) {
        return false;
    }
    return g_gltf_ctx.items[handle.id].data.loaded;
}

int asset_system_gltf_importer_get_info(asset_system_gltf_importer_handle_t handle, 
                                   asset_system_gltf_importer_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_gltf_ctx.count) {
        return -2;
    }

    const asset_system_gltf_importer_internal_t* item = &g_gltf_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void asset_system_gltf_importer_mark_dirty(asset_system_gltf_importer_handle_t handle) {
    if (handle.id < g_gltf_ctx.count) {
        g_gltf_ctx.items[handle.id].dirty = true;
    }
}

int asset_system_gltf_importer_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_gltf_ctx.count; i++) {
        asset_system_gltf_importer_internal_t* item = &g_gltf_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t asset_system_gltf_importer_get_count(void) {
    return g_gltf_ctx.count;
}

size_t asset_system_gltf_importer_get_memory_usage(void) {
    size_t total = sizeof(g_gltf_ctx);
    total += g_gltf_ctx.capacity * sizeof(asset_system_gltf_importer_internal_t);
    return total;
}

void asset_system_gltf_importer_debug_print(void) {
    // Debug output
}

/* End of gltf_importer.c */
