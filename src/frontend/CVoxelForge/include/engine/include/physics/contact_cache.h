#ifndef PHYSICS_CONTACT_CACHE_H
#define PHYSICS_CONTACT_CACHE_H

#include "math/quat.h"
#include "math/vec3.h"
#include "physics/physics.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Contact Cache Configuration
typedef struct {
  uint32_t max_contacts;
  uint32_t max_contact_pairs;
  float cache_duration;
  float position_tolerance;
  float normal_tolerance;
  float penetration_tolerance;
  bool enable_persistent_contacts;
  bool enable_contact_warm_starting;
  uint32_t warm_start_iterations;
} ContactCacheConfig;

// Contact Cache Entry
typedef struct {
  EntityID entity_a;
  EntityID entity_b;

  // Contact data
  Vec3 contact_point;
  Vec3 contact_normal;
  float penetration_depth;
  float restitution;
  float friction;

  // Cache metadata
  uint32_t frame_created;
  uint32_t last_accessed;
  uint32_t access_count;
  bool persistent;
  bool valid;

  // Position tracking for cache invalidation
  Vec3 last_position_a;
  Vec3 last_position_b;
  Quat last_rotation_a;
  Quat last_rotation_b;

  // Force accumulation (for warm starting)
  Vec3 accumulated_impulse;
  Vec3 accumulated_friction_impulse;
} ContactCacheEntry;

// Contact Pair (two bodies that may have contacts)
typedef struct {
  EntityID entity_a;
  EntityID entity_b;

  // Contact indices for this pair
  uint32_t *contact_indices;
  uint32_t contact_count;
  uint32_t contact_capacity;

  // Pair metadata
  uint32_t last_frame_updated;
  bool active;
  bool static_pair;
} ContactPair;

// Contact Cache Context
typedef struct {
  ContactCacheConfig config;

  // Contact storage
  ContactCacheEntry *contacts;
  uint32_t contact_count;
  uint32_t contact_capacity;

  // Contact pairs
  ContactPair *pairs;
  uint32_t pair_count;
  uint32_t pair_capacity;

  // Free lists
  uint32_t *free_contact_indices;
  uint32_t free_contact_count;
  uint32_t free_contact_capacity;

  uint32_t *free_pair_indices;
  uint32_t free_pair_count;
  uint32_t free_pair_capacity;

  // Current frame tracking
  uint32_t current_frame;

  // Performance statistics
  struct {
    uint32_t total_contacts;
    uint32_t active_contacts;
    uint32_t persistent_contacts;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint32_t warm_start_hits;
    uint32_t invalidations;
    float cache_hit_ratio;
    float average_cache_time;
    float total_cache_time;
  } stats;

  // Memory usage
  struct {
    size_t contacts_size;
    size_t pairs_size;
    size_t total_size;
  } memory;
} ContactCache;

// Contact Cache Functions
bool contact_cache_init(ContactCache *cache, const ContactCacheConfig *config);
void contact_cache_cleanup(ContactCache *cache);
void contact_cache_reset(ContactCache *cache);
void contact_cache_update(ContactCache *cache, float delta_time);

// Contact Management
uint32_t contact_cache_add_contact(ContactCache *cache, EntityID entity_a,
                                   EntityID entity_b, Vec3 contact_point,
                                   Vec3 normal, float penetration,
                                   float restitution, float friction,
                                   bool persistent);
bool contact_cache_remove_contact(ContactCache *cache, uint32_t contact_id);
bool contact_cache_remove_contacts_between_bodies(ContactCache *cache,
                                                  EntityID entity_a,
                                                  EntityID entity_b);

// Contact Retrieval
ContactCacheEntry *contact_cache_find_contact(ContactCache *cache,
                                              EntityID entity_a,
                                              EntityID entity_b);
ContactCacheEntry *
contact_cache_find_similar_contact(ContactCache *cache, EntityID entity_a,
                                   EntityID entity_b, Vec3 contact_point,
                                   Vec3 normal, float tolerance);
ContactPair *contact_cache_get_pair(ContactCache *cache, EntityID entity_a,
                                    EntityID entity_b);
uint32_t contact_cache_get_contacts_between_bodies(ContactCache *cache,
                                                   EntityID entity_a,
                                                   EntityID entity_b,
                                                   ContactCacheEntry **contacts,
                                                   uint32_t max_contacts);

// Cache Validation and Invalidation
bool contact_cache_is_contact_valid(ContactCache *cache,
                                    const ContactCacheEntry *contact,
                                    Vec3 position_a, Quat rotation_a,
                                    Vec3 position_b, Quat rotation_b);
void contact_cache_invalidate_contact(ContactCache *cache, uint32_t contact_id);
void contact_cache_invalidate_body_contacts(ContactCache *cache,
                                            EntityID entity_id);
void contact_cache_validate_all_contacts(ContactCache *cache);

