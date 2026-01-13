#ifndef FALLING_BLOCK_COMPONENT_H
#define FALLING_BLOCK_COMPONENT_H

#include <block/block.h>
#include <stdbool.h>

typedef struct {
  BlockID block_type;
  float fall_distance;
  bool on_ground;
} FallingBlockComponent;

#endif // FALLING_BLOCK_COMPONENT_H
