/**
 * @file physics_event_system.h
 * @brief Physics event system
 *
 * Provides comprehensive event handling for physics systems including
 * collision events, constraint events, object lifecycle events, and
 * custom user events with priority queuing and filtering.
 */

#ifndef PHYSICS_EVENT_SYSTEM_H
#define PHYSICS_EVENT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct PhysicsWorld PhysicsWorld;
typedef struct PhysicsObject PhysicsObject;
typedef struct PhysicsConstraint PhysicsConstraint;

// ========================================
// Event Types
// ========================================

typedef enum PhysicsEventType {
    // Collision events
    PHYSICS_EVENT_COLLISION_START,    // Collision began
    PHYSICS_EVENT_COLLISION_END,      // Collision ended
    PHYSICS_EVENT_COLLISION_PERSIST,  // Collision persists
    PHYSICS_EVENT_CONTACT_ADDED,      // New contact point
    PHYSICS_EVENT_CONTACT_REMOVED,    // Contact point removed
    PHYSICS_EVENT_CONTACT_PERSIST,    // Contact point persists
    
    // Constraint events
    PHYSICS_EVENT_CONSTRAINT_BROKEN,  // Constraint broke
    PHYSICS_EVENT_CONSTRAINT_LIMIT_REACHED, // Constraint limit reached
    PHYSICS_EVENT_CONSTRAINT_ACTIVATED, // Constraint activated
    PHYSICS_EVENT_CONSTRAINT_DEACTIVATED, // Constraint deactivated
    
    // Object lifecycle events
    PHYSICS_EVENT_OBJECT_CREATED,     // Object created
    PHYSICS_EVENT_OBJECT_DESTROYED,   // Object destroyed
    PHYSICS_EVENT_OBJECT_ACTIVATED,   // Object activated
    PHYSICS_EVENT_OBJECT_DEACTIVATED, // Object deactivated
    PHYSICS_EVENT_OBJECT_SLEEPING,     // Object went to sleep
    PHYSICS_EVENT_OBJECT_AWAKENED,     // Object woke up
    
    // Force and impulse events
    PHYSICS_EVENT_FORCE_APPLIED,      // Force applied to object
    PHYSICS_EVENT_IMPULSE_APPLIED,    // Impulse applied to object
    PHYSICS_EVENT_TORQUE_APPLIED,     // Torque applied to object
    
    // Material events
    PHYSICS_EVENT_MATERIAL_CHANGED,   // Material properties changed
    PHYSICS_EVENT_FRACTURE_OCCURRED,  // Material fracture occurred
    PHYSICS_EVENT_DEFORMATION_STARTED, // Deformation started
    PHYSICS_EVENT_DEFORMATION_ENDED,   // Deformation ended
    
    // Simulation events
    PHYSICS_EVENT_SIMULATION_STARTED, // Simulation started
    PHYSICS_EVENT_SIMULATION_PAUSED,  // Simulation paused
    PHYSICS_EVENT_SIMULATION_RESUMED, // Simulation resumed
    PHYSICS_EVENT_SIMULATION_STEP_COMPLETED, // Simulation step completed
    PHYSICS_EVENT_SIMULATION_RESET,    // Simulation reset
    
    // Performance events
    PHYSICS_EVENT_PERFORMANCE_WARNING, // Performance warning
    PHYSICS_EVENT_MEMORY_LIMIT_EXCEEDED, // Memory limit exceeded
    PHYSICS_EVENT_TIME_STEP_ADJUSTED, // Time step adjusted
    PHYSICS_EVENT_OPTIMIZATION_TRIGGERED, // Optimization triggered
    
    // User-defined events
    PHYSICS_EVENT_USER_DEFINED = 1000, // Start of user-defined events
    PHYSICS_EVENT_CUSTOM = 2000         // Custom events
} PhysicsEventType;

// ========================================
// Event Priority
// ========================================

