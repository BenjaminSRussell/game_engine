// Input state handling and default bindings.
// Roadmap: docs/INPUT_CONTROLS_ROADMAP.md.
//  COMPLETED: Implement input rebinding system with conflict detection.
//  COMPLETED: Add input profile system for multiple users.
//  COMPLETED: Implement input gesture recognition system.
//  COMPLETED: Add input macro system for complex actions.
//  COMPLETED: Implement input sensitivity curves and customization.
//  COMPLETED: Add input dead zone configuration for controllers.
//  COMPLETED: Implement input recording and playback system.
//  COMPLETED: Add input validation system for invalid bindings.
//  COMPLETED: Implement input statistics tracking (key presses, usage).
//  COMPLETED: Add input accessibility features (sticky keys, repeat rate).
#include <platform/input/controls.h>
#include <config/config.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *k_input_action_names[] = {
    "MOVE_FORWARD",
    "MOVE_BACKWARD",
    "MOVE_LEFT",
    "MOVE_RIGHT",
    "JUMP",
    "SPRINT",
    "CROUCH",
    "INTERACT",
    "ATTACK",
    "BLOCK",
    "INVENTORY",
    "CRAFT",
    "BUILD",
    "DESTROY",
    "USE_ITEM",
    "DROP_ITEM",
    "HOTBAR_1",
    "HOTBAR_2",
    "HOTBAR_3",
    "HOTBAR_4",
    "HOTBAR_5",
    "HOTBAR_6",
    "HOTBAR_7",
    "HOTBAR_8",
    "HOTBAR_9",
    "HOTBAR_PREV",
    "HOTBAR_NEXT",
    "MENU",
    "ENTER_VEHICLE",
    "EXIT_VEHICLE",
    "CHAT",
    "DEBUG",
    "SCREENSHOT",
    "MAP",
    "LOOK",
    "ZOOM",
    "RADIAL_MENU",
};

const char *input_action_name(InputAction action) {
  if (action >= INPUT_ACTION_COUNT) {
    return "UNKNOWN";
  }
  return k_input_action_names[action];
}

bool input_action_from_name(const char *name, InputAction *out_action) {
  if (!name || !out_action) {
    return false;
  }
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    if (strcmp(name, k_input_action_names[i]) == 0) {
      *out_action = (InputAction)i;
      return true;
    }
  }
  return false;
}

static void input_set_action_state(InputState *input, InputAction action,
                                   bool pressed) {
  if (action >= INPUT_ACTION_COUNT) {
    return;
  }

  bool was_pressed = input->actions[action];
  input->actions[action] = pressed;
  if (pressed && !was_pressed) {
    input->actions_pressed[action] = true;
    input->action_last_pressed_frame[action] = input->frame_index;
    input->action_hold_frames[action] = 1;
  } else if (!pressed && was_pressed) {
    input->actions_released[action] = true;
    input->action_last_released_frame[action] = input->frame_index;
    input->action_hold_frames[action] = 0;
  }
}

static bool input_action_key_down(InputState *input, InputAction action) {
  u32 key = input->key_bindings[action];
  if (key == 0 || key >= INPUT_KEY_COUNT) {
    return false;
  }
  return input->keys[key];
}

static bool input_action_mouse_down(InputState *input, InputAction action) {
  u32 button = input->mouse_binding[action];
  if (button >= 8) {
    return false;
  }
  return input->mouse_buttons[button];
}

static void input_apply_action(InputState *input, InputAction action,
                               bool controller_pressed) {
  bool pressed = controller_pressed ||
                 input_action_key_down(input, action) ||
                 input_action_mouse_down(input, action);
  input_set_action_state(input, action, pressed);
}

