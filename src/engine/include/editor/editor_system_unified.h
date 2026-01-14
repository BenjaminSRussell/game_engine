// include/editor/editor_system_unified.h
//
// Purpose: Unified editor system consolidating all editor subsystems
// This replaces multiple disparate editor APIs with a single interface

#ifndef EDITOR_SYSTEM_UNIFIED_H
#define EDITOR_SYSTEM_UNIFIED_H

#include <common.h>
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "rendering/camera.h"
#include "ecs/ecs.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UNIFIED EDITOR CONFIGURATION
// ============================================================================

typedef struct {
    // UI Configuration
    float ui_scale;
    bool dark_mode;
    bool zen_mode;
    bool show_performance_overlay;
    bool show_memory_overlay;
    
    // Viewport Configuration
    bool multi_viewport_enabled;
    bool grid_enabled;
    bool gizmos_enabled;
    bool outlines_enabled;
    
    // Editor Features
    bool auto_save_enabled;
    float auto_save_interval_seconds;
    bool hot_reload_enabled;
    bool play_in_editor_enabled;
    
    // Performance Settings
    uint32_t max_undo_commands;
    uint32_t max_selected_entities;
    float target_frame_rate;
    bool enable_profiling;
} EditorConfig;

// ============================================================================
// UNIFIED EDITOR TYPES
// ============================================================================

typedef enum {
    EDITOR_MODE_EDIT = 0,
    EDITOR_MODE_PLAY,
    EDITOR_MODE_PAUSE,
    EDITOR_MODE_STEP
} EditorMode;

typedef enum {
    GIZMO_MODE_NONE = 0,
    GIZMO_MODE_TRANSLATE,
    GIZMO_MODE_ROTATE,
    GIZMO_MODE_SCALE,
    GIZMO_MODE_COUNT
} GizmoMode;

typedef enum {
    GIZMO_SPACE_LOCAL = 0,
    GIZMO_SPACE_WORLD
} GizmoSpace;

typedef enum {
    COMMAND_TYPE_TRANSFORM = 0,
    COMMAND_TYPE_CREATE,
    COMMAND_TYPE_DELETE,
    COMMAND_TYPE_PROPERTY_CHANGE,
    COMMAND_TYPE_MATERIAL_CHANGE,
    COMMAND_TYPE_COUNT
} CommandType;

typedef enum {
    VIEWPORT_LAYOUT_SINGLE = 0,
    VIEWPORT_LAYOUT_DUAL_HORIZONTAL,
    VIEWPORT_LAYOUT_DUAL_VERTICAL,
    VIEWPORT_LAYOUT_QUAD,
    VIEWPORT_LAYOUT_COUNT
} ViewportLayout;

// ============================================================================
// ENTITY SELECTION SYSTEM
// ============================================================================

typedef struct {
    Entity* selected_entities;
    uint32_t selected_count;
    uint32_t max_selected;
    Entity hovered_entity;
    bool multi_select_enabled;
} EntitySelectionSystem;

// ============================================================================
// TRANSFORM GIZMO SYSTEM
// ============================================================================

typedef struct {
    GizmoMode mode;
    GizmoSpace space;
    Vec3 position;
    float size;
    Vec4 color_x;
    Vec4 color_y;
    Vec4 color_z;
    Vec4 color_selected;
    bool is_active;
    bool snap_to_grid;
    float snap_value;
    
    // Interaction state
    Vec3 initial_position;
    Vec3 initial_mouse_pos;
    bool is_dragging;
} TransformGizmoSystem;

// ============================================================================
// MULTI-VIEWPORT SYSTEM
// ============================================================================

typedef struct {
    ViewportLayout layout;
    Camera viewports[4];
    Vec2 viewport_positions[4];
    Vec2 viewport_sizes[4];
    uint32_t active_viewport;
    bool synchronize_selection;
    float split_ratio;
    bool maximized_viewport;
    uint32_t maximized_index;
} MultiViewportSystem;

// ============================================================================
// UNDO/REDO SYSTEM
// ============================================================================

typedef struct {
    CommandType type;
    uint32_t entity_id;
    void* old_data;
    void* new_data;
    size_t data_size;
    void (*execute)(void* command);
    void (*undo)(void* command);
    void (*redo)(void* command);
    void (*cleanup)(void* command);
    const char* description;
    double timestamp;
} EditorCommand;

typedef struct {
    EditorCommand* commands;
    uint32_t max_commands;
    uint32_t current_index;
    uint32_t command_count;
    bool is_recording;
    double total_execution_time;
} UndoRedoSystem;

// ============================================================================
// PLAY-IN-EDITOR SYSTEM
// ============================================================================

typedef struct {
    // Scene snapshot before play
    void* scene_snapshot;
    size_t snapshot_size;
    
    // Play state
    EditorMode mode;
    double play_start_time;
    double paused_time;
    float time_scale;
    
    // Hot-reload support
    bool hot_reload_enabled;
    uint32_t last_code_checksum;
    
    // Step debugging
    bool step_mode;
    uint32_t steps_per_frame;
    uint32_t current_step;
} PlayInEditorSystem;

