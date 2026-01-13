#ifndef PCG_COMPONENT_H
#define PCG_COMPONENT_H

#include "../../common.h"
#include "../../math/vec3.h"
#include "../../pcg/pcg_graph.h"
#include <stdbool.h>

typedef struct {
  u32 graph_id;  // ID of the graph to execute (0 = invalid)
  int seed;      // Seed override
  bool is_dirty; // If true, system will re-execute graph

  // Volume Generative Bounds (local to entity)
  Vec3 bounds_extent;
} PCGComponent;

// Registers the component with the ECS
void pcg_component_register(void);

#endif // PCG_COMPONENT_H
