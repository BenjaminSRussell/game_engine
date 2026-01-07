// src/render/sprite_renderer_2_5d.c
//
// 2.5D sprite renderer implementation (stub)
//
#include <core/logger.h>
#include <math/mat4.h>
#include <renderer/camera.h>
#include <renderer/renderer.h>
#include <renderer/sprite_renderer_2_5d.h>
#include <stdlib.h>
#include <string.h>

// Lifecycle
bool sprite_renderer_2_5d_init(SpriteRenderer2_5D *renderer,
                               struct IRenderer *backend_renderer,
                               struct Camera *camera, Sprite2_5DMode mode) {
  if (!renderer || !backend_renderer || !camera) {
    LOG_ERROR("Invalid parameters for sprite_renderer_2_5d_init");
    return false;
  }

  memset(renderer, 0, sizeof(SpriteRenderer2_5D));
  renderer->renderer = backend_renderer;
  renderer->camera = camera;
  renderer->mode = mode;
  renderer->enable_depth_sort = true;

  renderer->sprite_capacity = 1024;
  renderer->sprites =
      (Sprite2_5D *)calloc(renderer->sprite_capacity, sizeof(Sprite2_5D));

  renderer->batch_capacity = 32;
  renderer->batches = (SpriteBatch2_5D *)calloc(renderer->batch_capacity,
                                                sizeof(SpriteBatch2_5D));

  renderer->sort_indices =
      (u32 *)calloc(renderer->sprite_capacity, sizeof(u32));

  renderer->projection_matrix = mat4_identity();
  renderer->view_matrix = mat4_identity();

  renderer->initialized = true;
  LOG_INFO("2.5D sprite renderer initialized");
  return true;
}

void sprite_renderer_2_5d_shutdown(SpriteRenderer2_5D *renderer) {
  if (!renderer || !renderer->initialized) {
    return;
  }

  if (renderer->sprites) {
    free(renderer->sprites);
  }

  if (renderer->batches) {
    for (u32 i = 0; i < renderer->batch_count; i++) {
      if (renderer->batches[i].sprites) {
        free(renderer->batches[i].sprites);
      }
    }
    free(renderer->batches);
  }

  if (renderer->sort_indices) {
    free(renderer->sort_indices);
  }

  memset(renderer, 0, sizeof(SpriteRenderer2_5D));
  LOG_INFO("2.5D sprite renderer shut down");
}

// Sprite management
u32 sprite_renderer_2_5d_add_sprite(SpriteRenderer2_5D *renderer,
                                    const Sprite2_5D *sprite) {
  if (!renderer || !renderer->initialized || !sprite) {
    return 0;
  }

  if (renderer->sprite_count >= renderer->sprite_capacity) {
    u32 new_capacity = renderer->sprite_capacity * 2;
    Sprite2_5D *new_sprites = (Sprite2_5D *)realloc(
        renderer->sprites, new_capacity * sizeof(Sprite2_5D));
    if (new_sprites) {
      renderer->sprites = new_sprites;
      renderer->sprite_capacity = new_capacity;
    } else {
      return 0;
    }
  }

  u32 id = renderer->sprite_count;
  renderer->sprites[id] = *sprite;
  renderer->sprite_count++;

  return id;
}

void sprite_renderer_2_5d_remove_sprite(SpriteRenderer2_5D *renderer,
                                        u32 sprite_id) {
  if (!renderer || !renderer->initialized ||
      sprite_id >= renderer->sprite_count) {
    return;
  }

  // Move last sprite to this position
  if (sprite_id < renderer->sprite_count - 1) {
    renderer->sprites[sprite_id] =
        renderer->sprites[renderer->sprite_count - 1];
  }

  renderer->sprite_count--;
}

void sprite_renderer_2_5d_update_sprite(SpriteRenderer2_5D *renderer,
                                        u32 sprite_id,
                                        const Sprite2_5D *sprite) {
  if (renderer && renderer->initialized && sprite_id < renderer->sprite_count &&
      sprite) {
    renderer->sprites[sprite_id] = *sprite;
  }
}

Sprite2_5D *sprite_renderer_2_5d_get_sprite(SpriteRenderer2_5D *renderer,
                                            u32 sprite_id) {
  if (renderer && renderer->initialized && sprite_id < renderer->sprite_count) {
    return &renderer->sprites[sprite_id];
  }
  return NULL;
}

// Batch management
void sprite_renderer_2_5d_clear(SpriteRenderer2_5D *renderer) {
  if (renderer) {
    renderer->sprite_count = 0;
    renderer->batch_count = 0;
  }
}

void sprite_renderer_2_5d_batch_sprites(SpriteRenderer2_5D *renderer) {
  if (!renderer || !renderer->initialized) {
    return;
  }

  // Simple batching by texture_id
  // Full implementation would group sprites by texture
  renderer->batch_count = 0;
}

