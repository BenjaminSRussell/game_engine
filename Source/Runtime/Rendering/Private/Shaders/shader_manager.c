#include "shader_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <stdio.h>
#include <string.h>

// Global shader system instance
ShaderSystem *g_shader_system = NULL;

// =============================================================================
// SHADER SYSTEM INITIALIZATION
// =============================================================================

bool shader_system_init(RenderContext *renderer) {
  if (g_shader_system) {
    LOG_WARN(LOG_CAT_RENDERER, "Shader system already initialized");
    return true;
  }

  g_shader_system = (ShaderSystem *)UNIFIED_ALLOC(sizeof(ShaderSystem));
  if (!g_shader_system) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate shader system");
    return false;
  }
  memset(g_shader_system, 0, sizeof(ShaderSystem));

  g_shader_system->renderer = renderer;
  g_shader_system->initialized = true;
  g_shader_system->hot_reload_enabled = false;

  // Initialize shader cache
  g_shader_system->shader_capacity = 64;
  g_shader_system->shaders = (Shader **)UNIFIED_ALLOC(
      sizeof(Shader *) * g_shader_system->shader_capacity);

  // Initialize program cache
  g_shader_system->program_capacity = 32;
  g_shader_system->programs = (ShaderProgram **)UNIFIED_ALLOC(
      sizeof(ShaderProgram *) * g_shader_system->program_capacity);

  LOG_INFO(LOG_CAT_RENDERER, "Shader system initialized");
  return true;
}

void shader_system_shutdown(void) {
  if (!g_shader_system)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Shutting down shader system");

  // Destroy all programs
  for (uint32_t i = 0; i < g_shader_system->program_count; i++) {
    shader_program_destroy(g_shader_system->programs[i]);
  }
  UNIFIED_FREE(g_shader_system->programs);

  // Destroy all shaders
  for (uint32_t i = 0; i < g_shader_system->shader_count; i++) {
    shader_destroy(g_shader_system->shaders[i]);
  }
  UNIFIED_FREE(g_shader_system->shaders);

  UNIFIED_FREE(g_shader_system);
  g_shader_system = NULL;
}

// =============================================================================
// SHADER CREATION
// =============================================================================

Shader *shader_create(const ShaderDesc *desc) {
  if (!g_shader_system || !desc) {
    LOG_ERROR(LOG_CAT_RENDERER, "Invalid shader system or descriptor");
    return NULL;
  }

  Shader *shader = (Shader *)UNIFIED_ALLOC(sizeof(Shader));
  if (!shader) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate shader");
    return NULL;
  }
  memset(shader, 0, sizeof(Shader));

  // Copy basic properties
  if (desc->name) {
    strncpy(shader->name, desc->name, sizeof(shader->name) - 1);
  } else {
    snprintf(shader->name, sizeof(shader->name), "shader_%p", (void *)shader);
  }

  shader->stage = desc->stage;
  shader->language = desc->language;

  if (desc->entry_point) {
    strncpy(shader->entry_point, desc->entry_point,
            sizeof(shader->entry_point) - 1);
  } else {
    strcpy(shader->entry_point, "main");
  }

  // Copy source code if provided
  if (desc->source_code && desc->source_size > 0) {
    shader->source_size = desc->source_size;
    shader->source_code = (char *)UNIFIED_ALLOC(shader->source_size + 1);
    memcpy(shader->source_code, desc->source_code, shader->source_size);
    shader->source_code[shader->source_size] = '\0';
  }

  // Copy source path if provided
  if (desc->source_path) {
    strncpy(shader->source_path, desc->source_path,
            sizeof(shader->source_path) - 1);
  }

  // Add to shader cache
  if (g_shader_system->shader_count >= g_shader_system->shader_capacity) {
    // Grow capacity
    g_shader_system->shader_capacity *= 2;
    Shader **new_shaders = (Shader **)UNIFIED_ALLOC(
        sizeof(Shader *) * g_shader_system->shader_capacity);
    memcpy(new_shaders, g_shader_system->shaders,
           sizeof(Shader *) * g_shader_system->shader_count);
    UNIFIED_FREE(g_shader_system->shaders);
    g_shader_system->shaders = new_shaders;
  }

  g_shader_system->shaders[g_shader_system->shader_count++] = shader;

  LOG_INFO(LOG_CAT_RENDERER, "Created shader '%s' (stage=%d)", shader->name,
           shader->stage);
  return shader;
}

