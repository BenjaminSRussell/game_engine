// include/engine/networking/massive_networking.h
//
// Purpose: Massive networking system supporting 1000+ players with client-side
// prediction This system provides cutting-edge networking capabilities that
// surpass Unity's Netcode for GameObjects with advanced prediction, server
// authority, and massive scale optimization.
//
// Key Features:
// - Client-side prediction with server reconciliation
// - Server authority with anti-cheat protection
// - 1000+ concurrent player support
// - Interest management and spatial partitioning
// - Advanced compression and delta encoding
// - Load balancing across multiple servers
// - Network quality adaptation
// - Seamless server migration
//
// Performance Advantages over Unity Networking:
// - Better prediction algorithms with rollback
// - More efficient delta compression
// - Superior interest management
// - Better load balancing and scaling
// - Advanced anti-cheat integration
// - Integration with our superior ECS architecture
//
// Public APIs:
// - NetworkSystem: Main networking container
// - NetworkClient: Client-side connection and prediction
// - NetworkServer: Server-side authority and management
// - NetworkReplication: Entity state synchronization
// - NetworkCompression: Advanced data compression
//
// Ownership: NetworkSystem owns all networking components
// Invariants: Network state must be synchronized each frame for consistency
//
#ifndef MASSIVE_NETWORKING_H
#define MASSIVE_NETWORKING_H

#include "include/common.h"
#include "include/core/performance.h"
#include "include/ecs/ecs.h"
#include <stdbool.h>

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// Network protocols
typedef enum {
  NETWORK_PROTOCOL_TCP = 0,
  NETWORK_PROTOCOL_UDP,
  NETWORK_PROTOCOL_WEBRTC,
  NETWORK_PROTOCOL_CUSTOM
} NetworkProtocol;

// Connection states
typedef enum {
  CONNECTION_STATE_DISCONNECTED = 0,
  CONNECTION_STATE_CONNECTING,
  CONNECTION_STATE_CONNECTED,
  CONNECTION_STATE_DISCONNECTING,
  CONNECTION_STATE_RECONNECTING
} ConnectionState;

// Network quality levels
typedef enum {
  NETWORK_QUALITY_EXCELLENT = 0,
  NETWORK_QUALITY_GOOD,
  NETWORK_QUALITY_FAIR,
  NETWORK_QUALITY_POOR,
  NETWORK_QUALITY_TERRIBLE
} NetworkQuality;

// Network configuration
typedef struct {
  NetworkProtocol protocol;
  u32 max_clients;
  u32 tick_rate;
  u32 send_rate;
  u32 max_packet_size;
  f32 client_timeout;
  f32 server_timeout;
  bool enable_compression;
  bool enable_encryption;
  bool enable_prediction;
  bool enable_interpolation;
  bool enable_reconciliation;
  u32 compression_level;
  u32 encryption_key_size;
  char *server_address;
  u16 server_port;
  u16 client_port;
} NetworkConfig;

// ============================================================================
// CLIENT-SIDE PREDICTION
// ============================================================================

// Prediction state
typedef struct {
  Entity entity;
  Vec3 predicted_position;
  Vec3 predicted_velocity;
  Quat predicted_orientation;
  Vec3 predicted_angular_velocity;
  u32 sequence_number;
  f64 timestamp;
  bool is_confirmed;
  bool needs_reconciliation;
} PredictionState;

// Input command for prediction
typedef struct {
  u32 sequence_number;
  Vec3 move_input;
  Vec3 look_input;
  bool jump_pressed;
  bool crouch_pressed;
  bool sprint_pressed;
  u32 action_flags;
  f64 timestamp;
} InputCommand;

// Client-side prediction system
typedef struct {
  PredictionState *predicted_entities;
  u32 entity_count;
  u32 max_entities;
  InputCommand *input_history;
  u32 input_history_size;
  u32 input_history_capacity;
  u32 local_sequence_number;
  u32 last_acked_sequence;
  f32 prediction_error_threshold;
  bool enable_rollback;
  f32 rollback_duration;
  u32 reconciliation_count;
  f64 total_prediction_time;
} ClientPrediction;