// ============================================================================
// PERFORMANCE PROFILER
// ============================================================================

typedef struct {
    char name[64];
    double time_ms;
    double cpu_time;
    double gpu_time;
    uint32_t call_count;
    Vec4 color;
    bool is_visible;
} ProfileCategory;

typedef struct {
    double frame_times[360]; // 6 seconds at 60 FPS
    uint32_t frame_index;
    double current_frame_time;
    double average_frame_time;
    double min_frame_time;
    double max_frame_time;
    
    ProfileCategory categories[32];
    uint32_t category_count;
    
    // Memory tracking
    double memory_usage[360];
    double current_memory_usage;
    
    // Rendering stats
    uint32_t draw_calls;
    uint32_t triangle_count;
    uint32_t texture_switches;
    
    bool is_recording;
    double recording_start_time;
} PerformanceProfiler;

// ============================================================================
// MEMORY PROFILER
// ============================================================================

typedef struct {
    char module_name[64];
    size_t allocated_bytes;
    size_t peak_bytes;
    uint32_t allocation_count;
    uint32_t deallocation_count;
    Vec4 color;
    bool is_visible;
} MemoryModule;

typedef struct {
    MemoryModule modules[64];
    uint32_t module_count;
    
    // Allocation timeline
    struct {
        size_t allocated;
        size_t freed;
        double timestamp;
    } allocation_history[1000];
    uint32_t history_index;
    
    // Leak detection
    struct {
        void* pointer;
        size_t size;
        const char* file;
        int line;
        double timestamp;
    } leak_tracking[1000];
    uint32_t leak_count;
    
    size_t total_allocated;
    size_t total_freed;
    size_t peak_memory;
} MemoryProfiler;

// ============================================================================
// UNIFIED EDITOR SYSTEM
// ============================================================================

typedef struct EditorSystem {
    // Configuration and state
    EditorConfig config;
    EditorMode mode;
    bool is_initialized;
    
    // Core subsystems
    EntitySelectionSystem selection;
    TransformGizmoSystem gizmo;
    MultiViewportSystem viewport;
    UndoRedoSystem undo_redo;
    PlayInEditorSystem play_system;
    PerformanceProfiler performance_profiler;
    MemoryProfiler memory_profiler;
    
    // UI state
    bool show_asset_browser;
    bool show_outliner;
    bool show_inspector;
    bool show_console;
    bool show_performance_profiler;
    bool show_memory_profiler;
    
    // Font and rendering resources
    void* font_regular;
    void* font_bold;
    void* font_mono;
    
    // Thumbnail cache
    size_t thumbnail_cache_size;
    void* thumbnail_cache;
    
    // Platform-specific data
    void* platform_data;
} EditorSystem;

// ============================================================================
// MAIN EDITOR API
// ============================================================================

// System management
EditorSystem* editor_system_create(const EditorConfig* config);
void editor_system_destroy(EditorSystem* editor);
bool editor_system_is_initialized(EditorSystem* editor);
void editor_system_update(EditorSystem* editor, float delta_time);
void editor_system_render(EditorSystem* editor);

// Mode management
void editor_set_mode(EditorSystem* editor, EditorMode mode);
EditorMode editor_get_mode(EditorSystem* editor);
void editor_play(EditorSystem* editor);
void editor_pause(EditorSystem* editor);
void editor_stop(EditorSystem* editor);
void editor_step(EditorSystem* editor);

// ============================================================================
// ENTITY SELECTION API
// ============================================================================

void editor_selection_init(EditorSystem* editor);
void editor_selection_shutdown(EditorSystem* editor);
void editor_selection_add(EditorSystem* editor, Entity entity);
void editor_selection_remove(EditorSystem* editor, Entity entity);
void editor_selection_clear(EditorSystem* editor);
void editor_selection_set(EditorSystem* editor, Entity* entities, uint32_t count);
bool editor_selection_contains(EditorSystem* editor, Entity entity);
uint32_t editor_selection_get_count(EditorSystem* editor);
Entity* editor_selection_get_entities(EditorSystem* editor);
void editor_selection_set_hovered(EditorSystem* editor, Entity entity);
Entity editor_selection_get_hovered(EditorSystem* editor);

// ============================================================================
// TRANSFORM GIZMO API
// ============================================================================

void editor_gizmo_init(EditorSystem* editor);
void editor_gizmo_shutdown(EditorSystem* editor);
void editor_gizmo_update(EditorSystem* editor, const Vec3* target_position);
void editor_gizmo_render(EditorSystem* editor);
bool editor_gizmo_handle_input(EditorSystem* editor, const Vec2* mouse_pos, bool mouse_down);
void editor_gizmo_set_mode(EditorSystem* editor, GizmoMode mode);
void editor_gizmo_set_space(EditorSystem* editor, GizmoSpace space);
GizmoMode editor_gizmo_get_mode(EditorSystem* editor);
GizmoSpace editor_gizmo_get_space(EditorSystem* editor);

