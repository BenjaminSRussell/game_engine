// 3D Text Renderer for Debug System implementation
#include "debug/debug_3d_text_renderer.h"
#include <OpenGL/gl3.h>
#include "engine/include/core/logger.h"
#include <math/mat4.h>
#include "engine/include/math/math_all.h"
#include <math/vec3.h>
#include <math/vec4.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global debug text renderer instance
Debug3DTextRenderer *g_debug_text_renderer = NULL;

// MARK: - Helper Functions

static u32 get_next_power_of_two(u32 n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  // TODO(Jules): Optimize get_next_power_of_two using intrinsic functions.
  return n;
}

static void calculate_text_bounds(Text3D *text, Vec3 *min_bounds,
                                  Vec3 *max_bounds) {
  if (!text || !text->font || !min_bounds || !max_bounds) {
    return;
  }

  float text_width =
      strlen(text->text) * text->font->character_width * text->scale.x;
  float text_height = text->font->character_height * text->scale.y;

  // TODO(Jules): Add kerning support to calculate_text_bounds.
  // Calculate bounds based on anchor
  switch (text->anchor) {
  case TEXT_ANCHOR_TOP_LEFT:
    *min_bounds = text->position;
    *max_bounds = (Vec3){text->position.x + text_width,
                         text->position.y - text_height, text->position.z};
    break;
  case TEXT_ANCHOR_TOP_CENTER:
    *min_bounds = (Vec3){text->position.x - text_width * 0.5f, text->position.y,
                         text->position.z};
    *max_bounds = (Vec3){text->position.x + text_width * 0.5f,
                         text->position.y - text_height, text->position.z};
    break;
  case TEXT_ANCHOR_TOP_RIGHT:
    *min_bounds = (Vec3){text->position.x - text_width, text->position.y,
                         text->position.z};
    *max_bounds = (Vec3){text->position.x, text->position.y - text_height,
                         text->position.z};
    break;
  case TEXT_ANCHOR_MIDDLE_LEFT:
    *min_bounds =
        (Vec3){text->position.x, text->position.y - text_height * 0.5f,
               text->position.z};
    *max_bounds =
        (Vec3){text->position.x + text_width,
               text->position.y + text_height * 0.5f, text->position.z};
    break;
  case TEXT_ANCHOR_MIDDLE_CENTER:
    *min_bounds =
        (Vec3){text->position.x - text_width * 0.5f,
               text->position.y - text_height * 0.5f, text->position.z};
    *max_bounds =
        (Vec3){text->position.x + text_width * 0.5f,
               text->position.y + text_height * 0.5f, text->position.z};
    break;
  case TEXT_ANCHOR_MIDDLE_RIGHT:
    *min_bounds =
        (Vec3){text->position.x - text_width,
               text->position.y - text_height * 0.5f, text->position.z};
    *max_bounds =
        (Vec3){text->position.x, text->position.y + text_height * 0.5f,
               text->position.z};
    break;
  case TEXT_ANCHOR_BOTTOM_LEFT:
    *min_bounds = (Vec3){text->position.x, text->position.y - text_height,
                         text->position.z};
    *max_bounds = (Vec3){text->position.x + text_width, text->position.y,
                         text->position.z};
    break;
  case TEXT_ANCHOR_BOTTOM_CENTER:
    *min_bounds = (Vec3){text->position.x - text_width * 0.5f,
                         text->position.y - text_height, text->position.z};
    *max_bounds = (Vec3){text->position.x + text_width * 0.5f, text->position.y,
                         text->position.z};
    break;
  case TEXT_ANCHOR_BOTTOM_RIGHT:
    *min_bounds = (Vec3){text->position.x - text_width,
                         text->position.y - text_height, text->position.z};
    *max_bounds = (Vec3){text->position.x, text->position.y, text->position.z};
    break;
  }
}

