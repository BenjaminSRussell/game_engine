#ifndef LANDSCAPE_FOLIAGE_INTERACTION_H
#define LANDSCAPE_FOLIAGE_INTERACTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_foliage_interaction_handle {
    uint32_t id;
} landscape_foliage_interaction_handle_t;

typedef struct landscape_foliage_interaction_desc {
    uint32_t flags;
    void* user_data;
    float global_range; // How far interaction affects
    float global_stiffness; // Resistance to bending
} landscape_foliage_interaction_desc_t;

typedef struct landscape_foliage_interaction_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t active_influencers;
} landscape_foliage_interaction_info_t;

// An object that pushes foliage
typedef struct foliage_influencer {
    Vec3 position;
    float radius;
    float strength;
} foliage_influencer_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_foliage_interaction_init(void);
void landscape_foliage_interaction_shutdown(void);

/* Lifecycle */
int landscape_foliage_interaction_create(landscape_foliage_interaction_handle_t* out_handle, const landscape_foliage_interaction_desc_t* desc);
void landscape_foliage_interaction_destroy(landscape_foliage_interaction_handle_t handle);

/* Interaction Management */

// Add/Update influencers for this frame
int landscape_foliage_interaction_set_influencers(
    landscape_foliage_interaction_handle_t handle,
    const foliage_influencer_t* influencers,
    uint32_t count
);

// Get interaction vector for a position (displacement vector)
Vec3 landscape_foliage_interaction_get_displacement(
    landscape_foliage_interaction_handle_t handle,
    const Vec3* position
);

/* Operations */

int landscape_foliage_interaction_update(landscape_foliage_interaction_handle_t handle, float delta_time);
bool landscape_foliage_interaction_is_valid(landscape_foliage_interaction_handle_t handle);
int landscape_foliage_interaction_get_info(landscape_foliage_interaction_handle_t handle, landscape_foliage_interaction_info_t* out_info);
void landscape_foliage_interaction_mark_dirty(landscape_foliage_interaction_handle_t handle);
int landscape_foliage_interaction_process_pending(void);

/* Statistics */
uint32_t landscape_foliage_interaction_get_count(void);
size_t landscape_foliage_interaction_get_memory_usage(void);
void landscape_foliage_interaction_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_FOLIAGE_INTERACTION_H */
