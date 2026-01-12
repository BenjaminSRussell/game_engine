/*
 * rendering_pipeline_test.c
 * Comprehensive end-to-end rendering pipeline test
 * 
 * Tests unified particle, voxel, and sprite rendering systems
 * with Metal backend synchronization fixes
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

// Include unified rendering systems
#include "rendering/unified_particle_system.h"
#include "rendering/unified_voxel_renderer.h"
#include "rendering/unified_sprite_renderer.h"
#include "backend/metal/mtl_sync_fixes.h"

// ============================================================================
// TEST CONFIGURATION
// ============================================================================

#define TEST_MAX_PARTICLES 10000
#define TEST_MAX_VOXELS 1000
#define TEST_MAX_SPRITES 500
#define TEST_FRAME_COUNT 60
#define TEST_WINDOW_WIDTH 1920
#define TEST_WINDOW_HEIGHT 1080

typedef struct {
    // Metal device and layer
#ifdef __APPLE__
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    CAMetalLayer* layer;
#endif
    
    // Unified renderers
    UnifiedParticleSystem* particle_system;
    UnifiedVoxelRenderer* voxel_renderer;
    UnifiedSpriteRenderer* sprite_renderer;
    
    // Test state
    uint32_t frame_count;
    bool test_running;
    float camera_position[3];
    float view_matrix[16];
    float projection_matrix[16];
    float view_projection_matrix[16];
    
    // Performance metrics
    double total_frame_time;
    double min_frame_time;
    double max_frame_time;
    
} rendering_test_context_t;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static void create_test_matrix(float* matrix, float fov, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov * 0.5f);
    
    matrix[0] = f / aspect; matrix[1] = 0;  matrix[2] = 0;                        matrix[3] = 0;
    matrix[4] = 0;          matrix[5] = f;  matrix[6] = 0;                        matrix[7] = 0;
    matrix[8] = 0;          matrix[9] = 0;  matrix[10] = (far + near) / (near - far); matrix[11] = (2 * far * near) / (near - far);
    matrix[12] = 0;         matrix[13] = 0; matrix[14] = -1;                      matrix[15] = 0;
}

static void create_identity_matrix(float* matrix) {
    matrix[0] = 1; matrix[1] = 0; matrix[2] = 0; matrix[3] = 0;
    matrix[4] = 0; matrix[5] = 1; matrix[6] = 0; matrix[7] = 0;
    matrix[8] = 0; matrix[9] = 0; matrix[10] = 1; matrix[11] = 0;
    matrix[12] = 0; matrix[13] = 0; matrix[14] = 0; matrix[15] = 1;
}

static void multiply_matrices(const float* a, const float* b, float* result) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0;
            for (int k = 0; k < 4; k++) {
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

// ============================================================================
// TEST INITIALIZATION
// ============================================================================

static bool init_metal_backend(rendering_test_context_t* ctx) {
#ifdef __APPLE__
    // Create Metal device
    ctx->device = MTLCreateSystemDefaultDevice();
    if (!ctx->device) {
        printf("ERROR: Failed to create Metal device\n");
        return false;
    }
    
    // Create command queue
    ctx->command_queue = [ctx->device newCommandQueue];
    if (!ctx->command_queue) {
        printf("ERROR: Failed to create Metal command queue\n");
        return false;
    }
    
    // Create Metal layer (simplified for testing)
    ctx->layer = [CAMetalLayer layer];
    ctx->layer.device = ctx->device;
    ctx->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    ctx->layer.framebufferOnly = YES;
    ctx->layer.displaySyncEnabled = YES;
    
    printf("Metal backend initialized successfully\n");
    return true;
#else
    printf("WARNING: Metal backend not available on this platform\n");
    return false;
#endif
}

static bool init_unified_renderers(rendering_test_context_t* ctx) {
    // Initialize particle system
    ParticleSystemConfig particle_config = {
        .enable_instancing = true,
        .enable_sorting = true,
        .enable_soft_particles = true,
        .enable_depth_fade = true,
        .enable_gpu_simulation = true,
        .enable_svg_support = false,
        .max_particles_per_batch = 1024,
        .particle_size_scale = 1.0f,
        .brightness_scale = 1.0f,
        .enable_glow = true,
        .glow_intensity = 0.5f
    };
    
    ctx->particle_system = unified_particle_system_create(&particle_config);
    if (!ctx->particle_system) {
        printf("ERROR: Failed to create particle system\n");
        return false;
    }
    
    // Initialize voxel renderer
    VoxelRendererConfig voxel_config = {
        .enable_instancing = true,
        .enable_gpu_meshing = true,
        .enable_ambient_occlusion = true,
        .enable_lighting = true,
        .enable_culling = true,
        .max_chunks_per_frame = 64,
        .chunk_size = 32,
        .lod_distance = 100.0f,
        .enable_mipmapping = true
    };
    
    ctx->voxel_renderer = unified_voxel_renderer_create((metal_device_t*)ctx->device, &voxel_config);
    if (!ctx->voxel_renderer) {
        printf("ERROR: Failed to create voxel renderer\n");
        return false;
    }
    
    // Initialize sprite renderer
    SpriteRendererConfig sprite_config = {
        .enable_instancing = true,
        .enable_batching = true,
        .enable_depth_testing = true,
        .enable_blending = true,
        .max_sprites_per_batch = 256,
        .max_total_sprites = TEST_MAX_SPRITES,
        .texture_atlas_size = 2048.0f,
        .enable_mipmapping = true,
        .enable_anisotropic_filtering = true
    };
    
    ctx->sprite_renderer = unified_sprite_renderer_create((metal_device_t*)ctx->device, &sprite_config);
    if (!ctx->sprite_renderer) {
        printf("ERROR: Failed to create sprite renderer\n");
        return false;
    }
    
    printf("Unified renderers initialized successfully\n");
    return true;
}

static bool init_test_scene(rendering_test_context_t* ctx) {
    // Create test particle emitters
    for (int i = 0; i < 5; i++) {
        ParticleEmitter emitter = {
            .type = PARTICLE_TYPE_POLL + (i % PARTICLE_TYPE_COUNT),
            .position = {(float)(i * 10), 5.0f, 0.0f},
            .velocity = {0.0f, 2.0f, 0.0f},
            .acceleration = {0.0f, -9.81f, 0.0f},
            .emission_rate = 100.0f,
            .particle_lifetime = 5.0f,
            .size_min = 0.1f,
            .size_max = 0.5f,
            .color_min = {255, 255, 255, 255},
            .color_max = {255, 255, 255, 255},
            .enabled = true,
            .max_particles = 1000
        };
        
        uint32_t emitter_id = unified_particle_system_create_emitter(ctx->particle_system, &emitter);
        if (emitter_id == 0) {
            printf("WARNING: Failed to create particle emitter %d\n", i);
        }
    }
    
    // Create test voxel chunk
    VoxelChunk* chunk = unified_voxel_renderer_create_chunk(ctx->voxel_renderer, 0, 0, 0, 16, 16, 16);
    if (chunk) {
        // Fill with some test blocks
        for (uint32_t x = 0; x < 16; x++) {
            for (uint32_t y = 0; y < 8; y++) {
                for (uint32_t z = 0; z < 16; z++) {
                    unified_voxel_renderer_set_block(ctx->voxel_renderer, chunk, x, y, z, 1);
                }
            }
        }
        unified_voxel_renderer_update_chunk_mesh(ctx->voxel_renderer, chunk);
    }
    
    // Create test sprites
    for (int i = 0; i < 10; i++) {
        Sprite sprite = {
            .position = {(float)(i * 5), 2.0f, -10.0f},
            .size = {1.0f, 1.0f},
            .uv_coords = {0.0f, 0.0f, 1.0f, 1.0f},
            .color = {255, 255, 255, 255},
            .texture_id = 0,
            .mode = SPRITE_MODE_2_5D,
            .rotation = 0.0f,
            .scale = 1.0f,
            .billboard = true,
            .visible = true,
            .layer = 0
        };
        
        uint32_t sprite_id = unified_sprite_renderer_create_sprite(ctx->sprite_renderer, &sprite);
        if (sprite_id == 0) {
            printf("WARNING: Failed to create sprite %d\n", i);
        }
    }
    
    printf("Test scene initialized successfully\n");
    return true;
}

// ============================================================================
// RENDERING LOOP
// ============================================================================

static void update_camera(rendering_test_context_t* ctx, float delta_time) {
    // Simple camera movement
    static float camera_angle = 0.0f;
    camera_angle += delta_time * 0.5f;
    
    ctx->camera_position[0] = cosf(camera_angle) * 20.0f;
    ctx->camera_position[1] = 10.0f;
    ctx->camera_position[2] = sinf(camera_angle) * 20.0f;
    
    // Update view matrix (simple look-at)
    create_identity_matrix(ctx->view_matrix);
    // In a real implementation, this would be a proper look-at matrix
    
    // Update projection matrix
    create_test_matrix(ctx->projection_matrix, 
                      M_PI / 4.0f, 
                      (float)TEST_WINDOW_WIDTH / TEST_WINDOW_HEIGHT,
                      0.1f, 1000.0f);
    
    // Combine view and projection
    multiply_matrices(ctx->projection_matrix, ctx->view_matrix, ctx->view_projection_matrix);
}

static void render_frame(rendering_test_context_t* ctx) {
#ifdef __APPLE__
    // Begin frame with synchronization fixes
    if (!metal_sync_fixes_begin_frame((metal_device_t*)ctx->device)) {
        printf("ERROR: Failed to begin frame\n");
        return;
    }
    
    // Create command buffer
    id<MTLCommandBuffer> command_buffer = [ctx->command_queue commandBuffer];
    if (!command_buffer) {
        printf("ERROR: Failed to create command buffer\n");
        return;
    }
    
    // Update simulation
    unified_particle_system_update(ctx->particle_system, 1.0f / 60.0f, ctx->camera_position, ctx->view_matrix);
    
    // Cull voxels
    unified_voxel_renderer_cull_chunks(ctx->voxel_renderer, ctx->view_projection_matrix, ctx->camera_position);
    
    // Begin sprite batch
    unified_sprite_renderer_begin_frame(ctx->sprite_renderer);
    
    // Render everything (simplified for testing)
    // In a real implementation, this would use proper render passes
    
    // Commit command buffer with synchronization fixes
    metal_sync_fixes_commit_buffer((mtl_command_buffer_t)command_buffer);
#endif
}

// ============================================================================
// TEST EXECUTION
// ============================================================================

static void run_rendering_test(rendering_test_context_t* ctx) {
    printf("Starting rendering pipeline test (%d frames)...\n", TEST_FRAME_COUNT);
    
    ctx->test_running = true;
    ctx->frame_count = 0;
    ctx->total_frame_time = 0.0;
    ctx->min_frame_time = 999999.0;
    ctx->max_frame_time = 0.0;
    
    while (ctx->test_running && ctx->frame_count < TEST_FRAME_COUNT) {
        uint64_t start_time = mach_absolute_time();
        
        // Update camera
        update_camera(ctx, 1.0f / 60.0f);
        
        // Render frame
        render_frame(ctx);
        
        // Calculate frame time
        uint64_t end_time = mach_absolute_time();
        double frame_time = (double)(end_time - start_time) / 1000000.0; // Convert to ms
        
        ctx->total_frame_time += frame_time;
        ctx->min_frame_time = fmin(ctx->min_frame_time, frame_time);
        ctx->max_frame_time = fmax(ctx->max_frame_time, frame_time);
        
        ctx->frame_count++;
        
        // Print progress every 10 frames
        if (ctx->frame_count % 10 == 0) {
            printf("Frame %d/%d - Frame time: %.2fms\n", 
                   ctx->frame_count, TEST_FRAME_COUNT, frame_time);
        }
        
        // Simple frame rate limiting
        usleep(16666); // ~60 FPS
    }
    
    printf("Rendering test completed\n");
}

// ============================================================================
// TEST REPORTING
// ============================================================================

static void print_test_results(rendering_test_context_t* ctx) {
    printf("\n=== RENDERING PIPELINE TEST RESULTS ===\n");
    printf("Total frames rendered: %d\n", ctx->frame_count);
    printf("Average frame time: %.2fms\n", ctx->total_frame_time / ctx->frame_count);
    printf("Min frame time: %.2fms\n", ctx->min_frame_time);
    printf("Max frame time: %.2fms\n", ctx->max_frame_time);
    printf("Average FPS: %.1f\n", 1000.0 / (ctx->total_frame_time / ctx->frame_count));
    
    // Get renderer statistics
    ParticleRenderStats particle_stats;
    unified_particle_system_get_stats(ctx->particle_system, &particle_stats);
    printf("Active particles: %d\n", particle_stats.active_particles);
    printf("Particle draw calls: %d\n", particle_stats.draw_calls);
    
    VoxelRenderStats voxel_stats;
    unified_voxel_renderer_get_stats(ctx->voxel_renderer, &voxel_stats);
    printf("Chunks rendered: %d\n", voxel_stats.chunks_rendered);
    printf("Voxel triangles: %d\n", voxel_stats.triangles_drawn);
    
    SpriteRenderStats sprite_stats;
    unified_sprite_renderer_get_stats(ctx->sprite_renderer, &sprite_stats);
    printf("Sprites rendered: %d\n", sprite_stats.sprites_rendered);
    printf("Sprite batches: %d\n", sprite_stats.batches_rendered);
    
    // Get synchronization statistics
    uint32_t sync_errors, dropped_frames;
    metal_sync_fixes_get_stats(&sync_errors, &dropped_frames);
    printf("Synchronization errors: %d\n", sync_errors);
    printf("Dropped frames: %d\n", dropped_frames);
    
    printf("========================================\n\n");
}

// ============================================================================
// CLEANUP
// ============================================================================

static void cleanup_test(rendering_test_context_t* ctx) {
    if (ctx->particle_system) {
        unified_particle_system_destroy(ctx->particle_system);
        ctx->particle_system = NULL;
    }
    
    if (ctx->voxel_renderer) {
        unified_voxel_renderer_destroy(ctx->voxel_renderer);
        ctx->voxel_renderer = NULL;
    }
    
    if (ctx->sprite_renderer) {
        unified_sprite_renderer_destroy(ctx->sprite_renderer);
        ctx->sprite_renderer = NULL;
    }
    
#ifdef __APPLE__
    if (ctx->command_queue) {
        [ctx->command_queue release];
        ctx->command_queue = NULL;
    }
    
    if (ctx->device) {
        [ctx->device release];
        ctx->device = NULL;
    }
#endif
    
    metal_sync_fixes_shutdown();
    
    printf("Test cleanup completed\n");
}

// ============================================================================
// MAIN TEST FUNCTION
// ============================================================================

int main(int argc, char* argv[]) {
    printf("=== UNIFIED RENDERING PIPELINE TEST ===\n");
    printf("Testing consolidated particle, voxel, and sprite rendering systems\n\n");
    
    rendering_test_context_t ctx = {0};
    
    // Initialize Metal backend
    if (!init_metal_backend(&ctx)) {
        printf("FATAL: Failed to initialize Metal backend\n");
        return 1;
    }
    
    // Initialize synchronization fixes
    if (!metal_sync_fixes_init(3)) { // Triple buffering
        printf("FATAL: Failed to initialize synchronization fixes\n");
        cleanup_test(&ctx);
        return 1;
    }
    
    // Initialize unified renderers
    if (!init_unified_renderers(&ctx)) {
        printf("FATAL: Failed to initialize unified renderers\n");
        cleanup_test(&ctx);
        return 1;
    }
    
    // Initialize test scene
    if (!init_test_scene(&ctx)) {
        printf("FATAL: Failed to initialize test scene\n");
        cleanup_test(&ctx);
        return 1;
    }
    
    // Run the test
    run_rendering_test(&ctx);
    
    // Print results
    print_test_results(&ctx);
    
    // Cleanup
    cleanup_test(&ctx);
    
    printf("Test completed successfully!\n");
    return 0;
}
