// Sprite 3D Renderer Implementation
// 2.5D sprite-based rendering system for isometric and orthographic views

#ifndef SPRITE_3D_RENDERER_H
#define SPRITE_3D_RENDERER_H

#include "core/types.h"
#include "rendering/core/renderer.h"
#include "rendering/core/texture.h"
#include "rendering/core/buffer.h"
#include "engine/include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sprite rendering modes
typedef enum {
    SPRITE_MODE_ISOMETRIC,     // Isometric 2.5D view (diagonal top-down)
    SPRITE_MODE_ORTHOGRAPHIC,  // Orthographic 2.5D view (side-scroller with depth)
    SPRITE_MODE_TOP_DOWN,      // Pure top-down 2D
    SPRITE_MODE_SIDE_SCROLL,   // Pure side-scrolling 2D
    SPRITE_MODE_COUNT
} SpriteRenderMode;

// Sprite types
typedef enum {
    SPRITE_TYPE_STATIC,        // Static sprite
    SPRITE_TYPE_ANIMATED,      // Animated sprite
    SPRITE_TYPE_BILLBOARD,      // Always faces camera
    SPRITE_TYPE_ORIENTED,       // Fixed orientation
    SPRITE_TYPE_COUNT
} SpriteType;

// Sprite blend modes
typedef enum {
    SPRITE_BLEND_ALPHA,         // Alpha blending
    SPRITE_BLEND_ADDITIVE,      // Additive blending
    SPRITE_BLEND_MULTIPLY,      // Multiply blending
    SPRITE_BLEND_SCREEN,        // Screen blending
    SPRITE_BLEND_COUNT
} SpriteBlendMode;

// Sprite vertex structure
typedef struct {
    Vec3 position;      // World position
    Vec2 uv;            // Texture coordinates
    Vec2 size;          // Width/height in world units
    Vec4 color;         // Tint color
    f32 rotation;       // Rotation in radians
    u16 texture_id;     // Texture atlas index
    u16 frame;          // Animation frame
    u8 blend_mode;      // Blend mode
    u8 flags;           // Rendering flags
} SpriteVertex;

// Sprite animation
typedef struct {
    u16 frame_count;
    u16 current_frame;
    f32 frame_duration;
    f32 current_time;
    bool loop;
    bool playing;
} SpriteAnimation;

// Sprite instance
typedef struct {
    Vec3 position;
    Vec2 size;
    Vec4 color;
    f32 rotation;
    f32 depth;          // Depth for sorting
    u16 texture_id;
    u16 layer;          // Rendering layer
    SpriteType type;
    SpriteBlendMode blend_mode;
    bool visible;
    bool affected_by_light;
    SpriteAnimation animation;
} Sprite3D;

// Sprite batch for efficient rendering
typedef struct {
    SpriteVertex *vertices;
    u32 vertex_count;
    u32 vertex_capacity;
    u32 *indices;
    u32 index_count;
    u32 index_capacity;
    u32 texture_id;
    SpriteBlendMode blend_mode;
} SpriteBatch;

// Sprite 3D renderer context
typedef struct Sprite3DRenderer {
    // Rendering mode
    SpriteRenderMode render_mode;
    
    // Camera and projection
    Vec3 camera_position;
    Vec3 camera_target;
    Vec3 camera_up;
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Mat4 view_projection_matrix;
    
    // Rendering resources
    Texture *texture_atlas;
    Buffer vertex_buffer;
    Buffer index_buffer;
    Buffer uniform_buffer;
    
    // Sprite management
    Sprite3D *sprites;
    u32 sprite_count;
    u32 max_sprites;
    
    // Batching
    SpriteBatch *batches;
    u32 batch_count;
    u32 max_batches;
    
    // Lighting
    Vec3 ambient_light;
    Vec3 directional_light;
    f32 directional_intensity;
    
    // Performance
    u32 sprites_rendered;
    u32 batches_rendered;
    u32 draw_calls;
    
    // Viewport
    u32 viewport_width;
    u32 viewport_height;
    
    bool initialized;
} Sprite3DRenderer;

