// Editor/Landscape Editor Tool
#ifndef LANDSCAPE_EDITOR_H
#define LANDSCAPE_EDITOR_H

#include "include/common.h"
#include "math/vec3.h"

typedef enum {
  BRUSH_SCULPT,
  BRUSH_SMOOTH,
  BRUSH_FLATTEN,
  BRUSH_RAMP,
  BRUSH_EROSION
} LandscapeBrushType;

typedef struct {
  LandscapeBrushType active_brush;
  f32 brush_size;
  f32 brush_strength;
  f32 brush_falloff;

  // Layer painting
  int active_layer_index;
  bool paint_mode; // If false, sculpt mode

} LandscapeEditor;

#ifdef __cplusplus
extern "C" {
#endif

LandscapeEditor *landscape_editor_create(void);
void landscape_editor_apply_brush(LandscapeEditor *editor, Vec3 world_pos,
                                  f32 dt);

#ifdef __cplusplus
}
#endif

#endif
