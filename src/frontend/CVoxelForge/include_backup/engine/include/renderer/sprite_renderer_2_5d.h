// include/render/sprite_renderer_2_5d.h
//
// Purpose: 2.5D sprite rendering system optimized for efficient rendering
// of isometric and orthographic 2.5D games. Supports depth sorting, batching,
// and multiple rendering techniques.
//
// Public APIs:
// - SpriteRenderer2_5D: Main renderer for 2.5D sprites
// - Sprite2_5D: Individual sprite definition
// - Depth sorting and batching for performance
//
// Ownership: SpriteRenderer2_5D owns sprite data and batches
//
// Invariants:
// - Renderer must be initialized before use
// - Sprites must be added before rendering
//
#ifndef SPRITE_RENDERER_2_5D_H
#define SPRITE_RENDERER_2_5D_H

#include <common.h>
#include <math/vec3.h>
#include <math/vec2.h>
#include <math/mat4.h>

// Forward declarations
struct Camera;
struct IRenderer;

// Sprite rendering mode
typedef enum {
    SPRITE_2_5D_MODE_ISOMETRIC,  // Isometric projection (diagonal)
    SPRITE_2_5D_MODE_ORTHO,       // Orthographic projection (side-view)
    SPRITE_2_5D_MODE_PERSPECTIVE  // Perspective with fixed angle
} Sprite2_5DMode;

// Sprite definition
typedef struct {
    Vec3 position;        // World position
    Vec2 size;           // Sprite size in world units
    u32 texture_id;      // Texture ID
    f32 rotation;        // Rotation in radians
    u32 layer;           // Depth layer (for sorting)
    f32 depth;           // Depth value for sorting (Y position or custom)
    Vec3 color_tint;      // Color tint (1,1,1 = no tint)
    f32 alpha;           // Alpha transparency
    bool billboard;      // Face camera always
    bool flip_x;         // Flip horizontally
    bool flip_y;         // Flip vertically
} Sprite2_5D;

// Sprite batch (for efficient rendering)
typedef struct {
    Sprite2_5D *sprites;
    u32 sprite_count;
    u32 sprite_capacity;
    u32 texture_id;      // All sprites in batch use same texture
    bool sorted;         // Whether sprites are depth-sorted
} SpriteBatch2_5D;

// 2.5D Sprite Renderer
typedef struct {
    struct IRenderer *renderer;
    struct Camera *camera;
    
    Sprite2_5DMode mode;
    
    // Sprite storage
    Sprite2_5D *sprites;
    u32 sprite_count;
    u32 sprite_capacity;
    
    // Batching
    SpriteBatch2_5D *batches;
    u32 batch_count;
    u32 batch_capacity;
    
    // Depth sorting
    bool enable_depth_sort;
    u32 *sort_indices;   // Indices for sorted rendering
    
    // Rendering state
    bool initialized;
    Mat4 projection_matrix;
    Mat4 view_matrix;
} SpriteRenderer2_5D;

// Lifecycle
bool sprite_renderer_2_5d_init(SpriteRenderer2_5D *renderer, 
                                struct IRenderer *backend_renderer,
                                struct Camera *camera,
                                Sprite2_5DMode mode);
void sprite_renderer_2_5d_shutdown(SpriteRenderer2_5D *renderer);

// Sprite management
u32 sprite_renderer_2_5d_add_sprite(SpriteRenderer2_5D *renderer, 
                                     const Sprite2_5D *sprite);
void sprite_renderer_2_5d_remove_sprite(SpriteRenderer2_5D *renderer, u32 sprite_id);
void sprite_renderer_2_5d_update_sprite(SpriteRenderer2_5D *renderer, 
                                        u32 sprite_id, 
                                        const Sprite2_5D *sprite);
Sprite2_5D *sprite_renderer_2_5d_get_sprite(SpriteRenderer2_5D *renderer, u32 sprite_id);

// Batch management
void sprite_renderer_2_5d_clear(SpriteRenderer2_5D *renderer);
void sprite_renderer_2_5d_batch_sprites(SpriteRenderer2_5D *renderer);

// Rendering
void sprite_renderer_2_5d_render(SpriteRenderer2_5D *renderer);
void sprite_renderer_2_5d_render_batch(SpriteRenderer2_5D *renderer, u32 batch_id);

// Depth sorting
void sprite_renderer_2_5d_sort_by_depth(SpriteRenderer2_5D *renderer);
void sprite_renderer_2_5d_set_depth_sort_enabled(SpriteRenderer2_5D *renderer, bool enabled);

// Camera and projection
void sprite_renderer_2_5d_set_camera(SpriteRenderer2_5D *renderer, struct Camera *camera);
void sprite_renderer_2_5d_set_mode(SpriteRenderer2_5D *renderer, Sprite2_5DMode mode);
void sprite_renderer_2_5d_update_projection(SpriteRenderer2_5D *renderer, 
                                            u32 window_width, 
                                            u32 window_height);

// Isometric helpers
void sprite_renderer_2_5d_setup_isometric(SpriteRenderer2_5D *renderer, 
                                          f32 tile_size, 
                                          f32 height);
void sprite_renderer_2_5d_world_to_isometric(Vec3 world_pos, Vec2 *iso_pos);
void sprite_renderer_2_5d_isometric_to_world(Vec2 iso_pos, Vec3 *world_pos);

#endif // SPRITE_RENDERER_2_5D_H

