#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Stub implementations for Swift bridge functions
// These provide minimal functionality to allow the Swift frontend to compile
// and run Replace with real engine implementations when integrating with full C
// engine

// ============================================================================
// Engine Lifecycle
// ============================================================================

void engine_init(void) { printf("[C Bridge Stub] engine_init called\n"); }

void engine_shutdown(void) {
  printf("[C Bridge Stub] engine_shutdown called\n");
}

void engine_update(float delta_time) {
  // Silent - called every frame
}

// ============================================================================
// Render Stats
// ============================================================================

typedef struct {
  float fps;
  float frame_time_ms;
  uint32_t draw_calls;
  uint32_t triangles;
  uint32_t vertices;
  uint64_t memory_used;
} RenderStats_C;

void engine_get_render_stats(RenderStats_C *stats) {
  if (stats) {
    stats->fps = 60.0f;
    stats->frame_time_ms = 16.67f;
    stats->draw_calls = 124;
    stats->triangles = 50000;
    stats->vertices = 75000;
    stats->memory_used = 256 * 1024 * 1024; // 256 MB
  }
}

// ============================================================================
// Transform
// ============================================================================

typedef struct {
  float position[3];
  float rotation[3];
  float scale[3];
} Transform_C;

void engine_get_transform(uint64_t entity_id, Transform_C *transform) {
  if (transform) {
    transform->position[0] = 0.0f;
    transform->position[1] = 0.0f;
    transform->position[2] = 0.0f;
    transform->rotation[0] = 0.0f;
    transform->rotation[1] = 0.0f;
    transform->rotation[2] = 0.0f;
    transform->scale[0] = 1.0f;
    transform->scale[1] = 1.0f;
    transform->scale[2] = 1.0f;
  }
}

void engine_set_transform(uint64_t entity_id, const Transform_C *transform) {
  printf("[C Bridge Stub] Set transform for entity %llu\n", entity_id);
}

// ============================================================================
// Entity Management
// ============================================================================

uint64_t engine_create_entity(const char *name) {
  static uint64_t next_id = 1;
  printf("[C Bridge Stub] Created entity '%s' with ID %llu\n",
         name ? name : "Unnamed", next_id);
  return next_id++;
}

uint64_t engine_create_entity_with_id(uint64_t id, const char *name) {
  printf("[C Bridge Stub] Created entity '%s' with requested ID %llu\n",
         name ? name : "Unnamed", id);
  return id;
}

void engine_delete_entity(uint64_t entity_id) {
  printf("[C Bridge Stub] Deleted entity %llu\n", entity_id);
}

const char *engine_get_entity_name(uint64_t entity_id) {
  static char name_buffer[256];
  snprintf(name_buffer, sizeof(name_buffer), "Entity_%llu", entity_id);
  return name_buffer;
}

void engine_set_entity_name(uint64_t entity_id, const char *name) {
  printf("[C Bridge Stub] Set entity %llu name to '%s'\n", entity_id,
         name ? name : "Unnamed");
}

uint64_t engine_get_entity_parent(uint64_t entity_id) {
  return 0; // No parent (root)
}

void engine_set_entity_parent(uint64_t entity_id, uint64_t parent_id) {
  printf("[C Bridge Stub] Set entity %llu parent to %llu\n", entity_id,
         parent_id);
}

// ============================================================================
// Components
// ============================================================================

typedef enum {
  COMPONENT_TRANSFORM = 0,
  COMPONENT_MESH_RENDERER = 1,
  COMPONENT_PHYSICS = 2
} ComponentType;

bool engine_has_component(uint64_t entity_id, int32_t type) {
  return true; // Stub: all entities have all components
}

void engine_add_component(uint64_t entity_id, int32_t type) {
  printf("[C Bridge] Added component type %d to entity %llu\n", type,
         entity_id);
}

void engine_remove_component(uint64_t entity_id, int32_t type) {
  printf("[C Bridge] Removed component type %d from entity %llu\n", type,
         entity_id);
}

// ============================================================================
// Mesh Renderer
// ============================================================================

typedef struct {
  uint64_t mesh_id;
  uint64_t material_id;
  bool cast_shadows;
  bool receive_shadows;
} MeshRendererData_C;

