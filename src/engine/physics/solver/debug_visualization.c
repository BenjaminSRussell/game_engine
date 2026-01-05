#include <core/logger.h>
#include <core/memory.h>
#include <ecs/ecs.h>
#include <math/math.h>
// #include <physics/continuous_collision.h> // Commented out due to EntityID
// type mismatch
#include <physics/debug_visualization.h>
#include <physics/physics.h>
#include <physics/soft_body.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/utils.h"

// Global debug visualization instance
static PhysicsDebugVisualization g_physics_debug_vis = {0};

// Forward declarations
void physics_debug_draw_box_internal(PhysicsDebugVisualization *debug_vis,
                                     Vec3 center, Vec3 half_extents,
                                     Quat rotation, uint32_t color,
                                     bool wireframe);
void physics_debug_draw_capsule_internal(PhysicsDebugVisualization *debug_vis,
                                         Vec3 center, float radius,
                                         float height, Quat rotation,
                                         uint32_t color, bool wireframe,
                                         int segments);
void physics_debug_draw_sphere_wireframe(PhysicsDebugVisualization *debug_vis,
                                         Vec3 center, float radius,
                                         uint32_t color, int segments);

// Default colors for different physics elements
static const uint32_t DEFAULT_COLORS[DEBUG_COLOR_COUNT] = {
    0xFF808080, // Static - Gray
    0xFF00FF00, // Dynamic - Green
    0xFF0000FF, // Kinematic - Blue
    0xFFFFFF00, // Sleeping - Yellow
    0xFFFF00FF, // Trigger - Magenta
    0xFFFF0000, // Contact - Red
    0xFF00FFFF, // Constraint - Cyan
    0xFF800080, // Force - Purple
    0xFF0080FF, // Velocity - Light Blue
    0xFF00FF00, // Raycast Hit - Bright Green
    0xFF808080, // Raycast Miss - Dark Gray
    0xFF808000, // Broadphase - Orange
    0xFFFF80FF, // Soft Body - Pink
};

bool physics_debug_visualization_init(PhysicsDebugVisualization *debug_vis,
                                      uint32_t max_geometries) {
  if (!debug_vis || max_geometries == 0)
    return false;

  memset(debug_vis, 0, sizeof(PhysicsDebugVisualization));

  // Allocate geometry buffers
  uint32_t capacity = max_geometries;

  debug_vis->lines = (DebugLine *)malloc(capacity * sizeof(DebugLine));
  debug_vis->points = (DebugPoint *)malloc(capacity * sizeof(DebugPoint));
  debug_vis->triangles =
      (DebugTriangle *)malloc(capacity * sizeof(DebugTriangle));
  debug_vis->texts = (DebugText *)malloc(capacity * sizeof(DebugText));
  debug_vis->spheres = (DebugSphere *)malloc(capacity * sizeof(DebugSphere));
  debug_vis->boxes = (DebugBox *)malloc(capacity * sizeof(DebugBox));
  debug_vis->capsules = (DebugCapsule *)malloc(capacity * sizeof(DebugCapsule));

  if (!debug_vis->lines || !debug_vis->points || !debug_vis->triangles ||
      !debug_vis->texts || !debug_vis->spheres || !debug_vis->boxes ||
      !debug_vis->capsules) {
    physics_debug_visualization_cleanup(debug_vis);
    return false;
  }

  debug_vis->line_capacity = capacity;
  debug_vis->point_capacity = capacity;
  debug_vis->triangle_capacity = capacity;
  debug_vis->text_capacity = capacity;
  debug_vis->sphere_capacity = capacity;
  debug_vis->box_capacity = capacity;
  debug_vis->capsule_capacity = capacity;

  // Set default settings
  debug_vis->enabled = true;
  debug_vis->enabled_flags = DEBUG_VIS_ALL;
  debug_vis->scale = 1.0f;
  debug_vis->line_thickness = 1.0f;
  debug_vis->point_size = 3.0f;
  debug_vis->show_labels = true;
  debug_vis->fade_over_time = true;

  // Copy default colors
  memcpy(debug_vis->colors, DEFAULT_COLORS, sizeof(DEFAULT_COLORS));

  LOG_INFO(
      "Physics debug visualization initialized with capacity for %u geometries",
      max_geometries);
  return true;
}

