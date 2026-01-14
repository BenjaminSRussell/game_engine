#ifndef SHADER_COMPILER_H
#define SHADER_COMPILER_H

#include "include/core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ShaderStage {
    SHADER_STAGE_VERTEX,
    SHADER_STAGE_FRAGMENT,
    SHADER_STAGE_COMPUTE,
    SHADER_STAGE_GEOMETRY
} ShaderStage;

// Compiler options
typedef struct ShaderCompilerOptions {
    bool optimize;
    bool debug_info;
    const char *defines; // "DEFINE1=1;DEFINE2=0"
} ShaderCompilerOptions;

// Compile a shader source
// Returns 0 on failure, non-zero program/shader ID on success
u32 shader_compiler_compile(const char *source, ShaderStage stage, const ShaderCompilerOptions *options, char *out_log, u32 log_size);

#ifdef __cplusplus
}
#endif

#endif // SHADER_COMPILER_H
