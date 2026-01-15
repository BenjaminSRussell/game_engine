#pragma once

#include "../Public/unified_shader.h"

// Internal shader structure
struct Shader {
  char name[128];
  ShaderStage stage;
  ShaderLanguage language;
  char entry_point[64];

  // Source
  char *source_code;
  uint32_t source_size;
  char source_path[256];
  uint64_t source_timestamp; // For hot reload

  // Compiled bytecode
  void *bytecode;
  uint32_t bytecode_size;
  bool is_compiled;

  // Backend-specific handle
  void *native_handle;

  // Reflection
  ShaderReflection reflection;

  // Compilation status
  ShaderCompileResult last_compile_result;
  char last_error[512];
};

// Shader program (linked shaders)
struct ShaderProgram {
  char name[128];
  Shader *vertex_shader;
  Shader *fragment_shader;
  Shader *geometry_shader;
  Shader *compute_shader;

  // Backend-specific pipeline
  void *native_pipeline;
  bool is_linked;
};

// Shader system state
typedef struct {
  RenderContext *renderer;
  bool initialized;
  bool hot_reload_enabled;

  // Shader cache
  Shader **shaders;
  uint32_t shader_count;
  uint32_t shader_capacity;

  // Program cache
  ShaderProgram **programs;
  uint32_t program_count;
  uint32_t program_capacity;
} ShaderSystem;

// Global shader system
extern ShaderSystem *g_shader_system;
