#include <core/time_system.h>
#include <core/types.h>
#include <editor/editor_main.h>
#include <math.h>
#include <math/mat4.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper min/max if not defined
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

static f64 get_current_time_ms(void) {
  return time_get_high_res_time() * 1000.0;
}

// =================================================================================================
//                    MULTI-VIEWPORT SYSTEM IMPLEMENTATION
// =================================================================================================

void viewport_system_init(MultiViewportSystem *system) {
  printf("  [Viewport] Initializing multi-viewport system...\n");

  system->layout = VIEWPORT_SINGLE;
  system->active_viewport = 0;
  system->synchronize_selection = true;
  system->split_ratio = 0.5f;

  // Initialize default perspective viewport
  for (int i = 0; i < 4; i++) {
    system->viewports[i].type = VIEWPORT_PERSPECTIVE;
    system->viewports[i].position = (Vec2){0, 0};
    system->viewports[i].size = (Vec2){800, 600};
    system->viewports[i].is_maximized = false;
    system->viewports[i].is_visible = (i == 0);
    system->viewports[i].near_plane = 0.1f;
    system->viewports[i].far_plane = 1000.0f;
    system->viewports[i].fov = 45.0f;
    system->viewports[i].is_orthographic = false;
    system->viewports[i].ortho_size = 10.0f;

    // Set up different camera positions for each viewport type
    switch (i) {
    case 0: // Perspective
      system->viewports[i].camera.position = (Vec3){5, 5, 5};
      system->viewports[i].camera.yaw = -45.0f;
      system->viewports[i].camera.pitch = 45.0f;
      break;
    case 1: // Top
      system->viewports[i].type = VIEWPORT_TOP;
      system->viewports[i].camera.position = (Vec3){0, 10, 0};
      system->viewports[i].camera.yaw = -90.0f;
      system->viewports[i].camera.pitch = 0.0f;
      system->viewports[i].is_orthographic = true;
      break;
    case 2: // Front
      system->viewports[i].type = VIEWPORT_FRONT;
      system->viewports[i].camera.position = (Vec3){0, 5, 10};
      system->viewports[i].camera.yaw = 0.0f;
      system->viewports[i].camera.pitch = 0.0f;
      system->viewports[i].is_orthographic = true;
      break;
    case 3: // Side
      system->viewports[i].type = VIEWPORT_SIDE;
      system->viewports[i].camera.position = (Vec3){10, 5, 0};
      system->viewports[i].camera.yaw = 90.0f;
      system->viewports[i].camera.pitch = 0.0f;
      system->viewports[i].is_orthographic = true;
      break;
    }

    // Create render texture for each viewport
    system->viewports[i].render_texture = 1000 + i; // Placeholder texture IDs

    // Calculate view and projection matrices
    system->viewports[i].view_matrix = mat4_identity();
    system->viewports[i].projection_matrix = mat4_identity();
  }

  printf("  [Viewport] Multi-viewport system initialized\n");
}

void viewport_system_set_layout(MultiViewportSystem *system,
                                ViewportLayout layout) {
  if (system->layout == layout)
    return;

  system->layout = layout;

  switch (layout) {
  case VIEWPORT_SINGLE:
    system->viewports[0].is_visible = true;
    system->viewports[0].position = (Vec2){0, 0};
    system->viewports[0].size = (Vec2){800, 600};
    for (int i = 1; i < 4; i++) {
      system->viewports[i].is_visible = false;
    }
    break;

  case VIEWPORT_DUAL_HORIZONTAL:
    system->viewports[0].is_visible = true;
    system->viewports[0].position = (Vec2){0, 0};
    system->viewports[0].size = (Vec2){400, 600};
    system->viewports[1].is_visible = true;
    system->viewports[1].position = (Vec2){400, 0};
    system->viewports[1].size = (Vec2){400, 600};
    for (int i = 2; i < 4; i++) {
      system->viewports[i].is_visible = false;
    }
    break;

  case VIEWPORT_DUAL_VERTICAL:
    system->viewports[0].is_visible = true;
    system->viewports[0].position = (Vec2){0, 0};
    system->viewports[0].size = (Vec2){800, 300};
    system->viewports[1].is_visible = true;
    system->viewports[1].position = (Vec2){0, 300};
    system->viewports[1].size = (Vec2){800, 300};
    for (int i = 2; i < 4; i++) {
      system->viewports[i].is_visible = false;
    }
    break;

  case VIEWPORT_QUAD:
    for (int i = 0; i < 4; i++) {
      system->viewports[i].is_visible = true;
      int row = i / 2;
      int col = i % 2;
      system->viewports[i].position = (Vec2){col * 400, row * 300};
      system->viewports[i].size = (Vec2){400, 300};
    }
    break;

  case VIEWPORT_COUNT:
    break;
  }

  printf("  [Viewport] Layout changed to %d\n", layout);
}