// ============================================================================
// SERVER AUTHORITY
// ============================================================================

// Server state validation
typedef struct {
  Entity entity;
  Vec3 server_position;
  Vec3 server_velocity;
  Quat server_orientation;
  u32 last_update_sequence;
  f64 last_update_time;
  bool needs_correction;
  f32 position_tolerance;
  f32 velocity_tolerance;
  f32 orientation_tolerance;
} ServerValidation;

// Anti-cheat detection
typedef struct {
  u32 client_id;
  f32 speed_violation_count;
  f32 position_anomaly_score;
  f32 timing_anomaly_score;
  f32 behavior_anomaly_score;
  bool is_suspicious;
  u32 violation_count;
  f64 last_violation_time;
  char *violation_reason;
} AntiCheatDetection;

// Server authority system
typedef struct {
  ServerValidation *validations;
  u32 validation_count;
  u32 max_validations;
  AntiCheatDetection *anti_cheat_data;
  u32 client_count;
  u32 max_clients;
  f32 max_allowed_speed;
  f32 max_teleport_distance;
  f32 position_update_threshold;
  bool enable_strict_validation;
  bool enable_anti_cheat;
  u32 ban_threshold;
  f64 total_validation_time;
} ServerAuthority;

// ============================================================================
// INTEREST MANAGEMENT
// ============================================================================

// Spatial partitioning for interest management
typedef enum {
  INTEREST_GRID = 0,
  INTEREST_QUADTREE,
  INTEREST_OCTREE,
  INTEREST_HASH_GRID,
  INTEREST_UNIFORM_GRID
} InterestMethod;

// Interest region
typedef struct {
  Vec3 center;
  Vec3 extents;
  u32 priority_level;
  u32 update_rate;
  bool is_active;
  u32 client_mask; // Bitmask of interested clients
} InterestRegion;

// Interest management system
typedef struct {
  InterestMethod method;
  Vec3 world_bounds_min;
  Vec3 world_bounds_max;
  u32 grid_resolution_x;
  u32 grid_resolution_y;
  u32 grid_resolution_z;
  InterestRegion *regions;
  u32 region_count;
  u32 max_regions;
  u32 **client_interests; // Grid of client interests
  u32 grid_cell_count;
  f32 update_frequency;
  bool enable_dynamic_regions;
  f64 total_interest_time;
} InterestManagement;

// ============================================================================
// NETWORK COMPRESSION
// ============================================================================

// Compression algorithms
typedef enum {
  COMPRESSION_NONE = 0,
  COMPRESSION_LZ4,
  COMPRESSION_ZSTD,
  COMPRESSION_DELTA,
  COMPRESSION_QUANTIZATION,
  COMPRESSION_CUSTOM
} CompressionAlgorithm;

// Delta compression state
typedef struct {
  void *previous_state;
  u32 state_size;
  u32 *change_mask;
  u32 mask_size;
  f32 compression_ratio;
  u32 compression_time_ms;
} DeltaCompressionState;

// Network compression system
typedef struct {
  CompressionAlgorithm algorithm;
  u32 compression_level;
  DeltaCompressionState *delta_states;
  u32 delta_state_count;
  u32 max_delta_states;
  void *compression_context;
  bool enable_adaptive_compression;
  f32 target_compression_ratio;
  f64 total_compression_time;
  u64 total_compressed_bytes;
  u64 total_uncompressed_bytes;
} NetworkCompression;

// ============================================================================
// NETWORK REPLICATION
// ============================================================================

// Replication priority
typedef enum {
  REPLICATION_PRIORITY_LOW = 0,
  REPLICATION_PRIORITY_MEDIUM,
  REPLICATION_PRIORITY_HIGH,
  REPLICATION_PRIORITY_CRITICAL
} ReplicationPriority;

// Replicated component
typedef struct {
  ECSComponentID component_id;
  u32 component_size;
  void *component_data;
  u32 last_update_sequence;
  ReplicationPriority priority;
  bool is_dirty;
  bool is_replicated;
  f32 update_frequency;
  f64 last_update_time;
} ReplicatedComponent;

