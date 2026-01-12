// debug_overlay.c - Comprehensive Debugging Overlay Implementation
#include "debug_overlay.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static DebugOverlay *g_debug_overlay = NULL;

bool debug_overlay_init(u32 screen_width, u32 screen_height) {
  if (g_debug_overlay) {
    LOG_WARN("Debug overlay already initialized");
    return true;
  }
  
  g_debug_overlay = debug_overlay_create();
  if (!g_debug_overlay) {
    LOG_ERROR("Failed to create debug overlay");
    return false;
  }
  
  debug_overlay_set_screen_size(g_debug_overlay, screen_width, screen_height);
  
  LOG_INFO("Debug overlay initialized (%ux%u)", screen_width, screen_height);
  return true;
}

void debug_overlay_shutdown(void) {
  if (!g_debug_overlay) return;
  
  debug_overlay_destroy(g_debug_overlay);
  g_debug_overlay = NULL;
  
  LOG_INFO("Debug overlay shutdown");
}

DebugOverlay* debug_overlay_create(void) {
  DebugOverlay *overlay = (DebugOverlay*)calloc(1, sizeof(DebugOverlay));
  if (!overlay) {
    LOG_ERROR("Failed to allocate debug overlay");
    return NULL;
  }
  
  // Initialize default settings
  overlay->enabled = true;
  overlay->screen_size = (Vec2){1920.0f, 1080.0f};
  overlay->global_scale = 1.0f;
  overlay->global_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
  
  // Layout settings
  overlay->margin = (Vec2){10.0f, 10.0f};
  overlay->padding = (Vec2){5.0f, 5.0f};
  overlay->line_spacing = 2.0f;
  
  // Filtering
  overlay->min_log_level = DEBUG_LOG_INFO;
  overlay->show_performance = true;
  overlay->show_memory = true;
  overlay->show_markers = true;
  
  // Initialize performance element
  overlay->performance.base.visible = true;
  overlay->performance.base.position = (Vec2){10.0f, 10.0f};
  overlay->performance.base.size = (Vec2){300.0f, 150.0f};
  overlay->performance.base.color = (Vec4){0.0f, 0.0f, 0.0f, 0.8f};
  overlay->performance.base.priority = 100;
  overlay->performance.show_fps_graph = true;
  overlay->performance.show_frame_time_graph = false;
  overlay->performance.show_memory_graph = false;
  overlay->performance.history_index = 0;
  
  // Initialize memory element
  overlay->memory.base.visible = true;
  overlay->memory.base.position = (Vec2){320.0f, 10.0f};
  overlay->memory.base.size = (Vec2){250.0f, 100.0f};
  overlay->memory.base.color = (Vec4){0.0f, 0.0f, 0.0f, 0.8f};
  overlay->memory.base.priority = 99;
  overlay->memory.show_allocation_graph = true;
  overlay->memory.show_breakdown = false;
  overlay->memory.allocation_history_index = 0;
  
  LOG_INFO("Debug overlay created");
  return overlay;
}

void debug_overlay_destroy(DebugOverlay *overlay) {
  if (!overlay) return;
  
  free(overlay);
  LOG_INFO("Debug overlay destroyed");
}

void debug_overlay_set_enabled(DebugOverlay *overlay, bool enabled) {
  if (!overlay) return;
  overlay->enabled = enabled;
}

void debug_overlay_set_screen_size(DebugOverlay *overlay, u32 width, u32 height) {
  if (!overlay) return;
  overlay->screen_size = (Vec2){(f32)width, (f32)height};
}

void debug_overlay_set_global_scale(DebugOverlay *overlay, f32 scale) {
  if (!overlay) return;
  overlay->global_scale = scale;
}

void debug_overlay_clear(DebugOverlay *overlay) {
  if (!overlay) return;
  
  overlay->text_count = 0;
  overlay->graph_count = 0;
  overlay->marker_count = 0;
  overlay->log_count = 0;
}