// =================================================================================================
//                    UNDO/REDO SYSTEM IMPLEMENTATION
// =================================================================================================

void undo_redo_system_init(UndoRedoSystem *system) {
  printf("  [UndoRedo] Initializing undo/redo system...\n");

  memset(system->commands, 0, sizeof(system->commands));
  system->current_index = 0;
  system->command_count = 0;
  system->total_execution_time = 0.0;
  system->is_recording = true;

  printf("  [UndoRedo] System ready (max 100 commands)\n");
}

void undo_redo_execute_command(UndoRedoSystem *system, Command *command) {
  if (!system->is_recording)
    return;

  f64 start_time = get_current_time_ms();

  // Execute the command
  if (command->execute) {
    command->execute(command);
  }

  f64 execution_time = get_current_time_ms() - start_time;
  system->total_execution_time += execution_time;

  // Add to history (circular buffer)
  if (system->command_count >= MAX_UNDO_COMMANDS) {
    // Remove oldest command
    if (system->commands[system->current_index].cleanup) {
      system->commands[system->current_index].cleanup(
          &system->commands[system->current_index]);
    }
  } else {
    system->command_count++;
  }

  // Copy command to history
  system->commands[system->current_index] = *command;
  system->commands[system->current_index].timestamp = start_time;

  system->current_index = (system->current_index + 1) % MAX_UNDO_COMMANDS;

  printf("  [UndoRedo] Executed: %s (%.3fms)\n", command->description,
         execution_time);
}

void undo_redo_undo(UndoRedoSystem *system) {
  if (!undo_redo_can_undo(system))
    return;

  system->current_index =
      (system->current_index - 1 + MAX_UNDO_COMMANDS) % MAX_UNDO_COMMANDS;
  Command *cmd = &system->commands[system->current_index];

  if (cmd->undo) {
    f64 start_time = get_current_time_ms();
    cmd->undo(cmd);
    f64 undo_time = get_current_time_ms() - start_time;
    printf("  [UndoRedo] Undo: %s (%.3fms)\n", cmd->description, undo_time);
  }
}

void undo_redo_redo(UndoRedoSystem *system) {
  if (!undo_redo_can_redo(system))
    return;

  Command *cmd = &system->commands[system->current_index];

  if (cmd->redo) {
    f64 start_time = get_current_time_ms();
    cmd->redo(cmd);
    f64 redo_time = get_current_time_ms() - start_time;
    printf("  [UndoRedo] Redo: %s (%.3fms)\n", cmd->description, redo_time);
  }

  system->current_index = (system->current_index + 1) % MAX_UNDO_COMMANDS;
}

bool undo_redo_can_undo(UndoRedoSystem *system) {
  return system->command_count > 0;
}

bool undo_redo_can_redo(UndoRedoSystem *system) {
  return system->command_count > 0 &&
         system->current_index != (system->command_count % MAX_UNDO_COMMANDS);
}

// Command implementations
static void transform_command_execute(Command *cmd) {
  // Apply transform to entity
  printf("    Transform entity %d to new position\n",
         cmd->data.transform.entity_id);
}

