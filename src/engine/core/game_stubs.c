// game_stubs.c
// Minimal implementations for missing game functions to get the build working

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declarations for types
typedef struct NPC NPC;
typedef struct HousingSystem HousingSystem;
typedef struct DialogueManager DialogueManager;
typedef struct SpiritModel SpiritModel;

// NPC System Stubs
NPC* npc_create(const char* type, float x, float y, float z) {
    (void)type; (void)x; (void)y; (void)z;
    return NULL;
}

void npc_despawn_distant(NPC* npc) {
    (void)npc;
}

void npc_update(NPC* npc, float dt) {
    (void)npc; (void)dt;
}

void npc_jobs_update(float dt) {
    (void)dt;
}

void npc_profile_dump(NPC* npc) {
    (void)npc;
}

void npc_spawn_in_chunk(int chunk_x, int chunk_z) {
    (void)chunk_x; (void)chunk_z;
}

void npc_system_init(void) {}
void npc_system_free(void) {}

// NPC Visuals Stubs
void npc_visuals_init(void) {}
void npc_visuals_free(void) {}

// Housing System Stubs
void housing_update(float dt) {
    (void)dt;
}

// Dialogue System Stubs
void dialogue_manager_update(float dt) {
    (void)dt;
}

// Player Vehicle Stubs
void player_vehicle_init(void* player) {
    (void)player;
}

void player_update_vehicle_control(void* player) {
    (void)player;
}

void player_vehicle_apply_input(void* player, float input) {
    (void)player; (void)input;
}

// Player Magic Stubs
void player_magic_init(void* player) {
    (void)player;
}

void player_magic_update(void* player, float dt) {
    (void)player; (void)dt;
}

void player_cast_spell(void* player, int spell_id) {
    (void)player; (void)spell_id;
}

// Spirit Model Stubs
void spirit_model_init(void) {}
void spirit_model_update_animation(float dt) {
    (void)dt;
}

// Engine System Stubs
void* engine_get_assets(void) { return NULL; }
void* engine_get_audio(void) { return NULL; }
void* engine_get_config(void) { return NULL; }
void* engine_get_entities(void) { return NULL; }
void* engine_get_input(void) { return NULL; }
void* engine_get_physics(void) { return NULL; }
void* engine_get_renderer(void) { return NULL; }
bool engine_is_paused(void) { return false; }
void engine_pause(void) {}
void engine_resume(void) {}

// Combat System Stubs
void combat_system_bind_world(void* world) {
    (void)world;
}

// Game Module Stubs
void game_module_create_default_config(void) {}

// Texture System Stubs
void texture_create_sampler(void) {}
void texture_load_atlas(const char* path) {
    (void)path;
}
void texture_load_atlas_map(const char* path) {
    (void)path;
}
void texture_setup_descriptors(void) {}
void texture_validate_atlas_map(void) {}

// Global Variables
void* g_world_generator = NULL;
void* g_world_seed = NULL;
void* g_weather_system = NULL;
void* g_thread_pool = NULL;
void* g_particle_system = NULL;

// Memory Allocator Stubs
BuddyAllocator* buddy_allocator_create(u32 min_block_size, u32 max_block_size) {
    (void)min_block_size; (void)max_block_size;
    return NULL;
}
void* buddy_allocator_alloc(BuddyAllocator* alloc, u32 size) {
    (void)alloc; (void)size;
    return malloc(size);
}
void buddy_allocator_free(BuddyAllocator* alloc, void* ptr) {
    (void)alloc;
    free(ptr);
}
void buddy_allocator_destroy(BuddyAllocator* alloc) {
    (void)alloc;
}
void buddy_allocator_print_stats(BuddyAllocator* alloc) {
    (void)alloc;
}

LinearAllocator* linear_allocator_create(u64 size) {
    (void)size;
    return NULL;
}
void linear_allocator_destroy(LinearAllocator* alloc) {
    (void)alloc;
}
void* linear_allocator_alloc(LinearAllocator* alloc, u64 size, u64 alignment) {
    (void)alloc; (void)size; (void)alignment;
    return malloc(size);
}
void linear_allocator_reset(LinearAllocator* alloc) {
    (void)alloc;
}
u64 linear_allocator_get_usage(LinearAllocator* alloc) {
    (void)alloc;
    return 0;
}
void linear_allocator_reset_impl(void* allocator) {
    (void)allocator;
}

