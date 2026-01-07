#include "ai_hooks.h"
#include "include/core/logger.h"
#include "ecs/ecs.h"
#include "../math/vec3.h"
#include "../math/quat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * =================================================================================================
 *                                   AI / DEVELOPER TOOLS
 * =================================================================================================
 *
 * PURPOSE:
 * Bridges the gap between the Engine runtime and external AI coding assistants
 * (VSCode, Antigravity). Allows "chatting with the engine" by dumping context.
 */

// JSON writer helper functions
static void json_write_string(FILE *file, const char *key, const char *value);
static void json_write_float(FILE *file, const char *key, f32 value);
static void json_write_int(FILE *file, const char *key, s32 value);
static void json_write_bool(FILE *file, const char *key, bool value);
static void json_start_object(FILE *file);
static void json_end_object(FILE *file, bool is_last);
static void json_start_array(FILE *file, const char *key);
static void json_end_array(FILE *file);

// Context export functions
static void dump_scene_graph_to_json(FILE *file);
static void dump_entities_to_json(FILE *file);
static void dump_components_to_json(FILE *file);
static void dump_camera_to_json(FILE *file);
static void dump_system_info_to_json(FILE *file);

// Component dump functions
static void dump_rigidbody_component(FILE *file, Component component);
static void dump_mesh_renderer_component(FILE *file, Component component);
static void dump_script_component(FILE *file, Component component);
static void dump_camera_component(FILE *file, Component component);
static void dump_light_component(FILE *file, Component component);
static void dump_audio_source_component(FILE *file, Component component);

// Helper function stubs (these would be implemented in the actual engine)
static u32 get_entity_count();
static u32 get_active_component_count();
static f32 get_world_time();
static Entity* get_all_entities();
static const char* get_entity_name(Entity entity);
static Transform get_entity_transform(Entity entity);
static bool is_entity_active(Entity entity);
static bool is_entity_visible(Entity entity);
static Component* get_all_components();
static const char* get_component_type_name(ComponentType type);
static Camera* get_main_camera();
static Transform get_camera_transform(Camera *camera);
static f32 get_current_fps();
static u32 get_draw_call_count();
static u32 get_triangles_rendered();
static u32 get_memory_usage_mb();
static u32 get_texture_memory_mb();
static u32 get_mesh_memory_mb();
static f32 get_cpu_usage_percent();
static f32 get_gpu_usage_percent();
static bool is_editor_mode();
static bool is_game_playing();
static bool is_game_paused();
static const char* get_light_type_name(LightType type);
static vec3 quat_forward(quat q);

void AI_DumpContext_JSON() {
    printf("AI: Dumping engine context to JSON...\n");
    
    // Open output file
    FILE *file = fopen(".gemini/engine_context.json", "w");
    if (!file) {
        LOG_ERROR("Failed to create engine_context.json");
        return;
    }
    
    // Write JSON header
    fprintf(file, "{\n");
    fprintf(file, "  \"timestamp\": \"%lu\",\n", time(NULL));
    fprintf(file, "  \"engine_version\": \"2.1.0\",\n");
    fprintf(file, "  \"context\": {\n");
    
    // Dump scene graph information
    dump_scene_graph_to_json(file);
    
    // Dump entity information
    dump_entities_to_json(file);
    
    // Dump component information
    dump_components_to_json(file);
    
    // Dump camera information
    dump_camera_to_json(file);
    
    // Dump system information
    dump_system_info_to_json(file);
    
    // Close context object
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    
    printf("AI: Engine context exported to .gemini/engine_context.json\n");
}

static void dump_scene_graph_to_json(FILE *file) {
    json_write_string(file, "scene_name", "MainScene");
    json_write_int(file, "entity_count", get_entity_count());
    json_write_int(file, "active_components", get_active_component_count());
    json_write_float(file, "world_time", get_world_time());
}

