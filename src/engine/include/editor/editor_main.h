#pragma once

#include <core/types.h>
#include <include/common.h>
#include <include/rendering/camera.h>
#include <math/mat4.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <stdbool.h>

struct VFS;
struct Engine;
struct AssetManager;
typedef struct IRenderer IRenderer;

// =================================================================================================
//                                    EDITOR SYSTEM ARCHITECTURE
// =================================================================================================

// Forward declarations
typedef struct EditorState EditorState;
typedef struct ViewportConfig ViewportConfig;
typedef struct Command Command;
typedef struct TransformGizmo TransformGizmo;
typedef struct PerformanceProfiler PerformanceProfiler;
typedef struct EditorMemoryProfiler EditorMemoryProfiler;
typedef struct EditorLayout EditorLayout;
typedef struct EditorWindow EditorWindow;

// =================================================================================================
//                                    MULTI-VIEWPORT SYSTEM
// =================================================================================================

typedef enum {
  VIEWPORT_SINGLE = 0,
  VIEWPORT_DUAL_HORIZONTAL,
  VIEWPORT_DUAL_VERTICAL,
  VIEWPORT_QUAD,
  VIEWPORT_COUNT
} ViewportLayout;

typedef enum {
  VIEWPORT_PERSPECTIVE = 0,
  VIEWPORT_TOP,
  VIEWPORT_FRONT,
  VIEWPORT_SIDE,
  VIEWPORT_COUNT_TYPE
} ViewportType;

struct ViewportConfig {
  ViewportType type;
  Camera camera;
  Vec2 position; // Position in window
  Vec2 size;     // Size in window
  bool is_maximized;
  bool is_visible;
  u32 render_texture; // Render target texture ID
  Mat4 view_matrix;
  Mat4 projection_matrix;
  f32 near_plane;
  f32 far_plane;
  f32 fov; // For perspective cameras
  bool is_orthographic;
  f32 ortho_size; // For orthographic cameras
};

typedef struct {
  ViewportLayout layout;
  ViewportConfig viewports[4]; // Max 4 viewports for quad layout
  u32 active_viewport;         // Currently focused viewport
  bool synchronize_selection;  // Keep selection visible in all views
  f32 split_ratio;             // Split ratio for dual layouts
} MultiViewportSystem;

// =================================================================================================
//                                    UNDO/REDO SYSTEM
// =================================================================================================

typedef enum {
  COMMAND_TRANSFORM = 0,
  COMMAND_CREATE,
  COMMAND_DELETE,
  COMMAND_PROPERTY_CHANGE,
  COMMAND_MATERIAL_CHANGE,
  COMMAND_COUNT
} CommandType;

typedef struct {
  Vec3 position;
  Vec3 rotation;
  Vec3 scale;
  u32 entity_id;
} TransformState;

typedef struct {
  u32 entity_id;
  char name[256];
  // Additional creation data
} CreateState;

typedef struct {
  u32 entity_id;
  // Serialized entity data for restoration
  void *entity_data;
  size_t data_size;
} DeleteState;

typedef struct {
  char property_name[128];
  void *old_value;
  void *new_value;
  size_t value_size;
  u32 entity_id;
} PropertyChangeState;

struct Command {
  CommandType type;
  union {
    TransformState transform;
    CreateState create;
    DeleteState delete;
    PropertyChangeState property;
  } data;
  void (*execute)(Command *cmd);
  void (*undo)(Command *cmd);
  void (*redo)(Command *cmd);
  void (*cleanup)(Command *cmd);
  const char *description;
  f64 timestamp;
};

typedef struct {
  Command commands[100];    // Command history stack
  u32 current_index;        // Current position in history
  u32 command_count;        // Total number of commands
  f64 total_execution_time; // Performance tracking
  bool is_recording;        // Whether to record commands
} UndoRedoSystem;

// =================================================================================================
//                                    PLAY-IN-EDITOR SYSTEM
// =================================================================================================

