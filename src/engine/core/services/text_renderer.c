// src/render/text_renderer.c
//
// Implementation of the text rendering system with font atlas support.
//
// TEXT-SYSTEM-001: Implement text rendering batching system.
// TEXT-SYSTEM-002: Add text rendering caching system.
// TEXT-SYSTEM-003: Implement text rendering optimization.
// TEXT-SYSTEM-004: Add text rendering statistics tracking.
// TEXT-SYSTEM-005: Implement text rendering debugging visualization.
// TEXT-SYSTEM-006: Add text rendering performance profiling.
// TEXT-SYSTEM-007: Implement text rendering configuration system.
// TEXT-SYSTEM-008: Add text rendering unit testing framework.
// TEXT-SYSTEM-009: Implement text rendering documentation system.
// TEXT-SYSTEM-010: Add text rendering optimization suggestions.
#include <core/logger.h>
#include <ctype.h>
#include <include/math/math.h>
#include <rendering/text_renderer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==============================================================================
// Font Library Lifecycle
// ==============================================================================

void font_library_init(FontLibrary *library) {
  if (!library) {
    fprintf(stderr, "[TEXT] Invalid library pointer\n");
    return;
  }

  memset(library, 0, sizeof(FontLibrary));
  library->font_count = 0;
  library->mesh_count = 0;
  library->default_font_id = 0xFFFFFFFF;
  library->initialized = true;

  fprintf(stderr, "[TEXT] Font library initialized\n");
  fprintf(stderr, "[TEXT]  - Max fonts: %u\n", MAX_FONTS);
  fprintf(stderr, "[TEXT]  - Max text meshes: %u\n", MAX_TEXT_MESHES);
}

void font_library_shutdown(FontLibrary *library) {
  if (!library || !library->initialized) {
    return;
  }

  // Free all text meshes
  for (u32 i = 0; i < library->mesh_count; i++) {
    TextMesh *mesh = &library->meshes[i];
    if (mesh->vertices)
      free(mesh->vertices);
    if (mesh->uvs)
      free(mesh->uvs);
    if (mesh->colors)
      free(mesh->colors);
    if (mesh->indices)
      free(mesh->indices);
  }

  library->font_count = 0;
  library->mesh_count = 0;
  library->initialized = false;

  fprintf(stderr, "[TEXT] Font library shut down\n");
}

// ==============================================================================
// Font Loading
// ==============================================================================

u32 font_load_from_file(FontLibrary *library, const char *filepath, u32 size,
                        FontStyle style) {
  if (!library || !library->initialized || !filepath) {
    return 0xFFFFFFFF;
  }

  if (library->font_count >= MAX_FONTS) {
    fprintf(stderr, "[TEXT] Font library full (max %u)\n", MAX_FONTS);
    return 0xFFFFFFFF;
  }

  u32 font_id = library->font_count++;
  Font *font = &library->fonts[font_id];

  memset(font, 0, sizeof(Font));
  font->font_id = font_id;
  strncpy(font->name, filepath, sizeof(font->name) - 1);
  font->size = size;
  font->style = style;
  font->line_height = size + 4;
  font->baseline = size - 2;
  font->scale_x = 1.0f;
  font->scale_y = 1.0f;
  font->atlas_width = 512;
  font->atlas_height = 512;

  // Initialize basic ASCII glyphs (placeholder)
  for (u32 c = 32; c < 127; c++) {
    Glyph *glyph = &font->glyphs[c];
    glyph->codepoint = c;
    glyph->width = size / 2; // Estimate
    glyph->height = size;
    glyph->bearing_x = 0;
    glyph->bearing_y = size;
    glyph->advance = size / 2;
    glyph->uv_x = ((c - 32) % 16) * (glyph->width / 512.0f);
    glyph->uv_y = ((c - 32) / 16) * (glyph->height / 512.0f);
    glyph->uv_width = glyph->width / 512.0f;
    glyph->uv_height = glyph->height / 512.0f;
  }

  font->glyph_count = 127 - 32;
  font->initialized = true;

  if (library->default_font_id == 0xFFFFFFFF) {
    library->default_font_id = font_id;
  }

  fprintf(stderr, "[TEXT] Loaded font '%s' (ID: %u, size: %u, style: %u)\n",
          filepath, font_id, size, style);

  return font_id;
}

u32 font_load_async(FontLibrary *library, const char *filepath, u32 size,
                    FontStyle style) {
  if (!library || !library->initialized || !filepath) {
    return 0xFFFFFFFF;
  }

  // For now, load synchronously
  return font_load_from_file(library, filepath, size, style);
}