typedef enum EventPriority {
    PRIORITY_CRITICAL = 0,            // Critical events (immediate processing)
    PRIORITY_HIGH = 1,                // High priority events
    PRIORITY_NORMAL = 2,              // Normal priority events
    PRIORITY_LOW = 3,                 // Low priority events
    PRIORITY_BACKGROUND = 4           // Background events
} EventPriority;

// ========================================
// Event Flags
// ========================================

typedef enum EventFlags {
    EVENT_FLAG_NONE = 0x00,
    EVENT_FLAG_IMMEDIATE = 0x01,      // Process immediately
    EVENT_FLAG_QUEUED = 0x02,         // Queue for later processing
    EVENT_FLAG_BROADCAST = 0x04,       // Broadcast to all listeners
    EVENT_FLAG_TARGETED = 0x08,       // Target specific listener
    EVENT_FLAG_PERSISTENT = 0x10,     // Persistent event
    EVENT_FLAG_ONE_SHOT = 0x20,       // One-time event
    EVENT_FLAG_FILTERED = 0x40,       // Event is filtered
    EVENT_FLAG_LOGGED = 0x80          // Event is logged
} EventFlags;

// ========================================
// Event Data Structures
// ========================================

typedef struct CollisionEvent {
    uint64_t object_a_id;             // First object ID
    uint64_t object_b_id;             // Second object ID
    float contact_point[3];           // Contact point
    float contact_normal[3];          // Contact normal
    float penetration_depth;          // Penetration depth
    float relative_velocity[3];       // Relative velocity
    float impulse_magnitude;          // Impulse magnitude
    uint32_t contact_count;           // Number of contacts
} CollisionEvent;

typedef struct ConstraintEvent {
    uint64_t constraint_id;           // Constraint ID
    uint64_t body_a_id;               // First body ID
    uint64_t body_b_id;               // Second body ID
    float break_force;                // Break force
    float current_force;              // Current force
    float limit_value;                // Limit value
    bool is_broken;                   // Is broken
    uint32_t constraint_type;         // Constraint type
} ConstraintEvent;

typedef struct ObjectLifecycleEvent {
    uint64_t object_id;               // Object ID
    uint32_t object_type;             // Object type
    uint32_t body_type;               // Body type
    float position[3];                // Object position
    float velocity[3];                // Object velocity
    float mass;                       // Object mass
    bool is_active;                   // Is active
    bool is_sleeping;                 // Is sleeping
} ObjectLifecycleEvent;

typedef struct ForceEvent {
    uint64_t object_id;               // Object ID
    float force[3];                   // Applied force
    float application_point[3];        // Application point
    float duration;                   // Force duration
    bool is_impulse;                  // Is impulse
} ForceEvent;

typedef struct MaterialEvent {
    uint64_t object_id;               // Object ID
    uint32_t material_id;             // Material ID
    float stress[3];                  // Stress tensor
    float strain[3];                  // Strain tensor
    float damage;                     // Damage amount
    bool is_fractured;                // Is fractured
    float fracture_threshold;         // Fracture threshold
} MaterialEvent;

typedef struct SimulationEvent {
    float time_step;                 // Time step
    float simulation_time;            // Simulation time
    int step_count;                  // Step count
    int active_objects;              // Active objects
    int sleeping_objects;            // Sleeping objects
    float performance_score;         // Performance score
    bool is_paused;                  // Is paused
} SimulationEvent;

typedef struct PerformanceEvent {
    float frame_time_ms;             // Frame time
    float update_time_ms;            // Update time
    float collision_time_ms;         // Collision time
    float solver_time_ms;            // Solver time
    size_t memory_usage;             // Memory usage
    int object_count;                // Object count
    int contact_count;               // Contact count
    char warning_message[256];       // Warning message
} PerformanceEvent;

// ========================================
// Physics Event Structure
// ========================================

