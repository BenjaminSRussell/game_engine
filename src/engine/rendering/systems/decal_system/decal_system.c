#include "rendering/systems/decal_system/decal_system.h"

#include <core/logger.h>
#include <rendering/render_backend.h>
#include <rendering/camera.h>
#include <rendering/culling.h> // For Frustum and AABB
#include <math/aabb.h>
#include <stdlib.h> 

#define MAX_DECALS 2048

struct DecalSystem {
    Decal decals[MAX_DECALS];
    u32 active_count;
    
    // GPU Resources
    u32 shader_id;
    u32 cube_vao;
    u32 cube_vbo;
    u32 depth_texture_id;
};

// Internal helpers
static void create_unit_cube(DecalSystem* system);

// Helper to calc AABB for rotated decal
static AABB calculate_decal_aabb(Decal* d) {
    // Decal volume is -0.5 to 0.5 in local space scaled by d->scale
    // We transform the 8 corners
    Vec3 corners[8];
    Vec3 extents = vec3_mul_scalar(d->scale, 0.5f);
    
    Vec3 min_pt = vec3_mul_scalar(extents, -1.0f);
    Vec3 max_pt = extents;
    
    // Generate 8 corners logic manually because loop is tedious without array
    corners[0] = (Vec3){min_pt.x, min_pt.y, min_pt.z};
    corners[1] = (Vec3){max_pt.x, min_pt.y, min_pt.z};
    corners[2] = (Vec3){min_pt.x, max_pt.y, min_pt.z};
    corners[3] = (Vec3){max_pt.x, max_pt.y, min_pt.z};
    corners[4] = (Vec3){min_pt.x, min_pt.y, max_pt.z};
    corners[5] = (Vec3){max_pt.x, min_pt.y, max_pt.z};
    corners[6] = (Vec3){min_pt.x, max_pt.y, max_pt.z};
    corners[7] = (Vec3){max_pt.x, max_pt.y, max_pt.z};
    
    Mat4 rot = quat_to_mat4(d->rotation);
    Vec3 world_min = (Vec3){FLT_MAX, FLT_MAX, FLT_MAX};
    Vec3 world_max = (Vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX};
    
    for (u32 i = 0; i < 8; ++i) {
        Vec3 rotated = mat4_mul_vec3(rot, corners[i]);
        Vec3 final_pos = vec3_add(d->position, rotated);
        
        world_min = vec3_min(world_min, final_pos);
        world_max = vec3_max(world_max, final_pos);
    }
    
    AABB box;
    box.min = world_min;
    box.max = world_max;
    return box;
}

DecalSystem* decal_system_create(void) {
    DecalSystem* system = (DecalSystem*)calloc(1, sizeof(DecalSystem));
    if (!system) {
        LOG_FATAL("Failed to allocate DecalSystem memory.");
        return NULL;
    }
    
    system->active_count = 0;
    
    // TODO: Load shader 'decal.vert' and 'decal.frag'
    // system->shader_id = render_backend_shader_load("decal");
    
    create_unit_cube(system);
    
    LOG_INFO("Decal System initialized. Max Decals: %d", MAX_DECALS);
    return system;
}

void decal_system_destroy(DecalSystem* system) {
    if (system) {
        // TODO: Release GPU resources
        
        free(system);
        LOG_INFO("Decal System destroyed.");
    }
}

void decal_system_update(DecalSystem* system, f32 dt) {
    if (!system) return;
    
    system->active_count = 0;
    
    for (u32 i = 0; i < MAX_DECALS; ++i) {
        if (system->decals[i].active) {
            Decal* d = &system->decals[i];
            
            // Update lifetime
            if (d->lifetime > 0.0f) {
                d->lifetime -= dt;
                if (d->lifetime <= 0.0f) {
                    d->active = false;
                }
            }
            
            if (d->active) {
                system->active_count++;
            }
        }
    }
}