static void transform_command_undo(Command *cmd) {
  // Restore old transform
  printf("    Restore entity %d transform\n", cmd->data.transform.entity_id);
}

static void transform_command_redo(Command *cmd) {
  // Re-apply transform
  transform_command_execute(cmd);
}

Command *create_transform_command(u32 entity_id, Vec3 old_pos, Vec3 new_pos,
                                  Vec3 old_rot, Vec3 new_rot, Vec3 old_scale,
                                  Vec3 new_scale) {
  Command *cmd = malloc(sizeof(Command));
  cmd->type = COMMAND_TRANSFORM;
  cmd->data.transform.entity_id = entity_id;
  cmd->data.transform.position = old_pos; // Store old for undo
  cmd->execute = transform_command_execute;
  cmd->undo = transform_command_undo;
  cmd->redo = transform_command_redo;
  cmd->cleanup = NULL;
  cmd->description = "Transform Entity";
  return cmd;
}

// =================================================================================================
//                    PLAY-IN-EDITOR SYSTEM IMPLEMENTATION
// =================================================================================================

void play_in_editor_init(PlayInEditorSystem *system) {
  printf("  [PlayInEditor] Initializing play-in-editor system...\n");

  system->scene_snapshot = NULL;
  system->snapshot_size = 0;
  system->state = PLAY_STATE_STOPPED;
  system->play_start_time = 0.0;
  system->paused_time = 0.0;
  system->time_scale = 1.0f;
  system->hot_reload_enabled = true;
  system->last_code_checksum = 0;
  system->step_mode = false;
  system->steps_per_frame = 1;
  system->current_step = 0;

  printf("  [PlayInEditor] System ready for hot-reload testing\n");
}

void play_in_editor_play(PlayInEditorSystem *system) {
  if (system->state == PLAY_STATE_PLAYING)
    return;

  printf("  [PlayInEditor] Starting play mode...\n");

  // Create scene snapshot
  if (!system->scene_snapshot) {
    system->snapshot_size = 1024 * 1024; // 1MB snapshot
    system->scene_snapshot = malloc(system->snapshot_size);
    printf("    Scene snapshot created (%zu bytes)\n", system->snapshot_size);
  }

  system->state = PLAY_STATE_PLAYING;
  system->play_start_time = get_current_time_ms();
  system->paused_time = 0.0;

  // Initialize game runtime context
  printf("    Game runtime context initialized\n");
  printf("    Hot-reload monitoring enabled\n");
}

void play_in_editor_pause(PlayInEditorSystem *system) {
  if (system->state != PLAY_STATE_PLAYING)
    return;

  system->state = PLAY_STATE_PAUSED;
  system->paused_time = get_current_time_ms();
  printf("  [PlayInEditor] Game paused\n");
}

void play_in_editor_stop(PlayInEditorSystem *system) {
  if (system->state == PLAY_STATE_STOPPED)
    return;

  printf("  [PlayInEditor] Stopping play mode...\n");

  // Restore scene from snapshot
  if (system->scene_snapshot) {
    printf("    Restoring scene from snapshot\n");
    // In real implementation, restore actual scene data
  }

  system->state = PLAY_STATE_STOPPED;
  printf("    Scene restored, game stopped\n");
}

void play_in_editor_step(PlayInEditorSystem *system) {
  if (system->state != PLAY_STATE_PAUSED)
    return;

  system->state = PLAY_STATE_STEPPING;
  system->current_step = 0;
  printf("  [PlayInEditor] Step mode enabled (1 frame)\n");
}

bool play_in_editor_is_playing(PlayInEditorSystem *system) {
  return system->state == PLAY_STATE_PLAYING;
}

// =================================================================================================
//                    TRANSFORM GIZMO SYSTEM IMPLEMENTATION
// =================================================================================================

