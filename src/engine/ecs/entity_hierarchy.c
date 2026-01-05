#include "ecs/entity_hierarchy.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_ENTITIES 4096
#define MAX_CHILDREN_PER_ENTITY 32

typedef struct {
    uint64_t parent_id;
    uint64_t children[MAX_CHILDREN_PER_ENTITY];
    uint32_t child_count;
    bool active;
} EntityNode;

static struct {
    EntityNode nodes[MAX_ENTITIES];
    uint32_t entity_count;
} hierarchy_state;

void entity_hierarchy_init(void) {
    memset(&hierarchy_state, 0, sizeof(hierarchy_state));
    LOG_INFO("Entity Hierarchy System Initialized");
}

void entity_hierarchy_shutdown(void) {
    LOG_INFO("Entity Hierarchy System Shutdown");
}

static EntityNode* get_or_create_node(uint64_t entity_id) {
    // Try to find existing
    for (uint32_t i = 0; i < MAX_ENTITIES; i++) {
        if (hierarchy_state.nodes[i].active) {
            // Check if it's this entity by looking at children or parent
            // For simplicity, use the index as the entity ID mapping
            if (i == entity_id % MAX_ENTITIES) {
                return &hierarchy_state.nodes[i];
            }
        }
    }
    
    // Create new
    uint32_t idx = entity_id % MAX_ENTITIES;
    if (!hierarchy_state.nodes[idx].active) {
        hierarchy_state.nodes[idx].active = true;
        hierarchy_state.nodes[idx].parent_id = 0;
        hierarchy_state.nodes[idx].child_count = 0;
        return &hierarchy_state.nodes[idx];
    }
    
    return NULL;
}

static EntityNode* get_node(uint64_t entity_id) {
    uint32_t idx = entity_id % MAX_ENTITIES;
    if (hierarchy_state.nodes[idx].active) {
        return &hierarchy_state.nodes[idx];
    }
    return NULL;
}

void entity_set_parent(uint64_t entity_id, uint64_t parent_id) {
    if (entity_id == parent_id) {
        LOG_WARN("Cannot set entity %llu as its own parent", (unsigned long long)entity_id);
        return;
    }
    
    // Check for circular dependency
    if (entity_is_ancestor_of(entity_id, parent_id)) {
        LOG_WARN("Circular hierarchy detected, cannot set parent");
        return;
    }
    
    EntityNode* entity_node = get_or_create_node(entity_id);
    EntityNode* parent_node = get_or_create_node(parent_id);
    
    if (!entity_node || !parent_node) return;
    
    // Remove from old parent
    if (entity_node->parent_id != 0) {
        EntityNode* old_parent = get_node(entity_node->parent_id);
        if (old_parent) {
            for (uint32_t i = 0; i < old_parent->child_count; i++) {
                if (old_parent->children[i] == entity_id) {
                    old_parent->children[i] = old_parent->children[--old_parent->child_count];
                    break;
                }
            }
        }
    }
    
    // Add to new parent
    if (parent_node->child_count < MAX_CHILDREN_PER_ENTITY) {
        parent_node->children[parent_node->child_count++] = entity_id;
        entity_node->parent_id = parent_id;
        LOG_INFO("Entity %llu now child of %llu", (unsigned long long)entity_id, (unsigned long long)parent_id);
    } else {
        LOG_WARN("Parent entity %llu has max children", (unsigned long long)parent_id);
    }
}

uint64_t entity_get_parent(uint64_t entity_id) {
    EntityNode* node = get_node(entity_id);
    return node ? node->parent_id : 0;
}

void entity_detach(uint64_t entity_id) {
    EntityNode* node = get_node(entity_id);
    if (!node || node->parent_id == 0) return;
    
    EntityNode* parent = get_node(node->parent_id);
    if (parent) {
        for (uint32_t i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == entity_id) {
                parent->children[i] = parent->children[--parent->child_count];
                break;
            }
        }
    }
    
    node->parent_id = 0;
    LOG_INFO("Entity %llu detached from parent", (unsigned long long)entity_id);
}

bool entity_has_parent(uint64_t entity_id) {
    EntityNode* node = get_node(entity_id);
    return node && node->parent_id != 0;
}

bool entity_has_children(uint64_t entity_id) {
    EntityNode* node = get_node(entity_id);
    return node && node->child_count > 0;
}

uint32_t entity_get_child_count(uint64_t entity_id) {
    EntityNode* node = get_node(entity_id);
    return node ? node->child_count : 0;
}

uint32_t entity_get_children(uint64_t entity_id, uint64_t* out_children, uint32_t max_count) {
    EntityNode* node = get_node(entity_id);
    if (!node) return 0;
    
    uint32_t count = node->child_count < max_count ? node->child_count : max_count;
    memcpy(out_children, node->children, count * sizeof(uint64_t));
    return count;
}

void entity_traverse_hierarchy(uint64_t root_id, void(*callback)(uint64_t entity_id, void* user_data), void* user_data) {
    if (!callback) return;
    
    callback(root_id, user_data);
    
    EntityNode* node = get_node(root_id);
    if (!node) return;
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        entity_traverse_hierarchy(node->children[i], callback, user_data);
    }
}

bool entity_is_ancestor_of(uint64_t potential_ancestor, uint64_t entity_id) {
    uint64_t current = entity_id;
    while (current != 0) {
        EntityNode* node = get_node(current);
        if (!node) break;
        
        if (node->parent_id == potential_ancestor) {
            return true;
        }
        current = node->parent_id;
    }
    return false;
}

uint64_t entity_get_root(uint64_t entity_id) {
    uint64_t root = entity_id;
    while (entity_has_parent(root)) {
        root = entity_get_parent(root);
    }
    return root;
}
