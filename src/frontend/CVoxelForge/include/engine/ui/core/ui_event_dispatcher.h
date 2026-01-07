#ifndef UI_EVENT_DISPATCHER_H
#define UI_EVENT_DISPATCHER_H

#include <stdbool.h>
#include <stdint.h>

// UI Event Dispatcher - Placeholder header
typedef struct UIEventDispatcher {
    uint32_t id;
    bool initialized;
} UIEventDispatcher;

void ui_event_dispatcher_init(UIEventDispatcher *dispatcher);
void ui_event_dispatcher_shutdown(UIEventDispatcher *dispatcher);

#endif // UI_EVENT_DISPATCHER_H
