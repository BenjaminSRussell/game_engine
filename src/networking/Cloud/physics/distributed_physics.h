#ifndef DISTRIBUTED_PHYSICS_H
#define DISTRIBUTED_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    Vec3 force;
    float mass;
    float radius;
    uint32_t id;
    bool is_active;
} PhysicsBody;

typedef struct {
    Vec3 normal;
    float depth;
    uint32_t body1_id;
    uint32_t body2_id;
} Collision;

typedef struct {
    PhysicsBody* bodies;
    uint32_t body_count;
    uint32_t max_bodies;
    Collision* collisions;
    uint32_t collision_count;
    uint32_t max_collisions;
    float time_step;
    Vec3 gravity;
    float damping;
} PhysicsWorld;

typedef struct {
    uint32_t server_id;
    char address[64];
    uint16_t port;
    bool is_connected;
    float latency_ms;
    uint32_t region_start_x;
    uint32_t region_start_z;
    uint32_t region_size;
} PhysicsServer;

typedef struct {
    PhysicsServer* servers;
    uint32_t server_count;
    uint32_t active_servers;
    PhysicsWorld* local_world;
    uint32_t local_region_start_x;
    uint32_t local_region_start_z;
    uint32_t region_size;
    bool is_master;
    uint32_t sync_frequency;
    float last_sync_time;
} DistributedPhysicsManager;

// Distributed physics initialization
DistributedPhysicsManager* distributed_physics_create_manager(bool is_master);
bool distributed_physics_destroy_manager(DistributedPhysicsManager* manager);

// Server management
bool distributed_physics_add_server(DistributedPhysicsManager* manager, const char* address, uint16_t port, 
                                   uint32_t region_start_x, uint32_t region_start_z, uint32_t region_size);
bool distributed_physics_remove_server(DistributedPhysicsManager* manager, uint32_t server_id);
bool distributed_physics_connect_to_servers(DistributedPhysicsManager* manager);
bool distributed_physics_disconnect_from_servers(DistributedPhysicsManager* manager);

// Region management
bool distributed_physics_set_local_region(DistributedPhysicsManager* manager, uint32_t start_x, uint32_t start_z, uint32_t size);
bool distributed_physics_assign_regions(DistributedPhysicsManager* manager);
bool distributed_physics_rebalance_regions(DistributedPhysicsManager* manager);

// Body management across servers
bool distributed_physics_add_body(DistributedPhysicsManager* manager, const PhysicsBody* body);
bool distributed_physics_remove_body(DistributedPhysicsManager* manager, uint32_t body_id);
bool distributed_physics_update_body(DistributedPhysicsManager* manager, const PhysicsBody* body);
bool distributed_physics_transfer_body(DistributedPhysicsManager* manager, uint32_t body_id, uint32_t target_server_id);

// Physics simulation
bool distributed_physics_step(DistributedPhysicsManager* manager, float delta_time);
bool distributed_physics_simulate_local(DistributedPhysicsManager* manager, float delta_time);
bool distributed_physics_simulate_remote(DistributedPhysicsManager* manager, float delta_time);

// Synchronization and consistency
typedef struct {
    uint32_t body_id;
    Vec3 position;
    Vec3 velocity;
    uint32_t timestamp;
    uint32_t server_id;
} BodyStateUpdate;

typedef struct {
    BodyStateUpdate* updates;
    uint32_t update_count;
    uint32_t max_updates;
} StateSyncBuffer;

bool distributed_physics_sync_states(DistributedPhysicsManager* manager);
bool distributed_physics_send_state_updates(DistributedPhysicsManager* manager);
bool distributed_physics_receive_state_updates(DistributedPhysicsManager* manager);
bool distributed_physics_apply_state_updates(DistributedPhysicsManager* manager, const BodyStateUpdate* updates, uint32_t count);

// Collision detection across regions
bool distributed_physics_detect_cross_region_collisions(DistributedPhysicsManager* manager);
bool distributed_physics_resolve_cross_region_collisions(DistributedPhysicsManager* manager);
bool distributed_physics_send_collision_data(DistributedPhysicsManager* manager, const Collision* collisions, uint32_t count);

// Load balancing
typedef struct {
    uint32_t server_id;
    uint32_t body_count;
    float cpu_usage;
    float network_usage;
    uint32_t region_count;
} ServerLoadInfo;

bool distributed_physics_get_server_loads(DistributedPhysicsManager* manager, ServerLoadInfo* loads, uint32_t max_loads);
bool distributed_physics_balance_load(DistributedPhysicsManager* manager);
bool distributed_physics_migrate_bodies(DistributedPhysicsManager* manager, uint32_t from_server, uint32_t to_server, uint32_t count);

// Fault tolerance and recovery
bool distributed_physics_detect_server_failure(DistributedPhysicsManager* manager);
bool distributed_physics_handle_server_failure(DistributedPhysicsManager* manager, uint32_t failed_server_id);
bool distributed_physics_recover_lost_state(DistributedPhysicsManager* manager, uint32_t server_id);

// Optimization and performance
typedef struct {
    uint32_t total_bodies;
    uint32_t local_bodies;
    uint32_t remote_bodies;
    uint32_t cross_region_collisions;
    float sync_overhead_ms;
    float simulation_time_ms;
    float network_latency_ms;
    uint32_t messages_sent;
    uint32_t messages_received;
} DistributedPhysicsStats;

DistributedPhysicsStats distributed_physics_get_statistics(const DistributedPhysicsManager* manager);
bool distributed_physics_enable_optimization(DistributedPhysicsManager* manager, bool enabled);
bool distributed_physics_set_sync_frequency(DistributedPhysicsManager* manager, uint32_t frequency_hz);

// Interest management
typedef struct {
    Vec3 center;
    float radius;
    uint32_t max_bodies;
    bool include_remote;
} InterestArea;

bool distributed_physics_set_interest_area(DistributedPhysicsManager* manager, const InterestArea* area);
bool distributed_physics_get_bodies_in_area(const DistributedPhysicsManager* manager, const InterestArea* area, 
                                           PhysicsBody* bodies, uint32_t max_bodies, uint32_t* count);

// Prediction and interpolation
typedef struct {
    uint32_t body_id;
    Vec3 predicted_position;
    Vec3 predicted_velocity;
    float confidence;
    uint32_t prediction_time;
} BodyPrediction;

bool distributed_physics_predict_remote_bodies(DistributedPhysicsManager* manager, float prediction_time);
bool distributed_physics_interpolate_states(DistributedPhysicsManager* manager, float alpha);
bool distributed_physics_get_body_predictions(const DistributedPhysicsManager* manager, BodyPrediction* predictions, uint32_t max_predictions);

// Debugging and monitoring
bool distributed_physics_enable_debug_mode(DistributedPhysicsManager* manager, bool enabled);
bool distributed_physics_export_debug_data(const DistributedPhysicsManager* manager, const char* filename);
bool distributed_physics_validate_consistency(const DistributedPhysicsManager* manager);

// Utility functions
uint32_t distributed_physics_get_server_for_position(const DistributedPhysicsManager* manager, const Vec3* position);
bool distributed_physics_is_position_in_region(const Vec3* position, uint32_t start_x, uint32_t start_z, uint32_t size);
float distributed_physics_calculate_distance(const Vec3* a, const Vec3* b);

// Cleanup
void distributed_physics_cleanup(DistributedPhysicsManager* manager);

#ifdef __cplusplus
}
#endif

#endif // DISTRIBUTED_PHYSICS_H
