#include "unified_logger.h"
#include "unified_memory.h"
#include "unified_renderer.h"
#include "unified_shader.h"
#include <stdio.h>
#include <string.h>

// Simple test shaders
static const char *test_vertex_shader =
    "#version 330 core\n"
    "layout(location = 0) in vec3 position;\n"
    "void main() {\n"
    "    gl_Position = vec4(position, 1.0);\n"
    "}\n";

static const char *test_fragment_shader =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

int main(void) {
  printf("Starting Shader System Verification...\\n");

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
                                .swapchain_image_count = 2,
                                .enable_validation = true};

  RenderContext *renderer = render_init(&render_config);
  if (!renderer) {
    printf("FAILED: Renderer initialization\\n");
    return 1;
  }

  // Test 1: Initialize shader system
  if (!shader_system_init(renderer)) {
    printf("FAILED: Shader system init\\n");
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Shader system initialized\\n");

  // Test 2: Create vertex shader from source
  Shader *vertex_shader = shader_load_from_source(
      test_vertex_shader, SHADER_STAGE_VERTEX, "test_vertex");
  if (!vertex_shader) {
    printf("FAILED: Vertex shader creation\\n");
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Vertex shader created\\n");

  // Test 3: Create fragment shader from source
  Shader *fragment_shader = shader_load_from_source(
      test_fragment_shader, SHADER_STAGE_FRAGMENT, "test_fragment");
  if (!fragment_shader) {
    printf("FAILED: Fragment shader creation\\n");
    shader_destroy(vertex_shader);
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Fragment shader created\\n");

  // Test 4: Compile vertex shader
  char error_buffer[512];
  ShaderCompileResult result =
      shader_compile(vertex_shader, error_buffer, sizeof(error_buffer));
  if (result != SHADER_COMPILE_SUCCESS) {
    printf("FAILED: Vertex shader compilation: %s\\n", error_buffer);
    shader_destroy(fragment_shader);
    shader_destroy(vertex_shader);
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Vertex shader compiled\\n");

  // Test 5: Compile fragment shader
  result = shader_compile(fragment_shader, error_buffer, sizeof(error_buffer));
  if (result != SHADER_COMPILE_SUCCESS) {
    printf("FAILED: Fragment shader compilation: %s\\n", error_buffer);
    shader_destroy(fragment_shader);
    shader_destroy(vertex_shader);
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Fragment shader compiled\\n");

  // Test 6: Check compilation status
  if (!shader_is_compiled(vertex_shader) ||
      !shader_is_compiled(fragment_shader)) {
    printf("FAILED: Shaders not marked as compiled\\n");
    shader_destroy(fragment_shader);
    shader_destroy(vertex_shader);
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Shaders marked as compiled\\n");

  // Test 7: Create shader program
  ShaderProgramDesc program_desc = {.name = "test_program",
                                    .vertex_shader = vertex_shader,
                                    .fragment_shader = fragment_shader};

  ShaderProgram *program = shader_program_create(&program_desc);
  if (!program) {
    printf("FAILED: Shader program creation\\n");
    shader_destroy(fragment_shader);
    shader_destroy(vertex_shader);
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Shader program created\\n");

  // Test 8: Shader stage to string
  const char *stage_str = shader_stage_to_string(SHADER_STAGE_VERTEX);
  if (strcmp(stage_str, "VERTEX") != 0) {
    printf("FAILED: Shader stage to string (expected 'VERTEX', got '%s')\\n",
           stage_str);
    shader_program_destroy(program);
    shader_destroy(fragment_shader);
    shader_destroy(vertex_shader);
    shader_system_shutdown();
    render_shutdown(renderer);
    return 1;
  }
  printf("PASSED: Shader stage to string\\n");

  // Test 9: Hot reload toggle
  shader_enable_hot_reload(true);
  shader_enable_hot_reload(false);
  printf("PASSED: Hot reload toggle\\n");

  // Test 10: Cleanup
  shader_program_destroy(program);
  shader_destroy(fragment_shader);
  shader_destroy(vertex_shader);
  shader_system_shutdown();
  printf("PASSED: Shader system cleanup\\n");

  render_shutdown(renderer);
  logger_shutdown();

  printf("\\nShader System Verification Successful!\\n");
  return 0;
}