static u32 create_shader_program(void) {
  const char *vertex_shader_source =
      "#version 330 core\n"
      "layout (location = 0) in vec3 aPosition;\n"
      "layout (location = 1) in vec2 aTexCoord;\n"
      "layout (location = 2) in vec4 aColor;\n"
      "\n"
      "uniform mat4 uViewMatrix;\n"
      "uniform mat4 uProjectionMatrix;\n"
      "uniform mat4 uModelMatrix;\n"
      "uniform vec4 uGlobalColor;\n"
      "uniform float uGlobalAlpha;\n"
      "\n"
      "out vec2 TexCoord;\n"
      "out vec4 Color;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * "
      "vec4(aPosition, 1.0);\n"
      "    TexCoord = aTexCoord;\n"
      "    Color = aColor * uGlobalColor * uGlobalAlpha;\n"
      "}\n";

  const char *fragment_shader_source =
      "#version 330 core\n"
      "in vec2 TexCoord;\n"
      "in vec4 Color;\n"
      "\n"
      "uniform sampler2D uTextureAtlas;\n"
      "uniform vec4 uOutlineColor;\n"
      "uniform vec4 uShadowColor;\n"
      "uniform vec4 uGlowColor;\n"
      "uniform float uOutlineWidth;\n"
      "uniform vec2 uShadowOffset;\n"
      "uniform float uGlowIntensity;\n"
      "uniform bool uHasOutline;\n"
      "uniform bool uHasShadow;\n"
      "uniform bool uHasGlow;\n"
      "\n"
      "out vec4 FragColor;\n"
      "\n"
      "void main()\n"
      "{\n"
      "    vec4 texColor = texture(uTextureAtlas, TexCoord);\n"
      "    vec4 finalColor = Color * texColor;\n"
      "    \n"
      "    if (texColor.a < 0.1) discard;\n"
      "    \n"
      "    // Apply effects\n"
      "    if (uHasOutline) {\n"
      "        // Outline effect would be implemented here\n"
      "    }\n"
      "    \n"
      "    if (uHasShadow) {\n"
      "        // Shadow effect would be implemented here\n"
      "    }\n"
      "    \n"
      "    if (uHasGlow) {\n"
      "        // Glow effect would be implemented here\n"
      "        finalColor.rgb += uGlowColor.rgb * uGlowIntensity * "
      "texColor.a;\n"
      "    }\n"
      "    \n"
      "    FragColor = finalColor;\n"
      "}\n";

  // Compile vertex shader
  u32 vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);

  // Check vertex shader compilation
  int success;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
    LOG_ERROR("Vertex shader compilation failed: %s", info_log);
    glDeleteShader(vertex_shader);
    return 0;
  }

  // Compile fragment shader
  u32 fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
  glCompileShader(fragment_shader);

  // Check fragment shader compilation
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
    LOG_ERROR("Fragment shader compilation failed: %s", info_log);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return 0;
  }

  // Link shader program
  u32 shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  // Check linking
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    char info_log[512];
    glGetProgramInfoLog(shader_program, 512, NULL, info_log);
    LOG_ERROR("Shader program linking failed: %s", info_log);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(shader_program);
    return 0;
  }

  // Clean up shaders
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return shader_program;
}

// MARK: - 3D Text Renderer Management

bool debug_text_renderer_init(Debug3DTextRenderer *renderer, u32 max_fonts,
                              u32 max_batches) {
  if (!renderer || max_fonts == 0 || max_batches == 0) {
    LOG_ERROR("Invalid parameters for debug_text_renderer_init");
    return false;
  }

  memset(renderer, 0, sizeof(Debug3DTextRenderer));

  // Allocate fonts array
  renderer->fonts = (FontMetrics *)calloc(max_fonts, sizeof(FontMetrics));
  if (!renderer->fonts) {
    LOG_ERROR("Failed to allocate fonts array");
    return false;
  }
  renderer->font_capacity = max_fonts;

  // Allocate batches array
  renderer->batches = (TextBatch *)calloc(max_batches, sizeof(TextBatch));
  if (!renderer->batches) {
    free(renderer->fonts);
    LOG_ERROR("Failed to allocate batches array");
    return false;
  }
  renderer->batch_capacity = max_batches;

  // Create shader program
  renderer->shader_program = create_shader_program();
  if (renderer->shader_program == 0) {
    free(renderer->fonts);
    free(renderer->batches);
    LOG_ERROR("Failed to create shader program");
    return false;
  }

  // Create vertex array
  glGenVertexArrays(1, &renderer->vertex_array);
  glBindVertexArray(renderer->vertex_array);

  // Set default settings
  renderer->enable_depth_testing = true;
  renderer->enable_face_camera = false;
  renderer->global_alpha = 1.0f;
  renderer->global_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};

  // Load default font
  renderer->default_font = debug_text_load_system_font(renderer, "Arial", 16);
  if (!renderer->default_font) {
    LOG_WARN("Failed to load default system font");
  }

  // Set global instance
  g_debug_text_renderer = renderer;

  LOG_INFO("3D text renderer initialized (fonts: %u, batches: %u)", max_fonts,
           max_batches);
  return true;
}

