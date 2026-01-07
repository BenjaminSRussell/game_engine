/*
 * mtl_sampler.c
 * Metal sampler implementation
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#import <Metal/Metal.h>
#include "mtl_sampler.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

#define MAX_CACHED_SAMPLERS 128

typedef struct cached_sampler {
    metal_sampler_t* sampler;
    uint32_t hash;
    uint32_t ref_count;
    bool in_use;
} cached_sampler_t;

static struct {
    metal_device_t* device;
    cached_sampler_t cache[MAX_CACHED_SAMPLERS];
    uint32_t cache_count;
    metal_sampler_cache_stats_t stats;
    bool initialized;
    pthread_mutex_t mutex;
} g_sampler_cache = {0};

static void ensure_cache_initialized(void) {
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ^{
        pthread_mutex_init(&g_sampler_cache.mutex, NULL);
    });
}

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

// Simple hash function for sampler descriptors
static uint32_t hash_sampler_desc(const metal_sampler_desc_t* desc) {
    uint32_t hash = 2166136261u;  // FNV-1a offset basis
    
    hash ^= (uint32_t)desc->min_filter;
    hash *= 16777619u;  // FNV-1a prime
    
    hash ^= (uint32_t)desc->mag_filter;
    hash *= 16777619u;
    
    hash ^= (uint32_t)desc->mip_filter;
    hash *= 16777619u;
    
    hash ^= (uint32_t)desc->address_mode_u;
    hash *= 16777619u;
    
    hash ^= (uint32_t)desc->address_mode_v;
    hash *= 16777619u;
    
    hash ^= (uint32_t)desc->address_mode_w;
    hash *= 16777619u;
    
    hash ^= desc->max_anisotropy;
    hash *= 16777619u;
    
    hash ^= (uint32_t)desc->compare_function;
    hash *= 16777619u;
    
    hash ^= desc->compare_enabled ? 1 : 0;
    hash *= 16777619u;
    
    hash ^= desc->normalized_coordinates ? 1 : 0;
    hash *= 16777619u;
    
    return hash;
}

static MTLSamplerMinMagFilter convert_filter(metal_filter_mode_t filter) {
    switch (filter) {
        case METAL_FILTER_NEAREST:
            return MTLSamplerMinMagFilterNearest;
        case METAL_FILTER_LINEAR:
            return MTLSamplerMinMagFilterLinear;
        default:
            return MTLSamplerMinMagFilterLinear;
    }
}

static MTLSamplerMipFilter convert_mip_filter(metal_filter_mode_t filter) {
    switch (filter) {
        case METAL_FILTER_NEAREST:
            return MTLSamplerMipFilterNearest;
        case METAL_FILTER_LINEAR:
            return MTLSamplerMipFilterLinear;
        default:
            return MTLSamplerMipFilterLinear;
    }
}

static MTLSamplerAddressMode convert_address_mode(metal_address_mode_t mode) {
    switch (mode) {
        case METAL_ADDRESS_CLAMP_TO_EDGE:
            return MTLSamplerAddressModeClampToEdge;
        case METAL_ADDRESS_REPEAT:
            return MTLSamplerAddressModeRepeat;
        case METAL_ADDRESS_MIRROR_REPEAT:
            return MTLSamplerAddressModeMirrorRepeat;
        case METAL_ADDRESS_CLAMP_TO_ZERO:
            return MTLSamplerAddressModeClampToZero;
        case METAL_ADDRESS_CLAMP_TO_BORDER:
#if TARGET_OS_OSX
            return MTLSamplerAddressModeClampToBorderColor;
#else
            return MTLSamplerAddressModeClampToEdge;  // iOS doesn't support border
#endif
        default:
            return MTLSamplerAddressModeClampToEdge;
    }
}

static MTLCompareFunction convert_compare_function(metal_compare_function_t func) {
    switch (func) {
        case METAL_COMPARE_NEVER:
            return MTLCompareFunctionNever;
        case METAL_COMPARE_LESS:
            return MTLCompareFunctionLess;
        case METAL_COMPARE_EQUAL:
            return MTLCompareFunctionEqual;
        case METAL_COMPARE_LESS_EQUAL:
            return MTLCompareFunctionLessEqual;
        case METAL_COMPARE_GREATER:
            return MTLCompareFunctionGreater;
        case METAL_COMPARE_NOT_EQUAL:
            return MTLCompareFunctionNotEqual;
        case METAL_COMPARE_GREATER_EQUAL:
            return MTLCompareFunctionGreaterEqual;
        case METAL_COMPARE_ALWAYS:
            return MTLCompareFunctionAlways;
        default:
            return MTLCompareFunctionLess;
    }
}

#if TARGET_OS_OSX
static MTLSamplerBorderColor convert_border_color(metal_border_color_t color) {
    switch (color) {
        case METAL_BORDER_COLOR_TRANSPARENT_BLACK:
            return MTLSamplerBorderColorTransparentBlack;
        case METAL_BORDER_COLOR_OPAQUE_BLACK:
            return MTLSamplerBorderColorOpaqueBlack;
        case METAL_BORDER_COLOR_OPAQUE_WHITE:
            return MTLSamplerBorderColorOpaqueWhite;
        default:
            return MTLSamplerBorderColorTransparentBlack;
    }
}
#endif

/* ============================================================================
 * SAMPLER LIFECYCLE
 * ============================================================================ */

