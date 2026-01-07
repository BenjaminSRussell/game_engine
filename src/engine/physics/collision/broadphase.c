#include "../physics_engine_core.h"
#include <stdlib.h>
#include <string.h>

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

void broadphase_insert(BroadPhase *bp, uint32_t body_id, const float *min, const float *max) {
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
    // Naive: do nothing
}

static bool aabb_overlap(const float *min1, const float *max1, 
                         const float *min2, const float *max2) {
    if (max1[0] < min2[0] || min1[0] > max2[0]) return false;
    if (max1[1] < min2[1] || min1[1] > max2[1]) return false;
    if (max1[2] < min2[2] || min1[2] > max2[2]) return false;
    return true; 
}

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
    
    return aabb_overlap(n1->bounds_min, n1->bounds_max,
                        n2->bounds_min, n2->bounds_max);
}