DebugTextElement* debug_overlay_add_text(DebugOverlay *overlay, const char *text, Vec2 position) {
  if (!overlay || !text || overlay->text_count >= DEBUG_OVERLAY_MAX_TEXT_LINES) {
    return NULL;
  }
  
  DebugTextElement *element = &overlay->text_elements[overlay->text_count++];
  memset(element, 0, sizeof(DebugTextElement));
  
  element->base.position = position;
  element->base.size = (Vec2){200.0f, 20.0f};
  element->base.color = overlay->global_color;
  element->base.visible = true;
  element->base.priority = 50;
  
  strncpy(element->text, text, sizeof(element->text) - 1);
  strcpy(element->font_name, "Arial");
  element->font_size = 14.0f;
  element->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
  element->has_background = false;
  element->background_color = (Vec4){0.0f, 0.0f, 0.0f, 0.5f};
  element->word_wrap = false;
  
  return element;
}

void debug_overlay_update_text(DebugOverlay *overlay, u32 index, const char *text) {
  if (!overlay || !text || index >= overlay->text_count) return;
  
  DebugTextElement *element = &overlay->text_elements[index];
  strncpy(element->text, text, sizeof(element->text) - 1);
}

void debug_overlay_set_text_color(DebugOverlay *overlay, u32 index, Vec4 color) {
  if (!overlay || index >= overlay->text_count) return;
  
  overlay->text_elements[index].text_color = color;
}

void debug_overlay_set_text_position(DebugOverlay *overlay, u32 index, Vec2 position) {
  if (!overlay || index >= overlay->text_count) return;
  
  overlay->text_elements[index].base.position = position;
}

DebugGraphElement* debug_overlay_add_graph(DebugOverlay *overlay, const char *label, Vec2 position, Vec2 size) {
  if (!overlay || !label || overlay->graph_count >= DEBUG_OVERLAY_MAX_GRAPHS) {
    return NULL;
  }
  
  DebugGraphElement *element = &overlay->graph_elements[overlay->graph_count++];
  memset(element, 0, sizeof(DebugGraphElement));
  
  element->base.position = position;
  element->base.size = size;
  element->base.color = overlay->global_color;
  element->base.visible = true;
  element->base.priority = 30;
  
  strncpy(element->label, label, sizeof(element->label) - 1);
  element->line_color = (Vec4){0.0f, 1.0f, 0.0f, 1.0f};
  element->background_color = (Vec4){0.0f, 0.0f, 0.0f, 0.3f};
  element->min_value = 0.0f;
  element->max_value = 100.0f;
  element->auto_scale = true;
  element->max_values = 256;
  
  return element;
}

void debug_overlay_graph_add_value(DebugOverlay *overlay, u32 graph_index, f32 value) {
  if (!overlay || graph_index >= overlay->graph_count) return;
  
  DebugGraphElement *graph = &overlay->graph_elements[graph_index];
  
  // Add value to circular buffer
  graph->values[graph->value_count % graph->max_values] = value;
  graph->value_count++;
  
  // Auto-scale if enabled
  if (graph->auto_scale) {
    if (value < graph->min_value) graph->min_value = value * 0.9f;
    if (value > graph->max_value) graph->max_value = value * 1.1f;
  }
}

void debug_overlay_graph_set_color(DebugOverlay *overlay, u32 graph_index, Vec4 color) {
  if (!overlay || graph_index >= overlay->graph_count) return;
  
  overlay->graph_elements[graph_index].line_color = color;
}

void debug_overlay_graph_set_range(DebugOverlay *overlay, u32 graph_index, f32 min_val, f32 max_val) {
  if (!overlay || graph_index >= overlay->graph_count) return;
  
  DebugGraphElement *graph = &overlay->graph_elements[graph_index];
  graph->min_value = min_val;
  graph->max_value = max_val;
  graph->auto_scale = false;
}

DebugMarkerElement* debug_overlay_add_marker(DebugOverlay *overlay, Vec3 world_pos, Vec4 color) {
  if (!overlay || overlay->marker_count >= DEBUG_OVERLAY_MAX_MARKERS) {
    return NULL;
  }
  
  DebugMarkerElement *element = &overlay->marker_elements[overlay->marker_count++];
  memset(element, 0, sizeof(DebugMarkerElement));
  
  element->base.position = (Vec2){0.0f, 0.0f}; // Will be calculated during render
  element->base.size = (Vec2){10.0f, 10.0f};
  element->base.color = color;
  element->base.visible = true;
  element->base.priority = 80;
  
  element->world_position = world_pos;
  element->world_size = (Vec3){1.0f, 1.0f, 1.0f};
  element->show_label = false;
  element->show_distance = false;
  
  return element;
}

