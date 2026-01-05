/*
 * mesh_data.c
 * Mesh vertex/index data storage and submesh management
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 *
 * Supports:
 * - Mesh data creation and destruction
 * - Vertex and index buffer management
 * - Submesh support with material binding
 * - AABB and bounding sphere calculation
 * - Memory pooling and statistics
 */

#include "mesh_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define GEOMETRY_MESH_DATA_MAX_COUNT 8192
#define GEOMETRY_MESH_DATA_DEFAULT_CAPACITY 512

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_mesh_data_internal {
    geometry_mesh_data_handle_t handle;
    mesh_data_t data;
    void* vertex_buffer;  // Owned copy of vertex data
    void* index_buffer;   // Owned copy of index data
    submesh_t* submeshes; // Owned copy of submeshes
    uint32_t flags;
    bool initialized;
    uint64_t frame_updated;
} geometry_mesh_data_internal_t;

typedef struct geometry_mesh_data_context {
    geometry_mesh_data_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} geometry_mesh_data_context_t;

static geometry_mesh_data_context_t g_mesh_data_ctx = {0};

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

const char* geometry_index_type_get_name(index_type_t type) {
    static const char* names[INDEX_TYPE_COUNT] = {
        "UINT16",
        "UINT32",
    };
    if (type >= INDEX_TYPE_COUNT) return "UNKNOWN";
    return names[type];
}

uint32_t geometry_index_type_get_size(index_type_t type) {
    static const uint32_t sizes[INDEX_TYPE_COUNT] = {2, 4};
    if (type >= INDEX_TYPE_COUNT) return 0;
    return sizes[type];
}

/* ============================================================================
 * BOUNDS CALCULATION
 * ============================================================================ */

// Helper to compute AABB from vertex data
// Assumes vertices have position data (3 floats) at beginning of each vertex
static int geometry_mesh_data_compute_aabb(
    const void* vertex_data,
    uint32_t vertex_count,
    uint32_t vertex_stride,
    aabb_t* out_aabb)
{
    if (!vertex_data || vertex_count == 0 || !out_aabb) return -1;

    const float* first_vertex = (const float*)vertex_data;
    float min_x = first_vertex[0];
    float min_y = first_vertex[1];
    float min_z = first_vertex[2];
    float max_x = min_x;
    float max_y = min_y;
    float max_z = min_z;

    // Iterate through all vertices (assuming position is first 3 floats)
    const uint8_t* vert_ptr = (const uint8_t*)vertex_data;
    for (uint32_t i = 1; i < vertex_count; i++) {
        vert_ptr += vertex_stride;
        const float* pos = (const float*)vert_ptr;

        if (pos[0] < min_x) min_x = pos[0];
        if (pos[0] > max_x) max_x = pos[0];
        if (pos[1] < min_y) min_y = pos[1];
        if (pos[1] > max_y) max_y = pos[1];
        if (pos[2] < min_z) min_z = pos[2];
        if (pos[2] > max_z) max_z = pos[2];
    }

    out_aabb->min_x = min_x;
    out_aabb->min_y = min_y;
    out_aabb->min_z = min_z;
    out_aabb->max_x = max_x;
    out_aabb->max_y = max_y;
    out_aabb->max_z = max_z;

    return 0;
}

