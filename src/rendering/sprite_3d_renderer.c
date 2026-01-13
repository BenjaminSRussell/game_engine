// Sprite 3D Renderer Implementation
// 2.5D sprite-based rendering system for isometric and orthographic views

#include "sprite_3d_renderer.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include "rendering/core/renderer.h"
#include "rendering/core/texture.h"
#include "rendering/core/buffer.h"
#include "engine/include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Constants
#define MAX_SPRITES 10000
#define MAX_BATCHES 64
#define VERTICES_PER_SPRITE 4
#define INDICES_PER_SPRITE 6
#define INITIAL_BATCH_CAPACITY 1000

// Sprite vertex structure for GPU
typedef struct {
    Vec3 position;
    Vec2 uv;
    Vec4 color;
    u16 texture_id;
    u16 frame;
    u8 blend_mode;
    u8 flags;
} GPUSpriteVertex;

// Helper functions
static inline f32 get_sprite_depth(const Sprite3D *sprite, SpriteRenderMode mode) {
    switch (mode) {
        case SPRITE_MODE_ISOMETRIC:
            return sprite->position.x + sprite->position.y + sprite->position.z * 0.5f;
        case SPRITE_MODE_ORTHOGRAPHIC:
            return sprite->position.y;
        case SPRITE_MODE_TOP_DOWN:
            return sprite->position.z;
        case SPRITE_MODE_SIDE_SCROLL:
            return sprite->position.x;
        default:
            return sprite->depth;
    }
}

static inline Vec3 world_to_screen(const Vec3 *world_pos, const Mat4 *view_projection) {
    Vec4 clip = mat4_multiply_vec4(view_projection, vec4_create(world_pos->x, world_pos->y, world_pos->z, 1.0f));
    return vec3_create(clip.x / clip.w, clip.y / clip.w, clip.z / clip.w);
}

static inline void create_sprite_vertices(const Sprite3D *sprite, GPUSpriteVertex *vertices, const Mat4 *view_projection) {
    Vec3 pos = sprite->position;
    Vec2 size = sprite->size;
    f32 rotation = sprite->rotation;
    Vec4 color = sprite->color;
    u16 texture_id = sprite->texture_id;
    u16 frame = sprite.animation.current_frame;
    u8 blend_mode = (u8)sprite->blend_mode;
    u8 flags = sprite->type == SPRITE_TYPE_BILLBOARD ? 1 : 0;
    
    // Calculate sprite corners in local space
    Vec2 corners[4] = {
        {-size.x * 0.5f, -size.y * 0.5f},
        { size.x * 0.5f, -size.y * 0.5f},
        { size.x * 0.5f,  size.y * 0.5f},
        {-size.x * 0.5f,  size.y * 0.5f}
    };
    
    // Apply rotation if needed
    if (rotation != 0.0f) {
        f32 cos_r = cosf(rotation);
        f32 sin_r = sinf(rotation);
        
        for (int i = 0; i < 4; i++) {
            f32 x = corners[i].x * cos_r - corners[i].y * sin_r;
            f32 y = corners[i].x * sin_r + corners[i].y * cos_r;
            corners[i].x = x;
            corners[i].y = y;
        }
    }
    
    // Transform to world space and create vertices
    for (int i = 0; i < 4; i++) {
        vertices[i].position = vec3_create(
            pos.x + corners[i].x,
            pos.y + corners[i].y,
            pos.z
        );
        
        // UV coordinates (simple quad)
        vertices[i].uv = (Vec2){(i & 1) ? 1.0f : 0.0f, (i & 2) ? 1.0f : 0.0f};
        vertices[i].color = color;
        vertices[i].texture_id = texture_id;
        vertices[i].frame = frame;
        vertices[i].blend_mode = blend_mode;
        vertices[i].flags = flags;
    }
}

