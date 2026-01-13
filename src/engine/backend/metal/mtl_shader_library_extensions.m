/*
 * mtl_shader_library_extensions.m
 * Enhanced shader library reflection and specialization
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_shader_library.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdlib.h>
#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

/* ============================================================================
 * FUNCTION REFLECTION CACHE
 * ============================================================================
 */

typedef struct mtl_function_reflection {
  id<MTLFunction> function;
  id<MTLArgumentEncoder> argument_encoder;
  NSArray<MTLArgument *> *arguments;
  NSArray<MTLArgument *> *buffer_arguments;
  NSArray<MTLArgument *> *texture_arguments;
  NSArray<MTLArgument *> *sampler_arguments;
  uint32_t buffer_count;
  uint32_t texture_count;
  uint32_t sampler_count;
  uint32_t total_arguments;
  bool is_reflection_valid;
} mtl_function_reflection_t;

static NSMutableDictionary<NSString *, NSValue *> *g_reflection_cache = nil;

static mtl_function_reflection_t* get_or_create_reflection(id<MTLFunction> function) {
  if (!function) {
    return NULL;
  }
  
  // Initialize cache if needed
  if (!g_reflection_cache) {
    g_reflection_cache = [[NSMutableDictionary alloc] init];
  }
  
  NSString *function_key = [NSString stringWithFormat:@"%@_%@", 
                           function.name, 
                           [[function.device UUID] UUIDString]];
  
  NSValue *cached_value = g_reflection_cache[function_key];
  if (cached_value) {
    return (mtl_function_reflection_t*)[cached_value pointerValue];
  }
  
  // Create new reflection
  mtl_function_reflection_t *reflection = calloc(1, sizeof(mtl_function_reflection_t));
  if (!reflection) {
    return NULL;
  }
  
  reflection->function = function;
  
  // Get function arguments using reflection API
  if (@available(macOS 10.12, iOS 10.0, *)) {
    MTLArgumentReflection *arg_reflection = [function newArgumentReflection];
    if (arg_reflection) {
      reflection->arguments = arg_reflection.arguments;
      
      // Categorize arguments
      NSMutableArray<MTLArgument *> *buffer_args = [[NSMutableArray alloc] init];
      NSMutableArray<MTLArgument *> *texture_args = [[NSMutableArray alloc] init];
      NSMutableArray<MTLArgument *> *sampler_args = [[NSMutableArray alloc] init];
      
      for (MTLArgument *arg in reflection->arguments) {
        switch (arg.type) {
          case MTLArgumentTypeBuffer:
            [buffer_args addObject:arg];
            reflection->buffer_count++;
            break;
          case MTLArgumentTypeTexture:
            [texture_args addObject:arg];
            reflection->texture_count++;
            break;
          case MTLArgumentTypeSampler:
            [sampler_args addObject:arg];
            reflection->sampler_count++;
            break;
          default:
            break;
        }
        reflection->total_arguments++;
      }
      
      reflection->buffer_arguments = [buffer_args copy];
      reflection->texture_arguments = [texture_args copy];
      reflection->sampler_arguments = [sampler_args copy];
      reflection->is_reflection_valid = true;
    }
  }
  
  // Cache the reflection
  NSValue *reflection_value = [NSValue valueWithPointer:reflection];
  g_reflection_cache[function_key] = reflection_value;
  
  return reflection;
}

/* ============================================================================
 * IMPROVED FUNCTION REFLECTION
 * ============================================================================
 */

uint32_t metal_function_get_buffer_bindings(MTLFunctionRef function_ref,
                                            uint32_t *out_indices,
                                            uint32_t max_count) {
  if (!function_ref || !out_indices || max_count == 0) {
    return 0;
  }

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ref;
  
  // Get reflection data
  mtl_function_reflection_t *reflection = get_or_create_reflection(function);
  if (!reflection || !reflection->is_reflection_valid) {
    return 0;
  }
  
  // Copy buffer binding indices
  uint32_t count = 0;
  for (MTLArgument *arg in reflection->buffer_arguments) {
    if (count < max_count) {
      out_indices[count++] = (uint32_t)arg.index;
    } else {
      break;
    }
  }
  
  return count;
}

uint32_t metal_function_get_texture_bindings(MTLFunctionRef function_ref,
                                             uint32_t *out_indices,
                                             uint32_t max_count) {
  if (!function_ref || !out_indices || max_count == 0) {
    return 0;
  }

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ref;
  
  // Get reflection data
  mtl_function_reflection_t *reflection = get_or_create_reflection(function);
  if (!reflection || !reflection->is_reflection_valid) {
    return 0;
  }
  
  // Copy texture binding indices
  uint32_t count = 0;
  for (MTLArgument *arg in reflection->texture_arguments) {
    if (count < max_count) {
      out_indices[count++] = (uint32_t)arg.index;
    } else {
      break;
    }
  }
  
  return count;
}

