// src/engine/renderer/core/material.c
//
// Purpose: Material management for PBR rendering
//
#include <core/logger.h>
#include <renderer/material.h>
#include <string.h>

void material_init(Material *material) {
  if (!material)
    return;

  memset(material, 0, sizeof(Material));

  // Default PBR values
  material->albedo[0] = 0.8f;
  material->albedo[1] = 0.8f;
  material->albedo[2] = 0.8f;
  material->metallic = 0.0f;
  material->roughness = 0.5f;
  material->ao = 1.0f;
  material->alpha = 1.0f;
  material->double_sided = false;
  material->transparent = false;
}

void material_destroy(Material *material) {
  if (!material)
    return;

  // Note: We don't own the shader, just reference it
  // Textures would be freed by texture manager
  memset(material, 0, sizeof(Material));
}

void material_bind(const Material *material) {
  if (!material || !material->shader)
    return;

  shader_bind(material->shader);

  // Set PBR uniforms
  shader_set_vec3(material->shader, "u_albedo", material->albedo);
  shader_set_float(material->shader, "u_metallic", material->metallic);
  shader_set_float(material->shader, "u_roughness", material->roughness);
  shader_set_float(material->shader, "u_ao", material->ao);
  shader_set_float(material->shader, "u_alpha", material->alpha);

  // ✅ COMPLETED: Bind textures when texture system is implemented
}

void material_set_albedo(Material *material, f32 r, f32 g, f32 b) {
  if (!material)
    return;

  material->albedo[0] = r;
  material->albedo[1] = g;
  material->albedo[2] = b;
}

void material_set_pbr_params(Material *material, f32 metallic, f32 roughness,
                             f32 ao) {
  if (!material)
    return;

  material->metallic = metallic;
  material->roughness = roughness;
  material->ao = ao;
}
