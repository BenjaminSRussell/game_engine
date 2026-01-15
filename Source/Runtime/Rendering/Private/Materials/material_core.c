#include "material_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

// Global material system instance
MaterialSystem *g_material_system = NULL;

// =============================================================================
// MATERIAL SYSTEM INITIALIZATION
// =============================================================================

bool material_system_init(RenderContext *renderer) {
  if (g_material_system) {
    LOG_WARN(LOG_CAT_RENDERER, "Material system already initialized");
    return true;
  }

  g_material_system = (MaterialSystem *)UNIFIED_ALLOC(sizeof(MaterialSystem));
  if (!g_material_system) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate material system");
    return false;
  }
  memset(g_material_system, 0, sizeof(MaterialSystem));

  g_material_system->renderer = renderer;
  g_material_system->initialized = true;

  // Initialize material cache
  g_material_system->material_capacity = 64;
  g_material_system->materials = (Material **)UNIFIED_ALLOC(
      sizeof(Material *) * g_material_system->material_capacity);

  // Initialize instance cache
  g_material_system->instance_capacity = 128;
  g_material_system->instances = (MaterialInstance **)UNIFIED_ALLOC(
      sizeof(MaterialInstance *) * g_material_system->instance_capacity);

  LOG_INFO(LOG_CAT_RENDERER, "Material system initialized");
  return true;
}

void material_system_shutdown(void) {
  if (!g_material_system)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Shutting down material system");

  // Destroy all instances
  for (uint32_t i = 0; i < g_material_system->instance_count; i++) {
    material_instance_destroy(g_material_system->instances[i]);
  }
  UNIFIED_FREE(g_material_system->instances);

  // Destroy all materials
  for (uint32_t i = 0; i < g_material_system->material_count; i++) {
    material_destroy(g_material_system->materials[i]);
  }
  UNIFIED_FREE(g_material_system->materials);

  UNIFIED_FREE(g_material_system);
  g_material_system = NULL;
}

// =============================================================================
// MATERIAL CREATION
// =============================================================================

Material *material_create(const MaterialDesc *desc) {
  if (!g_material_system || !desc) {
    LOG_ERROR(LOG_CAT_RENDERER, "Invalid material system or descriptor");
    return NULL;
  }

  if (!desc->shader_program) {
    LOG_ERROR(LOG_CAT_RENDERER, "Material requires a shader program");
    return NULL;
  }

  Material *material = (Material *)UNIFIED_ALLOC(sizeof(Material));
  if (!material) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate material");
    return NULL;
  }
  memset(material, 0, sizeof(Material));

  // Copy basic properties
  if (desc->name) {
    strncpy(material->name, desc->name, sizeof(material->name) - 1);
  } else {
    snprintf(material->name, sizeof(material->name), "material_%p",
             (void *)material);
  }

  material->shader_program = desc->shader_program;
  material->blend_mode = desc->blend_mode;
  material->cull_mode = desc->cull_mode;
  material->depth_test = desc->depth_test;
  material->depth_write = desc->depth_write;
  material->wireframe = desc->wireframe;

  // Allocate parameter storage
  material->param_capacity = 16;
  material->params = (MaterialParam *)UNIFIED_ALLOC(sizeof(MaterialParam) *
                                                    material->param_capacity);
  material->param_count = 0;

  // Copy initial parameters if provided
  if (desc->params && desc->param_count > 0) {
    for (uint32_t i = 0; i < desc->param_count; i++) {
      if (material->param_count >= material->param_capacity) {
        // Grow capacity
        material->param_capacity *= 2;
        MaterialParam *new_params = (MaterialParam *)UNIFIED_ALLOC(
            sizeof(MaterialParam) * material->param_capacity);
        memcpy(new_params, material->params,
               sizeof(MaterialParam) * material->param_count);
        UNIFIED_FREE(material->params);
        material->params = new_params;
      }
      material->params[material->param_count++] = desc->params[i];
    }
  }

  // Add to material cache
  if (g_material_system->material_count >=
      g_material_system->material_capacity) {
    g_material_system->material_capacity *= 2;
    Material **new_materials = (Material **)UNIFIED_ALLOC(
        sizeof(Material *) * g_material_system->material_capacity);
    memcpy(new_materials, g_material_system->materials,
           sizeof(Material *) * g_material_system->material_count);
    UNIFIED_FREE(g_material_system->materials);
    g_material_system->materials = new_materials;
  }

  g_material_system->materials[g_material_system->material_count++] = material;

  LOG_INFO(LOG_CAT_RENDERER, "Created material '%s'", material->name);
  return material;
}

void material_destroy(Material *material) {
  if (!material)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Destroying material '%s'", material->name);

  // Free parameters
  if (material->params) {
    UNIFIED_FREE(material->params);
  }

  // TODO: Destroy native handle

  // Remove from cache
  if (g_material_system) {
    for (uint32_t i = 0; i < g_material_system->material_count; i++) {
      if (g_material_system->materials[i] == material) {
        for (uint32_t j = i; j < g_material_system->material_count - 1; j++) {
          g_material_system->materials[j] = g_material_system->materials[j + 1];
        }
        g_material_system->material_count--;
        break;
      }
    }
  }

  UNIFIED_FREE(material);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

uint32_t material_get_param_size(MaterialParamType type) {
  switch (type) {
  case MATERIAL_PARAM_FLOAT:
    return sizeof(float);
  case MATERIAL_PARAM_VEC2:
    return sizeof(float) * 2;
  case MATERIAL_PARAM_VEC3:
    return sizeof(float) * 3;
  case MATERIAL_PARAM_VEC4:
    return sizeof(float) * 4;
  case MATERIAL_PARAM_INT:
    return sizeof(int32_t);
  case MATERIAL_PARAM_BOOL:
    return sizeof(bool);
  case MATERIAL_PARAM_TEXTURE_2D:
  case MATERIAL_PARAM_TEXTURE_CUBE:
    return sizeof(void *);
  default:
    return 0;
  }
}

const char *material_blend_mode_to_string(MaterialBlendMode mode) {
  switch (mode) {
  case MATERIAL_BLEND_OPAQUE:
    return "OPAQUE";
  case MATERIAL_BLEND_ALPHA:
    return "ALPHA";
  case MATERIAL_BLEND_ADDITIVE:
    return "ADDITIVE";
  case MATERIAL_BLEND_MULTIPLY:
    return "MULTIPLY";
  default:
    return "UNKNOWN";
  }
}

const char *material_cull_mode_to_string(MaterialCullMode mode) {
  switch (mode) {
  case MATERIAL_CULL_NONE:
    return "NONE";
  case MATERIAL_CULL_FRONT:
    return "FRONT";
  case MATERIAL_CULL_BACK:
    return "BACK";
  default:
    return "UNKNOWN";
  }
}