void debug_overlay_marker_set_label(DebugOverlay *overlay, u32 marker_index, const char *label) {
  if (!overlay || !label || marker_index >= overlay->marker_count) return;
  
  DebugMarkerElement *marker = &overlay->marker_elements[marker_index];
  strncpy(marker->label, label, sizeof(marker->label) - 1);
  marker->show_label = true;
}

void debug_overlay_marker_update_position(DebugOverlay *overlay, u32 marker_index, Vec3 world_pos) {
  if (!overlay || marker_index >= overlay->marker_count) return;
  
  overlay->marker_elements[marker_index].world_position = world_pos;
}

void debug_overlay_log(DebugOverlay *overlay, DebugLogLevel level, const char *message) {
  if (!overlay || !message || level < overlay->min_log_level) return;
  
  if (overlay->log_count >= DEBUG_OVERLAY_LOG_ENTRIES) {
    // Remove oldest entry
    for (u32 i = 0; i < overlay->log_count - 1; i++) {
      overlay->log_entries[i] = overlay->log_entries[i + 1];
    }
    overlay->log_count--;
  }
  
  DebugLogEntry *entry = &overlay->log_entries[overlay->log_count++];
  memset(entry, 0, sizeof(DebugLogEntry));
  
  entry->base.position = (Vec2){10.0f, 200.0f};
  entry->base.size = (Vec2){400.0f, 20.0f};
  entry->base.visible = true;
  entry->base.priority = 90;
  
  entry->level = level;
  strncpy(entry->message, message, sizeof(entry->message) - 1);
  entry->timestamp = (f32)time(NULL);
  entry->frame_number = 0; // Would be set by engine
  entry->auto_hide = true;
  entry->hide_delay = 5.0f; // 5 seconds
  
  // Set color based on level
  switch (level) {
    case DEBUG_LOG_INFO:
      entry->base.color = (Vec4){0.0f, 0.8f, 1.0f, 1.0f};
      break;
    case DEBUG_LOG_WARNING:
      entry->base.color = (Vec4){1.0f, 1.0f, 0.0f, 1.0f};
      break;
    case DEBUG_LOG_ERROR:
      entry->base.color = (Vec4){1.0f, 0.2f, 0.2f, 1.0f};
      break;
    case DEBUG_LOG_DEBUG:
      entry->base.color = (Vec4){0.6f, 0.6f, 0.6f, 1.0f};
      break;
    case DEBUG_LOG_SUCCESS:
      entry->base.color = (Vec4){0.2f, 1.0f, 0.2f, 1.0f};
      break;
  }
}

void debug_overlay_log_info(DebugOverlay *overlay, const char *message) {
  debug_overlay_log(overlay, DEBUG_LOG_INFO, message);
}

void debug_overlay_log_warning(DebugOverlay *overlay, const char *message) {
  debug_overlay_log(overlay, DEBUG_LOG_WARNING, message);
}

void debug_overlay_log_error(DebugOverlay *overlay, const char *message) {
  debug_overlay_log(overlay, DEBUG_LOG_ERROR, message);
}

void debug_overlay_log_debug(DebugOverlay *overlay, const char *message) {
  debug_overlay_log(overlay, DEBUG_LOG_DEBUG, message);
}

void debug_overlay_clear_log(DebugOverlay *overlay) {
  if (!overlay) return;
  overlay->log_count = 0;
}

void debug_overlay_update_performance(DebugOverlay *overlay, f32 fps, f32 frame_time, 
                                     f32 cpu_usage, f32 memory_usage, f32 gpu_usage) {
  if (!overlay) return;
  
  overlay->performance.fps = fps;
  overlay->performance.frame_time = frame_time;
  overlay->performance.cpu_usage = cpu_usage;
  overlay->performance.memory_usage = memory_usage;
  overlay->performance.gpu_usage = gpu_usage;
  
  // Update history
  u32 index = overlay->performance.history_index;
  overlay->performance.fps_history[index] = fps;
  overlay->performance.frame_time_history[index] = frame_time;
  overlay->performance.history_index = (index + 1) % 256;
}

