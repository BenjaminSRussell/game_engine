#ifndef ULTIMATE_ENGINE_COMPONENTS_REGISTRATION_H
#define ULTIMATE_ENGINE_COMPONENTS_REGISTRATION_H

#include "../Public/ECS.h"

// Global Component IDs
extern ComponentType COMPONENT_TRANSFORM;
extern ComponentType COMPONENT_PHYSICS;
extern ComponentType COMPONENT_PARTICLE;
extern ComponentType COMPONENT_AUDIO_SOURCE;
extern ComponentType COMPONENT_AUDIO_LISTENER;

// Registration Helper
void Components_RegisterAll(void);

#endif // ULTIMATE_ENGINE_COMPONENTS_REGISTRATION_H
