#ifndef SWIFT_BRIDGE_H
#define SWIFT_BRIDGE_H

#include <core/types.h>

// Initialize the engine (Swift host)
void engine_swift_init(void);

// Shutdown the engine
void engine_swift_shutdown(void);

// Update game loop (single tick)
void engine_swift_update(void);

// Render single frame
void engine_swift_render(void);

// Resize viewport
void engine_swift_resize(u32 width, u32 height);

// Input Injection
void engine_swift_input_mouse_move(f32 x, f32 y);
void engine_swift_input_mouse_click(int button, bool pressed);

#endif // SWIFT_BRIDGE_H