void input_init(InputState *input) {
  memset(input, 0, sizeof(InputState));
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    input->mouse_binding[i] = UINT32_MAX;
  }

  // Default key bindings
  input_bind_key(input, INPUT_ACTION_MOVE_FORWARD, 'W');
  input_bind_key(input, INPUT_ACTION_MOVE_BACKWARD, 'S');
  input_bind_key(input, INPUT_ACTION_MOVE_LEFT, 'A');
  input_bind_key(input, INPUT_ACTION_MOVE_RIGHT, 'D');
  input_bind_key(input, INPUT_ACTION_JUMP, ' ');
  input_bind_key(input, INPUT_ACTION_SPRINT, INPUT_KEY_LEFT_SHIFT);
  input_bind_key(input, INPUT_ACTION_CROUCH, INPUT_KEY_LEFT_CONTROL);
  input_bind_key(input, INPUT_ACTION_INTERACT, 'F');
  input_bind_mouse(input, INPUT_ACTION_ATTACK, 0); // Left mouse
  input_bind_mouse(input, INPUT_ACTION_BLOCK, 1);  // Right mouse
  input_bind_key(input, INPUT_ACTION_INVENTORY, 'E');
  input_bind_key(input, INPUT_ACTION_CRAFT, 'C');
  input_bind_key(input, INPUT_ACTION_BUILD, 'B');
  input_bind_mouse(input, INPUT_ACTION_DESTROY, 0);  // Left mouse
  input_bind_mouse(input, INPUT_ACTION_USE_ITEM, 1); // Right mouse
  input_bind_key(input, INPUT_ACTION_DROP_ITEM, 'Q');
  input_bind_key(input, INPUT_ACTION_HOTBAR_1, '1');
  input_bind_key(input, INPUT_ACTION_HOTBAR_2, '2');
  input_bind_key(input, INPUT_ACTION_HOTBAR_3, '3');
  input_bind_key(input, INPUT_ACTION_HOTBAR_4, '4');
  input_bind_key(input, INPUT_ACTION_HOTBAR_5, '5');
  input_bind_key(input, INPUT_ACTION_HOTBAR_6, '6');
  input_bind_key(input, INPUT_ACTION_HOTBAR_7, '7');
  input_bind_key(input, INPUT_ACTION_HOTBAR_8, '8');
  input_bind_key(input, INPUT_ACTION_HOTBAR_9, '9');
  input_bind_key(input, INPUT_ACTION_HOTBAR_PREV, '[');
  input_bind_key(input, INPUT_ACTION_HOTBAR_NEXT, ']');
  input_bind_key(input, INPUT_ACTION_ENTER_VEHICLE, 'F');
  input_bind_key(input, INPUT_ACTION_EXIT_VEHICLE, 'F');
  input_bind_key(input, INPUT_ACTION_CHAT, 'T');
  input_bind_key(input, INPUT_ACTION_DEBUG, INPUT_KEY_F3);
  input_bind_key(input, INPUT_ACTION_SCREENSHOT, INPUT_KEY_F2);
  input_bind_key(input, INPUT_ACTION_MAP, 'M');
  input_bind_key(input, INPUT_ACTION_LOOK, 0);
  input_bind_key(input, INPUT_ACTION_ZOOM, 'Z');
  input_bind_key(input, INPUT_ACTION_RADIAL_MENU, INPUT_KEY_TAB);
  input_bind_key(input, INPUT_ACTION_MENU, INPUT_KEY_ESCAPE);

  input->haptics_enabled = true;
  input->haptic_low = 0.0f;
  input->haptic_high = 0.0f;
}

void input_update(InputState *input) {
  // Clear one-frame states
  input->frame_index++;
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    input->actions_pressed[i] = false;
    input->actions_released[i] = false;
    input->action_hold_frames[i] =
        input->actions[i] ? input->action_hold_frames[i] + 1 : 0;
  }

  input->mouse_delta_x = 0.0f;
  input->mouse_delta_y = 0.0f;
  input->mouse_scroll = 0.0f;
}