typedef struct PhysicsEvent {
    uint64_t event_id;               // Unique event ID
    PhysicsEventType type;            // Event type
    EventPriority priority;           // Event priority
    uint32_t flags;                  // Event flags
    uint64_t timestamp;              // Event timestamp
    uint64_t source_id;              // Source object/system ID
    uint64_t target_id;              // Target object/system ID (0 for broadcast)
    
    // Event data union
    union {
        CollisionEvent collision;
        ConstraintEvent constraint;
        ObjectLifecycleEvent lifecycle;
        ForceEvent force;
        MaterialEvent material;
        SimulationEvent simulation;
        PerformanceEvent performance;
        struct {
            void *data;              // Custom event data
            size_t data_size;        // Data size
        } custom;
    } data;
    
    // Event metadata
    char description[128];           // Event description
    bool processed;                  // Event has been processed
    uint32_t listener_mask;          // Listener mask
    void *user_data;                 // User data pointer
    
} PhysicsEvent;

// ========================================
// Event Listener
// ========================================

typedef struct EventListener {
    uint64_t listener_id;             // Unique listener ID
    char name[64];                   // Listener name
    uint32_t event_mask;             // Event type mask
    uint32_t priority_mask;          // Priority mask
    void (*callback)(const PhysicsEvent *event, void *user_data); // Callback function
    void *user_data;                 // User data pointer
    bool enabled;                    // Listener is enabled
    bool one_shot;                   // One-time listener
    uint32_t event_count;            // Events received
    uint64_t last_event_time;        // Last event timestamp
    float filter_threshold;          // Filter threshold
    bool filter_enabled;             // Filter enabled
} EventListener;

// ========================================
// Event Filter
// ========================================

typedef struct EventFilter {
    uint64_t filter_id;               // Unique filter ID
    char name[64];                   // Filter name
    uint32_t event_type_mask;         // Event type mask
    uint64_t source_mask;            // Source object mask
    uint64_t target_mask;            // Target object mask
    uint32_t priority_mask;          // Priority mask
    float time_window;               // Time window for filtering
    float frequency_limit;           // Frequency limit (events per second)
    uint64_t last_filter_time;       // Last filter time
    uint32_t events_filtered;        // Events filtered count
    bool enabled;                    // Filter is enabled
    bool (*custom_filter)(const PhysicsEvent *event, void *user_data); // Custom filter function
    void *user_data;                 // User data pointer
} EventFilter;

// ========================================
// Event Queue
// ========================================

typedef struct EventQueue {
    PhysicsEvent *events;             // Event array
    int capacity;                    // Queue capacity
    int size;                        // Current size
    int head;                        // Head index
    int tail;                        // Tail index
    bool priority_queue;              // Priority queue mode
    uint64_t total_events;           // Total events processed
    uint64_t dropped_events;         // Dropped events count
    float average_processing_time;    // Average processing time
} EventQueue;

// ========================================
// Event System
// ========================================

typedef struct PhysicsEventSystem {
    // Event queues
    EventQueue immediate_queue;        // Immediate event queue
    EventQueue normal_queue;          // Normal event queue
    EventQueue background_queue;      // Background event queue
    
    // Listeners
    EventListener *listeners;         // Listener array
    int listener_count;               // Number of listeners
    int listener_capacity;            // Listener capacity
    
    // Filters
    EventFilter *filters;             // Filter array
    int filter_count;                // Number of filters
    int filter_capacity;             // Filter capacity
    
    // Event pool
    PhysicsEvent *event_pool;         // Event pool
    int pool_size;                    // Pool size
    int pool_used;                    // Pool used count
    uint64_t next_event_id;          // Next event ID
    
    // Statistics
    uint64_t events_generated;        // Total events generated
    uint64_t events_processed;        // Total events processed
    uint64_t events_dropped;          // Total events dropped
    uint64_t events_filtered;         // Total events filtered
    float total_processing_time;      // Total processing time
    uint64_t system_start_time;       // System start time
    
    // Configuration
    bool enabled;                     // System enabled
    bool logging_enabled;             // Logging enabled
    bool statistics_enabled;          // Statistics enabled
    int max_events_per_frame;         // Maximum events per frame
    float max_processing_time;        // Maximum processing time per frame
    bool auto_cleanup;                // Auto cleanup old events
    
    // User data
    void *user_data;                 // User data pointer
    
} PhysicsEventSystem;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create physics event system
 * @param immediate_queue_size Immediate queue size
 * @param normal_queue_size Normal queue size
 * @param background_queue_size Background queue size
 * @return Event system or NULL on failure
 */
