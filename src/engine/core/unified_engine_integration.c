// src/core/unified_engine_integration.c
//
// Purpose: Integration code that ties all systems together
//
#include "../include/core/config_system.h"
#include "../include/core/logger.h"
#include "../include/core/time_system.h"
#include "../include/core/unified_engine.h"
#include <audio/audio.h>
#include <include/platform/input/input.h>
#include <network/network_system.h>
#include <rendering/renderer.h>
#include <rendering/renderer_api.h>
#include <stdlib.h>
#include <string.h>
#include <ui/ui_manager.h>

// Enhanced engine with integrated systems
typedef struct {
  Engine base;
  TimeSystem time_system;
  ConfigSystem config_system;
} EnhancedEngine;

// Initialize all integrated systems
bool engine_integrate_all_systems(Engine *engine) {
  if (!engine)
    return false;

  EnhancedEngine *enhanced = (EnhancedEngine *)engine;

  // Initialize time system
  time_system_init(&enhanced->time_system, engine->config.physics_timestep);

  // Initialize config system
  if (!config_system_init(&enhanced->config_system,
                          engine->config.config_path)) {
    LOG_WARN("Failed to initialize config system, using defaults");
  }

  // Load config values
  if (enhanced->config_system.document) {
    engine->config.window_width =
        config_system_get_int(&enhanced->config_system, "window", "width",
                              engine->config.window_width);
    engine->config.window_height =
        config_system_get_int(&enhanced->config_system, "window", "height",
                              engine->config.window_height);
    engine->config.master_volume =
        config_system_get_float(&enhanced->config_system, "audio",
                                "master_volume", engine->config.master_volume);
    engine->config.render_distance = config_system_get_int(
        &enhanced->config_system, "rendering", "render_distance",
        engine->config.render_distance);
  }

  // Initialize renderer if not already done
  if (!engine->renderer) {
    engine->renderer = create_vulkan_renderer();
    if (engine->renderer) {
      RendererConfig render_config = renderer_create_default_config();
      if (!engine->renderer->init(engine->renderer, &render_config)) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
      }
    }
  }

  // Initialize audio if not already done
  if (!engine->audio) {
    engine->audio = create_openal_audio_system();
    if (engine->audio) {
      AudioConfig audio_config = audio_create_default_config();
      audio_config.master_volume = engine->config.master_volume;
      audio_config.music_volume = engine->config.music_volume;
      audio_config.sfx_volume = engine->config.sfx_volume;
      if (!engine->audio->init(engine->audio, &audio_config)) {
        LOG_WARN("Failed to initialize audio system");
      }
    }
  }

  // Initialize input if not already done
  if (!engine->input) {
    engine->input = create_glfw_input_system();
    if (engine->input) {
      InputConfig input_config = input_create_default_config();
      if (!engine->input->init(engine->input, &input_config)) {
        LOG_WARN("Failed to initialize input system");
      }
    }
  }

  // Initialize network if enabled
  if (engine->config.enable_networking && !engine->network) {
    engine->network = create_network_system();
    if (engine->network) {
      NetworkConfig net_config = network_create_default_config();
      net_config.port = engine->config.server_port;
      net_config.max_clients = engine->config.max_clients;
      if (!engine->network->init(engine->network, &net_config)) {
        LOG_WARN("Failed to initialize network system");
      }
    }
  }

  // Initialize UI if not already done
  if (!engine->ui) {
    engine->ui = create_ui_manager();
    if (engine->ui) {
      if (!engine->ui->init(engine->ui, 1024)) {
        LOG_WARN("Failed to initialize UI manager");
      }
    }
  }

  LOG_INFO("All systems integrated successfully");
  return true;
}

// Update all integrated systems
void engine_update_integrated_systems(Engine *engine, f32 delta_time) {
  if (!engine)
    return;

  EnhancedEngine *enhanced = (EnhancedEngine *)engine;

  // Update time system
  time_system_update(&enhanced->time_system);

  // Check for config changes
  config_system_check_for_changes(&enhanced->config_system);

  // Update subsystems
  if (engine->input) {
    engine->input->update(engine->input, delta_time);
  }

  if (engine->audio) {
    engine->audio->update(engine->audio, delta_time);
  }

  if (engine->network) {
    engine->network->update(engine->network, delta_time);
  }

  if (engine->ui) {
    engine->ui->update(engine->ui, delta_time);
  }
}

// Render all integrated systems
void engine_render_integrated_systems(Engine *engine) {
  if (!engine)
    return;

  if (engine->renderer) {
    engine->renderer->begin_frame(engine->renderer);

    // Render game
    if (engine->game_render) {
      engine->game_render(engine);
    }

    // Render UI
    if (engine->ui) {
      engine->ui->render(engine->ui);
    }

    engine->renderer->end_frame(engine->renderer);
  }
}

// Shutdown all integrated systems
void engine_shutdown_integrated_systems(Engine *engine) {
  if (!engine)
    return;

  EnhancedEngine *enhanced = (EnhancedEngine *)engine;

  // Shutdown config system
  config_system_shutdown(&enhanced->config_system);

  // Shutdown time system
  time_system_shutdown(&enhanced->time_system);

  // Shutdown subsystems
  if (engine->ui) {
    engine->ui->shutdown(engine->ui);
    // destroy_ui_manager(engine->ui);
  }

  if (engine->network) {
    engine->network->shutdown(engine->network);
    // destroy_network_system(engine->network);
  }

  if (engine->input) {
    engine->input->shutdown(engine->input);
    // destroy_input_system(engine->input);
  }

  if (engine->audio) {
    engine->audio->shutdown(engine->audio);
    // destroy_audio_system(engine->audio);
  }

  if (engine->renderer) {
    engine->renderer->shutdown(engine->renderer);
    // destroy_renderer(engine->renderer);
  }
}
