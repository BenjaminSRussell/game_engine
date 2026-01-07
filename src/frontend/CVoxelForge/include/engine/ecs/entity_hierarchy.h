#ifndef ENTITY_HIERARCHY_H
#define ENTITY_HIERARCHY_H

#include <stdint.h>
#include <stdbool.h>

void entity_hierarchy_init(void);
void entity_hierarchy_shutdown(void);

// Parent-child relationships
void entity_set_parent(uint64_t entity_id, uint64_t parent_id);
uint64_t entity_get_parent(uint64_t entity_id);
void entity_detach(uint64_t entity_id);

// Queries
bool entity_has_parent(uint64_t entity_id);
bool entity_has_children(uint64_t entity_id);
uint32_t entity_get_child_count(uint64_t entity_id);
uint32_t entity_get_children(uint64_t entity_id, uint64_t* out_children, uint32_t max_count);

// Traversal
void entity_traverse_hierarchy(uint64_t root_id, void(*callback)(uint64_t entity_id, void* user_data), void* user_data);

// Utility
bool entity_is_ancestor_of(uint64_t potential_ancestor, uint64_t entity_id);
uint64_t entity_get_root(uint64_t entity_id);

#endif // ENTITY_HIERARCHY_H