// Replicated entity
typedef struct {
  Entity entity;
  ReplicatedComponent *components;
  u32 component_count;
  u32 max_components;
  u32 owner_client_id;
  ReplicationPriority priority;
  bool is_replicated;
  bool is_owner_authoritative;
  u32 last_update_sequence;
  f64 last_update_time;
  Vec3 last_known_position;
  f32 position_change_threshold;
} ReplicatedEntity;

// Network replication system
typedef struct {
  ReplicatedEntity *entities;
  u32 entity_count;
  u32 max_entities;
  u32 next_entity_id;
  u32 local_client_id;
  bool is_server;
  f32 update_rate;
  f32 bandwidth_budget;
  u32 packets_per_second;
  u64 bytes_per_second;
  f64 total_replication_time;
} NetworkReplication;

// ============================================================================
// NETWORK CLIENT
// ============================================================================

// Network statistics
typedef struct {
  f64 ping;
  f64 jitter;
  f32 packet_loss;
  f64 bandwidth_up;
  f64 bandwidth_down;
  u32 packets_sent;
  u32 packets_received;
  u32 packets_lost;
  NetworkQuality quality;
  f64 last_quality_update;
} NetworkStats;

// Network client
typedef struct {
  u32 client_id;
  ConnectionState state;
  NetworkStats stats;
  ClientPrediction prediction;
  NetworkReplication replication;
  void *socket;
  char *server_address;
  u16 server_port;
  u16 local_port;
  f64 connection_time;
  f64 last_ping_time;
  f64 last_receive_time;
  f64 last_send_time;
  bool is_connected;
  bool is_reconnecting;
  u32 reconnect_attempts;
  u32 max_reconnect_attempts;
  f64 reconnect_delay;
} NetworkClient;

// ============================================================================
// NETWORK SERVER
// ============================================================================

// Connected client info
typedef struct {
  u32 client_id;
  void *socket;
  char *address;
  u16 port;
  NetworkStats stats;
  ConnectionState state;
  f64 connection_time;
  f64 last_ping_time;
  f64 last_receive_time;
  f64 last_send_time;
  u32 ping_sequence_number;
  bool is_authenticated;
  bool is_banned;
  u8 *encryption_key;
} ConnectedClient;

// Network server
typedef struct {
  void *listen_socket;
  u16 port;
  u32 max_clients;
  ConnectedClient *clients;
  u32 client_count;
  ServerAuthority authority;
  InterestManagement interest;
  NetworkCompression compression;
  NetworkReplication replication;
  u32 next_client_id;
  bool is_running;
  f64 start_time;
  u64 total_bytes_received;
  u64 total_bytes_sent;
  u32 total_packets_received;
  u32 total_packets_sent;
  f64 total_server_time;
} NetworkServer;

// ============================================================================
// MASSIVE NETWORKING SYSTEM
// ============================================================================

// Load balancer types
typedef enum {
  LOAD_BALANCER_ROUND_ROBIN = 0,
  LOAD_BALANCER_LEAST_CONNECTIONS,
  LOAD_BALANCER_WEIGHTED,
  LOAD_BALANCER_GEOGRAPHIC,
  LOAD_BALANCER_CUSTOM
} LoadBalancerType;

// Server node in distributed system
typedef struct {
  u32 node_id;
  char *address;
  u16 port;
  u32 current_clients;
  u32 max_clients;
  f32 cpu_usage;
  f32 memory_usage;
  f64 bandwidth_usage;
  bool is_active;
  f64 last_heartbeat;
  u32 region_id;
} ServerNode;

// Load balancer
typedef struct {
  LoadBalancerType type;
  ServerNode *nodes;
  u32 node_count;
  u32 max_nodes;
  u32 current_node_index;
  bool enable_geographic_routing;
  bool enable_health_checks;
  f64 health_check_interval;
  u32 failed_request_threshold;
} LoadBalancer;

