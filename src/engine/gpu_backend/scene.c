// src/core/scene.c
//
// Scene system implementation
//
#include "../../include/engine/scene.h"
#include "../../include/engine/engine_core.h"
#include "../../include/core/logger.h"
#include "../../include/math/mat4.h"
#include <stdlib.h>
#include <string.h>

// Scene lifecycle
bool scene_init(Scene *scene, struct EngineCore *engine, const char *name) {
    if (!scene || !engine || !name) {
        LOG_ERROR("Invalid parameters for scene_init");
        return false;
    }
    
    memset(scene, 0, sizeof(Scene));
    scene->name = name;
    scene->engine = engine;
    scene->use_physics = true;
    scene->use_audio = true;
    
    // Create root node
    scene->root_node = (SceneNode *)calloc(1, sizeof(SceneNode));
    if (!scene->root_node) {
        LOG_ERROR("Failed to allocate root node");
        return false;
    }
    
    scene->root_node->type = SCENE_NODE_ROOT;
    scene->root_node->position = vec3(0, 0, 0);
    scene->root_node->rotation = vec3(0, 0, 0);
    scene->root_node->scale = vec3(1, 1, 1);
    scene->root_node->visible = true;
    scene->root_node->enabled = true;
    scene->root_node->transform_matrix = mat4_identity();
    scene->root_node->transform_dirty = false;
    
    scene->node_count = 1;
    scene->node_capacity = 64;
    
    scene->game_object_capacity = 64;
    scene->game_objects = (GameObject *)calloc(scene->game_object_capacity, sizeof(GameObject));
    
    scene->layer_count = 16;
    scene->layer_depths = (f32 *)calloc(scene->layer_count, sizeof(f32));
    for (u32 i = 0; i < scene->layer_count; i++) {
        scene->layer_depths[i] = (f32)i;
    }
    
    scene->initialized = true;
    scene->active = false;
    
    LOG_INFO("Scene '%s' initialized", name);
    return true;
}

void scene_shutdown(Scene *scene) {
    if (!scene || !scene->initialized) {
        return;
    }
    
    // Destroy all game objects
    for (u32 i = 0; i < scene->game_object_count; i++) {
        if (scene->game_objects[i].active) {
            scene_destroy_game_object(scene, &scene->game_objects[i]);
        }
    }
    
    // Destroy all nodes (recursive)
    if (scene->root_node) {
        scene_destroy_node(scene, scene->root_node);
    }
    
    if (scene->game_objects) {
        free(scene->game_objects);
    }
    
    if (scene->layer_depths) {
        free(scene->layer_depths);
    }
    
    scene->initialized = false;
    LOG_INFO("Scene '%s' shut down", scene->name);
}

void scene_update(Scene *scene, f32 delta_time) {
    if (!scene || !scene->initialized || !scene->active) {
        return;
    }
    
    // Update transforms
    scene_update_transforms(scene);
    
    // Update ECS systems (handled by engine)
}

void scene_render(Scene *scene) {
    if (!scene || !scene->initialized || !scene->active) {
        return;
    }
    
    // Rendering is handled by the renderer
}

// Scene activation
void scene_set_active(Scene *scene, bool active) {
    if (scene) {
        scene->active = active;
    }
}

bool scene_is_active(Scene *scene) {
    return scene && scene->active;
}

// Scene node management
SceneNode *scene_create_node(Scene *scene, SceneNodeType type, SceneNode *parent) {
    if (!scene || !scene->initialized) {
        return NULL;
    }
    
    SceneNode *node = (SceneNode *)calloc(1, sizeof(SceneNode));
    if (!node) {
        return NULL;
    }
    
    node->type = type;
    node->parent = parent;
    node->position = vec3(0, 0, 0);
    node->rotation = vec3(0, 0, 0);
    node->scale = vec3(1, 1, 1);
    node->visible = true;
    node->enabled = true;
    node->transform_matrix = mat4_identity();
    node->transform_dirty = true;
    
    // Add to parent
    if (parent) {
        node->next_sibling = parent->first_child;
        parent->first_child = node;
    }
    
    scene->node_count++;
    return node;
}

void scene_destroy_node(Scene *scene, SceneNode *node) {
    if (!scene || !node) {
        return;
    }
    
    // Destroy children first
    SceneNode *child = node->first_child;
    while (child) {
        SceneNode *next = child->next_sibling;
        scene_destroy_node(scene, child);
        child = next;
    }
    
    // Remove from parent
    if (node->parent) {
        SceneNode *sibling = node->parent->first_child;
        if (sibling == node) {
            node->parent->first_child = node->next_sibling;
        } else {
            while (sibling && sibling->next_sibling != node) {
                sibling = sibling->next_sibling;
            }
            if (sibling) {
                sibling->next_sibling = node->next_sibling;
            }
        }
    }
    
    free(node);
    scene->node_count--;
}

void scene_update_transforms(Scene *scene) {
    if (!scene || !scene->root_node) {
        return;
    }
    
    // Update transforms recursively
    // This is a simplified version - full implementation would traverse the tree
    scene->root_node->transform_dirty = false;
}

