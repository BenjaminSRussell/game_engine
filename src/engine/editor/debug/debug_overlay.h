// debug_overlay.h - Comprehensive Debugging Overlay System
#ifndef DEBUG_OVERLAY_H
#define DEBUG_OVERLAY_H

#include "engine/include/common.h"
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

#define DEBUG_OVERLAY_MAX_TEXT_LINES 32
#define DEBUG_OVERLAY_MAX_GRAPHS 8
#define DEBUG_OVERLAY_MAX_MARKERS 64
#define DEBUG_OVERLAY_MAX_LOG_ENTRIES 128

typedef enum {
  DEBUG_OVERLAY_TEXT = 0,
  DEBUG_OVERLAY_GRAPH,
  DEBUG_OVERLAY_MARKER,
  DEBUG_OVERLAY_LOG,
  DEBUG_OVERLAY_PERFORMANCE,
  DEBUG_OVERLAY_MEMORY,
  DEBUG_OVERLAY_RENDER_STATS,
  DEBUG_OVERLAY_PROFILER
} DebugOverlayType;

typedef enum {
  DEBUG_LOG_INFO = 0,
  DEBUG_LOG_WARNING,
  DEBUG_LOG_ERROR,
  DEBUG_LOG_DEBUG,
  DEBUG_LOG_SUCCESS
} DebugLogLevel;

typedef struct {
  Vec2 position;
  Vec2 size;
  Vec4 color;
  bool visible;
  u32 priority; // Higher = drawn on top
} DebugOverlayElement;

typedef struct {
  DebugOverlayElement base;
  char text[256];
  char font_name[64];
  f32 font_size;
  Vec4 text_color;
  bool has_background;
  Vec4 background_color;
  bool word_wrap;
} DebugTextElement;

typedef struct {
  DebugOverlayElement base;
  f32 values[256]; // Circular buffer
  u32 value_count;
  u32 max_values;
  Vec4 line_color;
  Vec4 background_color;
  f32 min_value;
  f32 max_value;
  bool auto_scale;
  char label[64];
  char unit[16];
} DebugGraphElement;

typedef struct {
  DebugOverlayElement base;
  Vec3 world_position;
  Vec3 world_size;
  Vec4 color;
  char label[64];
  bool show_label;
  bool show_distance;
} DebugMarkerElement;

typedef struct {
  DebugOverlayElement base;
  DebugLogLevel level;
  char message[256];
  f32 timestamp;
  u32 frame_number;
  bool auto_hide;
  f32 hide_delay;
} DebugLogEntry;

typedef struct {
  DebugOverlayElement base;

  // Performance metrics
  f32 fps;
  f32 frame_time;
  f32 cpu_usage;
  f32 memory_usage;
  f32 gpu_usage;
  f32 draw_calls;
  f32 triangles;

  // History for graphs
  f32 fps_history[256];
  f32 frame_time_history[256];
  u32 history_index;

  bool show_fps_graph;
  bool show_frame_time_graph;
  bool show_memory_graph;
} DebugPerformanceElement;

typedef struct {
  DebugOverlayElement base;

  // Memory stats
  size_t total_allocated;
  size_t total_freed;
  size_t current_usage;
  size_t peak_usage;
  u32 allocation_count;
  u32 free_count;

  // Allocation history
  size_t allocation_history[256];
  u32 allocation_history_index;

  bool show_allocation_graph;
  bool show_breakdown;
} DebugMemoryElement;

typedef struct {
  DebugTextElement text_elements[DEBUG_OVERLAY_MAX_TEXT_LINES];
  DebugGraphElement graph_elements[DEBUG_OVERLAY_MAX_GRAPHS];
  DebugMarkerElement marker_elements[DEBUG_OVERLAY_MAX_MARKERS];
  DebugLogEntry log_entries[DEBUG_OVERLAY_MAX_LOG_ENTRIES];
  DebugPerformanceElement performance;
  DebugMemoryElement memory;

  u32 text_count;
  u32 graph_count;
  u32 marker_count;
  u32 log_count;

  bool enabled;
  Vec2 screen_size;
  f32 global_scale;
  Vec4 global_color;

  // Layout settings
  Vec2 margin;
  Vec2 padding;
  f32 line_spacing;

  // Filtering
  DebugLogLevel min_log_level;
  bool show_performance;
  bool show_memory;
  bool show_markers;

} DebugOverlay;

