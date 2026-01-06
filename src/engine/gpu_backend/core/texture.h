#ifndef RENDERER_CORE_TEXTURE_DEFINITIONS_H
#define RENDERER_CORE_TEXTURE_DEFINITIONS_H

#include "core/types.h"
#include "renderer/texture_system.h" // For Texture struct and TextureFormat

#ifdef __cplusplus
extern "C" {
#endif

// Texture usage flags - Missing from texture_system.h
typedef enum TextureUsage {
    TEXTURE_USAGE_SAMPLED = BIT(0),
    TEXTURE_USAGE_STORAGE = BIT(1),
    TEXTURE_USAGE_RENDER_TARGET = BIT(2),
    TEXTURE_USAGE_DEPTH_STENCIL = BIT(3),
    TEXTURE_USAGE_TRANSFER_SRC = BIT(4),
    TEXTURE_USAGE_TRANSFER_DST = BIT(5),
    TEXTURE_USAGE_COLOR_ATTACHMENT = BIT(2), // Alias for RENDER_TARGET
} TextureUsage;

typedef u32 TextureUsageFlags;

// Texture handle - compatibility with RenderGraph which expects TextureID type
typedef struct TextureID {
    u32 id;
} TextureID;

// Creation info - Missing from texture_system.h
typedef struct TextureCreateInfo {
    u32 width;
    u32 height;
    u32 depth;
    TextureFormat format;
    TextureUsageFlags usage;
    u32 mip_levels;
    u32 sample_count;
    const char *name;
} TextureCreateInfo;

// API Stubs bridging to texture_system.h or internal impl
TextureID texture_get_id(Texture *texture);
Texture *texture_create(const TextureCreateInfo *info);
void texture_destroy(Texture *texture);

#ifdef __cplusplus
}
#endif

#endif // RENDERER_CORE_TEXTURE_H
