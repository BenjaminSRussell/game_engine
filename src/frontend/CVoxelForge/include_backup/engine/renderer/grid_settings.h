// Grid and Snapping Settings
// Roadmap: grid_snapping_roadmap.md Phase 1-4

#ifndef GRID_SETTINGS_H
#define GRID_SETTINGS_H

#include <math.h>
#include <stdbool.h>

typedef struct GridSettings {
  // Grid visualization
  bool grid_enabled;
  float grid_spacing; // Units between grid lines (0.25, 0.5, 1.0, 5.0, 10.0)
  float grid_major_spacing; // Major grid line spacing (10x grid_spacing)
  float grid_opacity;       // 0.0 - 1.0
  float grid_color[4];      // RGBA
  float grid_fade_distance; // Distance at which grid starts fading

  // Position snapping
  bool snap_position_enabled;
  float snap_position_increment; // Snap increment in world units

  // Rotation snapping
  bool snap_rotation_enabled;
  float snap_rotation_increment; // Snap increment in degrees (5, 15, 45, 90)

  // Scale snapping
  bool snap_scale_enabled;
  float snap_scale_increment; // Snap increment (0.1, 0.25, 0.5, 1.0)

  // Advanced snapping
  bool snap_surface_enabled;  // Snap to surfaces via raycast
  bool snap_vertex_enabled;   // Snap to nearby vertices
  float snap_vertex_distance; // Maximum distance to snap to vertex

} GridSettings;

// Default settings
static inline GridSettings grid_settings_default(void) {
  GridSettings settings = {.grid_enabled = true,
                           .grid_spacing = 1.0f,
                           .grid_major_spacing = 10.0f,
                           .grid_opacity = 0.3f,
                           .grid_color = {0.5f, 0.5f, 0.5f, 1.0f},
                           .grid_fade_distance = 100.0f,

                           .snap_position_enabled = false,
                           .snap_position_increment = 1.0f,

                           .snap_rotation_enabled = false,
                           .snap_rotation_increment = 15.0f,

                           .snap_scale_enabled = false,
                           .snap_scale_increment = 0.25f,

                           .snap_surface_enabled = false,
                           .snap_vertex_enabled = false,
                           .snap_vertex_distance = 0.5f};
  return settings;
}

// Snap functions
static inline float snap_value(float value, float increment) {
  if (increment <= 0.0f)
    return value;
  return roundf(value / increment) * increment;
}

static inline void snap_position(float position[3], float increment) {
  position[0] = snap_value(position[0], increment);
  position[1] = snap_value(position[1], increment);
  position[2] = snap_value(position[2], increment);
}

static inline void snap_rotation(float rotation[3], float increment) {
  rotation[0] = snap_value(rotation[0], increment);
  rotation[1] = snap_value(rotation[1], increment);
  rotation[2] = snap_value(rotation[2], increment);
}

static inline void snap_scale(float scale[3], float increment) {
  scale[0] = snap_value(scale[0], increment);
  scale[1] = snap_value(scale[1], increment);
  scale[2] = snap_value(scale[2], increment);
}

#endif // GRID_SETTINGS_H
