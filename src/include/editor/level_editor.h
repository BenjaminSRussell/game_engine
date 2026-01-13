// Editor/Level Editor Tool
#ifndef LEVEL_EDITOR_H
#define LEVEL_EDITOR_H

#include "include/common.h"
#include "math/vec3.h"

typedef struct {
  void *selected_actor;
  bool is_dragging;
  Vec3 drag_start;

  // Tools
  enum { TOOL_SELECT, TOOL_TRANSLATE, TOOL_ROTATE, TOOL_SCALE } active_tool;
  bool snap_enabled;
  f32 snap_size;

  // Grid
  bool show_grid;
  f32 grid_size;

  // Camera
  Vec3 camera_pos;
  f32 camera_speed;

} LevelEditor;

#ifdef __cplusplus
extern "C" {
#endif

LevelEditor *level_editor_create(void);
void level_editor_update(LevelEditor *editor, f32 dt);
void level_editor_render_ui(LevelEditor *editor);
void level_editor_on_mouse_click(LevelEditor *editor, int button, bool down,
                                 f32 x, f32 y);

#ifdef __cplusplus
}
#endif

#endif
