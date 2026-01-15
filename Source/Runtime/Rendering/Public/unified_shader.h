#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct RenderContext RenderContext;
typedef struct Shader Shader;
typedef struct ShaderProgram ShaderProgram;

// Shader stage flags
typedef enum {
  SHADER_STAGE_VERTEX = 1 << 0,
  SHADER_STAGE_FRAGMENT = 1 << 1,
  SHADER_STAGE_COMPUTE = 1 << 2,
  SHADER_STAGE_GEOMETRY = 1 << 3,
  SHADER_STAGE_TESSELLATION_CONTROL = 1 << 4,
  SHADER_STAGE_TESSELLATION_EVAL = 1 << 5
} ShaderStage;

// Shader source language
typedef enum {
  SHADER_LANG_GLSL,  // OpenGL Shading Language
  SHADER_LANG_HLSL,  // DirectX HLSL
  SHADER_LANG_MSL,   // Metal Shading Language
  SHADER_LANG_SPIRV, // SPIR-V bytecode
  SHADER_LANG_AUTO   // Auto-detect from extension
} ShaderLanguage;

// Shader compilation result
typedef enum {
  SHADER_COMPILE_SUCCESS,
  SHADER_COMPILE_ERROR_SYNTAX,
  SHADER_COMPILE_ERROR_LINKING,
  SHADER_COMPILE_ERROR_IO,
  SHADER_COMPILE_ERROR_UNSUPPORTED
} ShaderCompileResult;

// Shader descriptor
typedef struct {
  const char *name;        // Shader name for debugging
  const char *source_path; // Path to shader file (optional)
  const char *source_code; // Inline source code (optional)
  uint32_t source_size;    // Size of source code
  ShaderStage stage;
  ShaderLanguage language;
  const char *entry_point; // Entry point function (default: "main")

  // Compilation options
  bool enable_debug;
  bool enable_optimization;
  const char **defines; // Array of #defines
  uint32_t define_count;
} ShaderDesc;

// Shader program descriptor (linked shaders)
typedef struct {
  const char *name;
  Shader *vertex_shader;
  Shader *fragment_shader;
  Shader *geometry_shader; // Optional
  Shader *compute_shader;  // Optional (mutually exclusive with vertex/fragment)
} ShaderProgramDesc;

// Shader reflection info
typedef struct {
  uint32_t uniform_count;
  uint32_t texture_count;
  uint32_t attribute_count;
  // TODO: Detailed reflection data
} ShaderReflection;

// =============================================================================
// SHADER MANAGEMENT
// =============================================================================

// Initialize shader system
bool shader_system_init(RenderContext *renderer);
void shader_system_shutdown(void);

// Create/destroy shaders
Shader *shader_create(const ShaderDesc *desc);
void shader_destroy(Shader *shader);

// Create/destroy shader programs
ShaderProgram *shader_program_create(const ShaderProgramDesc *desc);
void shader_program_destroy(ShaderProgram *program);

// Load shader from file
Shader *shader_load_from_file(const char *path, ShaderStage stage);
Shader *shader_load_from_source(const char *source, ShaderStage stage,
                                const char *name);

// Compilation
ShaderCompileResult shader_compile(Shader *shader, char *error_buffer,
                                   uint32_t error_buffer_size);
bool shader_is_compiled(Shader *shader);

// Hot reload
void shader_enable_hot_reload(bool enabled);
void shader_check_for_changes(void); // Call per frame

// Shader cache
void shader_cache_clear(void);
void shader_cache_save(const char *cache_path);
bool shader_cache_load(const char *cache_path);

// Reflection
const ShaderReflection *shader_get_reflection(Shader *shader);

// Utilities
const char *shader_stage_to_string(ShaderStage stage);
const char *shader_language_to_string(ShaderLanguage lang);
