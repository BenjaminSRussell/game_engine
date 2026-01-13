/*
 * modal_manager.h
 * Modal Dialog Manager
 * System-level management for modal dialogs, backdrop, and z-ordering
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_MODAL_MANAGER_H
#define UI_MODAL_MANAGER_H

#include "widgets/widget.h"
#include "include/math/math.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Forward declaration
typedef struct UIDialog UIDialog;

typedef struct {
    Vec4 color;           // Backdrop color (RGBA)
    float opacity;        // Backdrop opacity (0.0-1.0)
    bool intercept_events;  // Whether backdrop intercepts events
    float fade;           // Current fade value
} ModalBackdrop;

typedef struct {
    UIDialog** dialogs;      // Dynamic array of active modal dialogs
    uint32_t count;          // Number of active modals
    uint32_t capacity;       // Capacity of dialogs array

    ModalBackdrop backdrop;  // Backdrop configuration

    uint32_t base_z_index;   // Base Z-index for modals (starting point)
    uint32_t z_index_step;   // Step increment for each modal layer
    uint32_t next_z_index;   // Next Z-index to allocate

    bool intercept_events;   // Whether to intercept events for modal blocking
    bool backdrop_visible;   // Whether to render backdrop

    // Timing
    float animation_speed;   // How fast backdrop fades in/out
} ModalManager;

/* ============================================================================
 * GLOBAL SINGLETON
 * ============================================================================ */

/**
 * Get the global modal manager instance
 * Creates one if it doesn't exist
 *
 * @return  Pointer to global ModalManager
 */
ModalManager* modal_manager_get_global(void);

/* ============================================================================
 * API - Lifecycle
 * ============================================================================ */

/**
 * Create a new modal manager instance
 *
 * @return  Pointer to created ModalManager, or NULL on failure
 */
ModalManager* modal_manager_create(void);

/**
 * Destroy a modal manager and free all resources
 * Note: Does not destroy dialogs in the stack
 *
 * @param manager  ModalManager to destroy
 */
void modal_manager_destroy(ModalManager* manager);

/**
 * Initialize the global modal manager
 */
void modal_manager_init_global(void);

/**
 * Shutdown the global modal manager
 */
void modal_manager_shutdown_global(void);

/* ============================================================================
 * API - Modal Stack Management
 * ============================================================================ */

/**
 * Push a modal dialog onto the stack
 *
 * @param manager  ModalManager instance
 * @param dialog   Dialog to add to modal stack
 */
void modal_manager_push(ModalManager* manager, UIDialog* dialog);

/**
 * Pop the top modal dialog from the stack
 *
 * @param manager  ModalManager instance
 * @return         Pointer to removed dialog, or NULL if stack empty
 */
UIDialog* modal_manager_pop(ModalManager* manager);

/**
 * Get the top modal dialog without removing it
 *
 * @param manager  ModalManager instance
 * @return         Pointer to top dialog, or NULL if stack empty
 */
UIDialog* modal_manager_get_top(const ModalManager* manager);

/**
 * Get the number of active modal dialogs
 *
 * @param manager  ModalManager instance
 * @return         Number of modals in stack
 */
uint32_t modal_manager_get_count(const ModalManager* manager);

/**
 * Check if there are any active modals
 *
 * @param manager  ModalManager instance
 * @return         True if modal stack is not empty
 */
bool modal_manager_has_active_modal(const ModalManager* manager);

/**
 * Remove a specific dialog from the stack
 *
 * @param manager  ModalManager instance
 * @param dialog   Dialog to remove
 * @return         True if dialog was found and removed
 */
bool modal_manager_remove(ModalManager* manager, UIDialog* dialog);

/**
 * Clear all dialogs from the stack
 *
 * @param manager  ModalManager instance
 */
void modal_manager_clear(ModalManager* manager);

/* ============================================================================
 * API - Backdrop Configuration
 * ============================================================================ */