// Compute bounding sphere from AABB
static void geometry_mesh_data_sphere_from_aabb(const aabb_t* aabb, bounding_sphere_t* out_sphere)
{
    if (!aabb || !out_sphere) return;

    // Center is at midpoint of AABB
    out_sphere->center_x = (aabb->min_x + aabb->max_x) * 0.5f;
    out_sphere->center_y = (aabb->min_y + aabb->max_y) * 0.5f;
    out_sphere->center_z = (aabb->min_z + aabb->max_z) * 0.5f;

    // Radius is half the diagonal
    float dx = aabb->max_x - aabb->min_x;
    float dy = aabb->max_y - aabb->min_y;
    float dz = aabb->max_z - aabb->min_z;
    out_sphere->radius = sqrtf(dx*dx + dy*dy + dz*dz) * 0.5f;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static geometry_mesh_data_internal_t* geometry_mesh_data_get_internal(geometry_mesh_data_handle_t handle)
{
    if (handle.id >= g_mesh_data_ctx.count) return NULL;
    geometry_mesh_data_internal_t* item = &g_mesh_data_ctx.items[handle.id];
    if (!item->initialized) return NULL;
    return item;
}

static bool geometry_mesh_data_validate_desc(const geometry_mesh_data_desc_t* desc)
{
    if (!desc) return false;
    if (desc->vertex_count == 0 || desc->index_count == 0) return false;
    if (!desc->vertex_data || !desc->index_data) return false;
    if (desc->vertex_data_size == 0 || desc->index_data_size == 0) return false;
    if (desc->index_type >= INDEX_TYPE_COUNT) return false;
    if (!geometry_vertex_format_is_valid(desc->vertex_format)) return false;
    if (desc->submesh_count > 0 && !desc->submeshes) return false;
    return true;
}

static void geometry_mesh_data_cleanup_internal(geometry_mesh_data_internal_t* item)
{
    if (!item) return;
    if (item->vertex_buffer) {
        free(item->vertex_buffer);
        item->vertex_buffer = NULL;
    }
    if (item->index_buffer) {
        free(item->index_buffer);
        item->index_buffer = NULL;
    }
    if (item->submeshes) {
        free(item->submeshes);
        item->submeshes = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int geometry_mesh_data_init(void)
{
    if (g_mesh_data_ctx.initialized) {
        return 0;
    }

    g_mesh_data_ctx.capacity = GEOMETRY_MESH_DATA_DEFAULT_CAPACITY;
    g_mesh_data_ctx.items = calloc(g_mesh_data_ctx.capacity, sizeof(geometry_mesh_data_internal_t));
    if (!g_mesh_data_ctx.items) {
        return -1;
    }

    g_mesh_data_ctx.count = 0;
    g_mesh_data_ctx.initialized = true;

    return 0;
}

void geometry_mesh_data_shutdown(void)
{
    if (!g_mesh_data_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_mesh_data_ctx.count; i++) {
        geometry_mesh_data_cleanup_internal(&g_mesh_data_ctx.items[i]);
    }

    free(g_mesh_data_ctx.items);
    g_mesh_data_ctx.items = NULL;
    g_mesh_data_ctx.count = 0;
    g_mesh_data_ctx.capacity = 0;
    g_mesh_data_ctx.initialized = false;
}

int geometry_mesh_data_create(geometry_mesh_data_handle_t* out_handle, const geometry_mesh_data_desc_t* desc)
{
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_mesh_data_ctx.initialized) {
        return -2;
    }

    if (!geometry_mesh_data_validate_desc(desc)) {
        return -3;
    }

    if (g_mesh_data_ctx.count >= g_mesh_data_ctx.capacity) {
        return -4;
    }

    uint32_t index = g_mesh_data_ctx.count++;
    geometry_mesh_data_internal_t* item = &g_mesh_data_ctx.items[index];

    memset(item, 0, sizeof(geometry_mesh_data_internal_t));
    item->handle.id = index;

    // Copy vertex data
    item->vertex_buffer = malloc(desc->vertex_data_size);
    if (!item->vertex_buffer) {
        g_mesh_data_ctx.count--;
        return -5;
    }
    memcpy(item->vertex_buffer, desc->vertex_data, desc->vertex_data_size);

    // Copy index data
    item->index_buffer = malloc(desc->index_data_size);
    if (!item->index_buffer) {
        free(item->vertex_buffer);
        g_mesh_data_ctx.count--;
        return -6;
    }
    memcpy(item->index_buffer, desc->index_data, desc->index_data_size);

    // Copy submeshes
    if (desc->submesh_count > 0) {
        size_t submesh_size = desc->submesh_count * sizeof(submesh_t);
        item->submeshes = malloc(submesh_size);
        if (!item->submeshes) {
            free(item->vertex_buffer);
            free(item->index_buffer);
            g_mesh_data_ctx.count--;
            return -7;
        }
        memcpy(item->submeshes, desc->submeshes, submesh_size);
    }

    // Fill mesh_data
    item->data.vertex_data = item->vertex_buffer;
    item->data.index_data = item->index_buffer;
    item->data.vertex_count = desc->vertex_count;
    item->data.index_count = desc->index_count;
    item->data.vertex_data_size = desc->vertex_data_size;
    item->data.index_data_size = desc->index_data_size;
    item->data.vertex_format = desc->vertex_format;
    item->data.index_type = desc->index_type;
    item->data.submeshes = item->submeshes;
    item->data.submesh_count = desc->submesh_count;
    item->data.flags = desc->flags;

    // Get vertex stride from format
    geometry_vertex_format_info_t fmt_info;
    geometry_vertex_format_get_info(desc->vertex_format, &fmt_info);
    item->data.vertex_stride = fmt_info.vertex_stride;

    // Calculate bounds
    int res = geometry_mesh_data_compute_aabb(
        item->vertex_buffer,
        desc->vertex_count,
        fmt_info.vertex_stride,
        &item->data.bounds
    );
    if (res == 0) {
        geometry_mesh_data_sphere_from_aabb(&item->data.bounds, &item->data.bounding_sphere);
    }

    item->flags = desc->flags;
    item->initialized = true;
    item->frame_updated = 0;

    *out_handle = item->handle;
    return 0;
}

void geometry_mesh_data_destroy(geometry_mesh_data_handle_t handle)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item) return;

    geometry_mesh_data_cleanup_internal(item);
}

