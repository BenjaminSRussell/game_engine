/*
 * vfx_events.h
 * VFX Graph Event System
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_EVENTS_H
#define EFFECTS_VFX_EVENTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vfx_event_type {
    VFX_EVENT_SPAWN,
    VFX_EVENT_DEATH,
    VFX_EVENT_COLLISION,
    VFX_EVENT_CUSTOM
} vfx_event_type_t;

typedef struct vfx_event {
    vfx_event_type_t type;
    uint32_t particle_id;
    // ... payload
} vfx_event_t;

void vfx_events_broadcast(const vfx_event_t* event);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_EVENTS_H */
