#import "metal_backend.h"
#import <Metal/Metal.h>
#include <core/logger.h>
#include <core/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CACHED_LIBRARIES 32
static struct {
  char path[256];
  void *library; // id<MTLLibrary>
} s_library_cache[MAX_CACHED_LIBRARIES];
static int s_library_count = 0;

// Helper to convert NSString to C string copy
static char *nsstring_to_cstring(NSString *str) {
  if (!str)
    return NULL;
  const char *utf8 = [str UTF8String];
  return strdup(utf8);
}

void *metal_compile_shader_from_source(void *device_ptr, const char *source,
                                       const char *name, char **out_error) {
  if (!device_ptr || !source) {
    if (out_error)
      *out_error = strdup("Invalid device or source");
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ptr;
  NSString *sourceStr = [NSString stringWithUTF8String:source];
  MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
  options.fastMathEnabled = YES;
  options.languageVersion =
      MTLLanguageVersion2_1; // Target a reasonable version

  NSError *error = nil;
  id<MTLLibrary> library = [device newLibraryWithSource:sourceStr
                                                options:options
                                                  error:&error];

  if (!library) {
    LOG_ERROR("Shader compilation failed for %s: %s", name ? name : "unknown",
              [[error localizedDescription] UTF8String]);
    if (out_error && error) {
      *out_error = nsstring_to_cstring([error localizedDescription]);
    }
    return NULL;
  }

  LOG_INFO("Successfully compiled shader source: %s", name ? name : "unknown");
  return (__bridge_retained void *)library;
}

void *metal_load_shader_library(void *device_ptr, const char *path,
                                char **out_error) {
  if (!device_ptr || !path)
    return NULL;

  // Check cache
  for (int i = 0; i < s_library_count; i++) {
    if (strcmp(s_library_cache[i].path, path) == 0) {
      return s_library_cache[i].library;
    }
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ptr;
  NSString *pathStr = [NSString stringWithUTF8String:path];
  NSURL *url = [NSURL fileURLWithPath:pathStr];

  NSError *error = nil;
  id<MTLLibrary> library = [device newLibraryWithURL:url error:&error];

  if (!library) {
    LOG_ERROR("Failed to load shader library at %s: %s", path,
              [[error localizedDescription] UTF8String]);
    if (out_error && error) {
      *out_error = nsstring_to_cstring([error localizedDescription]);
    }
    return NULL;
  }

  // Cache it
  if (s_library_count < MAX_CACHED_LIBRARIES) {
    strncpy(s_library_cache[s_library_count].path, path, 255);
    s_library_cache[s_library_count].library =
        (__bridge_retained void *)library;
    s_library_count++;
  }

  LOG_INFO("Loaded shader library: %s", path);
  return (__bridge void *)library;
}

void *metal_get_function(void *library_ptr, const char *function_name) {
  if (!library_ptr || !function_name)
    return NULL;

  id<MTLLibrary> library = (__bridge id<MTLLibrary>)library_ptr;
  NSString *nameStr = [NSString stringWithUTF8String:function_name];

  id<MTLFunction> function = [library newFunctionWithName:nameStr];

  if (!function) {
    LOG_WARN("Function %s not found in library", function_name);
    return NULL;
  }

  return (__bridge void *)function;
}

void *metal_compile_shader_with_variant(void *device_ptr, const char *source,
                                        const char *name,
                                        const MetalShaderVariant *variant,
                                        char **out_error) {
  if (!device_ptr || !source)
    return NULL;

  id<MTLDevice> device = (__bridge id<MTLDevice>)device_ptr;
  NSString *sourceStr = [NSString stringWithUTF8String:source];
  MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
  options.fastMathEnabled = YES;
  options.languageVersion = MTLLanguageVersion2_1;

  if (variant && variant->macro_count > 0) {
    NSMutableDictionary *macros = [NSMutableDictionary dictionary];
    for (u32 i = 0; i < variant->macro_count; i++) {
      NSString *entry = [NSString stringWithUTF8String:variant->macros[i]];
      NSArray *components = [entry componentsSeparatedByString:@"="];
      if (components.count == 2) {
        macros[components[0]] = components[1];
      } else {
        macros[components[0]] = @(1);
      }
    }
    options.preprocessorMacros = macros;
  }

  NSError *error = nil;
  id<MTLLibrary> library = [device newLibraryWithSource:sourceStr
                                                options:options
                                                  error:&error];

  if (!library) {
    LOG_ERROR("Variant compilation failed for %s: %s", name ? name : "unknown",
              [[error localizedDescription] UTF8String]);
    if (out_error && error) {
      *out_error = nsstring_to_cstring([error localizedDescription]);
    }
    return NULL;
  }

  return (__bridge_retained void *)library;
}

void *metal_get_function_with_constants(void *library_ptr,
                                        const char *function_name,
                                        void *constant_values_ptr,
                                        char **out_error) {
  if (!library_ptr || !function_name)
    return NULL;

  id<MTLLibrary> library = (__bridge id<MTLLibrary>)library_ptr;
  MTLFunctionConstantValues *constants =
      (__bridge MTLFunctionConstantValues *)constant_values_ptr;
  NSString *nameStr = [NSString stringWithUTF8String:function_name];

  NSError *error = nil;
  id<MTLFunction> function = [library newFunctionWithName:nameStr
                                           constantValues:constants
                                                    error:&error];

  if (!function) {
    LOG_ERROR("Failed to create function %s with constants: %s", function_name,
              [[error localizedDescription] UTF8String]);
    if (out_error && error)
      *out_error = nsstring_to_cstring([error localizedDescription]);
    return NULL;
  }

  return (__bridge void *)function;
}

void *metal_reflect_function(void *function_ptr,
                             MetalReflectionData *out_data) {
  if (!function_ptr || !out_data)
    return NULL;

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ptr;

  // Basic reflection: Name
  if (function.name) {
    strncpy(out_data->entry_point_name, [function.name UTF8String], 63);
  } else {
    strncpy(out_data->entry_point_name, "unknown", 63);
  }

  out_data->arg_count = 0;

  // Reflection of vertex attributes
  if (function.functionType == MTLFunctionTypeVertex &&
      function.vertexAttributes) {
    NSArray *attrs = function.vertexAttributes;
    for (uint32_t i = 0; i < attrs.count; i++) {
      MTLVertexAttribute *attr = attrs[i];
      if (out_data->arg_count < 16) {
        MetalArgumentInfo *info = &out_data->args[out_data->arg_count++];
        strncpy(info->name, [attr.name UTF8String], 63);
        info->index = (u32)attr.attributeIndex;
        info->type = 1; // Attribute
      }
    }
  }

  // Compute reflection for thread group size and arguments
  if (function.functionType == MTLFunctionTypeCompute) {
    id<MTLDevice> device = function.device;
    MTLComputePipelineReflection *reflection = nil;
    NSError *error = nil;
    id<MTLComputePipelineState> pso = [device
        newComputePipelineStateWithFunction:function
                                    options:MTLPipelineOptionBufferTypeInfo
                                 reflection:&reflection
                                      error:&error];
    if (pso && reflection) {
      out_data->thread_group_size_x = (u32)pso.maxTotalThreadsPerThreadgroup;
      out_data->simd_group_size = (u32)pso.threadExecutionWidth;

      NSArray *args = reflection.arguments;
      for (uint32_t i = 0; i < args.count; i++) {
        MTLComputeArgument *arg = args[i];
        if (out_data->arg_count < 16) {
          MetalArgumentInfo *info = &out_data->args[out_data->arg_count++];
          strncpy(info->name, [arg.name UTF8String], 63);
          info->index = (u32)arg.index;
          if (arg.type == MTLArgumentTypeBuffer)
            info->type = 0;
          else if (arg.type == MTLArgumentTypeTexture)
            info->type = 2;
          else if (arg.type == MTLArgumentTypeSampler)
            info->type = 3;

          if (arg.type == MTLArgumentTypeBuffer) {
            info->buffer_data_size = (u32)arg.bufferAlignment;
          }
        }
      }
    }
  }

  out_data->signature_hash = [function hash];

  return NULL;
}

// Internal callback for file watcher
#include <core/file_watcher.h>

static void shader_file_changed(const FileEvent *event, void *user_data) {
  if (event->event == FILE_EVENT_MODIFIED) {
    LOG_INFO("Shader file modified: %s. Triggering reload...", event->path);
    // Reload logic: Invalidate the library cache for this path if it exists
    for (int i = 0; i < s_library_count; i++) {
      if (strcmp(s_library_cache[i].path, event->path) == 0) {
        // Invalidate entry (in real system we'd recompile)
        s_library_cache[i].path[0] = '\0';
        LOG_INFO("Invalidated library cache for %s", event->path);
        break;
      }
    }
  }
}

void metal_start_shader_hot_reload(const char *shader_dir) {
  FileWatcher *watcher = file_watcher_create(shader_dir);
  if (watcher) {
    file_watcher_add_callback(watcher, shader_file_changed, NULL);
    file_watcher_start(watcher);
    LOG_INFO("Started shader hot-reload watcher on %s", shader_dir);
  } else {
    LOG_ERROR("Failed to start shader hot-reload watcher on %s", shader_dir);
  }
}
