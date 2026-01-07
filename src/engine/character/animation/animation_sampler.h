#ifndef ANIMATION_SAMPLER_H
#define ANIMATION_SAMPLER_H

#include "animation_clip.h"
#include <simd/simd.h>

void animation_sample(const animation_clip_t* clip, float time,
                      simd_float4x4* bone_transforms, uint32_t bone_count);

#endif // ANIMATION_SAMPLER_H
