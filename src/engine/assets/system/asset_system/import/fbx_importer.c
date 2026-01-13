/*
 * fbx_importer.c
 * FBX asset import
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements FBX binary/ASCII parsing and data extraction
 */

#include "assets/system/asset_system/import/fbx_importer.h"
#include <geometry/geometry_types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define FBX_IMPORTER_MAX_COUNT 32
#define FBX_MAX_NODES 4096
#define FBX_MAX_MESHES 256
#define FBX_BINARY_MAGIC "Kaydara FBX Binary  "
#define FBX_BINARY_MAGIC_LEN 21

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

// Use engine types directly via includes in headers, but if not available:
// Assuming math/vec2.h, etc. are included in fbx_importer.h or earlier
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat4.h>

// If typedefs exist, we use them.
// Note: vec2_t, vec3_t, vec4_t, mat4_t are already defined in math headers
typedef Mat4 mat4_t;

/* ============================================================================
 * FBX TYPES
 * ============================================================================ */

typedef enum fbx_format {
    FBX_FORMAT_UNKNOWN = 0,
    FBX_FORMAT_BINARY,
    FBX_FORMAT_ASCII
} fbx_format_t;

typedef enum fbx_property_type {
    FBX_PROP_INT16 = 'Y',
    FBX_PROP_BOOL = 'C',
    FBX_PROP_INT32 = 'I',
    FBX_PROP_FLOAT = 'F',
    FBX_PROP_DOUBLE = 'D',
    FBX_PROP_INT64 = 'L',
    FBX_PROP_FLOAT_ARRAY = 'f',
    FBX_PROP_DOUBLE_ARRAY = 'd',
    FBX_PROP_INT64_ARRAY = 'l',
    FBX_PROP_INT32_ARRAY = 'i',
    FBX_PROP_BOOL_ARRAY = 'b',
    FBX_PROP_STRING = 'S',
    FBX_PROP_RAW = 'R'
} fbx_property_type_t;

typedef struct fbx_property {
    fbx_property_type_t type;
    union {
        int16_t i16;
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        bool boolean;
        struct {
            char* data;
            uint32_t length;
        } string;
        struct {
            void* data;
            uint32_t count;
        } array;
    } value;
} fbx_property_t;

typedef struct fbx_node {
    char name[64];
    fbx_property_t* properties;
    uint32_t property_count;
    struct fbx_node* children;
    uint32_t child_count;
    struct fbx_node* parent;
} fbx_node_t;

typedef struct fbx_mesh {
    char name[64];
    
    // Vertex data
    float* positions;
    uint32_t position_count;
    
    float* normals;
    uint32_t normal_count;
    
    float* uvs;
    uint32_t uv_count;
    
    // Index data
    int32_t* polygon_indices;
    uint32_t polygon_index_count;
    
    // Materials
    int32_t* material_indices;
    uint32_t material_count;
} fbx_mesh_t;

typedef struct fbx_material {
    char name[64];
    vec3_t diffuse_color;
    vec3_t specular_color;
    vec3_t ambient_color;
    float shininess;
    float opacity;
    char diffuse_texture[256];
    char normal_texture[256];
} fbx_material_t;

typedef struct fbx_bone {
    char name[64];
    int64_t id;
    int32_t parent_index;
    mat4_t transform;
    mat4_t offset_matrix;
} fbx_bone_t;

typedef struct fbx_skin {
    fbx_bone_t* bones;
    uint32_t bone_count;
    
    // Per-vertex weights
    struct {
        uint32_t* bone_indices;
        float* weights;
        uint32_t count;
    }* vertex_weights;
    uint32_t vertex_count;
} fbx_skin_t;

typedef struct fbx_animation_curve {
    float* times;
    float* values;
    uint32_t keyframe_count;
} fbx_animation_curve_t;

typedef struct fbx_animation {
    char name[64];
    float duration;
    float fps;
    
    struct {
        int64_t node_id;
        fbx_animation_curve_t translation[3];
        fbx_animation_curve_t rotation[3];
        fbx_animation_curve_t scale[3];
    }* node_curves;
    uint32_t node_curve_count;
} fbx_animation_t;

typedef struct fbx_data {
    fbx_format_t format;
    uint32_t version;
    
    fbx_node_t* root;
    
    fbx_mesh_t* meshes;
    uint32_t mesh_count;
    
    fbx_material_t* materials;
    uint32_t material_count;
    
    fbx_skin_t* skins;
    uint32_t skin_count;
    
    fbx_animation_t* animations;
    uint32_t animation_count;
    
    bool loaded;
} fbx_data_t;

typedef struct asset_system_fbx_importer_internal {
    uint32_t id;
    uint32_t flags;
    fbx_data_t data;
    char file_path[512];
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} asset_system_fbx_importer_internal_t;