void physics_debug_visualization_cleanup(PhysicsDebugVisualization *debug_vis) {
  if (!debug_vis)
    return;

  if (debug_vis->lines)
    free(debug_vis->lines);
  if (debug_vis->points)
    free(debug_vis->points);
  if (debug_vis->triangles)
    free(debug_vis->triangles);
  if (debug_vis->texts)
    free(debug_vis->texts);
  if (debug_vis->spheres)
    free(debug_vis->spheres);
  if (debug_vis->boxes)
    free(debug_vis->boxes);
  if (debug_vis->capsules)
    free(debug_vis->capsules);

  memset(debug_vis, 0, sizeof(PhysicsDebugVisualization));
  LOG_INFO("Physics debug visualization cleaned up");
}

void physics_debug_visualization_update(PhysicsDebugVisualization *debug_vis,
                                        float delta_time) {
  if (!debug_vis || !debug_vis->fade_over_time)
    return;

  float start_time = get_time();

  // Update temporary geometries with lifetime
// Helper macro for updating lifetime
#define UPDATE_LIFETIME(geometries, count, delta_time)                         \
  do {                                                                         \
    for (uint32_t i = 0; i < *(count); i++) {                                  \
      if (!geometries[i].persistent && geometries[i].lifetime > 0.0f) {        \
        geometries[i].lifetime -= delta_time;                                  \
        if (geometries[i].lifetime <= 0.0f) {                                  \
          for (uint32_t j = i; j < *(count) - 1; j++) {                        \
            geometries[j] = geometries[j + 1];                                 \
          }                                                                    \
          (*count)--;                                                          \
          i--;                                                                 \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  } while (0)

  UPDATE_LIFETIME(debug_vis->lines, &debug_vis->line_count, delta_time);
  UPDATE_LIFETIME(debug_vis->points, &debug_vis->point_count, delta_time);
  UPDATE_LIFETIME(debug_vis->triangles, &debug_vis->triangle_count, delta_time);
  UPDATE_LIFETIME(debug_vis->texts, &debug_vis->text_count, delta_time);
  UPDATE_LIFETIME(debug_vis->spheres, &debug_vis->sphere_count, delta_time);
  UPDATE_LIFETIME(debug_vis->boxes, &debug_vis->box_count, delta_time);
  UPDATE_LIFETIME(debug_vis->capsules, &debug_vis->capsule_count, delta_time);

  debug_vis->stats.render_time = get_time() - start_time;
}

void physics_debug_visualization_render(PhysicsDebugVisualization *debug_vis,
                                        struct IRenderer *renderer,
                                        struct Camera *camera) {
  if (!debug_vis || !debug_vis->enabled || !renderer || !camera)
    return;

  float start_time = get_time();
  debug_vis->stats.total_draw_calls = 0;
  debug_vis->stats.lines_drawn = 0;
  debug_vis->stats.points_drawn = 0;
  debug_vis->stats.triangles_drawn = 0;
  debug_vis->stats.texts_drawn = 0;
  debug_vis->stats.spheres_drawn = 0;
  debug_vis->stats.boxes_drawn = 0;
  debug_vis->stats.capsules_drawn = 0;

  // Render lines
  for (uint32_t i = 0; i < debug_vis->line_count; i++) {
    const DebugLine *line = &debug_vis->lines[i];
    // In a real implementation, this would call renderer functions
    // renderer_draw_line(renderer, line->start, line->end, line->color,
    // line->thickness);
    debug_vis->stats.lines_drawn++;
    debug_vis->stats.total_draw_calls++;
  }

  // Render points
  for (uint32_t i = 0; i < debug_vis->point_count; i++) {
    const DebugPoint *point = &debug_vis->points[i];
    // renderer_draw_point(renderer, point->position, point->color,
    // point->size);
    debug_vis->stats.points_drawn++;
    debug_vis->stats.total_draw_calls++;
  }

  // Render triangles
  for (uint32_t i = 0; i < debug_vis->triangle_count; i++) {
    const DebugTriangle *triangle = &debug_vis->triangles[i];
    if (triangle->wireframe) {
      // Draw triangle outline
      for (int j = 0; j < 3; j++) {
        Vec3 start = triangle->vertices[j];
        Vec3 end = triangle->vertices[(j + 1) % 3];
        // renderer_draw_line(renderer, start, end, triangle->color, 1.0f);
        debug_vis->stats.total_draw_calls++;
      }
    } else {
      // Draw filled triangle
      // renderer_draw_triangle(renderer, triangle->vertices, triangle->color);
      debug_vis->stats.triangles_drawn++;
      debug_vis->stats.total_draw_calls++;
    }
  }

  // Render texts
  for (uint32_t i = 0; i < debug_vis->text_count; i++) {
    const DebugText *text = &debug_vis->texts[i];
    if (text->world_space) {
      // renderer_draw_text_3d(renderer, text->position, text->text,
      // text->color, text->size, camera);
    } else {
      // renderer_draw_text_2d(renderer, text->position, text->text,
      // text->color, text->size);
    }
    debug_vis->stats.texts_drawn++;
    debug_vis->stats.total_draw_calls++;
  }

  // Render spheres
  for (uint32_t i = 0; i < debug_vis->sphere_count; i++) {
    const DebugSphere *sphere = &debug_vis->spheres[i];
    if (sphere->wireframe) {
      // Draw sphere wireframe
      physics_debug_draw_sphere_wireframe(debug_vis, sphere->center,
                                          sphere->radius, sphere->color,
                                          sphere->segments);
    } else {
      // Draw solid sphere (simplified as bounding box for debug)
      Vec3 half_extents = {sphere->radius, sphere->radius, sphere->radius};
      physics_debug_draw_box_internal(debug_vis, sphere->center, half_extents,
                                      quat_identity(), sphere->color, true);
    }
    debug_vis->stats.spheres_drawn++;
    debug_vis->stats.total_draw_calls++;
  }

  // Render boxes
  for (uint32_t i = 0; i < debug_vis->box_count; i++) {
    const DebugBox *box = &debug_vis->boxes[i];
    physics_debug_draw_box_internal(debug_vis, box->center, box->half_extents,
                                    box->rotation, box->color, box->wireframe);
    debug_vis->stats.boxes_drawn++;
    debug_vis->stats.total_draw_calls++;
  }

  // Render capsules
  for (uint32_t i = 0; i < debug_vis->capsule_count; i++) {
    const DebugCapsule *capsule = &debug_vis->capsules[i];
    physics_debug_draw_capsule_internal(debug_vis, capsule->center,
                                        capsule->radius, capsule->height,
                                        capsule->rotation, capsule->color,
                                        capsule->wireframe, capsule->segments);
    debug_vis->stats.capsules_drawn++;
    debug_vis->stats.total_draw_calls++;
  }

  debug_vis->stats.render_time = get_time() - start_time;
}

void physics_debug_draw_collision_shapes(PhysicsDebugVisualization *debug_vis,
                                         PhysicsWorld *world) {
  if (!debug_vis || !world ||
      !(debug_vis->enabled_flags & DEBUG_VIS_COLLISION_SHAPES))
    return;

  // This would iterate through all physics bodies and draw their collision
  // shapes For now, we'll provide a placeholder implementation

  LOG_DEBUG("Drawing collision shapes for physics world");

  // Example: Draw a box shape
  Vec3 center = {0.0f, 1.0f, 0.0f};
  Vec3 half_extents = {0.5f, 0.5f, 0.5f};
  uint32_t color = debug_vis->colors[DEBUG_COLOR_DYNAMIC];

  physics_debug_draw_box(debug_vis, center, half_extents, quat_identity(),
                         color, true, false);
}

void physics_debug_draw_bounding_boxes(PhysicsDebugVisualization *debug_vis,
                                       PhysicsWorld *world) {
  if (!debug_vis || !world ||
      !(debug_vis->enabled_flags & DEBUG_VIS_BOUNDING_BOXES))
    return;

  LOG_DEBUG("Drawing bounding boxes for physics world");

  // Example: Draw bounding box
  Vec3 center = {0.0f, 2.0f, 0.0f};
  Vec3 half_extents = {1.0f, 1.0f, 1.0f};
  uint32_t color = debug_vis->colors[DEBUG_COLOR_BROADPHASE];

  physics_debug_draw_box(debug_vis, center, half_extents, quat_identity(),
                         color, true, false);
}

void physics_debug_draw_contacts(PhysicsDebugVisualization *debug_vis,
                                 PhysicsWorld *world) {
  if (!debug_vis || !world ||
      !(debug_vis->enabled_flags & DEBUG_VIS_COLLISION_CONTACTS))
    return;

  LOG_DEBUG("Drawing collision contacts for physics world");

  // Example: Draw contact point
  Vec3 contact_point = {0.0f, 0.0f, 0.0f};
  Vec3 contact_normal = {0.0f, 1.0f, 0.0f};
  float penetration = 0.1f;
  uint32_t color = debug_vis->colors[DEBUG_COLOR_CONTACT];

  physics_debug_draw_contact_point(debug_vis, contact_point, contact_normal,
                                   penetration, color);
}

void physics_debug_draw_velocity_vectors(PhysicsDebugVisualization *debug_vis,
                                         PhysicsWorld *world) {
  if (!debug_vis || !world ||
      !(debug_vis->enabled_flags & DEBUG_VIS_VELOCITY_VECTORS))
    return;

  LOG_DEBUG("Drawing velocity vectors for physics world");

  // Example: Draw velocity vector
  Vec3 origin = {1.0f, 1.0f, 1.0f};
  Vec3 velocity = {2.0f, 0.0f, 1.0f};
  uint32_t color = debug_vis->colors[DEBUG_COLOR_VELOCITY];

  physics_debug_draw_vector(debug_vis, origin, velocity, color,
                            debug_vis->scale);
}

void physics_debug_draw_force_vectors(PhysicsDebugVisualization *debug_vis,
                                      PhysicsWorld *world) {
  if (!debug_vis || !world ||
      !(debug_vis->enabled_flags & DEBUG_VIS_FORCE_VECTORS))
    return;

  LOG_DEBUG("Drawing force vectors for physics world");

  // Example: Draw force vector
  Vec3 origin = {2.0f, 1.0f, 0.0f};
  Vec3 force = {0.0f, -9.81f, 0.0f}; // Gravity
  uint32_t color = debug_vis->colors[DEBUG_COLOR_FORCE];

  physics_debug_draw_vector(debug_vis, origin, force, color, debug_vis->scale);
}

void physics_debug_draw_soft_bodies(PhysicsDebugVisualization *debug_vis,
                                    struct SoftBodyWorld *world) {
  if (!debug_vis || !world ||
      !(debug_vis->enabled_flags & DEBUG_VIS_SOFT_BODIES))
    return;

  LOG_DEBUG("Drawing soft bodies for soft body world");

  // Draw each soft body
  for (uint32_t i = 0; i < world->body_count; i++) {
    SoftBody *body = &world->bodies[i];
    if (!body->active)
      continue;

    uint32_t color = debug_vis->colors[DEBUG_COLOR_SOFT_BODY];

    // Draw particles as points
    for (uint32_t j = 0; j < body->config.particle_count; j++) {
      SoftParticle *particle = &body->particles[j];
      physics_debug_draw_point(debug_vis, particle->position, color, false);
    }

    // Draw constraints as lines
    for (uint32_t j = 0; j < body->config.constraint_count; j++) {
      SoftConstraint *constraint = &body->constraints[j];
      if (!constraint->enabled)
        continue;

      SoftParticle *p1 = &body->particles[constraint->particle_a];
      SoftParticle *p2 = &body->particles[constraint->particle_b];

      physics_debug_draw_line(debug_vis, p1->position, p2->position, color,
                              false);
    }

    // Draw faces for cloth
    if (body->config.type == SOFT_BODY_CLOTH) {
      for (uint32_t j = 0; j < body->config.face_count; j++) {
        SoftFace *face = &body->faces[j];
        if (!face->active)
          continue;

        Vec3 vertices[3] = {body->particles[face->indices[0]].position,
                            body->particles[face->indices[1]].position,
                            body->particles[face->indices[2]].position};

        physics_debug_draw_triangle(debug_vis, vertices, color, true, false);
      }
    }
  }
}

uint32_t physics_debug_draw_line(PhysicsDebugVisualization *debug_vis,
                                 Vec3 start, Vec3 end, uint32_t color,
                                 bool persistent) {
  if (!debug_vis || debug_vis->line_count >= debug_vis->line_capacity)
    return UINT32_MAX;

  DebugLine *line = &debug_vis->lines[debug_vis->line_count];
  line->start = start;
  line->end = end;
  line->color = color;
  line->thickness = debug_vis->line_thickness;
  line->persistent = persistent;
  line->lifetime = persistent ? 0.0f : 5.0f; // 5 seconds for temporary lines

  return debug_vis->line_count++;
}

uint32_t physics_debug_draw_point(PhysicsDebugVisualization *debug_vis,
                                  Vec3 position, uint32_t color,
                                  bool persistent) {
  if (!debug_vis || debug_vis->point_count >= debug_vis->point_capacity)
    return UINT32_MAX;

  DebugPoint *point = &debug_vis->points[debug_vis->point_count];
  point->position = position;
  point->color = color;
  point->size = debug_vis->point_size;
  point->persistent = persistent;
  point->lifetime = persistent ? 0.0f : 5.0f;

  return debug_vis->point_count++;
}

uint32_t physics_debug_draw_triangle(PhysicsDebugVisualization *debug_vis,
                                     const Vec3 vertices[3], uint32_t color,
                                     bool wireframe, bool persistent) {
  if (!debug_vis || debug_vis->triangle_count >= debug_vis->triangle_capacity)
    return UINT32_MAX;

  DebugTriangle *triangle = &debug_vis->triangles[debug_vis->triangle_count];
  triangle->vertices[0] = vertices[0];
  triangle->vertices[1] = vertices[1];
  triangle->vertices[2] = vertices[2];
  triangle->color = color;
  triangle->wireframe = wireframe;
  triangle->persistent = persistent;
  triangle->lifetime = persistent ? 0.0f : 5.0f;

  return debug_vis->triangle_count++;
}

uint32_t physics_debug_draw_box(PhysicsDebugVisualization *debug_vis,
                                Vec3 center, Vec3 half_extents, Quat rotation,
                                uint32_t color, bool wireframe,
                                bool persistent) {
  if (!debug_vis || debug_vis->box_count >= debug_vis->box_capacity)
    return UINT32_MAX;

  DebugBox *box = &debug_vis->boxes[debug_vis->box_count];
  box->center = center;
  box->half_extents = half_extents;
  box->rotation = rotation;
  box->color = color;
  box->wireframe = wireframe;
  box->persistent = persistent;
  box->lifetime = persistent ? 0.0f : 5.0f;

  return debug_vis->box_count++;
}

uint32_t physics_debug_draw_sphere(PhysicsDebugVisualization *debug_vis,
                                   Vec3 center, float radius, uint32_t color,
                                   bool wireframe, bool persistent) {
  if (!debug_vis || debug_vis->sphere_count >= debug_vis->sphere_capacity)
    return UINT32_MAX;

  DebugSphere *sphere = &debug_vis->spheres[debug_vis->sphere_count];
  sphere->center = center;
  sphere->radius = radius;
  sphere->color = color;
  sphere->wireframe = wireframe;
  sphere->persistent = persistent;
  sphere->lifetime = persistent ? 0.0f : 5.0f;
  sphere->segments = 16;

  return debug_vis->sphere_count++;
}

void physics_debug_draw_contact_point(PhysicsDebugVisualization *debug_vis,
                                      Vec3 position, Vec3 normal,
                                      float penetration, uint32_t color) {
  if (!debug_vis)
    return;

  // Draw contact point
  physics_debug_draw_point(debug_vis, position, color, false);

  // Draw contact normal
  Vec3 normal_end = vec3_add(position, vec3_mul(normal, penetration + 0.5f));
  physics_debug_draw_line(debug_vis, position, normal_end, color, false);

  // Draw penetration indicator
  Vec3 penetration_end = vec3_add(position, vec3_mul(normal, -penetration));
  physics_debug_draw_line(debug_vis, position, penetration_end, 0xFFFF0000,
                          false); // Red for penetration
}

void physics_debug_draw_vector(PhysicsDebugVisualization *debug_vis,
                               Vec3 origin, Vec3 vector, uint32_t color,
                               float scale) {
  if (!debug_vis)
    return;

  Vec3 scaled_vector = vec3_mul(vector, scale);
  Vec3 end = vec3_add(origin, scaled_vector);

  // Draw main vector line
  physics_debug_draw_line(debug_vis, origin, end, color, false);

  // Draw arrowhead
  float arrow_size = 0.1f * scale;
  float vector_length = vec3_length(scaled_vector);

  if (vector_length > 0.001f) {
    Vec3 vector_dir = vec3_div(scaled_vector, vector_length);

    // Calculate perpendicular vectors for arrowhead
    Vec3 perp1 = vec3_cross(vector_dir, (Vec3){0, 1, 0});
    if (vec3_length_sq(perp1) < 0.001f) {
      perp1 = vec3_cross(vector_dir, (Vec3){1, 0, 0});
    }
    perp1 = vec3_normalize(perp1);

    Vec3 perp2 = vec3_cross(vector_dir, perp1);
    perp2 = vec3_normalize(perp2);

    // Draw arrowhead lines
    Vec3 arrow_base = vec3_sub(end, vec3_mul(vector_dir, arrow_size));
    Vec3 arrow_point1 =
        vec3_add(arrow_base, vec3_mul(perp1, arrow_size * 0.5f));
    Vec3 arrow_point2 =
        vec3_add(arrow_base, vec3_mul(perp2, arrow_size * 0.5f));
    Vec3 arrow_point3 =
        vec3_sub(arrow_base, vec3_mul(perp1, arrow_size * 0.5f));
    Vec3 arrow_point4 =
        vec3_sub(arrow_base, vec3_mul(perp2, arrow_size * 0.5f));

    physics_debug_draw_line(debug_vis, end, arrow_point1, color, false);
    physics_debug_draw_line(debug_vis, end, arrow_point2, color, false);
    physics_debug_draw_line(debug_vis, end, arrow_point3, color, false);
    physics_debug_draw_line(debug_vis, end, arrow_point4, color, false);
  }
}

// Internal helper functions
void physics_debug_draw_box_internal(PhysicsDebugVisualization *debug_vis,
                                     Vec3 center, Vec3 half_extents,
                                     Quat rotation, uint32_t color,
                                     bool wireframe) {
  if (!debug_vis)
    return;

  // Calculate box corners
  Vec3 corners[8];
  Vec3 extents[8] = {{-half_extents.x, -half_extents.y, -half_extents.z},
                     {half_extents.x, -half_extents.y, -half_extents.z},
                     {half_extents.x, half_extents.y, -half_extents.z},
                     {-half_extents.x, half_extents.y, -half_extents.z},
                     {-half_extents.x, -half_extents.y, half_extents.z},
                     {half_extents.x, -half_extents.y, half_extents.z},
                     {half_extents.x, half_extents.y, half_extents.z},
                     {-half_extents.x, half_extents.y, half_extents.z}};

  for (int i = 0; i < 8; i++) {
    corners[i] = vec3_add(center, quat_rotate_vec3(rotation, extents[i]));
  }

  if (wireframe) {
    // Draw box edges
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Bottom face
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Top face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Vertical edges
    };

    for (int i = 0; i < 12; i++) {
      physics_debug_draw_line(debug_vis, corners[edges[i][0]],
                              corners[edges[i][1]], color, false);
    }
  } else {
    // Draw box faces (simplified as triangles)
    int faces[12][3] = {// Bottom face
                        {0, 1, 2},
                        {0, 2, 3},
                        // Top face
                        {4, 7, 6},
                        {4, 6, 5},
                        // Front face
                        {0, 4, 5},
                        {0, 5, 1},
                        // Back face
                        {2, 6, 7},
                        {2, 7, 3},
                        // Left face
                        {0, 3, 7},
                        {0, 7, 4},
                        // Right face
                        {1, 5, 6},
                        {1, 6, 2}};

    for (int i = 0; i < 12; i++) {
      Vec3 vertices[3] = {corners[faces[i][0]], corners[faces[i][1]],
                          corners[faces[i][2]]};
      physics_debug_draw_triangle(debug_vis, vertices, color, false, false);
    }
  }
}

void physics_debug_draw_velocity_vector(PhysicsDebugVisualization *debug_vis,
                                        RigidBody *body, uint32_t color) {
  if (!debug_vis || !body)
    return;
  Vec3 pos = rigid_body_get_position(body);
  Vec3 vel = rigid_body_get_velocity(body);
  physics_debug_draw_vector(debug_vis, pos, vel, color, 1.0f);
}

void physics_debug_draw_force_vector(PhysicsDebugVisualization *debug_vis,
                                     RigidBody *body, Vec3 force,
                                     uint32_t color) {
  if (!debug_vis || !body)
    return;
  Vec3 pos = rigid_body_get_position(body);
  physics_debug_draw_vector(debug_vis, pos, force, color, 0.1f);
}

void physics_debug_draw_angular_velocity(PhysicsDebugVisualization *debug_vis,
                                         RigidBody *body, uint32_t color) {
  if (!debug_vis || !body)
    return;
  // Placeholder: Need rigid_body_get_angular_velocity access
  // Vec3 pos = rigid_body_get_position(body);
  // Vec3 ang_vel = rigid_body_get_angular_velocity(body);
  // physics_debug_draw_vector(debug_vis, pos, ang_vel, color, 1.0f);
}

void physics_debug_draw_body_info(PhysicsDebugVisualization *debug_vis,
                                  RigidBody *body,
                                  const struct Camera *camera) {
  // Placeholder implementation
}

void physics_debug_draw_sphere_wireframe(PhysicsDebugVisualization *debug_vis,
                                         Vec3 center, float radius,
                                         uint32_t color, int segments) {
  if (!debug_vis || segments < 4)
    return;

  float angle_step = 2.0f * M_PI / segments;

  // Draw horizontal circles
  for (int i = 0; i <= segments / 2; i++) {
    float y = radius * cosf(i * angle_step);
    float circle_radius = radius * sinf(i * angle_step);

    for (int j = 0; j < segments; j++) {
      float angle1 = j * angle_step;
      float angle2 = (j + 1) * angle_step;

      Vec3 p1 = {center.x + circle_radius * cosf(angle1), center.y + y,
                 center.z + circle_radius * sinf(angle1)};

      Vec3 p2 = {center.x + circle_radius * cosf(angle2), center.y + y,
                 center.z + circle_radius * sinf(angle2)};

      physics_debug_draw_line(debug_vis, p1, p2, color, false);
    }
  }

  // Draw vertical circles
  for (int i = 0; i < segments; i++) {
    float angle = i * angle_step;
    Vec3 axis = {cosf(angle), 0.0f, sinf(angle)};

    for (int j = 0; j < segments; j++) {
      float angle1 = j * angle_step;
      float angle2 = (j + 1) * angle_step;

      Vec3 p1 = {center.x + radius * sinf(angle1) * axis.x,
                 center.y + radius * cosf(angle1),
                 center.z + radius * sinf(angle1) * axis.z};

      Vec3 p2 = {center.x + radius * sinf(angle2) * axis.x,
                 center.y + radius * cosf(angle2),
                 center.z + radius * sinf(angle2) * axis.z};

      physics_debug_draw_line(debug_vis, p1, p2, color, false);
    }
  }
}

void physics_debug_draw_capsule_internal(PhysicsDebugVisualization *debug_vis,
                                         Vec3 center, float radius,
                                         float height, Quat rotation,
                                         uint32_t color, bool wireframe,
                                         int segments) {
  if (!debug_vis)
    return;

  float half_height = height * 0.5f;

  // Draw cylinder part
  Vec3 cylinder_center = center;
  Vec3 cylinder_half_extents = {radius, half_height, radius};
  physics_debug_draw_box_internal(debug_vis, cylinder_center,
                                  cylinder_half_extents, rotation, color, true);

  // Draw top and bottom hemispheres
  Vec3 top_center =
      vec3_add(center, vec3_mul(quat_rotate_vec3(rotation, (Vec3){0, 1, 0}),
                                half_height));
  Vec3 bottom_center =
      vec3_add(center, vec3_mul(quat_rotate_vec3(rotation, (Vec3){0, -1, 0}),
                                half_height));

  physics_debug_draw_sphere(debug_vis, top_center, radius, color, wireframe,
                            false);
  physics_debug_draw_sphere(debug_vis, bottom_center, radius, color, wireframe,
                            false);
}

// Utility functions
uint32_t physics_debug_color_from_rgb(float r, float g, float b) {
  // Clamp values to [0, 1] range
  r = fmaxf(0.0f, fminf(r, 1.0f));
  g = fmaxf(0.0f, fminf(g, 1.0f));
  b = fmaxf(0.0f, fminf(b, 1.0f));

  uint8_t red = (uint8_t)(r * 255.0f);
  uint8_t green = (uint8_t)(g * 255.0f);
  uint8_t blue = (uint8_t)(b * 255.0f);

  return 0xFF000000 | (red << 16) | (green << 8) | blue;
}

uint32_t physics_debug_color_from_rgba(float r, float g, float b, float a) {
  // Clamp values to [0, 1] range
  r = fmaxf(0.0f, fminf(r, 1.0f));
  g = fmaxf(0.0f, fminf(g, 1.0f));
  b = fmaxf(0.0f, fminf(b, 1.0f));
  a = fmaxf(0.0f, fminf(a, 1.0f));

  uint8_t red = (uint8_t)(r * 255.0f);
  uint8_t green = (uint8_t)(g * 255.0f);
  uint8_t blue = (uint8_t)(b * 255.0f);
  uint8_t alpha = (uint8_t)(a * 255.0f);

  return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

void physics_debug_clear_all(PhysicsDebugVisualization *debug_vis) {
  if (!debug_vis)
    return;

  debug_vis->line_count = 0;
  debug_vis->point_count = 0;
  debug_vis->triangle_count = 0;
  debug_vis->text_count = 0;
  debug_vis->sphere_count = 0;
  debug_vis->box_count = 0;
  debug_vis->capsule_count = 0;
}

// Global accessor functions
PhysicsDebugVisualization *get_physics_debug_visualization(void) {
  return &g_physics_debug_vis;
}

bool init_physics_debug_visualization(uint32_t max_geometries) {
  return physics_debug_visualization_init(&g_physics_debug_vis, max_geometries);
}

void cleanup_physics_debug_visualization(void) {
  physics_debug_visualization_cleanup(&g_physics_debug_vis);
}

// Alias for vulkan_render.c compatibility
PhysicsDebugVisualization *get_debug_vis(void) { return &g_physics_debug_vis; }

// Configuration Functions
void physics_debug_set_enabled(PhysicsDebugVisualization *debug_vis,
                               bool enabled) {
  if (debug_vis) {
    debug_vis->enabled = enabled;
  }
}

void physics_debug_set_flags(PhysicsDebugVisualization *debug_vis,
                             uint32_t flags) {
  if (debug_vis) {
    debug_vis->enabled_flags = flags;
  }
}

void physics_debug_clear_flags(PhysicsDebugVisualization *debug_vis,
                               uint32_t flags) {
  if (debug_vis) {
    debug_vis->enabled_flags &= ~flags;
  }
}

bool physics_debug_is_enabled(const PhysicsDebugVisualization *debug_vis,
                              uint32_t flag) {
  return debug_vis && (debug_vis->enabled_flags & flag);
}

void physics_debug_set_color(PhysicsDebugVisualization *debug_vis,
                             DebugColorType type, uint32_t color) {
  if (debug_vis && type < DEBUG_COLOR_COUNT) {
    debug_vis->colors[type] = color;
  }
}

uint32_t physics_debug_get_color(const PhysicsDebugVisualization *debug_vis,
                                 DebugColorType type) {
  if (debug_vis && type < DEBUG_COLOR_COUNT) {
    return debug_vis->colors[type];
  }
  return 0xFFFFFFFF;
}

void physics_debug_reset_colors(PhysicsDebugVisualization *debug_vis) {
  if (debug_vis) {
    memcpy(debug_vis->colors, DEFAULT_COLORS, sizeof(DEFAULT_COLORS));
  }
}

void physics_debug_set_scale(PhysicsDebugVisualization *debug_vis,
                             float scale) {
  if (debug_vis) {
    debug_vis->scale = scale;
  }
}

void physics_debug_set_line_thickness(PhysicsDebugVisualization *debug_vis,
                                      float thickness) {
  if (debug_vis) {
    debug_vis->line_thickness = thickness;
  }
}

void physics_debug_set_point_size(PhysicsDebugVisualization *debug_vis,
                                  float size) {
  if (debug_vis) {
    debug_vis->point_size = size;
  }
}

void physics_debug_set_fade_enabled(PhysicsDebugVisualization *debug_vis,
                                    bool enabled) {
  if (debug_vis) {
    debug_vis->fade_over_time = enabled;
  }
}