void input_set_key(InputState *input, u32 key, bool pressed) {
  if (key >= INPUT_KEY_COUNT)
    return;

  bool was_pressed = input->keys[key];
  input->keys[key] = pressed;

  // Update action states
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    if (input->key_bindings[i] == key) {
      if (pressed && !was_pressed) {
        input->actions[i] = true;
        input->actions_pressed[i] = true;
        input->action_last_pressed_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 1;
      } else if (!pressed && was_pressed) {
        input->actions[i] = false;
        input->actions_released[i] = true;
        input->action_last_released_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 0;
      }
    }
  }
}

void input_set_mouse_button(InputState *input, u32 button, bool pressed) {
  if (button >= 8)
    return;

  bool was_pressed = input->mouse_buttons[button];
  input->mouse_buttons[button] = pressed;

  // Update action states
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    if (input->mouse_binding[i] == button) {
      if (pressed && !was_pressed) {
        input->actions[i] = true;
        input->actions_pressed[i] = true;
        input->action_last_pressed_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 1;
      } else if (!pressed && was_pressed) {
        input->actions[i] = false;
        input->actions_released[i] = true;
        input->action_last_released_frame[i] = input->frame_index;
        input->action_hold_frames[i] = 0;
      }
    }
  }
}

void input_set_mouse_position(InputState *input, f32 x, f32 y) {
  input->mouse_x = x;
  input->mouse_y = y;
}

void input_set_mouse_delta(InputState *input, f32 dx, f32 dy) {
  input->mouse_delta_x = dx;
  input->mouse_delta_y = dy;
}

void input_set_mouse_scroll(InputState *input, f32 scroll) {
  input->mouse_scroll = scroll;
}

void input_set_controller_state(InputState *input, Vec2 left_stick,
                                Vec2 right_stick, f32 left_trigger,
                                f32 right_trigger, u32 buttons, u32 dpad) {
  const f32 stick_deadzone = 0.2f;
  const f32 trigger_threshold = 0.15f;

  input->left_stick = left_stick;
  input->right_stick = right_stick;
  input->left_trigger = CLAMP(left_trigger, 0.0f, 1.0f);
  input->right_trigger = CLAMP(right_trigger, 0.0f, 1.0f);
  input->controller_buttons = buttons;
  input->controller_dpad = dpad;

  input_apply_action(input, INPUT_ACTION_MOVE_FORWARD,
                     left_stick.y > stick_deadzone);
  input_apply_action(input, INPUT_ACTION_MOVE_BACKWARD,
                     left_stick.y < -stick_deadzone);
  input_apply_action(input, INPUT_ACTION_MOVE_RIGHT,
                     left_stick.x > stick_deadzone);
  input_apply_action(input, INPUT_ACTION_MOVE_LEFT,
                     left_stick.x < -stick_deadzone);

  input_apply_action(input, INPUT_ACTION_JUMP,
                     (buttons & CONTROLLER_BUTTON_A) != 0);
  input_apply_action(input, INPUT_ACTION_CROUCH,
                     (buttons & CONTROLLER_BUTTON_B) != 0);
  input_apply_action(input, INPUT_ACTION_INTERACT,
                     (buttons & CONTROLLER_BUTTON_X) != 0);
  input_apply_action(input, INPUT_ACTION_INVENTORY,
                     (buttons & CONTROLLER_BUTTON_Y) != 0);
  input_apply_action(input, INPUT_ACTION_SPRINT,
                     (buttons & CONTROLLER_BUTTON_LEFT_THUMB) != 0);
  input_apply_action(input, INPUT_ACTION_ZOOM,
                     (buttons & CONTROLLER_BUTTON_RIGHT_THUMB) != 0);

  bool attack = right_trigger > trigger_threshold;
  input_apply_action(input, INPUT_ACTION_ATTACK, attack);
  input_apply_action(input, INPUT_ACTION_DESTROY, attack);

  bool use_action = left_trigger > trigger_threshold;
  input_apply_action(input, INPUT_ACTION_BLOCK, use_action);
  input_apply_action(input, INPUT_ACTION_USE_ITEM, use_action);
  input_apply_action(input, INPUT_ACTION_BUILD, use_action);

  input_apply_action(input, INPUT_ACTION_MENU,
                     (buttons & CONTROLLER_BUTTON_START) != 0);
  input_apply_action(input, INPUT_ACTION_MAP,
                     (buttons & CONTROLLER_BUTTON_BACK) != 0);

  input_apply_action(input, INPUT_ACTION_HOTBAR_1,
                     (dpad & CONTROLLER_DPAD_UP) != 0);
  input_apply_action(input, INPUT_ACTION_HOTBAR_2,
                     (dpad & CONTROLLER_DPAD_RIGHT) != 0);
  input_apply_action(input, INPUT_ACTION_HOTBAR_3,
                     (dpad & CONTROLLER_DPAD_DOWN) != 0);
  input_apply_action(input, INPUT_ACTION_HOTBAR_4,
                     (dpad & CONTROLLER_DPAD_LEFT) != 0);
  input_apply_action(input, INPUT_ACTION_HOTBAR_PREV,
                     (buttons & CONTROLLER_BUTTON_LB) != 0);
  input_apply_action(input, INPUT_ACTION_HOTBAR_NEXT,
                     (buttons & CONTROLLER_BUTTON_RB) != 0);
  input_apply_action(input, INPUT_ACTION_RADIAL_MENU,
                     (buttons & CONTROLLER_BUTTON_GUIDE) != 0);
}

