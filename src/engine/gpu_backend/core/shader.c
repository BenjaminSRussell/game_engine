// src/engine/renderer/core/shader.c
//
// Purpose: OpenGL shader compilation and management
//
#include <core/logger.h>
#include <renderer/shader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// Helper function to compile a single shader
static u32 compile_shader(const char *source, GLenum shader_type) {
  u32 shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  // Check compilation status
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetShaderInfoLog(shader, 512, NULL, info_log);
    const char *type_str =
        (shader_type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
    LOG_ERROR("Shader compilation failed (%s): %s", type_str, info_log);
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

bool shader_create_from_source(Shader *shader, const char *vertex_src,
                               const char *fragment_src) {
  if (!shader || !vertex_src || !fragment_src) {
    LOG_ERROR("shader_create_from_source: Invalid arguments");
    return false;
  }

  memset(shader, 0, sizeof(Shader));

  // Compile vertex shader
  shader->vertex_id = compile_shader(vertex_src, GL_VERTEX_SHADER);
  if (shader->vertex_id == 0) {
    return false;
  }

  // Compile fragment shader
  shader->fragment_id = compile_shader(fragment_src, GL_FRAGMENT_SHADER);
  if (shader->fragment_id == 0) {
    glDeleteShader(shader->vertex_id);
    return false;
  }

  // Link program
  shader->program_id = glCreateProgram();
  glAttachShader(shader->program_id, shader->vertex_id);
  glAttachShader(shader->program_id, shader->fragment_id);
  glLinkProgram(shader->program_id);

  // Check link status
  GLint success;
  glGetProgramiv(shader->program_id, GL_LINK_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetProgramInfoLog(shader->program_id, 512, NULL, info_log);
    LOG_ERROR("Shader linking failed: %s", info_log);
    glDeleteShader(shader->vertex_id);
    glDeleteShader(shader->fragment_id);
    glDeleteProgram(shader->program_id);
    return false;
  }

  // Cache uniform locations
  shader->uniforms.model = glGetUniformLocation(shader->program_id, "u_model");
  shader->uniforms.view = glGetUniformLocation(shader->program_id, "u_view");
  shader->uniforms.projection =
      glGetUniformLocation(shader->program_id, "u_projection");
  shader->uniforms.color = glGetUniformLocation(shader->program_id, "u_color");
  shader->uniforms.time = glGetUniformLocation(shader->program_id, "u_time");

  shader->is_compiled = true;
  LOG_INFO("Shader compiled successfully (program id: %u)", shader->program_id);

  return true;
}

bool shader_create_from_files(Shader *shader, const char *vertex_path,
                              const char *fragment_path) {
  // Read vertex shader
  FILE *vf = fopen(vertex_path, "r");
  if (!vf) {
    LOG_ERROR("Failed to open vertex shader: %s", vertex_path);
    return false;
  }

  fseek(vf, 0, SEEK_END);
  long vsize = ftell(vf);
  fseek(vf, 0, SEEK_SET);

  char *vsrc = (char *)malloc(vsize + 1);
  fread(vsrc, 1, vsize, vf);
  vsrc[vsize] = '\0';
  fclose(vf);

  // Read fragment shader
  FILE *ff = fopen(fragment_path, "r");
  if (!ff) {
    LOG_ERROR("Failed to open fragment shader: %s", fragment_path);
    free(vsrc);
    return false;
  }

  fseek(ff, 0, SEEK_END);
  long fsize = ftell(ff);
  fseek(ff, 0, SEEK_SET);

  char *fsrc = (char *)malloc(fsize + 1);
  fread(fsrc, 1, fsize, ff);
  fsrc[fsize] = '\0';
  fclose(ff);

  // Compile from source
  bool result = shader_create_from_source(shader, vsrc, fsrc);

  free(vsrc);
  free(fsrc);

  return result;
}

void shader_destroy(Shader *shader) {
  if (shader && shader->is_compiled) {
    glDeleteProgram(shader->program_id);
    glDeleteShader(shader->vertex_id);
    glDeleteShader(shader->fragment_id);
    memset(shader, 0, sizeof(Shader));
  }
}

void shader_bind(const Shader *shader) {
  if (shader && shader->is_compiled) {
    glUseProgram(shader->program_id);
  }
}

void shader_unbind(void) { glUseProgram(0); }

void shader_set_int(const Shader *shader, const char *name, i32 value) {
  if (shader && shader->is_compiled) {
    GLint loc = glGetUniformLocation(shader->program_id, name);
    if (loc != -1)
      glUniform1i(loc, value);
  }
}

void shader_set_float(const Shader *shader, const char *name, f32 value) {
  if (shader && shader->is_compiled) {
    GLint loc = glGetUniformLocation(shader->program_id, name);
    if (loc != -1)
      glUniform1f(loc, value);
  }
}

void shader_set_vec3(const Shader *shader, const char *name, const f32 *value) {
  if (shader && shader->is_compiled) {
    GLint loc = glGetUniformLocation(shader->program_id, name);
    if (loc != -1)
      glUniform3fv(loc, 1, value);
  }
}

void shader_set_vec4(const Shader *shader, const char *name, const f32 *value) {
  if (shader && shader->is_compiled) {
    GLint loc = glGetUniformLocation(shader->program_id, name);
    if (loc != -1)
      glUniform4fv(loc, 1, value);
  }
}

void shader_set_mat4(const Shader *shader, const char *name, const f32 *value) {
  if (shader && shader->is_compiled) {
    GLint loc = glGetUniformLocation(shader->program_id, name);
    if (loc != -1)
      glUniformMatrix4fv(loc, 1, GL_FALSE, value);
  }
}
