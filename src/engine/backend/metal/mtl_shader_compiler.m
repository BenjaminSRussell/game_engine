/*
 * mtl_shader_compiler.m
 * Metal Shader Runtime Compilation Implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#include "mtl_shader_compiler.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * UTILITIES
 * ============================================================================
 */

// Simple FNV-1a hash for source code
static uint64_t hash_source(const char *source) {
  const uint64_t FNV_OFFSET = 14695981039346656037ULL;
  const uint64_t FNV_PRIME = 1099511628211ULL;

  uint64_t hash = FNV_OFFSET;
  const char *p = source;
  while (*p) {
    hash ^= (uint64_t)(*p++);
    hash *= FNV_PRIME;
  }
  return hash;
}

/* ============================================================================
 * COMPILER INITIALIZATION
 * ============================================================================
 */

metal_shader_compiler_t *metal_shader_compiler_create(MTLDeviceRef device_ref) {
  if (!device_ref) {
    return NULL;
  }

  metal_shader_compiler_t *compiler =
      (metal_shader_compiler_t *)calloc(1, sizeof(metal_shader_compiler_t));

  if (!compiler) {
    return NULL;
  }

  compiler->device = device_ref;
  compiler->cache_count = 0;
  compiler->compile_count = 0;
  compiler->cache_hits = 0;

  // Initialize default options
  metal_compile_options_init(&compiler->default_options);

  return compiler;
}

void metal_shader_compiler_destroy(metal_shader_compiler_t *compiler) {
  if (!compiler) {
    return;
  }

  // Release all cached shaders
  for (uint32_t i = 0; i < compiler->cache_count; i++) {
    metal_shader_cache_entry_t *entry = &compiler->cache[i];
    if (entry->in_use && entry->library) {
      CFRelease(entry->library);
      entry->library = NULL;
      entry->in_use = false;
    }
  }

  free(compiler);
}

void metal_shader_compiler_set_default_options(
    metal_shader_compiler_t *compiler, const metal_compile_options_t *options) {

  if (!compiler || !options) {
    return;
  }

  compiler->default_options = *options;
}

/* ============================================================================
 * COMPILE OPTIONS
 * ============================================================================
 */

void metal_compile_options_init(metal_compile_options_t *options) {
  if (!options) {
    return;
  }

  memset(options, 0, sizeof(metal_compile_options_t));
  options->optimization_level = METAL_OPTIMIZATION_DEFAULT;
  options->fast_math_enabled = true;
  options->enable_debug_info = false;
  options->preprocessor_defines = NULL;
  options->preprocessor_define_count = 0;
  options->language_version = NULL; // Use default
}

static MTLCompileOptions *
create_mtl_compile_options(const metal_compile_options_t *options) {
  MTLCompileOptions *mtlOptions = [[MTLCompileOptions alloc] init];

  if (!options) {
    return mtlOptions;
  }

  // Set optimization level
  switch (options->optimization_level) {
  case METAL_OPTIMIZATION_NONE:
    if (@available(macOS 15.0, *)) {
      mtlOptions.mathMode = MTLMathModeSafe;
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      mtlOptions.fastMathEnabled = NO;
#pragma clang diagnostic pop
    }
    break;
  case METAL_OPTIMIZATION_SIZE:
  case METAL_OPTIMIZATION_SPEED:
  case METAL_OPTIMIZATION_DEFAULT:
  default:
    if (@available(macOS 15.0, *)) {
      mtlOptions.mathMode =
          options->fast_math_enabled ? MTLMathModeFast : MTLMathModeSafe;
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      mtlOptions.fastMathEnabled = options->fast_math_enabled ? YES : NO;
#pragma clang diagnostic pop
    }
    break;
  }

  // Preprocessor defines
  if (options->preprocessor_defines && options->preprocessor_define_count > 0) {
    NSMutableDictionary *defines = [NSMutableDictionary dictionary];
    for (uint32_t i = 0; i < options->preprocessor_define_count; i++) {
      const char *define = options->preprocessor_defines[i];

      // Parse "KEY=VALUE" or "KEY"
      char key[256] = {0};
      char value[256] = {0};

      const char *equals = strchr(define, '=');
      if (equals) {
        size_t key_len = equals - define;
        strncpy(key, define, key_len < 255 ? key_len : 255);
        strncpy(value, equals + 1, 255);
      } else {
        strncpy(key, define, 255);
        strcpy(value, "1");
      }

      NSString *nsKey = [NSString stringWithUTF8String:key];
      NSString *nsValue = [NSString stringWithUTF8String:value];
      defines[nsKey] = nsValue;
    }
    mtlOptions.preprocessorMacros = defines;
  }

  // Language version
  if (options->language_version) {
    NSString *version =
        [NSString stringWithUTF8String:options->language_version];

    if ([version isEqualToString:@"2.4"]) {
      mtlOptions.languageVersion = MTLLanguageVersion2_4;
    } else if ([version isEqualToString:@"3.0"]) {
      if (@available(macOS 12.0, *)) {
        mtlOptions.languageVersion = MTLLanguageVersion3_0;
      }
    }
    // Add more version mappings as needed
  }

  return mtlOptions;
}