typedef enum {
  PLAY_STATE_STOPPED = 0,
  PLAY_STATE_PLAYING,
  PLAY_STATE_PAUSED,
  PLAY_STATE_STEPPING
} PlayState;

typedef struct {
  // Scene snapshot before play
  void *scene_snapshot;
  size_t snapshot_size;

  // Play state
  PlayState state;
  f64 play_start_time;
  f64 paused_time;
  f32 time_scale;

  // Hot-reload support
  bool hot_reload_enabled;
  u32 last_code_checksum;

  // Step debugging
  bool step_mode;
  u32 steps_per_frame;
  u32 current_step;
} PlayInEditorSystem;

// =================================================================================================
//                                    TRANSFORM GIZMOS
// =================================================================================================

typedef enum {
  GIZMO_NONE = 0,
  GIZMO_TRANSLATE,
  GIZMO_ROTATE,
  GIZMO_SCALE,
  GIZMO_COUNT
} GizmoMode;

typedef enum { GIZMO_SPACE_LOCAL = 0, GIZMO_SPACE_WORLD } GizmoSpace;

typedef enum {
  GIZMO_AXIS_X = 0,
  GIZMO_AXIS_Y,
  GIZMO_AXIS_Z,
  GIZMO_AXIS_XY,
  GIZMO_AXIS_XZ,
  GIZMO_AXIS_YZ,
  GIZMO_AXIS_ALL
} GizmoAxis;

struct TransformGizmo {
  GizmoMode mode;
  GizmoSpace space;
  GizmoAxis selected_axis;
  Vec3 position;
  f32 size;
  Vec4 color_x;
  Vec4 color_y;
  Vec4 color_z;
  Vec4 color_selected;
  bool is_active;
  bool snap_to_grid;
  f32 snap_value;

  // Interaction state
  Vec3 initial_position;
  Vec3 initial_mouse_pos;
  bool is_dragging;
};

// =================================================================================================
//                                    SELECTION OUTLINE SYSTEM
// =================================================================================================

typedef struct {
  u32 selected_entities[256]; // Max selected entities
  u32 selected_count;
  Vec4 outline_color;
  f32 outline_thickness;
  bool hover_enabled;
  u32 hovered_entity;
  Vec4 hover_color;

  // Rendering resources
  u32 stencil_shader;
  u32 outline_shader;
  u32 frame_buffer;
  u32 stencil_texture;
} SelectionOutlineSystem;

// =================================================================================================
//                                    PERFORMANCE PROFILER
// =================================================================================================

typedef struct {
  char name[64];
  f64 time_ms;
  f64 cpu_time;
  f64 gpu_time;
  u32 call_count;
  Vec4 color;
  bool is_visible;
} ProfileCategory;

struct PerformanceProfiler {
  f64 frame_times[360]; // 6 seconds at 60 FPS
  u32 frame_index;
  f64 current_frame_time;
  f64 average_frame_time;
  f64 min_frame_time;
  f64 max_frame_time;

  ProfileCategory categories[32];
  u32 category_count;

  // Memory tracking
  f64 memory_usage[360];
  f64 current_memory_usage;

  // Rendering stats
  u32 draw_calls;
  u32 triangle_count;
  u32 texture_switches;

  bool is_recording;
  f64 recording_start_time;
};

// =================================================================================================
//                                    MEMORY PROFILER
// =================================================================================================

typedef struct {
  char module_name[64];
  size_t allocated_bytes;
  size_t peak_bytes;
  u32 allocation_count;
  u32 deallocation_count;
  Vec4 color;
  bool is_visible;
} MemoryModule;

struct EditorMemoryProfiler {
  MemoryModule modules[64];
  u32 module_count;

  // Allocation timeline
  struct {
    size_t allocated;
    size_t freed;
    f64 timestamp;
  } allocation_history[1000];
  u32 history_index;

  // Leak detection
  struct {
    void *pointer;
    size_t size;
    const char *file;
    int line;
    f64 timestamp;
  } leak_tracking[1000];
  u32 leak_count;

