#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *name;
    bool visible;
    bool locked;
    uint32_t color; // For debug visualization
    
    uint64_t *entity_ids; // Entities in this layer
    int entity_count;
} SceneLayer;

typedef struct {
    SceneLayer *layers;
    int layer_count;
    int capacity;
    
    int active_layer; // Current editing layer
} LayerManager;

void layer_manager_init(LayerManager *mgr);
void layer_manager_destroy(LayerManager *mgr);

// Layer management
SceneLayer* layer_create(LayerManager *mgr, const char *name);
void layer_delete(LayerManager *mgr, int layer_index);
void layer_set_active(LayerManager *mgr, int layer_index);

// Layer properties
void layer_set_visible(SceneLayer *layer, bool visible);
void layer_set_locked(SceneLayer *layer, bool locked);

// Entity assignment
void layer_add_entity(SceneLayer *layer, uint64_t entity_id);
void layer_remove_entity(SceneLayer *layer, uint64_t entity_id);

// Visibility sets (save/load layer configs)
void layer_save_visibility_set(LayerManager *mgr, const char *name);
void layer_load_visibility_set(LayerManager *mgr, const char *name);
