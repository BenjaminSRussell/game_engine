#ifndef PHYSICS_DEBUG_VISUALIZATION_H
#define PHYSICS_DEBUG_VISUALIZATION_H

#include "math/quat.h"
#include "math/vec3.h"
#include "physics/physics.h"
#include <renderer/renderer.h>
// Forward declarations for types used in pointers
struct PhysicsBody;
struct TriangleMeshShape;
struct SoftBodyWorld;
struct CCDWorld;
struct SphereShape;
struct BoxShape;
struct CapsuleShape;
struct ConvexHullShape;
struct IRenderer;
struct Camera;

// Remove typedefs here to avoid conflicts with defining headers.
// Users of this header should rely on struct tags or include appropriate
// headers if they need the types.

#ifdef __cplusplus
extern "C" {
#endif

// ... (omitting lines between)

// Debug Visualization Types
typedef enum {
  DEBUG_VIS_COLLISION_SHAPES = 1 << 0,
  DEBUG_VIS_COLLISION_CONTACTS = 1 << 1,
  DEBUG_VIS_BOUNDING_BOXES = 1 << 2,
  DEBUG_VIS_VELOCITY_VECTORS = 1 << 3,
  DEBUG_VIS_FORCE_VECTORS = 1 << 4,
  DEBUG_VIS_CONSTRAINTS = 1 << 5,
  DEBUG_VIS_BROADPHASE = 1 << 6,
  DEBUG_VIS_SPAWITAL_HASH = 1 << 7,
  DEBUG_VIS_RAYCASTS = 1 << 8,
  DEBUG_VIS_TRIGGER_VOLUMES = 1 << 9,
  DEBUG_VIS_SLEEPING_BODIES = 1 << 10,
  DEBUG_VIS_SOFT_BODIES = 1 << 11,
  DEBUG_VIS_CONTINUOUS_COLLISION = 1 << 12,
  DEBUG_VIS_ALL = 0xFFFFFFFF
} DebugVisualizationFlags;

// Debug Color Types
typedef enum {
  DEBUG_COLOR_STATIC = 0,
  DEBUG_COLOR_DYNAMIC,
  DEBUG_COLOR_KINEMATIC,
  DEBUG_COLOR_SLEEPING,
  DEBUG_COLOR_TRIGGER,
  DEBUG_COLOR_CONTACT,
  DEBUG_COLOR_CONSTRAINT,
  DEBUG_COLOR_FORCE,
  DEBUG_COLOR_VELOCITY,
  DEBUG_COLOR_RAYCAST_HIT,
  DEBUG_COLOR_RAYCAST_MISS,
  DEBUG_COLOR_BROADPHASE,
  DEBUG_COLOR_SOFT_BODY,
  DEBUG_COLOR_COUNT
} DebugColorType;

// Debug Line Definition
typedef struct {
  Vec3 start;
  Vec3 end;
  uint32_t color;
  float thickness;
  bool persistent;
  float lifetime;
} DebugLine;

// Debug Point Definition
typedef struct {
  Vec3 position;
  uint32_t color;
  float size;
  bool persistent;
  float lifetime;
} DebugPoint;

// Debug Triangle Definition
typedef struct {
  Vec3 vertices[3];
  uint32_t color;
  bool wireframe;
  bool persistent;
  float lifetime;
} DebugTriangle;

// Debug Text Definition
typedef struct {
  Vec3 position;
  char text[128];
  uint32_t color;
  float size;
  bool persistent;
  float lifetime;
  bool world_space;
} DebugText;

// Debug Sphere Definition
typedef struct {
  Vec3 center;
  float radius;
  uint32_t color;
  bool wireframe;
  bool persistent;
  float lifetime;
  int segments;
} DebugSphere;

// Debug Box Definition
typedef struct {
  Vec3 center;
  Vec3 half_extents;
  Quat rotation;
  uint32_t color;
  bool wireframe;
  bool persistent;
  float lifetime;
} DebugBox;

// Debug Capsule Definition
typedef struct {
  Vec3 center;
  float radius;
  float height;
  Quat rotation;
  uint32_t color;
  bool wireframe;
  bool persistent;
  float lifetime;
  int segments;
} DebugCapsule;

// Debug Visualization Context
typedef struct PhysicsDebugVisualization {
  // Visualization settings
  uint32_t enabled_flags;
  bool enabled;
  float scale;
  float line_thickness;
  float point_size;
  bool show_labels;
  bool fade_over_time;

  // Color settings
  uint32_t colors[DEBUG_COLOR_COUNT];

  // Debug geometry buffers
  DebugLine *lines;
  uint32_t line_count;
  uint32_t line_capacity;

  DebugPoint *points;
  uint32_t point_count;
  uint32_t point_capacity;

  DebugTriangle *triangles;
  uint32_t triangle_count;
  uint32_t triangle_capacity;

  DebugText *texts;
  uint32_t text_count;
  uint32_t text_capacity;

  DebugSphere *spheres;
  uint32_t sphere_count;
  uint32_t sphere_capacity;

  DebugBox *boxes;
  uint32_t box_count;
  uint32_t box_capacity;

  DebugCapsule *capsules;
  uint32_t capsule_count;
  uint32_t capsule_capacity;

  // Performance statistics
  struct {
    uint32_t total_draw_calls;
    uint32_t lines_drawn;
    uint32_t points_drawn;
    uint32_t triangles_drawn;
    uint32_t texts_drawn;
    uint32_t spheres_drawn;
    uint32_t boxes_drawn;
    uint32_t capsules_drawn;
    float render_time;
  } stats;
} PhysicsDebugVisualization;

// Physics Debug Visualization Functions
bool physics_debug_visualization_init(PhysicsDebugVisualization *debug_vis,
                                      uint32_t max_geometries);
void physics_debug_visualization_cleanup(PhysicsDebugVisualization *debug_vis);
void physics_debug_visualization_update(PhysicsDebugVisualization *debug_vis,
                                        float delta_time);
void physics_debug_visualization_render(PhysicsDebugVisualization *debug_vis,
                                        struct IRenderer *renderer,
                                        struct Camera *camera);

// Visualization Control
void physics_debug_set_enabled(PhysicsDebugVisualization *debug_vis,
                               bool enabled);
void physics_debug_set_flags(PhysicsDebugVisualization *debug_vis,
                             uint32_t flags);
void physics_debug_clear_flags(PhysicsDebugVisualization *debug_vis,
                               uint32_t flags);
bool physics_debug_is_enabled(const PhysicsDebugVisualization *debug_vis,
                              uint32_t flag);

// Color Management
void physics_debug_set_color(PhysicsDebugVisualization *debug_vis,
                             DebugColorType type, uint32_t color);
uint32_t physics_debug_get_color(const PhysicsDebugVisualization *debug_vis,
                                 DebugColorType type);
void physics_debug_reset_colors(PhysicsDebugVisualization *debug_vis);

// Geometry Drawing Functions
uint32_t physics_debug_draw_line(PhysicsDebugVisualization *debug_vis,
                                 Vec3 start, Vec3 end, uint32_t color,
                                 bool persistent);
uint32_t physics_debug_draw_point(PhysicsDebugVisualization *debug_vis,
                                  Vec3 position, uint32_t color,
                                  bool persistent);
uint32_t physics_debug_draw_triangle(PhysicsDebugVisualization *debug_vis,
                                     const Vec3 vertices[3], uint32_t color,
                                     bool wireframe, bool persistent);
uint32_t physics_debug_draw_text(PhysicsDebugVisualization *debug_vis,
                                 Vec3 position, const char *text,
                                 uint32_t color, bool persistent);
uint32_t physics_debug_draw_sphere(PhysicsDebugVisualization *debug_vis,
                                   Vec3 center, float radius, uint32_t color,
                                   bool wireframe, bool persistent);
uint32_t physics_debug_draw_box(PhysicsDebugVisualization *debug_vis,
                                Vec3 center, Vec3 half_extents, Quat rotation,
                                uint32_t color, bool wireframe,
                                bool persistent);

// Forward declarations
struct PhysicsBody;
struct TriangleMeshShape;
struct SoftBodyWorld;
struct CCDWorld;
struct SphereShape;
struct BoxShape;
struct CapsuleShape;
struct ConvexHullShape;

#ifdef __cplusplus
extern "C" {
#endif

// Physics-specific Drawing Functions
void physics_debug_draw_collision_shapes(PhysicsDebugVisualization *debug_vis,
                                         PhysicsWorld *world);
void physics_debug_draw_bounding_boxes(PhysicsDebugVisualization *debug_vis,
                                       PhysicsWorld *world);
void physics_debug_draw_contacts(PhysicsDebugVisualization *debug_vis,
                                 PhysicsWorld *world);
void physics_debug_draw_velocity_vectors(PhysicsDebugVisualization *debug_vis,
                                         PhysicsWorld *world);
void physics_debug_draw_force_vectors(PhysicsDebugVisualization *debug_vis,
                                      PhysicsWorld *world);
void physics_debug_draw_constraints(PhysicsDebugVisualization *debug_vis,
                                    PhysicsWorld *world);
void physics_debug_draw_broadphase(PhysicsDebugVisualization *debug_vis,
                                   PhysicsWorld *world);
void physics_debug_draw_spatial_hash(PhysicsDebugVisualization *debug_vis,
                                     PhysicsWorld *world);
void physics_debug_draw_raycasts(PhysicsDebugVisualization *debug_vis,
                                 PhysicsWorld *world);
void physics_debug_draw_trigger_volumes(PhysicsDebugVisualization *debug_vis,
                                        PhysicsWorld *world);
void physics_debug_draw_sleeping_bodies(PhysicsDebugVisualization *debug_vis,
                                        PhysicsWorld *world);
void physics_debug_draw_soft_bodies(PhysicsDebugVisualization *debug_vis,
                                    struct SoftBodyWorld *world);
void physics_debug_draw_continuous_collision(
    PhysicsDebugVisualization *debug_vis, struct CCDWorld *world);

// Shape-specific Drawing Functions
void physics_debug_draw_sphere_shape(PhysicsDebugVisualization *debug_vis,
                                     const struct SphereShape *sphere,
                                     Vec3 position, uint32_t color);
void physics_debug_draw_box_shape(PhysicsDebugVisualization *debug_vis,
                                  const struct BoxShape *box, Vec3 position,
                                  Quat rotation, uint32_t color);
void physics_debug_draw_capsule_shape(PhysicsDebugVisualization *debug_vis,
                                      const struct CapsuleShape *capsule,
                                      Vec3 position, Quat rotation,
                                      uint32_t color);
void physics_debug_draw_convex_hull_shape(PhysicsDebugVisualization *debug_vis,
                                          const struct ConvexHullShape *hull,
                                          Vec3 position, Quat rotation,
                                          uint32_t color);
void physics_debug_draw_triangle_mesh_shape(
    PhysicsDebugVisualization *debug_vis, const struct TriangleMeshShape *mesh,
    Vec3 position, Quat rotation, uint32_t color);

// Contact and Constraint Visualization
void physics_debug_draw_contact_point(PhysicsDebugVisualization *debug_vis,
                                      Vec3 position, Vec3 normal,
                                      float penetration, uint32_t color);
void physics_debug_draw_constraint(PhysicsDebugVisualization *debug_vis,
                                   Vec3 anchor_a, Vec3 anchor_b,
                                   uint32_t color);
void physics_debug_draw_spring_constraint(PhysicsDebugVisualization *debug_vis,
                                          Vec3 anchor_a, Vec3 anchor_b,
                                          float rest_length, uint32_t color);
void physics_debug_draw_hinge_constraint(PhysicsDebugVisualization *debug_vis,
                                         Vec3 pivot, Vec3 axis, uint32_t color);

// Vector Visualization
void physics_debug_draw_vector(PhysicsDebugVisualization *debug_vis,
                               Vec3 origin, Vec3 vector, uint32_t color,
                               float scale);
void physics_debug_draw_velocity_vector(PhysicsDebugVisualization *debug_vis,
                                        RigidBody *body, uint32_t color);
void physics_debug_draw_force_vector(PhysicsDebugVisualization *debug_vis,
                                     RigidBody *body, Vec3 force,
                                     uint32_t color);
void physics_debug_draw_angular_velocity(PhysicsDebugVisualization *debug_vis,
                                         RigidBody *body, uint32_t color);

// Utility Functions
void physics_debug_clear_all(PhysicsDebugVisualization *debug_vis);
void physics_debug_clear_persistent(PhysicsDebugVisualization *debug_vis);
void physics_debug_clear_temporary(PhysicsDebugVisualization *debug_vis);
void physics_debug_remove_geometry(PhysicsDebugVisualization *debug_vis,
                                   uint32_t geometry_id);

// Performance and Statistics
void physics_debug_get_performance_stats(
    const PhysicsDebugVisualization *debug_vis, uint32_t *total_draw_calls,
    float *render_time);
void physics_debug_reset_performance_stats(
    PhysicsDebugVisualization *debug_vis);

// Configuration Functions
void physics_debug_set_scale(PhysicsDebugVisualization *debug_vis, float scale);
void physics_debug_set_line_thickness(PhysicsDebugVisualization *debug_vis,
                                      float thickness);
void physics_debug_set_point_size(PhysicsDebugVisualization *debug_vis,
                                  float size);
void physics_debug_set_fade_enabled(PhysicsDebugVisualization *debug_vis,
                                    bool enabled);

// Helper Functions
uint32_t physics_debug_color_from_rgb(float r, float g, float b);
uint32_t physics_debug_color_from_rgba(float r, float g, float b, float a);
Vec3 physics_debug_world_to_screen(Vec3 world_pos, const struct Camera *camera);
bool physics_debug_is_visible(Vec3 position, const struct Camera *camera,
                              float max_distance);

// Debug Text Utilities
void physics_debug_draw_body_info(PhysicsDebugVisualization *debug_vis,
                                  RigidBody *body, const struct Camera *camera);
void physics_debug_draw_performance_info(PhysicsDebugVisualization *debug_vis,
                                         const PhysicsWorld *world,
                                         const struct Camera *camera);
void physics_debug_draw_collision_info(PhysicsDebugVisualization *debug_vis,
                                       const PhysicsWorld *world,
                                       const struct Camera *camera);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_DEBUG_VISUALIZATION_H