// Create sprite 3D renderer
Sprite3DRenderer *sprite_3d_renderer_create(SpriteRenderMode mode) {
    Sprite3DRenderer *renderer = malloc(sizeof(Sprite3DRenderer));
    if (!renderer) {
        LOG_ERROR("Failed to allocate sprite 3D renderer");
        return NULL;
    }
    
    memset(renderer, 0, sizeof(Sprite3DRenderer));
    
    renderer->render_mode = mode;
    renderer->max_sprites = MAX_SPRITES;
    renderer->max_batches = MAX_BATCHES;
    
    // Allocate sprite array
    renderer->sprites = malloc(sizeof(Sprite3D) * renderer->max_sprites);
    if (!renderer->sprites) {
        LOG_ERROR("Failed to allocate sprite array");
        free(renderer);
        return NULL;
    }
    
    // Allocate batch array
    renderer->batches = malloc(sizeof(SpriteBatch) * renderer->max_batches);
    if (!renderer->batches) {
        LOG_ERROR("Failed to allocate batch array");
        free(renderer->sprites);
        free(renderer);
        return NULL;
    }
    
    // Initialize sprites
    memset(renderer->sprites, 0, sizeof(Sprite3D) * renderer->max_sprites);
    memset(renderer->batches, 0, sizeof(SpriteBatch) * renderer->max_batches);
    
    // Create vertex and index buffers
    BufferDesc vertex_desc = {
        .size = sizeof(GPUSpriteVertex) * VERTICES_PER_SPRITE * renderer->max_sprites,
        .usage = BUFFER_USAGE_VERTEX
    };
    
    BufferDesc index_desc = {
        .size = sizeof(u32) * INDICES_PER_SPRITE * renderer->max_sprites,
        .usage = BUFFER_USAGE_INDEX
    };
    
    renderer->vertex_buffer = buffer_create(&vertex_desc);
    renderer->index_buffer = buffer_create(&index_desc);
    
    if (!buffer_is_valid(renderer->vertex_buffer) || !buffer_is_valid(renderer->index_buffer)) {
        LOG_ERROR("Failed to create sprite buffers");
        free(renderer->sprites);
        free(renderer->batches);
        if (buffer_is_valid(renderer->vertex_buffer)) buffer_destroy(renderer->vertex_buffer);
        if (buffer_is_valid(renderer->index_buffer)) buffer_destroy(renderer->index_buffer);
        free(renderer);
        return NULL;
    }
    
    // Create uniform buffer
    BufferDesc uniform_desc = {
        .size = sizeof(Mat4) * 3 + sizeof(Vec3) * 2 + sizeof(f32),
        .usage = BUFFER_USAGE_UNIFORM
    };
    
    renderer->uniform_buffer = buffer_create(&uniform_desc);
    if (!buffer_is_valid(renderer->uniform_buffer)) {
        LOG_ERROR("Failed to create uniform buffer");
        free(renderer->sprites);
        free(renderer->batches);
        buffer_destroy(renderer->vertex_buffer);
        buffer_destroy(renderer->index_buffer);
        free(renderer);
        return NULL;
    }
    
    // Initialize camera
    renderer->camera_position = vec3_create(0.0f, 0.0f, 10.0f);
    renderer->camera_target = vec3_create(0.0f, 0.0f, 0.0f);
    renderer->camera_up = vec3_create(0.0f, 1.0f, 0.0f);
    
    // Initialize matrices
    renderer->view_matrix = mat4_identity();
    renderer->projection_matrix = mat4_identity();
    renderer->view_projection_matrix = mat4_identity();
    
    // Initialize lighting
    renderer->ambient_light = vec3_create(0.3f, 0.3f, 0.3f);
    renderer->directional_light = vec3_create(0.7f, 0.7f, 0.7f);
    renderer->directional_intensity = 1.0f;
    
    // Set default viewport
    renderer->viewport_width = 1920;
    renderer->viewport_height = 1080;
    
    // Initialize batches
    for (u32 i = 0; i < renderer->max_batches; i++) {
        SpriteBatch *batch = &renderer->batches[i];
        batch->vertex_capacity = INITIAL_BATCH_CAPACITY;
        batch->index_capacity = INITIAL_BATCH_CAPACITY * 6 / 4;
        
        batch->vertices = malloc(sizeof(GPUSpriteVertex) * batch->vertex_capacity);
        batch->indices = malloc(sizeof(u32) * batch->index_capacity);
        
        if (!batch->vertices || !batch->indices) {
            LOG_ERROR("Failed to allocate batch memory");
            sprite_3d_renderer_destroy(renderer);
            return NULL;
        }
    }
    
    renderer->initialized = true;
    LOG_INFO("Sprite 3D renderer created successfully (mode: %d)", mode);
    
    return renderer;
}