void engine_set_mesh_renderer(uint64_t entity_id,
                              const MeshRendererData_C *data) {
  printf("[C Bridge Stub] Set mesh renderer for entity %llu\n", entity_id);
}

void engine_get_mesh_renderer(uint64_t entity_id, MeshRendererData_C *data) {
  if (data) {
    data->mesh_id = 1;
    data->material_id = 1;
    data->cast_shadows = true;
    data->receive_shadows = true;
  }
}

// ============================================================================
// Physics
// ============================================================================

typedef struct {
  float mass;
  float drag;
  float angular_drag;
  bool use_gravity;
  bool is_kinematic;
  float velocity[3];
  float angular_velocity[3];
} PhysicsData_C;

void engine_set_physics(uint64_t entity_id, const PhysicsData_C *data) {
  printf("[C Bridge Stub] Set physics for entity %llu\n", entity_id);
}

void engine_get_physics(uint64_t entity_id, PhysicsData_C *data) {
  if (data) {
    data->mass = 1.0f;
    data->drag = 0.0f;
    data->angular_drag = 0.05f;
    data->use_gravity = true;
    data->is_kinematic = false;
    data->velocity[0] = 0.0f;
    data->velocity[1] = 0.0f;
    data->velocity[2] = 0.0f;
    data->angular_velocity[0] = 0.0f;
    data->angular_velocity[1] = 0.0f;
    data->angular_velocity[2] = 0.0f;
  }
}

// ============================================================================
// Scene Management
// ============================================================================

void engine_save_scene(const char *path) {
  printf("[C Bridge Stub] Saved scene to '%s'\n", path ? path : "unknown");
}

void engine_load_scene(const char *path) {
  printf("[C Bridge Stub] Loaded scene from '%s'\n", path ? path : "unknown");
}

void engine_new_scene(void) { printf("[C Bridge Stub] Created new scene\n"); }

// ============================================================================
// Logging
// ============================================================================

