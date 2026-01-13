#ifndef BRUSH_SYSTEM_H
#define BRUSH_SYSTEM_H

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// Brush operation types
typedef enum {
    BRUSH_OP_RAISE,
    BRUSH_OP_LOWER,
    BRUSH_OP_FLATTEN,
    BRUSH_OP_SMOOTH,
    BRUSH_OP_NOISE,
    BRUSH_OP_EROSION,
    BRUSH_OP_PAINT
} BrushOperation;

// Brush shape types
typedef enum {
    BRUSH_SHAPE_CIRCLE,
    BRUSH_SHAPE_SQUARE,
    BRUSH_SHAPE_CUSTOM
} BrushShape;

// Falloff curve types
typedef enum {
    BRUSH_FALLOFF_LINEAR,
    BRUSH_FALLOFF_SMOOTH,
    BRUSH_FALLOFF_SPHERICAL,
    BRUSH_FALLOFF_TIP
} BrushFalloff;

// Brush configuration
typedef struct {
    BrushOperation operation;
    BrushShape shape;
    BrushFalloff falloff;
    
    float radius;
    float strength;
    float opacity;
    
    // Operation-specific parameters
    float target_height;  // For flatten
    float noise_scale;    // For noise
    float erosion_strength; // For erosion
    
    // Brush mask (optional custom shape)
    float* mask_data;
    u32 mask_width;
    u32 mask_height;
} BrushConfig;

// Terrain data (simplified - would integrate with actual terrain system)
typedef struct {
    float* height_data;
    u32 width;
    u32 height;
    float unit_size; // World space size per grid unit
} TerrainData;

// Undo/Redo system for terrain editing
typedef struct {
    float* height_backup;
    u32 x_min, x_max;
    u32 y_min, y_max;
    u64 timestamp;
} TerrainUndoState;

typedef struct {
    TerrainUndoState* undo_stack;
    u32 undo_count;
    u32 undo_capacity;
    u32 undo_index;
} TerrainUndoSystem;

// Brush system context
typedef struct {
    BrushConfig config;
    TerrainData* active_terrain;
    TerrainUndoSystem undo_system;
    
    // Stroke state
    bool is_stroking;
    Vec2 last_stroke_pos;
} BrushSystem;

// Core API
void brush_system_init(BrushSystem* system, TerrainData* terrain);
void brush_system_shutdown(BrushSystem* system);

// Brush configuration
void brush_set_operation(BrushSystem* system, BrushOperation op);
void brush_set_radius(BrushSystem* system, float radius);
void brush_set_strength(BrushSystem* system, float strength);
void brush_set_falloff(BrushSystem* system, BrushFalloff falloff);

// Brush operations
void brush_begin_stroke(BrushSystem* system, Vec2 world_pos);
void brush_update_stroke(BrushSystem* system, Vec2 world_pos);
void brush_end_stroke(BrushSystem* system);

// Individual operations
void brush_apply_raise(BrushSystem* system, Vec2 center, float delta_time);
void brush_apply_lower(BrushSystem* system, Vec2 center, float delta_time);
void brush_apply_flatten(BrushSystem* system, Vec2 center, float target_height);
void brush_apply_smooth(BrushSystem* system, Vec2 center, u32 iterations);
void brush_apply_noise(BrushSystem* system, Vec2 center);
void brush_apply_erosion(BrushSystem* system, Vec2 center);

// Undo/Redo
void brush_undo(BrushSystem* system);
void brush_redo(BrushSystem* system);
bool brush_can_undo(BrushSystem* system);
bool brush_can_redo(BrushSystem* system);

// Utilities
float brush_get_falloff(BrushSystem* system, float distance_from_center);
void brush_get_affected_region(BrushSystem* system, Vec2 center, u32* x_min, u32* x_max, u32* y_min, u32* y_max);

#ifdef __cplusplus
}
#endif

#endif // BRUSH_SYSTEM_H
