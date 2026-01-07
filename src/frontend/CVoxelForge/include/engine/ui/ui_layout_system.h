#ifndef UI_LAYOUT_SYSTEM_H
#define UI_LAYOUT_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

// UI Layout System - Placeholder header
// TODO: Define layout system structures and functions

typedef struct UILayoutSystem {
    uint32_t id;
    bool initialized;
} UILayoutSystem;

void ui_layout_system_init(UILayoutSystem *system);
void ui_layout_system_shutdown(UILayoutSystem *system);
void ui_layout_system_update(UILayoutSystem *system, float dt);

#endif // UI_LAYOUT_SYSTEM_H
