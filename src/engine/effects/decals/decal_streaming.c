// decal_streaming.c - Deferred Decal System Implementation
#include <include/materials/decal_system.h>
#include <include/core/logger.h>
#include <stdlib.h>
#include <string.h>

static DecalSystem *g_decal_system = NULL;

DecalSystem* decal_system_create(id<MTLDevice> device) {
  if (!device) {
    LOG_ERROR("Cannot create decal system without Metal device");
    return NULL;
  }
  
  DecalSystem *system = (DecalSystem*)calloc(1, sizeof(DecalSystem));
  if (!system) {
    LOG_ERROR("Failed to allocate decal system");
    return NULL;
  }
  
  system->device = device;
  system->decal_count = 0;
  
  // Create decal buffer
  size_t decal_buffer_size = MAX_DECALS * sizeof(Decal);
  system->decal_buffer = [device newBufferWithLength:decal_buffer_size 
                                                options:MTLResourceStorageModeShared];
  
  // Create render pipeline for deferred decal rendering
  NSError *error = nil;
  id<MTLLibrary> library = [device newDefaultLibrary];
  
  id<MTLFunction> vertex_fn = [library newFunctionWithName:@"decal_vertex"];
  id<MTLFunction> fragment_fn = [library newFunctionWithName:@"decal_fragment"];
  
  if (vertex_fn && fragment_fn) {
    MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.vertexFunction = vertex_fn;
    desc.fragmentFunction = fragment_fn;
    
    // Set up render targets for deferred rendering
    desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;  // Albedo
    desc.colorAttachments[1].pixelFormat = MTLPixelFormatBGRA8Unorm;  // Normal
    desc.colorAttachments[2].pixelFormat = MTLPixelFormatBGRA8Unorm;  // Material
    desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    
    // Set up blending
    for (int i = 0; i < 3; i++) {
      desc.colorAttachments[i].blendingEnabled = YES;
      desc.colorAttachments[i].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
      desc.colorAttachments[i].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    }
    
    system->pipeline_state = [device newRenderPipelineStateWithDescriptor:desc error:&error];
    
    if (error) {
      LOG_ERROR("Failed to create decal render pipeline: %s", error.localizedDescription.UTF8String);
    }
  }
  
  // Create depth stencil state
  MTLDepthStencilDescriptor *depth_desc = [[MTLDepthStencilDescriptor alloc] init];
  depth_desc.depthCompareFunction = MTLCompareFunctionLessEqual;
  depth_desc.depthWriteEnabled = NO; // Don't write depth for decals
  system->depth_stencil = [device newDepthStencilStateWithDescriptor:depth_desc];
  
  [library release];
  [vertex_fn release];
  [fragment_fn release];
  [depth_desc release];
  
  LOG_INFO("Decal system created with %u max decals", MAX_DECALS);
  return system;
}

void decal_system_destroy(DecalSystem *system) {
  if (!system) return;
  
  if (system->pipeline_state) {
    [system->pipeline_state release];
  }
  
  if (system->depth_stencil) {
    [system->depth_stencil release];
  }
  
  if (system->decal_buffer) {
    [system->decal_buffer release];
  }
  
  // Release all textures
  for (u32 i = 0; i < system->decal_count; i++) {
    Decal *decal = &system->decals[i];
    
    if (decal->albedo_texture) {
      [decal->albedo_texture release];
    }
    if (decal->normal_texture) {
      [decal->normal_texture release];
    }
    if (decal->material_texture) {
      [decal->material_texture release];
    }
  }
  
  free(system);
  LOG_INFO("Decal system destroyed");
}

u32 decal_add(DecalSystem *system, const Vec3 *position, const Vec3 *size, f32 rotation) {
  if (!system || !position || !size || system->decal_count >= MAX_DECALS) {
    return 0;
  }
  
  u32 decal_id = system->decal_count++;
  Decal *decal = &system->decals[decal_id];
  
  // Initialize decal
  memset(decal, 0, sizeof(Decal));
  
  // Set transform
  decal->transform = mat4_identity();
  decal->transform.m[3][0] = position->x;
  decal->transform.m[3][1] = position->y;
  decal->transform.m[3][2] = position->z;
  
  // Apply rotation around Y axis
  Mat4 rotation_matrix = mat4_rotation_y(rotation);
  decal->transform = mat4_mul(decal->transform, rotation_matrix);
  
  // Set size
  decal->size = *size;
  decal->transform.m[0][0] = size->x;
  decal->transform.m[1][1] = size->y;
  decal->transform.m[2][2] = size->z;
  
  // Calculate inverse transform
  decal->inverse_transform = mat4_inverse(decal->transform);
  
  // Set default properties
  decal->tint_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
  decal->opacity = 1.0f;
  decal->normal_strength = 1.0f;
  decal->blend_mode = DECAL_BLEND_TRANSLUCENT;
  decal->receive_gbuffer_normal = true;
  decal->render_order = 100;
  decal->fade_distance = 50.0f;
  decal->active = true;
  
  LOG_DEBUG("Added decal %u at position (%.2f, %.2f, %.2f)", 
           decal_id, position->x, position->y, position->z);
  return decal_id;
}

