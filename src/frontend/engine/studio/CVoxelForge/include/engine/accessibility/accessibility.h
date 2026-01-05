#ifndef ACCESSIBILITY_H
#define ACCESSIBILITY_H

/**
 * =================================================================================================
 *                              ACCESSIBILITY SYSTEM - HEADER
 * =================================================================================================
 *
 * This header defines the accessibility system interface for Minecraft v2.
 * Includes support for visual, auditory, control, and cognitive accessibility features.
 */

#include <stdint.h>
#include <stdbool.h>

/* ===== VISUAL ACCESSIBILITY ===== */

/**
 * Colorblind vision modes
 */
typedef enum {
    COLORBLIND_NONE,
    COLORBLIND_DEUTERANOPIA,  // Red-green, red-weak
    COLORBLIND_PROTANOPIA,    // Red-green, red-blind
    COLORBLIND_TRITANOPIA,    // Blue-yellow
} ColorblindMode;

/**
 * Set the colorblind mode and apply LUT (Look-Up Table)
 */
void accessibility_set_colorblind_mode(ColorblindMode mode);

/**
 * Enable/disable high contrast UI
 */
void accessibility_set_high_contrast(bool enabled);

/**
 * Set UI scaling percentage (100-400%)
 */
void accessibility_set_ui_scale(uint32_t scale_percent);

/**
 * Enable/disable text-to-speech for menus and dialogue
 */
void accessibility_set_tts_enabled(bool enabled);

/**
 * Enable/disable screen shake (for motion sickness)
 */
void accessibility_set_screen_shake(bool enabled);

/**
 * Configure reticle appearance
 */
typedef struct {
    uint32_t size;      // Size multiplier (percentage)
    uint32_t color;     // RGBA color
    uint8_t opacity;    // 0-255
} ReticleConfig;

void accessibility_set_reticle_config(ReticleConfig config);

/* ===== AUDITORY ACCESSIBILITY ===== */

/**
 * Enable/disable visual sound indicators (2D icons for footsteps/shots, etc.)
 */
void accessibility_set_visual_sound_indicators(bool enabled);

/**
 * Enable/disable mono audio mixdown
 */
void accessibility_set_mono_audio(bool enabled);

/**
 * Enable/disable speech-to-text for voice chat
 */
void accessibility_set_stt_enabled(bool enabled);

/**
 * Enable/disable frequency filter for human voice frequencies
 */
void accessibility_set_frequency_filter(bool enabled);

/* ===== CONTROL ACCESSIBILITY ===== */

/**
 * Enable/disable one-handed mode with controller remapping
 */
void accessibility_set_one_handed_mode(bool enabled);

/**
 * Control type for input mapping
 */
typedef enum {
    CONTROL_TOGGLE,
    CONTROL_HOLD,
} ControlType;

/**
 * Set control type (toggle vs hold) for actions like sprint, ADS
 */
void accessibility_set_control_type(const char *action, ControlType type);

/**
 * Enable/disable button mashing assist (single press replaces rapid tap)
 */
void accessibility_set_button_mashing_assist(bool enabled);

/* ===== COGNITIVE ACCESSIBILITY ===== */

/**
 * Set game speed multiplier (0.5 = 50% slower, 1.0 = normal, 2.0 = 2x faster)
 */
void accessibility_set_game_speed(float multiplier);

/**
 * Enable/disable navigation assist (persistent line to objective)
 */
void accessibility_set_navigation_assist(bool enabled);

/**
 * Enable/disable simplified HUD (removes distracting elements)
 */
void accessibility_set_simplified_hud(bool enabled);

/* ===== PLATFORM INTEGRATION ===== */

/**
 * Read OS accessibility settings and mirror them (iOS/macOS)
 */
void accessibility_sync_os_settings(void);

/**
 * Initialize screen reader integration (VoiceOver/TalkBack)
 */
void accessibility_init_screen_reader(void);

/**
 * Shutdown accessibility system
 */
void accessibility_shutdown(void);

/**
 * Initialize accessibility system
 */
void accessibility_init(void);

#endif /* ACCESSIBILITY_H */