void debug_overlay_set_performance_visibility(DebugOverlay *overlay, bool show_fps, bool show_frame_time, bool show_memory) {
  if (!overlay) return;
  
  overlay->performance.show_fps_graph = show_fps;
  overlay->performance.show_frame_time_graph = show_frame_time;
  overlay->performance.show_memory_graph = show_memory;
}

void debug_overlay_update_memory(DebugOverlay *overlay, size_t allocated, size_t freed, 
                                size_t current, size_t peak, u32 alloc_count, u32 free_count) {
  if (!overlay) return;
  
  overlay->memory.total_allocated = allocated;
  overlay->memory.total_freed = freed;
  overlay->memory.current_usage = current;
  overlay->memory.peak_usage = peak;
  overlay->memory.allocation_count = alloc_count;
  overlay->memory.free_count = free_count;
  
  // Update allocation history
  u32 index = overlay->memory.allocation_history_index;
  overlay->memory.allocation_history[index] = current;
  overlay->memory.allocation_history_index = (index + 1) % 256;
}

void debug_overlay_track_allocation(DebugOverlay *overlay, size_t size) {
  if (!overlay) return;
  overlay->memory.current_usage += size;
  overlay->memory.allocation_count++;
  
  if (overlay->memory.current_usage > overlay->memory.peak_usage) {
    overlay->memory.peak_usage = overlay->memory.current_usage;
  }
}

void debug_overlay_render(DebugOverlay *overlay, void *render_context) {
  if (!overlay || !overlay->enabled || !render_context) return;
  
  // Sort elements by priority
  debug_overlay_sort_elements(overlay);
  
  // Render different element types
  debug_overlay_render_text(overlay, render_context);
  debug_overlay_render_graphs(overlay, render_context);
  debug_overlay_render_markers(overlay, render_context);
  
  // Render performance and memory overlays
  if (overlay->show_performance) {
    debug_overlay_render_performance(overlay, render_context);
  }
  
  if (overlay->show_memory) {
    debug_overlay_render_memory(overlay, render_context);
  }
}

void debug_overlay_render_text(DebugOverlay *overlay, void *render_context) {
  // This would integrate with the actual rendering system
  // For now, we'll just log what would be rendered
  
  for (u32 i = 0; i < overlay->text_count; i++) {
    DebugTextElement *text = &overlay->text_elements[i];
    if (text->base.visible) {
      LOG_DEBUG("Rendering text: %s at (%.1f, %.1f)", text->text, 
                text->base.position.x, text->base.position.y);
    }
  }
}

void debug_overlay_render_graphs(DebugOverlay *overlay, void *render_context) {
  for (u32 i = 0; i < overlay->graph_count; i++) {
    DebugGraphElement *graph = &overlay->graph_elements[i];
    if (graph->base.visible) {
      LOG_DEBUG("Rendering graph: %s with %u values", graph->label, graph->value_count);
    }
  }
}

void debug_overlay_render_markers(DebugOverlay *overlay, void *render_context) {
  for (u32 i = 0; i < overlay->marker_count; i++) {
    DebugMarkerElement *marker = &overlay->marker_elements[i];
    if (marker->base.visible) {
      // Convert world position to screen position
      Vec2 screen_pos = debug_overlay_world_to_screen(overlay, marker->world_position);
      marker->base.position = screen_pos;
      
      LOG_DEBUG("Rendering marker at world (%.2f, %.2f, %.2f) -> screen (%.1f, %.1f)", 
                marker->world_position.x, marker->world_position.y, marker->world_position.z,
                screen_pos.x, screen_pos.y);
    }
  }
}

Vec2 debug_overlay_world_to_screen(const DebugOverlay *overlay, Vec3 world_pos) {
  // This would integrate with the camera system
  // For now, return a simple orthographic projection
  Vec2 screen_pos = {
    world_pos.x + overlay->screen_size.x * 0.5f,
    overlay->screen_size.y - (world_pos.y + overlay->screen_size.y * 0.5f)
  };
  return screen_pos;
}

