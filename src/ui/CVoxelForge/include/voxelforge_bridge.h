#ifndef VOXELFORGE_SWIFT_BRIDGE_H
#define VOXELFORGE_SWIFT_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Engine Lifecycle - Minimal Bridge API
// ============================================================================

typedef struct Engine Engine;
typedef struct EngineConfig EngineConfig;

bool bridge_init_engine(Engine *engine, const EngineConfig *config);
void bridge_shutdown_engine(Engine *engine);
void bridge_update(float delta_time);

// ============================================================================
// Render Stats
// ============================================================================

typedef struct {
    float fps;
    float frame_time_ms;
    uint32_t draw_calls;
    uint32_t triangles;
    uint32_t vertices;
    uint64_t memory_used;
} BridgeRenderStats;

void bridge_get_render_stats(BridgeRenderStats *stats);

// ============================================================================
// Scene Management
// ============================================================================

void bridge_create_scene(const char *name);
void bridge_load_scene(const char *path);
void bridge_save_scene(const char *path);

// ============================================================================
// Entity Management
// ============================================================================

uint64_t bridge_create_entity(void);
void bridge_destroy_entity(uint64_t entity_id);

// ============================================================================
// Physics Interface
// ============================================================================

typedef struct {
    float x, y, z;
} BridgeVec3;

typedef struct {
    BridgeVec3 position;
    BridgeVec3 velocity;
    float mass;
} BridgePhysicsData;

void bridge_get_physics(uint64_t entity_id, BridgePhysicsData *data);

// ============================================================================
// Material System
// ============================================================================

void bridge_create_material(const char *name);
void bridge_set_material_property(const char *property, float value);

// ============================================================================
// Asset Management
// ============================================================================

void bridge_load_asset(const char *path);
void bridge_unload_asset(const char *path);

// ============================================================================
// Debug Interface
// ============================================================================

void bridge_enable_debug_mode(bool enabled);
void bridge_toggle_wireframe(void);

// ============================================================================
// Hot Reload
// ============================================================================

void bridge_enable_hot_reload(bool enabled);
void bridge_reload_shaders(void);

// ============================================================================
// Logging
// ============================================================================

void bridge_log(int32_t level, const char *source, const char *message);

// ============================================================================
// Save Game System
// ============================================================================

typedef struct {
    char name[64];
    uint32_t seed;
    uint64_t last_played;
    uint64_t play_time;
    uint32_t chunk_count;
    float spawn_x, spawn_y, spawn_z;
    char version[32];
} BridgeWorldMetadata;

bool bridge_get_world_list(BridgeWorldMetadata **worlds, uint32_t *count);
bool bridge_load_world(const char *world_name);
bool bridge_save_world(const char *world_name);
bool bridge_delete_world(const char *world_name);
bool bridge_world_exists(const char *world_name);

// ============================================================================
// Player Data System
// ============================================================================

typedef struct {
    float health;
    float max_health;
    float hunger;
    float max_hunger;
    float armor;
    float max_armor;
    float experience;
    uint32_t level;
    float position_x, position_y, position_z;
    uint32_t hotbar_selected_slot;
    bool is_sprinting;
    bool is_crouching;
    float stamina;
    float oxygen;
} BridgePlayerData;

bool bridge_get_player_data(BridgePlayerData *player_data);
void bridge_get_hotbar_items(uint32_t slot_index, char *item_name, uint32_t name_size, uint32_t *count, uint32_t *max_stack);

// ============================================================================
// Multiplayer/Networking System
// ============================================================================

typedef struct {
    char name[64];
    char address[256];
    uint32_t port;
    uint32_t player_count;
    uint32_t max_players;
    char version[32];
    uint32_t ping;
    char description[256];
    bool is_online;
    bool has_password;
} BridgeServerInfo;

bool bridge_connect_to_server(const char *address, uint32_t port);
bool bridge_create_server(const char *server_name, uint32_t max_players, const char *password);
bool bridge_get_server_list(BridgeServerInfo **servers, uint32_t *count);
bool bridge_disconnect_from_server(void);
bool bridge_is_connected_to_server(void);

// ============================================================================
// External Controller Support
// ============================================================================

typedef struct {
    bool is_connected;
    char name[64];
    uint32_t vendor_id;
    uint32_t product_id;
    float left_stick_x, left_stick_y;
    float right_stick_x, right_stick_y;
    float left_trigger, right_trigger;
    bool button_a, button_b, button_x, button_y;
    bool button_l1, button_r1, button_l2, button_r2;
    bool dpad_up, dpad_down, dpad_left, dpad_right;
    bool start_button, select_button, home_button;
} BridgeControllerState;

bool bridge_init_controllers(void);
void bridge_update_controllers(void);
bool bridge_get_controller_state(uint32_t controller_index, BridgeControllerState *state);
uint32_t bridge_get_connected_controller_count(void);
void bridge_set_controller_vibration(uint32_t controller_index, float left_motor, float right_motor, uint32_t duration_ms);

// ============================================================================
// Enchanting System
// ============================================================================

typedef struct {
    uint32_t enchantment_type;
    uint32_t level;
    uint32_t cost_levels;
    uint32_t cost_lapis;
    bool is_available;
} BridgeEnchantmentOffer;

bool bridge_enchanting_scan_bookshelves(void);
uint32_t bridge_enchanting_get_power(void);
bool bridge_enchanting_get_offers(BridgeEnchantmentOffer *offers, uint32_t offer_count);
bool bridge_enchanting_apply_enchantment(uint32_t offer_index);
bool bridge_enchanting_can_apply_to_item(uint32_t item_type, uint32_t enchantment_type);
bool bridge_enchanting_has_conflict(uint32_t item_type, uint32_t enchantment_type);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_SWIFT_BRIDGE_H
