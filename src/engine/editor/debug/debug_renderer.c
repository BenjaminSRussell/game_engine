#include "editor/debug/debug_renderer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Force ObjC syntax if compiling as C but needing Metal
// In a real project, build rules would handle this.
#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

debug_renderer_t *debug_renderer_create(id device_ptr, uint32_t max_lines) {
#ifdef __OBJC__
  id<MTLDevice> device = (id<MTLDevice>)device_ptr;
  debug_renderer_t *dbg =
      (debug_renderer_t *)calloc(1, sizeof(debug_renderer_t));
  if (!dbg)
    return NULL;

  dbg->max_lines = max_lines;
  dbg->line_count = 0;

  // Create Buffers
  dbg->line_buffer =
      [device newBufferWithLength:sizeof(debug_line_t) * max_lines
                          options:MTLResourceStorageModeShared];

  // Create Pipeline
  id<MTLLibrary> library = [device newDefaultLibrary];
  id<MTLFunction> vertex_fn = [library newFunctionWithName:@"vertex_debug"];
  id<MTLFunction> fragment_fn = [library newFunctionWithName:@"fragment_debug"];

  if (vertex_fn && fragment_fn) {
    MTLRenderPipelineDescriptor *desc =
        [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertex_fn;
    desc.fragmentFunction = fragment_fn;
    desc.colorAttachments[0].pixelFormat =
        MTLPixelFormatBGRA8Unorm; // Assumed swapchain format
    desc.colorAttachments[0].blendingEnabled = YES;
    desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor =
        MTLBlendFactorOneMinusSourceAlpha;
    desc.depthAttachmentPixelFormat =
        MTLPixelFormatDepth32Float; // Assumed depth format

    NSError *error = nil;
    dbg->line_pipeline =
        [device newRenderPipelineStateWithDescriptor:desc error:&error];
  }

  // Depth States
  MTLDepthStencilDescriptor *depth_desc =
      [[MTLDepthStencilDescriptor alloc] init];
  depth_desc.depthCompareFunction = MTLCompareFunctionLessEqual;
  depth_desc.depthWriteEnabled = NO; // Debug lines usually don't write depth
  dbg->depth_test_state =
      [device newDepthStencilStateWithDescriptor:depth_desc];

  depth_desc.depthCompareFunction = MTLCompareFunctionAlways;
  dbg->no_depth_state = [device newDepthStencilStateWithDescriptor:depth_desc];

  return dbg;
#else
  return NULL;
#endif
}

void debug_renderer_destroy(debug_renderer_t *dbg) {
  if (dbg)
    free(dbg);
}

void debug_draw_line(debug_renderer_t *dbg, simd_float3 start, simd_float3 end,
                     simd_float4 color) {
#ifdef __OBJC__
  if (!dbg || dbg->line_count >= dbg->max_lines)
    return;

  debug_line_t *lines =
      (debug_line_t *)[(id<MTLBuffer>)dbg->line_buffer contents];
  lines[dbg->line_count++] = (debug_line_t){
      .start = start, .start_color = color, .end = end, .end_color = color};
#endif
}

void debug_draw_aabb(debug_renderer_t *dbg, simd_float3 min, simd_float3 max,
                     simd_float4 color) {
  simd_float3 corners[8] = {
      simd_make_float3(min.x, min.y, min.z),
      simd_make_float3(max.x, min.y, min.z),
      simd_make_float3(max.x, max.y, min.z),
      simd_make_float3(min.x, max.y, min.z),
      simd_make_float3(min.x, min.y, max.z),
      simd_make_float3(max.x, min.y, max.z),
      simd_make_float3(max.x, max.y, max.z),
      simd_make_float3(min.x, max.y, max.z),
  };

  int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                      {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

  for (int i = 0; i < 12; i++) {
    debug_draw_line(dbg, corners[edges[i][0]], corners[edges[i][1]], color);
  }
}

void debug_draw_sphere(debug_renderer_t *dbg, simd_float3 center, float radius,
                       simd_float4 color) {
#ifdef __OBJC__
  const int segments = 16;
  const float step = 2.0f * 3.14159f / (float)segments;

  // Draw 3 circles
  for (int i = 0; i < segments; i++) {
    float theta1 = i * step;
    float theta2 = (i + 1) * step;

    float c1 = cos(theta1) * radius;
    float s1 = sin(theta1) * radius;
    float c2 = cos(theta2) * radius;
    float s2 = sin(theta2) * radius;

    // XY Plane
    debug_draw_line(dbg, center + simd_make_float3(c1, s1, 0),
                    center + simd_make_float3(c2, s2, 0), color);
    // XZ Plane
    debug_draw_line(dbg, center + simd_make_float3(c1, 0, s1),
                    center + simd_make_float3(c2, 0, s2), color);
    // YZ Plane
    debug_draw_line(dbg, center + simd_make_float3(0, c1, s1),
                    center + simd_make_float3(0, c2, s2), color);
  }
#endif
}

void debug_draw_frustum(debug_renderer_t *dbg, simd_float4x4 inv_view_proj,
                        simd_float4 color) {
#ifdef __OBJC__
  simd_float4 corners[8] = {
      simd_make_float4(-1, -1, 0, 1), simd_make_float4(1, -1, 0, 1),
      simd_make_float4(1, 1, 0, 1),   simd_make_float4(-1, 1, 0, 1),
      simd_make_float4(-1, -1, 1, 1), simd_make_float4(1, -1, 1, 1),
      simd_make_float4(1, 1, 1, 1),   simd_make_float4(-1, 1, 1, 1)};

  simd_float3 world_corners[8];
  for (int i = 0; i < 8; i++) {
    simd_float4 world = simd_mul(inv_view_proj, corners[i]);
    world_corners[i] = world.xyz / world.w;
  }

  // Near plane
  debug_draw_line(dbg, world_corners[0], world_corners[1], color);
  debug_draw_line(dbg, world_corners[1], world_corners[2], color);
  debug_draw_line(dbg, world_corners[2], world_corners[3], color);
  debug_draw_line(dbg, world_corners[3], world_corners[0], color);

  // Far plane
  debug_draw_line(dbg, world_corners[4], world_corners[5], color);
  debug_draw_line(dbg, world_corners[5], world_corners[6], color);
  debug_draw_line(dbg, world_corners[6], world_corners[7], color);
  debug_draw_line(dbg, world_corners[7], world_corners[4], color);

  // Connections
  debug_draw_line(dbg, world_corners[0], world_corners[4], color);
  debug_draw_line(dbg, world_corners[1], world_corners[5], color);
  debug_draw_line(dbg, world_corners[2], world_corners[6], color);
  debug_draw_line(dbg, world_corners[3], world_corners[7], color);
#endif
}

void debug_draw_grid(debug_renderer_t *dbg, simd_float3 center, float size,
                     int divisions, simd_float4 color) {
#ifdef __OBJC__
  if (!dbg || divisions <= 0)
    return;

  float step = size / (float)divisions;
  float half_size = size * 0.5f;

  // Draw grid on XZ plane (assume Y is up)
  for (int i = 0; i <= divisions; i++) {
    float offset = -half_size + (i * step);

    // Lines parallel to X axis
    simd_float3 start_x = center + simd_make_float3(-half_size, 0, offset);
    simd_float3 end_x = center + simd_make_float3(half_size, 0, offset);

    // Lines parallel to Z axis
    simd_float3 start_z = center + simd_make_float3(offset, 0, -half_size);
    simd_float3 end_z = center + simd_make_float3(offset, 0, half_size);

    // Dim the color for non-center lines
    simd_float4 line_color = color;
    if (i == divisions / 2) {
      line_color.w = 1.0f; // Full alpha for center lines
    } else {
      line_color.w = 0.3f; // Dimmer for other lines
    }

    debug_draw_line(dbg, start_x, end_x, line_color);
    debug_draw_line(dbg, start_z, end_z, line_color);
  }
#endif
}

// Forward declarations
static void debug_draw_quad(debug_renderer_t *dbg, simd_float3 bl,
                            simd_float3 br, simd_float3 tr, simd_float3 tl,
                            simd_float4 color);
static void debug_draw_text_outline(debug_renderer_t *dbg, simd_float3 position,
                                    const char *text, simd_float4 color);
static void debug_draw_char_A(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_B(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_C(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_D(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_E(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_F(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_G(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_H(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_I(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_L(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_N(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_O(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_R(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_S(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_T(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_U(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color);
static void debug_draw_char_dash(debug_renderer_t *dbg, simd_float3 pos,
                                 float size, simd_float3 up, simd_float3 right,
                                 simd_float4 color);
static void debug_draw_char_box(debug_renderer_t *dbg, simd_float3 pos,
                                float size, simd_float3 up, simd_float3 right,
                                simd_float4 color);

void debug_draw_text_3d(debug_renderer_t *dbg, simd_float3 position,
                        const char *text, simd_float4 color) {
#ifdef __OBJC__
  if (!dbg || !text)
    return;

  // Calculate text dimensions (approximate)
  const float char_width = 0.1f;
  const float char_height = 0.15f;
  const float char_spacing = 0.05f;

  size_t text_length = strlen(text);
  float total_width = text_length * (char_width + char_spacing) - char_spacing;

  // Create billboard quad for text background
  simd_float3 camera_pos = {0}; // This should come from the active camera
  simd_float3 to_camera = simd_normalize(camera_pos - position);
  simd_float3 right =
      simd_normalize(simd_cross(to_camera, (simd_float3){0, 1, 0}));
  simd_float3 up = simd_cross(right, to_camera);

  // Scale factors for the text quad
  float quad_width = total_width * 0.5f;
  float quad_height = char_height * 0.5f;

  // Calculate quad corners
  simd_float3 bottom_left = position - right * quad_width - up * quad_height;
  simd_float3 bottom_right = position + right * quad_width - up * quad_height;
  simd_float3 top_right = position + right * quad_width + up * quad_height;
  simd_float3 top_left = position - right * quad_width + up * quad_height;

  // Draw text background quad
  simd_float4 bg_color = {0.0f, 0.0f, 0.0f, 0.7f}; // Semi-transparent black
  debug_draw_quad(dbg, bottom_left, bottom_right, top_right, top_left,
                  bg_color);

  // Draw text outline
  simd_float4 outline_color = {1.0f, 1.0f, 1.0f, 0.9f};
  debug_draw_quad(dbg, bottom_left, bottom_right, top_right, top_left,
                  outline_color);

  // For each character, draw a simple representation
  simd_float3 char_pos =
      bottom_left + right * (char_width * 0.5f) + up * (char_height * 0.5f);

  for (size_t i = 0; i < text_length; i++) {
    // Draw a small sphere to represent each character
    debug_draw_sphere(dbg, char_pos, 0.02f, color);

    // Move to next character position
    char_pos = char_pos + right * (char_width + char_spacing);
  }

  // Alternative: Draw text as connected lines forming character outlines
  debug_draw_text_outline(dbg, position, text, color);
#endif
}

static void debug_draw_quad(debug_renderer_t *dbg, simd_float3 bl,
                            simd_float3 br, simd_float3 tr, simd_float3 tl,
                            simd_float4 color) {
#ifdef __OBJC__
  // Draw quad as two triangles using lines
  debug_draw_line(dbg, bl, br, color);
  debug_draw_line(dbg, br, tr, color);
  debug_draw_line(dbg, tr, tl, color);
  debug_draw_line(dbg, tl, bl, color);

  // Draw diagonal lines for better visibility
  debug_draw_line(dbg, bl, tr, color);
  debug_draw_line(dbg, br, tl, color);
#endif
}

static void debug_draw_text_outline(debug_renderer_t *dbg, simd_float3 position,
                                    const char *text, simd_float4 color) {
#ifdef __OBJC__
  if (!dbg || !text)
    return;

  // Simple bitmap-style text rendering using line segments
  const float char_size = 0.08f;
  const float char_spacing = 0.1f;

  simd_float3 camera_pos = {0}; // Should come from active camera
  simd_float3 to_camera = simd_normalize(camera_pos - position);
  simd_float3 right =
      simd_normalize(simd_cross(to_camera, (simd_float3){0, 1, 0}));
  simd_float3 up = simd_cross(right, to_camera);

  simd_float3 char_start =
      position - right * (strlen(text) * char_spacing * 0.5f);

  for (size_t i = 0; i < strlen(text); i++) {
    char c = text[i];
    simd_float3 char_pos = char_start + right * (i * char_spacing);

    // Draw simple character representations
    switch (c) {
    case 'A':
    case 'a':
      debug_draw_char_A(dbg, char_pos, char_size, up, right, color);
      break;
    case 'B':
    case 'b':
      debug_draw_char_B(dbg, char_pos, char_size, up, right, color);
      break;
    case 'C':
    case 'c':
      debug_draw_char_C(dbg, char_pos, char_size, up, right, color);
      break;
    case 'D':
    case 'd':
      debug_draw_char_D(dbg, char_pos, char_size, up, right, color);
      break;
    case 'E':
    case 'e':
      debug_draw_char_E(dbg, char_pos, char_size, up, right, color);
      break;
    case 'F':
    case 'f':
      debug_draw_char_F(dbg, char_pos, char_size, up, right, color);
      break;
    case 'G':
    case 'g':
      debug_draw_char_G(dbg, char_pos, char_size, up, right, color);
      break;
    case 'H':
    case 'h':
      debug_draw_char_H(dbg, char_pos, char_size, up, right, color);
      break;
    case 'I':
    case 'i':
      debug_draw_char_I(dbg, char_pos, char_size, up, right, color);
      break;
    case 'L':
    case 'l':
      debug_draw_char_L(dbg, char_pos, char_size, up, right, color);
      break;
    case 'N':
    case 'n':
      debug_draw_char_N(dbg, char_pos, char_size, up, right, color);
      break;
    case 'O':
    case 'o':
      debug_draw_char_O(dbg, char_pos, char_size, up, right, color);
      break;
    case 'R':
    case 'r':
      debug_draw_char_R(dbg, char_pos, char_size, up, right, color);
      break;
    case 'S':
    case 's':
      debug_draw_char_S(dbg, char_pos, char_size, up, right, color);
      break;
    case 'T':
    case 't':
      debug_draw_char_T(dbg, char_pos, char_size, up, right, color);
      break;
    case 'U':
    case 'u':
      debug_draw_char_U(dbg, char_pos, char_size, up, right, color);
      break;
    case '-':
      debug_draw_char_dash(dbg, char_pos, char_size, up, right, color);
      break;
    case ' ':
      // Skip spaces
      break;
    default:
      // Draw a box for unknown characters
      debug_draw_char_box(dbg, char_pos, char_size, up, right, color);
      break;
    }
  }
#endif
}

// Character drawing helper functions
static void debug_draw_char_A(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;
  simd_float3 middle = pos + up * size * 0.1f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  // Adjusted: Draw cross bar using two points
  debug_draw_line(dbg, left + right * size * 0.3f,
                  right_pt - right * size * 0.3f,
                  color); // Roughly horizontal bar
}

static void debug_draw_char_B(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;
  simd_float3 middle = pos;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, right_pt, middle, color);
  debug_draw_line(dbg, left, middle, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, bottom, right_pt, color);
}

static void debug_draw_char_C(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;

  debug_draw_line(dbg, right_pt, top, color);
  debug_draw_line(dbg, top, left, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, bottom, right_pt, color);
}

static void debug_draw_char_D(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.3f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  debug_draw_line(dbg, bottom, left, color);
}

static void debug_draw_char_E(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;
  simd_float3 middle = pos;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, left, middle, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, bottom, right_pt, color);
}

static void debug_draw_char_F(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;
  simd_float3 middle = pos;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, left, middle, color);
  debug_draw_line(dbg, left, bottom, color);
}

static void debug_draw_char_G(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;
  simd_float3 middle = pos;

  debug_draw_line(dbg, right_pt, top, color);
  debug_draw_line(dbg, top, left, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, bottom, right_pt, color);
  debug_draw_line(dbg, right_pt, middle, color);
  debug_draw_line(dbg, middle, middle + right * size * 0.3f, color);
}

static void debug_draw_char_H(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, right_pt, top, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  // Fixed: Connect middle
  debug_draw_line(dbg, left - up * 0.0f, right_pt - up * 0.0f, color);
}

static void debug_draw_char_I(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.2f;
  simd_float3 right_pt = pos + right * size * 0.2f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, right_pt, top, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  // Fixed: Vertical line
  debug_draw_line(dbg, top, bottom, color);
}

static void debug_draw_char_L(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, bottom, right_pt, color);
}

static void debug_draw_char_N(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, right_pt, top, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  debug_draw_line(dbg, left, right_pt, color);
}

static void debug_draw_char_O(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  debug_draw_line(dbg, bottom, left, color);
}

static void debug_draw_char_R(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;
  simd_float3 middle = pos;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, right_pt, middle, color);
  debug_draw_line(dbg, left, middle, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, middle, right_pt, color);
}

static void debug_draw_char_S(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;
  simd_float3 middle = pos;

  debug_draw_line(dbg, right_pt, top, color);
  debug_draw_line(dbg, top, left, color);
  debug_draw_line(dbg, left, middle, color);
  debug_draw_line(dbg, right_pt, middle, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  debug_draw_line(dbg, bottom, left, color);
}

static void debug_draw_char_T(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, right_pt, top, color);
  // Fixed: vertical line
  debug_draw_line(dbg, top, bottom, color);
}

static void debug_draw_char_U(debug_renderer_t *dbg, simd_float3 pos,
                              float size, simd_float3 up, simd_float3 right,
                              simd_float4 color) {
  simd_float3 top = pos + up * size * 0.5f;
  simd_float3 bottom = pos - up * size * 0.5f;
  simd_float3 left = pos - right * size * 0.4f;
  simd_float3 right_pt = pos + right * size * 0.4f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, left, bottom, color);
  debug_draw_line(dbg, right_pt, top, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  debug_draw_line(dbg, bottom, right_pt, color);
}

static void debug_draw_char_dash(debug_renderer_t *dbg, simd_float3 pos,
                                 float size, simd_float3 up, simd_float3 right,
                                 simd_float4 color) {
  simd_float3 left = pos - right * size * 0.3f;
  simd_float3 right_pt = pos + right * size * 0.3f;

  debug_draw_line(dbg, left, right_pt, color);
}

static void debug_draw_char_box(debug_renderer_t *dbg, simd_float3 pos,
                                float size, simd_float3 up, simd_float3 right,
                                simd_float4 color) {
  simd_float3 top = pos + up * size * 0.4f;
  simd_float3 bottom = pos - up * size * 0.4f;
  simd_float3 left = pos - right * size * 0.3f;
  simd_float3 right_pt = pos + right * size * 0.3f;

  debug_draw_line(dbg, left, top, color);
  debug_draw_line(dbg, top, right_pt, color);
  debug_draw_line(dbg, right_pt, bottom, color);
  debug_draw_line(dbg, bottom, left, color);
}
