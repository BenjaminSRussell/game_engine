#include "../editor_common.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <math.h>

/**
 * =================================================================================================
 *                           TERRAIN EDITOR VIEWPORT
 * =================================================================================================
 * 
 * Handles 3D visualization of the terrain in the editor.
 * Responsible for:
 * 1. Generating mesh data (vertices/indices) from heightmap
 * 2. Rendering the terrain mesh
 * 3. Handling Level of Detail (LOD) - simplified for now
 * 4. Drawing editor helpers (grid, brush cursor)
 */

typedef struct {
    Vec3 position;
    Vec3 normal;
    Vec2 texcoord;
} TerrainVertex;

typedef struct {
    u32 vao;
    u32 vbo;
    u32 ibo;
    u32 index_count;
    bool is_dirty;
} TerrainMesh;

// Global state for simplicity in this refactor
static TerrainMesh g_terrain_mesh = {0};

// Forward declarations
static void generate_terrain_mesh(TerrainHeightmap* heightmap, TerrainMesh* mesh);
static void calculate_normals(TerrainHeightmap* heightmap, TerrainVertex* vertices);

void terrain_editor_viewport_init(EditorContext* ctx) {
    g_terrain_mesh.is_dirty = true;
    LOG_INFO("Terrain Editor Viewport Initialized");
}

void terrain_editor_viewport_render(EditorContext* ctx) {
    if (!ctx || !ctx->terrain_system || !ctx->terrain_system->heightmap) return;
    
    TerrainHeightmap* heightmap = ctx->terrain_system->heightmap;
    
    // Regenerate mesh if needed (e.g. after sculpting)
    // In a real engine, we'd update only the modified chunks
    if (g_terrain_mesh.is_dirty) {
        generate_terrain_mesh(heightmap, &g_terrain_mesh);
        g_terrain_mesh.is_dirty = false;
    }
    
    // Setup camera/view matrix
    // render_set_camera(ctx->viewport.view_matrix, ctx->viewport.projection_matrix);
    
    // Render Terrain
    // render_bind_shader("terrain_standard");
    // render_bind_mesh(g_terrain_mesh.vao);
    // render_draw_indexed(g_terrain_mesh.index_count);
    
    // Render Grid
    if (ctx->show_grid) {
        // render_draw_grid(ctx->grid_size);
    }
    
    // Render Gizmos
    if (ctx->show_gizmos) {
        // render_draw_axis_gizmo();
    }
}

// Mark mesh as needing update (called by sculpting system)
void terrain_editor_viewport_invalidate(void) {
    g_terrain_mesh.is_dirty = true;
}

static void generate_terrain_mesh(TerrainHeightmap* heightmap, TerrainMesh* mesh) {
    u32 width = heightmap->width;
    u32 height = heightmap->height;
    u32 vertex_count = width * height;
    u32 index_count = (width - 1) * (height - 1) * 6;
    
    TerrainVertex* vertices = (TerrainVertex*)malloc(sizeof(TerrainVertex) * vertex_count);
    u32* indices = (u32*)malloc(sizeof(u32) * index_count);
    
    if (!vertices || !indices) {
        LOG_ERROR("Failed to allocate terrain mesh data");
        if (vertices) free(vertices);
        if (indices) free(indices);
        return;
    }
    
    // Generate Vertices
    for (u32 z = 0; z < height; z++) {
        for (u32 x = 0; x < width; x++) {
            u32 i = z * width + x;
            
            f32 y = heightmap->heights[i] * heightmap->height_scale;
            
            vertices[i].position = (Vec3){
                (f32)x * heightmap->scale_x,
                y,
                (f32)z * heightmap->scale_z
            };
            
            vertices[i].texcoord = (Vec2){
                (f32)x / (f32)width,
                (f32)z / (f32)height
            };
            
            // Normals calculated later
            vertices[i].normal = (Vec3){0, 1, 0}; 
        }
    }
    
    // Calculate Normals
    calculate_normals(heightmap, vertices);
    
    // Generate Indices
    u32 idx = 0;
    for (u32 z = 0; z < height - 1; z++) {
        for (u32 x = 0; x < width - 1; x++) {
            u32 tl = z * width + x;
            u32 tr = tl + 1;
            u32 bl = (z + 1) * width + x;
            u32 br = bl + 1;
            
            // Triangle 1
            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;
            
            // Triangle 2
            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }
    
    // Upload to GPU (Mock)
    mesh->index_count = index_count;
    // mesh->vao = render_create_mesh(vertices, vertex_count, indices, index_count);
    
    // LOG_INFO("Generated terrain mesh: %u vertices, %u indices", vertex_count, index_count);
    
    free(vertices);
    free(indices);
}

static void calculate_normals(TerrainHeightmap* heightmap, TerrainVertex* vertices) {
    u32 width = heightmap->width;
    u32 height = heightmap->height;
    
    for (u32 z = 0; z < height; z++) {
        for (u32 x = 0; x < width; x++) {
            // Compute central difference
            f32 hL = (x > 0) ? vertices[z * width + (x - 1)].position.y : vertices[z * width + x].position.y;
            f32 hR = (x < width - 1) ? vertices[z * width + (x + 1)].position.y : vertices[z * width + x].position.y;
            f32 hD = (z > 0) ? vertices[(z - 1) * width + x].position.y : vertices[z * width + x].position.y;
            f32 hU = (z < height - 1) ? vertices[(z + 1) * width + x].position.y : vertices[z * width + x].position.y;
            
            Vec3 normal;
            normal.x = hL - hR;
            normal.y = 2.0f; // Scale factor
            normal.z = hD - hU;
            
            // Normalize
            f32 len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
            if (len > 0.0001f) {
                normal.x /= len;
                normal.y /= len;
                normal.z /= len;
            }
            
            vertices[z * width + x].normal = normal;
        }
    }
}