  // Fragmentation map
  struct {
    void *start;
    size_t size;
    bool is_free;
  } memory_blocks[1000];
  u32 block_count;

  size_t total_allocated;
  size_t total_freed;
  size_t peak_memory;
};

// =================================================================================================
//                                    CUSTOMIZABLE LAYOUTS
// =================================================================================================

struct EditorWindow {
  bool is_open;
  Vec2 position;
  Vec2 size;
  bool is_docked;
  char dock_node[64];
};

struct EditorLayout {
  char name[64];
  char description[256];

  // Window states
  EditorWindow windows[16];
  u32 window_count;

  // Viewport configuration
  MultiViewportSystem viewport_config;

  // UI preferences
  f32 ui_scale;
  bool dark_mode;
  bool zen_mode;

  // Layout metadata
  f64 created_time;
  f64 modified_time;
  u32 version;
};

typedef struct LayoutSystem {
  EditorLayout layouts[8]; // Max 8 custom layouts
  u32 layout_count;
  u32 active_layout;
  char current_layout_name[64];
} LayoutSystem;

// =================================================================================================
//                                    MAIN EDITOR STATE
// =================================================================================================

typedef enum {
  COLOR_WINDOW_BG = 0,
  COLOR_WINDOW_TEXT,
  COLOR_BUTTON,
  COLOR_BUTTON_HOVER,
  COLOR_BUTTON_ACTIVE,
  COLOR_FRAME_BG,
  COLOR_FRAME_BORDER,
  COLOR_TITLE_BG,
  COLOR_TITLE_TEXT,
  COLOR_MENUBAR_BG,
  COLOR_COUNT
} EditorColor;

struct EditorState {
  bool is_initialized;

  // Window visibility
  bool show_demo_window;
  bool show_metrics_window;
  bool show_style_editor;
  bool show_asset_browser;
  bool show_viewport;
  bool show_outliner;
  bool show_inspector;
  bool show_console;
  bool show_performance_profiler;
  bool show_memory_profiler;

  // Docking layout state
  struct {
    bool dockspace_open;
    bool main_menu_bar_open;
    bool viewport_open;
    bool asset_browser_open;
    bool outliner_open;
    bool inspector_open;
  } docking;

  // UI state
  f32 ui_scale;
  bool dark_mode;
  bool zen_mode;
  u32 colors[COLOR_COUNT];

  // Font handles
  void *font_regular;
  void *font_bold;
  void *font_mono;

  // Thumbnail cache
  size_t thumbnail_cache_size;
  void *thumbnail_cache;

  // Enhanced systems
  MultiViewportSystem viewport_system;
  UndoRedoSystem undo_redo;
  PlayInEditorSystem play_system;
  TransformGizmo transform_gizmo;
  SelectionOutlineSystem selection_outline;
  PerformanceProfiler performance_profiler;
  EditorMemoryProfiler memory_profiler;
  LayoutSystem layout_system;
};

// =================================================================================================
//                                    CORE EDITOR API
// =================================================================================================

// Main editor lifecycle
void Editor_Init(void);
void Editor_Shutdown(void);
void Editor_Update(f32 delta_time);

// Preferences and settings
void editor_load_preferences(void);
void editor_save_preferences(void);
void editor_init_thumbnail_cache(void);
void editor_apply_zen_mode(void);

// Font and style management
void editor_init_fonts(void);
void editor_init_style(void);

// Docking and layout
void editor_dockspace_begin(void);
void editor_dockspace_end(void);

// Window rendering
void AssetBrowser_Init(struct VFS *vfs, struct AssetManager *assets);
void editor_main_menu_bar(void);
void editor_viewport_window(void);
void Editor_DrawAssetBrowser(void);
void editor_outliner_window(void);
void editor_inspector_window(void);
void editor_console_window(void);
void editor_metrics_window(void);
void editor_style_editor_window(void);
void editor_demo_window(void);
void Editor_DrawAssetBrowser(void);