void transform_gizmo_init(TransformGizmo *gizmo) {
  printf("  [Gizmo] Initializing transform gizmo...\n");

  gizmo->mode = GIZMO_TRANSLATE;
  gizmo->space = GIZMO_SPACE_WORLD;
  gizmo->selected_axis = GIZMO_AXIS_ALL;
  gizmo->position = (Vec3){0, 0, 0};
  gizmo->size = DEFAULT_GIZMO_SIZE;
  gizmo->color_x = (Vec4){1, 0, 0, 1};
  gizmo->color_y = (Vec4){0, 1, 0, 1};
  gizmo->color_z = (Vec4){0, 0, 1, 1};
  gizmo->color_selected = (Vec4){1, 1, 0, 1};
  gizmo->is_active = false;
  gizmo->snap_to_grid = false;
  gizmo->snap_value = DEFAULT_SNAP_VALUE;
  gizmo->is_dragging = false;

  printf("  [Gizmo] Transform gizmo ready\n");
}

void transform_gizmo_update(TransformGizmo *gizmo,
                            const Vec3 *target_position) {
  if (!gizmo->is_active)
    return;

  gizmo->position = *target_position;

  // Update gizmo rendering based on mode
  switch (gizmo->mode) {
  case GIZMO_TRANSLATE:
    // Update translation arrows
    break;
  case GIZMO_ROTATE:
    // Update rotation arcs
    break;
  case GIZMO_SCALE:
    // Update scale boxes
    break;
  case GIZMO_NONE:
  case GIZMO_COUNT:
    break;
  }
}

bool transform_gizmo_handle_input(TransformGizmo *gizmo, const Vec2 *mouse_pos,
                                  bool mouse_down) {
  if (!gizmo->is_active)
    return false;

  // Ray-picking for gizmo interaction
  // In real implementation, this would use actual ray casting

  if (mouse_down && !gizmo->is_dragging) {
    gizmo->is_dragging = true;
    gizmo->initial_mouse_pos = (Vec3){mouse_pos->x, mouse_pos->y, 0};
    gizmo->initial_position = gizmo->position;
    printf("    Gizmo interaction started\n");
    return true;
  } else if (!mouse_down && gizmo->is_dragging) {
    gizmo->is_dragging = false;
    printf("    Gizmo interaction ended\n");
    return true;
  }

  return false;
}

// =================================================================================================
//                    SELECTION OUTLINE SYSTEM IMPLEMENTATION
// =================================================================================================

void selection_outline_init(SelectionOutlineSystem *system) {
  printf("  [Selection] Initializing selection outline system...\n");

  memset(system->selected_entities, 0, sizeof(system->selected_entities));
  system->selected_count = 0;
  system->outline_color = DEFAULT_OUTLINE_COLOR;
  system->outline_thickness = DEFAULT_OUTLINE_THICKNESS;
  system->hover_enabled = true;
  system->hovered_entity = 0;
  system->hover_color = (Vec4){1.0f, 1.0f, 0.0f, 1.0f}; // Yellow

  // Initialize rendering resources
  system->stencil_shader = 2001;  // Placeholder shader ID
  system->outline_shader = 2002;  // Placeholder shader ID
  system->frame_buffer = 3001;    // Placeholder framebuffer ID
  system->stencil_texture = 4001; // Placeholder texture ID

  printf("  [Selection] Outline system ready (<0.5ms rendering)\n");
}

void selection_outline_set_selected(SelectionOutlineSystem *system,
                                    u32 *entities, u32 count) {
  count = min(count, MAX_SELECTED_ENTITIES);
  memcpy(system->selected_entities, entities, count * sizeof(u32));
  system->selected_count = count;
  printf("  [Selection] %d entities selected\n", count);
}

void selection_outline_render(SelectionOutlineSystem *system) {
  if (system->selected_count == 0)
    return;

  f64 start_time = get_current_time_ms();

  // Render selected objects to stencil buffer
  // Apply edge detection shader for outline
  // Render with configurable color and thickness

  f64 render_time = get_current_time_ms() - start_time;

  if (render_time > OUTLINE_RENDER_TIME_TARGET) {
    printf(
        "  [Selection] Warning: Outline render took %.3fms (>%.1fms target)\n",
        render_time, OUTLINE_RENDER_TIME_TARGET);
  }
}

