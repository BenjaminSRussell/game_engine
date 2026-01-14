#include "materials/editor/shader_editor.h"
#include "include/core/memory.h"
#include "include/core/logger.h"
#include <stdio.h>
#include <string.h>

ShaderEditorSession *shader_editor_open(const char *shader_path) {
    if (!shader_path) return NULL;

    ShaderEditorSession *session = (ShaderEditorSession*)MALLOC(sizeof(ShaderEditorSession));
    if (!session) return NULL;

    strncpy(session->shader_path, shader_path, sizeof(session->shader_path) - 1);
    session->shader_path[sizeof(session->shader_path) - 1] = '\0';

    // Stub: In real app, load from VFS
    const char *stub_source = "// Shader Source\nvoid main() {}";
    session->source_length = strlen(stub_source);
    session->source_code = (char*)MALLOC(session->source_length + 1);
    strcpy(session->source_code, stub_source);

    session->is_dirty = false;

    LOG_INFO("Shader editor opened for: %s", shader_path);
    return session;
}

void shader_editor_close(ShaderEditorSession *session) {
    if (!session) return;
    if (session->source_code) FREE(session->source_code);
    FREE(session);
}

void shader_editor_set_source(ShaderEditorSession *session, const char *source) {
    if (!session || !source) return;

    size_t len = strlen(source);
    if (len > session->source_length) {
        session->source_code = (char*)REALLOC(session->source_code, len + 1);
    }
    strcpy(session->source_code, source);
    session->source_length = (u32)len;
    session->is_dirty = true;
}

bool shader_editor_save(ShaderEditorSession *session) {
    if (!session) return false;
    // Stub: Save to VFS
    LOG_INFO("Saved shader: %s", session->shader_path);
    session->is_dirty = false;
    return true;
}

bool shader_editor_check_syntax(ShaderEditorSession *session, char *out_error, u32 error_buffer_size) {
    // Stub: Always success
    if (out_error && error_buffer_size > 0) out_error[0] = '\0';
    return true;
}
