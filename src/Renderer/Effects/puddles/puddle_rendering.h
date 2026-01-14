#ifndef EFFECTS_PUDDLE_RENDERING_H
#define EFFECTS_PUDDLE_RENDERING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct effects_puddle_handle {
    uint32_t id;
} effects_puddle_handle_t;

typedef struct effects_puddle_desc {
    uint32_t flags;
} effects_puddle_desc_t;

int effects_puddle_init(void);
void effects_puddle_shutdown(void);
int effects_puddle_create(effects_puddle_handle_t* out_handle, const effects_puddle_desc_t* desc);
void effects_puddle_update(effects_puddle_handle_t handle, float dt);
void effects_puddle_render(void);

#ifdef __cplusplus
}
#endif

#endif // EFFECTS_PUDDLE_RENDERING_H
