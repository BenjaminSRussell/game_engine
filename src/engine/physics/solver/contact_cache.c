#include "physics/contact_cache.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// Global contact cache instance
static ContactCache g_contact_cache = {0};

bool contact_cache_init(ContactCache *cache, const ContactCacheConfig *config) {
  if (!cache || !config)
    return false;

  memset(cache, 0, sizeof(ContactCache));
  cache->config = *config;

  // Allocate contacts
  cache->contact_capacity = cache->config.max_contacts;
  cache->contacts = (ContactCacheEntry *)core_alloc(cache->contact_capacity *
                                                    sizeof(ContactCacheEntry));
  if (!cache->contacts)
    return false;
  memset(cache->contacts, 0,
         cache->contact_capacity * sizeof(ContactCacheEntry));

  // Allocate pairs
  cache->pair_capacity = cache->config.max_contact_pairs;
  cache->pairs =
      (ContactPair *)core_alloc(cache->pair_capacity * sizeof(ContactPair));
  if (!cache->pairs) {
    core_free(cache->contacts);
    return false;
  }
  memset(cache->pairs, 0, cache->pair_capacity * sizeof(ContactPair));

  // Allocate free lists
  cache->free_contact_capacity = cache->contact_capacity;
  cache->free_contact_indices =
      (uint32_t *)core_alloc(cache->free_contact_capacity * sizeof(uint32_t));
  if (!cache->free_contact_indices) {
    core_free(cache->contacts);
    core_free(cache->pairs);
    return false;
  }

  cache->free_pair_capacity = cache->pair_capacity;
  cache->free_pair_indices =
      (uint32_t *)core_alloc(cache->free_pair_capacity * sizeof(uint32_t));
  if (!cache->free_pair_indices) {
    core_free(cache->contacts);
    core_free(cache->pairs);
    core_free(cache->free_contact_indices);
    return false;
  }

  // Initialize free lists
  for (uint32_t i = 0; i < cache->free_contact_capacity; i++) {
    cache->free_contact_indices[i] = i;
  }
  cache->free_contact_count = cache->free_contact_capacity;

  for (uint32_t i = 0; i < cache->free_pair_capacity; i++) {
    cache->free_pair_indices[i] = i;
  }
  cache->free_pair_count = cache->free_pair_capacity;

  // Initialize frame counter
  cache->current_frame = 0;

  // Calculate memory usage
  cache->memory.contacts_size =
      cache->contact_capacity * sizeof(ContactCacheEntry);
  cache->memory.pairs_size = cache->pair_capacity * sizeof(ContactPair);
  cache->memory.total_size =
      cache->memory.contacts_size + cache->memory.pairs_size;

  LOG_INFO(
      "Contact cache initialized: %u contacts, %u pairs, cache duration: %.2fs",
      cache->contact_capacity, cache->pair_capacity,
      cache->config.cache_duration);
  return true;
}

void contact_cache_cleanup(ContactCache *cache) {
  if (!cache)
    return;

  // Free contact arrays
  for (uint32_t i = 0; i < cache->pair_capacity; i++) {
    if (cache->pairs[i].contact_indices) {
      core_free(cache->pairs[i].contact_indices);
    }
  }

  // Free main allocations
  if (cache->contacts)
    core_free(cache->contacts);
  if (cache->pairs)
    core_free(cache->pairs);
  if (cache->free_contact_indices)
    core_free(cache->free_contact_indices);
  if (cache->free_pair_indices)
    core_free(cache->free_pair_indices);

  memset(cache, 0, sizeof(ContactCache));
  LOG_INFO("Contact cache cleaned up");
}

