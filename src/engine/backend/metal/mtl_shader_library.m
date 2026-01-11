/*
 * mtl_shader_library.m
 * Metal Shader Library Management Implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "backend/metal/mtl_shader_library.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

/* ============================================================================
 * LIBRARY MANAGER
 * ============================================================================
 */

metal_shader_library_manager_t *
metal_shader_library_manager_create(MTLDeviceRef device_ref) {
  if (!device_ref) {
    return NULL;
  }

  metal_shader_library_manager_t *manager =
      (metal_shader_library_manager_t *)calloc(
          1, sizeof(metal_shader_library_manager_t));

  if (!manager) {
    return NULL;
  }

  manager->device = device_ref;
  manager->library_count = 0;
  manager->total_memory_bytes = 0;

  return manager;
}

void metal_shader_library_manager_destroy(
    metal_shader_library_manager_t *manager) {
  if (!manager) {
    return;
  }

  // Release all libraries
  for (uint32_t i = 0; i < manager->library_count; i++) {
    metal_library_entry_t *entry = &manager->libraries[i];
    if (entry->in_use && entry->library) {
      CFRelease(entry->library);
      entry->library = NULL;
      entry->in_use = false;
    }
  }

  free(manager);
}

#pragma clang diagnostic pop

/* ============================================================================
 * LIBRARY LOADING
 * ============================================================================
 */

static uint64_t get_file_mod_time(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    return (uint64_t)st.st_mtime;
  }
  return 0;
}

MTLLibraryRef
metal_library_manager_load_file(metal_shader_library_manager_t *manager,
                                const char *path) {

  if (!manager || !path) {
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)manager->device;

  // Check if already loaded
  for (uint32_t i = 0; i < manager->library_count; i++) {
    metal_library_entry_t *entry = &manager->libraries[i];
    if (entry->in_use && strcmp(entry->path, path) == 0) {
      NSLog(@"Library already loaded: %s", path);
      return entry->library;
    }
  }

  // Load from file
  NSString *nsPath = [NSString stringWithUTF8String:path];

  id<MTLLibrary> library = nil;
  NSError *error = nil;

  if ([nsPath hasSuffix:@".metal"]) {
    // Load source and compile at runtime
    NSString *source = [NSString stringWithContentsOfFile:nsPath
                                                 encoding:NSUTF8StringEncoding
                                                    error:&error];
    if (source) {
      MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
      library = [device newLibraryWithSource:source
                                     options:options
                                       error:&error];
    }
  } else {
    // Assume compiled library
    NSURL *url = [NSURL fileURLWithPath:nsPath];
    library = [device newLibraryWithURL:url error:&error];
  }

  if (error || !library) {
    NSLog(@"Failed to load shader library from %@: %@", nsPath, error);
    return NULL;
  }

  // Add to cache
  if (manager->library_count >= METAL_MAX_CACHED_LIBRARIES) {
    NSLog(@"Library cache full, cannot load more libraries");
    return (__bridge_retained MTLLibraryRef)library; // Return but don't cache
  }

  metal_library_entry_t *entry = &manager->libraries[manager->library_count++];
  entry->library = (__bridge_retained MTLLibraryRef)library;
  strncpy(entry->path, path, sizeof(entry->path) - 1);
  entry->file_mod_time = get_file_mod_time(path);
  entry->function_count = (uint32_t)[[library functionNames] count];
  entry->in_use = true;

  // Estimate memory (rough: 100KB base + 10KB per function)
  manager->total_memory_bytes +=
      100 * 1024 + (entry->function_count * 10 * 1024);

  NSLog(@"Loaded shader library: %s (%u functions)", path,
        entry->function_count);

  return entry->library;
}

MTLLibraryRef
metal_library_manager_load_data(metal_shader_library_manager_t *manager,
                                const void *data, size_t size,
                                const char *debug_name) {

  if (!manager || !data || size == 0) {
    return NULL;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)manager->device;

  dispatch_data_t dispatchData =
      dispatch_data_create(data, size, NULL, DISPATCH_DATA_DESTRUCTOR_DEFAULT);

  NSError *error = nil;
  id<MTLLibrary> library = [device newLibraryWithData:dispatchData
                                                error:&error];

  if (error || !library) {
    NSLog(@"Failed to load shader library from data: %@", error);
    return NULL;
  }

  // Add to cache if space available
  if (manager->library_count >= METAL_MAX_CACHED_LIBRARIES) {
    NSLog(@"Library cache full, returning uncached library");
    return (__bridge_retained MTLLibraryRef)library;
  }

  metal_library_entry_t *entry = &manager->libraries[manager->library_count++];
  entry->library = (__bridge_retained MTLLibraryRef)library;
  strncpy(entry->path, debug_name ? debug_name : "<embedded>",
          sizeof(entry->path) - 1);
  entry->file_mod_time = 0; // No file backing
  entry->function_count = (uint32_t)[[library functionNames] count];
  entry->in_use = true;

  manager->total_memory_bytes += size;

  NSLog(@"Loaded shader library from data: %s (%u functions)", entry->path,
        entry->function_count);

  return entry->library;
}