// Main massive networking system
typedef struct {
  // Configuration
  NetworkConfig config;

  // Client or server mode
  bool is_server;
  NetworkClient *client;
  NetworkServer *server;

  // Distributed system
  LoadBalancer load_balancer;
  u32 region_id;
  bool enable_distributed;

  // Performance
  Profiler *network_profiler;
  f64 total_network_time;
  f64 compression_time;
  f64 replication_time;
  f64 prediction_time;
  f64 interest_time;

  // ECS integration
  World *ecs_world;

  // Threading
  void *network_threads;
  u32 network_thread_count;

  // Quality adaptation
  NetworkQuality current_quality;
  f64 last_quality_adaptation;
  bool enable_adaptive_quality;
} MassiveNetworkSystem;

// ============================================================================
// PUBLIC API
// ============================================================================

// Network system management
MassiveNetworkSystem *massive_network_create(const NetworkConfig *config,
                                             World *ecs_world);
void massive_network_destroy(MassiveNetworkSystem *system);
void massive_network_update(MassiveNetworkSystem *system, f32 delta_time);

// Configuration
NetworkConfig massive_network_create_default_config(void);
NetworkConfig massive_network_create_server_config(u32 max_clients);
NetworkConfig massive_network_create_client_config(const char *server_address);

// ============================================================================
// CLIENT API
// ============================================================================

// Client connection
bool massive_network_connect(MassiveNetworkSystem *system,
                             const char *server_address, u16 port);
void massive_network_disconnect(MassiveNetworkSystem *system);
bool massive_network_reconnect(MassiveNetworkSystem *system);

// Client input
void massive_network_send_input(MassiveNetworkSystem *system,
                                const InputCommand *input);
void massive_network_send_rpc(MassiveNetworkSystem *system,
                              const char *rpc_name, void *data, u32 data_size);

// Client prediction
void massive_network_enable_prediction(MassiveNetworkSystem *system,
                                       bool enable);
void massive_network_set_prediction_tolerance(MassiveNetworkSystem *system,
                                              f32 tolerance);
PredictionState *
massive_network_get_predicted_state(MassiveNetworkSystem *system,
                                    Entity entity);

// Client statistics
NetworkStats massive_network_get_stats(MassiveNetworkSystem *system);
NetworkQuality massive_network_get_quality(MassiveNetworkSystem *system);

// ============================================================================
// SERVER API
// ============================================================================

// Server management
bool massive_network_start_server(MassiveNetworkSystem *system, u16 port);
void massive_network_stop_server(MassiveNetworkSystem *system);
bool massive_network_is_running(MassiveNetworkSystem *system);

// Client management
u32 massive_network_get_client_count(MassiveNetworkSystem *system);
ConnectedClient *massive_network_get_client(MassiveNetworkSystem *system,
                                            u32 client_id);
bool massive_network_kick_client(MassiveNetworkSystem *system, u32 client_id,
                                 const char *reason);
bool massive_network_ban_client(MassiveNetworkSystem *system, u32 client_id,
                                const char *reason);

// Server broadcasting
void massive_network_broadcast_to_all(MassiveNetworkSystem *system,
                                      const void *data, u32 data_size);
void massive_network_broadcast_to_client(MassiveNetworkSystem *system,
                                         u32 client_id, const void *data,
                                         u32 data_size);
void massive_network_broadcast_to_region(MassiveNetworkSystem *system,
                                         Vec3 center, f32 radius,
                                         const void *data, u32 data_size);

// ============================================================================
// REPLICATION API
// ============================================================================

// Entity replication
bool massive_network_replicate_entity(MassiveNetworkSystem *system,
                                      Entity entity,
                                      ReplicationPriority priority);
bool massive_network_stop_replicating_entity(MassiveNetworkSystem *system,
                                             Entity entity);
bool massive_network_set_entity_owner(MassiveNetworkSystem *system,
                                      Entity entity, u32 client_id);

// Component replication
bool massive_network_replicate_component(MassiveNetworkSystem *system,
                                         Entity entity,
                                         ECSComponentID component_id,
                                         ReplicationPriority priority);
bool massive_network_set_component_dirty(MassiveNetworkSystem *system,
                                         Entity entity,
                                         ECSComponentID component_id);
