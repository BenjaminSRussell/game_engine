#include "Public/Shader_System.h"
#include <stdlib.h>
#include <string.h>

// Struct Stubs
struct Shader {
  int id;
};
struct ShaderCompiler {
  int id;
};
struct ShaderCache {
  int id;
};
struct ShaderLibrary {
  int id;
};

static ShaderSystem g_shader_system = {0};

// Implementation Stubs
ShaderCompiler *ShaderCompiler_Create(void) {
  return malloc(sizeof(ShaderCompiler));
}
void ShaderCompiler_Destroy(ShaderCompiler *c) { free(c); }
Shader *ShaderCompiler_Compile(ShaderCompiler *c, const char *src,
                               ShaderType type) {
  (void)c;
  (void)src;
  (void)type;
  return malloc(sizeof(Shader));
}

ShaderCache *ShaderCache_Create(void) { return malloc(sizeof(ShaderCache)); }
void ShaderCache_Destroy(ShaderCache *c) { free(c); }
Shader *ShaderCache_Find(ShaderCache *c, const char *src) {
  (void)c;
  (void)src;
  return NULL;
}
void ShaderCache_Add(ShaderCache *c, const char *src, Shader *shader) {
  (void)c;
  (void)src;
  (void)shader;
}

ShaderLibrary *ShaderLibrary_Create(void) {
  return malloc(sizeof(ShaderLibrary));
}
void ShaderLibrary_Destroy(ShaderLibrary *l) { free(l); }
void ShaderLibrary_LoadBuiltIn(ShaderLibrary *l) { (void)l; }

bool ShaderSystem_Init(void) {
  g_shader_system.compiler = ShaderCompiler_Create();
  g_shader_system.cache = ShaderCache_Create();
  g_shader_system.library = ShaderLibrary_Create();

  // Load built-in shaders
  ShaderLibrary_LoadBuiltIn(g_shader_system.library);

  return true;
}

Shader *ShaderSystem_Compile(const char *source, ShaderType type) {
  // Check cache first
  Shader *cached = ShaderCache_Find(g_shader_system.cache, source);
  if (cached) {
    return cached;
  }

  // Compile shader
  Shader *shader =
      ShaderCompiler_Compile(g_shader_system.compiler, source, type);

  // Add to cache
  ShaderCache_Add(g_shader_system.cache, source, shader);

  return shader;
}

void ShaderSystem_Shutdown(void) {
  ShaderLibrary_Destroy(g_shader_system.library);
  ShaderCache_Destroy(g_shader_system.cache);
  ShaderCompiler_Destroy(g_shader_system.compiler);
}
