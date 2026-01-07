#include "network/optimization/interest_management.h"

#include <include/math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                          INTEREST MANAGEMENT / RELEVANCY - AGENT_NET_5
 * =================================================================================================
 *
 * PURPOSE: Determine which entities are relevant to a client (scoping).
 * =================================================================================================
 */

struct InterestManager {
  InterestEntity *entities;
  uint32_t entity_count;
  uint32_t entity_capacity;
  InterestClient *clients;
  uint32_t client_count;
  uint32_t client_capacity;
  float default_view_distance;
  bool room_isolation;
  bool frustum_culling;
  InterestPVSCallback pvs_callback;
  void *pvs_user;
};

static float interest_distance_sq(const float a[3], const float b[3]) {
  float dx = a[0] - b[0];
  float dy = a[1] - b[1];
  float dz = a[2] - b[2];
  return dx * dx + dy * dy + dz * dz;
}

static bool interest_sphere_in_frustum(const float planes[6][4],
                                       const float position[3], float radius) {
  for (int i = 0; i < 6; ++i) {
    const float *p = planes[i];
    float distance = p[0] * position[0] + p[1] * position[1] +
                     p[2] * position[2] + p[3];
    if (distance < -radius) {
      return false;
    }
  }
  return true;
}

static bool interest_upsert_entity(InterestManager *manager,
                                   const InterestEntity *entity) {
  for (uint32_t i = 0; i < manager->entity_count; ++i) {
    if (manager->entities[i].id == entity->id) {
      manager->entities[i] = *entity;
      return true;
    }
  }
  if (manager->entity_count >= manager->entity_capacity) {
    return false;
  }
  manager->entities[manager->entity_count++] = *entity;
  return true;
}

static bool interest_upsert_client(InterestManager *manager,
                                   const InterestClient *client) {
  for (uint32_t i = 0; i < manager->client_count; ++i) {
    if (manager->clients[i].id == client->id) {
      manager->clients[i] = *client;
      return true;
    }
  }
  if (manager->client_count >= manager->client_capacity) {
    return false;
  }
  manager->clients[manager->client_count++] = *client;
  return true;
}

InterestManager *interest_manager_init(uint32_t entity_capacity,
                                       uint32_t client_capacity,
                                       float default_view_distance) {
  InterestManager *manager =
      (InterestManager *)calloc(1, sizeof(InterestManager));
  if (!manager) {
    return NULL;
  }

  if (entity_capacity == 0) {
    entity_capacity = 128;
  }
  if (client_capacity == 0) {
    client_capacity = 32;
  }

  manager->entities =
      (InterestEntity *)calloc(entity_capacity, sizeof(InterestEntity));
  manager->clients =
      (InterestClient *)calloc(client_capacity, sizeof(InterestClient));
  if (!manager->entities || !manager->clients) {
    free(manager->entities);
    free(manager->clients);
    free(manager);
    return NULL;
  }

  manager->entity_capacity = entity_capacity;
  manager->client_capacity = client_capacity;
  manager->default_view_distance =
      default_view_distance > 0.0f ? default_view_distance : 100.0f;
  manager->room_isolation = false;
  manager->frustum_culling = false;
  manager->pvs_callback = NULL;
  manager->pvs_user = NULL;
  return manager;
}

void interest_manager_shutdown(InterestManager *manager) {
  if (!manager) {
    return;
  }
  free(manager->entities);
  free(manager->clients);
  free(manager);
}

bool interest_manager_add_entity(InterestManager *manager,
                                 const InterestEntity *entity) {
  if (!manager || !entity) {
    return false;
  }
  return interest_upsert_entity(manager, entity);
}

bool interest_manager_add_client(InterestManager *manager,
                                 const InterestClient *client) {
  if (!manager || !client) {
    return false;
  }
  return interest_upsert_client(manager, client);
}

void interest_manager_set_pvs_callback(InterestManager *manager,
                                       InterestPVSCallback callback,
                                       void *user) {
  if (!manager) {
    return;
  }
  manager->pvs_callback = callback;
  manager->pvs_user = user;
}

void interest_manager_enable_room_isolation(InterestManager *manager,
                                            bool enabled) {
  if (manager) {
    manager->room_isolation = enabled;
  }
}

void interest_manager_enable_frustum_culling(InterestManager *manager,
                                             bool enabled) {
  if (manager) {
    manager->frustum_culling = enabled;
  }
}

bool interest_manager_is_relevant(const InterestManager *manager,
                                  const InterestClient *client,
                                  const InterestEntity *entity) {
  if (!manager || !client || !entity) {
    return false;
  }

  if (entity->persistent) {
    return true;
  }

  if (manager->room_isolation && client->room_id != entity->room_id) {
    return false;
  }

  float view_distance =
      client->view_distance > 0.0f ? client->view_distance
                                   : manager->default_view_distance;
  float dist_sq = interest_distance_sq(client->position, entity->position);
  if (dist_sq > view_distance * view_distance) {
    return false;
  }

  if (manager->frustum_culling) {
    if (!interest_sphere_in_frustum(client->frustum_planes, entity->position,
                                    entity->radius)) {
      return false;
    }
  }

  if (manager->pvs_callback) {
    if (!manager->pvs_callback(entity->id, client->id, manager->pvs_user)) {
      return false;
    }
  }

  return true;
}

float interest_manager_lod_frequency(const InterestManager *manager,
                                     const InterestClient *client,
                                     const InterestEntity *entity) {
  if (!manager || !client || !entity) {
    return 0.0f;
  }

  float view_distance =
      client->view_distance > 0.0f ? client->view_distance
                                   : manager->default_view_distance;
  float dist_sq = interest_distance_sq(client->position, entity->position);
  float distance = sqrtf(dist_sq);
  float t = distance / view_distance;
  if (t > 1.0f) {
    t = 1.0f;
  }

  float base = entity->base_frequency_hz > 0.0f ? entity->base_frequency_hz
                                                : 20.0f;
  float scaled = base * (1.0f - 0.75f * t);
  if (scaled < 1.0f) {
    scaled = 1.0f;
  }
  return scaled;
}

bool interest_manager_visualize_grid(const InterestManager *manager,
                                     const char *file_path,
                                     float cell_size) {
  if (!manager || !file_path) {
    return false;
  }
  if (cell_size <= 0.0f) {
    cell_size = 1.0f;
  }

  FILE *file = fopen(file_path, "w");
  if (!file) {
    return false;
  }

  fprintf(file, "type,id,x,y,z,cell_x,cell_y,cell_z\n");
  for (uint32_t i = 0; i < manager->entity_count; ++i) {
    const InterestEntity *entity = &manager->entities[i];
    int cx = (int)floorf(entity->position[0] / cell_size);
    int cy = (int)floorf(entity->position[1] / cell_size);
    int cz = (int)floorf(entity->position[2] / cell_size);
    fprintf(file, "E,%u,%.3f,%.3f,%.3f,%d,%d,%d\n", entity->id,
            entity->position[0], entity->position[1], entity->position[2], cx,
            cy, cz);
  }

  for (uint32_t i = 0; i < manager->client_count; ++i) {
    const InterestClient *client = &manager->clients[i];
    int cx = (int)floorf(client->position[0] / cell_size);
    int cy = (int)floorf(client->position[1] / cell_size);
    int cz = (int)floorf(client->position[2] / cell_size);
    fprintf(file, "C,%u,%.3f,%.3f,%.3f,%d,%d,%d\n", client->id,
            client->position[0], client->position[1], client->position[2], cx,
            cy, cz);
  }

  fclose(file);
  return true;
}