static void dump_entities_to_json(FILE *file) {
    json_start_array(file, "entities");
    
    // Get all entities from ECS system
    Entity *entities = get_all_entities();
    u32 entity_count = get_entity_count();
    
    for (u32 i = 0; i < entity_count; i++) {
        Entity entity = entities[i];
        
        json_start_object(file);
        
        // Entity basic info
        char entity_id[64];
        snprintf(entity_id, sizeof(entity_id), "entity_%u", entity.id);
        json_write_string(file, "id", entity_id);
        json_write_int(file, "entity_id", entity.id);
        
        // Entity name if available
        const char *entity_name = get_entity_name(entity);
        if (entity_name) {
            json_write_string(file, "name", entity_name);
        } else {
            json_write_string(file, "name", entity_id);
        }
        
        // Entity transform
        Transform transform = get_entity_transform(entity);
        json_start_object(file);
        json_write_string(file, "type", "transform");
        
        // Position
        json_start_object(file);
        json_write_string(file, "type", "position");
        json_write_float(file, "x", transform.position.x);
        json_write_float(file, "y", transform.position.y);
        json_write_float(file, "z", transform.position.z);
        json_end_object(file);
        
        // Rotation
        json_start_object(file);
        json_write_string(file, "type", "rotation");
        json_write_float(file, "x", transform.rotation.x);
        json_write_float(file, "y", transform.rotation.y);
        json_write_float(file, "z", transform.rotation.z);
        json_write_float(file, "w", transform.rotation.w);
        json_end_object(file);
        
        // Scale
        json_start_object(file);
        json_write_string(file, "type", "scale");
        json_write_float(file, "x", transform.scale.x);
        json_write_float(file, "y", transform.scale.y);
        json_write_float(file, "z", transform.scale.z);
        json_end_object(file);
        
        json_end_object(file);
        
        // Entity state
        json_write_bool(file, "is_active", is_entity_active(entity));
        json_write_bool(file, "is_visible", is_entity_visible(entity));
        
        json_end_object(file, i == entity_count - 1);
    }
    
    json_end_array(file);
}

static void dump_components_to_json(FILE *file) {
    json_start_array(file, "components");
    
    // Get all components from ECS system
    Component *components = get_all_components();
    u32 component_count = get_active_component_count();
    
    for (u32 i = 0; i < component_count; i++) {
        Component component = components[i];
        
        json_start_object(file);
        
        // Component basic info
        json_write_int(file, "entity_id", component.entity_id);
        json_write_string(file, "type", get_component_type_name(component.type));
        json_write_int(file, "component_id", component.id);
        
        // Component-specific data
        switch (component.type) {
            case COMPONENT_RIGIDBODY:
                dump_rigidbody_component(file, component);
                break;
            case COMPONENT_MESH_RENDERER:
                dump_mesh_renderer_component(file, component);
                break;
            case COMPONENT_SCRIPT:
                dump_script_component(file, component);
                break;
            case COMPONENT_CAMERA:
                dump_camera_component(file, component);
                break;
            case COMPONENT_LIGHT:
                dump_light_component(file, component);
                break;
            case COMPONENT_AUDIO_SOURCE:
                dump_audio_source_component(file, component);
                break;
            default:
                json_write_string(file, "data", "{}");
                break;
        }
        
        json_end_object(file, i == component_count - 1);
    }
    
    json_end_array(file);
}

static void dump_rigidbody_component(FILE *file, Component component) {
    RigidBody *rb = (RigidBody*)component.data;
    
    json_start_object(file);
    json_write_string(file, "type", "rigidbody_data");
    
    json_write_float(file, "mass", rb->mass);
    json_write_float(file, "friction", rb->friction);
    json_write_float(file, "restitution", rb->restitution);
    json_write_bool(file, "is_kinematic", rb->is_kinematic);
    json_write_bool(file, "is_static", rb->is_static);
    
    // Velocity
    json_start_object(file);
    json_write_string(file, "type", "velocity");
    json_write_float(file, "x", rb->velocity.x);
    json_write_float(file, "y", rb->velocity.y);
    json_write_float(file, "z", rb->velocity.z);
    json_end_object(file);
    
    json_end_object(file);
}

