#pragma once

#include <common.h>
#include <core/types.h>
#include <math/mat4.h>
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

/**
 * =================================================================================================
 *                              RENDERER TYPE DEFINITIONS
 * =================================================================================================
 *
 * Purpose: Defines fundamental types and structures used throughout the
 * rendering pipeline. Includes vertex formats, texture descriptors, material
 * definitions, and render pass configurations.
 */

/* ===== VERTEX FORMATS ===== */

/**
 * Standard vertex layout with position, normal, and texture coordinates
 */
typedef struct {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
} Vertex;

/**
 * Extended vertex layout with tangent space for normal mapping
 */
typedef struct {
  Vec3 position;
  Vec3 normal;
  Vec3 tangent;
  Vec3 bitangent;
  Vec2 uv;
} VertexTangent;

/**
 * Skeletal/rigged vertex with bone weights
 */
typedef struct {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
  u8 bone_ids[4];
  f32 bone_weights[4];
} VertexSkinned;

/* ===== TEXTURE TYPES ===== */

typedef enum {
  // Standard Color Formats
  TEX_FORMAT_RGBA8,
  TEX_FORMAT_RGBA16F,
  TEX_FORMAT_RGBA32F,
  TEX_FORMAT_RGB10A2,

  // G-Buffer Optimized Formats
  TEX_FORMAT_RG16F,      // 2-channel 16-bit float (normals, velocity)
  TEX_FORMAT_R11G11B10F, // Packed 32-bit HDR (emissive)

  // Depth/Stencil Formats
  TEX_FORMAT_D32F,  // 32-bit float depth
  TEX_FORMAT_D24S8, // 24-bit depth + 8-bit stencil

  // Compressed Formats
  TEX_FORMAT_BC1,
  TEX_FORMAT_BC3,
  TEX_FORMAT_BC5,
  TEX_FORMAT_BC7,
  TEX_FORMAT_ASTC_6x6,
} TextureFormat;

typedef enum {
  TEX_FILTER_NEAREST,
  TEX_FILTER_LINEAR,
  TEX_FILTER_CUBIC,
} TextureFilter;

typedef enum {
  TEX_WRAP_CLAMP,
  TEX_WRAP_REPEAT,
  TEX_WRAP_MIRROR,
  TEX_WRAP_BORDER,
} TextureWrap;

/**
 * Texture descriptor for creating GPU textures
 */
typedef struct {
  u32 width;
  u32 height;
  u32 depth;
  u32 mip_levels;
  u32 array_layers;
  TextureFormat format;
  TextureFilter filter;
  TextureWrap wrap_u;
  TextureWrap wrap_v;
  TextureWrap wrap_w;
  void *data;
  u32 data_size;
} TextureDescriptor;

/* ===== MATERIAL TYPES ===== */

typedef enum {
  MATERIAL_OPAQUE,
  MATERIAL_MASKED,
  MATERIAL_TRANSLUCENT,
  MATERIAL_ADDITIVE,
} MaterialBlendMode;

typedef struct {
  Vec4 base_color;
  f32 metallic;
  f32 roughness;
  f32 normal_strength;
  MaterialBlendMode blend_mode;
  bool two_sided;
  bool cast_shadow;
  bool receive_shadow;
} MaterialProperties;

/* ===== RENDER PASS TYPES ===== */

typedef enum {
  RENDERPASS_OPAQUE,
  RENDERPASS_TRANSPARENT,
  RENDERPASS_SHADOW,
  RENDERPASS_POSTPROCESS,
  RENDERPASS_UI,
} RenderPassType;

/**
 * Render pass configuration
 */
typedef struct {
  RenderPassType type;
  Mat4 view_matrix;
  Mat4 projection_matrix;
  Vec3 camera_position;
  u32 target_width;
  u32 target_height;
  bool clear_color;
  bool clear_depth;
  Vec4 clear_color_value;
  f32 clear_depth_value;
} RenderPassDescriptor;

/* ===== DRAW COMMAND ===== */

typedef struct {
  u32 vertex_offset;
  u32 vertex_count;
  u32 instance_count;
  u32 index_offset;
  u32 index_count;
} DrawCommand;

/* ===== TRANSFORM ===== */

typedef struct {
  Vec3 position;
  Vec3 rotation; // In radians (euler angles)
  Vec3 scale;
} Transform;

Mat4 transform_to_matrix(Transform *t);
Transform matrix_to_transform(Mat4 *m);