metal_sampler_t* metal_sampler_create(metal_device_t* device, const metal_sampler_desc_t* desc) {
    if (!device || !desc) {
        return NULL;
    }
    
    ensure_cache_initialized();
    pthread_mutex_lock(&g_sampler_cache.mutex);
    
    // Initialize cache on first use
    if (!g_sampler_cache.initialized) {
        g_sampler_cache.device = device;
        g_sampler_cache.initialized = true;
    }
    
    // Calculate hash for caching
    uint32_t hash = hash_sampler_desc(desc);
    
    // Check cache
    for (uint32_t i = 0; i < g_sampler_cache.cache_count; i++) {
        if (g_sampler_cache.cache[i].hash == hash && g_sampler_cache.cache[i].in_use) {
            // Found matching sampler
            g_sampler_cache.cache[i].ref_count++;
            g_sampler_cache.stats.cache_hits++;
            metal_sampler_t* result = g_sampler_cache.cache[i].sampler;
            pthread_mutex_unlock(&g_sampler_cache.mutex);
            return result;
        }
    }
    
    // Not in cache, create new sampler
    MTLSamplerDescriptor* mtl_desc = [[MTLSamplerDescriptor alloc] init];
    
    // Set filter modes
    mtl_desc.minFilter = convert_filter(desc->min_filter);
    mtl_desc.magFilter = convert_filter(desc->mag_filter);
    mtl_desc.mipFilter = convert_mip_filter(desc->mip_filter);
    
    // Set address modes
    mtl_desc.sAddressMode = convert_address_mode(desc->address_mode_u);
    mtl_desc.tAddressMode = convert_address_mode(desc->address_mode_v);
    mtl_desc.rAddressMode = convert_address_mode(desc->address_mode_w);
    
    // Set LOD parameters
    mtl_desc.lodMinClamp = desc->min_lod;
    mtl_desc.lodMaxClamp = desc->max_lod;
    mtl_desc.lodAverage = desc->lod_bias != 0.0f;  // Enable LOD bias if non-zero
    
    // Set anisotropy
    if (desc->max_anisotropy > 1) {
        mtl_desc.maxAnisotropy = desc->max_anisotropy;
    }
    
    // Set comparison for shadow sampling
    if (desc->compare_enabled) {
        mtl_desc.compareFunction = convert_compare_function(desc->compare_function);
    }
    
    // Set border color (macOS only)
#if TARGET_OS_OSX
    if (desc->address_mode_u == METAL_ADDRESS_CLAMP_TO_BORDER ||
        desc->address_mode_v == METAL_ADDRESS_CLAMP_TO_BORDER ||
        desc->address_mode_w == METAL_ADDRESS_CLAMP_TO_BORDER) {
        mtl_desc.borderColor = convert_border_color(desc->border_color);
    }
#endif
    
    // Set normalized coordinates
    mtl_desc.normalizedCoordinates = desc->normalized_coordinates;
    
    // Set label
    if (desc->label) {
        mtl_desc.label = [NSString stringWithUTF8String:desc->label];
    }
    
    // Create Metal sampler state
    id<MTLSamplerState> mtl_sampler = [device->device newSamplerStateWithDescriptor:mtl_desc];
    if (!mtl_sampler) {
        return NULL;
    }
    
    // Create wrapper
    metal_sampler_t* sampler = (metal_sampler_t*)calloc(1, sizeof(metal_sampler_t));
    if (!sampler) {
        return NULL;
    }
    
    sampler->sampler = mtl_sampler;
    sampler->hash = hash;
    if (desc->label) {
        strncpy(sampler->label, desc->label, sizeof(sampler->label) - 1);
    }
    
    // Add to cache if space available
    if (g_sampler_cache.cache_count < MAX_CACHED_SAMPLERS) {
        cached_sampler_t* cached = &g_sampler_cache.cache[g_sampler_cache.cache_count++];
        cached->sampler = sampler;
        cached->hash = hash;
        cached->ref_count = 1;
        cached->in_use = true;
        
        g_sampler_cache.stats.total_samplers++;
        g_sampler_cache.stats.memory_used += sizeof(metal_sampler_t);
    }
    
    g_sampler_cache.stats.cache_misses++;
    
    pthread_mutex_unlock(&g_sampler_cache.mutex);
    return sampler;
}

