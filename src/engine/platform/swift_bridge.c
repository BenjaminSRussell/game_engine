#include <stdio.h>
#include "core/engine.h"
#include "input/input.h"
#include "core/logger.h"
#include "renderer/renderer.h"
#include "core/gpu_acceleration.h"

// Globals for simple bridge management
static Engine g_engine;
static bool g_engine_initialized = false;

// Exported functions for Swift
void engine_swift_init(void) {
    if (g_engine_initialized) return;

    LOG_INFO("Initializing Engine from Swift Host...");
    
    // On macOS Swift host, we default to Metal
    EngineConfig config = engine_create_default_config();
    config.renderer_backend = GPU_BACKEND_METAL; 
    
    if (engine_init(&g_engine, &config)) {
        g_engine_initialized = true;
        LOG_INFO("Engine Initialized (Swift Host)");
    } else {
        LOG_ERROR("Failed to initialize Engine (Swift Host)");
    }
}

void engine_swift_shutdown(void) {
    if (g_engine_initialized) {
        engine_shutdown(&g_engine);
        g_engine_initialized = false;
    }
}

void engine_swift_update(void) {
    if (!g_engine_initialized) return;
    
    // Manually pump the loop for one frame/tick
    engine_update(&g_engine, 0.016f); // Assume 60fps for now
}

void engine_swift_render(void) {
    if (!g_engine_initialized) return;
    
    engine_render(&g_engine);
}

void engine_swift_resize(u32 width, u32 height) {
    if (g_engine_initialized && g_engine.subsystems.renderer) {
        // g_engine.subsystems.renderer->resize(g_engine.subsystems.renderer, width, height);
    }
}

// Input injection
void engine_swift_input_mouse_move(f32 x, f32 y) {
    // Inject into input system
}

void engine_swift_input_mouse_click(int button, bool pressed) {
    // Inject
}
