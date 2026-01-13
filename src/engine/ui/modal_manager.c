/*
 * modal_manager.c
 * Modal Dialog Manager Implementation
 * System-level management for modal dialogs, backdrop, and z-ordering
 */

#include "modal_manager.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static ModalManager* g_global_modal_manager = NULL;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static void modal_manager_grow_capacity(ModalManager* manager) {
    if (!manager) return;

    uint32_t new_capacity = manager->capacity == 0 ? 4 : manager->capacity * 2;
    UIDialog** new_dialogs = memory_alloc(sizeof(UIDialog*) * new_capacity);

    if (manager->dialogs) {
        memcpy(new_dialogs, manager->dialogs, sizeof(UIDialog*) * manager->count);
        free(manager->dialogs);
    }

    manager->dialogs = new_dialogs;
    manager->capacity = new_capacity;
}

/* ============================================================================
 * GLOBAL SINGLETON
 * ============================================================================ */

ModalManager* modal_manager_get_global(void) {
    if (!g_global_modal_manager) {
        g_global_modal_manager = modal_manager_create();
        LOG_INFO("Created global modal manager");
    }
    return g_global_modal_manager;
}

/* ============================================================================
 * PUBLIC API - Lifecycle
 * ============================================================================ */

ModalManager* modal_manager_create(void) {
    ModalManager* manager = memory_alloc(sizeof(ModalManager));
    if (!manager) {
        LOG_ERROR("Failed to allocate modal manager");
        return NULL;
    }

    memset(manager, 0, sizeof(ModalManager));

    // Initialize backdrop
    manager->backdrop.color = (Vec4){0.0f, 0.0f, 0.0f, 0.8f};  // Black with 80% opacity
    manager->backdrop.opacity = 0.8f;
    manager->backdrop.intercept_events = true;
    manager->backdrop.fade = 0.0f;

    // Initialize Z-index management
    manager->base_z_index = 1000;
    manager->z_index_step = 10;
    manager->next_z_index = manager->base_z_index;

    // Initialize settings
    manager->intercept_events = true;
    manager->backdrop_visible = true;
    manager->animation_speed = 2.0f;  // Fade in/out over ~0.5 seconds

    // Initialize dialogs array
    manager->dialogs = NULL;
    manager->count = 0;
    manager->capacity = 0;

    LOG_INFO("Created modal manager instance");
    return manager;
}

void modal_manager_destroy(ModalManager* manager) {
    if (!manager) return;

    // Clear dialog array (doesn't destroy the dialogs themselves)
    modal_manager_clear(manager);

    if (manager->dialogs) {
        free(manager->dialogs);
        manager->dialogs = NULL;
    }

    free(manager);
    LOG_INFO("Destroyed modal manager");
}

void modal_manager_init_global(void) {
    if (!g_global_modal_manager) {
        g_global_modal_manager = modal_manager_create();
    }
}

void modal_manager_shutdown_global(void) {
    if (g_global_modal_manager) {
        modal_manager_destroy(g_global_modal_manager);
        g_global_modal_manager = NULL;
    }
}

/* ============================================================================
 * PUBLIC API - Modal Stack Management
 * ============================================================================ */

void modal_manager_push(ModalManager* manager, UIDialog* dialog) {
    if (!manager || !dialog) return;

    // Grow capacity if needed
    if (manager->count >= manager->capacity) {
        modal_manager_grow_capacity(manager);
    }

    manager->dialogs[manager->count] = dialog;
    manager->count++;

    // Allocate Z-index for this modal
    // (Dialog should store this value)
    uint32_t z_index = modal_manager_allocate_z_index(manager);
    LOG_INFO("Pushed modal dialog (Z-index: %u), total modals: %u", z_index, manager->count);
}

UIDialog* modal_manager_pop(ModalManager* manager) {
    if (!manager || manager->count == 0) return NULL;

    manager->count--;
    UIDialog* dialog = manager->dialogs[manager->count];
    manager->dialogs[manager->count] = NULL;

    LOG_INFO("Popped modal dialog, remaining modals: %u", manager->count);
    return dialog;
}

UIDialog* modal_manager_get_top(const ModalManager* manager) {
    if (!manager || manager->count == 0) return NULL;
    return manager->dialogs[manager->count - 1];
}

uint32_t modal_manager_get_count(const ModalManager* manager) {
    if (!manager) return 0;
    return manager->count;
}

bool modal_manager_has_active_modal(const ModalManager* manager) {
    if (!manager) return false;
    return manager->count > 0;
}

bool modal_manager_remove(ModalManager* manager, UIDialog* dialog) {
    if (!manager || !dialog) return false;

    // Find and remove the dialog
    for (uint32_t i = 0; i < manager->count; i++) {
        if (manager->dialogs[i] == dialog) {
            // Shift remaining dialogs down
            for (uint32_t j = i; j < manager->count - 1; j++) {
                manager->dialogs[j] = manager->dialogs[j + 1];
            }
            manager->count--;
            manager->dialogs[manager->count] = NULL;
            LOG_INFO("Removed modal dialog, remaining modals: %u", manager->count);
            return true;
        }
    }

    return false;
}

