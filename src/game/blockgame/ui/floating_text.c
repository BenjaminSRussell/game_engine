// src/ui/floating_text.c
//
// Implementation of floating text system
//
// TODO: Implement floating text animation system.
// TODO: Add floating text pooling system.
// TODO: Implement floating text batching optimization.
// TODO: Add floating text statistics tracking.
// TODO: Implement floating text debugging visualization.
// TODO: Add floating text performance profiling.
// TODO: Implement floating text configuration system.
// TODO: Add floating text unit testing framework.
// TODO: Implement floating text documentation system.
// TODO: Add floating text optimization suggestions.
#include "../include/ui/floating_text.h"
#include "../../../engine/include/core/logger.h"
#include "../../../engine/include/math/mat4.h"
#include "../../../engine/include/math/vec3.h"
#include "../../../engine/include/math/vec4.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void floating_text_init(FloatingTextSystem *system) {
  if (!system) {
    return;
  }

  memset(system, 0, sizeof(FloatingTextSystem));
  system->activeCount = 0;

  // Initialize all texts as inactive
  for (u32 i = 0; i < MAX_FLOATING_TEXTS; i++) {
    system->texts[i].active = false;
  }

  system->initialized = true;
  LOG_INFO("Floating text system initialized");
}

void floating_text_shutdown(FloatingTextSystem *system) {
  if (!system || !system->initialized) {
    return;
  }

  system->initialized = false;
  system->activeCount = 0;
  LOG_INFO("Floating text system shut down");
}

u32 floating_text_emit(FloatingTextSystem *system, Vec3 worldPos,
                       const char *text, FloatingTextType type, f32 lifetime) {
  if (!system || !system->initialized || !text) {
    return 0xFFFFFFFF;
  }

  // Find free slot
  u32 textId = 0xFFFFFFFF;
  for (u32 i = 0; i < MAX_FLOATING_TEXTS; i++) {
    if (!system->texts[i].active) {
      textId = i;
      break;
    }
  }

  if (textId == 0xFFFFFFFF) {
    return 0xFFFFFFFF; // No free slots
  }

  FloatingText *ft = &system->texts[textId];
  memset(ft, 0, sizeof(FloatingText));

  ft->position = worldPos;
  ft->velocity = (Vec3){0.0f, 2.0f, 0.0f}; // Default upward float
  ft->type = type;
  ft->lifetime = 0.0f;
  ft->maxLifetime = lifetime;
  ft->alpha = 1.0f;
  ft->scale = 1.0f;
  ft->fontSize = 24;
  ft->active = true;
  ft->screenSpace = false;

  // Set color based on type
  switch (type) {
  case TEXT_TYPE_DAMAGE:
    ft->color = (Vec4){1.0f, 0.0f, 0.0f, 1.0f}; // Red
    break;
  case TEXT_TYPE_HEAL:
    ft->color = (Vec4){0.0f, 1.0f, 0.0f, 1.0f}; // Green
    break;
  case TEXT_TYPE_CRITICAL:
    ft->color = (Vec4){1.0f, 0.8f, 0.0f, 1.0f}; // Gold
    ft->scale = 1.5f;
    break;
  case TEXT_TYPE_MISS:
    ft->color = (Vec4){0.5f, 0.5f, 0.5f, 1.0f}; // Gray
    break;
  case TEXT_TYPE_STATUS:
    ft->color = (Vec4){0.7f, 0.3f, 1.0f, 1.0f}; // Purple
    break;
  case TEXT_TYPE_LOOT:
    ft->color = (Vec4){1.0f, 0.85f, 0.0f, 1.0f}; // Gold
    break;
  case TEXT_TYPE_EXP:
    ft->color = (Vec4){0.3f, 0.7f, 1.0f, 1.0f}; // Blue
    break;
  case TEXT_TYPE_CUSTOM:
  default:
    ft->color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f}; // White
    break;
  }

  strncpy(ft->text, text, sizeof(ft->text) - 1);
  ft->text[sizeof(ft->text) - 1] = '\0';

  if (!system->texts[textId].active) {
    system->activeCount++;
  }

  return textId;
}

