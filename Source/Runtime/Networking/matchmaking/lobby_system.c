#include "network/lobby_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// ============================================================================
// Internal State
// ============================================================================

#define MAX_CACHED_LOBBIES 64

typedef struct {
    void (*event_handler)(const LobbyEvent*, void*);
    void* user_data;
} EventHandlerEntry;

// Private struct extending LobbySystem if we needed hidden state,
// but LobbySystem has internal state exposed in the header.
// So we can just use LobbySystem directly.
// But we might need storage for the "found lobbies" cache if find_lobbies is called.

// For now, let's just use the fields in LobbySystem.
// Wait, LobbySystem has `event_queue`.

// ============================================================================
// Helper Functions
// ============================================================================

static void push_event(LobbySystem* system, LobbyEventType type, const char* msg, uint32_t player_id) {
    if (!system) return;

    // Check bounds
    uint32_t next = (system->event_queue_tail + 1) % 32;
    if (next == system->event_queue_head) {
        // Queue full, drop oldest? or drop newest?
        // Let's drop oldest.
        system->event_queue_head = (system->event_queue_head + 1) % 32;
    }

    LobbyEvent* event = &system->event_queue[system->event_queue_tail];
    event->type = type;
    event->player_id = player_id;
    event->lobby_id = system->current_lobby ? system->current_lobby->id : 0;
    event->timestamp = (uint64_t)time(NULL);
    if (msg) {
        strncpy(event->message, msg, MAX_CHAT_MESSAGE_LENGTH - 1);
        event->message[MAX_CHAT_MESSAGE_LENGTH - 1] = '\0';
    } else {
        event->message[0] = '\0';
    }

    system->event_queue_tail = next;
}

// ============================================================================
// Implementation
// ============================================================================

static bool lobby_init(LobbySystem* system) {
    if (!system) return false;
    memset(system->event_queue, 0, sizeof(system->event_queue));
    system->event_queue_head = 0;
    system->event_queue_tail = 0;
    system->current_lobby = NULL;
    system->local_player = (LobbyPlayer*)malloc(sizeof(LobbyPlayer));
    if (system->local_player) {
        memset(system->local_player, 0, sizeof(LobbyPlayer));
        system->local_player->state = PLAYER_STATE_DISCONNECTED;
    }
    system->is_host = false;
    system->is_connected = false;
    system->last_update = 0;
    return true;
}

static void lobby_shutdown(LobbySystem* system) {
    if (!system) return;
    if (system->current_lobby) {
        free(system->current_lobby);
        system->current_lobby = NULL;
    }
    if (system->local_player) {
        free(system->local_player);
        system->local_player = NULL;
    }
}

static void lobby_update(LobbySystem* system, float delta_time) {
    if (!system) return;
    // Process network packets if we were integrated.
    // For now, just update internal timers?
}

static uint32_t lobby_create(LobbySystem* system, const char* name, const char* description,
                           LobbyType type, GameMode game_mode, const char* map_name,
                           uint32_t max_players, const char* password) {
    if (!system) return 0;

    // Clear existing lobby
    if (system->current_lobby) {
        free(system->current_lobby);
    }

    system->current_lobby = (Lobby*)malloc(sizeof(Lobby));
    if (!system->current_lobby) return 0;

    Lobby* l = system->current_lobby;
    memset(l, 0, sizeof(Lobby));

    l->id = (uint32_t)time(NULL); // Random ID
    strncpy(l->name, name, MAX_LOBBY_NAME_LENGTH - 1);
    strncpy(l->description, description ? description : "", MAX_LOBBY_DESCRIPTION_LENGTH - 1);
    l->type = type;
    l->game_mode = game_mode;
    l->state = LOBBY_STATE_WAITING;
    strncpy(l->map_name, map_name ? map_name : "default_map", 63);
    l->max_players = max_players > MAX_LOBBY_PLAYERS ? MAX_LOBBY_PLAYERS : max_players;
    if (password && strlen(password) > 0) {
        strncpy(l->password, password, 31);
        l->has_password = true;
    }
    l->host_id = system->local_player ? system->local_player->id : 1;
    l->created_time = (uint64_t)time(NULL);
    l->version_compatible = true;

    // Add local player
    l->current_players = 1;
    if (system->local_player) {
        // Set local player ID if not set
        if (system->local_player->id == 0) system->local_player->id = l->host_id; // Simple ID assignment

        system->local_player->is_host = true;
        system->local_player->state = PLAYER_STATE_CONNECTED;
        l->players[0] = *system->local_player;
    }

    system->is_host = true;
    system->is_connected = true;

    push_event(system, LOBBY_EVENT_PLAYER_JOINED, "Lobby created", l->host_id);

    return l->id;
}

