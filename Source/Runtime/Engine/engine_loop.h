#ifndef ENGINE_LOOP_H
#define ENGINE_LOOP_H

#include <common.h>

// Initialize engine loop systems
void engine_loop_init(void);

// Update game logic (one frame)
void engine_update(void);

// Render game frame
void engine_render(void);

// Shutdown engine loop
void engine_shutdown(void);

// Run the main loop (blocking)
void engine_run(void);

#endif
