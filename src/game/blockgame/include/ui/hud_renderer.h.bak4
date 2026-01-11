#ifndef HUD_RENDERER_H
#define HUD_RENDERER_H

#include "../game_common.h"
#include "hud.h"
#include <math/vec2.h>

// Forward declarations
struct VulkanRenderer;

// Initialize HUD renderer with screen dimensions
void hud_renderer_init(Vec2 screen_size);

// Free HUD renderer resources
void hud_renderer_free(void);

// Render the HUD using the provided HUD system state
void hud_render_impl(HUDSystem *hud, struct VulkanRenderer *renderer);

#endif // HUD_RENDERER_H