bool debug_overlay_is_visible(const DebugOverlay *overlay, const DebugOverlayElement *element) {
  if (!overlay || !element) return false;
  
  return overlay->enabled && element->visible &&
         element->position.x < overlay->screen_size.x &&
         element->position.y < overlay->screen_size.y &&
         element->position.x + element->size.x >= 0.0f &&
         element->position.y + element->size.y >= 0.0f;
}

void debug_overlay_sort_elements(DebugOverlay *overlay) {
  if (!overlay) return;
  
  // Simple bubble sort by priority (higher priority = drawn on top)
  // In a real implementation, you'd use a more efficient sorting algorithm
  
  // Sort text elements
  for (u32 i = 0; i < overlay->text_count - 1; i++) {
    for (u32 j = 0; j < overlay->text_count - i - 1; j++) {
      if (overlay->text_elements[j].base.priority < overlay->text_elements[j + 1].base.priority) {
        DebugTextElement temp = overlay->text_elements[j];
        overlay->text_elements[j] = overlay->text_elements[j + 1];
        overlay->text_elements[j + 1] = temp;
      }
    }
  }
  
  // Sort graph elements
  for (u32 i = 0; i < overlay->graph_count - 1; i++) {
    for (u32 j = 0; j < overlay->graph_count - i - 1; j++) {
      if (overlay->graph_elements[j].base.priority < overlay->graph_elements[j + 1].base.priority) {
        DebugGraphElement temp = overlay->graph_elements[j];
        overlay->graph_elements[j] = overlay->graph_elements[j + 1];
        overlay->graph_elements[j + 1] = temp;
      }
    }
  }
  
  // Sort marker elements
  for (u32 i = 0; i < overlay->marker_count - 1; i++) {
    for (u32 j = 0; j < overlay->marker_count - i - 1; j++) {
      if (overlay->marker_elements[j].base.priority < overlay->marker_elements[j + 1].base.priority) {
        DebugMarkerElement temp = overlay->marker_elements[j];
        overlay->marker_elements[j] = overlay->marker_elements[j + 1];
        overlay->marker_elements[j + 1] = temp;
      }
    }
  }
}

void debug_overlay_setup_performance_view(DebugOverlay *overlay) {
  if (!overlay) return;
  
  debug_overlay_clear(overlay);
  
  // Add FPS graph
  DebugGraphElement *fps_graph = debug_overlay_add_graph(overlay, "FPS", (Vec2){10.0f, 10.0f}, (Vec2){200.0f, 60.0f});
  if (fps_graph) {
    fps_graph->line_color = (Vec4){0.0f, 1.0f, 0.0f, 1.0f};
    fps_graph->min_value = 0.0f;
    fps_graph->max_value = 120.0f;
  }
  
  // Add frame time graph
  DebugGraphElement *frame_graph = debug_overlay_add_graph(overlay, "Frame Time", (Vec2){220.0f, 10.0f}, (Vec2){200.0f, 60.0f});
  if (frame_graph) {
    frame_graph->line_color = (Vec4){1.0f, 0.5f, 0.0f, 1.0f};
    frame_graph->min_value = 0.0f;
    frame_graph->max_value = 33.0f; // 30 FPS
  }
  
  // Add performance text
  debug_overlay_add_text(overlay, "Performance Monitor", (Vec2){10.0f, 80.0f});
  
  overlay->show_performance = true;
  overlay->performance.show_fps_graph = true;
  overlay->performance.show_frame_time_graph = true;
}

void debug_overlay_setup_memory_view(DebugOverlay *overlay) {
  if (!overlay) return;
  
  debug_overlay_clear(overlay);
  
  // Add memory graph
  DebugGraphElement *memory_graph = debug_overlay_add_graph(overlay, "Memory Usage", (Vec2){10.0f, 10.0f}, (Vec2){300.0f, 80.0f});
  if (memory_graph) {
    memory_graph->line_color = (Vec4){1.0f, 0.0f, 1.0f, 1.0f};
    memory_graph->auto_scale = true;
  }
  
  // Add memory text
  debug_overlay_add_text(overlay, "Memory Monitor", (Vec2){10.0f, 100.0f});
  
  overlay->show_memory = true;
  overlay->memory.show_allocation_graph = true;
}