// Camera Stubs
void camera_init(void) {}
void camera_set_position(float x, float y, float z) {
    (void)x; (void)y; (void)z;
}
void* camera_get_forward(void) { return NULL; }
void* camera_get_view_matrix(void) { return NULL; }
void* camera_get_projection_matrix(void) { return NULL; }

// Block Lighting Stubs
void block_calculate_light_level(int x, int y, int z) {
    (void)x; (void)y; (void)z;
}
void block_propagate_light(int x, int y, int z) {
    (void)x; (void)y; (void)z;
}

// Chunk Manager Stubs
void* chunk_manager_get_block(int x, int y, int z) {
    (void)x; (void)y; (void)z;
    return NULL;
}

// Game Loop Stubs
void game_loop_run(void) {}
void game_loop_set_render_callback(void* callback) {}
void game_loop_set_update_callback(void* callback) {}
void game_loop_set_user_data(void* data) {}
void game_loop_shutdown(void) {}
void game_loop_stop(void) {}

// Hashmap Stubs  
void* hashmap_create(void) { 
    return malloc(sizeof(void*));
}
void hashmap_destroy(void* map) {
    (void)map;
}
void* hashmap_get(void* map, void* key) { 
    (void)map; (void)key;
    return NULL;
}
void hashmap_insert(void* map, void* key, void* value) {
    (void)map; (void)key; (void)value;
}

// Mutex Stubs
void* mutex_create(void) { return NULL; }
void mutex_destroy(void* mutex) {}
void mutex_lock(void* mutex) {}
void mutex_unlock(void* mutex) {}

// Time Stubs
double time_get_high_res_time(void) { return 0.0; }

// GOAP Stubs
void goap_action_apply(void) {}

// Hot Reload Stubs
void hot_reload_init(void) {}
void hot_reload_shutdown(void) {}
void hot_reload_save_state(void) {}
void hot_reload_restore_state(void) {}
void hot_reload_register_module(void) {}

// Survival Stubs
double survival_get_time_of_day(void) { return 0.0; }

// World Generator Stubs
void world_generator_generate_chunk(void) {}

// VFS Stubs
void vfs_init(void) {}
bool vfs_exists(const char* path) {
    (void)path;
    return false;
}
void vfs_mount(const char* path, const char* mount_point) {
    (void)path; (void)mount_point;
}
void vfs_update(void) {}
void vfs_free(void) {}

// Renderer Stubs
void* renderer_create_with_backend(void) { return NULL; }

// Scene Manager Stubs
void scene_manager_init(void) {}
void scene_manager_shutdown(void) {}

// Profiler Stubs
void profiler_init(void) {}
void profiler_shutdown(void) {}

// Post Process Stubs
void post_process_init(void) {}
void post_process_shutdown(void) {}

// Window Stubs
void window_init(void) {}
void window_poll_events(void) {}
void window_swap_buffers(void) {}

// Particle Renderer Stubs
void particle_renderer_free(void) {}

// Rigid Body Stubs
void rigid_body_set_mass(void) {}

// Math Stubs (only needed if real implementations aren't available)
void mat4_mul(void* result, void* a, void* b) {
    (void)result; (void)a; (void)b;
}
void quat_from_euler(void* result, float x, float y, float z) {
    (void)result; (void)x; (void)y; (void)z;
}
void quat_rotate_vec3(void* result, void* quat, void* vec) {
    (void)result; (void)quat; (void)vec;
}

// Additional missing functions
void game_loop_init(void) {}
void framebuffer_create(void) {}
void framebuffer_destroy(void) {}

// Asset Manager Stubs
void* asset_manager_create(void) { return NULL; }
void asset_manager_destroy(void) {}

// Block System Stubs (additional ones)
void block_calculate_light_level(int x, int y, int z) {
    (void)x; (void)y; (void)z;
}
void block_propagate_light(int x, int y, int z) {
    (void)x; (void)y; (void)z;
}

// Buddy Allocator Stubs (additional ones)
void* buddy_allocator_create(size_t size) {
    (void)size;
    return NULL;
}
void* buddy_allocator_alloc(void* allocator, size_t size) {
    (void)allocator; (void)size;
    return malloc(size);
}
void buddy_allocator_free(void* allocator, void* ptr) {
    (void)allocator;
    free(ptr);
}
void buddy_allocator_destroy(void* allocator) {
    (void)allocator;
}

// Camera Stubs (additional ones)
void* camera_get_forward(void) { return NULL; }
void* camera_get_projection_matrix(void) { return NULL; }
