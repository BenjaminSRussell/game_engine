#include "game/save_system/save_manager.h"
#include <stdlib.h>

void save_manager_init() {}

void save_create_slot(int slot_id) {}

void save_write_to_slot(int slot_id, void *game_state) {
    // Serialize and write game state
}

void save_load_from_slot(int slot_id, void *game_state) {
    // Load and deserialize game state
}

void save_delete_slot(int slot_id) {}

void save_get_slot_info(int slot_id, void *info) {}