bool font_is_loaded(FontLibrary *library, u32 font_id) {
  if (!library || !library->initialized) {
    return false;
  }

  return font_id < library->font_count && library->fonts[font_id].initialized;
}

Font *font_get(FontLibrary *library, u32 font_id) {
  if (!library || !library->initialized) {
    return NULL;
  }

  if (font_id >= library->font_count) {
    return NULL;
  }

  return library->fonts[font_id].initialized ? &library->fonts[font_id] : NULL;
}

Font *font_get_default(FontLibrary *library) {
  if (!library || !library->initialized) {
    return NULL;
  }

  if (library->default_font_id >= library->font_count) {
    return NULL;
  }

  return &library->fonts[library->default_font_id];
}

void font_set_default(FontLibrary *library, u32 font_id) {
  if (library && library->initialized && font_id < library->font_count) {
    library->default_font_id = font_id;
  }
}

// ==============================================================================
// Glyph Management
// ==============================================================================

Glyph *font_get_glyph(Font *font, u32 codepoint) {
  if (!font) {
    return NULL;
  }

  if (codepoint >= 256) {
    return NULL; // Simplified: only support ASCII + extended ASCII
  }

  return &font->glyphs[codepoint];
}

void font_precache_glyphs(Font *font, const char *text) {
  if (!font || !text) {
    return;
  }

  // Placeholder: in production, would load glyphs from font file
  (void)text;
}

i32 font_get_kerning(Font *font, u32 left_codepoint, u32 right_codepoint) {
  if (!font) {
    return 0;
  }

  // Simple kerning lookup
  for (u32 i = 0; i < font->kerning_count; i++) {
    if (font->kerning_pairs[i].left_glyph == left_codepoint &&
        font->kerning_pairs[i].right_glyph == right_codepoint) {
      return font->kerning_pairs[i].kerning_amount;
    }
  }

  return 0;
}

// ==============================================================================
// Text Measurement
// ==============================================================================

f32 text_measure_width(Font *font, const char *text) {
  if (!font || !text) {
    return 0.0f;
  }

  f32 width = 0.0f;
  u32 prev_codepoint = 0;

  for (const char *p = text; *p; p++) {
    u32 codepoint = (u8)(*p);

    // Get glyph
    Glyph *glyph = font_get_glyph(font, codepoint);
    if (!glyph) {
      continue;
    }

    // Add kerning
    width += font_get_kerning(font, prev_codepoint, codepoint);

    // Add advance width
    width += glyph->advance;

    prev_codepoint = codepoint;
  }

  return width;
}

f32 text_measure_height(Font *font, const char *text) {
  if (!font) {
    return 0.0f;
  }

  // Count lines
  u32 line_count = 1;
  for (const char *p = text; *p; p++) {
    if (*p == '\n') {
      line_count++;
    }
  }

  return (f32)line_count * font->line_height;
}

TextMeasurement text_measure_formatted(FormattedText *formatted_text) {
  TextMeasurement result = {0, 0, 1};

  if (!formatted_text || !formatted_text->font) {
    return result;
  }

  result.height =
      text_measure_height(formatted_text->font, formatted_text->text);
  result.width = text_measure_width(formatted_text->font, formatted_text->text);

  // Count lines
  for (const char *p = formatted_text->text; *p; p++) {
    if (*p == '\n') {
      result.line_count++;
    }
  }

  return result;
}

Vec2 text_get_char_position(Font *font, const char *text, u32 char_index) {
  Vec2 pos = {0.0f, 0.0f};

  if (!font || !text) {
    return pos;
  }

  u32 current_char = 0;
  u32 prev_codepoint = 0;

  for (const char *p = text; *p && current_char <= char_index;
       p++, current_char++) {
    if (current_char == char_index) {
      break;
    }

    u32 codepoint = (u8)(*p);

    if (codepoint == '\n') {
      pos.x = 0.0f;
      pos.y += font->line_height;
      prev_codepoint = 0;
      continue;
    }

    Glyph *glyph = font_get_glyph(font, codepoint);
    if (glyph) {
      pos.x += font_get_kerning(font, prev_codepoint, codepoint);
      pos.x += glyph->advance;
      prev_codepoint = codepoint;
    }
  }

  return pos;
}