// =================================================================================================
//                    PERFORMANCE PROFILER SYSTEM IMPLEMENTATION
// =================================================================================================

void performance_profiler_init(PerformanceProfiler *profiler) {
  printf("  [Profiler] Initializing performance profiler...\n");

  memset(profiler->frame_times, 0, sizeof(profiler->frame_times));
  profiler->frame_index = 0;
  profiler->current_frame_time = 0.0;
  profiler->average_frame_time = 16.67; // 60 FPS target
  profiler->min_frame_time = 1000.0;
  profiler->max_frame_time = 0.0;

  profiler->category_count = 0;
  profiler->draw_calls = 0;
  profiler->triangle_count = 0;
  profiler->texture_switches = 0;

  profiler->is_recording = false;
  profiler->recording_start_time = 0.0;

  // Add default categories
  performance_profiler_add_category(profiler, "Rendering", (Vec4){1, 0, 0, 1});
  performance_profiler_add_category(profiler, "Physics", (Vec4){0, 1, 0, 1});
  performance_profiler_add_category(profiler, "Audio", (Vec4){0, 0, 1, 1});
  performance_profiler_add_category(profiler, "AI", (Vec4){1, 1, 0, 1});

  printf("  [Profiler] Performance profiler ready\n");
}

void performance_profiler_begin_frame(PerformanceProfiler *profiler) {
  profiler->current_frame_time = get_current_time_ms();
}

void performance_profiler_end_frame(PerformanceProfiler *profiler) {
  f64 frame_time = get_current_time_ms() - profiler->current_frame_time;

  profiler->frame_times[profiler->frame_index] = frame_time;
  profiler->frame_index = (profiler->frame_index + 1) % FRAME_HISTORY_SIZE;

  // Update statistics
  profiler->min_frame_time = min(profiler->min_frame_time, frame_time);
  profiler->max_frame_time = max(profiler->max_frame_time, frame_time);

  // Calculate average (simple moving average)
  f64 sum = 0.0;
  for (int i = 0; i < FRAME_HISTORY_SIZE; i++) {
    sum += profiler->frame_times[i];
  }
  profiler->average_frame_time = sum / FRAME_HISTORY_SIZE;

  // Reset per-frame counters
  profiler->draw_calls = 0;
  profiler->triangle_count = 0;
  profiler->texture_switches = 0;
}

void performance_profiler_add_category(PerformanceProfiler *profiler,
                                       const char *name, Vec4 color) {
  if (profiler->category_count >= MAX_PROFILE_CATEGORIES)
    return;

  ProfileCategory *category = &profiler->categories[profiler->category_count];
  strncpy(category->name, name, sizeof(category->name) - 1);
  category->color = color;
  category->time_ms = 0.0;
  category->cpu_time = 0.0;
  category->gpu_time = 0.0;
  category->call_count = 0;
  category->is_visible = true;

  profiler->category_count++;
  printf("    Added profile category: %s\n", name);
}

// =================================================================================================
//                    MEMORY PROFILER SYSTEM IMPLEMENTATION
// =================================================================================================

// Static helper prototype
static void editor_memory_profiler_add_module(EditorMemoryProfiler *profiler,
                                              const char *name, Vec4 color);