PhysicsEventSystem* physics_event_system_create(int immediate_queue_size, 
                                                int normal_queue_size, 
                                                int background_queue_size);

/**
 * Destroy physics event system
 * @param system Event system
 */
void physics_event_system_destroy(PhysicsEventSystem *system);

// ========================================
// System Control
// ========================================

/**
 * Enable/disable event system
 * @param system Event system
 * @param enabled Enable system
 */
void physics_event_system_set_enabled(PhysicsEventSystem *system, bool enabled);

/**
 * Check if event system is enabled
 * @param system Event system
 * @return True if enabled
 */
bool physics_event_system_is_enabled(PhysicsEventSystem *system);

/**
 * Process all pending events
 * @param system Event system
 * @param max_time Maximum processing time
 * @return Number of events processed
 */
int physics_event_system_process_events(PhysicsEventSystem *system, float max_time);

/**
 * Process events of specific priority
 * @param system Event system
 * @param priority Event priority
 * @param max_events Maximum events to process
 * @return Number of events processed
 */
int physics_event_system_process_priority(PhysicsEventSystem *system, 
                                          EventPriority priority, int max_events);

/**
 * Clear all event queues
 * @param system Event system
 */
void physics_event_system_clear_queues(PhysicsEventSystem *system);

// ========================================
// Event Generation
// ========================================

/**
 * Generate collision event
 * @param system Event system
 * @param object_a_id First object ID
 * @param object_b_id Second object ID
 * @param contact_point Contact point
 * @param contact_normal Contact normal
 * @param penetration Penetration depth
 * @param relative_velocity Relative velocity
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_collision(PhysicsEventSystem *system,
                                           uint64_t object_a_id, uint64_t object_b_id,
                                           const float *contact_point, const float *contact_normal,
                                           float penetration, const float *relative_velocity);

/**
 * Generate constraint event
 * @param system Event system
 * @param constraint_id Constraint ID
 * @param body_a_id First body ID
 * @param body_b_id Second body ID
 * @param break_force Break force
 * @param current_force Current force
 * @param is_broken Is broken
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_constraint(PhysicsEventSystem *system,
                                           uint64_t constraint_id, uint64_t body_a_id, uint64_t body_b_id,
                                           float break_force, float current_force, bool is_broken);

/**
 * Generate object lifecycle event
 * @param system Event system
 * @param event_type Event type
 * @param object_id Object ID
 * @param object_type Object type
 * @param body_type Body type
 * @param position Object position
 * @param velocity Object velocity
 * @param mass Object mass
 * @param is_active Is active
 * @param is_sleeping Is sleeping
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_lifecycle(PhysicsEventSystem *system,
                                         PhysicsEventType event_type, uint64_t object_id,
                                         uint32_t object_type, uint32_t body_type,
                                         const float *position, const float *velocity,
                                         float mass, bool is_active, bool is_sleeping);

/**
 * Generate force event
 * @param system Event system
 * @param object_id Object ID
 * @param force Applied force
 * @param application_point Application point
 * @param duration Force duration
 * @param is_impulse Is impulse
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_force(PhysicsEventSystem *system,
                                      uint64_t object_id, const float *force,
                                      const float *application_point, 
                                      float duration, bool is_impulse);

/**
 * Generate material event
 * @param system Event system
 * @param event_type Event type
 * @param object_id Object ID
 * @param material_id Material ID
 * @param stress Stress tensor
 * @param strain Strain tensor
 * @param damage Damage amount
 * @param is_fractured Is fractured
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_material(PhysicsEventSystem *system,
                                         PhysicsEventType event_type, uint64_t object_id,
                                         uint32_t material_id, const float *stress,
                                         const float *strain, float damage, bool is_fractured);

/**
 * Generate simulation event
 * @param system Event system
 * @param event_type Event type
 * @param time_step Time step
 * @param simulation_time Simulation time
 * @param step_count Step count
 * @param active_objects Active objects
 * @param sleeping_objects Sleeping objects
 * @param performance_score Performance score
 * @param is_paused Is paused
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_simulation(PhysicsEventSystem *system,
                                           PhysicsEventType event_type, float time_step,
                                           float simulation_time, int step_count,
                                           int active_objects, int sleeping_objects,
                                           float performance_score, bool is_paused);

/**
 * Generate performance event
 * @param system Event system
 * @param event_type Event type
 * @param frame_time Frame time
 * @param update_time Update time
 * @param collision_time Collision time
 * @param solver_time Solver time
 * @param memory_usage Memory usage
 * @param object_count Object count
 * @param contact_count Contact count
 * @param warning_message Warning message
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_performance(PhysicsEventSystem *system,
                                            PhysicsEventType event_type, float frame_time,
                                            float update_time, float collision_time,
                                            float solver_time, size_t memory_usage,
                                            int object_count, int contact_count,
                                            const char *warning_message);

/**
 * Generate custom event
 * @param system Event system
 * @param event_type Event type
 * @param data Custom data
 * @param data_size Data size
 * @param description Event description
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_custom(PhysicsEventSystem *system,
                                       PhysicsEventType event_type, const void *data,
                                       size_t data_size, const char *description);

// ========================================
// Listener Management
// ========================================

/**
 * Register event listener
 * @param system Event system
 * @param name Listener name
 * @param event_mask Event type mask
 * @param callback Callback function
 * @param user_data User data
 * @return Listener ID or 0 on failure
 */