void metal_sampler_destroy(metal_sampler_t* sampler) {
    if (!sampler) {
        return;
    }
    
    pthread_mutex_lock(&g_sampler_cache.mutex);
    
    // Check cache and decrement ref count
    for (uint32_t i = 0; i < g_sampler_cache.cache_count; i++) {
        if (g_sampler_cache.cache[i].sampler == sampler) {
            g_sampler_cache.cache[i].ref_count--;
            
            // Only actually destroy if ref count reaches 0
            if (g_sampler_cache.cache[i].ref_count == 0) {
                g_sampler_cache.cache[i].in_use = false;
                sampler->sampler = nil;
                free(sampler);
                g_sampler_cache.stats.total_samplers--;
                g_sampler_cache.stats.memory_used -= sizeof(metal_sampler_t);
            }
            pthread_mutex_unlock(&g_sampler_cache.mutex);
            return;
        }
    }
    
    // Not in cache, just destroy
    sampler->sampler = nil;
    free(sampler);
    pthread_mutex_unlock(&g_sampler_cache.mutex);
}

/* ============================================================================
 * PREDEFINED SAMPLERS
 * ============================================================================ */

metal_sampler_t* metal_sampler_create_point_clamp(metal_device_t* device) {
    metal_sampler_desc_t desc = {
        .min_filter = METAL_FILTER_NEAREST,
        .mag_filter = METAL_FILTER_NEAREST,
        .mip_filter = METAL_FILTER_NEAREST,
        .address_mode_u = METAL_ADDRESS_CLAMP_TO_EDGE,
        .address_mode_v = METAL_ADDRESS_CLAMP_TO_EDGE,
        .address_mode_w = METAL_ADDRESS_CLAMP_TO_EDGE,
        .min_lod = 0.0f,
        .max_lod = 1000.0f,
        .lod_bias = 0.0f,
        .max_anisotropy = 1,
        .compare_enabled = false,
        .normalized_coordinates = true,
        .label = "Point Clamp"
    };
    return metal_sampler_create(device, &desc);
}

metal_sampler_t* metal_sampler_create_linear_clamp(metal_device_t* device) {
    metal_sampler_desc_t desc = {
        .min_filter = METAL_FILTER_LINEAR,
        .mag_filter = METAL_FILTER_LINEAR,
        .mip_filter = METAL_FILTER_LINEAR,
        .address_mode_u = METAL_ADDRESS_CLAMP_TO_EDGE,
        .address_mode_v = METAL_ADDRESS_CLAMP_TO_EDGE,
        .address_mode_w = METAL_ADDRESS_CLAMP_TO_EDGE,
        .min_lod = 0.0f,
        .max_lod = 1000.0f,
        .lod_bias = 0.0f,
        .max_anisotropy = 1,
        .compare_enabled = false,
        .normalized_coordinates = true,
        .label = "Linear Clamp"
    };
    return metal_sampler_create(device, &desc);
}