void debug_text_renderer_shutdown(Debug3DTextRenderer *renderer) {
  if (!renderer) {
    return;
  }

  // Delete shader program
  if (renderer->shader_program) {
    glDeleteProgram(renderer->shader_program);
  }

  // Delete vertex array
  if (renderer->vertex_array) {
    glDeleteVertexArrays(1, &renderer->vertex_array);
  }

  // Delete texture atlas
  if (renderer->texture_atlas) {
    glDeleteTextures(1, &renderer->texture_atlas);
  }

  // Clean up batches
  if (renderer->batches) {
    for (u32 i = 0; i < renderer->batch_count; i++) {
      TextBatch *batch = &renderer->batches[i];
      if (batch->texts) {
        free(batch->texts);
      }
      if (batch->vertex_buffer) {
        glDeleteBuffers(1, &batch->vertex_buffer);
      }
      if (batch->index_buffer) {
        glDeleteBuffers(1, &batch->index_buffer);
      }
    }
    free(renderer->batches);
  }

  // Clean up fonts
  if (renderer->fonts) {
    free(renderer->fonts);
  }

  // Clear global instance
  g_debug_text_renderer = NULL;

  memset(renderer, 0, sizeof(Debug3DTextRenderer));
  LOG_INFO("3D text renderer shutdown");
}

// MARK: - Font Management

FontMetrics *debug_text_load_system_font(Debug3DTextRenderer *renderer,
                                         const char *font_name, u32 font_size) {
  if (!renderer || !font_name || font_size == 0) {
    return NULL;
  }

  if (renderer->font_count >= renderer->font_capacity) {
    LOG_ERROR("Font capacity exceeded");
    // TODO(Jules): Implement texture atlas dynamic resizing if capacity is
    // exceeded.
    return NULL;
  }

  FontMetrics *font = &renderer->fonts[renderer->font_count++];
  memset(font, 0, sizeof(FontMetrics));

  font->font_size = font_size;
  font->line_height = font_size * 1.2f;
  font->character_width = font_size * 0.6f;
  font->character_height = font_size;
  font->characters_per_row = 16;
  font->character_count = 128; // ASCII range

  // TODO(Jules): Add support for multisampled fonts in
  // debug_text_load_system_font. Calculate atlas dimensions
  u32 chars_per_row = font->characters_per_row;
  u32 rows = (font->character_count + chars_per_row - 1) / chars_per_row;
  font->atlas_width = get_next_power_of_two(chars_per_row * font_size);
  font->atlas_height = get_next_power_of_two(rows * font_size);

  // Load font atlas (simplified - would use actual font loading library)
  u8 *atlas_data = NULL;
  if (!platform_load_system_font(font_name, font_size, &atlas_data,
                                 &font->atlas_width, &font->atlas_height)) {
    LOG_ERROR("Failed to load system font: %s", font_name);
    renderer->font_count--;
    return NULL;
  }

  // Create OpenGL texture
  glGenTextures(1, &renderer->texture_atlas);
  glBindTexture(GL_TEXTURE_2D, renderer->texture_atlas);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, font->atlas_width, font->atlas_height,
               0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  if (atlas_data) {
    free(atlas_data);
  }

  LOG_DEBUG("Loaded system font: %s (%u px)", font_name, font_size);

  if (renderer->on_font_loaded) {
    renderer->on_font_loaded(font);
  }

  return font;
}

// MARK: - Text Creation and Management