uint64_t physics_event_register_listener(PhysicsEventSystem *system,
                                        const char *name, uint32_t event_mask,
                                        void (*callback)(const PhysicsEvent*, void*),
                                        void *user_data);

/**
 * Unregister event listener
 * @param system Event system
 * @param listener_id Listener ID
 * @return True if successful
 */
bool physics_event_unregister_listener(PhysicsEventSystem *system, uint64_t listener_id);

/**
 * Enable/disable listener
 * @param system Event system
 * @param listener_id Listener ID
 * @param enabled Enable listener
 * @return True if successful
 */
bool physics_event_set_listener_enabled(PhysicsEventSystem *system, uint64_t listener_id, bool enabled);

/**
 * Set listener priority mask
 * @param system Event system
 * @param listener_id Listener ID
 * @param priority_mask Priority mask
 * @return True if successful
 */
bool physics_event_set_listener_priority_mask(PhysicsEventSystem *system, 
                                             uint64_t listener_id, uint32_t priority_mask);

/**
 * Get listener statistics
 * @param system Event system
 * @param listener_id Listener ID
 * @param event_count Events received
 * @param last_event_time Last event time
 * @return True if successful
 */
bool physics_event_get_listener_stats(PhysicsEventSystem *system, uint64_t listener_id,
                                      uint32_t *event_count, uint64_t *last_event_time);

// ========================================
// Filter Management
// ========================================

/**
 * Create event filter
 * @param system Event system
 * @param name Filter name
 * @param event_type_mask Event type mask
 * @return Filter ID or 0 on failure
 */
uint64_t physics_event_create_filter(PhysicsEventSystem *system, const char *name, 
                                     uint32_t event_type_mask);

/**
 * Destroy event filter
 * @param system Event system
 * @param filter_id Filter ID
 * @return True if successful
 */
bool physics_event_destroy_filter(PhysicsEventSystem *system, uint64_t filter_id);

/**
 * Set filter parameters
 * @param system Event system
 * @param filter_id Filter ID
 * @param source_mask Source mask
 * @param target_mask Target mask
 * @param priority_mask Priority mask
 * @param time_window Time window
 * @param frequency_limit Frequency limit
 * @return True if successful
 */