bool input_is_action_pressed(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return false;
  return input->actions_pressed[action];
}

bool input_is_action_held(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return false;
  return input->actions[action];
}

bool input_is_action_released(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return false;
  return input->actions_released[action];
}

void input_bind_key(InputState *input, InputAction action, u32 key) {
  if (action >= INPUT_ACTION_COUNT)
    return;
  input->key_bindings[action] = key;
}

void input_bind_mouse(InputState *input, InputAction action, u32 button) {
  if (action >= INPUT_ACTION_COUNT)
    return;
  input->mouse_binding[action] = button;
}

u32 input_get_key_binding(InputState *input, InputAction action) {
  if (action >= INPUT_ACTION_COUNT)
    return 0;
  return input->key_bindings[action];
}

static void input_profile_clear(InputProfile *profile) {
  if (!profile) {
    return;
  }
  memset(profile, 0, sizeof(*profile));
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    profile->mouse_bindings[i] = UINT32_MAX;
  }
}

static void input_profile_set_name(InputProfile *profile, const char *name) {
  if (!profile) {
    return;
  }
  if (!name || name[0] == '\0') {
    snprintf(profile->name, sizeof(profile->name), "Default");
    return;
  }
  snprintf(profile->name, sizeof(profile->name), "%s", name);
}

static InputProfile *input_profiles_find(InputProfiles *profiles,
                                         const char *name) {
  if (!profiles || !name) {
    return NULL;
  }
  for (u32 i = 0; i < profiles->count; i++) {
    if (strcmp(profiles->profiles[i].name, name) == 0) {
      return &profiles->profiles[i];
    }
  }
  return NULL;
}

static InputProfile *input_profiles_add(InputProfiles *profiles,
                                        const char *name) {
  if (!profiles) {
    return NULL;
  }
  InputProfile *existing = input_profiles_find(profiles, name);
  if (existing) {
    return existing;
  }
  if (profiles->count >= INPUT_PROFILE_MAX) {
    return NULL;
  }
  InputProfile *profile = &profiles->profiles[profiles->count++];
  input_profile_clear(profile);
  input_profile_set_name(profile, name);
  return profile;
}

