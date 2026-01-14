#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float position[3];
    float rotation[4]; // quaternion
    float scale[3];
    
    uint64_t texture_id;
    float lifetime;
    float fade_duration;
    bool use_normal_blending;
} Decal;

typedef struct {
    Decal *decals;
    int decal_count;
    int capacity;
    
    // Rendering
    void *deferred_buffer; // DBuffer for deferred decals
    void *clustered_data;
} DecalManager;

void decal_manager_init(DecalManager *mgr);
void decal_manager_destroy(DecalManager *mgr);

// Decal management
Decal* decal_spawn(DecalManager *mgr, const float *pos, const float *normal, uint64_t texture);
void decal_remove(DecalManager *mgr, int index);
void decal_update(DecalManager *mgr, float delta_time);

// Rendering
void decal_render_deferred(DecalManager *mgr, void *render_pass);
void decal_render_mesh_clipped(DecalManager *mgr, Decal *decal, void *mesh);

// Batching
void decal_batch_by_texture(DecalManager *mgr);
