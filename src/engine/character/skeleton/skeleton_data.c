#include "character/skeleton/skeleton_data.h"
#include <stdlib.h>
#include <string.h>

skeleton_t* skeleton_create(const bone_t* bones, uint32_t count) {
    if (!bones || count == 0 || count > MAX_BONES) {
        return NULL;
    }

    skeleton_t* skeleton = (skeleton_t*)malloc(sizeof(skeleton_t));
    if (!skeleton) {
        return NULL;
    }

    skeleton->bones = (bone_t*)malloc(sizeof(bone_t) * count);
    if (!skeleton->bones) {
        free(skeleton);
        return NULL;
    }

    memcpy(skeleton->bones, bones, sizeof(bone_t) * count);
    skeleton->bone_count = count;

    // Find root bone (first bone with parent_index == -1)
    skeleton->root_bone = 0; // Default to 0
    for (uint32_t i = 0; i < count; ++i) {
        if (skeleton->bones[i].parent_index == -1) {
            skeleton->root_bone = i;
            break;
        }
    }

    return skeleton;
}

void skeleton_destroy(skeleton_t* skeleton) {
    if (skeleton) {
        if (skeleton->bones) {
            free(skeleton->bones);
        }
        free(skeleton);
    }
}