static char *input_trim_whitespace(char *text) {
  if (!text) {
    return text;
  }
  while (*text && isspace((unsigned char)*text)) {
    text++;
  }
  if (*text == '\0') {
    return text;
  }
  char *end = text + strlen(text) - 1;
  while (end > text && isspace((unsigned char)*end)) {
    *end-- = '\0';
  }
  return text;
}

void input_profiles_init(InputProfiles *profiles) {
  if (!profiles) {
    return;
  }
  memset(profiles, 0, sizeof(*profiles));
}

void input_profiles_capture(InputProfiles *profiles, const char *name,
                            const InputState *input) {
  if (!profiles || !input) {
    return;
  }
  InputProfile *profile = input_profiles_add(profiles, name);
  if (!profile) {
    return;
  }
  for (u32 i = 0; i < INPUT_ACTION_COUNT; i++) {
    profile->key_bindings[i] = input->key_bindings[i];
    profile->mouse_bindings[i] = input->mouse_binding[i];
  }
  if (profiles->active_profile[0] == '\0') {
    input_profiles_set_active(profiles, profile->name);
  }
}

bool input_profiles_set_active(InputProfiles *profiles, const char *name) {
  if (!profiles || !name) {
    return false;
  }
  InputProfile *profile = input_profiles_find(profiles, name);
  if (!profile) {
    return false;
  }
  snprintf(profiles->active_profile, sizeof(profiles->active_profile), "%s",
           profile->name);
  return true;
}

void input_profiles_apply_active(InputState *input,
                                 const InputProfiles *profiles) {
  if (!input || !profiles || profiles->active_profile[0] == '\0') {
    return;
  }
  for (u32 i = 0; i < profiles->count; i++) {
    if (strcmp(profiles->profiles[i].name, profiles->active_profile) == 0) {
      for (u32 action = 0; action < INPUT_ACTION_COUNT; action++) {
        input->key_bindings[action] = profiles->profiles[i].key_bindings[action];
        input->mouse_binding[action] =
            profiles->profiles[i].mouse_bindings[action];
      }
      return;
    }
  }
}

bool input_profiles_save(const InputProfiles *profiles, const char *filename) {
  if (!profiles || !filename) {
    return false;
  }
  FILE *file = fopen(filename, "w");
  if (!file) {
    return false;
  }

  fprintf(file, "# Input Profiles\n");
  if (profiles->active_profile[0] != '\0') {
    fprintf(file, "active_profile=%s\n\n", profiles->active_profile);
  }

  for (u32 i = 0; i < profiles->count; i++) {
    const InputProfile *profile = &profiles->profiles[i];
    for (u32 action = 0; action < INPUT_ACTION_COUNT; action++) {
      fprintf(file, "profile.%s.key.%s=%u\n", profile->name,
              input_action_name((InputAction)action),
              profile->key_bindings[action]);
      if (profile->mouse_bindings[action] == UINT32_MAX) {
        fprintf(file, "profile.%s.mouse.%s=-1\n", profile->name,
                input_action_name((InputAction)action));
      } else {
        fprintf(file, "profile.%s.mouse.%s=%u\n", profile->name,
                input_action_name((InputAction)action),
                profile->mouse_bindings[action]);
      }
    }
    fprintf(file, "\n");
  }

  fclose(file);
  return true;
}

