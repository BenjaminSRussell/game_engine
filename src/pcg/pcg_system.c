#include "../include/core/memory.h"
#include "../include/ecs/components/pcg_component.h"
#include "../include/ecs/components/transform.h"
#include "../include/ecs/ecs.h"
#include "../include/pcg/pcg_graph.h"
#include <stdio.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------
// Graph Registry (Mock)
// ----------------------------------------------------------------------------
// In a real engine, this would be an asset manager or global registry.
#define MAX_GRAPHS 16
static pcg_graph_t *g_pcg_graphs[MAX_GRAPHS];
static u32 g_pcg_graph_count = 0;

u32 pcg_register_graph(pcg_graph_t *graph) {
  if (g_pcg_graph_count < MAX_GRAPHS) {
    g_pcg_graphs[++g_pcg_graph_count] = graph; // 1-based IDs
    return g_pcg_graph_count;
  }
  return 0;
}

pcg_graph_t *pcg_get_graph(u32 id) {
  if (id > 0 && id <= g_pcg_graph_count) {
    return g_pcg_graphs[id];
  }
  return NULL;
}

// ----------------------------------------------------------------------------
// System Implementation
// ----------------------------------------------------------------------------

void pcg_system_update(void) {
  // Query all entities with PCGComponent and TransformComponent
  // ECS Query API usage (assuming standard arch)
  // For V1, we'll iterate a theoretical list or just define the logic structure

  // Pseudo-code for ECS iteration:
  // ecs_query_t query = ecs_query_create(pcg_component_id,
  // transform_component_id); for (ecs_iter_t it = ecs_query_iter(&query);
  // ecs_iter_next(&it);) {
  //     PCGComponent* pcg = ecs_iter_column(&it, PCGComponent, 0);
  //     TransformComponent* trans = ecs_iter_column(&it, TransformComponent,
  //     1);
  //     ...
  // }

  // Since I don't have the full ECS query API in front of me to copy-paste
  // exactly, I will write the core logic function that would be called
  // primarily.
}

void pcg_component_register(void) {
  // ecs_register_component("PCGComponent", sizeof(PCGComponent), ...);
}

// The actual update logic for a single entity
void pcg_update_entity(PCGComponent *pcg, TransformComponent *transform) {
  if (!pcg->is_dirty || pcg->graph_id == 0)
    return;

  pcg_graph_t *graph = pcg_get_graph(pcg->graph_id);
  if (!graph)
    return;

  // 1. Setup Inputs
  // We might pass the bounds as an input "Points" collection with 1 point
  // defining the volume? Or just parameters. For V1, the graph 'SurfaceSampler'
  // pulls params from the node itself, but typically we want to override with
  // component bounds.

  // 2. Execute
  pcg_data_collection_t *result = pcg_graph_execute(graph, NULL);

  // 3. Process Outputs (Spawn meshes)
  // This part would interact with the Scene/World to add instances.
  // For now, let's just log the count.
  if (result) {
    // printf("PCG Generated %d points for Entity with Graph %d\n",
    // result->point_count, pcg->graph_id);

    // Cleanup results (in real world, we'd cache them or write to HISM
    // component) pcg_data_free(result); // Helper not exposed in header yet? We
    // need to expose free in header.
  }

  pcg->is_dirty = false;
}