u32 text_get_char_at_position(Font *font, const char *text, f32 x) {
  if (!font || !text) {
    return 0;
  }

  f32 current_x = 0.0f;
  u32 char_index = 0;
  u32 prev_codepoint = 0;

  for (const char *p = text; *p; p++, char_index++) {
    u32 codepoint = (u8)(*p);

    if (codepoint == '\n') {
      current_x = 0.0f;
      prev_codepoint = 0;
      continue;
    }

    Glyph *glyph = font_get_glyph(font, codepoint);
    if (!glyph) {
      continue;
    }

    current_x += font_get_kerning(font, prev_codepoint, codepoint);
    current_x += glyph->advance;

    if (current_x > x) {
      return char_index;
    }

    prev_codepoint = codepoint;
  }

  return char_index;
}

// ==============================================================================
// Text Rendering (Mesh Generation)
// ==============================================================================

u32 text_mesh_create(FontLibrary *library, Font *font, const char *text,
                     Vec2 position, Vec4 color) {
  if (!library || !library->initialized || !font || !text) {
    return 0xFFFFFFFF;
  }

  if (library->mesh_count >= MAX_TEXT_MESHES) {
    fprintf(stderr, "[TEXT] Text mesh pool exhausted\n");
    return 0xFFFFFFFF;
  }

  u32 mesh_id = library->mesh_count++;
  TextMesh *mesh = &library->meshes[mesh_id];

  // Estimate vertex count (4 verts per glyph)
  u32 text_len = strlen(text);
  u32 estimated_verts = text_len * 4;

  // Allocate mesh data
  mesh->vertices = malloc(estimated_verts * sizeof(Vec3));
  mesh->uvs = malloc(estimated_verts * sizeof(Vec2));
  mesh->colors = malloc(estimated_verts * sizeof(Vec4));
  mesh->indices = malloc(estimated_verts * 2 * sizeof(u32));

  if (!mesh->vertices || !mesh->uvs || !mesh->colors || !mesh->indices) {
    fprintf(stderr, "[TEXT] Failed to allocate text mesh\n");
    if (mesh->vertices)
      free(mesh->vertices);
    if (mesh->uvs)
      free(mesh->uvs);
    if (mesh->colors)
      free(mesh->colors);
    if (mesh->indices)
      free(mesh->indices);
    library->mesh_count--;
    return 0xFFFFFFFF;
  }

  mesh->vertex_count = 0;
  mesh->index_count = 0;

  // Generate vertices for each glyph
  Vec2 current_pos = position;
  u32 prev_codepoint = 0;

  for (const char *p = text; *p; p++) {
    u32 codepoint = (u8)(*p);

    if (codepoint == '\n') {
      current_pos.x = position.x;
      current_pos.y += font->line_height;
      prev_codepoint = 0;
      continue;
    }

    Glyph *glyph = font_get_glyph(font, codepoint);
    if (!glyph) {
      continue;
    }

    // Apply kerning
    current_pos.x += font_get_kerning(font, prev_codepoint, codepoint);

    // Create quad for glyph
    u32 v_start = mesh->vertex_count;

    // Positions (quad: bottom-left, bottom-right, top-right, top-left)
    mesh->vertices[mesh->vertex_count++] =
        (Vec3){current_pos.x, current_pos.y, 0.0f};
    mesh->vertices[mesh->vertex_count++] =
        (Vec3){current_pos.x + glyph->width, current_pos.y, 0.0f};
    mesh->vertices[mesh->vertex_count++] = (Vec3){
        current_pos.x + glyph->width, current_pos.y + glyph->height, 0.0f};
    mesh->vertices[mesh->vertex_count++] =
        (Vec3){current_pos.x, current_pos.y + glyph->height, 0.0f};

    // UVs
    mesh->uvs[v_start + 0] = (Vec2){glyph->uv_x, glyph->uv_y};
    mesh->uvs[v_start + 1] = (Vec2){glyph->uv_x + glyph->uv_width, glyph->uv_y};
    mesh->uvs[v_start + 2] =
        (Vec2){glyph->uv_x + glyph->uv_width, glyph->uv_y + glyph->uv_height};
    mesh->uvs[v_start + 3] =
        (Vec2){glyph->uv_x, glyph->uv_y + glyph->uv_height};

    // Colors
    for (u32 i = 0; i < 4; i++) {
      mesh->colors[v_start + i] = color;
    }

    // Indices (two triangles per glyph)
    mesh->indices[mesh->index_count++] = v_start + 0;
    mesh->indices[mesh->index_count++] = v_start + 1;
    mesh->indices[mesh->index_count++] = v_start + 2;
    mesh->indices[mesh->index_count++] = v_start + 0;
    mesh->indices[mesh->index_count++] = v_start + 2;
    mesh->indices[mesh->index_count++] = v_start + 3;

    current_pos.x += glyph->advance;
    prev_codepoint = codepoint;
  }

  fprintf(stderr,
          "[TEXT] Created text mesh (ID: %u, %u vertices, %u indices)\n",
          mesh_id, mesh->vertex_count, mesh->index_count);

  return mesh_id;
}