// =================================================================================================
//                                    MULTI-VIEWPORT API
// =================================================================================================

void viewport_system_init(MultiViewportSystem *system);
void viewport_system_shutdown(MultiViewportSystem *system);
void viewport_system_update(MultiViewportSystem *system, f32 delta_time);
void viewport_system_render(MultiViewportSystem *system);
void viewport_set_layout(MultiViewportSystem *system, ViewportLayout layout);
void viewport_maximize(MultiViewportSystem *system, u32 viewport_index);
void viewport_restore_all(MultiViewportSystem *system);
void viewport_synchronize_selection(MultiViewportSystem *system, bool enable);

// =================================================================================================
//                                    UNDO/REDO API
// =================================================================================================

void undo_redo_system_init(UndoRedoSystem *system);
void undo_redo_system_shutdown(UndoRedoSystem *system);
void undo_redo_execute_command(UndoRedoSystem *system, Command *command);
void undo_redo_undo(UndoRedoSystem *system);
void undo_redo_redo(UndoRedoSystem *system);
void undo_redo_clear_history(UndoRedoSystem *system);
bool undo_redo_can_undo(UndoRedoSystem *system);
bool undo_redo_can_redo(UndoRedoSystem *system);

// Command creation helpers
Command *create_transform_command(u32 entity_id, Vec3 old_pos, Vec3 new_pos,
                                  Vec3 old_rot, Vec3 new_rot, Vec3 old_scale,
                                  Vec3 new_scale);
Command *create_create_command(u32 entity_id, const char *name);
Command *create_delete_command(u32 entity_id);
Command *create_property_change_command(u32 entity_id,
                                        const char *property_name,
                                        void *old_value, void *new_value,
                                        size_t value_size);

// =================================================================================================
//                                    PLAY-IN-EDITOR API
// =================================================================================================

void play_in_editor_init(PlayInEditorSystem *system);
void play_in_editor_shutdown(PlayInEditorSystem *system);
void play_in_editor_update(PlayInEditorSystem *system, f32 delta_time);
void play_in_editor_play(PlayInEditorSystem *system);
void play_in_editor_pause(PlayInEditorSystem *system);
void play_in_editor_stop(PlayInEditorSystem *system);
void play_in_editor_step(PlayInEditorSystem *system);
bool play_in_editor_is_playing(PlayInEditorSystem *system);

// =================================================================================================
//                                    TRANSFORM GIZMO API
// =================================================================================================

void transform_gizmo_init(TransformGizmo *gizmo);
void transform_gizmo_shutdown(TransformGizmo *gizmo);
void transform_gizmo_update(TransformGizmo *gizmo, const Vec3 *target_position);
void transform_gizmo_render(TransformGizmo *gizmo, IRenderer *renderer);
bool transform_gizmo_handle_input(TransformGizmo *gizmo, const Vec2 *mouse_pos,
                                  bool mouse_down);
void transform_gizmo_set_mode(TransformGizmo *gizmo, GizmoMode mode);
void transform_gizmo_set_space(TransformGizmo *gizmo, GizmoSpace space);

// =================================================================================================
//                                    SELECTION OUTLINE API
// =================================================================================================

void selection_outline_init(SelectionOutlineSystem *system);
void selection_outline_shutdown(SelectionOutlineSystem *system);
void selection_outline_update(SelectionOutlineSystem *system);
void selection_outline_render(SelectionOutlineSystem *system, IRenderer *renderer);
void selection_outline_set_selected(SelectionOutlineSystem *system,
                                    u32 *entities, u32 count);
void selection_outline_add_selected(SelectionOutlineSystem *system, u32 entity);
void selection_outline_remove_selected(SelectionOutlineSystem *system,
                                       u32 entity);
void selection_outline_clear_selected(SelectionOutlineSystem *system);
void selection_outline_set_hovered(SelectionOutlineSystem *system, u32 entity);

// =================================================================================================
//                                    PERFORMANCE PROFILER API
// =================================================================================================