Text3D *debug_text_create(Debug3DTextRenderer *renderer, const char *text,
                          const Vec3 *position, FontMetrics *font) {
  if (!renderer || !text || !position) {
    return NULL;
  }

  // Use default font if none specified
  if (!font) {
    font = renderer->default_font;
  }

  if (!font) {
    LOG_ERROR("No font available for text creation");
    return NULL;
  }

  // Find or create a batch for this font
  TextBatch *batch = NULL;
  for (u32 i = 0; i < renderer->batch_count; i++) {
    if (renderer->batches[i].font == font) {
      batch = &renderer->batches[i];
      break;
    }
  }

  if (!batch) {
    batch = debug_text_create_batch(renderer, font, 100);
    if (!batch) {
      return NULL;
    }
  }

  if (batch->text_count >= batch->capacity) {
    LOG_ERROR("Batch capacity exceeded");
    return NULL;
  }

  Text3D *text_obj = &batch->texts[batch->text_count++];
  memset(text_obj, 0, sizeof(Text3D));

  strncpy(text_obj->text, text, sizeof(text_obj->text) - 1);
  text_obj->text[sizeof(text_obj->text) - 1] = '\0';

  text_obj->position = *position;
  text_obj->scale = (Vec3){1.0f, 1.0f, 1.0f};
  text_obj->color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
  text_obj->font = font;
  text_obj->style = TEXT_STYLE_NORMAL;
  text_obj->alignment = TEXT_ALIGN_LEFT;
  text_obj->anchor = TEXT_ANCHOR_MIDDLE_CENTER;
  text_obj->visible = true;
  text_obj->depth_test = true;
  text_obj->alpha = 1.0f;

  // Update batch bounds
  Vec3 min_bounds, max_bounds;
  calculate_text_bounds(text_obj, &min_bounds, &max_bounds);

  if (batch->text_count == 1) {
    batch->min_bounds = min_bounds;
    batch->max_bounds = max_bounds;
  } else {
    batch->min_bounds.x = fmin(batch->min_bounds.x, min_bounds.x);
    batch->min_bounds.y = fmin(batch->min_bounds.y, min_bounds.y);
    batch->min_bounds.z = fmin(batch->min_bounds.z, min_bounds.z);
    batch->max_bounds.x = fmax(batch->max_bounds.x, max_bounds.x);
    batch->max_bounds.y = fmax(batch->max_bounds.y, max_bounds.y);
    batch->max_bounds.z = fmax(batch->max_bounds.z, max_bounds.z);
  }

  batch->needs_update = true;

  LOG_DEBUG("Created 3D text: %s", text);

  if (renderer->on_text_created) {
    renderer->on_text_created(text_obj);
  }

  return text_obj;
}

bool debug_text_update_text(Debug3DTextRenderer *renderer, Text3D *text,
                            const char *new_text) {
  if (!renderer || !text || !new_text) {
    return false;
  }

  strncpy(text->text, new_text, sizeof(text->text) - 1);
  text->text[sizeof(text->text) - 1] = '\0';

  // Find the batch and mark it for update
  for (u32 i = 0; i < renderer->batch_count; i++) {
    TextBatch *batch = &renderer->batches[i];
    for (u32 j = 0; j < batch->text_count; j++) {
      if (&batch->texts[j] == text) {
        batch->needs_update = true;
        return true;
      }
    }
  }

  return false;
}

// MARK: - Text Batching

TextBatch *debug_text_create_batch(Debug3DTextRenderer *renderer,
                                   FontMetrics *font, u32 capacity) {
  if (!renderer || !font || capacity == 0) {
    return NULL;
  }

  if (renderer->batch_count >= renderer->batch_capacity) {
    LOG_ERROR("Batch capacity exceeded");
    return NULL;
  }

  TextBatch *batch = &renderer->batches[renderer->batch_count++];
  memset(batch, 0, sizeof(TextBatch));

  batch->texts = (Text3D *)calloc(capacity, sizeof(Text3D));
  if (!batch->texts) {
    LOG_ERROR("Failed to allocate batch texts");
    renderer->batch_count--;
    return NULL;
  }

  batch->capacity = capacity;
  batch->font = font;
  batch->needs_update = true;

  // Create OpenGL buffers
  glGenBuffers(1, &batch->vertex_buffer);
  glGenBuffers(1, &batch->index_buffer);

  LOG_DEBUG("Created text batch (capacity: %u)", capacity);
  return batch;
}

// MARK: - Rendering

