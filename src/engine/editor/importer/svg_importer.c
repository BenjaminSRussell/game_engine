// svg_importer.c - SVG Vector Graphics Importer Implementation
#include "svg_importer.h"
#include <core/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

static SVGImporter *g_svg_importer = NULL;

bool svg_importer_init(void) {
  if (g_svg_importer) {
    LOG_WARN("SVG importer already initialized");
    return true;
  }
  
  g_svg_importer = (SVGImporter*)calloc(1, sizeof(SVGImporter));
  if (!g_svg_importer) {
    LOG_ERROR("Failed to allocate SVG importer");
    return false;
  }
  
  LOG_INFO("SVG importer initialized");
  return true;
}

void svg_importer_shutdown(void) {
  if (!g_svg_importer) return;
  
  svg_importer_destroy(g_svg_importer);
  g_svg_importer = NULL;
  
  LOG_INFO("SVG importer shutdown");
}

SVGImporter* svg_importer_create(void) {
  SVGImporter *importer = (SVGImporter*)calloc(1, sizeof(SVGImporter));
  if (!importer) {
    LOG_ERROR("Failed to create SVG importer");
    return NULL;
  }
  
  return importer;
}

void svg_importer_destroy(SVGImporter *importer) {
  if (!importer) return;
  
  // Free all documents
  for (u32 i = 0; i < importer->document_count; i++) {
    svg_document_destroy(importer->documents[i]);
  }
  
  if (importer->current_buffer) {
    free(importer->current_buffer);
  }
  
  free(importer);
}