void performance_profiler_init(PerformanceProfiler *profiler);
void performance_profiler_shutdown(PerformanceProfiler *profiler);
void performance_profiler_begin_frame(PerformanceProfiler *profiler);
void performance_profiler_end_frame(PerformanceProfiler *profiler);
void performance_profiler_begin_category(PerformanceProfiler *profiler,
                                         const char *name);
void performance_profiler_end_category(PerformanceProfiler *profiler,
                                       const char *name);
void performance_profiler_add_category(PerformanceProfiler *profiler,
                                       const char *name, Vec4 color);
void performance_profiler_render_window(PerformanceProfiler *profiler);

// =================================================================================================
//                                    MEMORY PROFILER API
// =================================================================================================

void editor_memory_profiler_init(EditorMemoryProfiler *profiler);
void editor_memory_profiler_shutdown(EditorMemoryProfiler *profiler);
void editor_memory_profiler_record_allocation(EditorMemoryProfiler *profiler,
                                              void *ptr, size_t size,
                                              const char *file, int line);
void editor_memory_profiler_record_deallocation(EditorMemoryProfiler *profiler,
                                                void *ptr);
void editor_memory_profiler_update(EditorMemoryProfiler *profiler);
void editor_memory_profiler_render_window(EditorMemoryProfiler *profiler);
void editor_memory_profiler_detect_leaks(EditorMemoryProfiler *profiler);
void editor_memory_profiler_generate_fragmentation_map(
    EditorMemoryProfiler *profiler);

// =================================================================================================
//                                    LAYOUT SYSTEM API
// =================================================================================================

void layout_system_init(LayoutSystem *system);
void layout_system_shutdown(LayoutSystem *system);
void layout_system_save_current(LayoutSystem *system, const char *name);
void layout_system_load(LayoutSystem *system, const char *name);
void layout_system_delete(LayoutSystem *system, const char *name);
void layout_system_reset_to_default(LayoutSystem *system);
void layout_system_apply(LayoutSystem *system, const EditorLayout *layout);
void layout_system_export(LayoutSystem *system, const char *filename);
void layout_system_import(LayoutSystem *system, const char *filename);

// =================================================================================================
//                                    EXTERNAL INTEGRATION
// =================================================================================================

// These functions would be provided by other engine systems
extern u32 get_asset_count(void);
extern const char *get_selected_asset_name(void);
extern u32 get_scene_node_count(void);
extern const char *get_selected_scene_node(void);
extern const char *get_selected_entity_name(void);
extern u32 get_selected_component_count(void);
extern f32 get_current_fps(void);
extern f64 get_frame_time_ms(void);
extern u32 get_memory_usage_mb(void);
extern u32 get_draw_call_count(void);

// =================================================================================================
//                                    CONSTANTS AND CONFIGURATION
// =================================================================================================

#define MAX_UNDO_COMMANDS 100
#define MAX_SELECTED_ENTITIES 256
#define MAX_VIEWPORTS 4
#define MAX_PROFILE_CATEGORIES 32
#define MAX_MEMORY_MODULES 64
#define MAX_LAYOUTS 8
#define MAX_WINDOWS_PER_LAYOUT 16
#define FRAME_HISTORY_SIZE 360
#define ALLOCATION_HISTORY_SIZE 1000
#define MEMORY_BLOCKS_SIZE 1000
#define LEAK_TRACKING_SIZE 1000

// Global editor state
extern EditorState g_editor;

// Performance targets
#define COMMAND_EXECUTION_TIME_TARGET 0.1 // ms
#define OUTLINE_RENDER_TIME_TARGET 0.5    // ms
#define FRAME_TIME_BUDGET 16.67           // ms (60 FPS)

// Editor configuration
#define DEFAULT_OUTLINE_COLOR (Vec4){0.0f, 1.0f, 0.0f, 1.0f}
#define DEFAULT_OUTLINE_THICKNESS 2.0f
#define DEFAULT_GIZMO_SIZE 1.0f
#define DEFAULT_SNAP_VALUE 0.5f
