#include "network/lobby_system.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_lobby_lifecycle() {
    printf("Testing Lobby Lifecycle...\n");

    LobbySystem* system = lobby_create_system();
    assert(system != NULL);

    // Create Lobby
    uint32_t lobby_id = system->create_lobby(system, "Test Lobby", "Description",
                                           LOBBY_TYPE_PUBLIC, GAME_MODE_SURVIVAL,
                                           "map1", 8, NULL);
    assert(lobby_id != 0);
    assert(system->current_lobby != NULL);
    assert(system->is_host == true);
    assert(strcmp(system->current_lobby->name, "Test Lobby") == 0);

    // Check Event
    LobbyEvent event;
    bool has_event = system->poll_event(system, &event);
    assert(has_event == true);
    assert(event.type == LOBBY_EVENT_PLAYER_JOINED);

    // Send Chat
    system->send_chat_message(system, "Hello World");
    has_event = system->poll_event(system, &event);
    assert(has_event == true);
    assert(event.type == LOBBY_EVENT_CHAT_MESSAGE);
    assert(strcmp(event.message, "Hello World") == 0);

    // Leave
    system->leave_lobby(system);
    assert(system->current_lobby == NULL);
    assert(system->is_host == false);

    // Check Leave Event
    has_event = system->poll_event(system, &event);
    assert(has_event == true);
    assert(event.type == LOBBY_EVENT_PLAYER_LEFT);

    lobby_destroy_system(system);
    printf("Lobby Lifecycle Passed.\n");
}

void test_lobby_discovery() {
    printf("Testing Lobby Discovery...\n");

    LobbySystem* system = lobby_create_system();

    LobbySearchFilters filters = {0};
    Lobby results[10];

    uint32_t count = system->find_lobbies(system, &filters, results, 10);
    assert(count > 0); // Mock returns at least 2
    assert(results[0].id != 0);

    printf("Found %d lobbies.\n", count);

    // Test Join
    bool joined = system->join_lobby(system, results[0].id, NULL);
    assert(joined == true);
    assert(system->current_lobby != NULL);
    assert(system->current_lobby->id == results[0].id);
    assert(system->is_host == false);

    lobby_destroy_system(system);
    printf("Lobby Discovery Passed.\n");
}

int main() {
    test_lobby_lifecycle();
    test_lobby_discovery();
    printf("All Lobby System tests passed.\n");
    return 0;
}
