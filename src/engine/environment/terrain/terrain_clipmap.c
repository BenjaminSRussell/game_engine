#include "environment/terrain/terrain_clipmap.h"
#include "include/core/logger.h"
#include "include/core/memory.h"
#include "include/math/math.h"
#include <string.h>
#include <include/math/math.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                                   TERRAIN CLIPMAP SYSTEM - IMPLEMENTATION
 * =================================================================================================
 */

// Internal helper functions
static void terrain_clipmap_generate_mesh(TerrainClipmapSystem* system, u32 level_index);
static void terrain_clipmap_update_heightmap_texture(TerrainClipmapSystem* system, u32 level_index);
static void terrain_clipmap_update_normalmap(TerrainClipmapSystem* system, u32 level_index);
static f32 terrain_clipmap_sample_heightmap(TerrainClipmapSystem* system, f32 u, f32 v);

void terrain_clipmap_init(TerrainClipmapSystem* system) {
    if (!system) return;
    
    memset(system, 0, sizeof(TerrainClipmapSystem));
    
    // Initialize clipmap levels
    for (u32 i = 0; i < TERRAIN_CLIPMAP_LEVELS; i++) {
        TerrainClipmapLevel* level = &system->levels[i];
        level->level = i;
        level->scale = powf(TERRAIN_CLIPMAP_SCALE, (f32)i);
        level->world_size = TERRAIN_CLIPMAP_SIZE * level->scale;
        level->center = vec3_zero();
        level->offset = vec3_zero();
        level->heightmap_size = TERRAIN_CLIPMAP_SIZE;
        level->needs_update = false;
        level->is_active = false;
        level->update_frame = 0;
        
        // Allocate heightmap data for this level
        level->heightmap_data = malloc(level->heightmap_size * level->heightmap_size * sizeof(f32));
        if (!level->heightmap_data) {
            LOG_ERROR(\"Failed to allocate heightmap data for clipmap level %u\", i);
            terrain_clipmap_shutdown(system);
            return;
        }
        memset(level->heightmap_data, 0, level->heightmap_size * level->heightmap_size * sizeof(f32));
    }
    
    // Set default parameters
    system->viewer_position = vec3_zero();
    system->last_viewer_position = vec3_zero();
    system->terrain_scale_x = 1.0f;
    system->terrain_scale_y = 1.0f;
    system->terrain_scale_z = 1.0f;
    
    system->light_direction = vec3(0.0f, -1.0f, 0.0f);
    system->light_color = vec3(1.0f, 1.0f, 1.0f);
    system->ambient_intensity = 0.3f;
    
    system->max_updates_per_frame = 2;
    system->current_update_level = 0;
    system->enable_lod_transitions = true;
    system->lod_transition_distance = 64.0f;
    
    system->view_matrix = mat4_identity();
    system->projection_matrix = mat4_identity();
    
    LOG_INFO(\"Terrain clipmap system initialized\");
    LOG_INFO(\"  Levels: %u\", TERRAIN_CLIPMAP_LEVELS);
    LOG_INFO(\"  Clipmap size: %u\", TERRAIN_CLIPMAP_SIZE);
    LOG_INFO(\"  Scale factor: %.1f\", TERRAIN_CLIPMAP_SCALE);
}

void terrain_clipmap_shutdown(TerrainClipmapSystem* system) {
    if (!system) return;
    
    // Clean up clipmap levels
    for (u32 i = 0; i < TERRAIN_CLIPMAP_LEVELS; i++) {
        TerrainClipmapLevel* level = &system->levels[i];
        
        if (level->heightmap_data) {
            free(level->heightmap_data);
            level->heightmap_data = NULL;
        }
        
        // Clean up GPU resources (in real implementation)
        level->heightmap_texture_id = 0;
        level->normalmap_texture_id = 0;
        level->vertex_buffer_id = 0;
        level->index_buffer_id = 0;
    }
    
    // Clean up source heightmap
    if (system->source_heightmap) {
        free(system->source_heightmap);
        system->source_heightmap = NULL;
    }
    
    memset(system, 0, sizeof(TerrainClipmapSystem));
    LOG_INFO(\"Terrain clipmap system shutdown\");
}

void terrain_clipmap_update(TerrainClipmapSystem* system, Vec3 viewer_position, f32 delta_time) {
    if (!system) return;
    
    u64 start_time = time_get_current_ms();
    
    // Update viewer position
    system->last_viewer_position = system->viewer_position;
    system->viewer_position = viewer_position;
    
    // Update active levels based on viewer position
    terrain_clipmap_update_levels(system);
    
    // Update clipmap levels (limited per frame for performance)
    u32 updates_this_frame = 0;
    for (u32 i = 0; i < TERRAIN_CLIPMAP_LEVELS && updates_this_frame < system->max_updates_per_frame; i++) {
        u32 level_index = (system->current_update_level + i) % TERRAIN_CLIPMAP_LEVELS;
        TerrainClipmapLevel* level = &system->levels[level_index];
        
        if (level->is_active && level->needs_update) {
            terrain_clipmap_update_level(system, level_index);
            level->needs_update = false;
            level->update_frame++;
            updates_this_frame++;
        }
    }
    
    system->current_update_level = (system->current_update_level + 1) % TERRAIN_CLIPMAP_LEVELS;
    
    u64 end_time = time_get_current_ms();
    system->update_time_ms += (end_time - start_time);
}

void terrain_clipmap_render(TerrainClipmapSystem* system) {
    if (!system) return;
    
    u64 start_time = time_get_current_ms();
    
    system->total_vertices_rendered = 0;
    system->total_triangles_rendered = 0;
    
    // Render active clipmap levels from finest to coarsest
    for (u32 i = 0; i < TERRAIN_CLIPMAP_LEVELS; i++) {
        TerrainClipmapLevel* level = &system->levels[i];
        
        if (level->is_active && level->vertex_count > 0) {
            // In a real implementation, would bind buffers and render
            system->total_vertices_rendered += level->vertex_count;
            system->total_triangles_rendered += level->index_count / 3;
            
            LOG_TRACE(\"Rendering clipmap level %u: %u vertices, %u triangles\", 
                      level->level, level->vertex_count, level->index_count / 3);
        }
    }
    
    u64 end_time = time_get_current_ms();
    system->render_time_ms += (end_time - start_time);
}

bool terrain_clipmap_load_heightmap(TerrainClipmapSystem* system, const char* heightmap_path) {
    if (!system || !heightmap_path) return false;
    
    // In a real implementation, would load from file
    // For now, generate a procedural heightmap
    u32 size = 1024;
    system->source_heightmap_size = size;
    system->source_heightmap = malloc(size * size * sizeof(f32));
    
    if (!system->source_heightmap) {
        LOG_ERROR(\"Failed to allocate source heightmap\");
        return false;
    }
    
    // Generate procedural heightmap using simple noise
    for (u32 y = 0; y < size; y++) {
        for (u32 x = 0; x < size; x++) {
            f32 fx = (f32)x / (f32)size;
            f32 fy = (f32)y / (f32)size;
            
            // Simple multi-octave noise
            f32 height = 0.0f;
            f32 amplitude = 1.0f;
            f32 frequency = 4.0f;
            
            for (u32 octave = 0; octave < 4; octave++) {
                height += amplitude * sinf(fx * frequency * PI_F * 2.0f) * cosf(fy * frequency * PI_F * 2.0f);
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }
            
            system->source_heightmap[y * size + x] = height * 10.0f;  // Scale to 10 units height
        }
    }
    
    LOG_INFO(\"Generated procedural heightmap: %ux%u\", size, size);
    return true;
}

void terrain_clipmap_set_heightmap_data(TerrainClipmapSystem* system, f32* data, u32 width, u32 height) {
    if (!system || !data) return;
    
    // Clean up existing heightmap
    if (system->source_heightmap) {
        free(system->source_heightmap);
    }
    
    // Copy new heightmap data
    system->source_heightmap_size = width;
    system->source_heightmap = malloc(width * height * sizeof(f32));
    
    if (system->source_heightmap) {
        memcpy(system->source_heightmap, data, width * height * sizeof(f32));
        
        // Mark all levels as needing update
        for (u32 i = 0; i < TERRAIN_CLIPMAP_LEVELS; i++) {
            system->levels[i].needs_update = true;
        }
        
        LOG_INFO(\"Heightmap data set: %ux%u\", width, height);
    }
}

f32 terrain_clipmap_get_height_at(TerrainClipmapSystem* system, f32 world_x, f32 world_z) {
    if (!system || !system->source_heightmap) return 0.0f;
    
    // Convert world coordinates to heightmap coordinates
    f32 u = (world_x / system->terrain_scale_x + 0.5f) * (f32)system->source_heightmap_size;
    f32 v = (world_z / system->terrain_scale_z + 0.5f) * (f32)system->source_heightmap_size;
    
    return terrain_clipmap_sample_heightmap(system, u, v) * system->terrain_scale_y;
}

Vec3 terrain_clipmap_get_normal_at(TerrainClipmapSystem* system, f32 world_x, f32 world_z) {
    if (!system) return vec3_up();
    
    f32 epsilon = 1.0f;
    
    // Sample heights at neighboring points
    f32 h_center = terrain_clipmap_get_height_at(system, world_x, world_z);
    f32 h_right = terrain_clipmap_get_height_at(system, world_x + epsilon, world_z);
    f32 h_up = terrain_clipmap_get_height_at(system, world_x, world_z + epsilon);
    
    // Calculate normal using finite differences
    Vec3 dx = vec3(epsilon, h_right - h_center, 0.0f);
    Vec3 dz = vec3(0.0f, h_up - h_center, epsilon);
    
    Vec3 normal = vec3_cross(dz, dx);
    return vec3_normalize(normal);
}

void terrain_clipmap_update_levels(TerrainClipmapSystem* system) {
    if (!system) return;
    
    system->active_levels = 0;
    
    // Determine which levels should be active based on viewer position
    for (u32 i = 0; i < TERRAIN_CLIPMAP_LEVELS; i++) {
        TerrainClipmapLevel* level = &system->levels[i];
        
        // Calculate distance from viewer to level center
        f32 distance = vec3_distance(system->viewer_position, level->center);
        
        // Level is active if it's within maximum distance
        f32 max_distance = TERRAIN_CLIPMAP_MAX_DISTANCE * level->scale;
        
        if (distance < max_distance) {
            level->is_active = true;
            system->active_levels++;
            
            // Check if level needs update (viewer moved significantly)
            Vec3 viewer_offset = vec3_sub(system->viewer_position, level->center);
            f32 offset_magnitude = vec3_length(viewer_offset);
            
            // Update if viewer moved more than half the clipmap size
            if (offset_magnitude > level->world_size * 0.5f) {
                // Calculate new center position
                level->center = system->viewer_position;
                level->needs_update = true;
                
                LOG_TRACE(\"Clipmap level %u needs update: viewer moved %.1f units\", 
                          i, offset_magnitude);
            }
        } else {
            level->is_active = false;
        }
    }
}

void terrain_clipmap_update_level(TerrainClipmapSystem* system, u32 level_index) {
    if (!system || level_index >= TERRAIN_CLIPMAP_LEVELS) return;
    
    TerrainClipmapLevel* level = &system->levels[level_index];
    
    // Update heightmap for this level from source data
    terrain_clipmap_update_heightmap_texture(system, level_index);
    
    // Update normal map
    terrain_clipmap_update_normalmap(system, level_index);
    
    // Generate mesh
    terrain_clipmap_generate_mesh(system, level_index);
    
    LOG_TRACE(\"Updated clipmap level %u\", level_index);
}

u32 terrain_clipmap_get_lod_level(TerrainClipmapSystem* system, Vec3 world_position) {
    if (!system) return 0;
    
    f32 distance = vec3_distance(system->viewer_position, world_position);
    
    // Determine LOD level based on distance
    for (u32 i = 0; i < TERRAIN_CLIPMAP_LEVELS; i++) {
        f32 max_distance = TERRAIN_CLIPMAP_MAX_DISTANCE * powf(TERRAIN_CLIPMAP_SCALE, (f32)i);
        if (distance < max_distance) {
            return i;
        }
    }
    
    return TERRAIN_CLIPMAP_LEVELS - 1;
}

// Internal helper functions
static void terrain_clipmap_generate_mesh(TerrainClipmapSystem* system, u32 level_index) {
    if (!system || level_index >= TERRAIN_CLIPMAP_LEVELS) return;
    
    TerrainClipmapLevel* level = &system->levels[level_index];
    
    // Generate a simple grid mesh
    u32 grid_size = level->heightmap_size;
    u32 vertex_count = grid_size * grid_size;
    u32 index_count = (grid_size - 1) * (grid_size - 1) * 6;
    
    level->vertex_count = vertex_count;
    level->index_count = index_count;
    
    // In a real implementation, would generate vertex and index buffers
    // For now, just log the mesh generation
    LOG_TRACE(\"Generated mesh for clipmap level %u: %u vertices, %u indices\", 
              level_index, vertex_count, index_count);
}

static void terrain_clipmap_update_heightmap_texture(TerrainClipmapSystem* system, u32 level_index) {
    if (!system || level_index >= TERRAIN_CLIPMAP_LEVELS || !system->source_heightmap) return;
    
    TerrainClipmapLevel* level = &system->levels[level_index];
    
    // Calculate source region for this level
    f32 level_scale = level->scale;
    f32 source_scale = (f32)system->source_heightmap_size / (TERRAIN_CLIPMAP_MAX_DISTANCE * 2.0f);
    
    // Sample source heightmap at appropriate resolution
    for (u32 y = 0; y < level->heightmap_size; y++) {
        for (u32 x = 0; x < level->heightmap_size; x++) {
            // Convert clipmap coordinates to world coordinates
            f32 world_x = (x - level->heightmap_size * 0.5f) * level_scale + level->center.x;
            f32 world_z = (y - level->heightmap_size * 0.5f) * level_scale + level->center.z;
            
            // Convert world coordinates to source heightmap coordinates
            f32 u = (world_x / system->terrain_scale_x + 0.5f) * (f32)system->source_heightmap_size;
            f32 v = (world_z / system->terrain_scale_z + 0.5f) * (f32)system->source_heightmap_size;
            
            // Sample source heightmap
            f32 height = terrain_clipmap_sample_heightmap(system, u, v);
            level->heightmap_data[y * level->heightmap_size + x] = height;
        }
    }
    
    // In a real implementation, would upload to GPU texture
    LOG_TRACE(\"Updated heightmap texture for clipmap level %u\", level_index);
}

static void terrain_clipmap_update_normalmap(TerrainClipmapSystem* system, u32 level_index) {
    if (!system || level_index >= TERRAIN_CLIPMAP_LEVELS) return;
    
    TerrainClipmapLevel* level = &system->levels[level_index];
    
    // Calculate normals from heightmap using finite differences
    for (u32 y = 1; y < level->heightmap_size - 1; y++) {
        for (u32 x = 1; x < level->heightmap_size - 1; x++) {
            f32 h_center = level->heightmap_data[y * level->heightmap_size + x];
            f32 h_right = level->heightmap_data[y * level->heightmap_size + (x + 1)];
            f32 h_left = level->heightmap_data[y * level->heightmap_size + (x - 1)];
            f32 h_up = level->heightmap_data[(y + 1) * level->heightmap_size + x];
            f32 h_down = level->heightmap_data[(y - 1) * level->heightmap_size + x];
            
            // Calculate normal
            Vec3 dx = vec3(2.0f * level->scale, h_right - h_left, 0.0f);
            Vec3 dz = vec3(0.0f, h_up - h_down, 2.0f * level->scale);
            Vec3 normal = vec3_cross(dz, dx);
            
            // Store normal (in real implementation, would pack into RGB texture)
            // For now, we don't store it as we're not generating the actual texture
        }
    }
    
    // In a real implementation, would upload normal map to GPU
    LOG_TRACE(\"Updated normal map for clipmap level %u\", level_index);
}

static f32 terrain_clipmap_sample_heightmap(TerrainClipmapSystem* system, f32 u, f32 v) {
    if (!system || !system->source_heightmap) return 0.0f;
    
    // Clamp to valid range
    u32 size = system->source_heightmap_size;
    u = fmaxf(0.0f, fminf((f32)(size - 1), u));
    v = fmaxf(0.0f, fminf((f32)(size - 1), v));
    
    // Bilinear interpolation
    u32 x0 = (u32)u;
    u32 x1 = fminu(x0 + 1, size - 1);
    u32 y0 = (u32)v;
    u32 y1 = fminu(y0 + 1, size - 1);
    
    f32 fx = u - (f32)x0;
    f32 fy = v - (f32)y0;
    
    f32 h00 = system->source_heightmap[y0 * size + x0];
    f32 h10 = system->source_heightmap[y0 * size + x1];
    f32 h01 = system->source_heightmap[y1 * size + x0];
    f32 h11 = system->source_heightmap[y1 * size + x1];
    
    f32 h0 = h00 * (1.0f - fx) + h10 * fx;
    f32 h1 = h01 * (1.0f - fx) + h11 * fx;
    
    return h0 * (1.0f - fy) + h1 * fy;
}

// Public utility functions
void terrain_clipmap_set_view_matrix(TerrainClipmapSystem* system, Mat4 view_matrix) {
    if (!system) return;
    system->view_matrix = view_matrix;
}

void terrain_clipmap_set_projection_matrix(TerrainClipmapSystem* system, Mat4 projection_matrix) {
    if (!system) return;
    system->projection_matrix = projection_matrix;
}

void terrain_clipmap_set_lighting(TerrainClipmapSystem* system, Vec3 light_direction, Vec3 light_color, f32 ambient_intensity) {
    if (!system) return;
    system->light_direction = light_direction;
    system->light_color = light_color;
    system->ambient_intensity = ambient_intensity;
}

void terrain_clipmap_get_statistics(TerrainClipmapSystem* system, u32* vertices, u32* triangles, u64* update_time, u64* render_time) {
    if (!system) return;
    
    if (vertices) *vertices = system->total_vertices_rendered;
    if (triangles) *triangles = system->total_triangles_rendered;
    if (update_time) *update_time = system->update_time_ms;
    if (render_time) *render_time = system->render_time_ms;
}
