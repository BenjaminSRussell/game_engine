/*
 * hzb_builder.h
 * Hierarchical Z-Buffer Construction
 */

#ifndef CULLING_HZB_BUILDER_H
#define CULLING_HZB_BUILDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct hzb_builder {
#ifdef __OBJC__
    id<MTLTexture> hzb_texture;
    id<MTLComputePipelineState> reduce_pipeline;
    uint32_t mip_levels;
#else
    void* hzb_texture;
    void* reduce_pipeline;
    uint32_t mip_levels;
#endif
    uint32_t width;
    uint32_t height;
    bool initialized;
} hzb_builder_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Initialize HZB builder
 */
#ifdef __OBJC__
int hzb_builder_init(hzb_builder_t* hzb, id<MTLDevice> device, uint32_t width, uint32_t height);
#else
int hzb_builder_init(void* hzb, void* device, uint32_t width, uint32_t height);
#endif

/**
 * Shutdown HZB builder
 */
void hzb_builder_shutdown(hzb_builder_t* hzb);

/**
 * Build HZB pyramid from depth buffer
 */
#ifdef __OBJC__
void hzb_builder_build(hzb_builder_t* hzb, id<MTLCommandBuffer> cmd, id<MTLTexture> source_depth);
#else
void hzb_builder_build(void* hzb, void* cmd, void* source_depth);
#endif

/**
 * Resize HZB
 */
#ifdef __OBJC__
void hzb_builder_resize(hzb_builder_t* hzb, id<MTLDevice> device, uint32_t width, uint32_t height);
#else
void hzb_builder_resize(void* hzb, void* device, uint32_t width, uint32_t height);
#endif

/**
 * Get HZB texture
 */
#ifdef __OBJC__
id<MTLTexture> hzb_builder_get_texture(hzb_builder_t* hzb);
#else
void* hzb_builder_get_texture(void* hzb);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CULLING_HZB_BUILDER_H */
