/**
 * =================================================================================================
 *                              SCRIPTING INTERFACE
 *                              Agent: AGENT_SCRIPT_1
 * =================================================================================================
 */

#ifndef SCRIPT_INTERFACE_H
#define SCRIPT_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    LUA BINDING
 * =================================================================================================
 */

void *lua_state_create(void);
void lua_state_destroy(void *state);
bool lua_load_file(void *state, const char *path);
bool lua_load_string(void *state, const char *code);
bool lua_call_function(void *state, const char *func, int args, int results);
void lua_get_global(void *state, const char *name);
void lua_set_global(void *state, const char *name);
void lua_register_function(void *state, const char *name, void *func);
void lua_register_class(void *state, const char *name, void *meta);
void lua_register_enum(void *state, const char *name, void *values);
void lua_push_value(void *state, void *value);
void *lua_get_value(void *state, int index);
void lua_error_handler(void *state, const char *msg);
void *lua_coroutine_create(void *state);
void lua_coroutine_resume(void *co);
void lua_coroutine_yield(void *co);
void lua_gc(void *state);
void *lua_sandbox_create(void *state);
void lua_hot_reload(void *state, const char *path);

/* =================================================================================================
 *                                    SCRIPT COMPONENT
 * =================================================================================================
 */

void *script_component_create(const char *path);
void script_component_destroy(void *comp);
void script_component_attach(void *comp, uint32_t entity_id);
void script_component_detach(void *comp);
void script_component_awake(void *comp);
void script_component_start(void *comp);
void script_component_update(void *comp, float dt);
void script_component_fixed_update(void *comp, float dt);
void script_component_late_update(void *comp, float dt);
void script_component_on_enable(void *comp);
void script_component_on_disable(void *comp);
void script_component_on_destroy(void *comp);
void script_component_set_property(void *comp, const char *name, void *value);
void *script_component_get_property(void *comp, const char *name);
void script_component_serialize(void *comp, const char *path);
void script_component_deserialize(void *comp, const char *path);

/* =================================================================================================
 *                                    API BINDINGS
 * =================================================================================================
 */

void bind_math_api(void *state);
void bind_entity_api(void *state);
void bind_component_api(void *state);
void bind_transform_api(void *state);
void bind_physics_api(void *state);
void bind_audio_api(void *state);
void bind_input_api(void *state);
void bind_ui_api(void *state);
void bind_scene_api(void *state);
void bind_resource_api(void *state);
void bind_debug_api(void *state);
void bind_time_api(void *state);
void bind_coroutine_api(void *state);
void bind_event_api(void *state);
void bind_network_api(void *state);

/* =================================================================================================
 *                                    VISUAL SCRIPTING
 * =================================================================================================
 */

void *vs_graph_create(void);
void vs_graph_destroy(void *graph);
void *vs_node_create(void *graph, int type);
void vs_node_destroy(void *node);
void vs_node_connect(void *node_a, void *node_b);
void vs_node_disconnect(void *node_a, void *node_b);
bool vs_graph_compile(void *graph);
void vs_graph_execute(void *graph);
bool vs_graph_validate(void *graph);
void vs_node_library_register(void *library);
void *vs_node_library_get(const char *name);
void vs_graph_serialize(void *graph, const char *path);
void vs_graph_deserialize(void *graph, const char *path);
void vs_debugger_start(void *graph);
void vs_debugger_step(void);
void vs_debugger_breakpoint(void *node);

/* =================================================================================================
 *                                    SCRIPT MANAGER
 * =================================================================================================
 */

void script_manager_init(void);
void script_manager_shutdown(void);
void script_manager_update(float dt);
void script_manager_fixed_update(float dt);
void script_manager_late_update(float dt);
void *script_manager_load_script(const char *path);
void script_manager_unload_script(void *script);
void script_manager_reload(void);
void script_manager_gc_collect(void);
uint64_t script_manager_get_memory(void);
void script_manager_stats(void);

#endif // SCRIPT_INTERFACE_H
