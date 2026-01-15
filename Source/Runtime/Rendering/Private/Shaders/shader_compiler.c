#include "shader_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

// =============================================================================
// SHADER COMPILATION (Stub Implementation)
// =============================================================================

ShaderCompileResult shader_compile(Shader *shader, char *error_buffer,
                                   uint32_t error_buffer_size) {
  if (!shader) {
    if (error_buffer) {
      snprintf(error_buffer, error_buffer_size, "Invalid shader");
    }
    return SHADER_COMPILE_ERROR_SYNTAX;
  }

  if (!shader->source_code || shader->source_size == 0) {
    if (error_buffer) {
      snprintf(error_buffer, error_buffer_size, "No source code provided");
    }
    shader->last_compile_result = SHADER_COMPILE_ERROR_IO;
    return SHADER_COMPILE_ERROR_IO;
  }

  LOG_INFO(LOG_CAT_RENDERER, "Compiling shader '%s' (%s, %s)", shader->name,
           shader_stage_to_string(shader->stage),
           shader_language_to_string(shader->language));

  // TODO: Actual compilation based on backend
  // For now, just mark as compiled (stub)

  shader->is_compiled = true;
  shader->last_compile_result = SHADER_COMPILE_SUCCESS;

  LOG_INFO(LOG_CAT_RENDERER, "Shader '%s' compiled successfully (stub)",
           shader->name);
  return SHADER_COMPILE_SUCCESS;
}

bool shader_is_compiled(Shader *shader) {
  return shader && shader->is_compiled;
}

const ShaderReflection *shader_get_reflection(Shader *shader) {
  return shader ? &shader->reflection : NULL;
}

// =============================================================================
// SHADER PROGRAM MANAGEMENT
// =============================================================================

ShaderProgram *shader_program_create(const ShaderProgramDesc *desc) {
  if (!g_shader_system || !desc) {
    LOG_ERROR(LOG_CAT_RENDERER, "Invalid shader system or program descriptor");
    return NULL;
  }

  ShaderProgram *program =
      (ShaderProgram *)UNIFIED_ALLOC(sizeof(ShaderProgram));
  if (!program) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate shader program");
    return NULL;
  }
  memset(program, 0, sizeof(ShaderProgram));

  // Copy name
  if (desc->name) {
    strncpy(program->name, desc->name, sizeof(program->name) - 1);
  } else {
    snprintf(program->name, sizeof(program->name), "program_%p",
             (void *)program);
  }

  // Reference shaders
  program->vertex_shader = desc->vertex_shader;
  program->fragment_shader = desc->fragment_shader;
  program->geometry_shader = desc->geometry_shader;
  program->compute_shader = desc->compute_shader;

  // Validate shader combination
  if (desc->compute_shader) {
    if (desc->vertex_shader || desc->fragment_shader || desc->geometry_shader) {
      LOG_ERROR(LOG_CAT_RENDERER,
                "Compute shader cannot be combined with graphics shaders");
      UNIFIED_FREE(program);
      return NULL;
    }
  } else {
    if (!desc->vertex_shader || !desc->fragment_shader) {
      LOG_ERROR(LOG_CAT_RENDERER,
                "Graphics program requires both vertex and fragment shaders");
      UNIFIED_FREE(program);
      return NULL;
    }
  }

  // TODO: Link shaders and create pipeline
  program->is_linked = true; // Stub

  // Add to program cache
  if (g_shader_system->program_count >= g_shader_system->program_capacity) {
    g_shader_system->program_capacity *= 2;
    ShaderProgram **new_programs = (ShaderProgram **)UNIFIED_ALLOC(
        sizeof(ShaderProgram *) * g_shader_system->program_capacity);
    memcpy(new_programs, g_shader_system->programs,
           sizeof(ShaderProgram *) * g_shader_system->program_count);
    UNIFIED_FREE(g_shader_system->programs);
    g_shader_system->programs = new_programs;
  }

  g_shader_system->programs[g_shader_system->program_count++] = program;

  LOG_INFO(LOG_CAT_RENDERER, "Created shader program '%s'", program->name);
  return program;
}

void shader_program_destroy(ShaderProgram *program) {
  if (!program)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Destroying shader program '%s'", program->name);

  // TODO: Destroy native pipeline

  // Remove from cache
  if (g_shader_system) {
    for (uint32_t i = 0; i < g_shader_system->program_count; i++) {
      if (g_shader_system->programs[i] == program) {
        for (uint32_t j = i; j < g_shader_system->program_count - 1; j++) {
          g_shader_system->programs[j] = g_shader_system->programs[j + 1];
        }
        g_shader_system->program_count--;
        break;
      }
    }
  }

  UNIFIED_FREE(program);
}

// =============================================================================
// HOT RELOAD & CACHING (Stubs)
// =============================================================================

void shader_enable_hot_reload(bool enabled) {
  if (g_shader_system) {
    g_shader_system->hot_reload_enabled = enabled;
    LOG_INFO(LOG_CAT_RENDERER, "Shader hot reload %s",
             enabled ? "enabled" : "disabled");
  }
}

void shader_check_for_changes(void) {
  if (!g_shader_system || !g_shader_system->hot_reload_enabled)
    return;

  // TODO: Check file timestamps and recompile if changed
}

void shader_cache_clear(void) {
  LOG_INFO(LOG_CAT_RENDERER, "Shader cache cleared (stub)");
}

void shader_cache_save(const char *cache_path) {
  LOG_INFO(LOG_CAT_RENDERER, "Saving shader cache to %s (stub)",
           cache_path ? cache_path : "null");
}

bool shader_cache_load(const char *cache_path) {
  LOG_INFO(LOG_CAT_RENDERER, "Loading shader cache from %s (stub)",
           cache_path ? cache_path : "null");
  return false;
}
