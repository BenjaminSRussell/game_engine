// svg_test.c - SVG Import Test Utility
#include "svg_importer.h"
#include <core/logger.h>
#include <stdio.h>
#include <stdlib.h>

void svg_test_basic_import(void) {
  LOG_INFO("=== SVG Basic Import Test ===");
  
  SVGImporter *importer = svg_importer_create();
  if (!importer) {
    LOG_ERROR("Failed to create SVG importer");
    return;
  }
  
  // Create a simple SVG document
  SVGDocument *doc = svg_document_create("test_svg");
  if (!doc) {
    LOG_ERROR("Failed to create SVG document");
    svg_importer_destroy(importer);
    return;
  }
  
  // Add a rectangle
  SVGElement *rect = svg_document_add_element(doc, SVG_ELEMENT_RECT);
  if (rect) {
    svg_element_set_position(rect, (Vec2){100.0f, 100.0f});
    svg_element_set_size(rect, (Vec2){200.0f, 150.0f});
    svg_element_set_fill_color(rect, (Vec4){1.0f, 0.0f, 0.0f, 1.0f});
    svg_element_set_stroke_color(rect, (Vec4){0.0f, 0.0f, 0.0f, 1.0f});
    svg_element_set_stroke_width(rect, 2.0f);
    LOG_INFO("Added rectangle to SVG");
  }
  
  // Add a circle
  SVGElement *circle = svg_document_add_element(doc, SVG_ELEMENT_CIRCLE);
  if (circle) {
    svg_element_set_position(circle, (Vec2){400.0f, 200.0f});
    circle->radius = (Vec2){50.0f, 50.0f};
    svg_element_set_fill_color(circle, (Vec4){0.0f, 0.0f, 1.0f, 1.0f});
    svg_element_set_stroke_color(circle, (Vec4){0.0f, 0.0f, 0.0f, 1.0f});
    svg_element_set_stroke_width(circle, 1.0f);
    LOG_INFO("Added circle to SVG");
  }
  
  // Add a path
  SVGElement *path = svg_document_add_element(doc, SVG_ELEMENT_PATH);
  if (path) {
    svg_element_set_position(path, (Vec2){100.0f, 300.0f});
    
    // Create a simple triangle path
    Vec2 points[3] = {{0.0f, 0.0f}, {100.0f, 0.0f}, {50.0f, 86.6f}};
    svg_element_add_path_segment(path, SVG_PATH_MOVE_TO, &points[0], 1);
    svg_element_add_path_segment(path, SVG_PATH_LINE_TO, &points[1], 1);
    svg_element_add_path_segment(path, SVG_PATH_LINE_TO, &points[2], 1);
    svg_element_add_path_segment(path, SVG_PATH_CLOSE_PATH, NULL, 0);
    
    svg_element_set_fill_color(path, (Vec4){0.0f, 1.0f, 0.0f, 0.5f});
    svg_element_set_stroke_color(path, (Vec4){0.0f, 0.5f, 0.0f, 1.0f});
    svg_element_set_stroke_width(path, 1.0f);
    LOG_INFO("Added triangle path to SVG");
  }
  
  // Print document info
  svg_document_print_info(doc);
  
  // Validate document
  if (svg_validate_document(doc)) {
    LOG_INFO("SVG document validation passed");
  } else {
    LOG_ERROR("SVG document validation failed");
  }
  
  // Test mesh conversion
  Vec3 vertices[1024];
  u32 vertex_count = 0;
  u32 indices[2048];
  u32 index_count = 0;
  
  if (svg_convert_to_mesh(doc, vertices, &vertex_count, indices, &index_count)) {
    LOG_INFO("SVG to mesh conversion successful: %u vertices, %u indices", vertex_count, index_count);
    
    // Print first few vertices for verification
    for (u32 i = 0; i < vertex_count && i < 10; i++) {
      LOG_DEBUG("Vertex %u: (%.2f, %.2f, %.2f)", i, vertices[i].x, vertices[i].y, vertices[i].z);
    }
  } else {
    LOG_ERROR("SVG to mesh conversion failed");
  }
  
  // Test 2D path conversion
  Vec2 points[512];
  u32 point_count = 0;
  
  if (svg_convert_to_2d_paths(doc, points, &point_count)) {
    LOG_INFO("SVG to 2D paths conversion successful: %u points", point_count);
  } else {
    LOG_ERROR("SVG to 2D paths conversion failed");
  }
  
  // Save to file
  if (svg_importer_save_file(importer, doc, "test_output.svg")) {
    LOG_INFO("SVG saved to test_output.svg");
  } else {
    LOG_ERROR("Failed to save SVG file");
  }
  
  svg_document_destroy(doc);
  svg_importer_destroy(importer);
  
  LOG_INFO("=== SVG Basic Import Test Complete ===");
}

