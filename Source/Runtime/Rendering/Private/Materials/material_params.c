#include "material_types_private.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

// =============================================================================
// PARAMETER MANAGEMENT - MATERIALS
// =============================================================================

static MaterialParam *find_or_create_param(Material *material,
                                           const char *name) {
  // Check if parameter exists
  for (uint32_t i = 0; i < material->param_count; i++) {
    if (strcmp(material->params[i].name, name) == 0) {
      return &material->params[i];
    }
  }

  // Create new parameter
  if (material->param_count >= material->param_capacity) {
    material->param_capacity *= 2;
    MaterialParam *new_params = (MaterialParam *)UNIFIED_ALLOC(
        sizeof(MaterialParam) * material->param_capacity);
    memcpy(new_params, material->params,
           sizeof(MaterialParam) * material->param_count);
    UNIFIED_FREE(material->params);
    material->params = new_params;
  }

  MaterialParam *param = &material->params[material->param_count++];
  memset(param, 0, sizeof(MaterialParam));
  strncpy(param->name, name, sizeof(param->name) - 1);
  return param;
}

bool material_set_param_float(Material *material, const char *name,
                              float value) {
  if (!material || !name)
    return false;

  MaterialParam *param = find_or_create_param(material, name);
  param->type = MATERIAL_PARAM_FLOAT;
  param->value.f = value;
  return true;
}

bool material_set_param_vec3(Material *material, const char *name,
                             const float *value) {
  if (!material || !name || !value)
    return false;

  MaterialParam *param = find_or_create_param(material, name);
  param->type = MATERIAL_PARAM_VEC3;
  memcpy(param->value.vec3, value, sizeof(float) * 3);
  return true;
}

bool material_set_param_vec4(Material *material, const char *name,
                             const float *value) {
  if (!material || !name || !value)
    return false;

  MaterialParam *param = find_or_create_param(material, name);
  param->type = MATERIAL_PARAM_VEC4;
  memcpy(param->value.vec4, value, sizeof(float) * 4);
  return true;
}

bool material_set_param_texture(Material *material, const char *name,
                                void *texture) {
  if (!material || !name)
    return false;

  MaterialParam *param = find_or_create_param(material, name);
  param->type = MATERIAL_PARAM_TEXTURE_2D;
  param->value.texture = texture;
  return true;
}

const MaterialParam *material_get_param(Material *material, const char *name) {
  if (!material || !name)
    return NULL;

  for (uint32_t i = 0; i < material->param_count; i++) {
    if (strcmp(material->params[i].name, name) == 0) {
      return &material->params[i];
    }
  }

  return NULL;
}

// =============================================================================
// MATERIAL INSTANCES
// =============================================================================

MaterialInstance *material_instance_create(Material *base_material) {
  if (!g_material_system || !base_material) {
    LOG_ERROR(LOG_CAT_RENDERER, "Invalid material system or base material");
    return NULL;
  }

  MaterialInstance *instance =
      (MaterialInstance *)UNIFIED_ALLOC(sizeof(MaterialInstance));
  if (!instance) {
    LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate material instance");
    return NULL;
  }
  memset(instance, 0, sizeof(MaterialInstance));

  instance->base_material = base_material;
  snprintf(instance->name, sizeof(instance->name), "%s_instance_%p",
           base_material->name, (void *)instance);

  // Allocate parameter override storage
  instance->override_capacity = 8;
  instance->param_overrides = (MaterialParam *)UNIFIED_ALLOC(
      sizeof(MaterialParam) * instance->override_capacity);
  instance->override_count = 0;

  // Add to instance cache
  if (g_material_system->instance_count >=
      g_material_system->instance_capacity) {
    g_material_system->instance_capacity *= 2;
    MaterialInstance **new_instances = (MaterialInstance **)UNIFIED_ALLOC(
        sizeof(MaterialInstance *) * g_material_system->instance_capacity);
    memcpy(new_instances, g_material_system->instances,
           sizeof(MaterialInstance *) * g_material_system->instance_count);
    UNIFIED_FREE(g_material_system->instances);
    g_material_system->instances = new_instances;
  }

  g_material_system->instances[g_material_system->instance_count++] = instance;

  LOG_INFO(LOG_CAT_RENDERER, "Created material instance of '%s'",
           base_material->name);
  return instance;
}

void material_instance_destroy(MaterialInstance *instance) {
  if (!instance)
    return;

  LOG_INFO(LOG_CAT_RENDERER, "Destroying material instance '%s'",
           instance->name);

  // Free parameter overrides
  if (instance->param_overrides) {
    UNIFIED_FREE(instance->param_overrides);
  }

  // Remove from cache
  if (g_material_system) {
    for (uint32_t i = 0; i < g_material_system->instance_count; i++) {
      if (g_material_system->instances[i] == instance) {
        for (uint32_t j = i; j < g_material_system->instance_count - 1; j++) {
          g_material_system->instances[j] = g_material_system->instances[j + 1];
        }
        g_material_system->instance_count--;
        break;
      }
    }
  }

  UNIFIED_FREE(instance);
}

