#ifndef DESTRUCTION_IMPL_H
#define DESTRUCTION_IMPL_H

#include <stdbool.h>
#include <stdint.h>

void destruction_sys_set_enabled(bool enabled);
bool destruction_sys_is_enabled(void);

void destruction_sys_set_debris_lifetime(float lifetime);
float destruction_sys_get_debris_lifetime(void);

// Triggers the physical destruction of an entity
void destruction_sys_trigger_break(uint64_t entity_id, float x, float y,
                                   float z, float force);

#endif // DESTRUCTION_IMPL_H