bool geometry_mesh_data_is_valid(geometry_mesh_data_handle_t handle)
{
    return geometry_mesh_data_get_internal(handle) != NULL;
}

int geometry_mesh_data_get_info(geometry_mesh_data_handle_t handle, geometry_mesh_data_info_t* out_info)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || !out_info) return -1;

    out_info->id = handle.id;
    out_info->vertex_count = item->data.vertex_count;
    out_info->index_count = item->data.index_count;
    out_info->submesh_count = item->data.submesh_count;
    out_info->total_size = item->data.vertex_data_size + item->data.index_data_size;
    out_info->flags = item->data.flags;
    out_info->initialized = item->initialized;
    memcpy(&out_info->bounds, &item->data.bounds, sizeof(aabb_t));

    return 0;
}

const mesh_data_t* geometry_mesh_data_get_data(geometry_mesh_data_handle_t handle)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item) return NULL;
    return &item->data;
}

int geometry_mesh_data_get_vertex_data(geometry_mesh_data_handle_t handle, const void** out_data, size_t* out_size)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || !out_data || !out_size) return -1;

    *out_data = item->data.vertex_data;
    *out_size = item->data.vertex_data_size;
    return 0;
}

int geometry_mesh_data_get_index_data(geometry_mesh_data_handle_t handle, const void** out_data, size_t* out_size)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || !out_data || !out_size) return -1;

    *out_data = item->data.index_data;
    *out_size = item->data.index_data_size;
    return 0;
}

uint32_t geometry_mesh_data_get_submesh_count(geometry_mesh_data_handle_t handle)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item) return 0;
    return item->data.submesh_count;
}

int geometry_mesh_data_get_submesh(geometry_mesh_data_handle_t handle, uint32_t submesh_index, submesh_t* out_submesh)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || !out_submesh || submesh_index >= item->data.submesh_count) return -1;

    memcpy(out_submesh, &item->data.submeshes[submesh_index], sizeof(submesh_t));
    return 0;
}

int geometry_mesh_data_set_submesh_material(geometry_mesh_data_handle_t handle, uint32_t submesh_index, uint32_t material_id)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || submesh_index >= item->data.submesh_count) return -1;

    item->data.submeshes[submesh_index].material_id = material_id;
    return 0;
}

int geometry_mesh_data_set_submesh_visibility(geometry_mesh_data_handle_t handle, uint32_t submesh_index, bool visible)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || submesh_index >= item->data.submesh_count) return -1;

    item->data.submeshes[submesh_index].visible = visible;
    return 0;
}

