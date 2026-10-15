// EditorApp entry point. Creates the shared editor context, runs one update and tears it
// down. The windowed UI is not wired yet, so this headless driver is what the target
// builds around.

#include "editor_common.h"

int main(void) {
    EditorContext* ctx = editor_context_create();
    if (!ctx) {
        return 1;
    }

    if (!editor_context_init(ctx)) {
        editor_context_destroy(ctx);
        return 1;
    }

    editor_context_update(ctx, 1.0f / 60.0f);
    editor_context_destroy(ctx);
    return 0;
}
