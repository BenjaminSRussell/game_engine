#include "geometry/mesh/mesh_loader.h"
#include "geometry/mesh.h"
#include "geometry/geometry_types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// ============================================================================
// CONSTANTS AND DEFAULTS
// ============================================================================

#define MESH_LOADER_MAX_PATH_LENGTH 256
#define MESH_LOADER_MAX_LINE_LENGTH 1024
#define MESH_LOADER_INITIAL_CAPACITY 1024

const mesh_load_options_t MESH_LOAD_OPTIONS_DEFAULT = {
    .calculate_normals = true,
    .calculate_tangents = true,
    .optimize_vertices = true,
    .generate_bounds = true,
    .flip_uvs = false,
    .merge_vertices = true,
    .merge_tolerance = 0.001f,
    .max_vertices = 1000000,
    .load_materials = true,
    .load_animations = false
};

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct mesh_loader_state_t {
    bool initialized;
    u32 load_count;
    size_t total_memory_used;
    mesh_load_stats_t global_stats;
} mesh_loader_state_t;

static mesh_loader_state_t g_loader_state = {0};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static const char* get_file_extension(const char* filepath) {
    const char* dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return "";
    return dot + 1;
}

static mesh_format_e detect_format_from_extension(const char* extension) {
    char ext_lower[16];
    strncpy(ext_lower, extension, sizeof(ext_lower) - 1);
    ext_lower[sizeof(ext_lower) - 1] = '\0';
    
    for (int i = 0; ext_lower[i]; i++) {
        ext_lower[i] = tolower(ext_lower[i]);
    }
    
    if (strcmp(ext_lower, "obj") == 0) return MESH_FORMAT_OBJ;
    if (strcmp(ext_lower, "fbx") == 0) return MESH_FORMAT_FBX;
    if (strcmp(ext_lower, "gltf") == 0) return MESH_FORMAT_GLTF;
    if (strcmp(ext_lower, "glb") == 0) return MESH_FORMAT_GLTF;
    if (strcmp(ext_lower, "ply") == 0) return MESH_FORMAT_PLY;
    if (strcmp(ext_lower, "stl") == 0) return MESH_FORMAT_STL;
    
    return MESH_FORMAT_CUSTOM;
}

static Vec3 parse_vec3(const char* str) {
    Vec3 result = {0};
    sscanf(str, "%f %f %f", &result.x, &result.y, &result.z);
    return result;
}

static Vec2 parse_vec2(const char* str) {
    Vec2 result = {0};
    sscanf(str, "%f %f", &result.x, &result.y);
    return result;
}

// ============================================================================
// OBJ LOADER IMPLEMENTATION
// ============================================================================

typedef struct obj_parse_context_t {
    Vec3* positions;
    Vec3* normals;
    Vec2* uvs;
    u32 position_count;
    u32 normal_count;
    u32 uv_count;
    u32 position_capacity;
    u32 normal_capacity;
    u32 uv_capacity;
    
    vertex_t* vertices;
    u32* indices;
    u32 vertex_count;
    u32 index_count;
    u32 vertex_capacity;
    u32 index_capacity;
} obj_parse_context_t;

static obj_parse_context_t* obj_parse_context_create(void) {
    obj_parse_context_t* ctx = calloc(1, sizeof(obj_parse_context_t));
    if (!ctx) return NULL;
    
    ctx->position_capacity = MESH_LOADER_INITIAL_CAPACITY;
    ctx->normal_capacity = MESH_LOADER_INITIAL_CAPACITY;
    ctx->uv_capacity = MESH_LOADER_INITIAL_CAPACITY;
    ctx->vertex_capacity = MESH_LOADER_INITIAL_CAPACITY;
    ctx->index_capacity = MESH_LOADER_INITIAL_CAPACITY * 3;
    
    ctx->positions = malloc(ctx->position_capacity * sizeof(Vec3));
    ctx->normals = malloc(ctx->normal_capacity * sizeof(Vec3));
    ctx->uvs = malloc(ctx->uv_capacity * sizeof(Vec2));
    ctx->vertices = malloc(ctx->vertex_capacity * sizeof(vertex_t));
    ctx->indices = malloc(ctx->index_capacity * sizeof(u32));
    
    if (!ctx->positions || !ctx->normals || !ctx->uvs || !ctx->vertices || !ctx->indices) {
        obj_parse_context_destroy(ctx);
        return NULL;
    }
    
    return ctx;
}

