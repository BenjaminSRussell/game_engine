#include "procedural/dungeon/room_generator.h"
#include <stdlib.h>

void room_generator_init() {}

void room_generator_create_rectangular(int width, int height, void *output) {
    // Generate rectangular room
}

void room_generator_create_circular(float radius, void *output) {}

void room_generator_add_door(void *room, int x, int y) {}

void room_generator_populate(void *room, void *spawn_table) {
    // Place enemies, items, etc.
}
