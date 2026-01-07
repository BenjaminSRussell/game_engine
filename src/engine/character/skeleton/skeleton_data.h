#ifndef SKELETON_DATA_H
#define SKELETON_DATA_H

#include <simd/simd.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_BONES 256
#define MAX_BONE_INFLUENCES 4

typedef struct bone {
    char name[64];
    int32_t parent_index;      // -1 for root
    simd_float4x4 local_bind;  // Local bind pose
    simd_float4x4 inv_bind;    // Inverse bind matrix
} bone_t;

typedef struct skeleton {
    bone_t* bones;
    uint32_t bone_count;
    uint32_t root_bone;
} skeleton_t;

skeleton_t* skeleton_create(const bone_t* bones, uint32_t count);
void skeleton_destroy(skeleton_t* skeleton);

#endif // SKELETON_DATA_H
