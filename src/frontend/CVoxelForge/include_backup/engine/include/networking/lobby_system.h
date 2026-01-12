#ifndef LOBBY_SYSTEM_H
#define LOBBY_SYSTEM_H

#include "../core/core.h"
#include "../network/network_types.h"

#define MAX_LOBBY_NAME_LENGTH 64
#define MAX_PLAYER_NAME_LENGTH 32
#define MAX_LOBBY_PLAYERS 16
#define MAX_CHAT_MESSAGE_LENGTH 256
#define MAX_LOBBY_DESCRIPTION_LENGTH 128

// Lobby types
typedef enum {
    LOBBY_TYPE_PUBLIC = 0,
    LOBBY_TYPE_PRIVATE,
    LOBBY_TYPE_FRIENDS_ONLY
} LobbyType;

// Game modes
typedef enum {
    GAME_MODE_SURVIVAL = 0,
    GAME_MODE_CREATIVE,
    GAME_MODE_ADVENTURE,
    GAME_MODE_SPECTATOR
} GameMode;

// Lobby states
typedef enum {
    LOBBY_STATE_WAITING = 0,
    LOBBY_STATE_STARTING,
    LOBBY_STATE_IN_PROGRESS,
    LOBBY_STATE_FINISHED
} LobbyState;

// Player states
typedef enum {
    PLAYER_STATE_DISCONNECTED = 0,
    PLAYER_STATE_CONNECTED,
    PLAYER_STATE_READY,
    PLAYER_STATE_IN_GAME
} PlayerState;

// Player information
typedef struct {
    uint32_t id;
    char name[MAX_PLAYER_NAME_LENGTH];
    NetAddress address;
    PlayerState state;
    bool is_host;
    uint32_t ping_ms;
    float ready_time;
} LobbyPlayer;

// Lobby information
typedef struct {
    uint32_t id;
    char name[MAX_LOBBY_NAME_LENGTH];
    char description[MAX_LOBBY_DESCRIPTION_LENGTH];
    LobbyType type;
    GameMode game_mode;
    LobbyState state;
    char map_name[64];
    uint32_t max_players;
    uint32_t current_players;
    LobbyPlayer players[MAX_LOBBY_PLAYERS];
    uint32_t host_id;
    uint64_t created_time;
    uint64_t last_activity;
    bool has_password;
    char password[32];
    uint32_t region_code;
    float average_ping;
    bool version_compatible;
} Lobby;

// Lobby search filters
typedef struct {
    LobbyType type_filter;
    GameMode game_mode_filter;
    uint32_t region_filter;
    bool hide_full;
    bool hide_in_progress;
    bool friends_only;
    float max_ping;
    char map_filter[64];
} LobbySearchFilters;

// Lobby events
typedef enum {
    LOBBY_EVENT_PLAYER_JOINED = 0,
    LOBBY_EVENT_PLAYER_LEFT,
    LOBBY_EVENT_PLAYER_READY,
    LOBBY_EVENT_PLAYER_NOT_READY,
    LOBBY_EVENT_HOST_CHANGED,
    LOBBY_EVENT_SETTINGS_CHANGED,
    LOBBY_EVENT_GAME_STARTED,
    LOBBY_EVENT_CHAT_MESSAGE,
    LOBBY_EVENT_COUNT
} LobbyEventType;

// Lobby event data
typedef struct {
    LobbyEventType type;
    uint32_t player_id;
    uint32_t lobby_id;
    char message[MAX_CHAT_MESSAGE_LENGTH];
    uint64_t timestamp;
} LobbyEvent;

// Lobby system interface
typedef struct LobbySystem {
    // Lifecycle
    bool (*init)(struct LobbySystem* lobby);
    void (*shutdown)(struct LobbySystem* lobby);
    void (*update)(struct LobbySystem* lobby, float delta_time);
    
    // Lobby management
    uint32_t (*create_lobby)(struct LobbySystem* lobby, const char* name, const char* description, 
                           LobbyType type, GameMode game_mode, const char* map_name, 
                           uint32_t max_players, const char* password);
    bool (*join_lobby)(struct LobbySystem* lobby, uint32_t lobby_id, const char* password);
    bool (*leave_lobby)(struct LobbySystem* lobby);
    bool (*kick_player)(struct LobbySystem* lobby, uint32_t player_id);
    bool (*promote_to_host)(struct LobbySystem* lobby, uint32_t player_id);
    
    // Lobby discovery
    uint32_t (*find_lobbies)(struct LobbySystem* lobby, const LobbySearchFilters* filters, 
                           Lobby* results, uint32_t max_results);
    bool (*refresh_lobby_list)(struct LobbySystem* lobby);
    Lobby* (*get_current_lobby)(struct LobbySystem* lobby);
    
    // Player management
    bool (*set_ready_status)(struct LobbySystem* lobby, bool ready);
    bool (*send_chat_message)(struct LobbySystem* lobby, const char* message);
    bool (*update_player_info)(struct LobbySystem* lobby, const char* name);
    
    // Host functions
    bool (*start_game)(struct LobbySystem* lobby);
    bool (*set_lobby_settings)(struct LobbySystem* lobby, const char* map_name, 
                              GameMode game_mode, uint32_t max_players);
    bool (*migrate_host)(struct LobbySystem* lobby, uint32_t new_host_id);
    
    // Event handling
    bool (*poll_event)(struct LobbySystem* lobby, LobbyEvent* event);
    void (*set_event_handler)(struct LobbySystem* lobby, LobbyEventType type, 
                            void (*handler)(const LobbyEvent*, void*), void* user_data);
    
    // Statistics
    uint32_t (*get_player_count)(struct LobbySystem* lobby);
    uint32_t (*get_lobby_count)(struct LobbySystem* lobby);
    float (*get_average_ping)(struct LobbySystem* lobby);
    
    // Internal state
    Lobby* current_lobby;
    LobbyPlayer* local_player;
    LobbyEvent event_queue[32];
    uint32_t event_queue_head;
    uint32_t event_queue_tail;
    bool is_host;
    bool is_connected;
    uint64_t last_update;
} LobbySystem;

// Factory functions
LobbySystem* lobby_create_system(void);
void lobby_destroy_system(LobbySystem* lobby);

// Utility functions
const char* lobby_type_to_string(LobbyType type);
const char* game_mode_to_string(GameMode mode);
const char* lobby_state_to_string(LobbyState state);
const char* player_state_to_string(PlayerState state);

bool lobby_validate_name(const char* name);
bool lobby_validate_password(const char* password);
bool lobby_is_compatible_version(const char* client_version, const char* lobby_version);

#endif // LOBBY_SYSTEM_H