bool physics_event_set_filter_params(PhysicsEventSystem *system, uint64_t filter_id,
                                     uint64_t source_mask, uint64_t target_mask,
                                     uint32_t priority_mask, float time_window,
                                     float frequency_limit);

/**
 * Enable/disable filter
 * @param system Event system
 * @param filter_id Filter ID
 * @param enabled Enable filter
 * @return True if successful
 */
bool physics_event_set_filter_enabled(PhysicsEventSystem *system, uint64_t filter_id, bool enabled);

/**
 * Set custom filter function
 * @param system Event system
 * @param filter_id Filter ID
 * @param filter_func Custom filter function
 * @param user_data User data
 * @return True if successful
 */
bool physics_event_set_custom_filter(PhysicsEventSystem *system, uint64_t filter_id,
                                     bool (*filter_func)(const PhysicsEvent*, void*),
                                     void *user_data);

// ========================================
// Statistics and Monitoring
// ========================================

/**
 * Get system statistics
 * @param system Event system
 * @param events_generated Events generated
 * @param events_processed Events processed
 * @param events_dropped Events dropped
 * @param events_filtered Events filtered
 * @param total_processing_time Total processing time
 * @param average_processing_time Average processing time
 */
void physics_event_get_statistics(PhysicsEventSystem *system,
                                   uint64_t *events_generated, uint64_t *events_processed,
                                   uint64_t *events_dropped, uint64_t *events_filtered,
                                   float *total_processing_time, float *average_processing_time);

/**
 * Get queue statistics
 * @param system Event system
 * @param immediate_size Immediate queue size
 * @param normal_size Normal queue size
 * @param background_size Background queue size
 * @param total_queued Total queued events
 */
void physics_event_get_queue_statistics(PhysicsEventSystem *system,
                                        int *immediate_size, int *normal_size,
                                        int *background_size, int *total_queued);

/**
 * Get listener statistics
 * @param system Event system
 * @param total_listeners Total listeners
 * @param active_listeners Active listeners
 * @param total_events_received Total events received
 */
void physics_event_get_listener_statistics(PhysicsEventSystem *system,
                                           int *total_listeners, int *active_listeners,
                                           uint64_t *total_events_received);

/**
 * Reset statistics
 * @param system Event system
 */
void physics_event_reset_statistics(PhysicsEventSystem *system);

// ========================================
// Configuration
// ========================================

/**
 * Set maximum events per frame
 * @param system Event system
 * @param max_events Maximum events
 */
void physics_event_set_max_events_per_frame(PhysicsEventSystem *system, int max_events);

/**
 * Set maximum processing time
 * @param system Event system
 * @param max_time Maximum time in seconds
 */
void physics_event_set_max_processing_time(PhysicsEventSystem *system, float max_time);

/**
 * Enable/disable logging
 * @param system Event system
 * @param enabled Enable logging
 */
void physics_event_set_logging_enabled(PhysicsEventSystem *system, bool enabled);

/**
 * Enable/disable statistics
 * @param system Event system
 * @param enabled Enable statistics
 */
void physics_event_set_statistics_enabled(PhysicsEventSystem *system, bool enabled);

/**
 * Enable/disable auto cleanup
 * @param system Event system
 * @param enabled Enable auto cleanup
 */
void physics_event_set_auto_cleanup(PhysicsEventSystem *system, bool enabled);

// ========================================
// Utility Functions
// ========================================

/**
 * Get event type name
 * @param event_type Event type
 * @return Event type name
 */
const char* physics_event_get_type_name(PhysicsEventType event_type);

/**
 * Get priority name
 * @param priority Event priority
 * @return Priority name
 */
const char* physics_event_get_priority_name(EventPriority priority);

/**
 * Validate event system
 * @param system Event system
 * @return True if valid
 */
bool physics_event_validate(PhysicsEventSystem *system);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_EVENT_SYSTEM_H */