static bool lobby_join(LobbySystem* system, uint32_t lobby_id, const char* password) {
    if (!system) return false;

    // Mock join
    // In a real system, we would request connection to server/host

    if (system->current_lobby) {
        free(system->current_lobby);
    }

    system->current_lobby = (Lobby*)malloc(sizeof(Lobby));
    if (!system->current_lobby) return false;
    memset(system->current_lobby, 0, sizeof(Lobby));
    system->current_lobby->id = lobby_id;
    system->current_lobby->state = LOBBY_STATE_WAITING;
    system->current_lobby->max_players = MAX_LOBBY_PLAYERS;
    system->current_lobby->current_players = 2; // Mock

    system->is_host = false;
    system->is_connected = true;

    if (system->local_player) {
        system->local_player->is_host = false;
        system->local_player->state = PLAYER_STATE_CONNECTED;
    }

    push_event(system, LOBBY_EVENT_PLAYER_JOINED, "Joined lobby", system->local_player ? system->local_player->id : 0);

    return true;
}

static bool lobby_leave(LobbySystem* system) {
    if (!system || !system->current_lobby) return false;

    push_event(system, LOBBY_EVENT_PLAYER_LEFT, "Left lobby", system->local_player ? system->local_player->id : 0);

    free(system->current_lobby);
    system->current_lobby = NULL;
    system->is_host = false;
    system->is_connected = false;

    if (system->local_player) {
        system->local_player->state = PLAYER_STATE_DISCONNECTED;
    }

    return true;
}

static bool lobby_kick_player(LobbySystem* system, uint32_t player_id) {
    if (!system || !system->is_host || !system->current_lobby) return false;
    // Logic to kick player
    // For now, just fire event
    push_event(system, LOBBY_EVENT_PLAYER_LEFT, "Player kicked", player_id);
    return true;
}

static bool lobby_promote_to_host(LobbySystem* system, uint32_t player_id) {
    if (!system || !system->is_host || !system->current_lobby) return false;
    system->current_lobby->host_id = player_id;
    if (system->local_player && system->local_player->id == player_id) {
        system->is_host = true;
    } else {
        system->is_host = false; // We gave up host
    }
    push_event(system, LOBBY_EVENT_HOST_CHANGED, "Host changed", player_id);
    return true;
}

static uint32_t lobby_find(LobbySystem* system, const LobbySearchFilters* filters,
                           Lobby* results, uint32_t max_results) {
    // Mock implementation returning fake lobbies
    uint32_t count = 0;
    if (!results || max_results == 0) return 0;

    // Fake Lobby 1
    if (count < max_results) {
        Lobby* l = &results[count++];
        memset(l, 0, sizeof(Lobby));
        l->id = 101;
        strcpy(l->name, "Survival 24/7");
        strcpy(l->description, "No griefing");
        l->type = LOBBY_TYPE_PUBLIC;
        l->game_mode = GAME_MODE_SURVIVAL;
        l->current_players = 5;
        l->max_players = 16;
        l->average_ping = 45.0f;
    }

    // Fake Lobby 2
    if (count < max_results) {
        Lobby* l = &results[count++];
        memset(l, 0, sizeof(Lobby));
        l->id = 102;
        strcpy(l->name, "Creative Build");
        l->type = LOBBY_TYPE_PUBLIC;
        l->game_mode = GAME_MODE_CREATIVE;
        l->current_players = 2;
        l->max_players = 8;
        l->average_ping = 120.0f;
    }

    return count;
}

