#include "game/dialogue/dialogue_tree.h"

void dialogue_tree_init() {}

void dialogue_tree_load(const char *dialogue_file) {}

void dialogue_tree_start(void *npc) {}

void dialogue_tree_choose_option(int option_index) {}

const char* dialogue_tree_get_current_text() {
    return "Hello!";
}

void dialogue_tree_get_options(void *output_list) {}