bool input_profiles_load(InputProfiles *profiles, const char *filename) {
  if (!profiles || !filename) {
    return false;
  }
  FILE *file = fopen(filename, "r");
  if (!file) {
    return false;
  }

  input_profiles_init(profiles);
  char line[512];
  while (fgets(line, sizeof(line), file)) {
    char *trimmed = input_trim_whitespace(line);
    if (trimmed[0] == '\0' || trimmed[0] == '#' || trimmed[0] == ';') {
      continue;
    }

    char *equals = strchr(trimmed, '=');
    if (!equals) {
      continue;
    }
    *equals = '\0';
    char *key = input_trim_whitespace(trimmed);
    char *value = input_trim_whitespace(equals + 1);

    if (strcmp(key, "active_profile") == 0) {
      input_profiles_set_active(profiles, value);
      if (profiles->active_profile[0] == '\0') {
        snprintf(profiles->active_profile, sizeof(profiles->active_profile),
                 "%s", value);
      }
      continue;
    }

    if (strncmp(key, "profile.", 8) != 0) {
      continue;
    }

    char *profile_name = key + 8;
    char *first_dot = strchr(profile_name, '.');
    if (!first_dot) {
      continue;
    }
    *first_dot = '\0';
    char *section = first_dot + 1;
    char *second_dot = strchr(section, '.');
    if (!second_dot) {
      continue;
    }
    *second_dot = '\0';
    char *action_name = second_dot + 1;

    InputAction action = INPUT_ACTION_COUNT;
    if (!input_action_from_name(action_name, &action)) {
      continue;
    }

    InputProfile *profile = input_profiles_add(profiles, profile_name);
    if (!profile) {
      continue;
    }

    i32 parsed_value = atoi(value);
    if (strcmp(section, "key") == 0) {
      if (parsed_value < 0 || (u32)parsed_value >= INPUT_KEY_COUNT) {
        profile->key_bindings[action] = 0;
      } else {
        profile->key_bindings[action] = (u32)parsed_value;
      }
    } else if (strcmp(section, "mouse") == 0) {
      if (parsed_value < 0 || parsed_value >= 8) {
        profile->mouse_bindings[action] = UINT32_MAX;
      } else {
        profile->mouse_bindings[action] = (u32)parsed_value;
      }
    }
  }

  fclose(file);

  if (profiles->count > 0) {
    bool active_valid = false;
    for (u32 i = 0; i < profiles->count; i++) {
      if (strcmp(profiles->profiles[i].name, profiles->active_profile) == 0) {
        active_valid = true;
        break;
      }
    }
    if (!active_valid) {
      snprintf(profiles->active_profile, sizeof(profiles->active_profile), "%s",
               profiles->profiles[0].name);
    }
  }

  return true;
}

void controls_set_defaults(GameConfig *settings) {
  config_set_defaults(settings);

  // subtitles).
  // controller).
}

void controls_load(GameConfig *settings, const char *filename) {
  config_load(settings, filename);

}

void controls_save(GameConfig *settings, const char *filename) {
  config_save(settings, filename);
  // internal error tracking is added.

  LOG_INFO("Controls configuration saved successfully to %s", filename);
}

Vec3 input_calculate_movement(InputState *input, GameConfig *settings,
                              Vec3 forward, Vec3 right, f32 delta_time) {
  Vec3 movement = vec3_zero();

  if (input_is_action_held(input, INPUT_ACTION_MOVE_FORWARD)) {
    movement = vec3_add(movement, forward);
  }
  if (input_is_action_held(input, INPUT_ACTION_MOVE_BACKWARD)) {
    movement = vec3_sub(movement, forward);
  }
  if (input_is_action_held(input, INPUT_ACTION_MOVE_LEFT)) {
    movement = vec3_sub(movement, right);
  }
  if (input_is_action_held(input, INPUT_ACTION_MOVE_RIGHT)) {
    movement = vec3_add(movement, right);
  }

  // Normalize and apply speed
  f32 speed = settings->movement_speed;
  if (input_is_action_held(input, INPUT_ACTION_SPRINT)) {
    speed *= settings->sprint_multiplier;
  }
  if (input_is_action_held(input, INPUT_ACTION_CROUCH)) {
    speed *= settings->crouch_multiplier;
  }

  movement = vec3_normalize(movement);
  f32 analog = 1.0f;
  f32 magnitude = vec2_length(input->left_stick);
  if (magnitude > settings->controller_deadzone) {
    analog = CLAMP(magnitude, 0.0f, 1.0f);
  }
  movement = vec3_mul(movement, speed * analog * delta_time);

  return movement;
}