void debug_overlay_setup_development_view(DebugOverlay *overlay) {
  if (!overlay) return;
  
  debug_overlay_setup_performance_view(overlay);
  debug_overlay_setup_memory_view(overlay);
  
  // Add development-specific elements
  debug_overlay_add_text(overlay, "Development Mode", (Vec2){10.0f, 200.0f});
  
  // Enable all overlays
  overlay->show_performance = true;
  overlay->show_memory = true;
  overlay->show_markers = true;
  overlay->min_log_level = DEBUG_LOG_DEBUG;
}

void debug_overlay_setup_minimal_view(DebugOverlay *overlay) {
  if (!overlay) return;
  
  debug_overlay_clear(overlay);
  
  // Only show FPS
  DebugTextElement *fps_text = debug_overlay_add_text(overlay, "", (Vec2){10.0f, 10.0f});
  if (fps_text) {
    fps_text->has_background = true;
    fps_text->background_color = (Vec4){0.0f, 0.0f, 0.0f, 0.7f};
  }
  
  overlay->show_performance = true;
  overlay->performance.show_fps_graph = false;
  overlay->performance.show_frame_time_graph = false;
  overlay->show_memory = false;
}

bool debug_overlay_save_config(const DebugOverlay *overlay, const char *filepath) {
  if (!overlay || !filepath) return false;
  
  FILE *file = fopen(filepath, "w");
  if (!file) {
    LOG_ERROR("Failed to open debug overlay config file: %s", filepath);
    return false;
  }
  
  fprintf(file, "# Debug Overlay Configuration\n");
  fprintf(file, "enabled=%s\n", overlay->enabled ? "true" : "false");
  fprintf(file, "global_scale=%.2f\n", overlay->global_scale);
  fprintf(file, "show_performance=%s\n", overlay->show_performance ? "true" : "false");
  fprintf(file, "show_memory=%s\n", overlay->show_memory ? "true" : "false");
  fprintf(file, "show_markers=%s\n", overlay->show_markers ? "true" : "false");
  fprintf(file, "min_log_level=%d\n", overlay->min_log_level);
  
  fclose(file);
  LOG_INFO("Debug overlay config saved to: %s", filepath);
  return true;
}

bool debug_overlay_load_config(DebugOverlay *overlay, const char *filepath) {
  if (!overlay || !filepath) return false;
  
  FILE *file = fopen(filepath, "r");
  if (!file) {
    LOG_ERROR("Failed to open debug overlay config file: %s", filepath);
    return false;
  }
  
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    if (strstr(line, "enabled=")) {
      overlay->enabled = strstr(line, "true") != NULL;
    } else if (strstr(line, "global_scale=")) {
      sscanf(line, "global_scale=%f", &overlay->global_scale);
    } else if (strstr(line, "show_performance=")) {
      overlay->show_performance = strstr(line, "true") != NULL;
    } else if (strstr(line, "show_memory=")) {
      overlay->show_memory = strstr(line, "true") != NULL;
    } else if (strstr(line, "show_markers=")) {
      overlay->show_markers = strstr(line, "true") != NULL;
    } else if (strstr(line, "min_log_level=")) {
      sscanf(line, "min_log_level=%d", (int*)&overlay->min_log_level);
    }
  }
  
  fclose(file);
  LOG_INFO("Debug overlay config loaded from: %s", filepath);
  return true;
}

// Internal helper functions for rendering
static void debug_overlay_render_performance(DebugOverlay *overlay, void *render_context) {
  // This would render the performance overlay with FPS, frame time graphs, etc.
  LOG_DEBUG("Rendering performance overlay: FPS=%.1f, Frame Time=%.2fms", 
            overlay->performance.fps, overlay->performance.frame_time);
}

static void debug_overlay_render_memory(DebugOverlay *overlay, void *render_context) {
  // This would render the memory overlay with allocation graphs
  LOG_DEBUG("Rendering memory overlay: Current=%zu MB, Peak=%zu MB, Allocations=%u", 
            overlay->memory.current_usage / (1024 * 1024),
            overlay->memory.peak_usage / (1024 * 1024),
            overlay->memory.allocation_count);
}