typedef struct asset_system_fbx_importer_context {
    asset_system_fbx_importer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} asset_system_fbx_importer_context_t;

static asset_system_fbx_importer_context_t g_fbx_ctx = {0};

/* ============================================================================
 * FBX PARSING HELPERS
 * ============================================================================ */

static fbx_format_t detect_format(const uint8_t* data, size_t size) {
    if (size < FBX_BINARY_MAGIC_LEN) {
        return FBX_FORMAT_UNKNOWN;
    }
    
    if (memcmp(data, FBX_BINARY_MAGIC, FBX_BINARY_MAGIC_LEN) == 0) {
        return FBX_FORMAT_BINARY;
    }
    
    // Check for ASCII format (starts with comment or keyword)
    if (data[0] == ';' || strncmp((const char*)data, "FBXHeaderExtension", 18) == 0) {
        return FBX_FORMAT_ASCII;
    }
    
    return FBX_FORMAT_UNKNOWN;
}

static uint32_t read_u32_le(const uint8_t* data) {
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

static int64_t read_i64_le(const uint8_t* data) {
    uint64_t low = read_u32_le(data);
    uint64_t high = read_u32_le(data + 4);
    return (int64_t)(low | (high << 32));
}

static float read_f32_le(const uint8_t* data) {
    union { uint32_t i; float f; } u;
    u.i = read_u32_le(data);
    return u.f;
}

/* ============================================================================
 * FBX BINARY PARSING
 * ============================================================================ */

static int parse_fbx_binary_node(const uint8_t* data, size_t offset, 
                                   fbx_node_t* node, uint32_t version) {
    // FBX 7.5+ uses 64-bit offsets
    bool use_64bit = (version >= 7500);
    size_t header_size = use_64bit ? 25 : 13;
    
    const uint8_t* ptr = data + offset;
    
    // Read node header
    uint64_t end_offset, num_properties, property_list_len;
    if (use_64bit) {
        end_offset = read_i64_le(ptr); ptr += 8;
        num_properties = read_i64_le(ptr); ptr += 8;
        property_list_len = read_i64_le(ptr); ptr += 8;
    } else {
        end_offset = read_u32_le(ptr); ptr += 4;
        num_properties = read_u32_le(ptr); ptr += 4;
        property_list_len = read_u32_le(ptr); ptr += 4;
    }
    
    // Null node (end marker)
    if (end_offset == 0) {
        return 0;
    }
    
    // Read node name
    uint8_t name_len = *ptr++;
    if (name_len >= sizeof(node->name)) name_len = sizeof(node->name) - 1;
    memcpy(node->name, ptr, name_len);
    node->name[name_len] = '\0';
    ptr += name_len;
    
    // Parse properties (simplified - would need full implementation)
    node->property_count = (uint32_t)num_properties;
    
    return (int)end_offset;
}

static int parse_fbx_binary(fbx_data_t* data, const uint8_t* buffer, size_t size) {
    if (size < 27) return -1;
    
    // Read version
    data->version = read_u32_le(buffer + 23);
    data->format = FBX_FORMAT_BINARY;
    
    // Parse root nodes
    size_t offset = 27;
    
    while (offset < size - 1) {
        fbx_node_t temp_node = {0};
        int end_offset = parse_fbx_binary_node(buffer, offset, &temp_node, data->version);
        
        if (end_offset == 0) break;  // Null node
        
        // Process specific nodes
        if (strcmp(temp_node.name, "Objects") == 0) {
            // Parse mesh, material, etc. objects
        } else if (strcmp(temp_node.name, "Connections") == 0) {
            // Parse node connections
        }
        
        offset = end_offset;
    }
    
    data->loaded = true;
    return 0;
}

/* ============================================================================
 * FBX DATA MANAGEMENT
 * ============================================================================ */

static void fbx_node_cleanup(fbx_node_t* node) {
    if (!node) return;
    
    if (node->properties) {
        for (uint32_t i = 0; i < node->property_count; i++) {
            fbx_property_t* prop = &node->properties[i];
            if (prop->type == FBX_PROP_STRING && prop->value.string.data) {
                free(prop->value.string.data);
            } else if ((prop->type == FBX_PROP_FLOAT_ARRAY || 
                        prop->type == FBX_PROP_DOUBLE_ARRAY) && 
                       prop->value.array.data) {
                free(prop->value.array.data);
            }
        }
        free(node->properties);
    }
    
    if (node->children) {
        for (uint32_t i = 0; i < node->child_count; i++) {
            fbx_node_cleanup(&node->children[i]);
        }
        free(node->children);
    }
}

static void fbx_data_cleanup(fbx_data_t* data) {
    if (!data) return;
    
    if (data->root) {
        fbx_node_cleanup(data->root);
        free(data->root);
    }
    
    if (data->meshes) {
        for (uint32_t i = 0; i < data->mesh_count; i++) {
            fbx_mesh_t* mesh = &data->meshes[i];
            if (mesh->positions) free(mesh->positions);
            if (mesh->normals) free(mesh->normals);
            if (mesh->uvs) free(mesh->uvs);
            if (mesh->polygon_indices) free(mesh->polygon_indices);
            if (mesh->material_indices) free(mesh->material_indices);
        }
        free(data->meshes);
    }
    
    if (data->materials) free(data->materials);
    
    if (data->skins) {
        for (uint32_t i = 0; i < data->skin_count; i++) {
            if (data->skins[i].bones) free(data->skins[i].bones);
        }
        free(data->skins);
    }
    
    if (data->animations) {
        for (uint32_t i = 0; i < data->animation_count; i++) {
            if (data->animations[i].node_curves) free(data->animations[i].node_curves);
        }
        free(data->animations);
    }
    
    memset(data, 0, sizeof(fbx_data_t));
}

static int fbx_load_file(fbx_data_t* data, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return -2;
    }
    
    fread(buffer, 1, size, f);
    fclose(f);
    
    // Detect format
    data->format = detect_format(buffer, size);
    
    int result;
    if (data->format == FBX_FORMAT_BINARY) {
        result = parse_fbx_binary(data, buffer, size);
    } else if (data->format == FBX_FORMAT_ASCII) {
        // ASCII parsing would go here
        result = -3;  // Not implemented
    } else {
        result = -4;  // Unknown format
    }
    
    free(buffer);
    return result;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int asset_system_fbx_importer_init(void) {
    if (g_fbx_ctx.initialized) {
        return 0;
    }

    g_fbx_ctx.capacity = FBX_IMPORTER_MAX_COUNT;
    g_fbx_ctx.items = calloc(g_fbx_ctx.capacity, sizeof(asset_system_fbx_importer_internal_t));
    if (!g_fbx_ctx.items) {
        return -1;
    }

    g_fbx_ctx.count = 0;
    g_fbx_ctx.initialized = true;

    return 0;
}

void asset_system_fbx_importer_shutdown(void) {
    if (!g_fbx_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_fbx_ctx.count; i++) {
        fbx_data_cleanup(&g_fbx_ctx.items[i].data);
    }

    free(g_fbx_ctx.items);
    g_fbx_ctx.items = NULL;
    g_fbx_ctx.count = 0;
    g_fbx_ctx.capacity = 0;
    g_fbx_ctx.initialized = false;
}

int asset_system_fbx_importer_create(asset_system_fbx_importer_handle_t* out_handle, 
                                const asset_system_fbx_importer_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_fbx_ctx.initialized) {
        return -2;
    }

    if (g_fbx_ctx.count >= g_fbx_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_fbx_ctx.count++;
    asset_system_fbx_importer_internal_t* item = &g_fbx_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->data, 0, sizeof(fbx_data_t));
    item->file_path[0] = '\0';
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void asset_system_fbx_importer_destroy(asset_system_fbx_importer_handle_t handle) {
    if (handle.id >= g_fbx_ctx.count) {
        return;
    }

    fbx_data_cleanup(&g_fbx_ctx.items[handle.id].data);
    g_fbx_ctx.items[handle.id].initialized = false;
}

int asset_system_fbx_importer_load(asset_system_fbx_importer_handle_t handle, const char* path) {
    if (handle.id >= g_fbx_ctx.count || !path) {
        return -1;
    }
    
    asset_system_fbx_importer_internal_t* item = &g_fbx_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    fbx_data_cleanup(&item->data);
    strncpy(item->file_path, path, sizeof(item->file_path) - 1);
    
    int result = fbx_load_file(&item->data, path);
    if (result != 0) {
        return result;
    }
    
    item->dirty = true;
    return 0;
}

uint32_t asset_system_fbx_importer_get_mesh_count(asset_system_fbx_importer_handle_t handle) {
    if (handle.id >= g_fbx_ctx.count) {
        return 0;
    }
    return g_fbx_ctx.items[handle.id].data.mesh_count;
}

uint32_t asset_system_fbx_importer_get_material_count(asset_system_fbx_importer_handle_t handle) {
    if (handle.id >= g_fbx_ctx.count) {
        return 0;
    }
    return g_fbx_ctx.items[handle.id].data.material_count;
}

uint32_t asset_system_fbx_importer_get_animation_count(asset_system_fbx_importer_handle_t handle) {
    if (handle.id >= g_fbx_ctx.count) {
        return 0;
    }
    return g_fbx_ctx.items[handle.id].data.animation_count;
}

uint32_t asset_system_fbx_importer_get_version(asset_system_fbx_importer_handle_t handle) {
    if (handle.id >= g_fbx_ctx.count) {
        return 0;
    }
    return g_fbx_ctx.items[handle.id].data.version;
}

int asset_system_fbx_importer_update(asset_system_fbx_importer_handle_t handle, 
                                const void* data, size_t size) {
    if (handle.id >= g_fbx_ctx.count) {
        return -1;
    }

    g_fbx_ctx.items[handle.id].dirty = true;
    return 0;
}

bool asset_system_fbx_importer_is_valid(asset_system_fbx_importer_handle_t handle) {
    if (handle.id >= g_fbx_ctx.count) {
        return false;
    }
    return g_fbx_ctx.items[handle.id].data.loaded;
}

int asset_system_fbx_importer_get_info(asset_system_fbx_importer_handle_t handle, 
                                  asset_system_fbx_importer_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_fbx_ctx.count) {
        return -2;
    }

    const asset_system_fbx_importer_internal_t* item = &g_fbx_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void asset_system_fbx_importer_mark_dirty(asset_system_fbx_importer_handle_t handle) {
    if (handle.id < g_fbx_ctx.count) {
        g_fbx_ctx.items[handle.id].dirty = true;
    }
}

int asset_system_fbx_importer_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_fbx_ctx.count; i++) {
        asset_system_fbx_importer_internal_t* item = &g_fbx_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t asset_system_fbx_importer_get_count(void) {
    return g_fbx_ctx.count;
}

size_t asset_system_fbx_importer_get_memory_usage(void) {
    size_t total = sizeof(g_fbx_ctx);
    total += g_fbx_ctx.capacity * sizeof(asset_system_fbx_importer_internal_t);
    return total;
}

void asset_system_fbx_importer_debug_print(void) {
    // Debug output
}

mesh_t* asset_system_fbx_load_mesh_direct(const char* path) {
    if (!path) return NULL;

    // Ensure system is initialized
    if (!g_fbx_ctx.initialized) {
        asset_system_fbx_importer_init();
    }

    asset_system_fbx_importer_handle_t handle;
    asset_system_fbx_importer_desc_t desc = {0};

    if (asset_system_fbx_importer_create(&handle, &desc) != 0) {
        return NULL;
    }

    if (asset_system_fbx_importer_load(handle, path) != 0) {
        asset_system_fbx_importer_destroy(handle);
        return NULL;
    }

    asset_system_fbx_importer_internal_t* item = &g_fbx_ctx.items[handle.id];

    // For now, we only extract the first mesh if available
    // In a real implementation, we would likely merge all meshes or return a scene
    if (item->data.mesh_count == 0) {
        asset_system_fbx_importer_destroy(handle);
        return NULL;
    }

    fbx_mesh_t* src_mesh = &item->data.meshes[0];

    mesh_t* mesh = (mesh_t*)malloc(sizeof(mesh_t));
    if (!mesh) {
        asset_system_fbx_importer_destroy(handle);
        return NULL;
    }
    memset(mesh, 0, sizeof(mesh_t));

    // Convert vertices
    // Note: src_mesh uses float array for positions (3 floats per vertex)
    mesh->vertex_count = src_mesh->position_count;
    mesh->vertices = (vertex_t*)malloc(mesh->vertex_count * sizeof(vertex_t));

    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        mesh->vertices[i].position = vec3(
            src_mesh->positions[i * 3 + 0],
            src_mesh->positions[i * 3 + 1],
            src_mesh->positions[i * 3 + 2]
        );

        if (i < src_mesh->normal_count) {
            mesh->vertices[i].normal = vec3(
                src_mesh->normals[i * 3 + 0],
                src_mesh->normals[i * 3 + 1],
                src_mesh->normals[i * 3 + 2]
            );
        } else {
            mesh->vertices[i].normal = vec3(0, 1, 0);
        }

        if (i < src_mesh->uv_count) {
            mesh->vertices[i].uv = vec2(
                src_mesh->uvs[i * 2 + 0],
                src_mesh->uvs[i * 2 + 1]
            );
        } else {
            mesh->vertices[i].uv = vec2(0, 0);
        }

        mesh->vertices[i].tangent = vec4(0, 0, 0, 1);
    }

    // Convert indices
    mesh->index_count = src_mesh->polygon_index_count;
    mesh->indices = (uint32_t*)malloc(mesh->index_count * sizeof(uint32_t));

    for (uint32_t i = 0; i < mesh->index_count; i++) {
        int32_t index = src_mesh->polygon_indices[i];
        // FBX uses negative indices to mark end of polygon (xor with -1)
        if (index < 0) {
            index = index ^ -1;
        }
        mesh->indices[i] = (uint32_t)index;
    }

    asset_system_fbx_importer_destroy(handle);
    return mesh;
}

/* End of fbx_importer.c */
