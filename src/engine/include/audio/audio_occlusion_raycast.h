#ifndef AUDIO_OCCLUSION_RAYCAST_H
#define AUDIO_OCCLUSION_RAYCAST_H

#ifdef __cplusplus
extern "C" {
#endif

struct AudioSystem;
typedef struct AudioSystem AudioSystem;
struct BlockPhysicsSystem;

#include <audio/audio_engine_types.h>
#include <physics/block_physics.h>

void Audio_UpdateOcclusion(AudioSystem *sys, BlockPhysicsSystem *bp);
void audio_occlusion_raycast_init(void);
void audio_occlusion_raycast_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_OCCLUSION_RAYCAST_H
