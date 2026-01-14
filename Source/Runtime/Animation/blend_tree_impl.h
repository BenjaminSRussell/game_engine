#ifndef BLEND_TREE_IMPL_H
#define BLEND_TREE_IMPL_H

#include <stdbool.h>

// Forward declaration
typedef struct AnimState AnimState;

// Function prototypes
void anim_eval_state(AnimState *state, void *output_pose);
void anim_update_machine(AnimState *machine, float dt);

#endif // BLEND_TREE_IMPL_H
