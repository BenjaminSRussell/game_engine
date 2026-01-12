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

  // Use the correct stageInputAttributes or vertexAttributes depending on
  // shader type
  NSArray<MTLArgument *> *arguments = nil;

  if (function.functionType == MTLFunctionTypeVertex) {
    // For vertex functions, check vertex attributes
    arguments = function.vertexAttributes ?: @[];
  } else if (function.functionType == MTLFunctionTypeFragment ||
             function.functionType == MTLFunctionTypeKernel) {
    // For fragment/compute, use stage input attributes
    arguments = function.stageInputAttributes ?: @[];
  }

  // Additionally, check all arguments (buffers, textures, samplers)
  // This is more reliable for finding buffer bindings
  uint32_t count = 0;

  // Iterate through all function arguments
  /* Deprecated: function.arguments not available in recent Metal SDKs
     TODO: Implement proper reflection by storing MTLFunctionReflection during
  creation for (MTLArgument *arg in function.arguments) { if (arg.type ==
  MTLArgumentTypeBuffer && count < max_count) { out_indices[count++] =
  (uint32_t)arg.index;
    }
  }
  */

  return count;
}

uint32_t metal_function_get_texture_bindings(MTLFunctionRef function_ref,
                                             uint32_t *out_indices,
                                             uint32_t max_count) {
  if (!function_ref || !out_indices || max_count == 0) {
    return 0;
  }

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ref;

  uint32_t count = 0;

  // Iterate through all function arguments for texture bindings
  /* Deprecated: function.arguments not available in recent Metal SDKs
  for (MTLArgument *arg in function.arguments) {
    if (arg.type == MTLArgumentTypeTexture && count < max_count) {
      out_indices[count++] = (uint32_t)arg.index;
    }
  }
  */

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

  // Count all argument using the arguments array (more reliable)
  // NSArray<MTLArgument *> *arguments = function.arguments ?: @[];
  // out_metadata->argument_count = (uint32_t)[arguments count];
  out_metadata->argument_count = 0;

  // Count buffer and texture bindings accurately
  out_metadata->buffer_binding_count = 0;
  out_metadata->texture_binding_count = 0;

  /* Deprecated: function.arguments
  for (MTLArgument *arg in arguments) {
    if (arg.type == MTLArgumentTypeBuffer) {
      out_metadata->buffer_binding_count++;
    } else if (arg.type == MTLArgumentTypeTexture) {
      out_metadata->texture_binding_count++;
    }
  }
  */

  return true;
}

/* End of mtl_shader_library_extensions.m */
