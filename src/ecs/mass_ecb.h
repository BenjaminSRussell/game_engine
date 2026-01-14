#ifndef MASS_ECB_H
#define MASS_ECB_H

#include <common.h>
#include <ecs/ecs.h>

typedef struct MassCommandBuffer MassCommandBuffer;

// Create/Destroy
MassCommandBuffer *mass_ecb_create(World *world);
void mass_ecb_destroy(MassCommandBuffer *buffer);

// Command Recording
// Thread-safe if each thread has its own ECB, or if internal locking (we'll
// assume 1 ECB per thread for now)
Entity mass_ecb_create_entity(MassCommandBuffer *buffer);
void mass_ecb_destroy_entity(MassCommandBuffer *buffer, Entity entity);
void mass_ecb_add_component(MassCommandBuffer *buffer, Entity entity,
                            ECSComponentID component_id, const void *data);
void mass_ecb_remove_component(MassCommandBuffer *buffer, Entity entity,
                               ECSComponentID component_id);
void mass_ecb_set_component(MassCommandBuffer *buffer, Entity entity,
                            ECSComponentID component_id, const void *data);

// Playback (Must be called from main thread)
void mass_ecb_playback(MassCommandBuffer *buffer);
void mass_ecb_clear(MassCommandBuffer *buffer);

#endif // MASS_ECB_H
