#ifndef EDITOR_TERRAIN_EDITOR_SCULPTING_H
#define EDITOR_TERRAIN_EDITOR_SCULPTING_H

#include "core/core.h"
#include "core/math/vec3.h"
#include "renderer/render_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
//                           TERRAIN SCULPTING TOOLS - AGENT_EDITOR_2
// =================================================================================================

// Brush types for terrain sculpting
typedef enum {
    BRUSH_TYPE_CIRCLE,        // Circular brush
    BRUSH_TYPE_SQUARE,        // Square brush
    BRUSH_TYPE_TEXTURE        // Texture-based brush
} BrushType;

// Sculpting operations
typedef enum {
    SCULPT_RAISE,             // Raise terrain
    SCULPT_LOWER,             // Lower terrain
    SCULPT_FLATTEN,           // Flatten to target height
    SCULPT_SMOOTH,            // Smooth/average heights
    SCULPT_RAMP,              // Create linear slope
    SCULPT_NOISE,             // Add random variation
    SCULPT_PAINT_HOLES,       // Paint visibility mask
    SCULPT_PAINT_LAYERS       // Paint texture weights
} SculptOperation;

// Falloff curve types
typedef enum {
    FALLOFF_LINEAR,           // Linear falloff
    FALLOFF_SMOOTH,           // Smooth falloff
    FALLOFF_SHARP,            // Sharp falloff
    FALLOFF_CONSTANT          // No falloff
} FalloffType;

// Terrain brush structure
typedef struct {
    BrushType type;
    SculptOperation operation;
    FalloffType falloff;
    
    // Brush properties
    f32 radius;               // Brush radius in world units
    f32 strength;             // Brush strength (0.0 - 1.0)
    f32 hardness;             // Brush hardness (0.0 - 1.0)
    f32 target_height;        // Target height for flatten operation
    
    // Texture brush
    u32 texture_id;           // Texture for brush shape
    char texture_path[256];    // Path to texture file
    
    // Ramp operation
    vec3 ramp_start;          // Start point for ramp
    vec3 ramp_end;            // End point for ramp
    
    // Noise operation
    f32 noise_scale;          // Scale of noise
    f32 noise_strength;       // Strength of noise
    u32 noise_seed;           // Random seed
    
    // Layer painting
    u32 layer_count;          // Number of texture layers
    f32 layer_weights[8];     // Weight for each layer (max 8 layers)
    
    bool is_active;
} TerrainBrush;

// Terrain heightmap data
typedef struct {
    f32* heights;             // Height values
    u8* visibility_mask;       // Visibility mask for holes
    f32* layer_weights;       // Texture layer weights per vertex
    u32 width;                // Heightmap width
    u32 height;               // Heightmap height
    f32 scale_x;              // X scale per pixel
    f32 scale_z;              // Z scale per pixel
    f32 height_scale;         // Height scale multiplier
} TerrainHeightmap;

// Sculpting history for undo/redo
typedef struct {
    f32* height_data;         // Modified height data
    u32* modified_pixels;     // List of modified pixel indices
    u32 pixel_count;          // Number of modified pixels
    vec3 brush_position;      // Brush position
    TerrainBrush brush;       // Brush used for operation
} SculptHistoryEntry;

// Terrain sculpting system
typedef struct {
    TerrainBrush current_brush;
    TerrainHeightmap* heightmap;
    
    // History for undo/redo
    SculptHistoryEntry* history;
    u32 history_size;
    u32 history_capacity;
    u32 current_history_index;
    
    // Performance tracking
    u64 last_sculpt_time;
    f32 average_sculpt_time;
    u32 pixels_modified_last_frame;
    
    bool is_sculpting;
    bool auto_save;
} TerrainSculptingSystem;

// =================================================================================================
//                                    CORE API FUNCTIONS
// =================================================================================================

// System management
TerrainSculptingSystem* terrain_sculpting_create(TerrainHeightmap* heightmap);
void terrain_sculpting_destroy(TerrainSculptingSystem* system);
void terrain_sculpting_set_heightmap(TerrainSculptingSystem* system, TerrainHeightmap* heightmap);

