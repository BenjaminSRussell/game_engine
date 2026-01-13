#ifndef UI_LAYOUT_ENGINE_H
#define UI_LAYOUT_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

// UI Layout Engine - Placeholder header
typedef struct UILayoutEngine {
    uint32_t id;
    bool initialized;
} UILayoutEngine;

void ui_layout_engine_init(UILayoutEngine *engine);
void ui_layout_engine_shutdown(UILayoutEngine *engine);

#endif // UI_LAYOUT_ENGINE_H
