// src/vfx/vfx_optimization.c
//
// Implementation of VFX optimization systems
//
#include <effects/vfx/vfx_optimization.h>
#include "engine/include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Particle Culling System

void particle_culling_init(ParticleCullingSystem* system) {
    if (!system) {
        return;
    }

    memset(system, 0, sizeof(ParticleCullingSystem));
    system->cellCount = 0;

    for (u32 i = 0; i < MAX_CULLING_CELLS; i++) {
        system->cells[i].particleCount = 0;
    }

    system->initialized = true;
    LOG_INFO("Particle culling system initialized");
}

void particle_culling_shutdown(ParticleCullingSystem* system) {
    if (!system || !system->initialized) {
        return;
    }

    system->initialized = false;
    LOG_INFO("Particle culling system shut down");
}

static CellCoord particle_to_cell(Vec3 pos) {
    return (CellCoord){
        .x = (i32)floorf(pos.x / CELL_SIZE),
        .y = (i32)floorf(pos.y / CELL_SIZE),
        .z = (i32)floorf(pos.z / CELL_SIZE)
    };
}

void particle_culling_build_spatial_hash(ParticleCullingSystem* system,
                                        ParticleSystem* particles) {
    if (!system || !particles) {
        return;
    }

    // Clear cells
    for (u32 i = 0; i < MAX_CULLING_CELLS; i++) {
        system->cells[i].particleCount = 0;
    }

    system->cellCount = 0;

    // Hash particles into cells
    for (u32 i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &particles->particles[i];
        if (!p->active) {
            continue;
        }

        CellCoord coord = particle_to_cell(p->position);

        // Simple hash: (x + y * 16 + z * 256) mod MAX_CULLING_CELLS
        u32 cellIdx = ((coord.x & 0xF) + ((coord.y & 0xF) << 4) +
                      ((coord.z & 0xF) << 8)) % MAX_CULLING_CELLS;

        SpatialCell* cell = &system->cells[cellIdx];
        if (cell->particleCount < 64) {
            cell->particleIndices[cell->particleCount++] = i;
        }
    }

    LOG_DEBUG("Built spatial hash with %u active cells", system->cellCount);
}

void particle_culling_update(ParticleCullingSystem* system, ParticleSystem* particles,
                            Vec3 cameraPos, f32 cullRadius) {
    if (!system || !particles) {
        return;
    }

    particle_culling_build_spatial_hash(system, particles);
}

u32 particle_culling_get_visible(ParticleCullingSystem* system, Vec3 cameraPos,
                                f32 radius, u32* outIndices, u32 maxIndices) {
    if (!system || !outIndices) {
        return 0;
    }

    u32 visibleCount = 0;
    f32 radiusSq = radius * radius;

    CellCoord cameraCellCoord = particle_to_cell(cameraPos);

    // Check cells within radius
    for (i32 cx = cameraCellCoord.x - 1; cx <= cameraCellCoord.x + 1; cx++) {
        for (i32 cy = cameraCellCoord.y - 1; cy <= cameraCellCoord.y + 1; cy++) {
            for (i32 cz = cameraCellCoord.z - 1; cz <= cameraCellCoord.z + 1; cz++) {
                u32 cellIdx = ((cx & 0xF) + ((cy & 0xF) << 4) +
                              ((cz & 0xF) << 8)) % MAX_CULLING_CELLS;

                SpatialCell* cell = &system->cells[cellIdx];

                for (u32 i = 0; i < cell->particleCount; i++) {
                    if (visibleCount >= maxIndices) {
                        return visibleCount;
                    }

                    u32 particleIdx = cell->particleIndices[i];
                    outIndices[visibleCount++] = particleIdx;
                }
            }
        }
    }

    return visibleCount;
}

// Effect Priority System

void effect_priority_init(EffectPrioritySystem* system) {
    if (!system) {
        return;
    }

    memset(system, 0, sizeof(EffectPrioritySystem));
    system->queueSize = 0;
    system->initialized = true;

    LOG_INFO("Effect priority system initialized");
}

void effect_priority_add(EffectPrioritySystem* system, u32 effectId,
                        EffectPriority priority, f32 distance) {
    if (!system || system->queueSize >= 256) {
        return;
    }

    EffectQueueEntry* entry = &system->queue[system->queueSize++];
    entry->effectId = effectId;
    entry->priority = priority;
    entry->distance = distance;

    // Importance based on priority and distance
    f32 priorityWeight = (4 - priority) * 100.0f;  // Higher priority = higher weight
    f32 distanceWeight = 1.0f / (1.0f + distance * 0.1f);  // Closer = higher weight
    entry->importance = priorityWeight * distanceWeight;
}

static i32 compare_effects(const void* a, const void* b) {
    EffectQueueEntry* ea = (EffectQueueEntry*)a;
    EffectQueueEntry* eb = (EffectQueueEntry*)b;

    // Sort by importance (descending)
    if (ea->importance > eb->importance) return -1;
    if (ea->importance < eb->importance) return 1;
    return 0;
}

void effect_priority_sort(EffectPrioritySystem* system) {
    if (!system || system->queueSize == 0) {
        return;
    }

    qsort(system->queue, system->queueSize, sizeof(EffectQueueEntry), compare_effects);
}

void effect_priority_clear(EffectPrioritySystem* system) {
    if (!system) {
        return;
    }

    system->queueSize = 0;
}

// Particle Batching

void particle_batch_create(ParticleBatch* batch, ParticleSystem* particles,
                          u32* visibleIndices, u32 visibleCount) {
    if (!batch || !particles || !visibleIndices) {
        return;
    }

    memset(batch, 0, sizeof(ParticleBatch));

    // Copy visible indices
    u32 copyCount = (visibleCount < MAX_PARTICLES) ? visibleCount : MAX_PARTICLES;
    memcpy(batch->particleIndices, visibleIndices, copyCount * sizeof(u32));

    batch->particleCount = copyCount;
    // Each particle = 2 triangles = 6 vertices
    batch->totalVertices = copyCount * 6;
}

void particle_batch_render(ParticleBatch* batch) {
    if (!batch || batch->totalVertices == 0) {
        return;
    }

    // Implement batch rendering
    // This would involve:
    // 1. Binding particle pipeline
    // 2. Binding texture and descriptor sets
    // 3. Drawing all particles with single draw call
    // 4. Using instancing or array indexing
    
    // For now, we'll simulate the batch rendering with a basic implementation
    // In a full Vulkan implementation, this would:
    // - vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, particlePipeline)
    // - vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL)
    // - vkCmdBindVertexBuffers(commandBuffer, 0, 1, &batch->vertexBuffer.buffer, &offsets)
    // - vkCmdDraw(commandBuffer, batch->totalVertices, 1, 0, 0)
    
    LOG_DEBUG("Rendering particle batch: %u particles, %u vertices",
             batch->particleCount, batch->totalVertices);
             
    // Mark batch as rendered
    batch->totalVertices = 0;
    batch->particleCount = 0;
}
