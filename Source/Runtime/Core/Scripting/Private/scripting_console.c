#include "scripting_types.h"
#include "unified_logger.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * CONSOLE SYSTEM IMPLEMENTATION
 * ============================================================================
 */

void console_init(ScriptContext *context) {
  if (!context)
    return;

  memset(&context->console, 0, sizeof(ConsoleState));
  context->console.is_visible = false;

  LOG_INFO(LOG_CAT_GENERAL, "Developer Console initialized");
}

void console_shutdown(ScriptContext *context) {
  if (!context)
    return;
  // Cleanup if needed
}

void console_update(ScriptContext *context) {
  if (!context)
    return;
  // Logic to handle console state updates
}

void console_render(ScriptContext *context) {
  if (!context || !context->console.is_visible)
    return;

  // ImGui rendering would go here
  // This is a stub for now since ImGui integration is optional
#ifdef ENABLE_IMGUI
  // ImGui::Begin("Console", &context->console.is_visible);
  // ... render logic ...
  // ImGui::End();
#endif
}

void console_print(ScriptContext *context, const char *text) {
  if (!context || !text)
    return;

  ConsoleState *console = &context->console;

  if (console->log_count < MAX_CONSOLE_HISTORY) {
    strncpy(console->history[console->log_count].text, text,
            MAX_CONSOLE_BUFFER - 1);
    console->history[console->log_count].is_error = false;
    console->log_count++;
  } else {
    // Shift history
    for (uint32_t i = 0; i < MAX_CONSOLE_HISTORY - 1; i++) {
      console->history[i] = console->history[i + 1];
    }
    strncpy(console->history[MAX_CONSOLE_HISTORY - 1].text, text,
            MAX_CONSOLE_BUFFER - 1);
    console->history[MAX_CONSOLE_HISTORY - 1].is_error = false;
  }

  LOG_INFO(LOG_CAT_GENERAL, "[Console] %s", text);
}

void console_print_error(ScriptContext *context, const char *text) {
  if (!context || !text)
    return;

  ConsoleState *console = &context->console;

  if (console->log_count < MAX_CONSOLE_HISTORY) {
    strncpy(console->history[console->log_count].text, text,
            MAX_CONSOLE_BUFFER - 1);
    console->history[console->log_count].is_error = true;
    console->log_count++;
  } else {
    // Shift history
    for (uint32_t i = 0; i < MAX_CONSOLE_HISTORY - 1; i++) {
      console->history[i] = console->history[i + 1];
    }
    strncpy(console->history[MAX_CONSOLE_HISTORY - 1].text, text,
            MAX_CONSOLE_BUFFER - 1);
    console->history[MAX_CONSOLE_HISTORY - 1].is_error = true;
  }

  LOG_ERROR(LOG_CAT_GENERAL, "[Console] %s", text);
}

void console_clear(ScriptContext *context) {
  if (!context)
    return;

  memset(context->console.history, 0, sizeof(context->console.history));
  context->console.log_count = 0;
}

bool console_is_visible(ScriptContext *context) {
  if (!context)
    return false;
  return context->console.is_visible;
}

void console_toggle(ScriptContext *context) {
  if (!context)
    return;
  context->console.is_visible = !context->console.is_visible;
}
