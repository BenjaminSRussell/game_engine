#ifndef SELECTION_SYSTEM_H
#define SELECTION_SYSTEM_H

#include "../common.h"
#include "../ecs/ecs.h"
#include "../math/vec3.h"

// ----------------------------------------------------------------------------
// Types
// ----------------------------------------------------------------------------

typedef struct {
  Vec3 origin;
  Vec3 direction;
  float max_distance;
} SelectionRay;

typedef struct {
  EntityID selected_entity;
  bool has_selection;
  Vec3 intersection_point;
  float intersection_distance;
} SelectionState;

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

void selection_init(World *world);
void selection_shutdown(void);

// Cast a ray into the scene and select the closest entity
// Returns true if an entity was hit
bool selection_raycast(SelectionRay ray);

// Get current selection state
SelectionState selection_get_state(void);

// Clear selection
void selection_clear(void);

// Helper to generate ray from screen coordinates
SelectionRay selection_ray_from_camera(Vec3 cam_pos, Vec3 cam_fwd, Vec3 cam_up,
                                       float fov, float aspect, float mouse_x,
                                       float mouse_y);

#endif // SELECTION_SYSTEM_H
