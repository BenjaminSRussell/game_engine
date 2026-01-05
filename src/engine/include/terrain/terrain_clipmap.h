#ifndef TERRAIN_CLIPMAP_H
#define TERRAIN_CLIPMAP_H

#include "../core/types.h"
#include "../math/vec3.h"
#include "../math/mat4.h"
#include "../core/memory.h"

// Clipmap terrain configuration
#define TERRAIN_CLIPMAP_LEVELS 12
#define TERRAIN_CLIPMAP_SIZE 256
#define TERRAIN_CLIPMAP_SCALE 2.0f
#define TERRAIN_CLIPMAP_MAX_DISTANCE 8192.0f
#define TERRAIN_CLIPMAP_UPDATE_DISTANCE 64.0f

// Clipmap level data
typedef struct {
    u32 level;
    f32 scale;
    f32 world_size;
    Vec3 center;
    Vec3 offset;
    
    // Heightmap data
    u32 heightmap_size;
    f32* heightmap_data;
    u32 heightmap_texture_id;
    
    // Normal map data
    u32 normalmap_texture_id;
    
    // Mesh data
    u32 vertex_buffer_id;
    u32 index_buffer_id;
    u32 vertex_count;
    u32 index_count;
    
    // Update state
    bool needs_update;
    bool is_active;
    u32 update_frame;
} TerrainClipmapLevel;

// Clipmap terrain system
typedef struct {
    // Clipmap levels
    TerrainClipmapLevel levels[TERRAIN_CLIPMAP_LEVELS];
    u32 active_levels;
    
    // Viewer position
    Vec3 viewer_position;
    Vec3 last_viewer_position;
    
    // Heightmap source
    u32 source_heightmap_size;
    f32* source_heightmap;
    f32 terrain_scale_x;
    f32 terrain_scale_y;
    f32 terrain_scale_z;
    
    // Rendering
    u32 shader_program_id;
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Vec3 light_direction;
    Vec3 light_color;
    f32 ambient_intensity;
    
    // Performance
    u32 max_updates_per_frame;
    u32 current_update_level;
    bool enable_lod_transitions;
    f32 lod_transition_distance;
    
    // Statistics
    u32 total_vertices_rendered;
    u32 total_triangles_rendered;
    u64 update_time_ms;
    u64 render_time_ms;
} TerrainClipmapSystem;

// Core API functions
void terrain_clipmap_init(TerrainClipmapSystem* system);
void terrain_clipmap_shutdown(TerrainClipmapSystem* system);
void terrain_clipmap_update(TerrainClipmapSystem* system, Vec3 viewer_position, f32 delta_time);
void terrain_clipmap_render(TerrainClipmapSystem* system);

// Heightmap management
bool terrain_clipmap_load_heightmap(TerrainClipmapSystem* system, const char* heightmap_path);
void terrain_clipmap_set_heightmap_data(TerrainClipmapSystem* system, f32* data, u32 width, u32 height);
f32 terrain_clipmap_get_height_at(TerrainClipmapSystem* system, f32 world_x, f32 world_z);
Vec3 terrain_clipmap_get_normal_at(TerrainClipmapSystem* system, f32 world_x, f32 world_z);

// Level management
void terrain_clipmap_update_levels(TerrainClipmapSystem* system);
void terrain_clipmap_update_level(TerrainClipmapSystem* system, u32 level_index);
bool terrain_clipmap_is_level_active(TerrainClipmapSystem* system, u32 level_index);
f32 terrain_clipmap_get_level_scale(TerrainClipmapSystem* system, u32 level_index);

// LOD management
u32 terrain_clipmap_get_lod_level(TerrainClipmapSystem* system, Vec3 world_position);
void terrain_clipmap_enable_lod_transitions(TerrainClipmapSystem* system, bool enable);
void terrain_clipmap_set_lod_transition_distance(TerrainClipmapSystem* system, f32 distance);

// Rendering
void terrain_clipmap_set_view_matrix(TerrainClipmapSystem* system, Mat4 view_matrix);
void terrain_clipmap_set_projection_matrix(TerrainClipmapSystem* system, Mat4 projection_matrix);
void terrain_clipmap_set_lighting(TerrainClipmapSystem* system, Vec3 light_direction, Vec3 light_color, f32 ambient_intensity);

// Utility functions
Vec3 terrain_clipmap_world_to_clipmap_coords(TerrainClipmapSystem* system, Vec3 world_pos, u32 level);
Vec3 terrain_clipmap_clipmap_to_world_coords(TerrainClipmapSystem* system, Vec3 clipmap_pos, u32 level);
bool terrain_clipmap_is_in_bounds(TerrainClipmapSystem* system, Vec3 world_pos);

// Performance
void terrain_clipmap_set_max_updates_per_frame(TerrainClipmapSystem* system, u32 max_updates);
void terrain_clipmap_get_statistics(TerrainClipmapSystem* system, u32* vertices, u32* triangles, u64* update_time, u64* render_time);

#endif // TERRAIN_CLIPMAP_H
