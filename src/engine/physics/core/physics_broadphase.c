#include "physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* =================================================================================================
 *                                    BROADPHASE IMPLEMENTATION (NAIVE)
 * =================================================================================================
 */

void broadphase_init(BroadPhase *bp) {
  if (!bp) return;
  bp->node_capacity = MAX_PHYSICS_BODIES;
  bp->nodes = (BVHNode *)calloc(bp->node_capacity, sizeof(BVHNode));
  bp->node_count = 0;
}

void broadphase_shutdown(BroadPhase *bp) {
  if (bp && bp->nodes) {
    free(bp->nodes);
    bp->nodes = NULL;
  }
}

void broadphase_insert(BroadPhase *bp, uint32_t body_id, const float *min,
                       const float *max) {
  if (!bp || bp->node_count >= bp->node_capacity) return;
  
  BVHNode *node = &bp->nodes[bp->node_count];
  memcpy(node->bounds_min, min, 3 * sizeof(float));
  memcpy(node->bounds_max, max, 3 * sizeof(float));
  node->body_id = body_id;
  bp->node_count++;
}

void broadphase_remove(BroadPhase *bp, uint32_t body_id) {
    if (!bp) return;
    for (uint32_t i = 0; i < bp->node_count; i++) {
        if (bp->nodes[i].body_id == body_id) {
            // Swap with last
            bp->nodes[i] = bp->nodes[bp->node_count - 1];
            bp->node_count--;
            return;
        }
    }
}

void broadphase_update(BroadPhase *bp) {
    // Naive: do nothing, insert already sets bounds.
}

static bool test_aabb_overlap(const float *min_a, const float *max_a, const float *min_b, const float *max_b) {
    if (max_a[0] < min_b[0] || min_a[0] > max_b[0]) return false;
    if (max_a[1] < min_b[1] || min_a[1] > max_b[1]) return false;
    if (max_a[2] < min_b[2] || min_a[2] > max_b[2]) return false;
    return true;
}

void broadphase_find_pairs(BroadPhase *bp, BroadPhasePair *pairs, uint32_t *count) {
    if (!bp || !pairs || !count) return;
    *count = 0;
    
    for (uint32_t i = 0; i < bp->node_count; i++) {
        for (uint32_t j = i + 1; j < bp->node_count; j++) {
            if (*count >= MAX_CONTACT_PAIRS) return;
            
            if (test_aabb_overlap(bp->nodes[i].bounds_min, bp->nodes[i].bounds_max, 
                                  bp->nodes[j].bounds_min, bp->nodes[j].bounds_max)) {
                pairs[*count].body_a = bp->nodes[i].body_id;
                pairs[*count].body_b = bp->nodes[j].body_id;
                (*count)++;
            }
        }
    }
}

// Query if two bodies overlap in broadphase
bool broadphase_query(BroadPhase *bp, uint32_t id1, uint32_t id2) {
    if (!bp) return false;
    
    BVHNode *n1 = NULL;
    BVHNode *n2 = NULL;
    
    for (uint32_t i = 0; i < bp->node_count; i++) {
        if (bp->nodes[i].body_id == id1) n1 = &bp->nodes[i];
        if (bp->nodes[i].body_id == id2) n2 = &bp->nodes[i];
        if (n1 && n2) break;
    }
    
    if (!n1 || !n2) return false;
    
    return test_aabb_overlap(n1->bounds_min, n1->bounds_max,
                             n2->bounds_min, n2->bounds_max);
}