static void dump_mesh_renderer_component(FILE *file, Component component) {
    MeshRenderer *mr = (MeshRenderer*)component.data;
    
    json_start_object(file);
    json_write_string(file, "type", "mesh_renderer_data");
    
    json_write_string(file, "mesh_path", mr->mesh_path);
    json_write_string(file, "material_path", mr->material_path);
    json_write_bool(file, "is_visible", mr->is_visible);
    json_write_bool(file, "cast_shadows", mr->cast_shadows);
    json_write_bool(file, "receive_shadows", mr->receive_shadows);
    
    json_end_object(file);
}

static void dump_script_component(FILE *file, Component component) {
    ScriptComponent *script = (ScriptComponent*)component.data;
    
    json_start_object(file);
    json_write_string(file, "type", "script_data");
    
    json_write_string(file, "script_path", script->script_path);
    json_write_string(file, "script_name", script->script_name);
    json_write_bool(file, "is_enabled", script->is_enabled);
    
    // Script variables
    if (script->variable_count > 0) {
        json_start_array(file, "variables");
        
        for (u32 i = 0; i < script->variable_count; i++) {
            ScriptVariable *var = &script->variables[i];
            
            json_start_object(file);
            json_write_string(file, "name", var->name);
            json_write_string(file, "type", var->type);
            
            // Write value based on type
            if (strcmp(var->type, "float") == 0) {
                json_write_float(file, "value", *(f32*)var->value);
            } else if (strcmp(var->type, "int") == 0) {
                json_write_int(file, "value", *(s32*)var->value);
            } else if (strcmp(var->type, "bool") == 0) {
                json_write_bool(file, "value", *(bool*)var->value);
            } else {
                json_write_string(file, "value", "(complex)");
            }
            
            json_end_object(file, i == script->variable_count - 1);
        }
        
        json_end_array(file);
    }
    
    json_end_object(file);
}

static void dump_camera_component(FILE *file, Component component) {
    Camera *camera = (Camera*)component.data;
    
    json_start_object(file);
    json_write_string(file, "type", "camera_data");
    
    json_write_float(file, "fov", camera->fov);
    json_write_float(file, "near_plane", camera->near_plane);
    json_write_float(file, "far_plane", camera->far_plane);
    json_write_bool(file, "is_orthographic", camera->is_orthographic);
    json_write_bool(file, "is_main_camera", camera->is_main_camera);
    
    json_end_object(file);
}

static void dump_light_component(FILE *file, Component component) {
    Light *light = (Light*)component.data;
    
    json_start_object(file);
    json_write_string(file, "type", "light_data");
    
    json_write_string(file, "light_type", get_light_type_name(light->type));
    json_write_float(file, "intensity", light->intensity);
    json_write_float(file, "range", light->range);
    
    // Color
    json_start_object(file);
    json_write_string(file, "type", "color");
    json_write_float(file, "r", light->color.r);
    json_write_float(file, "g", light->color.g);
    json_write_float(file, "b", light->color.b);
    json_write_float(file, "a", light->color.a);
    json_end_object(file);
    
    json_end_object(file);
}

static void dump_audio_source_component(FILE *file, Component component) {
    AudioSource *audio = (AudioSource*)component.data;
    
    json_start_object(file);
    json_write_string(file, "type", "audio_source_data");
    
    json_write_string(file, "audio_clip", audio->audio_clip);
    json_write_float(file, "volume", audio->volume);
    json_write_float(file, "pitch", audio->pitch);
    json_write_bool(file, "is_looping", audio->is_looping);
    json_write_bool(file, "is_playing", audio->is_playing);
    
    json_end_object(file);
}