/* ============================================================================
 * COMPILATION
 * ============================================================================
 */

static metal_compile_result_t
compile_internal(metal_shader_compiler_t *compiler, const char *source,
                 const char *debug_name,
                 const metal_compile_options_t *options) {

  metal_compile_result_t result = {0};
  result.library = NULL;
  result.succeeded = false;
  result.error_count = 0;

  if (!compiler || !source) {
             "Invalid arguments");
    return result;
  }

  // Check cache first
  uint64_t source_hash = hash_source(source);
  for (uint32_t i = 0; i < compiler->cache_count; i++) {
    metal_shader_cache_entry_t *entry = &compiler->cache[i];
    if (entry->in_use && entry->source_hash == source_hash) {
      // Verify with actual source comparison to avoid hash collision issues
      if (strcmp(entry->source, source) == 0) {
        NSLog(@"Shader cache hit: %s", debug_name ? debug_name : "<unknown>");
        compiler->cache_hits++;
        result.library = entry->library;
        result.succeeded = true;
        return result;
      } else {
        // Hash collision - different source with same hash
        NSLog(@"Warning: Shader cache hash collision detected for %s",
              debug_name ? debug_name : "<unknown>");
      }
    }
  }

  // Compile
  id<MTLDevice> device = (__bridge id<MTLDevice>)compiler->device;
  NSString *sourceString = [NSString stringWithUTF8String:source];

  const metal_compile_options_t *compile_opts =
      options ? options : &compiler->default_options;
  MTLCompileOptions *mtlOptions = create_mtl_compile_options(compile_opts);

  NSError *error = nil;
  id<MTLLibrary> library = [device newLibraryWithSource:sourceString
                                                options:mtlOptions
                                                  error:&error];

  compiler->compile_count++;

  if (error || !library) {
    // Parse error message
    NSString *errorDesc = [error localizedDescription];
    const char *errorCStr = [errorDesc UTF8String];

    if (result.error_count < METAL_MAX_COMPILE_ERRORS) {
      metal_compile_error_t *compile_error =
          &result.errors[result.error_count++];
      strncpy(compile_error->message, errorCStr,
              sizeof(compile_error->message) - 1);
      compile_error->line =
          0; // Metal doesn't always provide line numbers easily
      compile_error->column = 0;
    }

    NSLog(@"Shader compilation failed for %s: %@",
          debug_name ? debug_name : "<unknown>", error);

    result.succeeded = false;
    return result;
  }

  // Success - add to cache
  MTLLibraryRef library_ref = (__bridge_retained MTLLibraryRef)library;
  result.library = library_ref;
  result.succeeded = true;

  if (compiler->cache_count < METAL_MAX_CACHED_SHADERS) {
    metal_shader_cache_entry_t *entry =
        &compiler->cache[compiler->cache_count++];
    entry->source_hash = source_hash;
    entry->library = library_ref;
    strncpy(entry->debug_name, debug_name ? debug_name : "<compiled>",
            sizeof(entry->debug_name) - 1);
    entry->in_use = true;
  } else {
    NSLog(@"Shader cache full, not caching compiled shader");
  }

  NSLog(@"Shader compiled successfully: %s",
        debug_name ? debug_name : "<unknown>");

  return result;
}

metal_compile_result_t
metal_shader_compile_file(metal_shader_compiler_t *compiler, const char *path,
                          const metal_compile_options_t *options) {

  metal_compile_result_t result = {0};
  result.succeeded = false;

  if (!compiler || !path) {
             "Invalid arguments");
    return result;
  }

  // Read file
  FILE *file = fopen(path, "r");
  if (!file) {
             "Failed to open file: %s", path);
    return result;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *source = (char *)malloc(size + 1);
  if (!source) {
    fclose(file);
    return result;
  }

  size_t read_size = fread(source, 1, size, file);
  source[read_size] = '\0';
  fclose(file);

  // Compile
  result = compile_internal(compiler, source, path, options);

  free(source);
  return result;
}

