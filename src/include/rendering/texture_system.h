// include/render/texture_system.h
//
// Purpose: Advanced texture system with mipmap generation, anisotropic
// filtering, texture streaming, and animation support.
//
#ifndef TEXTURE_SYSTEM_H
#define TEXTURE_SYSTEM_H

#include "engine/include/common.h"
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

// Texture formats
typedef enum {
  TEXFMT_R8,      // 8-bit grayscale
  TEXFMT_RGBA8,   // 32-bit RGBA
  TEXFMT_RGBA16F, // 16-bit float per channel
  TEXFMT_RGBA32F, // 32-bit float per channel
  TEXFMT_DXT1,    // BC1 compression (RGB/1-bit alpha)
  TEXFMT_DXT5,    // BC3 compression (RGBA)
  TEXFMT_BC4,     // BC4 compression (single channel)
  TEXFMT_BC5,     // BC5 compression (normal maps)
  TEXFMT_BC6H,    // BC6H compression (HDR)
  TEXFMT_BC7,     // BC7 compression (high quality)
} TextureFormat;

// Texture filter modes
typedef enum {
  FILTER_NEAREST,     // Nearest neighbor
  FILTER_LINEAR,      // Bilinear
  FILTER_ANISOTROPIC, // Anisotropic (16x)
  FILTER_CUBIC,       // Cubic interpolation
} TextureFilter;

// Texture wrap modes
typedef enum {
  WRAP_CLAMP,  // Clamp to edge
  WRAP_REPEAT, // Repeat/tile
  WRAP_MIRROR, // Mirror repeat
  WRAP_BORDER, // Border color
} TextureWrap;

// Texture type
typedef enum {
  TEXTYPE_2D,    // Standard 2D texture
  TEXTYPE_ARRAY, // Texture array
  TEXTYPE_3D,    // Volume texture
  TEXTYPE_CUBE,  // Cubemap
} TextureType;

// Mipmap info
typedef struct {
  u32 width, height;
  u32 byte_size;
  u32 offset; // Offset in texture data
  u8 *data;   // Mipmap pixel data
} Mipmap;

// Texture metadata
typedef struct Texture {
  u32 texture_id;
  char name[64];
  TextureFormat format;
  TextureFilter filter;
  TextureWrap wrap_u, wrap_v;
  TextureType type;
  u32 width, height, depth;
  u32 mipmap_count;
  Mipmap *mipmaps;
  u32 total_memory; // Total GPU memory used
  bool is_srgb;     // Apply gamma correction
  bool is_compressed;
  f32 anisotropy; // Anisotropic level (1.0-16.0)
} Texture;

// Animated texture data
typedef struct {
  struct Texture base_texture;
  u32 frame_count;
  u32 frames_per_second;
  u32 current_frame;
  f32 animation_time;
  bool is_looping;
  u32 *frame_textures; // IDs of texture frames
} AnimatedTexture;

// Texture library
#define MAX_TEXTURES 2048
#define MAX_ANIMATED_TEXTURES 256

typedef struct {
  struct Texture textures[MAX_TEXTURES];
  u32 texture_count;

  AnimatedTexture animated[MAX_ANIMATED_TEXTURES];
  u32 animated_count;

  // Memory tracking
  u32 total_memory_used;
  u32 max_memory_budget;

  bool initialized;
} TextureLibrary;

// ==============================================================================
// Texture Library Lifecycle
// ==============================================================================

// Initialize texture library
void texture_library_init(TextureLibrary *library, u32 max_memory_mb);

// Shutdown texture library
void texture_library_shutdown(TextureLibrary *library);

// ==============================================================================
// Texture Loading and Creation
// ==============================================================================

// Load texture from PNG/JPG file
u32 texture_load_from_file(TextureLibrary *library, const char *filepath,
                           bool is_srgb);

// Create texture from raw pixel data
u32 texture_create_from_pixels(TextureLibrary *library, const char *name,
                               u8 *pixel_data, u32 width, u32 height,
                               TextureFormat format, bool is_srgb);

// Load texture asynchronously (returns texture ID that becomes valid later)
u32 texture_load_async(TextureLibrary *library, const char *filepath,
                       bool is_srgb);

// Check if async texture is loaded
bool texture_is_async_loaded(TextureLibrary *library, u32 texture_id);

// ==============================================================================
// Texture Retrieval
// ==============================================================================

// Get texture by ID
Texture *texture_get(TextureLibrary *library, u32 texture_id);

// Get texture by name
Texture *texture_get_by_name(TextureLibrary *library, const char *name);

// Get animated texture
AnimatedTexture *texture_get_animated(TextureLibrary *library, u32 anim_id);

// ==============================================================================
// Mipmap Generation
// ==============================================================================

// Generate mipmaps for texture (in-place)
void texture_generate_mipmaps(Texture *texture);

// Generate mipmaps using box filter
void mipmaps_generate_box_filter(Texture *texture, u8 *source_data,
                                 u32 source_width, u32 source_height);

