#ifndef ShaderTypes_h
#define ShaderTypes_h

#include <simd/simd.h>

enum {
  GBufferTextureIndexAlbedo = 0,
  GBufferTextureIndexNormal = 1,
  GBufferTextureIndexDepth = 2,
  GBufferTextureIndexTotal = 3
};

struct Vertex {
  vector_float3 position;
  vector_float4 color;
};

struct Uniforms {
  matrix_float4x4 modelViewProjectionMatrix;
};

struct InstanceData {
  vector_float3 instancePosition;
  vector_float4 instanceColor;
  uint32_t textureIndex;
};

struct Frustum {
  vector_float4 planes[6];
};

struct Light {
  vector_float3 position;
  vector_float3 color;
  float intensity;
};

#endif /* ShaderTypes_h */