void decal_system_render(DecalSystem* system, struct GBuffer* gbuffer, struct Camera* camera) {
    if (!system || system->active_count == 0 || !gbuffer || !camera) return;
    
    // Frustum extraction (Assuming camera has it, or we build it)
    Frustum frustum;
    // Assuming camera->view_projection exists or calculated here
    // mat4 vp = mat4_mul(camera->projection, camera->view);
    // frustum_init(&frustum, vp); 
    // Using placeholder logic:
    (void)frustum; 

    // Render Setup
    // render_backend_set_depth_test(true);
    // render_backend_set_cull_mode(CULL_MODE_FRONT); 
    
    for (u32 i = 0; i < MAX_DECALS; ++i) {
        if (system->decals[i].active) {
            Decal* d = &system->decals[i];
            
            // Frustum Culling
            // AABB bounds = calculate_decal_aabb(d);
            // if (frustum_test_aabb(&frustum, &bounds) == OUTSIDE_FRUSTUM) continue;
            
            // Calculate Model Matrix
            Mat4 model = mat4_identity();
            model = mat4_translate(model, d->position);
            model = mat4_mul(model, quat_to_mat4(d->rotation));
            model = mat4_scale(model, d->scale);
            
            Mat4 invModel = mat4_inverse(model);
            
            struct {
                Mat4 model;
                Mat4 invModel;
                Vec4 colorTint;
                f32 normalOpacity;
                f32 roughnessScale;
                f32 metallicScale;
                f32 decalId;
            } push;
            
            push.model = model;
            push.invModel = invModel;
            push.colorTint = d->color_tint;
            push.normalOpacity = 1.0f;
            push.roughnessScale = 1.0f;
            push.metallicScale = 1.0f;
            push.decalId = (f32)(i + 1);
            
            // Draw
            // render_backend_push_constants(&push, sizeof(push));
            // render_backend_draw_indexed(36, 0, 0); 
        }
    }
}

void decal_system_render_debug(DecalSystem* system, struct Camera* camera) {
    if (!system || !camera) return;
    
    for (u32 i = 0; i < MAX_DECALS; ++i) {
        if (system->decals[i].active) {
            Decal* d = &system->decals[i];
            AABB bounds = calculate_decal_aabb(d);
            // render_debug_aabb(bounds, (Vec4){0, 1, 0, 1}); // Green box
            (void)bounds;
        }
    }
}

u32 decal_system_add_decal(DecalSystem* system, Decal decal) {
    if (!system) return 0;
    for (u32 i = 0; i < MAX_DECALS; ++i) {
        if (!system->decals[i].active) {
            system->decals[i] = decal;
            system->decals[i].active = true;
            system->active_count++;
            return i + 1;
        }
    }
    LOG_WARN("Decal System full!");
    return 0; 
}

void decal_system_remove_decal(DecalSystem* system, u32 decal_id) {
    if (!system || decal_id == 0 || decal_id > MAX_DECALS) return;
    system->decals[decal_id - 1].active = false;
}

void decal_system_clear(DecalSystem* system) {
    if (!system) return;
    for (u32 i = 0; i < MAX_DECALS; ++i) {
        system->decals[i].active = false;
    }
    system->active_count = 0;
}

void decal_system_set_depth_texture(DecalSystem* system, u32 depth_texture_id) {
    if (system) {
        system->depth_texture_id = depth_texture_id;
    }
}

static void create_unit_cube(DecalSystem* system) {
    f32 vertices[] = {
        // Back
        -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        // Front
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        // Left
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
        // Right
         0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
        // Bottom
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        // Top
        -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f
    };
    // system->cube_vbo = render_backend_buffer_create(BUFFER_TYPE_VERTEX, sizeof(vertices), vertices);
    // VertexLayout layout = {0}; 
    // system->cube_vao = render_backend_vao_create(&layout, system->cube_vbo, 0);
    (void)vertices;
}