bool massive_network_set_component_update_frequency(
    MassiveNetworkSystem *system, Entity entity, ECSComponentID component_id,
    f32 frequency);

// Replication configuration
void massive_network_set_replication_rate(MassiveNetworkSystem *system,
                                          f32 rate);
void massive_network_set_bandwidth_budget(MassiveNetworkSystem *system,
                                          f32 budget_mbps);

// ============================================================================
// INTEREST MANAGEMENT API
// ============================================================================

// Interest configuration
void massive_network_set_interest_method(MassiveNetworkSystem *system,
                                         InterestMethod method);
void massive_network_set_world_bounds(MassiveNetworkSystem *system,
                                      Vec3 min_bounds, Vec3 max_bounds);
void massive_network_set_grid_resolution(MassiveNetworkSystem *system,
                                         u32 resolution);

// Interest regions
InterestRegion *
massive_network_create_interest_region(MassiveNetworkSystem *system,
                                       Vec3 center, Vec3 extents);
void massive_network_destroy_interest_region(MassiveNetworkSystem *system,
                                             InterestRegion *region);
void massive_network_set_region_priority(MassiveNetworkSystem *system,
                                         InterestRegion *region, u32 priority);

// Client interests
void massive_network_set_client_interest(MassiveNetworkSystem *system,
                                         u32 client_id, InterestRegion *region);
void massive_network_clear_client_interests(MassiveNetworkSystem *system,
                                            u32 client_id);

// ============================================================================
// COMPRESSION API
// ============================================================================

// Compression configuration
void massive_network_set_compression_algorithm(MassiveNetworkSystem *system,
                                               CompressionAlgorithm algorithm);
void massive_network_set_compression_level(MassiveNetworkSystem *system,
                                           u32 level);
void massive_network_enable_adaptive_compression(MassiveNetworkSystem *system,
                                                 bool enable);

// Compression statistics
f32 massive_network_get_compression_ratio(MassiveNetworkSystem *system);
f64 massive_network_get_compression_time(MassiveNetworkSystem *system);

// ============================================================================
// DISTRIBUTED SYSTEMS API
// ============================================================================

// Load balancing
bool massive_network_enable_distributed(MassiveNetworkSystem *system,
                                        LoadBalancerType type);
ServerNode *massive_network_add_server_node(MassiveNetworkSystem *system,
                                            const char *address, u16 port,
                                            u32 max_clients);
void massive_network_remove_server_node(MassiveNetworkSystem *system,
                                        u32 node_id);
u32 massive_network_get_optimal_node(MassiveNetworkSystem *system,
                                     const char *client_address);

// Server migration
bool massive_network_migrate_client(MassiveNetworkSystem *system, u32 client_id,
                                    u32 target_node_id);
void massive_network_enable_server_migration(MassiveNetworkSystem *system,
                                             bool enable);

// Health monitoring
void massive_network_enable_health_checks(MassiveNetworkSystem *system,
                                          bool enable);
void massive_network_set_health_check_interval(MassiveNetworkSystem *system,
                                               f64 interval);

// ============================================================================
// QUALITY ADAPTATION API
// ============================================================================

// Quality adaptation
void massive_network_enable_quality_adaptation(MassiveNetworkSystem *system,
                                               bool enable);
void massive_network_set_quality_thresholds(MassiveNetworkSystem *system,
                                            f32 excellent_threshold,
                                            f32 good_threshold,
                                            f32 poor_threshold);
void massive_network_adapt_quality(MassiveNetworkSystem *system,
                                   NetworkQuality quality);

// Adaptive settings
void massive_network_adapt_tick_rate(MassiveNetworkSystem *system,
                                     u32 new_tick_rate);
void massive_network_adapt_send_rate(MassiveNetworkSystem *system,
                                     u32 new_send_rate);
void massive_network_adapt_compression(MassiveNetworkSystem *system,
                                       CompressionAlgorithm algorithm);

// ============================================================================
// DEBUGGING AND MONITORING
// ============================================================================

// Network debugging
void massive_network_debug_render_connections(MassiveNetworkSystem *system);
void massive_network_debug_render_interest_regions(
    MassiveNetworkSystem *system);