metal_compile_result_t
metal_shader_compile_source(metal_shader_compiler_t *compiler,
                            const char *source, const char *debug_name,
                            const metal_compile_options_t *options) {

  return compile_internal(compiler, source, debug_name, options);
}

void metal_compile_result_free(metal_compile_result_t *result) {
  if (!result) {
    return;
  }

  // Note: We don't release the library here because it may be cached
  // The cache owns the library reference
  result->library = NULL;
  result->succeeded = false;
}

/* ============================================================================
 * CACHE MANAGEMENT
 * ============================================================================
 */

MTLLibraryRef metal_shader_cache_get(metal_shader_compiler_t *compiler,
                                     const char *source) {

  if (!compiler || !source) {
    return NULL;
  }

  uint64_t source_hash = hash_source(source);

  for (uint32_t i = 0; i < compiler->cache_count; i++) {
    metal_shader_cache_entry_t *entry = &compiler->cache[i];
    if (entry->in_use && entry->source_hash == source_hash) {
      return entry->library;
    }
  }

  return NULL;
}

void metal_shader_cache_add(metal_shader_compiler_t *compiler,
                            const char *source, MTLLibraryRef library,
                            const char *debug_name) {

  if (!compiler || !source || !library) {
    return;
  }

  if (compiler->cache_count >= METAL_MAX_CACHED_SHADERS) {
    NSLog(@"Shader cache full, cannot add more entries");
    return;
  }

  uint64_t source_hash = hash_source(source);

  metal_shader_cache_entry_t *entry = &compiler->cache[compiler->cache_count++];
  entry->source_hash = source_hash;
  entry->library = library;
  strncpy(entry->debug_name, debug_name ? debug_name : "<manual>",
          sizeof(entry->debug_name) - 1);
  entry->in_use = true;
}

void metal_shader_cache_clear(metal_shader_compiler_t *compiler) {
  if (!compiler) {
    return;
  }

  for (uint32_t i = 0; i < compiler->cache_count; i++) {
    metal_shader_cache_entry_t *entry = &compiler->cache[i];
    if (entry->in_use && entry->library) {
      CFRelease(entry->library);
      entry->library = NULL;
      entry->in_use = false;
    }
  }

  compiler->cache_count = 0;
  NSLog(@"Shader cache cleared");
}

/* ============================================================================
 * STATISTICS
 * ============================================================================
 */

void metal_shader_compiler_get_stats(const metal_shader_compiler_t *compiler,
                                     uint32_t *out_compile_count,
                                     uint32_t *out_cache_hits,
                                     uint32_t *out_cached_shaders) {

  if (!compiler) {
    if (out_compile_count)
      *out_compile_count = 0;
    if (out_cache_hits)
      *out_cache_hits = 0;
    if (out_cached_shaders)
      *out_cached_shaders = 0;
    return;
  }

  if (out_compile_count)
    *out_compile_count = compiler->compile_count;
  if (out_cache_hits)
    *out_cache_hits = compiler->cache_hits;
  if (out_cached_shaders)
    *out_cached_shaders = compiler->cache_count;
}

void metal_shader_compiler_print_stats(
    const metal_shader_compiler_t *compiler) {
  if (!compiler) {
    return;
  }

  NSLog(@"=== Shader Compiler Statistics ===");
  NSLog(@"Total Compilations: %u", compiler->compile_count);
  NSLog(@"Cache Hits: %u", compiler->cache_hits);
  NSLog(@"Cached Shaders: %u", compiler->cache_count);

  float hit_rate = 0.0f;
  if (compiler->compile_count + compiler->cache_hits > 0) {
    hit_rate = (float)compiler->cache_hits /
               (float)(compiler->compile_count + compiler->cache_hits) * 100.0f;
  }
  NSLog(@"Cache Hit Rate: %.1f%%", hit_rate);

  NSLog(@"\nCached Shaders:");
  for (uint32_t i = 0; i < compiler->cache_count; i++) {
    const metal_shader_cache_entry_t *entry = &compiler->cache[i];
    if (entry->in_use) {
      NSLog(@"  [%u] %s (hash: 0x%llx)", i, entry->debug_name,
            entry->source_hash);
    }
  }
}

/* End of mtl_shader_compiler.m */