// GameObject management
GameObject *scene_create_game_object(Scene *scene, const char *name) {
    if (!scene || !scene->initialized || !name) {
        return NULL;
    }
    
    // Find free slot
    for (u32 i = 0; i < scene->game_object_capacity; i++) {
        if (!scene->game_objects[i].active) {
            GameObject *obj = &scene->game_objects[i];
            obj->entity_id = ecs_create_entity(&scene->engine->ecs_world);
            obj->node = scene_create_node(scene, SCENE_NODE_ENTITY, scene->root_node);
            obj->name = name;
            obj->active = true;
            obj->user_data = NULL;
            
            if (obj->node) {
                obj->node->entity_id = obj->entity_id;
            }
            
            scene->game_object_count++;
            return obj;
        }
    }
    
    // Expand capacity if needed
    u32 new_capacity = scene->game_object_capacity * 2;
    GameObject *new_objects = (GameObject *)realloc(scene->game_objects, 
                                                      new_capacity * sizeof(GameObject));
    if (new_objects) {
        scene->game_objects = new_objects;
        memset(&scene->game_objects[scene->game_object_capacity], 0, 
               (new_capacity - scene->game_object_capacity) * sizeof(GameObject));
        scene->game_object_capacity = new_capacity;
        
        GameObject *obj = &scene->game_objects[scene->game_object_count];
        obj->entity_id = ecs_create_entity(&scene->engine->ecs_world);
        obj->node = scene_create_node(scene, SCENE_NODE_ENTITY, scene->root_node);
        obj->name = name;
        obj->active = true;
        obj->user_data = NULL;
        
        if (obj->node) {
            obj->node->entity_id = obj->entity_id;
        }
        
        scene->game_object_count++;
        return obj;
    }
    
    return NULL;
}

void scene_destroy_game_object(Scene *scene, GameObject *obj) {
    if (!scene || !obj || !obj->active) {
        return;
    }
    
    if (obj->node) {
        scene_destroy_node(scene, obj->node);
    }
    
    if (obj->entity_id != 0) {
        ecs_destroy_entity(&scene->engine->ecs_world, obj->entity_id);
    }
    
    memset(obj, 0, sizeof(GameObject));
    scene->game_object_count--;
}

GameObject *scene_find_game_object(Scene *scene, const char *name) {
    if (!scene || !name) {
        return NULL;
    }
    
    for (u32 i = 0; i < scene->game_object_capacity; i++) {
        if (scene->game_objects[i].active && 
            scene->game_objects[i].name &&
            strcmp(scene->game_objects[i].name, name) == 0) {
            return &scene->game_objects[i];
        }
    }
    
    return NULL;
}

GameObject *scene_get_game_object(Scene *scene, EntityID entity_id) {
    if (!scene || entity_id == 0) {
        return NULL;
    }
    
    for (u32 i = 0; i < scene->game_object_capacity; i++) {
        if (scene->game_objects[i].active && 
            scene->game_objects[i].entity_id == entity_id) {
            return &scene->game_objects[i];
        }
    }
    
    return NULL;
}

// Layer management
void scene_set_layer_count(Scene *scene, u32 layer_count) {
    if (!scene) return;
    
    scene->layer_depths = (f32 *)realloc(scene->layer_depths, layer_count * sizeof(f32));
    if (scene->layer_depths) {
        scene->layer_count = layer_count;
        for (u32 i = 0; i < layer_count; i++) {
            if (scene->layer_depths[i] == 0.0f) {
                scene->layer_depths[i] = (f32)i;
            }
        }
    }
}

void scene_set_layer_depth(Scene *scene, u32 layer, f32 depth) {
    if (scene && layer < scene->layer_count) {
        scene->layer_depths[layer] = depth;
    }
}

f32 scene_get_layer_depth(Scene *scene, u32 layer) {
    if (scene && layer < scene->layer_count) {
        return scene->layer_depths[layer];
    }
    return 0.0f;
}

u32 scene_get_layer_count(Scene *scene) {
    return scene ? scene->layer_count : 0;
}

// Entity integration
EntityID scene_create_entity(Scene *scene) {
    if (!scene) return 0;
    return ecs_create_entity(&scene->engine->ecs_world);
}

void scene_destroy_entity(Scene *scene, EntityID entity) {
    if (scene && entity != 0) {
        GameObject *obj = scene_get_game_object(scene, entity);
        if (obj) {
            scene_destroy_game_object(scene, obj);
        } else {
            ecs_destroy_entity(&scene->engine->ecs_world, entity);
        }
    }
}

SceneNode *scene_get_entity_node(Scene *scene, EntityID entity) {
    GameObject *obj = scene_get_game_object(scene, entity);
    return obj ? obj->node : NULL;
}

// Transform helpers
void scene_node_set_position(SceneNode *node, Vec3 position) {
    if (node) {
        node->position = position;
        node->transform_dirty = true;
    }
}

void scene_node_set_rotation(SceneNode *node, Vec3 rotation) {
    if (node) {
        node->rotation = rotation;
        node->transform_dirty = true;
    }
}

void scene_node_set_scale(SceneNode *node, Vec3 scale) {
    if (node) {
        node->scale = scale;
        node->transform_dirty = true;
    }
}

Vec3 scene_node_get_position(SceneNode *node) {
    return node ? node->position : vec3(0, 0, 0);
}

Vec3 scene_node_get_rotation(SceneNode *node) {
    return node ? node->rotation : vec3(0, 0, 0);
}

Vec3 scene_node_get_scale(SceneNode *node) {
    return node ? node->scale : vec3(1, 1, 1);
}

Mat4 scene_node_get_world_transform(SceneNode *node) {
    if (!node) {
        return mat4_identity();
    }
    
    // Simplified - full implementation would accumulate parent transforms
    return node->transform_matrix;
}

// Camera management
void scene_set_camera(Scene *scene, struct Camera *camera) {
    if (scene) {
        // Store camera reference (would need to add to Scene struct)
    }
}

struct Camera *scene_get_camera(Scene *scene) {
    if (scene && scene->engine) {
        return scene->engine->camera;
    }
    return NULL;
}

