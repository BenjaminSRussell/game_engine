#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct SceneNode {
    uint64_t id;
    const char *name;
    struct SceneNode *parent;
    struct SceneNode **children;
    int child_count;
    int child_capacity;
    
    bool visible;
    bool locked;
    int icon_type; // For different entity types
} SceneNode;

typedef struct {
    SceneNode *root;
    SceneNode **flat_list; // For virtualized rendering
    int node_count;
    
    SceneNode **selected_nodes;
    int selection_count;
    
    char search_filter[128];
} SceneHierarchy;

void scene_hierarchy_init(SceneHierarchy *hierarchy);
void scene_hierarchy_destroy(SceneHierarchy *hierarchy);

// Node management
SceneNode* scene_hierarchy_add_node(SceneHierarchy *hierarchy, const char *name, SceneNode *parent);
void scene_hierarchy_remove_node(SceneHierarchy *hierarchy, SceneNode *node);
void scene_hierarchy_reparent(SceneNode *node, SceneNode *new_parent);

// Selection
void scene_hierarchy_select(SceneHierarchy *hierarchy, SceneNode *node, bool add_to_selection);
void scene_hierarchy_select_range(SceneHierarchy *hierarchy, SceneNode *start, SceneNode *end);
void scene_hierarchy_clear_selection(SceneHierarchy *hierarchy);

// Filtering
void scene_hierarchy_set_filter(SceneHierarchy *hierarchy, const char *filter);

// Rendering
void scene_hierarchy_draw(SceneHierarchy *hierarchy);
