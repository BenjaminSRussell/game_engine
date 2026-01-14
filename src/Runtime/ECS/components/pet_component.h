#ifndef PET_COMPONENT_H
#define PET_COMPONENT_H

#include "src/engine/ecs/ecs.h"

typedef struct {
    EntityId owner;
    char pet_name[50];
    // Add other pet properties here
} PetComponent;

#endif // PET_COMPONENT_H