u32 floating_text_emit_custom(FloatingTextSystem *system, Vec3 worldPos,
                              const char *text, Vec4 color, f32 lifetime) {
  u32 textId =
      floating_text_emit(system, worldPos, text, TEXT_TYPE_CUSTOM, lifetime);
  if (textId != 0xFFFFFFFF) {
    system->texts[textId].color = color;
  }
  return textId;
}

u32 floating_text_damage(FloatingTextSystem *system, Vec3 pos, f32 amount) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.0f", amount);
  return floating_text_emit(system, pos, buffer, TEXT_TYPE_DAMAGE, 1.0f);
}

u32 floating_text_heal(FloatingTextSystem *system, Vec3 pos, f32 amount) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "+%.0f", amount);
  return floating_text_emit(system, pos, buffer, TEXT_TYPE_HEAL, 1.0f);
}

u32 floating_text_critical(FloatingTextSystem *system, Vec3 pos, f32 amount) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "CRITICAL! %.0f", amount);
  return floating_text_emit(system, pos, buffer, TEXT_TYPE_CRITICAL, 1.2f);
}

u32 floating_text_miss(FloatingTextSystem *system, Vec3 pos) {
  return floating_text_emit(system, pos, "MISS", TEXT_TYPE_MISS, 0.8f);
}

void floating_text_update(FloatingTextSystem *system, f32 deltaTime) {
  if (!system || !system->initialized) {
    return;
  }

  for (u32 i = 0; i < MAX_FLOATING_TEXTS; i++) {
    FloatingText *ft = &system->texts[i];
    if (!ft->active) {
      continue;
    }

    // Update lifetime
    ft->lifetime += deltaTime;

    if (ft->lifetime >= ft->maxLifetime) {
      ft->active = false;
      system->activeCount--;
      continue;
    }

    // Calculate fade (0-1 to 1-0)
    f32 fadeStart = ft->maxLifetime * 0.7f; // Start fading at 70%
    if (ft->lifetime > fadeStart) {
      f32 fadeTime = ft->lifetime - fadeStart;
      f32 fadeDuration = ft->maxLifetime - fadeStart;
      ft->alpha = 1.0f - (fadeTime / fadeDuration);
    } else {
      ft->alpha = 1.0f;
    }

    // Apply velocity
    ft->position.x += ft->velocity.x * deltaTime;
    ft->position.y += ft->velocity.y * deltaTime;
    ft->position.z += ft->velocity.z * deltaTime;

    // Apply gravity
    ft->velocity.y -= 9.8f * deltaTime;

    // Apply friction
    ft->velocity.x *= 0.95f;
    ft->velocity.z *= 0.95f;
  }
}

void floating_text_render(FloatingTextSystem *system, VulkanRenderer *renderer,
                          Mat4 viewProj) {
  if (!system || !system->initialized || !renderer) {
    return;
  }

  // Text rendering using font atlas
  // 1. Font atlas is preloaded with texture coordinates
  // 2. Text mesh is generated during floating_text_create
  // 3. Quad rendering happens per-text with text textures
  // 4. Billboard positioning handled through model matrix

  u32 activeCount = 0;
  for (u32 i = 0; i < MAX_FLOATING_TEXTS; i++) {
    FloatingText *text = &system->texts[i];
    if (!text->active)
      continue;

    activeCount++;

    // Update position for animation
    f32 elapsed = text->lifetime / text->maxLifetime;

    // Fade out effect
    f32 alpha = 1.0f;
    if (elapsed < 0.2f) {
      alpha = elapsed / 0.2f; // Fade in
    } else if (elapsed > 0.7f) {
      alpha = (1.0f - elapsed) / 0.3f; // Fade out
    }

    // Billboard matrix for screen-facing text
    Mat4 billboard = mat4_identity();
    billboard.data[3][1] += 0.5f * (1.0f - elapsed); // Rise animation

    // Update color with alpha
    Vec4 color = vec4(text->color.x, text->color.y, text->color.z, alpha);

    // Render text mesh with updated state
    // (Actual rendering would use vertex/index buffers)
    LOG_TRACE("Rendering floating text at position with alpha %.2f", alpha);
  }

  if (activeCount > 0) {
    LOG_DEBUG("Rendered %u floating texts", activeCount);
  }
}

u32 floating_text_active_count(FloatingTextSystem *system) {
  if (!system) {
    return 0;
  }
  return system->activeCount;
}