static bool lobby_refresh(LobbySystem* system) {
    return true; // Mock success
}

static Lobby* lobby_get_current(LobbySystem* system) {
    return system ? system->current_lobby : NULL;
}

static bool lobby_set_ready(LobbySystem* system, bool ready) {
    if (!system || !system->local_player) return false;

    system->local_player->state = ready ? PLAYER_STATE_READY : PLAYER_STATE_CONNECTED;
    push_event(system, ready ? LOBBY_EVENT_PLAYER_READY : LOBBY_EVENT_PLAYER_NOT_READY,
               NULL, system->local_player->id);
    return true;
}

static bool lobby_send_chat(LobbySystem* system, const char* message) {
    if (!system || !message) return false;
    // In real system, send packet. Here, loopback event.
    push_event(system, LOBBY_EVENT_CHAT_MESSAGE, message, system->local_player ? system->local_player->id : 0);
    return true;
}

static bool lobby_update_player_info(LobbySystem* system, const char* name) {
    if (!system || !system->local_player) return false;
    strncpy(system->local_player->name, name, MAX_PLAYER_NAME_LENGTH - 1);
    return true;
}

static bool lobby_start_game(LobbySystem* system) {
    if (!system || !system->is_host) return false;
    if (system->current_lobby) {
        system->current_lobby->state = LOBBY_STATE_STARTING;
    }
    push_event(system, LOBBY_EVENT_GAME_STARTED, "Game starting", 0);
    return true;
}

static bool lobby_set_settings(LobbySystem* system, const char* map_name,
                              GameMode game_mode, uint32_t max_players) {
    if (!system || !system->is_host || !system->current_lobby) return false;

    if (map_name) strncpy(system->current_lobby->map_name, map_name, 63);
    system->current_lobby->game_mode = game_mode;
    system->current_lobby->max_players = max_players;

    push_event(system, LOBBY_EVENT_SETTINGS_CHANGED, "Settings updated", 0);
    return true;
}

static bool lobby_migrate_host(LobbySystem* system, uint32_t new_host_id) {
    return lobby_promote_to_host(system, new_host_id);
}

static bool lobby_poll_event(LobbySystem* system, LobbyEvent* event) {
    if (!system || !event) return false;

    if (system->event_queue_head == system->event_queue_tail) {
        return false; // Empty
    }

    *event = system->event_queue[system->event_queue_head];
    system->event_queue_head = (system->event_queue_head + 1) % 32;
    return true;
}

static void lobby_set_event_handler(LobbySystem* system, LobbyEventType type,
                                  void (*handler)(const LobbyEvent*, void*), void* user_data) {
    // Basic implementation doesn't support specific handler per type storage in struct (unless we added it).
    // The struct definition in header has `LobbySystem` fields.
    // It does NOT have a `handlers` array.
    // So this function can't really do much unless we extended the struct or used a global (bad).
    // Or we rely on the user polling.
    // However, the `set_event_handler` is in the interface.
    // For now, let's ignore it or implement a single handler if we could store it.
    // Since we can't easily modify the struct definition (it's in the header and fixed size/layout as defined there),
    // and the struct doesn't have a `handlers` field, this function is a bit of a zombie in the interface.
    // Wait, the struct definition IS in the header `lobby_system.h`.
    // It has `LobbyEvent event_queue[32];` etc.
    // It does NOT have function pointer storage for handlers.
    // So I will leave this as a stub.
}