/**
 * Set the backdrop color
 *
 * @param manager  ModalManager instance
 * @param color    Backdrop color (RGBA)
 */
void modal_manager_set_backdrop_color(ModalManager* manager, Vec4 color);

/**
 * Set the backdrop opacity
 *
 * @param manager   ModalManager instance
 * @param opacity   Opacity value (0.0 = transparent, 1.0 = opaque)
 */
void modal_manager_set_backdrop_opacity(ModalManager* manager, float opacity);

/**
 * Show or hide the backdrop
 *
 * @param manager   ModalManager instance
 * @param visible   Whether backdrop should be visible
 */
void modal_manager_set_backdrop_visible(ModalManager* manager, bool visible);

/**
 * Set whether backdrop intercepts events
 *
 * @param manager       ModalManager instance
 * @param intercept     Whether to intercept events
 */
void modal_manager_set_backdrop_intercept_events(ModalManager* manager, bool intercept);

/**
 * Set backdrop animation speed
 *
 * @param manager  ModalManager instance
 * @param speed    Animation speed multiplier
 */
void modal_manager_set_animation_speed(ModalManager* manager, float speed);

/* ============================================================================
 * API - Event Handling
 * ============================================================================ */

/**
 * Handle a UI event through the modal manager
 * This allows modals to block events from reaching widgets behind them
 *
 * @param manager  ModalManager instance
 * @param event    UIEvent to handle
 * @return         True if event was handled by modal
 */
bool modal_manager_handle_event(ModalManager* manager, UIEvent* event);

/**
 * Update modal animations and state
 *
 * @param manager  ModalManager instance
 * @param dt       Delta time in seconds
 */
void modal_manager_update(ModalManager* manager, float dt);

/* ============================================================================
 * API - Z-Index Management
 * ============================================================================ */

/**
 * Allocate a new Z-index for a modal dialog
 *
 * @param manager  ModalManager instance
 * @return         Z-index value to use
 */
uint32_t modal_manager_allocate_z_index(ModalManager* manager);

/**
 * Set the base Z-index for modal dialogs
 * All modals will be allocated Z-indices starting from this value
 *
 * @param manager     ModalManager instance
 * @param base_index  Base Z-index (default: 1000)
 */
void modal_manager_set_base_z_index(ModalManager* manager, uint32_t base_index);

/**
 * Set the Z-index step for stacking modals
 *
 * @param manager  ModalManager instance
 * @param step     Step increment (default: 10)
 */
void modal_manager_set_z_index_step(ModalManager* manager, uint32_t step);

/* ============================================================================
 * API - Rendering
 * ============================================================================ */

/**
 * Render the modal backdrop
 * Call this before rendering dialogs in your render loop
 *
 * @param manager  ModalManager instance
 * @param layer    Rendering layer/priority
 */
void modal_manager_render_backdrop(ModalManager* manager, uint32_t layer);

/**
 * Render all active modal dialogs
 * Call this after rendering the backdrop
 *
 * @param manager  ModalManager instance
 */
void modal_manager_render_dialogs(ModalManager* manager);

/* ============================================================================
 * API - Query and Utility
 * ============================================================================ */

/**
 * Check if any modal is blocking input
 * This helps determine if user input should be processed by game/editor
 *
 * @param manager  ModalManager instance
 * @return         True if input is blocked by modal
 */
bool modal_manager_is_blocking_input(const ModalManager* manager);

/**
 * Get the current backdrop fade value (0.0 = transparent, 1.0 = opaque)
 *
 * @param manager  ModalManager instance
 * @return         Current fade value
 */
float modal_manager_get_backdrop_fade(const ModalManager* manager);

/**
 * Get the top modal's Z-index
 *
 * @param manager  ModalManager instance
 * @return         Z-index of top modal, or 0 if no modals
 */
uint32_t modal_manager_get_top_z_index(const ModalManager* manager);

#ifdef __cplusplus
}
#endif

#endif /* UI_MODAL_MANAGER_H */