void modal_manager_clear(ModalManager* manager) {
    if (!manager) return;

    for (uint32_t i = 0; i < manager->count; i++) {
        manager->dialogs[i] = NULL;
    }
    manager->count = 0;
    manager->next_z_index = manager->base_z_index;
    LOG_INFO("Cleared all modals from modal manager");
}

/* ============================================================================
 * PUBLIC API - Backdrop Configuration
 * ============================================================================ */

void modal_manager_set_backdrop_color(ModalManager* manager, Vec4 color) {
    if (!manager) return;
    manager->backdrop.color = color;
    LOG_DEBUG("Set backdrop color to (%.2f, %.2f, %.2f, %.2f)", color.x, color.y, color.z, color.w);
}

void modal_manager_set_backdrop_opacity(ModalManager* manager, float opacity) {
    if (!manager) return;
    manager->backdrop.opacity = fmaxf(0.0f, fminf(1.0f, opacity));
    manager->backdrop.color.w = manager->backdrop.opacity;
    LOG_DEBUG("Set backdrop opacity to %.2f", manager->backdrop.opacity);
}

void modal_manager_set_backdrop_visible(ModalManager* manager, bool visible) {
    if (!manager) return;
    manager->backdrop_visible = visible;
}

void modal_manager_set_backdrop_intercept_events(ModalManager* manager, bool intercept) {
    if (!manager) return;
    manager->backdrop.intercept_events = intercept;
}

void modal_manager_set_animation_speed(ModalManager* manager, float speed) {
    if (!manager) return;
    manager->animation_speed = fmaxf(0.1f, speed);
}

/* ============================================================================
 * PUBLIC API - Event Handling
 * ============================================================================ */

bool modal_manager_handle_event(ModalManager* manager, UIEvent* event) {
    if (!manager || !event) return false;

    // If no active modals, don't handle
    if (manager->count == 0) return false;

    // Get the top modal
    UIDialog* top_modal = modal_manager_get_top(manager);
    if (!top_modal) return false;

    // For now, just mark that there's an active modal
    // The actual event propagation will be handled by the dialog widget system
    // In a full implementation, this would:
    // 1. Check if event is on backdrop (if intercept_events is true)
    // 2. If on backdrop, intercept and optionally close modal
    // 3. Otherwise allow event to propagate to the modal dialog

    LOG_DEBUG("Modal manager handling event type: %u", event->type);
    return false;  // Let the UI system handle the actual event
}

void modal_manager_update(ModalManager* manager, float dt) {
    if (!manager || dt <= 0.0f) return;

    // Animate backdrop fade
    if (manager->count > 0) {
        // Fade in
        manager->backdrop.fade = fminf(1.0f, manager->backdrop.fade + dt * manager->animation_speed);
    } else {
        // Fade out
        manager->backdrop.fade = fmaxf(0.0f, manager->backdrop.fade - dt * manager->animation_speed);
    }
}

/* ============================================================================
 * PUBLIC API - Z-Index Management
 * ============================================================================ */

uint32_t modal_manager_allocate_z_index(ModalManager* manager) {
    if (!manager) return 0;

    uint32_t z_index = manager->next_z_index;
    manager->next_z_index += manager->z_index_step;
    return z_index;
}

void modal_manager_set_base_z_index(ModalManager* manager, uint32_t base_index) {
    if (!manager) return;
    manager->base_z_index = base_index;
    manager->next_z_index = base_index;
}

void modal_manager_set_z_index_step(ModalManager* manager, uint32_t step) {
    if (!manager) return;
    manager->z_index_step = step > 0 ? step : 1;
}

/* ============================================================================
 * PUBLIC API - Rendering
 * ============================================================================ */

void modal_manager_render_backdrop(ModalManager* manager, uint32_t layer) {
    if (!manager) return;

    if (!manager->backdrop_visible || manager->count == 0) return;

    // TODO: Implement actual backdrop rendering
    // This would render a semi-transparent overlay covering the screen
    // The fade value should be multiplied with the opacity

    float final_opacity = manager->backdrop.opacity * manager->backdrop.fade;
    LOG_DEBUG("Render backdrop at layer %u with opacity %.2f", layer, final_opacity);
}

void modal_manager_render_dialogs(ModalManager* manager) {
    if (!manager) return;

    // Render all dialogs in order (back to front)
    for (uint32_t i = 0; i < manager->count; i++) {
        if (manager->dialogs[i]) {
            // TODO: Call render on each dialog widget
            // widget_render() or similar
            LOG_DEBUG("Render modal dialog %u", i);
        }
    }
}

/* ============================================================================
 * PUBLIC API - Query and Utility
 * ============================================================================ */

bool modal_manager_is_blocking_input(const ModalManager* manager) {
    if (!manager) return false;
    return manager->count > 0 && manager->intercept_events;
}

float modal_manager_get_backdrop_fade(const ModalManager* manager) {
    if (!manager) return 0.0f;
    return manager->backdrop.fade;
}

uint32_t modal_manager_get_top_z_index(const ModalManager* manager) {
    if (!manager || manager->count == 0) return 0;
    return manager->base_z_index + ((manager->count - 1) * manager->z_index_step);
}
