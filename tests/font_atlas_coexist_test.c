/*
 * Localization and advanced text each own a font atlas. Including both
 * headers must compile, and advanced glyph lookup / kerning must still run.
 */
#include "core/localization.h"
#include "rendering/text_rendering_advanced.h"

#include <stdio.h>
#include <stdlib.h>

static const char *g_font_path;

static void fail(const char *msg) {
    if (g_font_path) {
        remove(g_font_path);
    }
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

int main(void) {
    FontAtlas loc_atlas;
    AdvancedFontAtlas adv_atlas;
    FontLibrary *library;
    AdvancedFont *font;
    AdvancedGlyph *glyph_a;
    AdvancedGlyph *glyph_v;
    TextLayout *layout;
    TextMetrics metrics;
    FILE *fp;
    float advance;
    float width;
    const char *path = "font_atlas_coexist_dummy.font";

    g_font_path = path;

    /* Distinct structs: localization stores glyphs, advanced stores pixels. */
    loc_atlas.glyph_count = 1;
    loc_atlas.glyph_capacity = 4;
    loc_atlas.line_height = 12.0f;
    adv_atlas.width = 32;
    adv_atlas.height = 32;
    adv_atlas.region_capacity = 2;
    adv_atlas.owns_pixels = false;
    if (loc_atlas.glyph_capacity != 4 || adv_atlas.region_capacity != 2) {
        fail("atlas fields");
    }
    if (sizeof(struct FontCacheEntry) < 512) {
        fail("FontCacheEntry");
    }
    if (SCRIPT_SINHALA == SCRIPT_TAMIL || SCRIPT_SINHALA == SCRIPT_UNKNOWN ||
        SCRIPT_SINHALA == SCRIPT_ORIYA) {
        fail("SCRIPT_SINHALA");
    }

    library = font_library_create();
    if (!library || !library->font_cache || library->cache_capacity == 0) {
        fail("font_library_create");
    }

    fp = fopen(path, "wb");
    if (!fp || fputc('x', fp) == EOF) {
        if (fp) {
            fclose(fp);
        }
        fail("dummy font");
    }
    fclose(fp);

    if (font_library_load_font(library, path, 10.0f, FONT_RENDER_BITMAP) == 0xFFFFFFFFu) {
        fail("load font");
    }
    font = font_library_get_font(library, 0);
    if (!font || !font->initialized || font->max_atlases == 0) {
        fail("get font");
    }
    if (!font->atlases || font->max_atlases == 0) {
        fail("atlases");
    }

    glyph_a = advanced_font_get_glyph(font, (uint32_t)'A');
    glyph_v = advanced_font_get_glyph(font, (uint32_t)'V');
    if (!glyph_a || !glyph_v) {
        fail("glyph lookup");
    }
    if (advanced_font_get_glyph(font, 0x0D85u) != NULL) {
        fail("missing sinhala glyph");
    }
    advance = 10.0f * 0.6f;
    if (glyph_a->advance_x != advance || glyph_v->advance_x != advance) {
        fail("glyph advance");
    }

    font->kerning_pairs[0].left_glyph = (uint32_t)'A';
    font->kerning_pairs[0].right_glyph = (uint32_t)'V';
    font->kerning_pairs[0].kerning = -1.5f;
    font->kerning_count = 1;
    if (advanced_font_get_kerning(font, (uint32_t)'A', (uint32_t)'V') != -1.5f) {
        fail("kerning hit");
    }
    if (advanced_font_get_kerning(font, (uint32_t)'A', (uint32_t)'A') != 0.0f) {
        fail("kerning miss");
    }

    width = font_get_text_width(font, "AV", 2);
    if (width != advance + (-1.5f) + advance) {
        fail("text width");
    }

    layout = text_layout_create(font, "AV");
    if (!layout || !text_layout_calculate(layout)) {
        fail("layout");
    }
    metrics = text_layout_get_metrics(layout);
    if (metrics.line_count != 1 || metrics.width != width) {
        fail("layout metrics");
    }
    if (!layout->lines || layout->lines[0].end_offset != 2) {
        fail("text line");
    }

    text_layout_destroy(layout);
    font_library_destroy(library);
    remove(path);
    printf("ok\n");
    return 0;
}
