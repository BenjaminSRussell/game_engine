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

/* ============================================================================
 * MULTI-MONITOR SUPPORT
 * ============================================================================ */

/**
 * Get all available screens.
 * @param out_count Output parameter for number of screens.
 * @return Array of NSScreen pointers (void* for C). Must be freed with metal_display_free_screens().
 */
void** metal_display_get_all_screens(uint32_t* out_count);

/**
 * Free screen array returned by metal_display_get_all_screens.
 * @param screens Screen array to free.
 */
void metal_display_free_screens(void** screens);

/**
 * Get display info for a specific screen index.
 * @param screen_index Index of the screen (0 = main screen).
 * @return Display info structure, or NULL on failure. Must be freed with metal_display_info_destroy().
 */
metal_display_info_t* metal_display_get_info_by_index(uint32_t screen_index);

/**
 * Check if screen is the main display.
 * @param ns_screen Pointer to NSScreen.
 * @return true if main display, false otherwise.
 */
bool metal_display_is_main(void* ns_screen);

/**
 * Check if screen is a built-in display.
 * @param ns_screen Pointer to NSScreen.
 * @return true if built-in, false if external.
 */
bool metal_display_is_builtin(void* ns_screen);

/* ============================================================================
 * SCREEN CHANGE NOTIFICATIONS
 * ============================================================================ */

/**
 * Callback for display configuration changes.
 * @param user_data User-provided data.
 */
typedef void (*metal_display_change_callback_t)(void* user_data);

/**
 * Register callback for screen configuration changes.
 * @param callback Callback function to invoke on changes.
 * @param user_data User data passed to callback.
 */
void metal_display_register_change_callback(metal_display_change_callback_t callback, void* user_data);

/**
 * Unregister display change callback.
 */
void metal_display_unregister_change_callback(void);

/* ============================================================================
 * WINDOW INTEGRATION
 * ============================================================================ */

/**
 * Get the screen that a window is currently on.
 * @param ns_window Pointer to NSWindow.
 * @return Pointer to NSScreen, or NULL if window is invalid.
 */
void* metal_display_get_window_screen(void* ns_window);

/**
 * Check if window is in fullscreen mode.
 * @param ns_window Pointer to NSWindow.
 * @return true if fullscreen, false otherwise.
 */
bool metal_display_is_window_fullscreen(void* ns_window);

/**
 * Check if window is currently visible (not occluded).
 * @param ns_window Pointer to NSWindow.
 * @return true if visible, false if occluded.
 */
bool metal_display_is_window_visible(void* ns_window);

/**
 * Get window content scale factor (for DPI).
 * @param ns_window Pointer to NSWindow.
 * @return Scale factor (1.0 for non-Retina, 2.0 for Retina).
 */
float metal_display_get_window_scale(void* ns_window);

/* ============================================================================
 * HDR/EDR REAL-TIME MONITORING
 * ============================================================================ */

/**
 * Callback for EDR headroom changes.
 * @param new_headroom New EDR headroom value.
 * @param user_data User-provided data.
 */
typedef void (*metal_display_edr_callback_t)(float new_headroom, void* user_data);

/**
 * Register callback for EDR headroom changes.
 * @param ns_screen Pointer to NSScreen to monitor, or NULL for main screen.
 * @param callback Callback function.
 * @param user_data User data.
 */
void metal_display_register_edr_callback(void* ns_screen, metal_display_edr_callback_t callback, void* user_data);

/**
 * Unregister EDR callback.
 */
void metal_display_unregister_edr_callback(void);

/**
 * Get current EDR headroom without creating display info.
 * @param ns_screen Pointer to NSScreen, or NULL for main screen.
 * @return EDR headroom value (1.0 = SDR, >1.0 = HDR).
 */
float metal_display_get_current_edr_headroom(void* ns_screen);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_DISPLAY_H */