// Warm Starting
void contact_cache_warm_start_contacts(ContactCache *cache);
void contact_cache_accumulate_impulse(ContactCache *cache, uint32_t contact_id,
                                      Vec3 impulse);
void contact_cache_accumulate_friction_impulse(ContactCache *cache,
                                               uint32_t contact_id,
                                               Vec3 friction_impulse);
void contact_cache_reset_accumulated_impulses(ContactCache *cache,
                                              uint32_t contact_id);

// Persistent Contacts
void contact_cache_make_persistent(ContactCache *cache, uint32_t contact_id);
void contact_cache_make_temporary(ContactCache *cache, uint32_t contact_id);
bool contact_cache_is_persistent(const ContactCache *cache,
                                 uint32_t contact_id);

// Contact Pair Management
ContactPair *contact_cache_get_or_create_pair(ContactCache *cache,
                                              EntityID entity_a,
                                              EntityID entity_b);
bool contact_cache_remove_pair(ContactCache *cache, uint32_t pair_id);
void contact_cache_update_pair_activity(ContactCache *cache, uint32_t pair_id);

// Cache Optimization
void contact_cache_remove_old_contacts(ContactCache *cache);
void contact_cache_compact_storage(ContactCache *cache);
void contact_cache_optimize_for_static_bodies(ContactCache *cache);
void contact_cache_remove_duplicate_contacts(ContactCache *cache);

// Query Functions
uint32_t contact_cache_query_all_contacts(ContactCache *cache,
                                          ContactCacheEntry **contacts,
                                          uint32_t max_contacts);
uint32_t contact_cache_query_body_contacts(ContactCache *cache,
                                           EntityID entity_id,
                                           ContactCacheEntry **contacts,
                                           uint32_t max_contacts);
uint32_t cache_query_contacts_in_bounds(ContactCache *cache, Vec3 bounds_min,
                                        Vec3 bounds_max,
                                        ContactCacheEntry **contacts,
                                        uint32_t max_contacts);

// Debug and Visualization
void contact_cache_debug_draw_contacts(ContactCache *cache);
void contact_cache_debug_draw_contact_pairs(ContactCache *cache);
void contact_cache_debug_draw_cache_statistics(ContactCache *cache);
void contact_cache_debug_print_statistics(ContactCache *cache);

// Performance Analysis
void contact_cache_get_performance_stats(const ContactCache *cache,
                                         uint32_t *total_contacts,
                                         uint32_t *active_contacts,
                                         uint32_t *cache_hits,
                                         uint32_t *cache_misses,
                                         float *hit_ratio);
void contact_cache_get_memory_usage(const ContactCache *cache, size_t *contacts,
                                    size_t *pairs, size_t *total);
void contact_cache_reset_statistics(ContactCache *cache);

// Configuration Functions
void contact_cache_set_max_contacts(ContactCache *cache, uint32_t max_contacts);
void contact_cache_set_cache_duration(ContactCache *cache, float duration);
void contact_cache_set_tolerances(ContactCache *cache, float position,
                                  float normal, float penetration);
void contact_cache_enable_warm_starting(ContactCache *cache, bool enable,
                                        uint32_t iterations);

// Utility Functions
bool contact_cache_are_entities_equal(EntityID entity_a, EntityID entity_b);
bool contact_cache_are_positions_similar(Vec3 pos_a, Vec3 pos_b,
                                         float tolerance);
bool contact_cache_are_normals_similar(Vec3 normal_a, Vec3 normal_b,
                                       float tolerance);
float contact_cache_calculate_distance_to_plane(Vec3 point, Vec3 plane_point,
                                                Vec3 plane_normal);

// Validation and Testing
bool contact_cache_validate(const ContactCache *cache);
bool contact_cache_test_collision_detection(ContactCache *cache,
                                            uint32_t num_bodies,
                                            uint32_t iterations);
void contact_cache_run_performance_test(ContactCache *cache,
                                        uint32_t num_contacts,
                                        uint32_t iterations);

// Iterator Functions
typedef struct {
  ContactCache *cache;
  uint32_t current_index;
  uint32_t end_index;
  bool filter_persistent;
  bool filter_temporary;
  bool filter_valid;
} ContactCacheIterator;

void contact_cache_iterator_init(ContactCacheIterator *iterator,
                                 ContactCache *cache);
void contact_cache_iterator_set_filter(ContactCacheIterator *iterator,
                                       bool persistent, bool temporary,
                                       bool valid);
bool contact_cache_iterator_next(ContactCacheIterator *iterator,
                                 ContactCacheEntry **contact);

// Helper Functions
const char *contact_cache_get_error_string(uint32_t error_code);
uint32_t contact_cache_generate_contact_id(EntityID entity_a,
                                           EntityID entity_b);
bool contact_cache_is_valid_entity_id(EntityID entity_id);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_CONTACT_CACHE_H