// Destroy sprite 3D renderer
void sprite_3d_renderer_destroy(Sprite3DRenderer *renderer) {
    if (!renderer) return;
    
    // Destroy batches
    for (u32 i = 0; i < renderer->max_batches; i++) {
        SpriteBatch *batch = &renderer->batches[i];
        free(batch->vertices);
        free(batch->indices);
    }
    
    // Destroy resources
    if (buffer_is_valid(renderer->vertex_buffer)) {
        buffer_destroy(renderer->vertex_buffer);
    }
    
    if (buffer_is_valid(renderer->index_buffer)) {
        buffer_destroy(renderer->index_buffer);
    }
    
    if (buffer_is_valid(renderer->uniform_buffer)) {
        buffer_destroy(renderer->uniform_buffer);
    }
    
    if (renderer->texture_atlas && texture_is_valid(*renderer->texture_atlas)) {
        texture_destroy(*renderer->texture_atlas);
        free(renderer->texture_atlas);
    }
    
    free(renderer->sprites);
    free(renderer->batches);
    free(renderer);
    
    LOG_INFO("Sprite 3D renderer destroyed");
}

// Set rendering mode
void sprite_3d_renderer_set_mode(Sprite3DRenderer *renderer, SpriteRenderMode mode) {
    if (!renderer) return;
    
    renderer->render_mode = mode;
    
    // Update projection matrix based on mode
    switch (mode) {
        case SPRITE_MODE_ISOMETRIC:
            // Isometric projection
            renderer->projection_matrix = mat4_orthographic(-10.0f, 10.0f, -10.0f, 10.0f, -100.0f, 100.0f);
            break;
        case SPRITE_MODE_ORTHOGRAPHIC:
            // Orthographic projection
            renderer->projection_matrix = mat4_orthographic(
                -renderer->viewport_width * 0.5f, renderer->viewport_width * 0.5f,
                -renderer->viewport_height * 0.5f, renderer->viewport_height * 0.5f,
                -100.0f, 100.0f
            );
            break;
        case SPRITE_MODE_TOP_DOWN:
            // Top-down orthographic
            renderer->projection_matrix = mat4_orthographic(-20.0f, 20.0f, -20.0f, 20.0f, -100.0f, 100.0f);
            break;
        case SPRITE_MODE_SIDE_SCROLL:
            // Side-scrolling orthographic
            renderer->projection_matrix = mat4_orthographic(-20.0f, 20.0f, -15.0f, 15.0f, -100.0f, 100.0f);
            break;
    }
    
    LOG_DEBUG("Sprite renderer mode set to: %d", mode);
}

// Set camera
void sprite_3d_renderer_set_camera(Sprite3DRenderer *renderer, const Vec3 *position, const Vec3 *target, const Vec3 *up) {
    if (!renderer) return;
    
    renderer->camera_position = *position;
    renderer->camera_target = *target;
    renderer->camera_up = *up;
    
    renderer->view_matrix = mat4_look_at(position, target, up);
    renderer->view_projection_matrix = mat4_multiply(&renderer->projection_matrix, &renderer->view_matrix);
}

// Create sprite
u32 sprite_3d_create_sprite(Sprite3DRenderer *renderer) {
    if (!renderer || renderer->sprite_count >= renderer->max_sprites) {
        LOG_ERROR("Cannot create sprite: renderer null or at capacity");
        return UINT32_MAX;
    }
    
    u32 sprite_id = renderer->sprite_count++;
    Sprite3D *sprite = &renderer->sprites[sprite_id];
    
    // Initialize with default values
    sprite->position = vec3_create(0.0f, 0.0f, 0.0f);
    sprite->size = vec2_create(1.0f, 1.0f);
    sprite->color = vec4_create(1.0f, 1.0f, 1.0f, 1.0f);
    sprite->rotation = 0.0f;
    sprite->depth = 0.0f;
    sprite->texture_id = 0;
    sprite->layer = 0;
    sprite->type = SPRITE_TYPE_STATIC;
    sprite->blend_mode = SPRITE_BLEND_ALPHA;
    sprite->visible = true;
    sprite->affected_by_light = true;
    
    // Initialize animation
    sprite->animation.frame_count = 1;
    sprite->animation.current_frame = 0;
    sprite->animation.frame_duration = 0.1f;
    sprite->animation.current_time = 0.0f;
    sprite->animation.loop = true;
    sprite->animation.playing = false;
    
    return sprite_id;
}

