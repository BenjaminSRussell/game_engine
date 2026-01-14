#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

void plugin_loader_init(void);
void *plugin_load(const char *path);
void plugin_unload(void *plugin);
void *plugin_get_function(void *plugin, const char *name);
void plugin_loader_shutdown(void);

#endif // PLUGIN_LOADER_H
