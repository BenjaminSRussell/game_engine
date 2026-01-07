#include "environment/builder/building_placer.h"
#include <include/math/math.h>
#include <stdbool.h>

typedef struct {
    float pos[3];
    int socket_type; // 0=Floor, 1=WallTop, 2=WallSide
} Socket;

bool building_placer_snap(float *pos, float *rot, void *target_module) {
    // Find nearest compatible socket on target_module
    // If distance < SNAP_THRESHOLD:
    //   *pos = socket.pos;
    //   *rot = socket.rot;
    //   return true;
    return false;
}

void building_placer_generate_room(float width, float length, float height, void **out_modules) {
    // Place 4 corners
    // Fill walls between corners
    // Place floor tiles
    // Add ceiling tiles
}

void building_placer_variate_facade(void *building_group) {
    // Randomize modules
    // For each window_module:
    //   if (rand() > 0.5) replace_with(balcony_module);
    //   else replace_with(plain_wall_module);
}
