#pragma once

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec2 Vec2;
typedef struct Vec3 Vec3;
typedef struct Entity Entity;

// Drag and drop data types
typedef enum {
    DRAG_DATA_TYPE_ASSET,
    DRAG_DATA_TYPE_ENTITY,
    DRAG_DATA_TYPE_FILE,
    DRAG_DATA_TYPE_TEXT,
    DRAG_DATA_TYPE_CUSTOM
} DragDataType;

// Drag and drop state
typedef enum {
    DRAG_STATE_IDLE,
    DRAG_STATE_DRAGGING,
    DRAG_STATE_HOVERING,
    DRAG_STATE_DROPPING
} DragState;

// Drag and drop data payload
typedef struct {
    DragDataType type;
    void* data;
    size_t data_size;
    char description[128];
    char icon_path[256];
} DragData;

// Drop target interface
typedef struct {
    bool (*can_accept)(void* target, DragData* drag_data);
    void (*on_drop)(void* target, DragData* drag_data, Vec2 position);
    void (*on_hover)(void* target, DragData* drag_data, Vec2 position);
    void (*on_leave)(void* target);
    void* user_data;
} DropTarget;

// Drag and drop context
typedef struct {
    DragState state;
    DragData current_drag;
    Vec2 start_position;
    Vec2 current_position;
    Vec2 delta_position;
    
    DropTarget* current_target;
    DropTarget* potential_targets[16];
    u32 target_count;
    
    bool is_dragging;
    f32 drag_start_time;
    f32 drag_threshold;
    
    // Visual feedback
    bool show_ghost;
    Vec3 ghost_position;
    Entity ghost_entity;
} DragDropContext;

// MARK: - Drag and Drop System Management

bool drag_drop_init(void);
void drag_drop_shutdown(void);
void drag_drop_update(f32 delta_time);
void drag_drop_render(void);

DragDropContext* drag_drop_get_context(void);

// MARK: - Drag Operations

bool drag_drop_start_drag(DragData* data, Vec2 position);
void drag_drop_update_drag(Vec2 position);
bool drag_drop_end_drag(Vec2 position);
void drag_drop_cancel_drag(void);

// MARK: - Drop Target Management

void drag_drop_register_target(DropTarget* target);
void drag_drop_unregister_target(DropTarget* target);
void drag_drop_clear_targets(void);

// MARK: - Data Creation Helpers

DragData* drag_drop_create_asset_data(void* asset, const char* asset_name);
DragData* drag_drop_create_entity_data(Entity entity, const char* entity_name);
DragData* drag_drop_create_file_data(const char* file_path, const char* file_name);
DragData* drag_drop_create_text_data(const char* text, const char* description);
DragData* drag_drop_create_custom_data(void* data, size_t size, const char* description);

void drag_drop_free_data(DragData* data);

// MARK: - Utility Functions

const char* drag_drop_get_state_string(DragState state);
const char* drag_drop_get_type_string(DragDataType type);
bool drag_drop_is_dragging(void);
bool drag_drop_has_valid_target(void);
Vec2 drag_drop_get_drag_delta(void);

// MARK: - Configuration

void drag_drop_set_drag_threshold(f32 threshold);
void drag_drop_set_ghost_visibility(bool show);
void drag_drop_set_ghost_position(Vec3 position);