void debug_text_render(Debug3DTextRenderer *renderer, const Mat4 *view_matrix,
                       const Mat4 *projection_matrix) {
  if (!renderer || !view_matrix || !projection_matrix) {
    return;
  }

  renderer->rendered_texts_last_frame = 0;
  renderer->total_vertices = 0;
  renderer->total_indices = 0;

  // Enable blending for text transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (renderer->enable_depth_testing) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  // Use shader program
  glUseProgram(renderer->shader_program);

  // Set global uniforms
  GLint view_loc =
      glGetUniformLocation(renderer->shader_program, "uViewMatrix");
  GLint proj_loc =
      glGetUniformLocation(renderer->shader_program, "uProjectionMatrix");
  GLint color_loc =
      glGetUniformLocation(renderer->shader_program, "uGlobalColor");
  GLint alpha_loc =
      glGetUniformLocation(renderer->shader_program, "uGlobalAlpha");

  glUniformMatrix4fv(view_loc, 1, GL_FALSE, (const float *)view_matrix);
  glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (const float *)projection_matrix);
  glUniform4f(color_loc, renderer->global_color.x, renderer->global_color.y,
              renderer->global_color.z, renderer->global_color.w);
  glUniform1f(alpha_loc, renderer->global_alpha);

  // Bind texture atlas
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, renderer->texture_atlas);
  GLint texture_loc =
      glGetUniformLocation(renderer->shader_program, "uTextureAtlas");
  glUniform1i(texture_loc, 0);

  // Render each batch
  for (u32 i = 0; i < renderer->batch_count; i++) {
    TextBatch *batch = &renderer->batches[i];
    if (batch->text_count > 0) {
      debug_text_render_batch(renderer, batch, view_matrix, projection_matrix);
    }
  }

  // Restore state
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
}

void debug_text_render_batch(Debug3DTextRenderer *renderer, TextBatch *batch,
                             const Mat4 *view_matrix,
                             const Mat4 *projection_matrix) {
  if (!renderer || !batch || batch->text_count == 0) {
    return;
  }

  // Update batch if needed
  if (batch->needs_update) {
    debug_text_update_batch(renderer, batch);
  }

  // Bind vertex array
  glBindVertexArray(renderer->vertex_array);

  // Bind buffers
  glBindBuffer(GL_ARRAY_BUFFER, batch->vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer);

  // Set vertex attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9,
                        (void *)0); // Position
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 9,
                        (void *)(sizeof(float) * 3)); // TexCoord
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 9,
                        (void *)(sizeof(float) * 5)); // Color

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  // TODO(Jules): Implement instanced rendering for debug_text_render_batch to
  // reduce draw calls. Draw all visible texts in the batch
  u32 visible_count = 0;
  for (u32 i = 0; i < batch->text_count; i++) {
    Text3D *text = &batch->texts[i];
    if (text->visible) {
      visible_count++;

      // Set model matrix for this text
      Mat4 model_matrix = mat4_identity();
      model_matrix = mat4_translate(model_matrix, text->position);
      model_matrix = mat4_rotate_x(model_matrix, text->rotation.x);
      model_matrix = mat4_rotate_y(model_matrix, text->rotation.y);
      model_matrix = mat4_rotate_z(model_matrix, text->rotation.z);
      model_matrix = mat4_scale(model_matrix, text->scale);

      GLint model_loc =
          glGetUniformLocation(renderer->shader_program, "uModelMatrix");
      glUniformMatrix4fv(model_loc, 1, GL_FALSE, (const float *)&model_matrix);

      // Draw the text (simplified - would draw actual quads for each character)
      // For now, just count as rendered
      renderer->rendered_texts_last_frame++;
    }
  }

  renderer->total_vertices +=
      visible_count * 4; // Assuming 4 vertices per character
  renderer->total_indices +=
      visible_count * 6; // Assuming 6 indices per character (2 triangles)
}

bool debug_text_update_batch(Debug3DTextRenderer *renderer, TextBatch *batch) {
  if (!renderer || !batch) {
    return false;
  }

  // This would generate actual vertex data for all texts in the batch
  // For now, just mark as updated
  batch->needs_update = false;

  LOG_DEBUG("Updated text batch with %u texts", batch->text_count);
  return true;
}

// MARK: - Debug Drawing Helpers