SVGDocument* svg_importer_load_file(SVGImporter *importer, const char *filepath) {
  if (!importer || !filepath) {
    LOG_ERROR("Invalid parameters for SVG file loading");
    return NULL;
  }
  
  FILE *file = fopen(filepath, "r");
  if (!file) {
    LOG_ERROR("Failed to open SVG file: %s", filepath);
    return NULL;
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  if (file_size <= 0) {
    LOG_ERROR("Invalid SVG file size: %ld", file_size);
    fclose(file);
    return NULL;
  }
  
  // Read file content
  char *buffer = (char*)malloc(file_size + 1);
  if (!buffer) {
    LOG_ERROR("Failed to allocate buffer for SVG file");
    fclose(file);
    return NULL;
  }
  
  size_t bytes_read = fread(buffer, 1, file_size, file);
  fclose(file);
  
  if (bytes_read != (size_t)file_size) {
    LOG_ERROR("Failed to read complete SVG file");
    free(buffer);
    return NULL;
  }
  
  buffer[file_size] = '\0';
  
  SVGDocument *doc = svg_importer_load_memory(importer, buffer, (u32)file_size);
  free(buffer);
  
  return doc;
}

SVGDocument* svg_importer_load_memory(SVGImporter *importer, const char *data, u32 size) {
  if (!importer || !data || size == 0) {
    LOG_ERROR("Invalid parameters for SVG memory loading");
    return NULL;
  }
  
  // Store buffer for parsing
  if (importer->current_buffer) {
    free(importer->current_buffer);
  }
  
  importer->current_buffer = (char*)malloc(size + 1);
  if (!importer->current_buffer) {
    LOG_ERROR("Failed to allocate buffer for SVG data");
    return NULL;
  }
  
  memcpy(importer->current_buffer, data, size);
  importer->current_buffer[size] = '\0';
  importer->buffer_size = size;
  importer->current_position = 0;
  
  // Create new document
  SVGDocument *doc = svg_document_create("imported_svg");
  if (!doc) {
    LOG_ERROR("Failed to create SVG document");
    return NULL;
  }
  
  // Parse SVG content (simplified parsing)
  if (!svg_parse_svg_content(importer, doc)) {
    LOG_ERROR("Failed to parse SVG content");
    svg_document_destroy(doc);
    return NULL;
  }
  
  // Add to importer's document list
  if (importer->document_count < MAX_SVG_PATHS) {
    importer->documents[importer->document_count++] = doc;
  }
  
  LOG_INFO("Successfully loaded SVG document with %u elements", doc->element_count);
  return doc;
}

bool svg_importer_save_file(SVGImporter *importer, const SVGDocument *doc, const char *filepath) {
  if (!importer || !doc || !filepath) {
    LOG_ERROR("Invalid parameters for SVG file saving");
    return false;
  }
  
  FILE *file = fopen(filepath, "w");
  if (!file) {
    LOG_ERROR("Failed to open SVG file for writing: %s", filepath);
    return false;
  }
  
  // Write SVG header
  fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(file, "<svg width=\"%.2f\" height=\"%.2f\" viewBox=\"%.2f %.2f %.2f %.2f\" xmlns=\"http://www.w3.org/2000/svg\">\n",
          doc->viewport_size.x, doc->viewport_size.y,
          doc->viewbox_min.x, doc->viewbox_min.y,
          doc->viewbox_max.x - doc->viewbox_min.x,
          doc->viewbox_max.y - doc->viewbox_min.y);
  
  // Write elements
  for (u32 i = 0; i < doc->element_count; i++) {
    SVGElement *element = &doc->elements[i];
    svg_write_element_to_file(file, element);
  }
  
  // Write SVG footer
  fprintf(file, "</svg>\n");
  
  fclose(file);
  LOG_INFO("SVG document saved to: %s", filepath);
  return true;
}

SVGDocument* svg_document_create(const char *id) {
  SVGDocument *doc = (SVGDocument*)calloc(1, sizeof(SVGDocument));
  if (!doc) {
    LOG_ERROR("Failed to create SVG document");
    return NULL;
  }
  
  if (id) {
    strncpy(doc->id, id, sizeof(doc->id) - 1);
  }
  
  // Set default viewport
  doc->viewport_size = (Vec2){800.0f, 600.0f};
  doc->viewbox_min = (Vec2){0.0f, 0.0f};
  doc->viewbox_max = doc->viewport_size;
  
  // Set default colors
  doc->default_fill_color = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
  doc->default_stroke_color = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
  doc->default_stroke_width = 1.0f;
  
  return doc;
}

void svg_document_destroy(SVGDocument *doc) {
  if (!doc) return;
  free(doc);
}

SVGElement* svg_document_add_element(SVGDocument *doc, SVGElementType type) {
  if (!doc || doc->element_count >= MAX_SVG_ELEMENTS) {
    return NULL;
  }
  
  SVGElement *element = &doc->elements[doc->element_count++];
  memset(element, 0, sizeof(SVGElement));
  
  element->type = type;
  element->scale = (Vec2){1.0f, 1.0f};
  element->transform = mat4_identity();
  
  // Set default style from document
  element->fill_color = doc->default_fill_color;
  element->stroke_color = doc->default_stroke_color;
  element->stroke_width = doc->default_stroke_width;
  element->has_fill = (doc->default_fill_color.w > 0.0f);
  element->has_stroke = (doc->default_stroke_color.w > 0.0f);
  
  return element;
}

void svg_element_set_position(SVGElement *element, Vec2 position) {
  if (!element) return;
  element->position = position;
}

void svg_element_set_size(SVGElement *element, Vec2 size) {
  if (!element) return;
  element->size = size;
}

void svg_element_set_fill_color(SVGElement *element, Vec4 color) {
  if (!element) return;
  element->fill_color = color;
  element->has_fill = (color.w > 0.0f);
}

void svg_element_set_stroke_color(SVGElement *element, Vec4 color) {
  if (!element) return;
  element->stroke_color = color;
  element->has_stroke = (color.w > 0.0f);
}

void svg_element_set_stroke_width(SVGElement *element, f32 width) {
  if (!element) return;
  element->stroke_width = width;
}

void svg_element_add_path_segment(SVGElement *element, SVGPathCommand command, 
                                  const Vec2 *points, u32 point_count) {
  if (!element || !points || point_count == 0 || element->segment_count >= MAX_SVG_PATH_SEGMENTS) {
    return;
  }
  
  SVGPathSegment *segment = &element->segments[element->segment_count++];
  segment->command = command;
  segment->point_count = point_count;
  
  for (u32 i = 0; i < point_count && i < 3; i++) {
    segment->points[i] = points[i];
  }
}

bool svg_convert_to_mesh(const SVGDocument *doc, Vec3 *out_vertices, u32 *out_vertex_count,
                         u32 *out_indices, u32 *out_index_count) {
  if (!doc || !out_vertices || !out_vertex_count) {
    return false;
  }
  
  u32 vertex_count = 0;
  u32 index_count = 0;
  
  // Convert each element to mesh vertices
  for (u32 elem_idx = 0; elem_idx < doc->element_count; elem_idx++) {
    SVGElement *element = &doc->elements[elem_idx];
    
    switch (element->type) {
      case SVG_ELEMENT_RECT:
        svg_convert_rect_to_mesh(element, out_vertices + vertex_count, &vertex_count,
                                out_indices ? out_indices + index_count : NULL, &index_count);
        break;
        
      case SVG_ELEMENT_CIRCLE:
        svg_convert_circle_to_mesh(element, out_vertices + vertex_count, &vertex_count,
                                  out_indices ? out_indices + index_count : NULL, &index_count);
        break;
        
      case SVG_ELEMENT_PATH:
        svg_convert_path_to_mesh(element, out_vertices + vertex_count, &vertex_count,
                                out_indices ? out_indices + index_count : NULL, &index_count);
        break;
        
      default:
        LOG_WARN("Unsupported SVG element type for mesh conversion: %d", element->type);
        break;
    }
  }
  
  if (out_vertex_count) *out_vertex_count = vertex_count;
  if (out_index_count) *out_index_count = index_count;
  
  LOG_INFO("Converted SVG to mesh: %u vertices, %u indices", vertex_count, index_count);
  return true;
}

bool svg_convert_to_2d_paths(const SVGDocument *doc, Vec2 *out_points, u32 *out_point_count) {
  if (!doc || !out_points || !out_point_count) {
    return false;
  }
  
  u32 point_count = 0;
  
  // Extract 2D points from all path elements
  for (u32 elem_idx = 0; elem_idx < doc->element_count; elem_idx++) {
    SVGElement *element = &doc->elements[elem_idx];
    
    if (element->type == SVG_ELEMENT_PATH) {
      for (u32 seg_idx = 0; seg_idx < element->segment_count; seg_idx++) {
        SVGPathSegment *segment = &element->segments[seg_idx];
        
        for (u32 point_idx = 0; point_idx < segment->point_count; point_idx++) {
          if (point_count < MAX_SVG_PATH_SEGMENTS * 3) {
            out_points[point_count++] = segment->points[point_idx];
          }
        }
      }
    }
  }
  
  *out_point_count = point_count;
  LOG_INFO("Extracted %u 2D points from SVG paths", point_count);
  return true;
}

Vec2 svg_transform_point(const Mat4 *transform, Vec2 point) {
  Vec4 point4 = (Vec4){point.x, point.y, 0.0f, 1.0f};
  Vec4 transformed = mat4_mul_vec4(*transform, point4);
  return (Vec2){transformed.x, transformed.y};
}

Mat4 svg_create_transform_matrix(Vec2 translation, Vec2 scale, f32 rotation) {
  Mat4 transform = mat4_identity();
  
  // Apply scale
  transform.m[0][0] = scale.x;
  transform.m[1][1] = scale.y;
  
  // Apply rotation (around Z axis)
  f32 cos_r = cosf(rotation);
  f32 sin_r = sinf(rotation);
  Mat4 rotation_matrix = {
    {{cos_r, -sin_r, 0.0f, 0.0f},
     {sin_r,  cos_r, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f, 0.0f},
     {0.0f, 0.0f, 0.0f, 1.0f}}
  };
  
  transform = mat4_mul(transform, rotation_matrix);
  
  // Apply translation
  transform.m[3][0] = translation.x;
  transform.m[3][1] = translation.y;
  
  return transform;
}

f32 svg_parse_length(const char *str) {
  if (!str) return 0.0f;
  
  // Simple parsing - handles px, pt, em, % units
  f32 value = 0.0f;
  char unit[8] = {0};
  
  if (sscanf(str, "%f%s", &value, unit) == 2) {
    if (strcmp(unit, "px") == 0) {
      return value;
    } else if (strcmp(unit, "pt") == 0) {
      return value * 1.333f; // 1pt = 1.333px
    } else if (strcmp(unit, "em") == 0) {
      return value * 16.0f; // 1em = 16px (default font size)
    } else if (strcmp(unit, "%") == 0) {
      return value / 100.0f * 800.0f; // Percentage of default width
    }
  }
  
  return value; // Assume pixels if no unit
}

Vec4 svg_parse_color(const char *str) {
  if (!str) return (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
  
  // Handle #RRGGBB format
  if (str[0] == '#' && strlen(str) == 7) {
    u32 color;
    sscanf(str + 1, "%x", &color);
    
    f32 r = ((color >> 16) & 0xFF) / 255.0f;
    f32 g = ((color >> 8) & 0xFF) / 255.0f;
    f32 b = (color & 0xFF) / 255.0f;
    
    return (Vec4){r, g, b, 1.0f};
  }
  
  // Handle #RGB format
  if (str[0] == '#' && strlen(str) == 4) {
    u32 color;
    sscanf(str + 1, "%x", &color);
    
    f32 r = ((color >> 8) & 0xF) / 15.0f;
    f32 g = ((color >> 4) & 0xF) / 15.0f;
    f32 b = (color & 0xF) / 15.0f;
    
    return (Vec4){r, g, b, 1.0f};
  }
  
  // Handle named colors (simplified)
  if (strcmp(str, "black") == 0) return (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
  if (strcmp(str, "white") == 0) return (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
  if (strcmp(str, "red") == 0) return (Vec4){1.0f, 0.0f, 0.0f, 1.0f};
  if (strcmp(str, "green") == 0) return (Vec4){0.0f, 1.0f, 0.0f, 1.0f};
  if (strcmp(str, "blue") == 0) return (Vec4){0.0f, 0.0f, 1.0f, 1.0f};
  
  return (Vec4){0.0f, 0.0f, 0.0f, 1.0f}; // Default to black
}

void svg_document_print_info(const SVGDocument *doc) {
  if (!doc) return;
  
  LOG_INFO("SVG Document: %s", doc->id);
  LOG_INFO("  Elements: %u", doc->element_count);
  LOG_INFO("  Viewport: %.2f x %.2f", doc->viewport_size.x, doc->viewport_size.y);
  LOG_INFO("  ViewBox: (%.2f, %.2f) to (%.2f, %.2f)", 
           doc->viewbox_min.x, doc->viewbox_min.y, doc->viewbox_max.x, doc->viewbox_max.y);
  
  for (u32 i = 0; i < doc->element_count; i++) {
    SVGElement *element = &doc->elements[i];
    LOG_INFO("  Element %u: type=%d, pos=(%.2f,%.2f), size=(%.2f,%.2f)", 
             i, element->type, element->position.x, element->position.y,
             element->size.x, element->size.y);
  }
}

bool svg_validate_document(const SVGDocument *doc) {
  if (!doc) return false;
  
  if (doc->element_count == 0) {
    LOG_WARN("SVG document has no elements");
    return false;
  }
  
  if (doc->viewport_size.x <= 0.0f || doc->viewport_size.y <= 0.0f) {
    LOG_WARN("SVG document has invalid viewport size");
    return false;
  }
  
  // Validate elements
  for (u32 i = 0; i < doc->element_count; i++) {
    SVGElement *element = &doc->elements[i];
    
    if (element->type == SVG_ELEMENT_PATH && element->segment_count == 0) {
      LOG_WARN("SVG path element %u has no segments", i);
      return false;
    }
  }
  
  return true;
}

// Internal helper functions
static bool svg_parse_svg_content(SVGImporter *importer, SVGDocument *doc) {
  // This is a simplified SVG parser - in a real implementation,
  // you'd use a proper XML parser like tinyxml2 or expat
  
  const char *content = importer->current_buffer;
  
  // Simple regex-like parsing for basic SVG elements
  // This is just a placeholder implementation
  
  // Parse viewport
  const char *width_str = strstr(content, "width=\"");
  const char *height_str = strstr(content, "height=\"");
  
  if (width_str && height_str) {
    doc->viewport_size.x = svg_parse_length(width_str + 7);
    doc->viewport_size.y = svg_parse_length(height_str + 8);
  }
  
  // Parse viewBox
  const char *viewbox_str = strstr(content, "viewBox=\"");
  if (viewbox_str) {
    f32 x, y, w, h;
    if (sscanf(viewbox_str + 9, "%f %f %f %f", &x, &y, &w, &h) == 4) {
      doc->viewbox_min = (Vec2){x, y};
      doc->viewbox_max = (Vec2){x + w, y + h};
    }
  }
  
  // TODO: Parse individual elements (rect, circle, path, etc.)
  // This would require a proper XML parser for robust implementation
  
  return true;
}

static void svg_convert_rect_to_mesh(const SVGElement *element, Vec3 *vertices, u32 *vertex_count,
                                     u32 *indices, u32 *index_count) {
  if (!element || !vertices) return;
  
  Vec2 corners[4] = {
    {element->position.x, element->position.y},
    {element->position.x + element->size.x, element->position.y},
    {element->position.x + element->size.x, element->position.y + element->size.y},
    {element->position.x, element->position.y + element->size.y}
  };
  
  // Convert to 3D vertices (Z = 0)
  for (int i = 0; i < 4; i++) {
    vertices[*vertex_count + i] = (Vec3){corners[i].x, corners[i].y, 0.0f};
  }
  *vertex_count += 4;
  
  // Create two triangles for the quad
  if (indices) {
    u32 base = *index_count;
    indices[base + 0] = base + 0;
    indices[base + 1] = base + 1;
    indices[base + 2] = base + 2;
    indices[base + 3] = base + 0;
    indices[base + 4] = base + 2;
    indices[base + 5] = base + 3;
    *index_count += 6;
  }
}

static void svg_convert_circle_to_mesh(const SVGElement *element, Vec3 *vertices, u32 *vertex_count,
                                       u32 *indices, u32 *index_count) {
  if (!element || !vertices) return;
  
  const int segments = 32;
  const f32 step = 2.0f * M_PI / segments;
  
  // Create circle vertices
  for (int i = 0; i < segments; i++) {
    f32 angle = i * step;
    f32 x = element->position.x + cosf(angle) * element->radius.x;
    f32 y = element->position.y + sinf(angle) * element->radius.y;
    vertices[*vertex_count + i] = (Vec3){x, y, 0.0f};
  }
  
  // Center vertex
  vertices[*vertex_count + segments] = (Vec3){element->position.x, element->position.y, 0.0f};
  *vertex_count += segments + 1;
  
  // Create triangle indices
  if (indices) {
    u32 center_idx = *vertex_count - 1;
    for (int i = 0; i < segments; i++) {
      u32 next = (i + 1) % segments;
      u32 base = *index_count;
      
      indices[base + 0] = center_idx;
      indices[base + 1] = center_idx - segments + i;
      indices[base + 2] = center_idx - segments + next;
      
      *index_count += 3;
    }
  }
}

static void svg_convert_path_to_mesh(const SVGElement *element, Vec3 *vertices, u32 *vertex_count,
                                     u32 *indices, u32 *index_count) {
  if (!element || !vertices) return;
  
  // Convert path segments to line segments
  Vec2 current_pos = {0.0f, 0.0f};
  
  for (u32 i = 0; i < element->segment_count; i++) {
    SVGPathSegment *segment = &element->segments[i];
    
    switch (segment->command) {
      case SVG_PATH_MOVE_TO:
        if (segment->point_count > 0) {
          current_pos = segment->points[0];
        }
        break;
        
      case SVG_PATH_LINE_TO:
        if (segment->point_count > 0 && *vertex_count + 1 < MAX_SVG_PATH_SEGMENTS * 3) {
          vertices[*vertex_count++] = (Vec3){current_pos.x, current_pos.y, 0.0f};
          vertices[*vertex_count++] = (Vec3){segment->points[0].x, segment->points[0].y, 0.0f};
          current_pos = segment->points[0];
        }
        break;
        
      default:
        // TODO: Handle curves and other path commands
        break;
    }
  }
}

static void svg_write_element_to_file(FILE *file, const SVGElement *element) {
  if (!file || !element) return;
  
  switch (element->type) {
    case SVG_ELEMENT_RECT:
      fprintf(file, "  <rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\"",
              element->position.x, element->position.y, element->size.x, element->size.y);
      break;
      
    case SVG_ELEMENT_CIRCLE:
      fprintf(file, "  <circle cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"",
              element->position.x, element->position.y, element->radius.x);
      break;
      
    default:
      fprintf(file, "  <!-- Unsupported element type: %d -->", element->type);
      return;
  }
  
  // Write style attributes
  if (element->has_fill) {
    fprintf(file, " fill=\"rgb(%d,%d,%d)\" fill-opacity=\"%.2f\"",
            (int)(element->fill_color.x * 255), (int)(element->fill_color.y * 255), 
            (int)(element->fill_color.z * 255), element->fill_color.w);
  }
  
  if (element->has_stroke) {
    fprintf(file, " stroke=\"rgb(%d,%d,%d)\" stroke-opacity=\"%.2f\" stroke-width=\"%.2f\"",
            (int)(element->stroke_color.x * 255), (int)(element->stroke_color.y * 255),
            (int)(element->stroke_color.z * 255), element->stroke_color.w, element->stroke_width);
  }
  
  fprintf(file, " />\n");
}