void decal_remove(DecalSystem *system, u32 decal_id) {
  if (!system || decal_id >= system->decal_count) return;
  
  Decal *decal = &system->decals[decal_id];
  
  // Release textures
  if (decal->albedo_texture) {
    [decal->albedo_texture release];
  }
  if (decal->normal_texture) {
    [decal->normal_texture release];
  }
  if (decal->material_texture) {
    [decal->material_texture release];
  }
  
  // Remove decal from array
  for (u32 i = decal_id; i < system->decal_count - 1; i++) {
    system->decals[i] = system->decals[i + 1];
  }
  
  system->decal_count--;
  LOG_DEBUG("Removed decal %u", decal_id);
}

void decal_set_textures(DecalSystem *system, u32 decal_id,
                       id<MTLTexture> albedo, id<MTLTexture> normal,
                       id<MTLTexture> material) {
  if (!system || decal_id >= system->decal_count) return;
  
  Decal *decal = &system->decals[decal_id];
  
  // Release old textures
  if (decal->albedo_texture) {
    [decal->albedo_texture release];
  }
  if (decal->normal_texture) {
    [decal->normal_texture release];
  }
  if (decal->material_texture) {
    [decal->material_texture release];
  }
  
  // Set new textures
  decal->albedo_texture = albedo;
  decal->normal_texture = normal;
  decal->material_texture = material;
  
  // Retain textures
  if (albedo) [albedo retain];
  if (normal) [normal retain];
  if (material) [material retain];
  
  LOG_DEBUG("Set textures for decal %u", decal_id);
}

void decal_set_blend_mode(DecalSystem *system, u32 decal_id, DecalBlendMode mode) {
  if (!system || decal_id >= system->decal_count) return;
  
  system->decals[decal_id].blend_mode = mode;
  LOG_DEBUG("Set blend mode %d for decal %u", mode, decal_id);
}

void decal_set_tint_color(DecalSystem *system, u32 decal_id, const Vec4 *color) {
  if (!system || decal_id >= system->decal_count || !color) return;
  
  system->decals[decal_id].tint_color = *color;
}

void decal_set_opacity(DecalSystem *system, u32 decal_id, f32 opacity) {
  if (!system || decal_id >= system->decal_count) return;
  
  system->decals[decal_id].opacity = fmaxf(0.0f, fminf(1.0f, opacity));
}

void decal_set_fade_distance(DecalSystem *system, u32 decal_id, f32 fade_distance) {
  if (!system || decal_id >= system->decal_count) return;
  
  system->decals[decal_id].fade_distance = fade_distance;
}

void decal_set_normal_strength(DecalSystem *system, u32 decal_id, f32 strength) {
  if (!system || decal_id >= system->decal_count) return;
  
  system->decals[decal_id].normal_strength = strength;
}

void decal_set_render_order(DecalSystem *system, u32 decal_id, u32 render_order) {
  if (!system || decal_id >= system->decal_count) return;
  
  system->decals[decal_id].render_order = render_order;
}

void decal_set_active(DecalSystem *system, u32 decal_id, bool active) {
  if (!system || decal_id >= system->decal_count) return;
  
  system->decals[decal_id].active = active;
}

void decal_update_transform(DecalSystem *system, u32 decal_id, const Vec3 *position, 
                           const Vec3 *size, f32 rotation) {
  if (!system || decal_id >= system->decal_count || !position || !size) return;
  
  Decal *decal = &system->decals[decal_id];
  
  // Update transform
  decal->transform.m[3][0] = position->x;
  decal->transform.m[3][1] = position->y;
  decal->transform.m[3][2] = position->z;
  
  // Apply rotation
  Mat4 rotation_matrix = mat4_rotation_y(rotation);
  Mat4 scale_matrix = mat4_identity();
  scale_matrix.m[0][0] = size->x;
  scale_matrix.m[1][1] = size->y;
  scale_matrix.m[2][2] = size->z;
  
  decal->transform = mat4_mul(scale_matrix, rotation_matrix);
  decal->transform.m[3][0] = position->x;
  decal->transform.m[3][1] = position->y;
  decal->transform.m[3][2] = position->z;
  
  // Update inverse transform
  decal->inverse_transform = mat4_inverse(decal->transform);
  decal->size = *size;
}

