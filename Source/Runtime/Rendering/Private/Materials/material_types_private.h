#pragma once

#include "../Public/unified_material.h"
#include "../Public/unified_shader.h"

// Internal material structure
struct Material {
  char name[128];
  ShaderProgram *shader_program;

  // Render state
  MaterialBlendMode blend_mode;
  MaterialCullMode cull_mode;
  bool depth_test;
  bool depth_write;
  bool wireframe;

  // Parameters
  MaterialParam *params;
  uint32_t param_count;
  uint32_t param_capacity;

  // Backend-specific data
  void *native_handle;
};

// Material instance (shares base material, overrides parameters)
struct MaterialInstance {
  Material *base_material;
  char name[128];

  // Overridden parameters
  MaterialParam *param_overrides;
  uint32_t override_count;
  uint32_t override_capacity;

  // Backend-specific data
  void *native_handle;
};

// Material system state
typedef struct {
  RenderContext *renderer;
  bool initialized;

  // Material cache
  Material **materials;
  uint32_t material_count;
  uint32_t material_capacity;

  // Instance cache
  MaterialInstance **instances;
  uint32_t instance_count;
  uint32_t instance_capacity;
} MaterialSystem;

// Global material system
extern MaterialSystem *g_material_system;