// Set sprite position
void sprite_3d_set_position(Sprite3DRenderer *renderer, u32 sprite_id, const Vec3 *position) {
    if (!renderer || sprite_id >= renderer->sprite_count) return;
    
    renderer->sprites[sprite_id].position = *position;
}

// Set sprite size
void sprite_3d_set_size(Sprite3DRenderer *renderer, u32 sprite_id, const Vec2 *size) {
    if (!renderer || sprite_id >= renderer->sprite_count) return;
    
    renderer->sprites[sprite_id].size = *size;
}

// Set sprite color
void sprite_3d_set_color(Sprite3DRenderer *renderer, u32 sprite_id, const Vec4 *color) {
    if (!renderer || sprite_id >= renderer->sprite_count) return;
    
    renderer->sprites[sprite_id].color = *color;
}

// Set sprite rotation
void sprite_3d_set_rotation(Sprite3DRenderer *renderer, u32 sprite_id, f32 rotation) {
    if (!renderer || sprite_id >= renderer->sprite_count) return;
    
    renderer->sprites[sprite_id].rotation = rotation;
}

// Set sprite texture
void sprite_3d_set_texture(Sprite3DRenderer *renderer, u32 sprite_id, u16 texture_id) {
    if (!renderer || sprite_id >= renderer->sprite_count) return;
    
    renderer->sprites[sprite_id].texture_id = texture_id;
}

// Set sprite visible
void sprite_3d_set_visible(Sprite3DRenderer *renderer, u32 sprite_id, bool visible) {
    if (!renderer || sprite_id >= renderer->sprite_count) return;
    
    renderer->sprites[sprite_id].visible = visible;
}

// Set animation
void sprite_3d_set_animation(Sprite3DRenderer *renderer, u32 sprite_id, u16 frame_count, f32 frame_duration, bool loop) {
    if (!renderer || sprite_id >= renderer->sprite_count) return;
    
    SpriteAnimation *anim = &renderer->sprites[sprite_id].animation;
    anim->frame_count = frame_count;
    anim->frame_duration = frame_duration;
    anim->loop = loop;
    anim->current_frame = 0;
    anim->current_time = 0.0f;
    anim->playing = frame_count > 1;
}

// Update renderer
void sprite_3d_renderer_update(Sprite3DRenderer *renderer, float delta_time) {
    if (!renderer || !renderer->initialized) return;
    
    // Update animations
    for (u32 i = 0; i < renderer->sprite_count; i++) {
        Sprite3D *sprite = &renderer->sprites[i];
        
        if (sprite->animation.playing) {
            sprite->animation.current_time += delta_time;
            
            while (sprite->animation.current_time >= sprite->animation.frame_duration) {
                sprite->animation.current_time -= sprite->animation.frame_duration;
                sprite->animation.current_frame++;
                
                if (sprite->animation.current_frame >= sprite->animation.frame_count) {
                    if (sprite->animation.loop) {
                        sprite->animation.current_frame = 0;
                    } else {
                        sprite->animation.current_frame = sprite->animation.frame_count - 1;
                        sprite->animation.playing = false;
                    }
                }
            }
        }
    }
    
    // Update view-projection matrix
    renderer->view_projection_matrix = mat4_multiply(&renderer->projection_matrix, &renderer->view_matrix);
    
    // Update uniform buffer
    struct {
        Mat4 view, projection, view_projection;
        Vec3 ambient_light, directional_light;
        f32 directional_intensity;
    } uniforms;
    
    uniforms.view = renderer->view_matrix;
    uniforms.projection = renderer->projection_matrix;
    uniforms.view_projection = renderer->view_projection_matrix;
    uniforms.ambient_light = renderer->ambient_light;
    uniforms.directional_light = renderer->directional_light;
    uniforms.directional_intensity = renderer->directional_intensity;
    
    buffer_upload(renderer->uniform_buffer, &uniforms, sizeof(uniforms));
}

