#include "platform/swift_bridge.h"
#include <core/logger.h>

void engine_swift_init(void) {
    LOG_WARN("engine_swift_init called on unsupported platform/configuration (STUB)");
}

void engine_swift_shutdown(void) {}

void engine_swift_update(void) {}

void engine_swift_render(void) {}

void engine_swift_resize(u32 width, u32 height) {}

void engine_swift_input_mouse_move(f32 x, f32 y) {}

void engine_swift_input_mouse_click(int button, bool pressed) {}