void contact_cache_update(ContactCache *cache, float delta_time) {
  if (!cache)
    return;

  cache->current_frame++;

  // Remove old contacts based on cache duration
  contact_cache_remove_old_contacts(cache);

  // Update statistics
  cache->stats.total_contacts = cache->contact_count;
  cache->stats.active_contacts = 0;
  cache->stats.persistent_contacts = 0;

  for (uint32_t i = 0; i < cache->contact_count; i++) {
    ContactCacheEntry *contact = &cache->contacts[i];

    if (contact->valid) {
      cache->stats.active_contacts++;
      if (contact->persistent) {
        cache->stats.persistent_contacts++;
      }
    }
  }

  // Calculate cache hit ratio
  if (cache->stats.cache_hits + cache->stats.cache_misses > 0) {
    cache->stats.cache_hit_ratio =
        (float)cache->stats.cache_hits /
        (cache->stats.cache_hits + cache->stats.cache_misses);
  }
}

uint32_t contact_cache_add_contact(ContactCache *cache, EntityID entity_a,
                                   EntityID entity_b, Vec3 contact_point,
                                   Vec3 normal, float penetration,
                                   float restitution, float friction,
                                   bool persistent) {
  if (!cache || cache->contact_count >= cache->contact_capacity)
    return UINT32_MAX;

  // Get free contact index
  if (cache->free_contact_count == 0)
    return UINT32_MAX;
  uint32_t contact_index =
      cache->free_contact_indices[--cache->free_contact_count];

  // Initialize contact
  ContactCacheEntry *contact = &cache->contacts[contact_index];
  contact->entity_a = entity_a;
  contact->entity_b = entity_b;
  contact->contact_point = contact_point;
  contact->contact_normal = normal;
  contact->penetration_depth = penetration;
  contact->restitution = restitution;
  contact->friction = friction;
  contact->frame_created = cache->current_frame;
  contact->last_accessed = cache->current_frame;
  contact->access_count = 1;
  contact->persistent = persistent;
  contact->valid = true;

  // Initialize position tracking (would get from physics bodies)
  contact->last_position_a = contact_point; // Placeholder
  contact->last_position_b = contact_point; // Placeholder
  contact->last_rotation_a = quat_identity();
  contact->last_rotation_b = quat_identity();

  // Reset accumulated impulses
  contact->accumulated_impulse = vec3_zero();
  contact->accumulated_friction_impulse = vec3_zero();

  // Get or create contact pair
  ContactPair *pair =
      contact_cache_get_or_create_pair(cache, entity_a, entity_b);
  if (pair != NULL) {
    // Add contact to pair
    if (pair->contact_count < pair->contact_capacity) {
      if (pair->contact_indices == NULL) {
        pair->contact_indices =
            (uint32_t *)core_alloc(pair->contact_capacity * sizeof(uint32_t));
      }

      pair->contact_indices[pair->contact_count++] = contact_index;
      pair->last_frame_updated = cache->current_frame;
      pair->active = true;
    }
  }

  cache->contact_count++;

  if (persistent) {
    cache->stats.cache_misses++; // New persistent contact is a "miss" for now
  } else {
    cache->stats.cache_misses++;
  }

  return contact_index;
}

ContactCacheEntry *contact_cache_find_contact(ContactCache *cache,
                                              EntityID entity_a,
                                              EntityID entity_b) {
  if (!cache)
    return NULL;

  // Find contact pair first
  ContactPair *pair = contact_cache_get_pair(cache, entity_a, entity_b);
  if (!pair || !pair->active || pair->contact_count == 0)
    return NULL;

  // Find most recent contact in the pair
  uint32_t best_contact_index = UINT32_MAX;
  uint32_t latest_frame = 0;

  for (uint32_t i = 0; i < pair->contact_count; i++) {
    uint32_t contact_index = pair->contact_indices[i];
    if (contact_index < cache->contact_capacity) {
      ContactCacheEntry *contact = &cache->contacts[contact_index];
      if (contact->valid &&
          ((contact->entity_a == entity_a && contact->entity_b == entity_b) ||
           (contact->entity_a == entity_b && contact->entity_b == entity_a))) {
        if (contact->last_accessed > latest_frame) {
          latest_frame = contact->last_accessed;
          best_contact_index = contact_index;
        }
      }
    }
  }

  if (best_contact_index != UINT32_MAX) {
    ContactCacheEntry *contact = &cache->contacts[best_contact_index];
    contact->last_accessed = cache->current_frame;
    contact->access_count++;
    cache->stats.cache_hits++;
    return contact;
  }

  cache->stats.cache_misses++;
  return NULL;
}

