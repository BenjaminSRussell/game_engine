// include/renderer/shader.h
//
// Purpose: Shader compilation and management
//
#ifndef ENGINE_SHADER_H
#define ENGINE_SHADER_H

#include "include/common.h"

typedef enum {
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_FRAGMENT,
  SHADER_TYPE_GEOMETRY,
  SHADER_TYPE_COMPUTE,
} ShaderType;

typedef struct Shader {
  u32 program_id;  // OpenGL program ID
  u32 vertex_id;   // Vertex shader ID
  u32 fragment_id; // Fragment shader ID
  bool is_compiled;

  // Uniform locations cache
  struct {
    i32 model;
    i32 view;
    i32 projection;
    i32 color;
    i32 time;
  } uniforms;
} Shader;

// Shader lifecycle
bool shader_create_from_source(Shader *shader, const char *vertex_src,
                               const char *fragment_src);
bool shader_create_from_files(Shader *shader, const char *vertex_path,
                              const char *fragment_path);
void shader_destroy(Shader *shader);

// Shader operations
void shader_bind(const Shader *shader);
void shader_unbind(void);

// Uniform setters
void shader_set_int(const Shader *shader, const char *name, i32 value);
void shader_set_float(const Shader *shader, const char *name, f32 value);
void shader_set_vec3(const Shader *shader, const char *name, const f32 *value);
void shader_set_vec4(const Shader *shader, const char *name, const f32 *value);
void shader_set_mat4(const Shader *shader, const char *name, const f32 *value);

#endif // ENGINE_SHADER_H
