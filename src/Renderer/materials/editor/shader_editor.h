#ifndef SHADER_EDITOR_H
#define SHADER_EDITOR_H

#include "include/core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Represents a shader editing session
typedef struct ShaderEditorSession {
    char shader_path[256];
    char *source_code;
    u32 source_length;
    bool is_dirty;
} ShaderEditorSession;

// Open a shader for editing
ShaderEditorSession *shader_editor_open(const char *shader_path);

// Close editing session
void shader_editor_close(ShaderEditorSession *session);

// Update source code
void shader_editor_set_source(ShaderEditorSession *session, const char *source);

// Save changes to disk
bool shader_editor_save(ShaderEditorSession *session);

// Check for syntax errors (mock)
bool shader_editor_check_syntax(ShaderEditorSession *session, char *out_error, u32 error_buffer_size);

#ifdef __cplusplus
}
#endif

#endif // SHADER_EDITOR_H
