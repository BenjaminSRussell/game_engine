/*
 * mtl_display.h
 * Metal display capability detection and configuration
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_DISPLAY_H
#define PLATFORM_MTL_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <CoreGraphics/CoreGraphics.h>
#else
typedef void* CGColorSpaceRef;
typedef struct CGSize {
    double width;
    double height;
} CGSize;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/**
 * Display information structure containing capabilities and properties.
 */
typedef struct metal_display_info {
    bool supports_hdr;              // HDR/EDR capability
    bool supports_vrr;              // Variable refresh rate (ProMotion)
    double nominal_refresh_rate;    // Standard refresh rate (e.g., 60.0)
    double max_refresh_rate;        // Maximum refresh rate (e.g., 120.0 for ProMotion)
    float edr_max_headroom;         // Extended Dynamic Range max headroom (1.0 = SDR)
    CGColorSpaceRef native_color_space; // Native display color space
    CGSize physical_size_mm;        // Physical display size in millimeters
    int32_t ppi;                    // Pixels per inch
} metal_display_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Query display capabilities and properties.
 * @param ns_screen Pointer to NSScreen (void* for C compatibility), or NULL for main screen.
 * @return Display info structure, or NULL on failure. Must be freed with metal_display_info_destroy().
 */
metal_display_info_t* metal_display_get_info(void* ns_screen);

/**
 * Check if display supports HDR/EDR output.
 * @param info Display info structure.
 * @return true if HDR is supported, false otherwise.
 */
bool metal_display_supports_hdr(const metal_display_info_t* info);

/**
 * Check if display supports variable refresh rate (ProMotion).
 * @param info Display info structure.
 * @return true if VRR is supported, false otherwise.
 */
bool metal_display_supports_vrr(const metal_display_info_t* info);

/**
 * Get current refresh rate of the display.
 * @param info Display info structure.
 * @return Refresh rate in Hz (e.g., 60.0, 120.0).
 */
double metal_display_get_refresh_rate(const metal_display_info_t* info);

/**
 * Get maximum refresh rate of the display.
 * @param info Display info structure.
 * @return Maximum refresh rate in Hz.
 */
double metal_display_get_max_refresh_rate(const metal_display_info_t* info);

/**
 * Get Extended Dynamic Range headroom value.
 * @param info Display info structure.
 * @return EDR headroom multiplier (1.0 = SDR, >1.0 = HDR capable).
 */
float metal_display_get_edr_headroom(const metal_display_info_t* info);

/**
 * Get native color space of the display.
 * @param info Display info structure.
 * @return CGColorSpaceRef (do not release, owned by info structure).
 */
CGColorSpaceRef metal_display_get_color_space(const metal_display_info_t* info);

/**
 * Destroy display info structure and release resources.
 * @param info Display info to destroy.
 */
void metal_display_info_destroy(metal_display_info_t* info);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_DISPLAY_H */
