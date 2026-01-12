#ifndef UNIFIED_SPRITE_RENDERER_H
#define UNIFIED_SPRITE_RENDERER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __OBJC__
#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_encoder.h"
#else
typedef void* mtl_buffer_t;
typedef void* mtl_device_t;
typedef void* mtl_command_encoder_t;
typedef void* metal_buffer_t;
typedef void* metal_device_t;
typedef void* metal_render_command_encoder_t;
#endif

// ============================================================================
// Unified Sprite Renderer - Supports 2D, 2.5D, and 3D sprite rendering
// ============================================================================

// Forward declarations
typedef struct UnifiedSpriteRenderer UnifiedSpriteRenderer;
typedef struct Sprite Sprite;
typedef struct SpriteBatch SpriteBatch;

// Sprite rendering modes
typedef enum {
    SPRITE_MODE_2D = 0,      // Pure 2D rendering (UI, HUD)
    SPRITE_MODE_2_5D,        // 2.5D billboarding (world sprites)
    SPRITE_MODE_3D,          // Full 3D oriented sprites
    SPRITE_MODE_PARTICLE     // Particle-style sprites
} SpriteMode;

// Sprite vertex structure
typedef struct {
    float x, y, z;           // Position
    float u, v;              // Texture coordinates
    float w, h;              // Width/height (for 2.5D/3D)
    uint8_t r, g, b, a;      // Color
    uint16_t texture_id;     // Texture array index
    uint8_t mode;            // Rendering mode
    float rotation;          // Rotation angle
    float scale;             // Scale factor
} SpriteVertex;

// Sprite structure
struct Sprite {
    float position[3];        // World position
    float size[2];            // Width and height
    float uv_coords[4];       // U, V, U2, V2
    uint8_t color[4];         // RGBA color
    uint16_t texture_id;      // Texture ID
    SpriteMode mode;          // Rendering mode
    float rotation;           // Rotation angle
    float scale;              // Scale
    bool billboard;           // Face camera (2.5D/3D)
    bool visible;             // Visibility flag
    uint32_t layer;           // Rendering layer
};

// Sprite batch for efficient rendering
struct SpriteBatch {
    metal_buffer_t* vertex_buffer;
    metal_buffer_t* index_buffer;
    SpriteVertex* vertices;
    uint32_t* indices;
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t sprite_count;
    uint32_t max_sprites;
    bool dirty;
    uint16_t texture_id;     // Texture ID for this batch
};

// Sprite renderer configuration
typedef struct {
    bool enable_instancing;
    bool enable_batching;
    bool enable_depth_testing;
    bool enable_blending;
    uint32_t max_sprites_per_batch;
    uint32_t max_total_sprites;
    float texture_atlas_size;
    bool enable_mipmapping;
    bool enable_anisotropic_filtering;
} SpriteRendererConfig;

// Render statistics
typedef struct {
    uint32_t sprites_rendered;
    uint32_t batches_rendered;
    uint32_t draw_calls;
    uint32_t texture_switches;
    float batch_time_ms;
    float render_time_ms;
    uint32_t memory_usage_mb;
} SpriteRenderStats;

// ============================================================================
// Core Renderer Functions
// ============================================================================

// System lifecycle
UnifiedSpriteRenderer* unified_sprite_renderer_create(metal_device_t* device,
                                                     const SpriteRendererConfig* config);
void unified_sprite_renderer_destroy(UnifiedSpriteRenderer* renderer);

// Configuration
void unified_sprite_renderer_set_config(UnifiedSpriteRenderer* renderer,
                                       const SpriteRendererConfig* config);
void unified_sprite_renderer_get_config(const UnifiedSpriteRenderer* renderer,
                                       SpriteRendererConfig* out_config);

// ============================================================================
// Sprite Management
// ============================================================================

// Sprite lifecycle
uint32_t unified_sprite_renderer_create_sprite(UnifiedSpriteRenderer* renderer,
                                               const Sprite* sprite);
void unified_sprite_renderer_destroy_sprite(UnifiedSpriteRenderer* renderer,
                                            uint32_t sprite_id);
void unified_sprite_renderer_update_sprite(UnifiedSpriteRenderer* renderer,
                                           uint32_t sprite_id,
                                           const Sprite* sprite);