void editor_memory_profiler_init(EditorMemoryProfiler *profiler) {
  printf("  [MemoryProfiler] Initializing memory profiler...\n");

  memset(profiler->modules, 0, sizeof(profiler->modules));
  profiler->module_count = 0;

  memset(profiler->allocation_history, 0, sizeof(profiler->allocation_history));
  profiler->history_index = 0;

  memset(profiler->leak_tracking, 0, sizeof(profiler->leak_tracking));
  profiler->leak_count = 0;

  memset(profiler->memory_blocks, 0, sizeof(profiler->memory_blocks));
  profiler->block_count = 0;

  profiler->total_allocated = 0;
  profiler->total_freed = 0;
  profiler->peak_memory = 0;

  // Add default modules
  editor_memory_profiler_add_module(profiler, "Core", (Vec4){1, 0, 0, 1});
  editor_memory_profiler_add_module(profiler, "Renderer", (Vec4){0, 1, 0, 1});
  editor_memory_profiler_add_module(profiler, "Audio", (Vec4){0, 0, 1, 1});
  editor_memory_profiler_add_module(profiler, "Physics", (Vec4){1, 1, 0, 1});

  printf("  [MemoryProfiler] Memory profiler ready\n");
}

static void editor_memory_profiler_add_module(EditorMemoryProfiler *profiler,
                                              const char *name, Vec4 color) {
  if (profiler->module_count >= MAX_MEMORY_MODULES)
    return;

  MemoryModule *module = &profiler->modules[profiler->module_count];
  strncpy(module->module_name, name, sizeof(module->module_name) - 1);
  module->color = color;
  module->allocated_bytes = 0;
  module->peak_bytes = 0;
  module->allocation_count = 0;
  module->deallocation_count = 0;
  module->is_visible = true;

  profiler->module_count++;
  printf("    Added memory module: %s\n", name);
}

void editor_memory_profiler_record_allocation(EditorMemoryProfiler *profiler,
                                              void *ptr, size_t size,
                                              const char *file, int line) {
  profiler->total_allocated += size;
  profiler->peak_memory = max(profiler->peak_memory, profiler->total_allocated -
                                                         profiler->total_freed);

  // Add to allocation history
  profiler->allocation_history[profiler->history_index].allocated = size;
  profiler->allocation_history[profiler->history_index].freed = 0;
  profiler->allocation_history[profiler->history_index].timestamp =
      get_current_time_ms();
  profiler->history_index =
      (profiler->history_index + 1) % ALLOCATION_HISTORY_SIZE;

  // Track for leak detection
  if (profiler->leak_count < LEAK_TRACKING_SIZE) {
    profiler->leak_tracking[profiler->leak_count].pointer = ptr;
    profiler->leak_tracking[profiler->leak_count].size = size;
    profiler->leak_tracking[profiler->leak_count].file = file;
    profiler->leak_tracking[profiler->leak_count].line = line;
    profiler->leak_tracking[profiler->leak_count].timestamp =
        get_current_time_ms();
    profiler->leak_count++;
  }
}

void editor_memory_profiler_record_deallocation(EditorMemoryProfiler *profiler,
                                                void *ptr) {
  // Find and remove from leak tracking
  for (u32 i = 0; i < profiler->leak_count; i++) {
    if (profiler->leak_tracking[i].pointer == ptr) {
      profiler->total_freed += profiler->leak_tracking[i].size;

      // Remove from leak tracking
      memmove(&profiler->leak_tracking[i], &profiler->leak_tracking[i + 1],
              (profiler->leak_count - i - 1) *
                  sizeof(profiler->leak_tracking[0]));
      profiler->leak_count--;
      break;
    }
  }
}

void editor_memory_profiler_detect_leaks(EditorMemoryProfiler *profiler) {
  if (profiler->leak_count > 0) {
    printf("  [MemoryProfiler] Detected %d memory leaks:\n",
           profiler->leak_count);
    for (u32 i = 0; i < profiler->leak_count; i++) {
      printf("    Leak: %zu bytes at %p (%s:%d)\n",
             profiler->leak_tracking[i].size,
             profiler->leak_tracking[i].pointer,
             profiler->leak_tracking[i].file, profiler->leak_tracking[i].line);
    }
  } else {
    printf("  [MemoryProfiler] No memory leaks detected\n");
  }
}

// =================================================================================================
//                    LAYOUT SYSTEM IMPLEMENTATION
// =================================================================================================