void text_mesh_update(FontLibrary *library, u32 mesh_id, const char *new_text) {
  if (!library || mesh_id >= library->mesh_count) {
    return;
  }

  // Placeholder: would update existing mesh
  (void)new_text;
}

void text_mesh_delete(FontLibrary *library, u32 mesh_id) {
  if (!library || mesh_id >= library->mesh_count) {
    return;
  }

  TextMesh *mesh = &library->meshes[mesh_id];

  if (mesh->vertices)
    free(mesh->vertices);
  if (mesh->uvs)
    free(mesh->uvs);
  if (mesh->colors)
    free(mesh->colors);
  if (mesh->indices)
    free(mesh->indices);

  // Shift remaining meshes
  if (mesh_id < library->mesh_count - 1) {
    library->meshes[mesh_id] = library->meshes[library->mesh_count - 1];
  }

  library->mesh_count--;
}

TextMesh *text_mesh_get(FontLibrary *library, u32 mesh_id) {
  if (!library || mesh_id >= library->mesh_count) {
    return NULL;
  }

  return &library->meshes[mesh_id];
}

void text_render(Font *font, const char *text, Vec2 position, Vec4 color) {
  if (!font || !text) {
    return;
  }

  // Placeholder: would render text to screen
  fprintf(stderr, "[TEXT] Rendering: '%s' at (%.1f, %.1f)\n", text, position.x,
          position.y);
}

void text_render_formatted(FormattedText *formatted_text) {
  if (!formatted_text || !formatted_text->font) {
    return;
  }

  text_render(formatted_text->font, formatted_text->text,
              formatted_text->position, formatted_text->color);
}

// ==============================================================================
// Text Formatting
// ==============================================================================

FormattedText text_format_create(Font *font, const char *text) {
  FormattedText fmt;
  memset(&fmt, 0, sizeof(FormattedText));

  fmt.font = font;
  if (text) {
    strncpy(fmt.text, text, sizeof(fmt.text) - 1);
  }
  fmt.color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
  fmt.position = (Vec2){0.0f, 0.0f};
  fmt.alignment = TEXT_ALIGN_LEFT;
  fmt.valignment = TEXT_VALIGN_TOP;
  fmt.line_spacing = 1.0f;
  fmt.letter_spacing = 0.0f;
  fmt.max_width = 0;
  fmt.shadow = false;

  return fmt;
}

void text_format_set_color(FormattedText *text, Vec4 color) {
  if (text) {
    text->color = color;
  }
}

void text_format_set_position(FormattedText *text, Vec2 position) {
  if (text) {
    text->position = position;
  }
}

void text_format_set_alignment(FormattedText *text, TextAlignment align,
                               TextVerticalAlignment valign) {
  if (text) {
    text->alignment = align;
    text->valignment = valign;
  }
}

void text_format_set_line_spacing(FormattedText *text, f32 spacing) {
  if (text) {
    text->line_spacing = fmaxf(spacing, 0.5f);
  }
}

void text_format_set_letter_spacing(FormattedText *text, f32 spacing) {
  if (text) {
    text->letter_spacing = spacing;
  }
}

void text_format_set_shadow(FormattedText *text, Vec4 shadow_color,
                            Vec2 offset) {
  if (text) {
    text->shadow = true;
    text->shadow_color = shadow_color;
    text->shadow_offset = offset;
  }
}

void text_format_set_wrap_width(FormattedText *text, u32 max_width) {
  if (text) {
    text->max_width = max_width;
  }
}

// ==============================================================================
// Text Effects (Placeholder implementations)
// ==============================================================================

void text_effect_outline(TextMesh *mesh, f32 outline_width,
                         Vec4 outline_color) {
  if (mesh) {
    fprintf(stderr, "[TEXT] Applied outline effect (width: %.1f)\n",
            outline_width);
  }
}

void text_effect_glow(TextMesh *mesh, f32 glow_amount, Vec4 glow_color) {
  if (mesh) {
    fprintf(stderr, "[TEXT] Applied glow effect (amount: %.1f)\n", glow_amount);
  }
}

void text_effect_strikethrough(TextMesh *mesh, Vec4 line_color) {
  if (mesh) {
    fprintf(stderr, "[TEXT] Applied strikethrough effect\n");
  }
}

void text_effect_underline(TextMesh *mesh, Vec4 line_color) {
  if (mesh) {
    fprintf(stderr, "[TEXT] Applied underline effect\n");
  }
}