// =============================================================================
// PARAMETER MANAGEMENT - INSTANCES
// =============================================================================

static MaterialParam *
find_or_create_instance_override(MaterialInstance *instance, const char *name) {
  // Check if override exists
  for (uint32_t i = 0; i < instance->override_count; i++) {
    if (strcmp(instance->param_overrides[i].name, name) == 0) {
      return &instance->param_overrides[i];
    }
  }

  // Create new override
  if (instance->override_count >= instance->override_capacity) {
    instance->override_capacity *= 2;
    MaterialParam *new_overrides = (MaterialParam *)UNIFIED_ALLOC(
        sizeof(MaterialParam) * instance->override_capacity);
    memcpy(new_overrides, instance->param_overrides,
           sizeof(MaterialParam) * instance->override_count);
    UNIFIED_FREE(instance->param_overrides);
    instance->param_overrides = new_overrides;
  }

  MaterialParam *param = &instance->param_overrides[instance->override_count++];
  memset(param, 0, sizeof(MaterialParam));
  strncpy(param->name, name, sizeof(param->name) - 1);
  return param;
}

bool material_instance_set_param_float(MaterialInstance *instance,
                                       const char *name, float value) {
  if (!instance || !name)
    return false;

  MaterialParam *param = find_or_create_instance_override(instance, name);
  param->type = MATERIAL_PARAM_FLOAT;
  param->value.f = value;
  return true;
}

bool material_instance_set_param_vec3(MaterialInstance *instance,
                                      const char *name, const float *value) {
  if (!instance || !name || !value)
    return false;

  MaterialParam *param = find_or_create_instance_override(instance, name);
  param->type = MATERIAL_PARAM_VEC3;
  memcpy(param->value.vec3, value, sizeof(float) * 3);
  return true;
}

bool material_instance_set_param_vec4(MaterialInstance *instance,
                                      const char *name, const float *value) {
  if (!instance || !name || !value)
    return false;

  MaterialParam *param = find_or_create_instance_override(instance, name);
  param->type = MATERIAL_PARAM_VEC4;
  memcpy(param->value.vec4, value, sizeof(float) * 4);
  return true;
}

bool material_instance_set_param_texture(MaterialInstance *instance,
                                         const char *name, void *texture) {
  if (!instance || !name)
    return false;

  MaterialParam *param = find_or_create_instance_override(instance, name);
  param->type = MATERIAL_PARAM_TEXTURE_2D;
  param->value.texture = texture;
  return true;
}

const MaterialParam *material_instance_get_param(MaterialInstance *instance,
                                                 const char *name) {
  if (!instance || !name)
    return NULL;

  // Check overrides first
  for (uint32_t i = 0; i < instance->override_count; i++) {
    if (strcmp(instance->param_overrides[i].name, name) == 0) {
      return &instance->param_overrides[i];
    }
  }

  // Fall back to base material
  return material_get_param(instance->base_material, name);
}

// =============================================================================
// PBR MATERIAL CREATION
// =============================================================================

Material *material_create_pbr(const char *name, ShaderProgram *shader,
                              const PBRMaterialParams *params) {
  MaterialDesc desc = {.name = name,
                       .shader_program = shader,
                       .blend_mode = MATERIAL_BLEND_OPAQUE,
                       .cull_mode = MATERIAL_CULL_BACK,
                       .depth_test = true,
                       .depth_write = true,
                       .wireframe = false};

  Material *material = material_create(&desc);
  if (!material)
    return NULL;

  if (params) {
    material_set_param_vec4(material, "baseColor", params->base_color);
    material_set_param_float(material, "metallic", params->metallic);
    material_set_param_float(material, "roughness", params->roughness);
    material_set_param_float(material, "ao", params->ao);
    material_set_param_vec3(material, "emissive", params->emissive);

    if (params->albedo_texture)
      material_set_param_texture(material, "albedoTexture",
                                 params->albedo_texture);
    if (params->normal_texture)
      material_set_param_texture(material, "normalTexture",
                                 params->normal_texture);
    if (params->metallic_roughness_texture)
      material_set_param_texture(material, "metallicRoughnessTexture",
                                 params->metallic_roughness_texture);
  }

  return material;
}

// =============================================================================
// MATERIAL BINDING (Stubs)
// =============================================================================

void material_bind(Material *material) {
  if (!material)
    return;
  // TODO: Bind shader program and set parameters
  LOG_INFO(LOG_CAT_RENDERER, "Binding material '%s' (stub)", material->name);
}

void material_instance_bind(MaterialInstance *instance) {
  if (!instance)
    return;
  // TODO: Bind base material and apply overrides
  LOG_INFO(LOG_CAT_RENDERER, "Binding material instance '%s' (stub)",
           instance->name);
}
