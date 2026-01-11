#ifndef HUD_H
#define HUD_H

#include "../../../../engine/include/math/vec2.h"
#include "../../../../engine/include/math/vec3.h"
#include "../../../../engine/include/math/vec4.h"
#include "../game_common.h"

// Forward declarations
struct VulkanRenderer;
struct PlayerSystem;
struct InventorySystem;
struct WorldSystem;
struct WeatherSystem;

// --- Enumerations ---

typedef enum {
  HUD_ELEMENT_HEALTH_BAR,
  HUD_ELEMENT_HUNGER_BAR,
  HUD_ELEMENT_ARMOR_BAR,
  HUD_ELEMENT_STAMINA_BAR,
  HUD_ELEMENT_OXYGEN_BAR,
  HUD_ELEMENT_EXPERIENCE_BAR,
  HUD_ELEMENT_HOTBAR,
  HUD_ELEMENT_CROSSHAIR,
  HUD_ELEMENT_MINIMAP,
  HUD_ELEMENT_STATUS_EFFECTS,
  HUD_ELEMENT_COMPASS,
  HUD_ELEMENT_CLOCK,
  HUD_ELEMENT_COORDINATES,
  HUD_ELEMENT_BIOME_INFO,
  HUD_ELEMENT_WEATHER_STATUS,
  HUD_ELEMENT_COMBAT_LOG,
  HUD_ELEMENT_ITEM_TOOLTIP,
  HUD_ELEMENT_DEBUG_INFO,
  HUD_ELEMENT_NOTIFICATIONS,
  HUD_ELEMENT_COUNT
} HUDElementType;

typedef enum {
  CROSSHAIR_STYLE_DEFAULT,
  CROSSHAIR_STYLE_CIRCLE,
  CROSSHAIR_STYLE_DOT,
  CROSSHAIR_STYLE_CROSS,
  CROSSHAIR_STYLE_DIAMOND,
  CROSSHAIR_STYLE_TRIANGLE,
  CROSSHAIR_STYLE_CUSTOM
} CrosshairStyle;

typedef enum {
  HUD_THEME_DEFAULT,
  HUD_THEME_DARK,
  HUD_THEME_HIGH_CONTRAST
} HUDTheme;

typedef enum {
  HUD_NOTIFICATION_INFO,
  HUD_NOTIFICATION_SUCCESS,
  HUD_NOTIFICATION_WARNING,
  HUD_NOTIFICATION_ERROR,
  HUD_NOTIFICATION_ACHIEVEMENT
} HUDNotificationType;

// --- Sub-Structures ---

typedef struct {
  Vec2 position;
  Vec2 size;
  bool visible;
  f32 opacity;
} HUDElementBase;

typedef struct {
  f32 current;
  f32 max;
  f32 target;
  f32 absorption;
  f32 animation_speed;
  Vec4 color;
  Vec4 absorption_color;
} HealthBarConfig;

typedef struct {
  f32 current;
  f32 max;
  f32 target;
  f32 saturation;
  f32 exhaustion;
  Vec4 color;
} HungerBarConfig;

typedef struct {
  u32 level;
  f32 current_exp;
  f32 to_next;
  f32 target_exp;
  f32 animation_speed;
  Vec4 color;
} ExperienceBarConfig;

typedef struct {
  u32 selected_slot;
  u32 slot_count;
  bool show_names;
  bool show_durability;
  bool show_enchantments;
  Vec4 selected_color;
} HUDHotbarConfig;

typedef struct {
  CrosshairStyle style;
  Vec4 color;
  f32 size;
  f32 thickness;
  f32 hit_time;
  f32 crit_time;
} CrosshairConfig;

typedef struct {
  bool enabled;
  f32 zoom;
  bool rotation_follows_player;
  bool show_entities;
  bool show_waypoints;
} MinimapConfig;

typedef struct {
  char message[256];
  Vec4 color;
  f32 timer;
  f32 duration;
  f32 opacity;
  bool active;
  HUDNotificationType type;
} HUDNotification;

typedef struct {
  u64 render_calls;
  u64 update_calls;
  f32 avg_render_time;
  f32 avg_update_time;
  u64 damage_flashes;
} HUDStatistics;

// --- Main HUD System ---

typedef struct {
  // Universal state
  Vec2 screen_size;
  bool is_valid;
  char validation_errors[512];

  // Elements data
  HealthBarConfig health_bar;
  HungerBarConfig hunger_bar;
  HealthBarConfig stamina_bar; // Reusing HealthBarConfig for simplicity
  HealthBarConfig armor_bar;
  HealthBarConfig oxygen_bar;
  ExperienceBarConfig experience_bar;
  HUDHotbarConfig hotbar;
  CrosshairConfig crosshair;
  MinimapConfig minimap;

  // Visibility/Value array (Legacy support)
  bool elements_visible[HUD_ELEMENT_COUNT];
  f32 element_values[HUD_ELEMENT_COUNT];

  // Logging systems
  HUDNotification notifications[5];
  u32 notification_count;

  // Diagnostics & Performance
  HUDStatistics stats;
  u32 last_fps;
  f32 fps_timer;
  u32 frame_count;

  // Context
  u32 weather_type;
  f32 weather_intensity;
  bool is_underwater;

} HUDSystem;

// --- Public API ---

void hud_init(HUDSystem *hud, Vec2 screen_size);
void hud_update(HUDSystem *hud, struct PlayerSystem *player, f32 delta_time);
void hud_render(HUDSystem *hud, struct VulkanRenderer *renderer);
void hud_resize(HUDSystem *hud, Vec2 new_size);

// Status triggers
void hud_trigger_damage_flash(HUDSystem *hud);
void hud_trigger_crosshair_hit(HUDSystem *hud, bool is_critical);
void hud_add_notification(HUDSystem *hud, const char *message, f32 duration,
                          HUDNotificationType type);

// Configuration
void hud_set_element_visible(HUDSystem *hud, HUDElementType type, bool visible);
void hud_toggle_debug(HUDSystem *hud);

#endif // HUD_H