// Sort sprites by depth
void sprite_3d_sort_sprites_by_depth(Sprite3DRenderer *renderer) {
    if (!renderer) return;
    
    // Simple bubble sort for now (could be optimized with better algorithms)
    for (u32 i = 0; i < renderer->sprite_count - 1; i++) {
        for (u32 j = 0; j < renderer->sprite_count - i - 1; j++) {
            Sprite3D *sprite_a = &renderer->sprites[j];
            Sprite3D *sprite_b = &renderer->sprites[j + 1];
            
            if (!sprite_a->visible || !sprite_b->visible) continue;
            
            f32 depth_a = get_sprite_depth(sprite_a, renderer->render_mode);
            f32 depth_b = get_sprite_depth(sprite_b, renderer->render_mode);
            
            if (depth_a > depth_b) {
                // Swap sprites
                Sprite3D temp = *sprite_a;
                *sprite_a = *sprite_b;
                *sprite_b = temp;
            }
        }
    }
}

// Batch sprites
void sprite_3d_batch_sprites(Sprite3DRenderer *renderer) {
    if (!renderer) return;
    
    // Reset batches
    renderer->batch_count = 0;
    renderer->sprites_rendered = 0;
    renderer->batches_rendered = 0;
    
    // Find first visible sprite
    u32 start_index = 0;
    while (start_index < renderer->sprite_count && !renderer->sprites[start_index].visible) {
        start_index++;
    }
    
    if (start_index >= renderer->sprite_count) return;
    
    // Create initial batch
    SpriteBatch *current_batch = &renderer->batches[0];
    current_batch->vertex_count = 0;
    current_batch->index_count = 0;
    current_batch->texture_id = renderer->sprites[start_index].texture_id;
    current_batch->blend_mode = renderer->sprites[start_index].blend_mode;
    renderer->batch_count = 1;
    
    // Process sprites and create batches
    for (u32 i = start_index; i < renderer->sprite_count; i++) {
        Sprite3D *sprite = &renderer->sprites[i];
        
        if (!sprite->visible) continue;
        
        // Check if we need a new batch
        if (sprite->texture_id != current_batch->texture_id || 
            sprite->blend_mode != current_batch->blend_mode ||
            current_batch->vertex_count + VERTICES_PER_SPRITE > current_batch->vertex_capacity) {
            
            // Move to next batch
            if (renderer->batch_count >= renderer->max_batches) {
                LOG_WARN("Maximum batch count reached");
                break;
            }
            
            current_batch = &renderer->batches[renderer->batch_count++];
            current_batch->vertex_count = 0;
            current_batch->index_count = 0;
            current_batch->texture_id = sprite->texture_id;
            current_batch->blend_mode = sprite->blend_mode;
        }
        
        // Add sprite to current batch
        if (current_batch->vertex_count + VERTICES_PER_SPRITE <= current_batch->vertex_capacity) {
            GPUSpriteVertex vertices[VERTICES_PER_SPRITE];
            create_sprite_vertices(sprite, vertices, &renderer->view_projection_matrix);
            
            // Copy vertices
            memcpy(&current_batch->vertices[current_batch->vertex_count], 
                   vertices, sizeof(GPUSpriteVertex) * VERTICES_PER_SPRITE);
            
            // Add indices
            u32 base_vertex = current_batch->vertex_count;
            u32 indices[INDICES_PER_SPRITE] = {
                base_vertex + 0, base_vertex + 1, base_vertex + 2,
                base_vertex + 2, base_vertex + 3, base_vertex + 0
            };
            
            memcpy(&current_batch->indices[current_batch->index_count],
                   indices, sizeof(u32) * INDICES_PER_SPRITE);
            
            current_batch->vertex_count += VERTICES_PER_SPRITE;
            current_batch->index_count += INDICES_PER_SPRITE;
            renderer->sprites_rendered++;
        }
    }
    
    renderer->batches_rendered = renderer->batch_count;
}