// Sprite operations
void unified_sprite_renderer_set_sprite_position(UnifiedSpriteRenderer* renderer,
                                                 uint32_t sprite_id,
                                                 const float position[3]);
void unified_sprite_renderer_set_sprite_size(UnifiedSpriteRenderer* renderer,
                                            uint32_t sprite_id,
                                            const float size[2]);
void unified_sprite_renderer_set_sprite_color(UnifiedSpriteRenderer* renderer,
                                             uint32_t sprite_id,
                                             const uint8_t color[4]);
void unified_sprite_renderer_set_sprite_texture(UnifiedSpriteRenderer* renderer,
                                               uint32_t sprite_id,
                                               uint16_t texture_id);
void unified_sprite_renderer_set_sprite_visible(UnifiedSpriteRenderer* renderer,
                                               uint32_t sprite_id,
                                               bool visible);

// ============================================================================
// Batching and Rendering
// ============================================================================

// Batch management
void unified_sprite_renderer_begin_frame(UnifiedSpriteRenderer* renderer);
void unified_sprite_renderer_end_frame(UnifiedSpriteRenderer* renderer);

// Main rendering interface
void unified_sprite_renderer_render(UnifiedSpriteRenderer* renderer,
                                   metal_render_command_encoder_t encoder,
                                   const float view_projection_matrix[16],
                                   const float camera_position[3]);

// Mode-specific rendering
void unified_sprite_renderer_render_2d(UnifiedSpriteRenderer* renderer,
                                      metal_render_command_encoder_t encoder);
void unified_sprite_renderer_render_2_5d(UnifiedSpriteRenderer* renderer,
                                        metal_render_command_encoder_t encoder,
                                        const float view_matrix[16],
                                        const float camera_position[3]);
void unified_sprite_renderer_render_3d(UnifiedSpriteRenderer* renderer,
                                      metal_render_command_encoder_t encoder,
                                      const float view_projection_matrix[16]);

// ============================================================================
// Texture Management
// ============================================================================

// Texture atlas
uint16_t unified_sprite_renderer_load_texture(UnifiedSpriteRenderer* renderer,
                                              const char* texture_path);
void unified_sprite_renderer_create_texture_atlas(UnifiedSpriteRenderer* renderer,
                                                 const char** texture_paths,
                                                 uint32_t texture_count);
void unified_sprite_renderer_set_texture_filter(UnifiedSpriteRenderer* renderer,
                                               bool linear);

// ============================================================================
// Utility Functions
// ============================================================================

// Coordinate conversion
void unified_sprite_screen_to_world(const float screen_pos[2],
                                   const float view_projection_matrix[16],
                                   const float viewport[4],
                                   float world_pos[3]);
void unified_sprite_world_to_screen(const float world_pos[3],
                                   const float view_projection_matrix[16],
                                   const float viewport[4],
                                   float screen_pos[2]);

// Sprite utilities
void unified_sprite_calculate_uv_coords(uint16_t texture_id,
                                       uint32_t atlas_width,
                                       uint32_t atlas_height,
                                       uint32_t sprite_width,
                                       uint32_t sprite_height,
                                       float uv_coords[4]);

// ============================================================================
// Statistics and Debugging
// ============================================================================

// Performance monitoring
void unified_sprite_renderer_get_stats(const UnifiedSpriteRenderer* renderer,
                                      SpriteRenderStats* out_stats);
void unified_sprite_renderer_reset_stats(UnifiedSpriteRenderer* renderer);

// Debug utilities
void unified_sprite_renderer_debug_render_bounds(UnifiedSpriteRenderer* renderer);
void unified_sprite_renderer_debug_render_wireframe(UnifiedSpriteRenderer* renderer, bool enable);

// ============================================================================
// Platform-specific Interface
// ============================================================================

#ifdef __APPLE__
// Metal-specific functions
void unified_sprite_renderer_metal_update_buffers(UnifiedSpriteRenderer* renderer,
                                                 id<MTLCommandBuffer> command_buffer);
void unified_sprite_renderer_metal_render(UnifiedSpriteRenderer* renderer,
                                         id<MTLRenderCommandEncoder> encoder);
#endif

#endif // UNIFIED_SPRITE_RENDERER_H