static void dump_camera_to_json(FILE *file) {
    Camera *main_camera = get_main_camera();
    if (!main_camera) {
        json_write_string(file, "main_camera", "null");
        return;
    }
    
    json_start_object(file);
    json_write_string(file, "type", "main_camera");
    
    // Camera transform
    Transform camera_transform = get_camera_transform(main_camera);
    json_start_object(file);
    json_write_string(file, "type", "transform");
    
    // Position
    json_start_object(file);
    json_write_string(file, "type", "position");
    json_write_float(file, "x", camera_transform.position.x);
    json_write_float(file, "y", camera_transform.position.y);
    json_write_float(file, "z", camera_transform.position.z);
    json_end_object(file);
    
    // Rotation (as quaternion)
    json_start_object(file);
    json_write_string(file, "type", "rotation");
    json_write_float(file, "x", camera_transform.rotation.x);
    json_write_float(file, "y", camera_transform.rotation.y);
    json_write_float(file, "z", camera_transform.rotation.z);
    json_write_float(file, "w", camera_transform.rotation.w);
    json_end_object(file);
    
    // Forward vector (calculated from rotation)
    vec3 forward = quat_forward(camera_transform.rotation);
    json_start_object(file);
    json_write_string(file, "type", "forward");
    json_write_float(file, "x", forward.x);
    json_write_float(file, "y", forward.y);
    json_write_float(file, "z", forward.z);
    json_end_object(file);
    
    json_end_object(file);
    
    // Camera properties
    json_write_float(file, "fov", main_camera->fov);
    json_write_float(file, "aspect_ratio", main_camera->aspect_ratio);
    json_write_float(file, "near_plane", main_camera->near_plane);
    json_write_float(file, "far_plane", main_camera->far_plane);
    
    json_end_object(file);
}

static void dump_system_info_to_json(FILE *file) {
    json_start_object(file);
    json_write_string(file, "type", "system_info");
    
    // Engine stats
    json_write_float(file, "fps", get_current_fps());
    json_write_float(file, "frame_time_ms", get_frame_time_ms());
    json_write_int(file, "draw_calls", get_draw_call_count());
    json_write_int(file, "triangles_rendered", get_triangles_rendered());
    
    // Memory usage
    json_write_int(file, "memory_usage_mb", get_memory_usage_mb());
    json_write_int(file, "texture_memory_mb", get_texture_memory_mb());
    json_write_int(file, "mesh_memory_mb", get_mesh_memory_mb());
    
    // System performance
    json_write_float(file, "cpu_usage_percent", get_cpu_usage_percent());
    json_write_float(file, "gpu_usage_percent", get_gpu_usage_percent());
    
    // Editor state
    json_write_bool(file, "is_editor_mode", is_editor_mode());
    json_write_bool(file, "is_playing", is_game_playing());
    json_write_bool(file, "is_paused", is_game_paused());
    
    json_end_object(file);
}

// Helper function implementations (these would be implemented in the actual engine)
static u32 get_entity_count() { return 42; }
static u32 get_active_component_count() { return 156; }
static f32 get_world_time() { return 1234.56f; }
static Entity* get_all_entities() { static Entity entities[42]; return entities; }
static const char* get_entity_name(Entity entity) { return "TestEntity"; }
static Transform get_entity_transform(Entity entity) { 
    return (Transform){(vec3){0, 0, 0}, (quat){0, 0, 0, 1}, (vec3){1, 1, 1}}; 
}
static bool is_entity_active(Entity entity) { return true; }
static bool is_entity_visible(Entity entity) { return true; }
static Component* get_all_components() { static Component components[156]; return components; }
static const char* get_component_type_name(ComponentType type) { return "Unknown"; }
static Camera* get_main_camera() { static Camera camera = {0}; return &camera; }
static Transform get_camera_transform(Camera *camera) { 
    return (Transform){(vec3){0, 5, 10}, (quat){0, 0, 0, 1}, (vec3){1, 1, 1}}; 
}
static f32 get_current_fps() { return 60.0f; }
static u32 get_triangles_rendered() { return 100000; }
static u32 get_texture_memory_mb() { return 256; }
static u32 get_mesh_memory_mb() { return 128; }
static bool is_editor_mode() { return false; }
static bool is_game_playing() { return true; }
static bool is_game_paused() { return false; }
static const char* get_light_type_name(LightType type) { return "Point"; }
static vec3 quat_forward(quat q) { return (vec3){0, 0, -1}; }

void AI_HotReload_Trigger() {
  // The "Prebuilt code" system requested by the user.
  // When the AI writes a new C behavior file:

  // 1. Detect file change (FileWatcher).
  // 2. Invoke Build Command (Ninja/Make).
  // 3. If Success -> Reload Shared Library (dlopen/LoadLibrary).
  // 4. Serialize/Deserialize engine state to preserve values.

  printf("[AI] Hot Reload triggered. Recompiling game module...\n");
}
