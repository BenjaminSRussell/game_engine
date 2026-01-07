#include "integration/ai_tools/copilot_context.h"
#include <stdio.h>
#include <string.h>

static char g_context_buffer[4096];

void copilot_context_init() {
    memset(g_context_buffer, 0, sizeof(g_context_buffer));
}

const char* copilot_get_scene_context() {
    // Generate JSON context of active scene
    snprintf(g_context_buffer, sizeof(g_context_buffer),
        "{\"scene\":\"main\",\"entities\":[],\"scripts\":[\"player.lua\",\"enemy.lua\"]}");
    return g_context_buffer;
}

const char* copilot_get_system_prompt() {
    return "You are coding for a C-based game engine. Use ECS patterns.";
}

void copilot_explain_current_state(const char *game_state) {
    // Send to LLM for debugging advice
    printf("Explaining state to AI: %s\n", game_state);
}

void copilot_generate_script(const char *description, char *output, int max_len) {
    // Ask LLM to generate script
    snprintf(output, max_len, "-- Generated: %s\nfunction update() end", description);
}