// Core functions
bool debug_overlay_init(u32 screen_width, u32 screen_height);
void debug_overlay_shutdown(void);
DebugOverlay *debug_overlay_create(void);
void debug_overlay_destroy(DebugOverlay *overlay);

// Overlay management
void debug_overlay_set_enabled(DebugOverlay *overlay, bool enabled);
void debug_overlay_set_screen_size(DebugOverlay *overlay, u32 width,
                                   u32 height);
void debug_overlay_set_global_scale(DebugOverlay *overlay, f32 scale);
void debug_overlay_clear(DebugOverlay *overlay);

// Text elements
DebugTextElement *debug_overlay_add_text(DebugOverlay *overlay,
                                         const char *text, Vec2 position);
void debug_overlay_update_text(DebugOverlay *overlay, u32 index,
                               const char *text);
void debug_overlay_set_text_color(DebugOverlay *overlay, u32 index, Vec4 color);
void debug_overlay_set_text_position(DebugOverlay *overlay, u32 index,
                                     Vec2 position);

// Graph elements
DebugGraphElement *debug_overlay_add_graph(DebugOverlay *overlay,
                                           const char *label, Vec2 position,
                                           Vec2 size);
void debug_overlay_graph_add_value(DebugOverlay *overlay, u32 graph_index,
                                   f32 value);
void debug_overlay_graph_set_color(DebugOverlay *overlay, u32 graph_index,
                                   Vec4 color);
void debug_overlay_graph_set_range(DebugOverlay *overlay, u32 graph_index,
                                   f32 min_val, f32 max_val);

// Marker elements
DebugMarkerElement *debug_overlay_add_marker(DebugOverlay *overlay,
                                             Vec3 world_pos, Vec4 color);
void debug_overlay_marker_set_label(DebugOverlay *overlay, u32 marker_index,
                                    const char *label);
void debug_overlay_marker_update_position(DebugOverlay *overlay,
                                          u32 marker_index, Vec3 world_pos);

// Logging
void debug_overlay_log(DebugOverlay *overlay, DebugLogLevel level,
                       const char *message);
void debug_overlay_log_info(DebugOverlay *overlay, const char *message);
void debug_overlay_log_warning(DebugOverlay *overlay, const char *message);
void debug_overlay_log_error(DebugOverlay *overlay, const char *message);
void debug_overlay_log_debug(DebugOverlay *overlay, const char *message);
void debug_overlay_clear_log(DebugOverlay *overlay);

// Performance monitoring
void debug_overlay_update_performance(DebugOverlay *overlay, f32 fps,
                                      f32 frame_time, f32 cpu_usage,
                                      f32 memory_usage, f32 gpu_usage);
void debug_overlay_set_performance_visibility(DebugOverlay *overlay,
                                              bool show_fps,
                                              bool show_frame_time,
                                              bool show_memory);

// Memory monitoring
void debug_overlay_update_memory(DebugOverlay *overlay, size_t allocated,
                                 size_t freed, size_t current, size_t peak,
                                 u32 alloc_count, u32 free_count);
void debug_overlay_track_allocation(DebugOverlay *overlay, size_t size);

// Rendering
void debug_overlay_render(DebugOverlay *overlay, void *render_context);
void debug_overlay_render_text(DebugOverlay *overlay, void *render_context);
void debug_overlay_render_graphs(DebugOverlay *overlay, void *render_context);
void debug_overlay_render_markers(DebugOverlay *overlay, void *render_context);

// Utility functions
Vec2 debug_overlay_world_to_screen(const DebugOverlay *overlay, Vec3 world_pos);
bool debug_overlay_is_visible(const DebugOverlay *overlay,
                              const DebugOverlayElement *element);
void debug_overlay_sort_elements(DebugOverlay *overlay);

// Preset configurations
void debug_overlay_setup_performance_view(DebugOverlay *overlay);
void debug_overlay_setup_memory_view(DebugOverlay *overlay);
void debug_overlay_setup_development_view(DebugOverlay *overlay);
void debug_overlay_setup_minimal_view(DebugOverlay *overlay);

// Save/load configuration
bool debug_overlay_save_config(const DebugOverlay *overlay,
                               const char *filepath);
bool debug_overlay_load_config(DebugOverlay *overlay, const char *filepath);

// Internal helper functions (for rendering implementation)
static void debug_overlay_render_performance(DebugOverlay *overlay,
                                             void *render_context);
static void debug_overlay_render_memory(DebugOverlay *overlay,
                                        void *render_context);

#endif // DEBUG_OVERLAY_H
