#include "Components_Registration.h"
#include "Audio_Component.h"
#include "Particle_Component.h"
#include "Physics_Component.h"
#include "Transform_Component.h"

ComponentType COMPONENT_TRANSFORM = 0;
ComponentType COMPONENT_PHYSICS = 0;
ComponentType COMPONENT_PARTICLE = 0;
ComponentType COMPONENT_AUDIO_SOURCE = 0;
ComponentType COMPONENT_AUDIO_LISTENER = 0;

void Components_RegisterAll(void) {
  COMPONENT_TRANSFORM = ECS_RegisterComponent(
      "Transform", sizeof(TransformComponent), _Alignof(TransformComponent));
  COMPONENT_PHYSICS = ECS_RegisterComponent("Physics", sizeof(PhysicsComponent),
                                            _Alignof(PhysicsComponent));
  COMPONENT_PARTICLE = ECS_RegisterComponent(
      "Particle", sizeof(ParticleComponent), _Alignof(ParticleComponent));
  COMPONENT_AUDIO_SOURCE =
      ECS_RegisterComponent("AudioSource", sizeof(AudioSourceComponent),
                            _Alignof(AudioSourceComponent));
  COMPONENT_AUDIO_LISTENER =
      ECS_RegisterComponent("AudioListener", sizeof(AudioListenerComponent),
                            _Alignof(AudioListenerComponent));
}