ContactPair *contact_cache_get_pair(ContactCache *cache, EntityID entity_a,
                                    EntityID entity_b) {
  if (!cache)
    return NULL;
  for (uint32_t i = 0; i < cache->pair_count; i++) {
    ContactPair *pair = &cache->pairs[i];
    if ((pair->entity_a == entity_a && pair->entity_b == entity_b) ||
        (pair->entity_a == entity_b && pair->entity_b == entity_a)) {
      return pair;
    }
  }
  return NULL;
}

ContactPair *contact_cache_get_or_create_pair(ContactCache *cache,
                                              EntityID entity_a,
                                              EntityID entity_b) {
  if (!cache)
    return NULL;

  // Find existing pair
  for (uint32_t i = 0; i < cache->pair_count; i++) {
    ContactPair *pair = &cache->pairs[i];
    if ((pair->entity_a == entity_a && pair->entity_b == entity_b) ||
        (pair->entity_a == entity_b && pair->entity_b == entity_a)) {
      return pair;
    }
  }

  // Create new pair
  if (cache->pair_count >= cache->pair_capacity)
    return NULL;

  if (cache->free_pair_count == 0)
    return NULL;
  uint32_t pair_index = cache->free_pair_indices[--cache->free_pair_count];

  ContactPair *pair = &cache->pairs[pair_index];
  pair->entity_a = entity_a;
  pair->entity_b = entity_b;
  pair->contact_indices = NULL;
  pair->contact_count = 0;
  pair->contact_capacity = 0;
  pair->last_frame_updated = cache->current_frame;
  pair->active = true;
  pair->static_pair = false; // Would check if both bodies are static

  cache->pair_count++;
  return pair;
}

bool contact_cache_is_contact_valid(ContactCache *cache,
                                    const ContactCacheEntry *contact,
                                    Vec3 position_a, Quat rotation_a,
                                    Vec3 position_b, Quat rotation_b) {
  if (!cache || !contact)
    return false;

  // Check if contact is too old
  uint32_t age = cache->current_frame - contact->frame_created;
  if (age > cache->config.cache_duration / 0.016f) { // Assuming 60 FPS
    return false;
  }

  // Check position tolerance
  float position_diff_a = vec3_distance(contact->last_position_a, position_a);
  float position_diff_b = vec3_distance(contact->last_position_b, position_b);

  if (position_diff_a > cache->config.position_tolerance ||
      position_diff_b > cache->config.position_tolerance) {
    return false;
  }

  // Check normal tolerance
  float normal_diff =
      vec3_distance(contact->contact_normal,
                    quat_rotate_vec3(rotation_a, contact->contact_normal));
  if (normal_diff > cache->config.normal_tolerance) {
    return false;
  }

  return true;
}

void contact_cache_invalidate_contact(ContactCache *cache,
                                      uint32_t contact_id) {
  if (!cache || contact_id >= cache->contact_capacity)
    return;

  ContactCacheEntry *contact = &cache->contacts[contact_id];
  contact->valid = false;
  contact->access_count = 0;

  cache->stats.invalidations++;

  // Remove from pair if needed
  ContactPair *pair =
      contact_cache_get_pair(cache, contact->entity_a, contact->entity_b);
  if (pair) {
    for (uint32_t i = 0; i < pair->contact_count; i++) {
      if (pair->contact_indices[i] == contact_id) {
        // Remove by shifting remaining elements
        for (uint32_t j = i; j < pair->contact_count - 1; j++) {
          pair->contact_indices[j] = pair->contact_indices[j + 1];
        }
        pair->contact_count--;
        break;
      }
    }
  }
}

