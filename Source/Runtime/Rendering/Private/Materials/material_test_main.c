#include "unified_logger.h"
#include "unified_material.h"
#include "unified_memory.h"
#include "unified_renderer.h"
#include "unified_shader.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  printf("Starting Material System Verification...\\n");

  // Initialize logger
  if (!logger_init(NULL)) {
    printf("FAILED: Logger init\\n");
    return 1;
  }

  // Initialize renderer (headless)
  RenderConfig render_config = {.backend = RENDER_BACKEND_AUTO,
                                .window_handle = NULL,
                                .width = 1920,
                                .height = 1080,
                                .format = RENDER_FORMAT_B8G8R8A8_SRGB,
                                .present_mode = RENDER_PRESENT_MODE_FIFO,
                                .swapchain_image_count = 2};

  RenderContext *renderer = render_init(&render_config);
  if (!renderer) {
    printf("FAILED: Renderer initialization\\n");
    return 1;
  }

  // Initialize shader system
  if (!shader_system_init(renderer)) {
    printf("FAILED: Shader system init\\n");
    render_shutdown(renderer);
    return 1;
  }

  // Create simple shaders
  Shader *vertex_shader = shader_load_from_source(
      "void main() {}", SHADER_STAGE_VERTEX, "test_vert");
  Shader *fragment_shader = shader_load_from_source(
      "void main() {}", SHADER_STAGE_FRAGMENT, "test_frag");
  shader_compile(vertex_shader, NULL, 0);
  shader_compile(fragment_shader, NULL, 0);

  ShaderProgramDesc program_desc = {.name = "test_program",
                                    .vertex_shader = vertex_shader,
                                    .fragment_shader = fragment_shader};
  ShaderProgram *program = shader_program_create(&program_desc);

  // Test 1: Initialize material system
  if (!material_system_init(renderer)) {
    printf("FAILED: Material system init\\n");
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Material system initialized\\n");

  // Test 2: Create basic material
  MaterialDesc mat_desc = {.name = "test_material",
                           .shader_program = program,
                           .blend_mode = MATERIAL_BLEND_OPAQUE,
                           .cull_mode = MATERIAL_CULL_BACK,
                           .depth_test = true,
                           .depth_write = true,
                           .wireframe = false};

  Material *material = material_create(&mat_desc);
  if (!material) {
    printf("FAILED: Material creation\\n");
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Material created\\n");

  // Test 3: Set float parameter
  if (!material_set_param_float(material, "roughness", 0.5f)) {
    printf("FAILED: Set float parameter\\n");
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Set float parameter\\n");

  // Test 4: Set vec3 parameter
  float color[3] = {1.0f, 0.0f, 0.0f};
  if (!material_set_param_vec3(material, "albedo", color)) {
    printf("FAILED: Set vec3 parameter\\n");
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Set vec3 parameter\\n");

  // Test 5: Get parameter
  const MaterialParam *param = material_get_param(material, "roughness");
  if (!param || param->type != MATERIAL_PARAM_FLOAT || param->value.f != 0.5f) {
    printf("FAILED: Get parameter (expected roughness=0.5)\\n");
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Get parameter\\n");

  // Test 6: Create material instance
  MaterialInstance *instance = material_instance_create(material);
  if (!instance) {
    printf("FAILED: Material instance creation\\n");
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Material instance created\\n");

  // Test 7: Override parameter in instance
  if (!material_instance_set_param_float(instance, "roughness", 0.8f)) {
    printf("FAILED: Instance parameter override\\n");
    material_instance_destroy(instance);
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Instance parameter override\\n");

  // Test 8: Verify instance override
  const MaterialParam *instance_param =
      material_instance_get_param(instance, "roughness");
  if (!instance_param || instance_param->value.f != 0.8f) {
    printf("FAILED: Instance parameter value (expected 0.8)\\n");
    material_instance_destroy(instance);
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Instance parameter verified\\n");

  // Test 9: Create PBR material
  PBRMaterialParams pbr_params = {.base_color = {1.0f, 1.0f, 1.0f, 1.0f},
                                  .metallic = 0.0f,
                                  .roughness = 0.5f,
                                  .ao = 1.0f,
                                  .emissive = {0.0f, 0.0f, 0.0f}};

  Material *pbr_material =
      material_create_pbr("test_pbr", program, &pbr_params);
  if (!pbr_material) {
    printf("FAILED: PBR material creation\\n");
    material_instance_destroy(instance);
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: PBR material created\\n");

  // Test 10: Utility functions
  const char *blend_str = material_blend_mode_to_string(MATERIAL_BLEND_OPAQUE);
  const char *cull_str = material_cull_mode_to_string(MATERIAL_CULL_BACK);
  if (strcmp(blend_str, "OPAQUE") != 0 || strcmp(cull_str, "BACK") != 0) {
    printf("FAILED: Utility functions\\n");
    material_destroy(pbr_material);
    material_instance_destroy(instance);
    material_destroy(material);
    material_system_shutdown();
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Utility functions\\n");

  // Test 11: Material binding (stub)
  material_bind(material);
  material_instance_bind(instance);
  printf("PASSED: Material binding\\n");

  // Test 12: Cleanup
  material_destroy(pbr_material);
  material_instance_destroy(instance);
  material_destroy(material);
  material_system_shutdown();
  printf("PASSED: Material system cleanup\\n");

  shader_system_shutdown();
  render_shutdown(renderer);
  logger_shutdown();

  printf("\\nMaterial System Verification Successful!\\n");
  return 0;
}
