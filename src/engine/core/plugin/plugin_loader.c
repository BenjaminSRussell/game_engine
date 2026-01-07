#include "core/plugin/plugin_loader.h"
#include <stdlib.h>

void plugin_loader_init() {}

void *plugin_load(const char *path) {
    return NULL;
}

void plugin_unload(void *plugin) {}

void *plugin_get_function(void *plugin, const char *name) {
    return NULL;
}

void plugin_loader_shutdown() {}
