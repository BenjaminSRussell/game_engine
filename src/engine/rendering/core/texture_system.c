#include "rendering/texture_system.h"
#include <stdlib.h>

void texture_system_init() {}

// Deprecated/Legacy stub implementations
// kept to satisfy potential linker dependencies for now
void *texture_load(const char *path) { return NULL; }
void texture_unload(void *texture) {}
void texture_update(void *texture, void *data) {}
void texture_bind(void *texture, int slot) {}

// Valid implementations matching renderer/texture_system.h
void texture_generate_mipmaps(Texture *texture) {}

void texture_set_filter(Texture *texture, TextureFilter filter) {}

void texture_set_wrap(Texture *texture, TextureWrap wrap_u, TextureWrap wrap_v) {}
