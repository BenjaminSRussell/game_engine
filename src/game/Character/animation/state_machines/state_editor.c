#include <common.h>

void state_editor_init(void) {}

void state_editor_create_state(const char *name, void *animation_clip) {
  (void)name;
  (void)animation_clip;
}

void state_editor_add_transition(void *from_state, void *to_state,
                                 void *condition) {
  (void)from_state;
  (void)to_state;
  (void)condition;
}

void state_editor_render_ui(void) {}