// Create sprite 3D renderer
Sprite3DRenderer *sprite_3d_renderer_create(SpriteRenderMode mode);

// Destroy sprite 3D renderer
void sprite_3d_renderer_destroy(Sprite3DRenderer *renderer);

// Update renderer state
void sprite_3d_renderer_update(Sprite3DRenderer *renderer, float delta_time);

// Render frame
void sprite_3d_renderer_render(Sprite3DRenderer *renderer, const Mat4 *view, const Mat4 *projection);

// Set rendering mode
void sprite_3d_renderer_set_mode(Sprite3DRenderer *renderer, SpriteRenderMode mode);

// Camera control
void sprite_3d_renderer_set_camera(Sprite3DRenderer *renderer, const Vec3 *position, const Vec3 *target, const Vec3 *up);
void sprite_3d_renderer_set_orthographic(Sprite3DRenderer *renderer, f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
void sprite_3d_renderer_set_perspective(Sprite3DRenderer *renderer, f32 fov, f32 aspect, f32 near, f32 far);

// Sprite management
u32 sprite_3d_create_sprite(Sprite3DRenderer *renderer);
void sprite_3d_destroy_sprite(Sprite3DRenderer *renderer, u32 sprite_id);
Sprite3D *sprite_3d_get_sprite(Sprite3DRenderer *renderer, u32 sprite_id);

// Sprite properties
void sprite_3d_set_position(Sprite3DRenderer *renderer, u32 sprite_id, const Vec3 *position);
void sprite_3d_set_size(Sprite3DRenderer *renderer, u32 sprite_id, const Vec2 *size);
void sprite_3d_set_color(Sprite3DRenderer *renderer, u32 sprite_id, const Vec4 *color);
void sprite_3d_set_rotation(Sprite3DRenderer *renderer, u32 sprite_id, f32 rotation);
void sprite_3d_set_texture(Sprite3DRenderer *renderer, u32 sprite_id, u16 texture_id);
void sprite_3d_set_visible(Sprite3DRenderer *renderer, u32 sprite_id, bool visible);
void sprite_3d_set_layer(Sprite3DRenderer *renderer, u32 sprite_id, u16 layer);

// Animation control
void sprite_3d_set_animation(Sprite3DRenderer *renderer, u32 sprite_id, u16 frame_count, f32 frame_duration, bool loop);
void sprite_3d_play_animation(Sprite3DRenderer *renderer, u32 sprite_id, bool play);
void sprite_3d_set_animation_frame(Sprite3DRenderer *renderer, u32 sprite_id, u16 frame);

// Lighting
void sprite_3d_set_ambient_light(Sprite3DRenderer *renderer, const Vec3 *color);
void sprite_3d_set_directional_light(Sprite3DRenderer *renderer, const Vec3 *direction, const Vec3 *color, f32 intensity);

// Texture atlas management
u16 sprite_3d_add_texture(Sprite3DRenderer *renderer, const char *texture_path);
bool sprite_3d_load_texture_atlas(Sprite3DRenderer *renderer, const char *atlas_path);

// Viewport
void sprite_3d_renderer_set_viewport(Sprite3DRenderer *renderer, u32 width, u32 height);

// Statistics
void sprite_3d_renderer_get_stats(Sprite3DRenderer *renderer, u32 *sprites_rendered, u32 *batches_rendered, u32 *draw_calls);

// Depth sorting for correct rendering order
void sprite_3d_sort_sprites_by_depth(Sprite3DRenderer *renderer);

// Batch sprites by texture and blend mode
void sprite_3d_batch_sprites(Sprite3DRenderer *renderer);

#ifdef __cplusplus
}
#endif

#endif // SPRITE_3D_RENDERER_H
