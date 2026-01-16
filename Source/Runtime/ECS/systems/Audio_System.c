#include "../../Audio/Public/Audio.h"
#include "../../Audio/Public/AudioHelper.h"
#include "../Public/ECS.h"
#include "../components/Audio_Component.h"
#include "../components/Components_Registration.h"
#include "../components/Physics_Component.h" // For velocity
#include "../components/Transform_Component.h"
#include <math.h>
#include <stdio.h>

#if defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

void AudioSystem_Update(float dt) {
  (void)dt;

  // 1. Update Listener Position (Camera)
  for (EntityID id = 0; id < ECS_MAX_ENTITIES; ++id) {
    if (!ECS_IsEntityValid(id))
      continue;
    if (!ECS_HasComponent(id, COMPONENT_AUDIO_LISTENER))
      continue;

    TransformComponent *t =
        (TransformComponent *)ECS_GetComponent(id, COMPONENT_TRANSFORM);
    if (t) {
      ALfloat pos[] = {t->position.x, t->position.y, t->position.z};
      alListenerfv(AL_POSITION, pos);

      // Basic orientation (assuming camera looks roughly forward for now)
      // Ideally we extract forward/up vectors from rotation quaternion/euler
      // For now, let's just leave orientation default or implement simple yaw
      float yaw = t->rotation.y;
      float look_x = sinf(yaw);  // approximate
      float look_z = -cosf(yaw); // approximate

      ALfloat ori[] = {look_x, 0, look_z, 0, 1, 0};
      alListenerfv(AL_ORIENTATION, ori);
    }
  }

  // 2. Update Audio Sources
  for (EntityID id = 0; id < ECS_MAX_ENTITIES; ++id) {
    if (!ECS_IsEntityValid(id))
      continue;
    if (!ECS_HasComponent(id, COMPONENT_AUDIO_SOURCE))
      continue;

    AudioSourceComponent *audio =
        (AudioSourceComponent *)ECS_GetComponent(id, COMPONENT_AUDIO_SOURCE);
    TransformComponent *t =
        (TransformComponent *)ECS_GetComponent(id, COMPONENT_TRANSFORM);
    PhysicsComponent *p =
        (PhysicsComponent *)ECS_GetComponent(id, COMPONENT_PHYSICS);

    // Initialize source if needed
    if (audio->source_id == 0) {
      ALuint source;
      alGenSources(1, &source);
      audio->source_id = (int)source;
      alSourcei(source, AL_BUFFER, audio->buffer_id);
      alSourcef(source, AL_PITCH, audio->pitch > 0 ? audio->pitch : 1.0f);
      alSourcef(source, AL_GAIN, audio->gain > 0 ? audio->gain : 1.0f);
      alSourcei(source, AL_LOOPING, audio->loop ? AL_TRUE : AL_FALSE);

      if (audio->is_3d) {
        alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);
        alSourcef(source, AL_REFERENCE_DISTANCE, 5.0f);
        alSourcef(source, AL_MAX_DISTANCE, 50.0f);
      } else {
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        ALfloat zero[] = {0, 0, 0};
        alSourcefv(source, AL_POSITION, zero);
      }

      if (audio->play_on_start) {
        alSourcePlay(source);
        audio->is_playing = true;
      }
    }

    // Update 3D properties
    if (audio->is_3d && t) {
      ALfloat pos[] = {t->position.x, t->position.y, t->position.z};
      alSourcefv((ALuint)audio->source_id, AL_POSITION, pos);

      if (p) {
        ALfloat vel[] = {p->velocity.x, p->velocity.y, p->velocity.z};
        alSourcefv((ALuint)audio->source_id, AL_VELOCITY, vel);
      }
    }
  }
}

void AudioSystem_Register(void) {
  ECS_RegisterSystem("AudioSystem", AudioSystem_Update, NULL,
                     50); // Run before physics? or after?
}
