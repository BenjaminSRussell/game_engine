// svg_importer.h - SVG Vector Graphics Importer
#ifndef SVG_IMPORTER_H
#define SVG_IMPORTER_H

#include <common.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat4.h>

#define MAX_SVG_PATHS 1024
#define MAX_SVG_ELEMENTS 4096
#define MAX_SVG_PATH_SEGMENTS 8192

typedef enum {
  SVG_ELEMENT_PATH = 0,
  SVG_ELEMENT_RECT,
  SVG_ELEMENT_CIRCLE,
  SVG_ELEMENT_ELLIPSE,
  SVG_ELEMENT_LINE,
  SVG_ELEMENT_POLYLINE,
  SVG_ELEMENT_POLYGON,
  SVG_ELEMENT_TEXT,
  SVG_ELEMENT_GROUP
} SVGElementType;

typedef enum {
  SVG_PATH_MOVE_TO = 0,
  SVG_PATH_LINE_TO,
  SVG_PATH_HORIZONTAL_LINE_TO,
  SVG_PATH_VERTICAL_LINE_TO,
  SVG_PATH_CURVE_TO,
  SVG_PATH_SMOOTH_CURVE_TO,
  SVG_PATH_QUAD_CURVE_TO,
  SVG_PATH_SMOOTH_QUAD_CURVE_TO,
  SVG_PATH_CLOSE_PATH
} SVGPathCommand;

typedef struct {
  SVGPathCommand command;
  Vec2 points[3]; // Up to 3 control points for curves
  u32 point_count;
} SVGPathSegment;

typedef struct {
  char id[64];
  SVGElementType type;
  Vec2 position;
  Vec2 size;
  Vec2 radius;
  f32 rotation;
  Vec2 scale;
  
  // Path data for path elements
  SVGPathSegment segments[MAX_SVG_PATH_SEGMENTS];
  u32 segment_count;
  
  // Style properties
  Vec4 fill_color;
  Vec4 stroke_color;
  f32 stroke_width;
  bool has_fill;
  bool has_stroke;
  
  // Transform
  Mat4 transform;
  
  // Text data for text elements
  char text[256];
  char font_family[64];
  f32 font_size;
} SVGElement;

typedef struct {
  char id[64];
  SVGElement elements[MAX_SVG_ELEMENTS];
  u32 element_count;
  
  Vec2 viewport_size;
  Vec2 viewbox_min;
  Vec2 viewbox_max;
  
  // Global style
  Vec4 default_fill_color;
  Vec4 default_stroke_color;
  f32 default_stroke_width;
} SVGDocument;

typedef struct {
  SVGDocument *documents[MAX_SVG_PATHS];
  u32 document_count;
  
  // Parsing state
  char *current_buffer;
  u32 buffer_size;
  u32 current_position;
} SVGImporter;

// Core functions
bool svg_importer_init(void);
void svg_importer_shutdown(void);
SVGImporter* svg_importer_create(void);
void svg_importer_destroy(SVGImporter *importer);

// File operations
SVGDocument* svg_importer_load_file(SVGImporter *importer, const char *filepath);
SVGDocument* svg_importer_load_memory(SVGImporter *importer, const char *data, u32 size);
bool svg_importer_save_file(SVGImporter *importer, const SVGDocument *doc, const char *filepath);

// Document operations
SVGDocument* svg_document_create(const char *id);
void svg_document_destroy(SVGDocument *doc);
SVGElement* svg_document_add_element(SVGDocument *doc, SVGElementType type);

// Element operations
void svg_element_set_position(SVGElement *element, Vec2 position);
void svg_element_set_size(SVGElement *element, Vec2 size);
void svg_element_set_fill_color(SVGElement *element, Vec4 color);
void svg_element_set_stroke_color(SVGElement *element, Vec4 color);
void svg_element_set_stroke_width(SVGElement *element, f32 width);
void svg_element_add_path_segment(SVGElement *element, SVGPathCommand command, 
                                  const Vec2 *points, u32 point_count);

// Conversion utilities
bool svg_convert_to_mesh(const SVGDocument *doc, Vec3 *out_vertices, u32 *out_vertex_count,
                         u32 *out_indices, u32 *out_index_count);
bool svg_convert_to_2d_paths(const SVGDocument *doc, Vec2 *out_points, u32 *out_point_count);
bool svg_convert_to_bezier_curves(const SVGDocument *doc, Vec2 *out_control_points, 
                                  u32 *out_control_point_count);

// Utility functions
Vec2 svg_transform_point(const Mat4 *transform, Vec2 point);
Mat4 svg_create_transform_matrix(Vec2 translation, Vec2 scale, f32 rotation);
f32 svg_parse_length(const char *str);
Vec4 svg_parse_color(const char *str);
bool svg_parse_path_data(const char *data, SVGPathSegment *segments, u32 max_segments, u32 *out_count);
bool svg_parse_svg_content(const char *content, SVGDocument *out_doc);
bool svg_write_element_to_file(const SVGElement *element, const char *filepath);
bool svg_convert_rect_to_mesh(const SVGElement *rect, Vec3 *out_vertices, u32 *out_vertex_count,
                           u32 *out_indices, u32 *out_index_count);

// Debug functions
void svg_document_print_info(const SVGDocument *doc);
void svg_element_print_info(const SVGElement *element);
bool svg_validate_document(const SVGDocument *doc);

#endif // SVG_IMPORTER_H
