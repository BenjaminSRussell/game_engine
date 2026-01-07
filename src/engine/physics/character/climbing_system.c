#include "physics/character/climbing_system.h"

void climbing_init() {}

int climbing_can_grab(void *character, void *surface) {
    return 1;
}

void climbing_grab(void *character, void *surface) {}

void climbing_release(void *character) {}

void climbing_move(void *character, float direction[2]) {}