MTLFunctionRef
metal_library_manager_get_function(metal_shader_library_manager_t *manager,
                                   const char *name) {

  if (!manager || !name) {
    return NULL;
  }

  // Search all loaded libraries
  for (uint32_t i = 0; i < manager->library_count; i++) {
    metal_library_entry_t *entry = &manager->libraries[i];
    if (!entry->in_use) {
      continue;
    }

    MTLFunctionRef func = metal_library_get_function(entry->library, name);
    if (func) {
      return func;
    }
  }

  NSLog(@"Function '%s' not found in any loaded library", name);
  return NULL;
}

MTLFunctionRef metal_library_get_function(MTLLibraryRef library_ref,
                                          const char *name) {

  if (!library_ref || !name) {
    return NULL;
  }

  id<MTLLibrary> library = (__bridge id<MTLLibrary>)library_ref;
  NSString *functionName = [NSString stringWithUTF8String:name];
  id<MTLFunction> function = [library newFunctionWithName:functionName];

  if (!function) {
    return NULL;
  }

  return (__bridge_retained MTLFunctionRef)function;
}

/* ============================================================================
 * FUNCTION METADATA & REFLECTION
 * ============================================================================
 */

bool metal_function_get_metadata(MTLFunctionRef function_ref,
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

  // Count arguments
  NSArray<MTLArgument *> *arguments = function.vertexAttributes ?: @[];
  out_metadata->argument_count = (uint32_t)[arguments count];

  // Count buffer and texture bindings
  out_metadata->buffer_binding_count = 0;
  out_metadata->texture_binding_count = 0;

  for (id<MTLBinding> arg in function.vertexAttributes ?: @[]) {
    if (arg.type == MTLBindingTypeBuffer) {
      out_metadata->buffer_binding_count++;
    } else if (arg.type == MTLBindingTypeTexture) {
      out_metadata->texture_binding_count++;
    }
  }

  return true;
}

bool metal_function_validate_type(MTLFunctionRef function_ref,
                                  metal_shader_function_type_t expected_type) {

  if (!function_ref) {
    return false;
  }

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ref;
  MTLFunctionType mtl_type = function.functionType;

  switch (expected_type) {
  case METAL_FUNCTION_TYPE_VERTEX:
    return mtl_type == MTLFunctionTypeVertex;
  case METAL_FUNCTION_TYPE_FRAGMENT:
    return mtl_type == MTLFunctionTypeFragment;
  case METAL_FUNCTION_TYPE_COMPUTE:
    return mtl_type == MTLFunctionTypeKernel;
  default:
    return false;
  }
}