void massive_network_debug_render_replication(MassiveNetworkSystem *system);

// Performance monitoring
typedef struct {
  u32 total_clients;
  u32 active_connections;
  f64 average_ping;
  f64 total_bandwidth;
  f64 cpu_usage;
  f64 memory_usage;
  f64 packet_loss_rate;
  u32 packets_per_second;
  f64 compression_ratio;
  f64 prediction_accuracy;
} NetworkPerformanceReport;

NetworkPerformanceReport
massive_network_get_performance_report(MassiveNetworkSystem *system);
void massive_network_print_performance_report(MassiveNetworkSystem *system);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Network configuration macros
#define MASSIVE_NETWORK_SERVER_CONFIG(max_clients, port)                       \
  (NetworkConfig){.protocol = NETWORK_PROTOCOL_UDP,                            \
                  .max_clients = max_clients,                                  \
                  .tick_rate = 60,                                             \
                  .send_rate = 30,                                             \
                  .max_packet_size = 1200,                                     \
                  .client_timeout = 30.0f,                                     \
                  .server_timeout = 30.0f,                                     \
                  .enable_compression = true,                                  \
                  .enable_encryption = true,                                   \
                  .enable_prediction = true,                                   \
                  .enable_interpolation = true,                                \
                  .enable_reconciliation = true,                               \
                  .compression_level = 3,                                      \
                  .encryption_key_size = 256,                                  \
                  .server_address = NULL,                                      \
                  .server_port = port,                                         \
                  .client_port = 0}

#define MASSIVE_NETWORK_CLIENT_CONFIG(server_addr, port)                       \
  (NetworkConfig){.protocol = NETWORK_PROTOCOL_UDP,                            \
                  .max_clients = 1,                                            \
                  .tick_rate = 60,                                             \
                  .send_rate = 30,                                             \
                  .max_packet_size = 1200,                                     \
                  .client_timeout = 30.0f,                                     \
                  .server_timeout = 30.0f,                                     \
                  .enable_compression = true,                                  \
                  .enable_encryption = true,                                   \
                  .enable_prediction = true,                                   \
                  .enable_interpolation = true,                                \
                  .enable_reconciliation = true,                               \
                  .compression_level = 3,                                      \
                  .encryption_key_size = 256,                                  \
                  .server_address = server_addr,                               \
                  .server_port = port,                                         \
                  .client_port = 0}

// Input command creation macro
#define MASSIVE_INPUT_CREATE(seq, move, look, jump, crouch, sprint, actions)   \
  (InputCommand) {                                                             \
    .sequence_number = seq, .move_input = move, .look_input = look,            \
    .jump_pressed = jump, .crouch_pressed = crouch, .sprint_pressed = sprint,  \
    .action_flags = actions, .timestamp = massive_network_get_time()           \
  }

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Network simulation
void massive_network_enable_simulation(MassiveNetworkSystem *system,
                                       bool enable);
void massive_network_set_latency_simulation(MassiveNetworkSystem *system,
                                            f64 latency_ms, f64 jitter_ms);
void massive_network_set_packet_loss_simulation(MassiveNetworkSystem *system,
                                                f32 packet_loss_rate);

// Network recording and replay
void massive_network_start_recording(MassiveNetworkSystem *system,
                                     const char *filename);
void massive_network_stop_recording(MassiveNetworkSystem *system);
void massive_network_replay_recording(MassiveNetworkSystem *system,
                                      const char *filename);

// Network security
void massive_network_enable_ddos_protection(MassiveNetworkSystem *system,
                                            bool enable);
void massive_network_set_rate_limiting(MassiveNetworkSystem *system,
                                       u32 max_packets_per_second);
void massive_network_enable_connection_throttling(MassiveNetworkSystem *system,
                                                  bool enable);

// Network analytics
void massive_network_enable_analytics(MassiveNetworkSystem *system,
                                      bool enable);
void massive_network_track_network_event(MassiveNetworkSystem *system,
                                         const char *event_type,
                                         const char *data);

#endif // MASSIVE_NETWORKING_H