static void obj_parse_context_destroy(obj_parse_context_t* ctx) {
    if (!ctx) return;
    free(ctx->positions);
    free(ctx->normals);
    free(ctx->uvs);
    free(ctx->vertices);
    free(ctx->indices);
    free(ctx);
}

static bool obj_add_position(obj_parse_context_t* ctx, const Vec3 pos) {
    if (ctx->position_count >= ctx->position_capacity) {
        ctx->position_capacity *= 2;
        Vec3* new_positions = realloc(ctx->positions, ctx->position_capacity * sizeof(Vec3));
        if (!new_positions) return false;
        ctx->positions = new_positions;
    }
    
    ctx->positions[ctx->position_count++] = pos;
    return true;
}

static bool obj_add_normal(obj_parse_context_t* ctx, const Vec3 normal) {
    if (ctx->normal_count >= ctx->normal_capacity) {
        ctx->normal_capacity *= 2;
        Vec3* new_normals = realloc(ctx->normals, ctx->normal_capacity * sizeof(Vec3));
        if (!new_normals) return false;
        ctx->normals = new_normals;
    }
    
    ctx->normals[ctx->normal_count++] = normal;
    return true;
}

static bool obj_add_uv(obj_parse_context_t* ctx, const Vec2 uv) {
    if (ctx->uv_count >= ctx->uv_capacity) {
        ctx->uv_capacity *= 2;
        Vec2* new_uvs = realloc(ctx->uvs, ctx->uv_capacity * sizeof(Vec2));
        if (!new_uvs) return false;
        ctx->uvs = new_uvs;
    }
    
    ctx->uvs[ctx->uv_count++] = uv;
    return true;
}

static bool obj_parse_face_vertex(const char* token, u32* pos_idx, u32* uv_idx, u32* normal_idx) {
    // Format: pos/uv/normal or pos//normal or pos/uv or pos
    char token_copy[64];
    strncpy(token_copy, token, sizeof(token_copy) - 1);
    token_copy[sizeof(token_copy) - 1] = '\0';
    
    char* parts[3] = {0};
    int part_count = 0;
    
    char* ptr = token_copy;
    char* part = strtok(ptr, "/");
    while (part && part_count < 3) {
        parts[part_count++] = part;
        part = strtok(NULL, "/");
    }
    
    *pos_idx = parts[0] ? atoi(parts[0]) : 0;
    *uv_idx = parts[1] ? atoi(parts[1]) : 0;
    *normal_idx = parts[2] ? atoi(parts[2]) : 0;
    
    return true;
}

static bool obj_add_face(obj_parse_context_t* ctx, const char* line) {
    char line_copy[MESH_LOADER_MAX_LINE_LENGTH];
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';
    
    char* tokens[16];
    int token_count = 0;
    
    char* ptr = line_copy + 2; // Skip "f "
    char* token = strtok(ptr, " \t");
    while (token && token_count < 16) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t");
    }
    
    if (token_count < 3) return false;
    
    // Triangulate face (fan triangulation)
    for (int i = 1; i < token_count - 1; i++) {
        for (int j = 0; j < 3; j++) {
            int vertex_index = (j == 0) ? 0 : i + j - 1;
            
            u32 pos_idx, uv_idx, normal_idx;
            if (!obj_parse_face_vertex(tokens[vertex_index], &pos_idx, &uv_idx, &normal_idx)) {
                return false;
            }
            
            // Convert from OBJ (1-based) to 0-based indexing
            pos_idx = (pos_idx > 0) ? pos_idx - 1 : ctx->position_count + pos_idx;
            uv_idx = (uv_idx > 0) ? uv_idx - 1 : ctx->uv_count + uv_idx;
            normal_idx = (normal_idx > 0) ? normal_idx - 1 : ctx->normal_count + normal_idx;
            
            if (ctx->vertex_count >= ctx->vertex_capacity) {
                ctx->vertex_capacity *= 2;
                vertex_t* new_vertices = realloc(ctx->vertices, ctx->vertex_capacity * sizeof(vertex_t));
                if (!new_vertices) return false;
                ctx->vertices = new_vertices;
            }
            
            vertex_t* vertex = &ctx->vertices[ctx->vertex_count++];
            
            if (pos_idx < ctx->position_count) {
                vertex->position = ctx->positions[pos_idx];
            }
            
            if (uv_idx < ctx->uv_count) {
                vertex->uv = ctx->uvs[uv_idx];
            }
            
            if (normal_idx < ctx->normal_count) {
                vertex->normal = ctx->normals[normal_idx];
            }
            
            if (ctx->index_count >= ctx->index_capacity) {
                ctx->index_capacity *= 2;
                u32* new_indices = realloc(ctx->indices, ctx->index_capacity * sizeof(u32));
                if (!new_indices) return false;
                ctx->indices = new_indices;
            }
            
            ctx->indices[ctx->index_count++] = ctx->vertex_count - 1;
        }
    }
    
    return true;
}

