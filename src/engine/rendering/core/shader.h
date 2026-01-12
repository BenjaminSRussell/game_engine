// src/engine/rendering/core/shader.h
// Consolidated Shader System - Header file

#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Shader Types
// ============================================================================

typedef enum {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_COMPUTE,
    SHADER_TYPE_GEOMETRY,
    SHADER_TYPE_TESSELLATION_CONTROL,
    SHADER_TYPE_TESSELLATION_EVALUATION,
    SHADER_TYPE_COUNT
} ShaderType;

// ============================================================================
// Shader System Lifecycle
// ============================================================================

// Initialize the shader system
bool shader_system_init(void);

// Shutdown the shader system
void shader_system_shutdown(void);

// Check if shader system is initialized
bool shader_system_is_initialized(void);

// ============================================================================
// Shader Loading
// ============================================================================

// Load shader from file
uint32_t shader_load_from_file(const char* filepath, ShaderType type);

// Load shader from source string
uint32_t shader_load_from_source(const char* name, const char* source_code, ShaderType type);

// ============================================================================
// Shader Compilation
// ============================================================================

// Compile a shader module
bool shader_compile(uint32_t module_id);

// Check if shader is compiled
bool shader_is_compiled(uint32_t module_id);

// Get shader compilation error message
const char* shader_get_error(uint32_t module_id);

// Get platform-specific shader handle
void* shader_get_native_handle(uint32_t module_id);

// ============================================================================
// Hot Reload System
// ============================================================================

// Enable or disable hot reload
void shader_system_enable_hot_reload(bool enabled);

// Check for shader file changes and reload if needed
void shader_system_check_hot_reload(void);

// ============================================================================
// Statistics and Debugging
// ============================================================================

// Log shader system statistics
void shader_system_log_stats(void);

#ifdef __cplusplus
}
#endif

#endif // SHADER_H
