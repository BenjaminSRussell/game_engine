#include "ui/text/font_atlas_gen.h"
#include <stdlib.h>

void font_atlas_init() {}

void font_atlas_load_ttf(const char *path, int size) {}

void font_atlas_generate(void *output_texture) {
    // Rasterize glyphs into texture atlas
}

void font_atlas_get_glyph_uv(int character, float *uv) {}

void font_atlas_shutdown() {}