int geometry_mesh_data_get_bounds(geometry_mesh_data_handle_t handle, aabb_t* out_aabb)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || !out_aabb) return -1;

    memcpy(out_aabb, &item->data.bounds, sizeof(aabb_t));
    return 0;
}

int geometry_mesh_data_get_bounding_sphere(geometry_mesh_data_handle_t handle, bounding_sphere_t* out_sphere)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item || !out_sphere) return -1;

    memcpy(out_sphere, &item->data.bounding_sphere, sizeof(bounding_sphere_t));
    return 0;
}

int geometry_mesh_data_recalculate_bounds(geometry_mesh_data_handle_t handle)
{
    geometry_mesh_data_internal_t* item = geometry_mesh_data_get_internal(handle);
    if (!item) return -1;

    int res = geometry_mesh_data_compute_aabb(
        item->data.vertex_data,
        item->data.vertex_count,
        item->data.vertex_stride,
        &item->data.bounds
    );
    if (res == 0) {
        geometry_mesh_data_sphere_from_aabb(&item->data.bounds, &item->data.bounding_sphere);
    }
    return res;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================ */

uint32_t geometry_mesh_data_get_count(void)
{
    return g_mesh_data_ctx.count;
}

size_t geometry_mesh_data_get_memory_usage(void)
{
    size_t total = 0;

    for (uint32_t i = 0; i < g_mesh_data_ctx.count; i++) {
        geometry_mesh_data_internal_t* item = &g_mesh_data_ctx.items[i];
        if (!item->initialized) continue;

        total += sizeof(geometry_mesh_data_internal_t);
        total += item->data.vertex_data_size;
        total += item->data.index_data_size;
        if (item->submeshes) {
            total += item->data.submesh_count * sizeof(submesh_t);
        }
    }

    return total;
}

void geometry_mesh_data_debug_print(void)
{
    if (!g_mesh_data_ctx.initialized) {
        printf("Mesh Data System: Not initialized\n");
        return;
    }

    printf("=== Mesh Data System Debug ===\n");
    printf("Count: %u / %u\n", g_mesh_data_ctx.count, g_mesh_data_ctx.capacity);
    printf("Memory usage: %zu bytes\n", geometry_mesh_data_get_memory_usage());
    printf("\nMeshes:\n");

    for (uint32_t i = 0; i < g_mesh_data_ctx.count; i++) {
        geometry_mesh_data_internal_t* item = &g_mesh_data_ctx.items[i];
        if (!item->initialized) continue;

        printf("  Mesh #%u:\n", item->handle.id);
        printf("    Vertices: %u (stride=%u, total=%zu bytes)\n",
               item->data.vertex_count, item->data.vertex_stride, item->data.vertex_data_size);
        printf("    Indices: %u (%s, total=%zu bytes)\n",
               item->data.index_count, geometry_index_type_get_name(item->data.index_type),
               item->data.index_data_size);
        printf("    Submeshes: %u\n", item->data.submesh_count);
        printf("    Bounds: [%.2f, %.2f, %.2f] to [%.2f, %.2f, %.2f]\n",
               item->data.bounds.min_x, item->data.bounds.min_y, item->data.bounds.min_z,
               item->data.bounds.max_x, item->data.bounds.max_y, item->data.bounds.max_z);
        printf("    Sphere: center=[%.2f, %.2f, %.2f] radius=%.2f\n",
               item->data.bounding_sphere.center_x, item->data.bounding_sphere.center_y,
               item->data.bounding_sphere.center_z, item->data.bounding_sphere.radius);

        for (uint32_t s = 0; s < item->data.submesh_count; s++) {
            const submesh_t* sub = &item->data.submeshes[s];
            printf("    Submesh %u: indices [%u-%u] (%u), vertices [%u-%u], material=%u, visible=%d\n",
                   s, sub->index_offset, sub->index_offset + sub->index_count - 1, sub->index_count,
                   sub->vertex_offset, sub->vertex_offset + sub->vertex_count - 1,
                   sub->material_id, sub->visible);
        }
    }
}