// Render frame
void sprite_3d_renderer_render(Sprite3DRenderer *renderer, const Mat4 *view, const Mat4 *projection) {
    if (!renderer || !renderer->initialized) return;
    
    // Update matrices
    if (view) renderer->view_matrix = *view;
    if (projection) renderer->projection_matrix = *projection;
    
    // Sort sprites by depth
    sprite_3d_sort_sprites_by_depth(renderer);
    
    // Batch sprites
    sprite_3d_batch_sprites(renderer);
    
    // Upload batch data to GPU buffers
    u32 vertex_offset = 0;
    u32 index_offset = 0;
    
    for (u32 i = 0; i < renderer->batch_count; i++) {
        SpriteBatch *batch = &renderer->batches[i];
        
        if (batch->vertex_count == 0) continue;
        
        // Upload vertices and indices
        buffer_upload_sub(renderer->vertex_buffer, 
                        batch->vertices, 
                        sizeof(GPUSpriteVertex) * batch->vertex_count,
                        vertex_offset * sizeof(GPUSpriteVertex));
        
        buffer_upload_sub(renderer->index_buffer,
                        batch->indices,
                        sizeof(u32) * batch->index_count,
                        index_offset * sizeof(u32));
        
        // This would normally call the actual rendering API
        // For now, we just log the batch
        LOG_DEBUG("Render batch %d: %d vertices, %d indices, texture %d, blend %d",
                 i, batch->vertex_count, batch->index_count, batch->texture_id, batch->blend_mode);
        
        vertex_offset += batch->vertex_count;
        index_offset += batch->index_count;
    }
    
    renderer->draw_calls = renderer->batch_count;
    
    LOG_DEBUG("Sprite 3D render: %d sprites, %d batches, %d draw calls",
             renderer->sprites_rendered, renderer->batches_rendered, renderer->draw_calls);
}

// Get statistics
void sprite_3d_renderer_get_stats(Sprite3DRenderer *renderer, u32 *sprites_rendered, 
                                 u32 *batches_rendered, u32 *draw_calls) {
    if (!renderer) return;
    
    if (sprites_rendered) *sprites_rendered = renderer->sprites_rendered;
    if (batches_rendered) *batches_rendered = renderer->batches_rendered;
    if (draw_calls) *draw_calls = renderer->draw_calls;
}

// Service implementation for IRenderer interface
typedef struct Sprite3DRendererService {
    IRenderer base;
    Sprite3DRenderer *sprite_renderer;
    bool initialized;
} Sprite3DRendererService;

static bool sprite_3d_service_init(IRenderer *renderer, u32 width, u32 height) {
    Sprite3DRendererService *service = (Sprite3DRendererService *)renderer;
    
    service->sprite_renderer = sprite_3d_renderer_create(SPRITE_MODE_ISOMETRIC);
    if (!service->sprite_renderer) {
        LOG_ERROR("Failed to create sprite 3D renderer");
        return false;
    }
    
    sprite_3d_renderer_set_viewport(service->sprite_renderer, width, height);
    service->initialized = true;
    
    LOG_INFO("Sprite 3D renderer service initialized: %ux%u", width, height);
    return true;
}

static void sprite_3d_service_shutdown(IRenderer *renderer) {
    Sprite3DRendererService *service = (Sprite3DRendererService *)renderer;
    
    if (service->sprite_renderer) {
        sprite_3d_renderer_destroy(service->sprite_renderer);
        service->sprite_renderer = NULL;
    }
    
    service->initialized = false;
    LOG_INFO("Sprite 3D renderer service shutdown");
}

// Create sprite 3D renderer service
IRenderer *sprite_3d_renderer_create(void) {
    Sprite3DRendererService *service = malloc(sizeof(Sprite3DRendererService));
    if (!service) {
        LOG_ERROR("Failed to allocate sprite 3D renderer service");
        return NULL;
    }
    
    memset(service, 0, sizeof(Sprite3DRendererService));
    
    // Set up IRenderer interface (minimal implementation)
    service->base.init = sprite_3d_service_init;
    service->base.shutdown = sprite_3d_service_shutdown;
    service->base.begin_frame = NULL;
    service->base.end_frame = NULL;
    service->base.clear = NULL;
    service->base.present = NULL;
    service->base.set_viewport = NULL;
    service->base.set_camera = NULL;
    service->base.draw_mesh = NULL;
    service->base.draw_mesh_instanced = NULL;
    service->base.set_lighting = NULL;
    service->base.set_material = NULL;
    service->base.set_texture = NULL;
    service->base.set_uniform = NULL;
    service->base.get_stats = NULL;
    
    LOG_INFO("Sprite 3D renderer service created");
    return (IRenderer *)service;
}