void svg_test_color_parsing(void) {
  LOG_INFO("=== SVG Color Parsing Test ===");
  
  // Test hex colors
  Vec4 color1 = svg_parse_color("#FF0000");
  LOG_INFO("Parsed #FF0000: R=%.2f, G=%.2f, B=%.2f, A=%.2f", color1.x, color1.y, color1.z, color1.w);
  
  Vec4 color2 = svg_parse_color("#0F0");
  LOG_INFO("Parsed #0F0: R=%.2f, G=%.2f, B=%.2f, A=%.2f", color2.x, color2.y, color2.z, color2.w);
  
  // Test named colors
  Vec4 color3 = svg_parse_color("red");
  LOG_INFO("Parsed 'red': R=%.2f, G=%.2f, B=%.2f, A=%.2f", color3.x, color3.y, color3.z, color3.w);
  
  Vec4 color4 = svg_parse_color("blue");
  LOG_INFO("Parsed 'blue': R=%.2f, G=%.2f, B=%.2f, A=%.2f", color4.x, color4.y, color4.z, color4.w);
  
  Vec4 color5 = svg_parse_color("invalid_color");
  LOG_INFO("Parsed 'invalid_color': R=%.2f, G=%.2f, B=%.2f, A=%.2f", color5.x, color5.y, color5.z, color5.w);
  
  LOG_INFO("=== SVG Color Parsing Test Complete ===");
}

void svg_test_length_parsing(void) {
  LOG_INFO("=== SVG Length Parsing Test ===");
  
  f32 len1 = svg_parse_length("100px");
  LOG_INFO("Parsed '100px': %.2f", len1);
  
  f32 len2 = svg_parse_length("50pt");
  LOG_INFO("Parsed '50pt': %.2f", len2);
  
  f32 len3 = svg_parse_length("2em");
  LOG_INFO("Parsed '2em': %.2f", len3);
  
  f32 len4 = svg_parse_length("50%");
  LOG_INFO("Parsed '50%%: %.2f", len4);
  
  f32 len5 = svg_parse_length("75"); // No unit, assume pixels
  LOG_INFO("Parsed '75': %.2f", len5);
  
  LOG_INFO("=== SVG Length Parsing Test Complete ===");
}

void svg_test_transform_matrix(void) {
  LOG_INFO("=== SVG Transform Matrix Test ===");
  
  // Test translation
  Mat4 transform1 = svg_create_transform_matrix((Vec2){100.0f, 50.0f}, (Vec2){1.0f, 1.0f}, 0.0f);
  Vec2 test_point1 = {10.0f, 20.0f};
  Vec2 transformed1 = svg_transform_point(&transform1, test_point1);
  LOG_INFO("Translation test: (%.2f, %.2f) -> (%.2f, %.2f)", 
           test_point1.x, test_point1.y, transformed1.x, transformed1.y);
  
  // Test scaling
  Mat4 transform2 = svg_create_transform_matrix((Vec2){0.0f, 0.0f}, (Vec2){2.0f, 0.5f}, 0.0f);
  Vec2 test_point2 = {10.0f, 20.0f};
  Vec2 transformed2 = svg_transform_point(&transform2, test_point2);
  LOG_INFO("Scaling test: (%.2f, %.2f) -> (%.2f, %.2f)", 
           test_point2.x, test_point2.y, transformed2.x, transformed2.y);
  
  // Test rotation
  Mat4 transform3 = svg_create_transform_matrix((Vec2){0.0f, 0.0f}, (Vec2){1.0f, 1.0f}, M_PI / 4.0f);
  Vec2 test_point3 = {10.0f, 0.0f};
  Vec2 transformed3 = svg_transform_point(&transform3, test_point3);
  LOG_INFO("Rotation test (45°): (%.2f, %.2f) -> (%.2f, %.2f)", 
           test_point3.x, test_point3.y, transformed3.x, transformed3.y);
  
  // Test combined transform
  Mat4 transform4 = svg_create_transform_matrix((Vec2){50.0f, 25.0f}, (Vec2){1.5f, 1.5f}, M_PI / 6.0f);
  Vec2 test_point4 = {20.0f, 30.0f};
  Vec2 transformed4 = svg_transform_point(&transform4, test_point4);
  LOG_INFO("Combined test: (%.2f, %.2f) -> (%.2f, %.2f)", 
           test_point4.x, test_point4.y, transformed4.x, transformed4.y);
  
  LOG_INFO("=== SVG Transform Matrix Test Complete ===");
}

