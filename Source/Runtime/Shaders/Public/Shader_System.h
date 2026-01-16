#ifndef ULTIMATE_ENGINE_SHADER_SYSTEM_H
#define ULTIMATE_ENGINE_SHADER_SYSTEM_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

typedef struct Shader Shader;
typedef struct ShaderCompiler ShaderCompiler;
typedef struct ShaderCache ShaderCache;
typedef struct ShaderLibrary ShaderLibrary;

typedef enum ShaderType {
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_FRAGMENT,
  SHADER_TYPE_COMPUTE,
  SHADER_TYPE_GEOMETRY
} ShaderType;

typedef struct ShaderSystem {
  ShaderCompiler *compiler;
  ShaderCache *cache;
  ShaderLibrary *library;
} ShaderSystem;

// Lifecycle
bool ShaderSystem_Init(void);
void ShaderSystem_Shutdown(void);

// Subsystem
Shader *ShaderSystem_Compile(const char *source, ShaderType type);

// Internal Subsystems
ShaderCompiler *ShaderCompiler_Create(void);
void ShaderCompiler_Destroy(ShaderCompiler *c);
Shader *ShaderCompiler_Compile(ShaderCompiler *c, const char *src,
                               ShaderType type);

ShaderCache *ShaderCache_Create(void);
void ShaderCache_Destroy(ShaderCache *c);
Shader *ShaderCache_Find(ShaderCache *c, const char *src);
void ShaderCache_Add(ShaderCache *c, const char *src, Shader *shader);

ShaderLibrary *ShaderLibrary_Create(void);
void ShaderLibrary_Destroy(ShaderLibrary *l);
void ShaderLibrary_LoadBuiltIn(ShaderLibrary *l);

#endif // ULTIMATE_ENGINE_SHADER_SYSTEM_H
