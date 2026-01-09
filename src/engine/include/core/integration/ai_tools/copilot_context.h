#ifndef COPILOT_CONTEXT_H
#define COPILOT_CONTEXT_H

void copilot_context_init(void);
const char* copilot_get_scene_context(void);
const char* copilot_get_system_prompt(void);
void copilot_explain_current_state(const char *game_state);
void copilot_generate_script(const char *description, char *output, int max_len);

#endif // COPILOT_CONTEXT_H