static uint32_t lobby_get_player_count(LobbySystem* system) {
    return (system && system->current_lobby) ? system->current_lobby->current_players : 0;
}

static uint32_t lobby_get_lobby_count(LobbySystem* system) {
    return 0; // Mock
}

static float lobby_get_avg_ping(LobbySystem* system) {
    return (system && system->current_lobby) ? system->current_lobby->average_ping : 0.0f;
}

// ============================================================================
// Factory
// ============================================================================

LobbySystem* lobby_create_system(void) {
    LobbySystem* sys = (LobbySystem*)malloc(sizeof(LobbySystem));
    if (!sys) return NULL;

    // Bind methods
    sys->init = lobby_init;
    sys->shutdown = lobby_shutdown;
    sys->update = lobby_update;
    sys->create_lobby = lobby_create;
    sys->join_lobby = lobby_join;
    sys->leave_lobby = lobby_leave;
    sys->kick_player = lobby_kick_player;
    sys->promote_to_host = lobby_promote_to_host;
    sys->find_lobbies = lobby_find;
    sys->refresh_lobby_list = lobby_refresh;
    sys->get_current_lobby = lobby_get_current;
    sys->set_ready_status = lobby_set_ready;
    sys->send_chat_message = lobby_send_chat;
    sys->update_player_info = lobby_update_player_info;
    sys->start_game = lobby_start_game;
    sys->set_lobby_settings = lobby_set_settings;
    sys->migrate_host = lobby_migrate_host;
    sys->poll_event = lobby_poll_event;
    sys->set_event_handler = lobby_set_event_handler;
    sys->get_player_count = lobby_get_player_count;
    sys->get_lobby_count = lobby_get_lobby_count;
    sys->get_average_ping = lobby_get_avg_ping;

    // Init state
    if (!sys->init(sys)) {
        free(sys);
        return NULL;
    }

    return sys;
}

void lobby_destroy_system(LobbySystem* system) {
    if (system) {
        system->shutdown(system);
        free(system);
    }
}

// ============================================================================
// Utilities
// ============================================================================

const char* lobby_type_to_string(LobbyType type) {
    switch (type) {
        case LOBBY_TYPE_PUBLIC: return "Public";
        case LOBBY_TYPE_PRIVATE: return "Private";
        case LOBBY_TYPE_FRIENDS_ONLY: return "Friends Only";
        default: return "Unknown";
    }
}

const char* game_mode_to_string(GameMode mode) {
    switch (mode) {
        case GAME_MODE_SURVIVAL: return "Survival";
        case GAME_MODE_CREATIVE: return "Creative";
        case GAME_MODE_ADVENTURE: return "Adventure";
        case GAME_MODE_SPECTATOR: return "Spectator";
        default: return "Unknown";
    }
}

const char* lobby_state_to_string(LobbyState state) {
    switch (state) {
        case LOBBY_STATE_WAITING: return "Waiting";
        case LOBBY_STATE_STARTING: return "Starting";
        case LOBBY_STATE_IN_PROGRESS: return "In Progress";
        case LOBBY_STATE_FINISHED: return "Finished";
        default: return "Unknown";
    }
}

const char* player_state_to_string(PlayerState state) {
    switch (state) {
        case PLAYER_STATE_DISCONNECTED: return "Disconnected";
        case PLAYER_STATE_CONNECTED: return "Connected";
        case PLAYER_STATE_READY: return "Ready";
        case PLAYER_STATE_IN_GAME: return "In Game";
        default: return "Unknown";
    }
}

bool lobby_validate_name(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_LOBBY_NAME_LENGTH) return false;
    return true;
}

bool lobby_validate_password(const char* password) {
    if (password && strlen(password) > 32) return false;
    return true;
}

bool lobby_is_compatible_version(const char* client_version, const char* lobby_version) {
    if (!client_version || !lobby_version) return false;
    return strcmp(client_version, lobby_version) == 0;
}