void shader_destroy(Shader *shader) {
  if (!shader)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Destroying shader '%s'", shader->name);

  // Free source code
  if (shader->source_code) {
    UNIFIED_FREE(shader->source_code);
  }

  // Free bytecode
  if (shader->bytecode) {
    UNIFIED_FREE(shader->bytecode);
  }

  // TODO: Destroy native handle

  // Remove from cache
  if (g_shader_system) {
    for (uint32_t i = 0; i < g_shader_system->shader_count; i++) {
      if (g_shader_system->shaders[i] == shader) {
        // Shift remaining shaders
        for (uint32_t j = i; j < g_shader_system->shader_count - 1; j++) {
          g_shader_system->shaders[j] = g_shader_system->shaders[j + 1];
        }
        g_shader_system->shader_count--;
        break;
      }
    }
  }

  UNIFIED_FREE(shader);
}

// =============================================================================
// SHADER LOADING
// =============================================================================

Shader *shader_load_from_file(const char *path, ShaderStage stage) {
  if (!path)
    return NULL;

  LOG_INFO(LOG_CAT_RENDERER, "Loading shader from file: %s", path);

  // Open file
  FILE *file = fopen(path, "rb");
  if (!file) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to open shader file: %s", path);
    return NULL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size <= 0 || size > 10 * 1024 * 1024) { // Max 10MB
    LOG_ERROR(LOG_CAT_RENDERER, "Invalid shader file size: %ld", size);
    fclose(file);
    return NULL;
  }

  // Read file
  char *source = (char *)UNIFIED_ALLOC(size + 1);
  size_t read = fread(source, 1, size, file);
  fclose(file);

  if (read != (size_t)size) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to read shader file");
    UNIFIED_FREE(source);
    return NULL;
  }
  source[size] = '\0';

  // Create shader descriptor
  ShaderDesc desc = {.name = path,
                     .source_path = path,
                     .source_code = source,
                     .source_size = (uint32_t)size,
                     .stage = stage,
                     .language = SHADER_LANG_AUTO,
                     .entry_point = "main",
                     .enable_debug = true,
                     .enable_optimization = false};

  Shader *shader = shader_create(&desc);
  UNIFIED_FREE(source);

  return shader;
}

Shader *shader_load_from_source(const char *source, ShaderStage stage,
                                const char *name) {
  if (!source)
    return NULL;

  ShaderDesc desc = {.name = name ? name : "inline_shader",
                     .source_code = source,
                     .source_size = (uint32_t)strlen(source),
                     .stage = stage,
                     .language = SHADER_LANG_AUTO,
                     .entry_point = "main"};

  return shader_create(&desc);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

const char *shader_stage_to_string(ShaderStage stage) {
  switch (stage) {
  case SHADER_STAGE_VERTEX:
    return "VERTEX";
  case SHADER_STAGE_FRAGMENT:
    return "FRAGMENT";
  case SHADER_STAGE_COMPUTE:
    return "COMPUTE";
  case SHADER_STAGE_GEOMETRY:
    return "GEOMETRY";
  case SHADER_STAGE_TESSELLATION_CONTROL:
    return "TESS_CONTROL";
  case SHADER_STAGE_TESSELLATION_EVAL:
    return "TESS_EVAL";
  default:
    return "UNKNOWN";
  }
}

const char *shader_language_to_string(ShaderLanguage lang) {
  switch (lang) {
  case SHADER_LANG_GLSL:
    return "GLSL";
  case SHADER_LANG_HLSL:
    return "HLSL";
  case SHADER_LANG_MSL:
    return "MSL";
  case SHADER_LANG_SPIRV:
    return "SPIR-V";
  case SHADER_LANG_AUTO:
    return "AUTO";
  default:
    return "UNKNOWN";
  }
}