// Brush management
void terrain_sculpting_set_brush_type(TerrainSculptingSystem* system, BrushType type);
void terrain_sculpting_set_operation(TerrainSculptingSystem* system, SculptOperation operation);
void terrain_sculpting_set_falloff(TerrainSculptingSystem* system, FalloffType falloff);
void terrain_sculpting_set_brush_radius(TerrainSculptingSystem* system, f32 radius);
void terrain_sculpting_set_brush_strength(TerrainSculptingSystem* system, f32 strength);
void terrain_sculpting_set_brush_hardness(TerrainSculptingSystem* system, f32 hardness);
void terrain_sculpting_set_texture_brush(TerrainSculptingSystem* system, const char* texture_path);

// Sculpting operations
void terrain_sculpting_start(TerrainSculptingSystem* system, vec3 world_position);
void terrain_sculpting_update(TerrainSculptingSystem* system, vec3 world_position);
void terrain_sculpting_end(TerrainSculptingSystem* system);
bool terrain_sculpting_apply_brush(TerrainSculptingSystem* system, vec3 world_position);

// Heightmap utilities
TerrainHeightmap* terrain_heightmap_create(u32 width, u32 height);
void terrain_heightmap_destroy(TerrainHeightmap* heightmap);
f32 terrain_heightmap_get_height(TerrainHeightmap* heightmap, u32 x, u32 z);
void terrain_heightmap_set_height(TerrainHeightmap* heightmap, u32 x, u32 z, f32 height);
vec3 terrain_heightmap_world_to_pixel(TerrainHeightmap* heightmap, vec3 world_pos);
vec3 terrain_heightmap_pixel_to_world(TerrainHeightmap* heightmap, u32 x, u32 z);

// Brush utilities
f32 terrain_brush_get_falloff_weight(TerrainBrush* brush, f32 distance);
bool terrain_brush_is_in_range(TerrainBrush* brush, vec3 brush_pos, vec3 test_pos);
f32 terrain_brush_sample_texture(TerrainBrush* brush, vec2 uv);

// Undo/redo system
void terrain_sculpting_undo(TerrainSculptingSystem* system);
void terrain_sculpting_redo(TerrainSculptingSystem* system);
void terrain_sculpting_clear_history(TerrainSculptingSystem* system);
bool terrain_sculpting_can_undo(TerrainSculptingSystem* system);
bool terrain_sculpting_can_redo(TerrainSculptingSystem* system);

// Performance and statistics
void terrain_sculpting_get_stats(TerrainSculptingSystem* system, u32* history_size, 
                                f32* average_time, u32* pixels_last_frame);

// =================================================================================================
//                             SPECIFIC SCULPTING OPERATIONS
// =================================================================================================

// Basic operations
void terrain_sculpt_raise_height(TerrainSculptingSystem* system, vec3 position, f32 strength);
void terrain_sculpt_lower_height(TerrainSculptingSystem* system, vec3 position, f32 strength);
void terrain_sculpt_flatten_height(TerrainSculptingSystem* system, vec3 position, f32 target_height);
void terrain_sculpt_smooth_height(TerrainSculptingSystem* system, vec3 position, f32 radius);

// Advanced operations
void terrain_sculpt_create_ramp(TerrainSculptingSystem* system, vec3 start, vec3 end, f32 strength);
void terrain_sculpt_add_noise(TerrainSculptingSystem* system, vec3 position, f32 scale, f32 strength);
void terrain_sculpt_paint_holes(TerrainSculptingSystem* system, vec3 position, bool create_hole);
void terrain_sculpt_paint_layers(TerrainSculptingSystem* system, vec3 position, u32 layer_index, f32 weight);

// =================================================================================================
//                                   BRUSH BLENDING
// =================================================================================================

// Brush blending algorithms
f32 terrain_blend_linear(f32 current, f32 target, f32 weight);
f32 terrain_blend_smooth(f32 current, f32 target, f32 weight);
f32 terrain_blend_additive(f32 current, f32 target, f32 weight);
f32 terrain_blend_subtractive(f32 current, f32 target, f32 weight);

// Multi-brush blending
void terrain_sculpting_blend_multiple(TerrainSculptingSystem* system, TerrainBrush* brushes, u32 brush_count);

// =================================================================================================
//                                    DEBUG UTILITIES
// =================================================================================================

void terrain_sculpting_print_brush_info(TerrainBrush* brush);
void terrain_sculpting_print_heightmap_info(TerrainHeightmap* heightmap);
void terrain_sculpting_validate_heightmap(TerrainHeightmap* heightmap);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_TERRAIN_EDITOR_SCULPTING_H