void svg_test_complex_shapes(void) {
  LOG_INFO("=== SVG Complex Shapes Test ===");
  
  SVGImporter *importer = svg_importer_create();
  if (!importer) {
    LOG_ERROR("Failed to create SVG importer");
    return;
  }
  
  SVGDocument *doc = svg_document_create("complex_shapes");
  if (!doc) {
    LOG_ERROR("Failed to create SVG document");
    svg_importer_destroy(importer);
    return;
  }
  
  // Create a star shape using path
  SVGElement *star = svg_document_add_element(doc, SVG_ELEMENT_PATH);
  if (star) {
    svg_element_set_position(star, (Vec2){300.0f, 300.0f});
    
    // Create a 5-pointed star
    const int points = 5;
    const f32 outer_radius = 80.0f;
    const f32 inner_radius = 30.0f;
    
    Vec2 star_points[10];
    for (int i = 0; i < points * 2; i++) {
      f32 angle = (i * M_PI) / points;
      f32 radius = (i % 2 == 0) ? outer_radius : inner_radius;
      star_points[i] = (Vec2){cosf(angle) * radius, sinf(angle) * radius};
    }
    
    // Draw the star
    for (int i = 0; i < points * 2; i++) {
      int next = (i + 1) % (points * 2);
      if (i == 0) {
        svg_element_add_path_segment(star, SVG_PATH_MOVE_TO, &star_points[i], 1);
      } else {
        svg_element_add_path_segment(star, SVG_PATH_LINE_TO, &star_points[i], 1);
      }
    }
    svg_element_add_path_segment(star, SVG_PATH_CLOSE_PATH, NULL, 0);
    
    svg_element_set_fill_color(star, (Vec4){1.0f, 0.8f, 0.0f, 1.0f});
    svg_element_set_stroke_color(star, (Vec4){0.5f, 0.3f, 0.0f, 1.0f});
    svg_element_set_stroke_width(star, 2.0f);
    
    LOG_INFO("Added 5-pointed star to SVG");
  }
  
  // Create a hexagon
  SVGElement *hexagon = svg_document_add_element(doc, SVG_ELEMENT_PATH);
  if (hexagon) {
    svg_element_set_position(hexagon, (Vec2){500.0f, 300.0f});
    
    const int sides = 6;
    const f32 radius = 60.0f;
    Vec2 hex_points[6];
    
    for (int i = 0; i < sides; i++) {
      f32 angle = (i * 2.0f * M_PI) / sides;
      hex_points[i] = (Vec2){cosf(angle) * radius, sinf(angle) * radius};
    }
    
    for (int i = 0; i < sides; i++) {
      int next = (i + 1) % sides;
      if (i == 0) {
        svg_element_add_path_segment(hexagon, SVG_PATH_MOVE_TO, &hex_points[i], 1);
      } else {
        svg_element_add_path_segment(hexagon, SVG_PATH_LINE_TO, &hex_points[i], 1);
      }
    }
    svg_element_add_path_segment(hexagon, SVG_PATH_CLOSE_PATH, NULL, 0);
    
    svg_element_set_fill_color(hexagon, (Vec4){0.0f, 0.8f, 0.8f, 0.7f});
    svg_element_set_stroke_color(hexagon, (Vec4){0.0f, 0.4f, 0.4f, 1.0f});
    svg_element_set_stroke_width(hexagon, 2.0f);
    
    LOG_INFO("Added hexagon to SVG");
  }
  
  // Add some curves
  SVGElement *curves = svg_document_add_element(doc, SVG_ELEMENT_PATH);
  if (curves) {
    svg_element_set_position(curves, (Vec2){100.0f, 500.0f});
    
    // Create a bezier curve
    Vec2 start = {0.0f, 0.0f};
    Vec2 control1 = {50.0f, -50.0f};
    Vec2 control2 = {100.0f, -50.0f};
    Vec2 end = {150.0f, 0.0f};
    
    svg_element_add_path_segment(curves, SVG_PATH_MOVE_TO, &start, 1);
    svg_element_add_path_segment(curves, SVG_PATH_CURVE_TO, &control1, 3);
    
    // Add another curve
    Vec2 start2 = {150.0f, 0.0f};
    Vec2 control2_2 = {200.0f, 50.0f};
    Vec2 control3 = {250.0f, 50.0f};
    Vec2 end2 = {300.0f, 0.0f};
    
    svg_element_add_path_segment(curves, SVG_PATH_CURVE_TO, &control2_2, 3);
    
    svg_element_set_stroke_color(curves, (Vec4){1.0f, 0.0f, 1.0f, 1.0f});
    svg_element_set_stroke_width(curves, 3.0f);
    
    LOG_INFO("Added bezier curves to SVG");
  }
  
  // Save complex shapes
  if (svg_importer_save_file(importer, doc, "complex_shapes.svg")) {
    LOG_INFO("Complex shapes SVG saved to complex_shapes.svg");
  }
  
  svg_document_destroy(doc);
  svg_importer_destroy(importer);
  
  LOG_INFO("=== SVG Complex Shapes Test Complete ===");
}

int main(void) {
  LOG_INFO("Starting SVG Import Tests");
  
  // Initialize SVG importer
  if (!svg_importer_init()) {
    LOG_ERROR("Failed to initialize SVG importer");
    return 1;
  }
  
  // Run tests
  svg_test_basic_import();
  svg_test_color_parsing();
  svg_test_length_parsing();
  svg_test_transform_matrix();
  svg_test_complex_shapes();
  
  // Cleanup
  svg_importer_shutdown();
  
  LOG_INFO("All SVG tests completed successfully");
  return 0;
}
