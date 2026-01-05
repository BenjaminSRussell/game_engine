/**
 * =================================================================================================
 *                              NETWORK REPLICATION SYSTEM
 *                                   Agent: AGENT_MP_1
 * =================================================================================================
 *
 * Complete multiplayer state replication with prediction, interpolation, and
 * rollback.
 *
 * =================================================================================================
 */

#ifndef NETWORK_REPLICATION_H
#define NETWORK_REPLICATION_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    REPLICATION TYPES
 * =================================================================================================
 */

typedef enum ReplicationType {
  REPLICATION_NONE,          // Not replicated
  REPLICATION_INITIAL,       // Replicated on spawn only
  REPLICATION_OWNER,         // Replicated to owner only
  REPLICATION_SIMULATED,     // Replicated to all, simulated locally
  REPLICATION_AUTHORITATIVE, // Server/host authoritative
} ReplicationType;

typedef enum ReplicationReliability {
  RELIABILITY_UNRELIABLE,
  RELIABILITY_UNRELIABLE_SEQUENCED,
  RELIABILITY_RELIABLE,
  RELIABILITY_RELIABLE_ORDERED,
} ReplicationReliability;

/* =================================================================================================
 *                                    REPLICATED PROPERTY
 * =================================================================================================
 */

typedef struct ReplicatedProperty {
  char name[64];
  uint32_t offset;
  uint32_t size;
  ReplicationType type;
  ReplicationReliability reliability;
  float update_frequency;
  float last_update_time;
  bool is_dirty;
  uint32_t version;

  // Delta compression
  bool use_delta;
  void *previous_value;

  // Quantization
  bool quantize;
  float quantize_min;
  float quantize_max;
  uint32_t quantize_bits;

  // Condition
  bool (*should_replicate)(void *entity, uint32_t client_id);
} ReplicatedProperty;

// TODO(AGENT_MP_1): Implement property registration [Difficulty: 4]
// TODO(AGENT_MP_1): Implement property serialization [Difficulty: 5]
// TODO(AGENT_MP_1): Implement property deserialization [Difficulty: 5]
// TODO(AGENT_MP_1): Implement property delta compression [Difficulty: 6]
// TODO(AGENT_MP_1): Implement property quantization [Difficulty: 5]
// TODO(AGENT_MP_1): Implement property dirty tracking [Difficulty: 4]
// TODO(AGENT_MP_1): Implement property conditional replication [Difficulty: 5]
// TODO(AGENT_MP_1): Implement property interpolation [Difficulty: 6]

/* =================================================================================================
 *                                    REPLICATED ENTITY
 * =================================================================================================
 */

typedef struct ReplicatedEntity {
  uint32_t net_id;
  uint32_t owner_id;
  uint32_t archetype_id;

  ReplicatedProperty *properties;
  uint32_t property_count;

  // State
  bool is_spawned;
  bool is_dormant;
  float relevancy_distance;
  uint32_t priority;

  // Ownership
  bool has_authority;
  uint32_t controller_id;

  // Timestamps
  uint64_t spawn_time;
  uint64_t last_update;
  uint32_t last_acked_version;

  // Prediction
  void *predicted_state;
  void *server_state;
  bool is_predicting;
} ReplicatedEntity;

// TODO(AGENT_MP_1): Implement entity spawn RPC [Difficulty: 5]
// TODO(AGENT_MP_1): Implement entity despawn RPC [Difficulty: 4]
// TODO(AGENT_MP_1): Implement entity state snapshot [Difficulty: 5]
// TODO(AGENT_MP_1): Implement entity relevancy check [Difficulty: 5]
// TODO(AGENT_MP_1): Implement entity priority calculation [Difficulty: 5]
// TODO(AGENT_MP_1): Implement entity authority transfer [Difficulty: 6]
// TODO(AGENT_MP_1): Implement entity dormancy [Difficulty: 5]

/* =================================================================================================
 *                                    CLIENT PREDICTION
 * =================================================================================================
 */

typedef struct PredictedInput {
  uint32_t sequence_number;
  uint64_t timestamp;
  float input_data[16];
  uint32_t input_size;
  bool is_acked;
} PredictedInput;

typedef struct PredictionState {
  float position[3];
  float velocity[3];
  float rotation[4];
  uint32_t sequence_number;
  uint64_t timestamp;
} PredictionState;

typedef struct ClientPrediction {
  PredictedInput input_buffer[64];
  uint32_t input_head;
  uint32_t input_tail;

  PredictionState state_buffer[64];
  uint32_t state_head;

  uint32_t last_acked_input;
  uint32_t current_sequence;

  float reconciliation_threshold;
  bool needs_reconciliation;
} ClientPrediction;

// TODO(AGENT_MP_1): Implement prediction buffer add input [Difficulty: 4]
// TODO(AGENT_MP_1): Implement prediction buffer ack [Difficulty: 4]
// TODO(AGENT_MP_1): Implement prediction apply input [Difficulty: 5]
// TODO(AGENT_MP_1): Implement prediction compare states [Difficulty: 5]
// TODO(AGENT_MP_1): Implement prediction reconciliation [Difficulty: 7]
// TODO(AGENT_MP_1): Implement prediction rollback [Difficulty: 7]
// TODO(AGENT_MP_1): Implement prediction replay inputs [Difficulty: 6]
// TODO(AGENT_MP_1): Implement prediction smoothing [Difficulty: 6]