void contact_cache_remove_old_contacts(ContactCache *cache) {
  if (!cache)
    return;

  float start_time = get_time();

  for (uint32_t i = 0; i < cache->contact_count; i++) {
    ContactCacheEntry *contact = &cache->contacts[i];

    if (!contact->valid || contact->persistent)
      continue;

    // Check if contact is too old
    uint32_t age = cache->current_frame - contact->frame_created;
    float age_seconds = age * 0.016f; // Assuming 60 FPS

    if (age_seconds > cache->config.cache_duration) {
      contact_cache_invalidate_contact(cache, i);
    }
  }

  cache->stats.total_cache_time += get_time() - start_time;
}

void contact_cache_warm_start_contacts(ContactCache *cache) {
  if (!cache || !cache->config.enable_contact_warm_starting)
    return;

  for (uint32_t i = 0; i < cache->contact_count; i++) {
    ContactCacheEntry *contact = &cache->contacts[i];

    if (contact->valid) {
      // Apply accumulated impulses from previous frame
      // In a real implementation, this would apply to physics bodies
      // physics_body_apply_impulse(contact->entity_a,
      // contact->accumulated_impulse);
      // physics_body_apply_impulse(contact->entity_b,
      // vec3_neg(contact->accumulated_impulse));

      // Reset accumulated impulses
      contact->accumulated_impulse = vec3_zero();
      contact->accumulated_friction_impulse = vec3_zero();

      cache->stats.warm_start_hits++;
    }
  }
}

void contact_cache_accumulate_impulse(ContactCache *cache, uint32_t contact_id,
                                      Vec3 impulse) {
  if (!cache || contact_id >= cache->contact_capacity)
    return;

  ContactCacheEntry *contact = &cache->contacts[contact_id];
  if (!contact->valid)
    return;

  contact->accumulated_impulse =
      vec3_add(contact->accumulated_impulse, impulse);
}

uint32_t contact_cache_get_contacts_between_bodies(ContactCache *cache,
                                                   EntityID entity_a,
                                                   EntityID entity_b,
                                                   ContactCacheEntry **contacts,
                                                   uint32_t max_contacts) {
  if (!cache || !contacts || max_contacts == 0)
    return 0;

  ContactPair *pair = contact_cache_get_pair(cache, entity_a, entity_b);
  if (!pair || !pair->active || pair->contact_count == 0)
    return 0;

  uint32_t count = 0;
  for (uint32_t i = 0; i < pair->contact_count && count < max_contacts; i++) {
    uint32_t contact_index = pair->contact_indices[i];
    if (contact_index < cache->contact_capacity) {
      ContactCacheEntry *contact = &cache->contacts[contact_index];
      if (contact->valid) {
        contacts[count++] = contact;
      }
    }
  }

  return count;
}

void contact_cache_get_performance_stats(const ContactCache *cache,
                                         uint32_t *total_contacts,
                                         uint32_t *active_contacts,
                                         uint32_t *cache_hits,
                                         uint32_t *cache_misses,
                                         float *hit_ratio) {
  if (!cache)
    return;

  if (total_contacts)
    *total_contacts = cache->stats.total_contacts;
  if (active_contacts)
    *active_contacts = cache->stats.active_contacts;
  if (cache_hits)
    *cache_hits = cache->stats.cache_hits;
  if (cache_misses)
    *cache_misses = cache->stats.cache_misses;
  if (hit_ratio)
    *hit_ratio = cache->stats.cache_hit_ratio;
}

bool contact_cache_are_positions_similar(Vec3 pos_a, Vec3 pos_b,
                                         float tolerance) {
  return vec3_distance_sq(pos_a, pos_b) <= tolerance * tolerance;
}

bool contact_cache_are_normals_similar(Vec3 normal_a, Vec3 normal_b,
                                       float tolerance) {
  return vec3_distance_sq(normal_a, normal_b) <= tolerance * tolerance;
}

// Global accessor functions
ContactCache *get_contact_cache(void) { return &g_contact_cache; }

bool init_contact_cache(const ContactCacheConfig *config) {
  return contact_cache_init(&g_contact_cache, config);
}

void cleanup_contact_cache(void) { contact_cache_cleanup(&g_contact_cache); }
