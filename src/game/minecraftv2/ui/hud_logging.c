#include "../include/ui/hud.h"
#include <string.h>

void hud_add_notification(HUDSystem *hud, const char *message, f32 duration,
                          HUDNotificationType type) {
  if (!hud || !message)
    return;

  u32 slot = hud->notification_count;
  if (slot >= 5) {
    // Shift existing
    for (u32 i = 0; i < 4; i++) {
      hud->notifications[i] = hud->notifications[i + 1];
    }
    slot = 4;
  } else {
    hud->notification_count++;
  }

  HUDNotification *n = &hud->notifications[slot];
  strncpy(n->message, message, 255);
  n->message[255] = '\0';
  n->duration = duration;
  n->timer = duration;
  n->type = type;
  n->opacity = 0.0f;
  n->active = true;
}

void hud_update_logging(HUDSystem *hud, f32 delta_time) {
  if (!hud)
    return;

  for (u32 i = 0; i < hud->notification_count; i++) {
    HUDNotification *n = &hud->notifications[i];
    if (n->active) {
      n->timer -= delta_time;

      // Fade logic
      if (n->timer > n->duration - 0.5f) {
        n->opacity = (n->duration - n->timer) / 0.5f;
      } else if (n->timer < 0.5f) {
        n->opacity = n->timer / 0.5f;
      } else {
        n->opacity = 1.0f;
      }

      if (n->timer <= 0.0f) {
        n->active = false;
        // Shift remaining
        for (u32 j = i; j < hud->notification_count - 1; j++) {
          hud->notifications[j] = hud->notifications[j + 1];
        }
        hud->notification_count--;
        i--;
      }
    }
  }
}