/* =================================================================================================
 *                                    INTERPOLATION
 * =================================================================================================
 */

typedef struct InterpolationSnapshot {
  PredictionState state;
  uint64_t timestamp;
} InterpolationSnapshot;

typedef struct EntityInterpolation {
  InterpolationSnapshot buffer[32];
  uint32_t buffer_head;
  uint32_t buffer_count;

  float interpolation_delay;
  float extrapolation_limit;

  PredictionState current_interpolated;
  bool is_extrapolating;
} EntityInterpolation;

// TODO(AGENT_MP_1): Implement interpolation add snapshot [Difficulty: 4]
// TODO(AGENT_MP_1): Implement interpolation find snapshots [Difficulty: 5]
// TODO(AGENT_MP_1): Implement interpolation lerp [Difficulty: 5]
// TODO(AGENT_MP_1): Implement interpolation slerp rotation [Difficulty: 5]
// TODO(AGENT_MP_1): Implement interpolation extrapolation [Difficulty: 6]
// TODO(AGENT_MP_1): Implement interpolation hermite [Difficulty: 6]
// TODO(AGENT_MP_1): Implement interpolation time calculation [Difficulty: 5]

/* =================================================================================================
 *                                    RPC SYSTEM
 * =================================================================================================
 */

typedef enum RPCTarget {
  RPC_TARGET_SERVER,
  RPC_TARGET_CLIENT,
  RPC_TARGET_ALL_CLIENTS,
  RPC_TARGET_ALL_EXCEPT_OWNER,
  RPC_TARGET_OWNER_ONLY,
} RPCTarget;

typedef struct RPCDefinition {
  uint32_t id;
  char name[64];
  RPCTarget target;
  ReplicationReliability reliability;
  void (*handler)(void *params, uint32_t sender);
  uint32_t param_size;
  bool validate_sender;
} RPCDefinition;

typedef struct RPCCall {
  uint32_t rpc_id;
  uint32_t entity_id;
  uint32_t sender_id;
  void *params;
  uint32_t param_size;
  uint64_t timestamp;
} RPCCall;

// TODO(AGENT_MP_1): Implement RPC registration [Difficulty: 4]
// TODO(AGENT_MP_1): Implement RPC serialization [Difficulty: 5]
// TODO(AGENT_MP_1): Implement RPC deserialization [Difficulty: 5]
// TODO(AGENT_MP_1): Implement RPC dispatch [Difficulty: 5]
// TODO(AGENT_MP_1): Implement RPC validation [Difficulty: 5]
// TODO(AGENT_MP_1): Implement RPC batching [Difficulty: 5]
// TODO(AGENT_MP_1): Implement RPC rate limiting [Difficulty: 5]
// TODO(AGENT_MP_1): Implement server RPC [Difficulty: 5]
// TODO(AGENT_MP_1): Implement client RPC [Difficulty: 5]
// TODO(AGENT_MP_1): Implement multicast RPC [Difficulty: 5]

/* =================================================================================================
 *                                    REPLICATION MANAGER
 * =================================================================================================
 */

typedef struct ReplicationManager {
  ReplicatedEntity *entities;
  uint32_t entity_count;
  uint32_t entity_capacity;

  RPCDefinition *rpcs;
  uint32_t rpc_count;

  // Client state
  ClientPrediction *predictions;
  EntityInterpolation *interpolations;

  // Bandwidth management
  uint32_t bytes_per_second_limit;
  uint32_t current_bytes_used;
  float bandwidth_usage;

  // Statistics
  uint32_t packets_sent;
  uint32_t packets_received;
  uint32_t rpcs_sent;
  uint32_t rpcs_received;
  float average_rtt;
  float jitter;
} ReplicationManager;

// TODO(AGENT_MP_1): Implement replication_manager_init [Difficulty: 5]
// TODO(AGENT_MP_1): Implement replication_manager_shutdown [Difficulty: 4]
// TODO(AGENT_MP_1): Implement replication_manager_update [Difficulty: 6]
// TODO(AGENT_MP_1): Implement replication_spawn_entity [Difficulty: 5]
// TODO(AGENT_MP_1): Implement replication_despawn_entity [Difficulty: 4]
// TODO(AGENT_MP_1): Implement replication_update_entity [Difficulty: 6]
// TODO(AGENT_MP_1): Implement replication_send_rpc [Difficulty: 5]
// TODO(AGENT_MP_1): Implement replication_receive_rpc [Difficulty: 5]
// TODO(AGENT_MP_1): Implement replication_serialize_frame [Difficulty: 6]
// TODO(AGENT_MP_1): Implement replication_deserialize_frame [Difficulty: 6]
// TODO(AGENT_MP_1): Implement replication_prioritize [Difficulty: 6]
// TODO(AGENT_MP_1): Implement replication_bandwidth_limit [Difficulty: 5]
// TODO(AGENT_MP_1): Implement replication_relevancy_cull [Difficulty: 5]
// TODO(AGENT_MP_1): Implement replication_statistics [Difficulty: 4]

#endif // NETWORK_REPLICATION_H
