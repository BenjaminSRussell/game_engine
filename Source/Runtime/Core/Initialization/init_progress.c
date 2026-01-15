#include "init_progress.h"
#include <common.h>
#include <stdbool.h>

// Initialization progress tracking system
typedef struct {
  const char *current_stage;
  f32 progress;
  f32 total_stages;
  f32 current_stage_index;
  bool show_progress;
} InitProgress;

static InitProgress g_init_progress = {0};

void init_progress_start(const char *stage_name, f32 total_stages) {
  g_init_progress.current_stage = stage_name;
  g_init_progress.total_stages = total_stages;
  g_init_progress.current_stage_index = 0;
  g_init_progress.progress = 0.0f;
  g_init_progress.show_progress = true;

  LOG_INFO("=== Initialization Started ===");
  LOG_INFO("Stage: %s (%.0f/%.0f)", stage_name, 1.0f, total_stages);
}

void init_progress_update_stage(const char *stage_name) {
  g_init_progress.current_stage_index++;
  g_init_progress.current_stage = stage_name;
  g_init_progress.progress =
      (g_init_progress.current_stage_index / g_init_progress.total_stages) *
      100.0f;

  LOG_INFO("Stage: %s (%.0f/%.0f) - %.1f%% complete", stage_name,
           g_init_progress.current_stage_index, g_init_progress.total_stages,
           g_init_progress.progress);
}

void init_progress_complete(void) {
  g_init_progress.progress = 100.0f;
  g_init_progress.current_stage = "Complete";
  g_init_progress.show_progress = false;

  LOG_INFO("=== Initialization Complete ===");
  LOG_INFO("All systems initialized successfully");
}

void init_progress_error(const char *error_stage, const char *error_message) {
  LOG_ERROR("Initialization failed at stage: %s", error_stage);
  LOG_ERROR("Error: %s", error_message);
  LOG_ERROR("Progress: %.1f%% complete", g_init_progress.progress);

  // Note: Error dialog hook would go here if extracted
}