// ============================================================================
// MULTI-VIEWPORT API
// ============================================================================

void editor_viewport_init(EditorSystem* editor);
void editor_viewport_shutdown(EditorSystem* editor);
void editor_viewport_update(EditorSystem* editor, float delta_time);
void editor_viewport_render(EditorSystem* editor);
void editor_viewport_set_layout(EditorSystem* editor, ViewportLayout layout);
void editor_viewport_maximize(EditorSystem* editor, uint32_t viewport_index);
void editor_viewport_restore_all(EditorSystem* editor);
void editor_viewport_synchronize_selection(EditorSystem* editor, bool enable);
ViewportLayout editor_viewport_get_layout(EditorSystem* editor);
uint32_t editor_viewport_get_active(EditorSystem* editor);

// ============================================================================
// UNDO/REDO API
// ============================================================================

void editor_undo_redo_init(EditorSystem* editor);
void editor_undo_redo_shutdown(EditorSystem* editor);
void editor_undo_redo_execute_command(EditorSystem* editor, EditorCommand* command);
void editor_undo(EditorSystem* editor);
void editor_redo(EditorSystem* editor);
void editor_undo_redo_clear_history(EditorSystem* editor);
bool editor_can_undo(EditorSystem* editor);
bool editor_can_redo(EditorSystem* editor);

// Command creation helpers
EditorCommand* editor_create_transform_command(EditorSystem* editor, uint32_t entity_id, 
                                          Vec3 old_pos, Vec3 new_pos,
                                          Vec3 old_rot, Vec3 new_rot, 
                                          Vec3 old_scale, Vec3 new_scale);
EditorCommand* editor_create_create_command(EditorSystem* editor, uint32_t entity_id, const char* name);
EditorCommand* editor_create_delete_command(EditorSystem* editor, uint32_t entity_id);
EditorCommand* editor_create_property_change_command(EditorSystem* editor, uint32_t entity_id,
                                               const char* property_name,
                                               void* old_value, void* new_value,
                                               size_t value_size);

// ============================================================================
// PERFORMANCE PROFILER API
// ============================================================================

void editor_performance_profiler_init(EditorSystem* editor);
void editor_performance_profiler_shutdown(EditorSystem* editor);
void editor_performance_profiler_begin_frame(EditorSystem* editor);
void editor_performance_profiler_end_frame(EditorSystem* editor);
void editor_performance_profiler_begin_category(EditorSystem* editor, const char* name);
void editor_performance_profiler_end_category(EditorSystem* editor, const char* name);
void editor_performance_profiler_add_category(EditorSystem* editor, const char* name, Vec4 color);
void editor_performance_profiler_render_window(EditorSystem* editor);

// ============================================================================
// MEMORY PROFILER API
// ============================================================================

void editor_memory_profiler_init(EditorSystem* editor);
void editor_memory_profiler_shutdown(EditorSystem* editor);
void editor_memory_profiler_record_allocation(EditorSystem* editor, void* ptr, size_t size,
                                         const char* file, int line);
void editor_memory_profiler_record_deallocation(EditorSystem* editor, void* ptr);
void editor_memory_profiler_update(EditorSystem* editor);
void editor_memory_profiler_render_window(EditorSystem* editor);
void editor_memory_profiler_detect_leaks(EditorSystem* editor);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Configuration
EditorConfig editor_create_default_config(void);
bool editor_validate_config(const EditorConfig* config);

// Preferences
void editor_load_preferences(EditorSystem* editor);
void editor_save_preferences(EditorSystem* editor);

// Asset browser integration
void editor_asset_browser_init(EditorSystem* editor);
void editor_asset_browser_shutdown(EditorSystem* editor);
void editor_asset_browser_update(EditorSystem* editor, float delta_time);
void editor_asset_browser_render(EditorSystem* editor);

// Console integration
void editor_console_init(EditorSystem* editor);
void editor_console_shutdown(EditorSystem* editor);
void editor_console_update(EditorSystem* editor, float delta_time);
void editor_console_render(EditorSystem* editor);
void editor_console_log(EditorSystem* editor, const char* message);

// Statistics and diagnostics
typedef struct {
    uint32_t total_entities;
    uint32_t selected_entities;
    uint32_t draw_calls;
    uint32_t triangle_count;
    float current_fps;
    double frame_time_ms;
    size_t memory_usage_mb;
    uint32_t undo_count;
    uint32_t redo_count;
} EditorStatistics;

EditorStatistics editor_get_statistics(EditorSystem* editor);
void editor_reset_statistics(EditorSystem* editor);

#ifdef __cplusplus
}
#endif

#endif // EDITOR_SYSTEM_UNIFIED_H