uint32_t metal_function_get_buffer_bindings(MTLFunctionRef function_ref,
                                            uint32_t *out_indices,
                                            uint32_t max_count) {

  if (!function_ref || !out_indices || max_count == 0) {
    return 0;
  }

  id<MTLFunction> function = (__bridge id<MTLFunction>)function_ref;
  NSArray<MTLArgument *> *arguments = function.vertexAttributes ?: @[];

  uint32_t count = 0;
  for (id<MTLBinding> arg in function.vertexAttributes ?: @[]) {
    if (arg.type == MTLBindingTypeBuffer && count < max_count) {
      out_indices[count++] = (uint32_t)arg.index;
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
  NSArray<MTLArgument *> *arguments = function.vertexAttributes ?: @[];

  uint32_t count = 0;
  for (id<MTLBinding> arg in function.vertexAttributes ?: @[]) {
    if (arg.type == MTLBindingTypeTexture && count < max_count) {
      out_indices[count++] = (uint32_t)arg.index;
    }
  }

  return count;
}

/* ============================================================================
 * SPECIALIZATION CONSTANTS
 * ============================================================================
 */

MTLFunctionRef metal_function_create_specialized(MTLLibraryRef library_ref,
                                                 const char *name,
                                                 const void *constant_values,
                                                 size_t constant_values_size) {

  if (!library_ref || !name) {
    return NULL;
  }

  id<MTLLibrary> library = (__bridge id<MTLLibrary>)library_ref;
  NSString *functionName = [NSString stringWithUTF8String:name];

  // Create function constant values
  MTLFunctionConstantValues *constantValues =
      [[MTLFunctionConstantValues alloc] init];

  // Note: This is a simplified implementation
  // In a real implementation, you would need to know the constant indices and
  // types For now, we just log that specialization was requested
  if (constant_values && constant_values_size > 0) {
    NSLog(@"Specialization constants requested but not fully implemented");
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
 * HOT RELOAD
 * ============================================================================
 */

bool metal_library_manager_check_for_changes(
    metal_shader_library_manager_t *manager) {
  if (!manager) {
    return false;
  }

  bool has_changes = false;

  for (uint32_t i = 0; i < manager->library_count; i++) {
    metal_library_entry_t *entry = &manager->libraries[i];
    if (!entry->in_use || entry->file_mod_time == 0) {
      continue; // Skip embedded libraries
    }

    uint64_t current_mod_time = get_file_mod_time(entry->path);
    if (current_mod_time > entry->file_mod_time) {
      has_changes = true;
      NSLog(@"Library file modified: %s", entry->path);
    }
  }

  return has_changes;
}

uint32_t
metal_library_manager_reload_changed(metal_shader_library_manager_t *manager) {
  if (!manager) {
    return 0;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)manager->device;
  uint32_t reloaded_count = 0;

  for (uint32_t i = 0; i < manager->library_count; i++) {
    metal_library_entry_t *entry = &manager->libraries[i];
    if (!entry->in_use || entry->file_mod_time == 0) {
      continue; // Skip embedded libraries
    }

    uint64_t current_mod_time = get_file_mod_time(entry->path);
    if (current_mod_time > entry->file_mod_time) {
      // Reload library
      NSString *nsPath = [NSString stringWithUTF8String:entry->path];
      NSURL *url = [NSURL fileURLWithPath:nsPath];
      NSError *error = nil;
      id<MTLLibrary> new_library = [device newLibraryWithURL:url error:&error];

      if (error || !new_library) {
        NSLog(@"Failed to reload library %s: %@", entry->path, error);
        continue;
      }

      // Release old library
      if (entry->library) {
        CFRelease(entry->library);
      }

      // Update entry
      entry->library = (__bridge_retained MTLLibraryRef)new_library;
      entry->file_mod_time = current_mod_time;
      entry->function_count = (uint32_t)[[new_library functionNames] count];

      NSLog(@"Reloaded library: %s (%u functions)", entry->path,
            entry->function_count);
      reloaded_count++;
    }
  }

  return reloaded_count;
}

/* ============================================================================
 * STATISTICS
 * ============================================================================
 */

metal_library_stats_t
metal_library_manager_get_stats(const metal_shader_library_manager_t *manager) {

  metal_library_stats_t stats = {0};

  if (!manager) {
    return stats;
  }

  stats.total_libraries = manager->library_count;
  stats.total_memory_bytes = manager->total_memory_bytes;

  // Count total functions
  for (uint32_t i = 0; i < manager->library_count; i++) {
    if (manager->libraries[i].in_use) {
      stats.total_functions += manager->libraries[i].function_count;
    }
  }

  return stats;
}

void metal_library_manager_print_stats(
    const metal_shader_library_manager_t *manager) {

  if (!manager) {
    return;
  }

  metal_library_stats_t stats = metal_library_manager_get_stats(manager);

  NSLog(@"=== Shader Library Manager Statistics ===");
  NSLog(@"Total Libraries: %u", stats.total_libraries);
  NSLog(@"Total Functions: %u", stats.total_functions);
  NSLog(@"Total Memory: %.2f MB", stats.total_memory_bytes / (1024.0 * 1024.0));
  NSLog(@"Hot Reloads: %u", stats.hot_reloads);

  // List all loaded libraries
  NSLog(@"\nLoaded Libraries:");
  for (uint32_t i = 0; i < manager->library_count; i++) {
    const metal_library_entry_t *entry = &manager->libraries[i];
    if (entry->in_use) {
      NSLog(@"  [%u] %s (%u functions)", i, entry->path, entry->function_count);
    }
  }
}

/* End of mtl_shader_library.m */
