// Drag and Drop System implementation - STUBBED
#include "engine/include/core/logger.h"
#include <core/types.h>
#include <ecs/ecs.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Dummy types
typedef enum {
  DRAG_STATE_IDLE,
  DRAG_STATE_DRAGGING,
  DRAG_STATE_HOVERING,
  DRAG_STATE_DROPPING
} DragState;

typedef enum {
  DRAG_DATA_TYPE_ASSET,
  DRAG_DATA_TYPE_ENTITY,
  DRAG_DATA_TYPE_FILE,
  DRAG_DATA_TYPE_TEXT,
  DRAG_DATA_TYPE_CUSTOM
} DragDataType;

typedef struct {
  DragDataType type;
  void *data;
  size_t data_size;
  char description[64];
} DragData;

typedef struct DropTarget DropTarget;
struct DropTarget {
  bool (*can_accept)(void *, DragData *);
  void (*on_drop)(void *, DragData *, Vec2);
  void (*on_hover)(void *, DragData *, Vec2);
  void (*on_leave)(void *);
  void *user_data;
  u32 entity_id;
  Vec3 bounds_min;
  Vec3 bounds_max;
  Vec3 world_position;
  bool bounds_valid;
  u32 icon_texture_id;
};

typedef struct {
  DragState state;
  bool is_dragging;
  DragData current_drag;
  DropTarget *current_target;
  DropTarget *potential_targets[16];
  u32 target_count;
  Vec2 start_position;
  Vec2 current_position;
  Vec2 delta_position;
  f32 drag_start_time;
  f32 drag_time;
  f32 drag_threshold;
  bool show_ghost;
  Entity ghost_entity;
  Vec3 ghost_position;
} DragDropContext;

static DragDropContext g_context = {0};
static bool g_initialized = false;

// Helpers
void drag_drop_free_data(DragData *data);

// Implementations

bool drag_drop_init(void) {
  if (g_initialized)
    return true;
  memset(&g_context, 0, sizeof(DragDropContext));
  g_initialized = true;
  return true;
}

void drag_drop_shutdown(void) { g_initialized = false; }

void drag_drop_update(f32 delta_time) {}
void drag_drop_render(void) {}

DragDropContext *drag_drop_get_context(void) { return &g_context; }

bool drag_drop_start_drag(DragData *data, Vec2 position) { return true; }
void drag_drop_update_drag(Vec2 position) {}
bool drag_drop_end_drag(Vec2 position) { return true; }
void drag_drop_cancel_drag(void) {}

void drag_drop_register_target(DropTarget *target) {}
void drag_drop_unregister_target(DropTarget *target) {}
void drag_drop_clear_targets(void) {}

DragData *drag_drop_create_asset_data(void *asset, const char *asset_name) {
  return NULL;
}
DragData *drag_drop_create_entity_data(Entity entity, const char *entity_name) {
  return NULL;
}
DragData *drag_drop_create_file_data(const char *file_path,
                                     const char *file_name) {
  return NULL;
}
DragData *drag_drop_create_text_data(const char *text,
                                     const char *description) {
  return NULL;
}
DragData *drag_drop_create_custom_data(void *data, size_t size,
                                       const char *description) {
  return NULL;
}

void drag_drop_free_data(DragData *data) {
  if (data && data->data)
    free(data->data);
  if (data)
    free(data);
}

// Legacy compatibility
void drag_drop_begin(void *item, const char *type) {}
void drag_drop_accept(const char *type, void *target) {}
void *drag_drop_get_payload(void) { return NULL; }

const char *drag_drop_get_state_string(DragState state) { return ""; }
const char *drag_drop_get_type_string(DragDataType type) { return ""; }
bool drag_drop_is_dragging(void) { return false; }
bool drag_drop_has_valid_target(void) { return false; }
Vec2 drag_drop_get_drag_delta(void) { return (Vec2){0, 0}; }

void drag_drop_set_drag_threshold(f32 threshold) {}
void drag_drop_set_ghost_visibility(bool show) {}
void drag_drop_set_ghost_position(Vec3 position) {}
