// include/vfx/vfx_optimization.h
//
// Purpose: VFX optimization systems (culling, batching, priority)
//
#ifndef VFX_OPTIMIZATION_H
#define VFX_OPTIMIZATION_H

#include "engine/include/common.h"
#include <math/vec3.h>
#include "include/effects/vfx/particle_system.h"

#define MAX_CULLING_CELLS 256
#define CELL_SIZE 8.0f  // 8 meter cells

typedef struct {
    i32 x, y, z;
} CellCoord;

typedef struct {
    CellCoord coord;
    u32 particleIndices[64];  // Particles in this cell
    u32 particleCount;
} SpatialCell;

typedef struct {
    SpatialCell cells[MAX_CULLING_CELLS];
    u32 cellCount;
    bool initialized;
} ParticleCullingSystem;

// Particle culling
void particle_culling_init(ParticleCullingSystem* system);
void particle_culling_shutdown(ParticleCullingSystem* system);
void particle_culling_update(ParticleCullingSystem* system, ParticleSystem* particles,
                            Vec3 cameraPos, f32 cullRadius);
void particle_culling_build_spatial_hash(ParticleCullingSystem* system,
                                        ParticleSystem* particles);

// Query culled particles
u32 particle_culling_get_visible(ParticleCullingSystem* system, Vec3 cameraPos,
                                f32 radius, u32* outIndices, u32 maxIndices);

// Effect priority system
typedef enum {
    EFFECT_PRIORITY_LOW,
    EFFECT_PRIORITY_NORMAL,
    EFFECT_PRIORITY_HIGH,
    EFFECT_PRIORITY_CRITICAL
} EffectPriority;

typedef struct {
    u32 effectId;
    EffectPriority priority;
    f32 distance;
    f32 importance;  // Calculated based on type/size
} EffectQueueEntry;

typedef struct {
    EffectQueueEntry queue[256];
    u32 queueSize;
    bool initialized;
} EffectPrioritySystem;

void effect_priority_init(EffectPrioritySystem* system);
void effect_priority_add(EffectPrioritySystem* system, u32 effectId,
                        EffectPriority priority, f32 distance);
void effect_priority_sort(EffectPrioritySystem* system);
void effect_priority_clear(EffectPrioritySystem* system);

// Batching utilities
typedef struct {
    u32 particleIndices[MAX_PARTICLES];
    u32 particleCount;
    u32 totalVertices;
} ParticleBatch;

void particle_batch_create(ParticleBatch* batch, ParticleSystem* particles,
                          u32* visibleIndices, u32 visibleCount);
void particle_batch_render(ParticleBatch* batch);

#endif // VFX_OPTIMIZATION_H