void layout_system_init(LayoutSystem *system) {
  printf("  [Layout] Initializing layout system...\n");

  memset(system->layouts, 0, sizeof(system->layouts));
  system->layout_count = 0;
  system->active_layout = 0;
  strcpy(system->current_layout_name, "Default");

  // Create default layout
  EditorLayout *default_layout = &system->layouts[0];
  strcpy(default_layout->name, "Default");
  strcpy(default_layout->description, "Default editor layout");

  // Set up default window states
  default_layout->window_count = 6;
  default_layout->windows[0] =
      (EditorWindow){true, (Vec2){0, 0}, (Vec2){800, 600}, true, "MainDock"};
  default_layout->windows[1] =
      (EditorWindow){true, (Vec2){0, 600}, (Vec2){200, 200}, true, "Viewport"};
  default_layout->windows[2] = (EditorWindow){
      true, (Vec2){200, 600}, (Vec2){200, 200}, true, "Outliner"};
  default_layout->windows[3] = (EditorWindow){
      true, (Vec2){400, 600}, (Vec2){200, 200}, true, "Inspector"};
  default_layout->windows[4] = (EditorWindow){
      true, (Vec2){600, 600}, (Vec2){200, 200}, true, "AssetBrowser"};
  default_layout->windows[5] =
      (EditorWindow){false, (Vec2){800, 0}, (Vec2){300, 400}, false, "Console"};

  // Default viewport configuration
  default_layout->viewport_config.layout = VIEWPORT_SINGLE;
  default_layout->viewport_config.active_viewport = 0;

  // UI preferences
  default_layout->ui_scale = 1.0f;
  default_layout->dark_mode = true;
  default_layout->zen_mode = false;

  default_layout->created_time = get_current_time_ms();
  default_layout->modified_time = default_layout->created_time;
  default_layout->version = 1;

  system->layout_count = 1;

  // Create preset layouts (would be implemented similarly)
  // layout_system_create_preset(system, "Animator", "Animation-focused
  // layout");

  printf("  [Layout] Layout system ready (%d layouts)\n", system->layout_count);
}

void layout_system_save_current(LayoutSystem *system, const char *name) {
  if (system->layout_count >= MAX_LAYOUTS)
    return;

  EditorLayout *layout = &system->layouts[system->layout_count];
  strcpy(layout->name, name);
  strcpy(layout->description, "Custom saved layout");

  // Save current window states from global editor
  layout->window_count = 6;
  layout->windows[0] =
      (EditorWindow){true, (Vec2){0, 0}, (Vec2){800, 600}, true, "MainDock"};
  // ... save other windows from g_editor state

  layout->viewport_config = g_editor.viewport_system;
  layout->ui_scale = g_editor.ui_scale;
  layout->dark_mode = g_editor.dark_mode;
  layout->zen_mode = g_editor.zen_mode;

  layout->created_time = get_current_time_ms();
  layout->modified_time = layout->created_time;

  system->layout_count++;
  printf("  [Layout] Saved current layout as '%s'\n", name);
}

void layout_system_load(LayoutSystem *system, const char *name) {
  for (u32 i = 0; i < system->layout_count; i++) {
    if (strcmp(system->layouts[i].name, name) == 0) {
      layout_system_apply(system, &system->layouts[i]);
      system->active_layout = i;
      strcpy(system->current_layout_name, name);
      printf("  [Layout] Loaded layout: %s\n", name);
      return;
    }
  }
  printf("  [Layout] Layout not found: %s\n", name);
}

void layout_system_apply(LayoutSystem *system, const EditorLayout *layout) {
  // Apply layout to global editor state
  g_editor.viewport_system = layout->viewport_config;
  g_editor.ui_scale = layout->ui_scale;
  g_editor.dark_mode = layout->dark_mode;
  g_editor.zen_mode = layout->zen_mode;

  // Apply window states
  for (u32 i = 0; i < layout->window_count; i++) {
    // Apply window position, size, docking state
    printf("    Applied window: %s\n", layout->windows[i].dock_node);
  }
}

// Deleted duplicate Editor_Init, assuming it is defined in editor_main.c