// ==============================================================================
// Font Metrics
// ==============================================================================

f32 font_get_ascender(Font *font) {
  if (!font)
    return 0.0f;
  return (f32)font->baseline;
}

f32 font_get_descender(Font *font) {
  if (!font)
    return 0.0f;
  return (f32)(font->line_height - font->baseline);
}

f32 font_get_line_gap(Font *font) {
  if (!font)
    return 0.0f;
  return (f32)(font->line_height - font->size);
}

// ==============================================================================
// Debug Rendering
// ==============================================================================

void text_debug_render_glyph_atlas(Font *font) {
  if (!font) {
    fprintf(stderr, "[TEXT] Invalid font\n");
    return;
  }

  fprintf(stderr, "[TEXT] Glyph atlas: %ux%u\n", font->atlas_width,
          font->atlas_height);
  fprintf(stderr, "[TEXT] Glyphs cached: %u\n", font->glyph_count);
}

void text_log_font_info(Font *font) {
  if (!font) {
    fprintf(stderr, "[TEXT] Invalid font\n");
    return;
  }

  fprintf(stderr, "[TEXT] Font Info: '%s'\n", font->name);
  fprintf(stderr, "[TEXT]   Size: %u px\n", font->size);
  fprintf(stderr, "[TEXT]   Line height: %u px\n", font->line_height);
  fprintf(stderr, "[TEXT]   Glyphs: %u\n", font->glyph_count);
  fprintf(stderr, "[TEXT]   Kerning pairs: %u\n", font->kerning_count);
}

void text_library_log_stats(FontLibrary *library) {
  if (!library) {
    fprintf(stderr, "[TEXT] Library not initialized\n");
    return;
  }

  fprintf(stderr, "[TEXT] ===== Font Library Statistics =====\n");
  fprintf(stderr, "[TEXT] Fonts: %u / %u\n", library->font_count, MAX_FONTS);
  fprintf(stderr, "[TEXT] Text meshes: %u / %u\n", library->mesh_count,
          MAX_TEXT_MESHES);

  u32 total_glyphs = 0;
  for (u32 i = 0; i < library->font_count; i++) {
    total_glyphs += library->fonts[i].glyph_count;
  }
  fprintf(stderr, "[TEXT] Total glyphs: %u\n", total_glyphs);
}

// ==============================================================================
// Unicode Support
// ==============================================================================

f32 text_get_char_width(Font *font, u32 codepoint) {
  if (!font) {
    return 0.0f;
  }

  Glyph *glyph = font_get_glyph(font, codepoint);
  if (!glyph) {
    return 0.0f;
  }

  return (f32)glyph->advance;
}

u32 text_convert_utf8(const char *utf8_str, u32 *out_codepoints,
                      u32 max_codepoints) {
  if (!utf8_str || !out_codepoints) {
    return 0;
  }

  u32 count = 0;

  for (const u8 *p = (const u8 *)utf8_str; *p && count < max_codepoints; p++) {
    u32 codepoint = 0;

    if (*p < 0x80) {
      codepoint = *p;
      p += 0;
    } else if ((*p & 0xE0) == 0xC0) {
      codepoint = ((*p & 0x1F) << 6) | (*(p + 1) & 0x3F);
      p += 1;
    } else if ((*p & 0xF0) == 0xE0) {
      codepoint =
          ((*p & 0x0F) << 12) | ((*(p + 1) & 0x3F) << 6) | (*(p + 2) & 0x3F);
      p += 2;
    } else if ((*p & 0xF8) == 0xF0) {
      codepoint = ((*p & 0x07) << 18) | ((*(p + 1) & 0x3F) << 12) |
                  ((*(p + 2) & 0x3F) << 6) | (*(p + 3) & 0x3F);
      p += 3;
    }

    if (codepoint > 0) {
      out_codepoints[count++] = codepoint;
    }
  }

  return count;
}

// ==============================================================================
// Serialization
// ==============================================================================

bool font_serialize(Font *font, u8 *buffer, u32 buffer_size, u32 *out_size) {
  if (!font || !buffer || !out_size) {
    return false;
  }

  if (buffer_size < sizeof(Font)) {
    return false;
  }

  memcpy(buffer, font, sizeof(Font));
  *out_size = sizeof(Font);

  return true;
}

bool font_deserialize(u8 *buffer, u32 size, Font *out_font) {
  if (!buffer || !out_font) {
    return false;
  }

  if (size < sizeof(Font)) {
    return false;
  }

  memcpy(out_font, buffer, sizeof(Font));
  return true;
}