static mesh_t* obj_load_from_file(FILE* file, const mesh_load_options_t* options) {
    obj_parse_context_t* ctx = obj_parse_context_create();
    if (!ctx) return NULL;
    
    char line[MESH_LOADER_MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = '\0';
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') continue;
        
        char token[64];
        sscanf(line, "%63s", token);
        
        if (strcmp(token, "v") == 0) {
            Vec3 pos = parse_vec3(line + 2);
            obj_add_position(ctx, pos);
        } else if (strcmp(token, "vn") == 0) {
            Vec3 normal = parse_vec3(line + 3);
            obj_add_normal(ctx, normal);
        } else if (strcmp(token, "vt") == 0) {
            Vec2 uv = parse_vec2(line + 3);
            if (options->flip_uvs) {
                uv.y = 1.0f - uv.y;
            }
            obj_add_uv(ctx, uv);
        } else if (strcmp(token, "f") == 0) {
            if (!obj_add_face(ctx, line)) {
                printf("Warning: Failed to parse face: %s\n", line);
            }
        }
    }
    
    // Create mesh from parsed data
    mesh_t* mesh = mesh_create("loaded_obj");
    if (!mesh) {
        obj_parse_context_destroy(ctx);
        return NULL;
    }
    
    mesh_allocate_buffers(mesh, ctx->vertex_count, ctx->index_count);
    mesh_set_vertices(mesh, ctx->vertices, ctx->vertex_count, 0);
    mesh_set_indices(mesh, ctx->indices, ctx->index_count, 0);
    
    // Calculate missing data
    if (options->calculate_normals && ctx->normal_count == 0) {
        mesh_calculate_normals(mesh);
    }
    
    if (options->calculate_tangents) {
        mesh_calculate_tangents(mesh);
    }
    
    if (options->generate_bounds) {
        mesh_calculate_bounds(mesh);
    }
    
    obj_parse_context_destroy(ctx);
    return mesh;
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

int geometry_mesh_loader_init(void) {
    if (g_loader_state.initialized) {
        return 0; // Already initialized
    }
    
    memset(&g_loader_state, 0, sizeof(g_loader_state));
    g_loader_state.initialized = true;
    
    return 0;
}

void geometry_mesh_loader_shutdown(void) {
    if (!g_loader_state.initialized) return;
    
    memset(&g_loader_state, 0, sizeof(g_loader_state));
}

int geometry_mesh_loader_create(geometry_mesh_loader_handle_t* out_handle, const geometry_mesh_loader_desc_t* desc) {
    if (!out_handle || !g_loader_state.initialized) return -1;
    
    out_handle->id = ++g_loader_state.load_count;
    return 0;
}

void geometry_mesh_loader_destroy(geometry_mesh_loader_handle_t handle) {
    // No-op for now
}

int geometry_mesh_loader_update(geometry_mesh_loader_handle_t handle, const void* data, size_t size) {
    return 0; // No-op
}

bool geometry_mesh_loader_is_valid(geometry_mesh_loader_handle_t handle) {
    return g_loader_state.initialized && handle.id > 0 && handle.id <= g_loader_state.load_count;
}

int geometry_mesh_loader_get_info(geometry_mesh_loader_handle_t handle, geometry_mesh_loader_info_t* out_info) {
    if (!out_info || !geometry_mesh_loader_is_valid(handle)) return -1;
    
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = true;
    
    return 0;
}

void geometry_mesh_loader_mark_dirty(geometry_mesh_loader_handle_t handle) {
    // No-op
}

int geometry_mesh_loader_process_pending(void) {
    return 0;
}

int geometry_mesh_loader_load_async(
    geometry_mesh_loader_handle_t handle,
    const char* path,
    geometry_mesh_loader_callback_t callback,
    void* user_data
) {
    if (!path || !callback || !geometry_mesh_loader_is_valid(handle)) return -1;
    
    // For now, simulate async loading by calling callback immediately
    mesh_t* mesh = mesh_load_from_file(path, &MESH_LOAD_OPTIONS_DEFAULT);
    
    int status = mesh ? 0 : -1;
    callback(handle, user_data, status);
    
    return 0;
}

uint32_t geometry_mesh_loader_get_count(void) {
    return g_loader_state.load_count;
}

size_t geometry_mesh_loader_get_memory_usage(void) {
    return g_loader_state.total_memory_used;
}

void geometry_mesh_loader_debug_print(void) {
    printf("Mesh Loader State:\n");
    printf("  Initialized: %s\n", g_loader_state.initialized ? "Yes" : "No");
    printf("  Load Count: %u\n", g_loader_state.load_count);
    printf("  Memory Used: %zu bytes\n", g_loader_state.total_memory_used);
}

// ============================================================================
// HIGH-LEVEL MESH LOADING API
// ============================================================================

mesh_format_e mesh_detect_format(const char* filepath) {
    if (!filepath) return MESH_FORMAT_CUSTOM;
    return detect_format_from_extension(get_file_extension(filepath));
}

const char* mesh_format_to_string(mesh_format_e format) {
    switch (format) {
        case MESH_FORMAT_OBJ: return "OBJ";
        case MESH_FORMAT_FBX: return "FBX";
        case MESH_FORMAT_GLTF: return "GLTF";
        case MESH_FORMAT_PLY: return "PLY";
        case MESH_FORMAT_STL: return "STL";
        case MESH_FORMAT_CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

mesh_t* mesh_load_from_file(const char* filepath, const mesh_load_options_t* options) {
    if (!filepath) return NULL;
    
    if (!g_loader_state.initialized) {
        geometry_mesh_loader_init();
    }
    
    const mesh_load_options_t* opts = options ? options : &MESH_LOAD_OPTIONS_DEFAULT;
    mesh_format_e format = mesh_detect_format(filepath);
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        printf("Error: Could not open file: %s\n", filepath);
        return NULL;
    }
    
    mesh_t* mesh = NULL;
    
    switch (format) {
        case MESH_FORMAT_OBJ:
            mesh = obj_load_from_file(file, opts);
            break;
        case MESH_FORMAT_FBX:
            printf("Warning: FBX loading not yet implemented\n");
            break;
        case MESH_FORMAT_GLTF:
            printf("Warning: GLTF loading not yet implemented\n");
            break;
        case MESH_FORMAT_PLY:
            printf("Warning: PLY loading not yet implemented\n");
            break;
        case MESH_FORMAT_STL:
            printf("Warning: STL loading not yet implemented\n");
            break;
        default:
            printf("Error: Unsupported mesh format: %s\n", mesh_format_to_string(format));
            break;
    }
    
    fclose(file);
    
    if (mesh) {
        g_loader_state.total_memory_used += 
            mesh->vertex_count * sizeof(vertex_t) + 
            mesh->index_count * sizeof(u32);
    }
    
    return mesh;
}

void mesh_get_load_stats(const mesh_t* mesh, mesh_load_stats_t* stats) {
    if (!mesh || !stats) return;
    
    memset(stats, 0, sizeof(mesh_load_stats_t));
    stats->processed_vertices = mesh->vertex_count;
    stats->processed_indices = mesh->index_count;
    stats->submesh_count = mesh->submesh_count;
}
