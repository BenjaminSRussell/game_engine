#ifndef NETWORKING_OPTIMIZATION_INTEREST_MANAGEMENT_H
#define NETWORKING_OPTIMIZATION_INTEREST_MANAGEMENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct InterestManager InterestManager;

typedef struct InterestEntity {
  uint32_t id;
  float position[3];
  float radius;
  uint32_t room_id;
  bool persistent;
  float base_frequency_hz;
} InterestEntity;

typedef struct InterestClient {
  uint32_t id;
  float position[3];
  float view_distance;
  float frustum_planes[6][4];
  uint32_t room_id;
} InterestClient;

typedef bool (*InterestPVSCallback)(uint32_t entity_id, uint32_t client_id,
                                    void *user);

InterestManager *interest_manager_init(uint32_t entity_capacity,
                                       uint32_t client_capacity,
                                       float default_view_distance);
void interest_manager_shutdown(InterestManager *manager);

bool interest_manager_add_entity(InterestManager *manager,
                                 const InterestEntity *entity);
bool interest_manager_add_client(InterestManager *manager,
                                 const InterestClient *client);

void interest_manager_set_pvs_callback(InterestManager *manager,
                                       InterestPVSCallback callback,
                                       void *user);
void interest_manager_enable_room_isolation(InterestManager *manager,
                                            bool enabled);
void interest_manager_enable_frustum_culling(InterestManager *manager,
                                             bool enabled);

bool interest_manager_is_relevant(const InterestManager *manager,
                                  const InterestClient *client,
                                  const InterestEntity *entity);

float interest_manager_lod_frequency(const InterestManager *manager,
                                     const InterestClient *client,
                                     const InterestEntity *entity);

bool interest_manager_visualize_grid(const InterestManager *manager,
                                     const char *file_path,
                                     float cell_size);

#ifdef __cplusplus
}
#endif

#endif
