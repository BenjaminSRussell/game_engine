#ifndef EFFECTS_FOOTPRINT_SYSTEM_H
#define EFFECTS_FOOTPRINT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct effects_footprint_handle {
    uint32_t id;
} effects_footprint_handle_t;

typedef struct effects_footprint_desc {
    uint32_t flags;
} effects_footprint_desc_t;

int effects_footprint_init(void);
void effects_footprint_shutdown(void);
void effects_footprint_apply(float x, float z, uint32_t type);
void effects_footprint_update(float dt);

#ifdef __cplusplus
}
#endif

#endif // EFFECTS_FOOTPRINT_SYSTEM_H