// Rendering
void sprite_renderer_2_5d_render(SpriteRenderer2_5D *renderer) {
  if (!renderer || !renderer->initialized || !renderer->renderer) {
    return;
  }

  // Sort by depth if enabled
  if (renderer->enable_depth_sort) {
    sprite_renderer_2_5d_sort_by_depth(renderer);
  }

  // Render sprites (would use backend renderer)
  for (u32 i = 0; i < renderer->sprite_count; i++) {
    u32 idx = renderer->enable_depth_sort ? renderer->sort_indices[i] : i;
    Sprite2_5D *sprite = &renderer->sprites[idx];

    // Render sprite using backend renderer
    if (renderer->renderer->render_sprite) {
      renderer->renderer->render_sprite(renderer->renderer, sprite->position,
                                        sprite->size, sprite->texture_id,
                                        sprite->rotation);
    }
  }
}

void sprite_renderer_2_5d_render_batch(SpriteRenderer2_5D *renderer,
                                       u32 batch_id) {
  if (!renderer || !renderer->initialized ||
      batch_id >= renderer->batch_count) {
    return;
  }

  SpriteBatch2_5D *batch = &renderer->batches[batch_id];
  // Render batch (would use instancing)
}

// Depth sorting
static int compare_sprite_depth(const void *a, const void *b) {
  const u32 *idx_a = (const u32 *)a;
  const u32 *idx_b = (const u32 *)b;
  // This would compare actual depth values
  return 0;
}

void sprite_renderer_2_5d_sort_by_depth(SpriteRenderer2_5D *renderer) {
  if (!renderer || !renderer->initialized) {
    return;
  }

  // Initialize indices
  for (u32 i = 0; i < renderer->sprite_count; i++) {
    renderer->sort_indices[i] = i;
  }

  // Sort indices by sprite depth
  qsort(renderer->sort_indices, renderer->sprite_count, sizeof(u32),
        compare_sprite_depth);
}

void sprite_renderer_2_5d_set_depth_sort_enabled(SpriteRenderer2_5D *renderer,
                                                 bool enabled) {
  if (renderer) {
    renderer->enable_depth_sort = enabled;
  }
}

// Camera and projection
void sprite_renderer_2_5d_set_camera(SpriteRenderer2_5D *renderer,
                                     struct Camera *camera) {
  if (renderer) {
    renderer->camera = camera;
  }
}

void sprite_renderer_2_5d_set_mode(SpriteRenderer2_5D *renderer,
                                   Sprite2_5DMode mode) {
  if (renderer) {
    renderer->mode = mode;
  }
}

void sprite_renderer_2_5d_update_projection(SpriteRenderer2_5D *renderer,
                                            u32 window_width,
                                            u32 window_height) {
  if (!renderer || !renderer->camera) {
    return;
  }

  f32 aspect = (f32)window_width / (f32)window_height;

  if (renderer->mode == SPRITE_2_5D_MODE_ISOMETRIC) {
    // Isometric projection
    f32 size = 10.0f;
    renderer->projection_matrix =
        mat4_ortho(-size * aspect, size * aspect, -size, size, -100.0f, 100.0f);
  } else if (renderer->mode == SPRITE_2_5D_MODE_ORTHO) {
    // Orthographic projection
    f32 size = 10.0f;
    renderer->projection_matrix =
        mat4_ortho(-size * aspect, size * aspect, -size, size, -100.0f, 100.0f);
  } else {
    // Perspective
    renderer->projection_matrix =
        camera_get_projection_matrix(renderer->camera, aspect);
  }

  renderer->view_matrix = camera_get_view_matrix(renderer->camera);
}

// Isometric helpers
void sprite_renderer_2_5d_setup_isometric(SpriteRenderer2_5D *renderer,
                                          f32 tile_size, f32 height) {
  if (!renderer)
    return;

  renderer->mode = SPRITE_2_5D_MODE_ISOMETRIC;
  // Setup isometric parameters
}

void sprite_renderer_2_5d_world_to_isometric(Vec3 world_pos, Vec2 *iso_pos) {
  if (!iso_pos)
    return;

  // Isometric projection: x' = (x - z) * cos(30°), y' = (x + z) * sin(30°) - y
  f32 cos30 = 0.8660254f;
  f32 sin30 = 0.5f;

  iso_pos->x = (world_pos.x - world_pos.z) * cos30;
  iso_pos->y = (world_pos.x + world_pos.z) * sin30 - world_pos.y;
}

void sprite_renderer_2_5d_isometric_to_world(Vec2 iso_pos, Vec3 *world_pos) {
  if (!world_pos)
    return;

  // Inverse isometric projection
  f32 cos30 = 0.8660254f;
  f32 sin30 = 0.5f;

  world_pos->x = (iso_pos.x / cos30 + iso_pos.y / sin30) * 0.5f;
  world_pos->z = (iso_pos.y / sin30 - iso_pos.x / cos30) * 0.5f;
  world_pos->y = 0.0f; // Would need to determine from height map
}
