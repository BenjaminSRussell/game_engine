#include "modding/mod_api.h"
#include "core/logger.h"

static ModInfo mod_info = {
    .name = "ExampleMod",
    .description = "Example mod demonstrating the mod API",
    .version_major = 1,
    .version_minor = 0,
    .version_patch = 0,
    .author = "Minecraft Team",
    .on_init = NULL,
    .on_shutdown = NULL,
    .on_update = NULL
};

void mod_on_init(Mod *mod) {
    LOG_INFO("ExampleMod initialized");
}

void mod_on_shutdown(Mod *mod) {
    LOG_INFO("ExampleMod shutting down");
}

bool mod_on_update(Mod *mod, f32 delta_time) {
    return true;
}

void mod_on_block_broken(Mod *mod, void *event_data) {
    LOG_DEBUG("Block broken event received by ExampleMod");
}

ModInfo *get_mod_info(void) {
    mod_info.on_init = mod_on_init;
    mod_info.on_shutdown = mod_on_shutdown;
    mod_info.on_update = mod_on_update;
    return &mod_info;
}