void decal_render(DecalSystem *system, id<MTLRenderCommandEncoder> encoder,
                  id<MTLTexture> gbuffer_depth, id<MTLTexture> gbuffer_normal,
                  const Mat4 *view_proj) {
  if (!system || !encoder || system->decal_count == 0) return;
  
  // Set render pipeline
  [encoder setRenderPipelineState:system->pipeline_state];
  [encoder setDepthStencilState:system->depth_stencil];
  
  // Set G-buffer textures
  [encoder setFragmentTexture:gbuffer_depth atIndex:0];
  [encoder setFragmentTexture:gbuffer_normal atIndex:1];
  
  // Set view-projection matrix
  [encoder setVertexBytes:view_proj length:sizeof(Mat4) atIndex:0];
  
  // Sort decals by render order
  decal_sort_by_render_order(system);
  
  // Render each decal
  for (u32 i = 0; i < system->decal_count; i++) {
    Decal *decal = &system->decals[i];
    
    if (!decal->active) continue;
    
    // Set decal textures
    if (decal->albedo_texture) {
      [encoder setFragmentTexture:decal->albedo_texture atIndex:2];
    }
    if (decal->normal_texture) {
      [encoder setFragmentTexture:decal->normal_texture atIndex:3];
    }
    if (decal->material_texture) {
      [encoder setFragmentTexture:decal->material_texture atIndex:4];
    }
    
    // Set decal properties
    [encoder setVertexBytes:&decal->transform length:sizeof(Mat4) atIndex:1];
    [encoder setVertexBytes:&decal->inverse_transform length:sizeof(Mat4) atIndex:2];
    [encoder setFragmentBytes:&decal->tint_color length:sizeof(Vec4) atIndex:0];
    [encoder setFragmentBytes:&decal->opacity length:sizeof(f32) atIndex:1];
    [encoder setFragmentBytes:&decal->normal_strength length:sizeof(f32) atIndex:2];
    [encoder setFragmentBytes:&decal->blend_mode length:sizeof(u32) atIndex:3];
    [encoder setFragmentBytes:&decal->fade_distance length:sizeof(f32) atIndex:4];
    
    // Draw decal (as a box)
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:36];
  }
}

u32 decal_get_count(const DecalSystem *system) {
  return system ? system->decal_count : 0;
}

Decal* decal_get(const DecalSystem *system, u32 decal_id) {
  if (!system || decal_id >= system->decal_count) return NULL;
  
  return (Decal*)&system->decals[decal_id];
}

bool decal_is_active(const DecalSystem *system, u32 decal_id) {
  Decal *decal = decal_get(system, decal_id);
  return decal ? decal->active : false;
}

void decal_clear_all(DecalSystem *system) {
  if (!system) return;
  
  // Release all textures
  for (u32 i = 0; i < system->decal_count; i++) {
    Decal *decal = &system->decals[i];
    
    if (decal->albedo_texture) {
      [decal->albedo_texture release];
    }
    if (decal->normal_texture) {
      [decal->normal_texture release];
    }
    if (decal->material_texture) {
      [decal->material_texture release];
    }
  }
  
  system->decal_count = 0;
  LOG_INFO("Cleared all decals");
}

void decal_update_visibility(DecalSystem *system, const Vec3 *camera_position, f32 max_distance) {
  if (!system || !camera_position) return;
  
  for (u32 i = 0; i < system->decal_count; i++) {
    Decal *decal = &system->decals[i];
    
    // Calculate distance from camera to decal
    Vec3 decal_position = (Vec3){decal->transform.m[3][0], decal->transform.m[3][1], decal->transform.m[3][2]};
    Vec3 to_decal = vec3_sub(decal_position, *camera_position);
    f32 distance = vec3_length(&to_decal);
    
    // Disable decals that are too far away
    decal->active = (distance <= max_distance);
  }
}

// Internal helper functions
static void decal_sort_by_render_order(DecalSystem *system) {
  if (!system || system->decal_count <= 1) return;
  
  // Simple bubble sort by render order
  for (u32 i = 0; i < system->decal_count - 1; i++) {
    for (u32 j = 0; j < system->decal_count - i - 1; j++) {
      if (system->decals[j].render_order > system->decals[j + 1].render_order) {
        Decal temp = system->decals[j];
        system->decals[j] = system->decals[j + 1];
        system->decals[j + 1] = temp;
      }
    }
  }
}

static Mat4 mat4_rotation_y(f32 angle) {
  f32 c = cosf(angle);
  f32 s = sinf(angle);
  
  return (Mat4){{
    {{c, 0.0f, s, 0.0f},
     {0.0f, 1.0f, 0.0f, 0.0f},
     {-s, 0.0f, c, 0.0f},
     {0.0f, 0.0f, 0.0f, 1.0f}}
  }};
}

static Mat4 mat4_inverse(const Mat4 *m) {
  // Simplified inverse calculation (for transform matrices)
  Mat4 result = mat4_identity();
  
  // Extract translation
  result.m[3][0] = -m->m[3][0];
  result.m[3][1] = -m->m[3][1];
  result.m[3][2] = -m->m[3][2];
  
  // Transpose rotation part
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      result.m[i][j] = m->m[j][i];
    }
  }
  
  return result;
}

static Mat4 mat4_identity(void) {
  return (Mat4){{
    {{1.0f, 0.0f, 0.0f, 0.0f},
     {0.0f, 1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f, 0.0f},
     {0.0f, 0.0f, 0.0f, 1.0f}}
  }};
}

static Mat4 mat4_mul(const Mat4 *a, const Mat4 *b) {
  Mat4 result;
  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      result.m[i][j] = 0.0f;
      for (int k = 0; k < 4; k++) {
        result.m[i][j] += a->m[i][k] * b->m[k][j];
      }
    }
  }
  
  return result;
}