void engine_log(int32_t level, const char *source, const char *message) {
  const char *level_str[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
  if (level >= 0 && level < 4) {
    printf("[C Bridge] [%s] [%s] %s\n", level_str[level],
           source ? source : "Unknown", message ? message : "");
  }
}

// ============================================================================
// Selection & Gizmos
// ============================================================================

void engine_set_selected_entity(uint64_t entity_id) {
  printf("[C Bridge Stub] Selected entity %llu\n", entity_id);
}

void engine_set_gizmo_mode(int32_t mode) {
  printf("[C Bridge Stub] Set gizmo mode to %d\n", mode);
}

void engine_set_gizmo_space(int32_t space) {
  printf("[C Bridge Stub] Set gizmo space to %d\n", space);
}

// ============================================================================
// Snapping
// ============================================================================

void engine_set_position_snap_enabled(bool enabled) {
  // Silent
}

void engine_set_position_snap_increment(float increment) {
  // Silent
}

void engine_set_rotation_snap_enabled(bool enabled) {
  // Silent
}

void engine_set_rotation_snap_increment(float increment) {
  // Silent
}

void engine_set_scale_snap_enabled(bool enabled) {
  // Silent
}

void engine_set_scale_snap_increment(float increment) {
  // Silent
}

// ============================================================================
// Camera
// ============================================================================

void engine_set_editor_camera_position(float x, float y, float z) {
  // Silent
}

void engine_set_editor_camera_rotation(float pitch, float yaw) {
  // Silent
}

// ============================================================================
// Profiler
// ============================================================================

void engine_profiler_begin_frame(void) {
  // Silent
}

void engine_profiler_end_frame(void) {
  // Silent
}
// (Duplicate removed - see line 113)

bool engine_entity_exists(uint64_t entity_id) {
  return (entity_id > 0 &&
          entity_id < 1000); // Stub: assume entities 1-999 exist
}

uint32_t engine_get_all_entities(uint64_t *out_entities, uint32_t max_count) {
  if (!out_entities || max_count == 0)
    return 0;

  // Return demo entities
  uint32_t count = (max_count < 10) ? max_count : 10;
  for (uint32_t i = 0; i < count; i++) {
    out_entities[i] = i + 1;
  }
  return count;
}

// ============================================================================
// Entity Properties
// ============================================================================

bool engine_get_entity_active(uint64_t entity_id) { return true; }
bool engine_get_entity_static(uint64_t entity_id) { return false; }
const char *engine_get_entity_tag(uint64_t entity_id) { return "Untagged"; }
const char *engine_get_entity_layer(uint64_t entity_id) { return "Default"; }

void engine_set_entity_active(uint64_t entity_id, bool active) {}
void engine_set_entity_static(uint64_t entity_id, bool is_static) {}
void engine_set_entity_tag(uint64_t entity_id, const char *tag) {}
void engine_set_entity_layer(uint64_t entity_id, const char *layer) {}

int32_t engine_get_component_count(uint64_t entity_id) { return 3; }
void engine_get_component_types(uint64_t entity_id, int32_t *out_types,
                                int32_t max_count) {
  if (out_types && max_count >= 3) {
    out_types[0] = 0; // Transform
    out_types[1] = 1; // MeshRenderer
    out_types[2] = 2; // Physics
  }
}

// ============================================================================
// Mesh Overlay & Visualization
// ============================================================================

void engine_set_mesh_overlay_color(uint64_t entity_id, float r, float g,
                                   float b, float a) {
  printf("[C Bridge] Set mesh overlay color for entity %llu: (%.2f, %.2f, "
         "%.2f, %.2f)\n",
         entity_id, r, g, b, a);
}

void engine_set_mesh_wireframe_enabled(uint64_t entity_id, bool enabled) {
  printf("[C Bridge] Set mesh wireframe %s for entity %llu\n",
         enabled ? "enabled" : "disabled", entity_id);
}

void engine_set_mesh_vertex_colors_enabled(uint64_t entity_id, bool enabled) {
  printf("[C Bridge] Set vertex colors %s for entity %llu\n",
         enabled ? "enabled" : "disabled", entity_id);
}

void engine_set_mesh_uv_visualization(uint64_t entity_id, int32_t mode) {
  printf("[C Bridge] Set UV visualization mode %d for entity %llu\n", mode,
         entity_id);
}

void engine_set_mesh_normals_visualization(uint64_t entity_id, bool enabled) {
  printf("[C Bridge] Set normals visualization %s for entity %llu\n",
         enabled ? "enabled" : "disabled", entity_id);
}

void engine_set_mesh_bounds_visualization(uint64_t entity_id, bool enabled) {
  printf("[C Bridge] Set bounds visualization %s for entity %llu\n",
         enabled ? "enabled" : "disabled", entity_id);
}

void engine_set_material_override(uint64_t entity_id, uint64_t material_id) {
  printf("[C Bridge] Set material override %llu for entity %llu\n", material_id,
         entity_id);
}

void engine_clear_material_override(uint64_t entity_id) {
  printf("[C Bridge] Cleared material override for entity %llu\n", entity_id);
}

// ============================================================================
// Selection Visualization
// ============================================================================

void engine_set_selection_outline_color(float r, float g, float b) {
  printf("[C Bridge] Set selection outline color: (%.2f, %.2f, %.2f)\n", r, g,
         b);
}

void engine_set_selection_outline_width(float width) {
  printf("[C Bridge] Set selection outline width: %.2f\n", width);
}

void engine_set_hover_highlight_enabled(bool enabled) {
  printf("[C Bridge] Set hover highlight %s\n",
         enabled ? "enabled" : "disabled");
}

// ============================================================================
// Render Settings
// ============================================================================

void engine_set_render_mode(int32_t mode) {
  const char *mode_names[] = {
      "Shaded", "Wireframe", "ShadedWireframe", "Unlit",    "Overdraw",
      "Albedo", "Normals",   "Smoothness",      "Metallic", "AO"};
  if (mode >= 0 && mode < 10) {
    printf("[C Bridge] Set render mode: %s\n", mode_names[mode]);
  }
}

void engine_set_lighting_enabled(bool enabled) {
  printf("[C Bridge] Set lighting %s\n", enabled ? "enabled" : "disabled");
}

void engine_set_shadows_enabled(bool enabled) {
  printf("[C Bridge] Set shadows %s\n", enabled ? "enabled" : "disabled");
}

void engine_set_ambient_occlusion_enabled(bool enabled) {
  printf("[C Bridge] Set ambient occlusion %s\n",
         enabled ? "enabled" : "disabled");
}