// Generate mipmaps using Lanczos filter (higher quality)
void mipmaps_generate_lanczos(Texture *texture, u8 *source_data,
                              u32 source_width, u32 source_height);

// Generate mipmaps using Kaiser filter (best quality but slower)
void mipmaps_generate_kaiser(Texture *texture, u8 *source_data,
                             u32 source_width, u32 source_height);

// Calculate mipmap count needed
u32 mipmaps_calculate_count(u32 width, u32 height);

// Calculate mipmap resolution
void mipmaps_calculate_resolution(u32 level, u32 base_width, u32 base_height,
                                  u32 *out_width, u32 *out_height);

// ==============================================================================
// Anisotropic Filtering
// ==============================================================================

// Set anisotropic filtering level
void texture_set_anisotropy(Texture *texture, f32 level);

// Get maximum supported anisotropy
f32 texture_get_max_anisotropy(void);

// Enable anisotropic filtering for texture
void texture_enable_anisotropic(Texture *texture, bool enable);

// ==============================================================================
// Filtering and Wrapping
// ==============================================================================

// Set texture filter mode
void texture_set_filter(Texture *texture, TextureFilter filter);

// Set texture wrap mode
void texture_set_wrap(Texture *texture, TextureWrap wrap_u, TextureWrap wrap_v);

// Set texture border color (for WRAP_BORDER mode)
void texture_set_border_color(Texture *texture, Vec4 color);

// ==============================================================================
// Compression
// ==============================================================================

// Compress texture using DXT1/BC1
bool texture_compress_dxt1(Texture *source, Texture *out_compressed);

// Compress texture using DXT5/BC3
bool texture_compress_dxt5(Texture *source, Texture *out_compressed);

// Compress texture using BC7 (highest quality)
bool texture_compress_bc7(Texture *source, Texture *out_compressed);

// Compress texture using BC4 (single channel)
bool texture_compress_bc4(Texture *source, Texture *out_compressed);

// Compress texture using BC5 (normal maps)
bool texture_compress_bc5(Texture *source, Texture *out_compressed);

// Get compression ratio
f32 texture_get_compression_ratio(Texture *texture);

// ==============================================================================
// Animated Textures
// ==============================================================================

// Create animated texture from sprite sheet
u32 texture_create_animated(TextureLibrary *library, const char *name,
                            const char *sprite_sheet_path, u32 frame_width,
                            u32 frame_height, u32 frames_per_second);

// Create animated texture from frame sequence
u32 texture_create_animated_frames(TextureLibrary *library, const char *name,
                                   const char **frame_paths, u32 frame_count,
                                   u32 frames_per_second);

// Update animated texture playback
void texture_update_animation(AnimatedTexture *anim, f32 delta_time);

// Set animation speed
void texture_set_animation_speed(AnimatedTexture *anim, u32 fps);

// Pause/resume animation
void texture_set_animation_paused(AnimatedTexture *anim, bool paused);

// Get current animation frame texture
u32 texture_get_current_frame(AnimatedTexture *anim);

// ==============================================================================
// Memory Management
// ==============================================================================

// Get texture memory usage
u32 texture_get_memory_usage(Texture *texture);

// Get library memory usage
u32 texture_library_get_memory_usage(TextureLibrary *library);

// Clear unused textures
void texture_library_compact(TextureLibrary *library);

// Set memory budget
void texture_library_set_budget(TextureLibrary *library, u32 max_memory_mb);

// Get memory statistics
typedef struct {
  u32 total_memory;
  u32 compressed_memory;
  u32 uncompressed_memory;
  u32 mipmap_memory;
  u32 texture_count;
} TextureStats;

TextureStats texture_library_get_stats(TextureLibrary *library);

// ==============================================================================
// Texture Streaming
// ==============================================================================

// Begin streaming texture (load lower mipmaps first)
void texture_stream_begin(Texture *texture);

// Get streaming progress (0.0-1.0)
f32 texture_get_stream_progress(Texture *texture);

// Cancel streaming
void texture_stream_cancel(Texture *texture);

// ==============================================================================
// Quality Control
// ==============================================================================

// Set global anisotropic filtering quality
void texture_set_global_anisotropy(f32 level);

// Set mipmap filter quality
void texture_set_mipmap_quality(TextureFilter filter);

// Enable/disable mipmap generation
void texture_set_mipmaps_enabled(bool enabled);

// ==============================================================================
// Debug and Analysis
// ==============================================================================

// Log texture information
void texture_log_info(Texture *texture);

// Log library statistics
void texture_library_log_stats(TextureLibrary *library);

// Validate texture data
bool texture_validate(Texture *texture);

// Find largest texture
Texture *texture_library_find_largest(TextureLibrary *library);

// ==============================================================================
// Serialization
// ==============================================================================

// Export texture to file
bool texture_save_to_file(Texture *texture, const char *filepath);

// Import texture from file
bool texture_load_from_file_raw(const char *filepath, Texture *out_texture);

#endif // TEXTURE_SYSTEM_H
