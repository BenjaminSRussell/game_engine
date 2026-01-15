// Voxel Renderer Demo
// Demonstrates the voxel renderer with a simple world

#include "voxel_renderer.h"
#include "engine/include/core/logger.h"
#include "engine/include/math/math_all.h"
#include <stdlib.h>

// Demo world generation
static void generate_demo_world(VoxelRenderer *renderer) {
    // Create a few chunks with different terrain
    for (int chunk_x = -2; chunk_x <= 2; chunk_x++) {
        for (int chunk_z = -2; chunk_z <= 2; chunk_z++) {
            Vec3 chunk_pos = vec3_create(
                (f32)chunk_x * 16.0f,
                0.0f,
                (f32)chunk_z * 16.0f
            );
            
            VoxelChunk *chunk = voxel_renderer_create_chunk(
                renderer, &chunk_pos, 16, 64, 16);
            
            if (!chunk) continue;
            
            // Generate simple terrain
            for (u32 x = 0; x < 16; x++) {
                for (u32 z = 0; z < 16; z++) {
                    // Simple height function
                    f32 height_f = 32.0f + 
                        sinf((f32)(chunk_x * 16 + x) * 0.1f) * 8.0f +
                        cosf((f32)(chunk_z * 16 + z) * 0.1f) * 8.0f;
                    
                    u32 height = (u32)height_f;
                    height = (height < 4) ? 4 : (height > 60) ? 60 : height;
                    
                    for (u32 y = 0; y < height; y++) {
                        VoxelBlockType block_type = BLOCK_TYPE_STONE;
                        
                        if (y == height - 1) {
                            block_type = BLOCK_TYPE_GRASS;
                        } else if (y >= height - 3) {
                            block_type = BLOCK_TYPE_DIRT;
                        }
                        
                        voxel_renderer_set_block(renderer, chunk, x, y, z, block_type);
                    }
                    
                    // Add some trees occasionally
                    if (height > 10 && (rand() % 20) == 0) {
                        // Tree trunk
                        for (u32 y = height; y < height + 4 && y < 64; y++) {
                            voxel_renderer_set_block(renderer, chunk, x, y, z, BLOCK_TYPE_WOOD);
                        }
                        
                        // Tree leaves (simple cross shape)
                        u32 tree_top = height + 4;
                        if (tree_top < 64) {
                            voxel_renderer_set_block(renderer, chunk, x, tree_top, z, BLOCK_TYPE_LEAVES);
                            if (x > 0) voxel_renderer_set_block(renderer, chunk, x-1, tree_top, z, BLOCK_TYPE_LEAVES);
                            if (x < 15) voxel_renderer_set_block(renderer, chunk, x+1, tree_top, z, BLOCK_TYPE_LEAVES);
                            if (z > 0) voxel_renderer_set_block(renderer, chunk, x, tree_top, z-1, BLOCK_TYPE_LEAVES);
                            if (z < 15) voxel_renderer_set_block(renderer, chunk, x, tree_top, z+1, BLOCK_TYPE_LEAVES);
                        }
                    }
                }
            }
            
            LOG_DEBUG("Generated chunk at (%.1f, %.1f, %.1f)", 
                     chunk_pos.x, chunk_pos.y, chunk_pos.z);
        }
    }
}

// Demo main function
void voxel_renderer_demo_run(void) {
    LOG_INFO("Starting voxel renderer demo");
    
    // Create voxel renderer
    VoxelRenderer *renderer = voxel_renderer_create();
    if (!renderer) {
        LOG_ERROR("Failed to create voxel renderer");
        return;
    }
    
    // Generate demo world
    generate_demo_world(renderer);
    
    // Set up camera
    Vec3 camera_pos = vec3_create(0.0f, 40.0f, 50.0f);
    Vec3 camera_target = vec3_create(0.0f, 30.0f, 0.0f);
    Vec3 camera_up = vec3_create(0.0f, 1.0f, 0.0f);
    
    Mat4 view = mat4_look_at(&camera_pos, &camera_target, &camera_up);
    Mat4 projection = mat4_perspective(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    
    renderer->camera_position = camera_pos;
    renderer->camera_direction = vec3_normalize(vec3_sub(&camera_target, &camera_pos));
    
    // Simulate a few frames
    for (int frame = 0; frame < 10; frame++) {
        LOG_INFO("Rendering frame %d", frame + 1);
        
        // Update camera position (simple circular motion)
        f32 angle = (f32)frame * 0.1f;
        camera_pos.x = sinf(angle) * 50.0f;
        camera_pos.z = cosf(angle) * 50.0f;
        
        view = mat4_look_at(&camera_pos, &camera_target, &camera_up);
        
        // Update and render
        voxel_renderer_update(renderer, 0.016f);
        voxel_renderer_render(renderer, &view, &projection);
        
        // Get statistics
        u32 chunks_rendered, vertices_rendered, triangles_rendered;
        voxel_renderer_get_stats(renderer, &chunks_rendered, &vertices_rendered, &triangles_rendered);
        
        LOG_INFO("Frame %d stats: %d chunks, %d vertices, %d triangles", 
                frame + 1, chunks_rendered, vertices_rendered, triangles_rendered);
    }
    
    // Cleanup
    voxel_renderer_destroy(renderer);
    LOG_INFO("Voxel renderer demo completed");
}