Text3D *debug_text_draw_vector(Debug3DTextRenderer *renderer, const Vec3 *start,
                               const Vec3 *end, const char *label) {
  if (!renderer || !start || !end || !label) {
    return NULL;
  }

  // Calculate midpoint for text position
  Vec3 midpoint = {(start->x + end->x) * 0.5f, (start->y + end->y) * 0.5f,
                   (start->z + end->z) * 0.5f};

  return debug_text_create(renderer, label, &midpoint, renderer->default_font);
}

Text3D *debug_text_draw_point(Debug3DTextRenderer *renderer,
                              const Vec3 *position, const char *label) {
  if (!renderer || !position || !label) {
    return NULL;
  }

  Vec3 text_pos = {position->x, position->y + 0.5f, position->z};
  return debug_text_create(renderer, label, &text_pos, renderer->default_font);
}

Text3D *debug_text_draw_coordinate_system(Debug3DTextRenderer *renderer,
                                          const Vec3 *position, float scale) {
  if (!renderer || !position) {
    return NULL;
  }

  // Draw axis labels
  Vec3 x_pos = {position->x + scale, position->y, position->z};
  Vec3 y_pos = {position->x, position->y + scale, position->z};
  Vec3 z_pos = {position->x, position->y, position->z + scale};

  debug_text_create(renderer, "X", &x_pos, renderer->default_font);
  debug_text_create(renderer, "Y", &y_pos, renderer->default_font);
  debug_text_create(renderer, "Z", &z_pos, renderer->default_font);

  return NULL; // Return NULL since we created multiple texts
}

// MARK: - Platform-specific implementations (simplified)

bool platform_load_system_font(const char *font_name, u32 font_size,
                               u8 **atlas_data, u32 *atlas_width,
                               u32 *atlas_height) {
  // This is a simplified implementation
  // In a real system, you would use Core Text on macOS or similar APIs

  *atlas_width = 256;
  *atlas_height = 256;

  *atlas_data = (u8 *)calloc((*atlas_width) * (*atlas_height) * 4, sizeof(u8));
  if (!*atlas_data) {
    return false;
  }

  // Generate a simple checkerboard pattern as placeholder
  for (u32 y = 0; y < *atlas_height; y++) {
    for (u32 x = 0; x < *atlas_width; x++) {
      u32 index = (y * (*atlas_width) + x) * 4;
      u8 value = ((x / 16 + y / 16) % 2) * 255;
      (*atlas_data)[index] = value;     // R
      (*atlas_data)[index + 1] = value; // G
      (*atlas_data)[index + 2] = value; // B
      (*atlas_data)[index + 3] = 255;   // A
    }
  }

  return true;
}

// MARK: - Utility Functions

void debug_text_get_statistics(Debug3DTextRenderer *renderer, u32 *total_texts,
                               u32 *visible_texts, u32 *total_vertices,
                               u32 *total_batches) {
  if (!renderer) {
    return;
  }

  if (total_texts) {
    *total_texts = 0;
    for (u32 i = 0; i < renderer->batch_count; i++) {
      *total_texts += renderer->batches[i].text_count;
    }
  }

  if (visible_texts) {
    *visible_texts = renderer->rendered_texts_last_frame;
  }

  if (total_vertices) {
    *total_vertices = renderer->total_vertices;
  }

  if (total_batches) {
    *total_batches = renderer->batch_count;
  }
}

void debug_text_print_statistics(Debug3DTextRenderer *renderer) {
  if (!renderer) {
    printf("Debug text renderer is NULL\n");
    return;
  }

  u32 total_texts, visible_texts, total_vertices, total_batches;
  debug_text_get_statistics(renderer, &total_texts, &visible_texts,
                            &total_vertices, &total_batches);

  printf("=== 3D TEXT RENDERER STATISTICS ===\n");
  printf("Total Texts: %u\n", total_texts);
  printf("Visible Texts: %u\n", visible_texts);
  printf("Total Vertices: %u\n", total_vertices);
  printf("Total Batches: %u\n", total_batches);
  printf("Fonts Loaded: %u\n", renderer->font_count);
  printf("Depth Testing: %s\n",
         renderer->enable_depth_testing ? "Enabled" : "Disabled");
  printf("Global Alpha: %.2f\n", renderer->global_alpha);
  printf("===================================\n");
}