/* ============================================================================
 * ENHANCED SPECIALIZATION CONSTANTS
 * ============================================================================
 */

// Helper structure for passing constant values
typedef struct metal_constant_value {
  uint32_t index;
  MTLDataType type;
  union {
    bool bool_value;
    int int_value;
    float float_value;
  } data;
} metal_constant_value_t;

MTLFunctionRef metal_function_create_specialized_ex(
    MTLLibraryRef library_ref, const char *name,
    const metal_constant_value_t *constants, uint32_t constant_count) {

  if (!library_ref || !name) {
    return NULL;
  }

  id<MTLLibrary> library = (__bridge id<MTLLibrary>)library_ref;
  NSString *functionName = [NSString stringWithUTF8String:name];

  // Create function constant values
  MTLFunctionConstantValues *constantValues =
      [[MTLFunctionConstantValues alloc] init];

  // Set each constant value
  for (uint32_t i = 0; i < constant_count; i++) {
    const metal_constant_value_t *constant = &constants[i];

    switch (constant->type) {
    case MTLDataTypeBool:
      [constantValues setConstantValue:&constant->data.bool_value
                                  type:MTLDataTypeBool
                               atIndex:constant->index];
      break;

    case MTLDataTypeInt:
      [constantValues setConstantValue:&constant->data.int_value
                                  type:MTLDataTypeInt
                               atIndex:constant->index];
      break;

    case MTLDataTypeFloat:
      [constantValues setConstantValue:&constant->data.float_value
                                  type:MTLDataTypeFloat
                               atIndex:constant->index];
      break;

    default:
      NSLog(@"Unsupported constant type at index %u", i);
      break;
    }
  }

  NSError *error = nil;
  id<MTLFunction> function = [library newFunctionWithName:functionName
                                           constantValues:constantValues
                                                    error:&error];

  if (error || !function) {
    NSLog(@"Failed to create specialized function '%s': %@", name, error);
    return NULL;
  }

  return (__bridge_retained MTLFunctionRef)function;
}

/* ============================================================================
 * COMPUTE FUNCTION INTROSPECTION
 * ============================================================================
 */

bool metal_function_get_threadgroup_size(MTLFunctionRef function_ref,
                                         uint32_t *out_width,
                                         uint32_t *out_height,
                                         uint32_t *out_depth) {
  if (!function_ref) {
    return false;
  }

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ref;

  // Only applies to compute functions
  if (function.functionType != MTLFunctionTypeKernel) {
    return false;
  }

  // Get threadgroup size hint if specified in the shader
  // Note: This requires iOS 13+ / macOS 10.15+
  if (@available(macOS 10.15, iOS 13.0, *)) {
    // Check if function has a patchType (indicates threadgroup size metadata)
    // Metal doesn't directly expose threadgroup size from function metadata
    // But we can get it from the compiled pipeline state

    // For now, return false - this would need a pipeline state to query
    // Or the shader would need to be annotated with
    // [[max_total_threads_per_threadgroup(N)]]
    NSLog(@"Threadgroup size query requires pipeline state object");
    return false;
  }

  return false;
}

/* ============================================================================
 * ENHANCED METADATA EXTRACTION
 * ============================================================================
 */

bool metal_function_get_metadata_ex(MTLFunctionRef function_ref,
                                    metal_function_metadata_t *out_metadata) {
  if (!function_ref || !out_metadata) {
    return false;
  }

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ref;

  // Get function name
  strncpy(out_metadata->name, [function.name UTF8String],
          sizeof(out_metadata->name) - 1);

  // Determine function type
  MTLFunctionType mtl_type = function.functionType;
  switch (mtl_type) {
  case MTLFunctionTypeVertex:
    out_metadata->type = METAL_FUNCTION_TYPE_VERTEX;
    break;
  case MTLFunctionTypeFragment:
    out_metadata->type = METAL_FUNCTION_TYPE_FRAGMENT;
    break;
  case MTLFunctionTypeKernel:
    out_metadata->type = METAL_FUNCTION_TYPE_COMPUTE;
    break;
  default:
    return false;
  }

  // Get reflection data for accurate argument counts
  mtl_function_reflection_t *reflection = get_or_create_reflection(function);
  if (reflection && reflection->is_reflection_valid) {
    out_metadata->argument_count = reflection->total_arguments;
    out_metadata->buffer_binding_count = reflection->buffer_count;
    out_metadata->texture_binding_count = reflection->texture_count;
  } else {
    // Fallback to zero counts if reflection fails
    out_metadata->argument_count = 0;
    out_metadata->buffer_binding_count = 0;
    out_metadata->texture_binding_count = 0;
  }

  return true;
}

/* End of mtl_shader_library_extensions.m */