metal_sampler_t* metal_sampler_create_linear_repeat(metal_device_t* device) {
    metal_sampler_desc_t desc = {
        .min_filter = METAL_FILTER_LINEAR,
        .mag_filter = METAL_FILTER_LINEAR,
        .mip_filter = METAL_FILTER_LINEAR,
        .address_mode_u = METAL_ADDRESS_REPEAT,
        .address_mode_v = METAL_ADDRESS_REPEAT,
        .address_mode_w = METAL_ADDRESS_REPEAT,
        .min_lod = 0.0f,
        .max_lod = 1000.0f,
        .lod_bias = 0.0f,
        .max_anisotropy = 1,
        .compare_enabled = false,
        .normalized_coordinates = true,
        .label = "Linear Repeat"
    };
    return metal_sampler_create(device, &desc);
}

metal_sampler_t* metal_sampler_create_anisotropic(metal_device_t* device, uint32_t max_aniso) {
    // Clamp to valid range
    if (max_aniso < 1) max_aniso = 1;
    if (max_aniso > 16) max_aniso = 16;
    
    metal_sampler_desc_t desc = {
        .min_filter = METAL_FILTER_LINEAR,
        .mag_filter = METAL_FILTER_LINEAR,
        .mip_filter = METAL_FILTER_LINEAR,
        .address_mode_u = METAL_ADDRESS_REPEAT,
        .address_mode_v = METAL_ADDRESS_REPEAT,
        .address_mode_w = METAL_ADDRESS_REPEAT,
        .min_lod = 0.0f,
        .max_lod = 1000.0f,
        .lod_bias = 0.0f,
        .max_anisotropy = max_aniso,
        .compare_enabled = false,
        .normalized_coordinates = true,
        .label = "Anisotropic"
    };
    return metal_sampler_create(device, &desc);
}

metal_sampler_t* metal_sampler_create_shadow(metal_device_t* device) {
    metal_sampler_desc_t desc = {
        .min_filter = METAL_FILTER_LINEAR,
        .mag_filter = METAL_FILTER_LINEAR,
        .mip_filter = METAL_FILTER_LINEAR,
        .address_mode_u = METAL_ADDRESS_CLAMP_TO_EDGE,
        .address_mode_v = METAL_ADDRESS_CLAMP_TO_EDGE,
        .address_mode_w = METAL_ADDRESS_CLAMP_TO_EDGE,
        .min_lod = 0.0f,
        .max_lod = 0.0f,  // No mipmaps for shadow maps
        .lod_bias = 0.0f,
        .max_anisotropy = 1,
        .compare_function = METAL_COMPARE_LESS,
        .compare_enabled = true,
        .normalized_coordinates = true,
        .label = "Shadow Sampler"
    };
    return metal_sampler_create(device, &desc);
}

/* ============================================================================
 * SAMPLER CACHE
 * ============================================================================ */

void metal_sampler_cache_clear(void) {
    pthread_mutex_lock(&g_sampler_cache.mutex);
    
    for (uint32_t i = 0; i < g_sampler_cache.cache_count; i++) {
        if (g_sampler_cache.cache[i].sampler) {
            g_sampler_cache.cache[i].sampler->sampler = nil;
            free(g_sampler_cache.cache[i].sampler);
        }
    }
    
    g_sampler_cache.cache_count = 0;
    g_sampler_cache.stats.total_samplers = 0;
    g_sampler_cache.stats.memory_used = 0;
    
    pthread_mutex_unlock(&g_sampler_cache.mutex);
}

metal_sampler_cache_stats_t metal_sampler_cache_get_stats(void) {
    pthread_mutex_lock(&g_sampler_cache.mutex);
    metal_sampler_cache_stats_t stats = g_sampler_cache.stats;
    pthread_mutex_unlock(&g_sampler_cache.mutex);
    return stats;
}

/* End of mtl_sampler.c */
