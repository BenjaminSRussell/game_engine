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
// Event Types
// ========================================

typedef enum PhysicsEventType {
    PHYSICS_EVENT_COLLISION_START,    // Collision began
    PHYSICS_EVENT_COLLISION_END,      // Collision ended
    PHYSICS_EVENT_COLLISION_PERSIST,  // Collision persists
    PHYSICS_EVENT_CONTACT_ADDED,      // New contact point
    PHYSICS_EVENT_CONTACT_REMOVED,    // Contact point removed
    PHYSICS_EVENT_CONSTRAINT_BROKEN,  // Constraint broke
    PHYSICS_EVENT_OBJECT_CREATED,     // Object created
    PHYSICS_EVENT_OBJECT_DESTROYED,   // Object destroyed
    PHYSICS_EVENT_OBJECT_SLEEPING,     // Object went to sleep
    PHYSICS_EVENT_OBJECT_AWAKENED,     // Object woke up
    PHYSICS_EVENT_FORCE_APPLIED,      // Force applied to object
    PHYSICS_EVENT_SIMULATION_STEP_COMPLETED, // Simulation step completed
    PHYSICS_EVENT_PERFORMANCE_WARNING, // Performance warning
    PHYSICS_EVENT_USER_DEFINED = 1000, // Start of user-defined events
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
} CollisionEvent;

typedef struct ConstraintEvent {
    uint64_t constraint_id;           // Constraint ID
    uint64_t body_a_id;               // First body ID
    uint64_t body_b_id;               // Second body ID
    float break_force;                // Break force
    float current_force;              // Current force
    bool is_broken;                   // Is broken
} ConstraintEvent;

typedef struct ObjectLifecycleEvent {
    uint64_t object_id;               // Object ID
    uint32_t object_type;             // Object type
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

typedef struct SimulationEvent {
    float time_step;                 // Time step
    float simulation_time;            // Simulation time
    int step_count;                  // Step count
    int active_objects;              // Active objects
    int sleeping_objects;            // Sleeping objects
    float performance_score;         // Performance score
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
    uint64_t timestamp;              // Event timestamp
    uint64_t source_id;              // Source object/system ID
    uint64_t target_id;              // Target object/system ID (0 for broadcast)
    
    // Event data union
    union {
        CollisionEvent collision;
        ConstraintEvent constraint;
        ObjectLifecycleEvent lifecycle;
        ForceEvent force;
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
    void *user_data;                 // User data pointer
} PhysicsEvent;

// ========================================
// Event Listener
// ========================================

typedef struct EventListener {
    uint64_t listener_id;             // Unique listener ID
    char name[64];                   // Listener name
    uint32_t event_mask;             // Event type mask
    void (*callback)(const PhysicsEvent *event, void *user_data); // Callback function
    void *user_data;                 // User data pointer
    bool enabled;                    // Listener is enabled
    uint32_t event_count;            // Events received
} EventListener;

// ========================================
// Event System
// ========================================

typedef struct PhysicsEventSystem {
    // Event queues
    PhysicsEvent *immediate_queue;    // Immediate event queue
    PhysicsEvent *normal_queue;      // Normal event queue
    PhysicsEvent *background_queue;  // Background event queue
    
    int immediate_capacity;          // Immediate queue capacity
    int normal_capacity;            // Normal queue capacity
    int background_capacity;        // Background queue capacity
    
    int immediate_size;              // Immediate queue size
    int normal_size;                // Normal queue size
    int background_size;            // Background queue size
    
    // Listeners
    EventListener *listeners;         // Listener array
    int listener_count;               // Number of listeners
    int listener_capacity;            // Listener capacity
    
    // Event pool
    PhysicsEvent *event_pool;         // Event pool
    int pool_size;                    // Pool size
    int pool_used;                    // Pool used count
    uint64_t next_event_id;          // Next event ID
    
    // Statistics
    uint64_t events_generated;        // Total events generated
    uint64_t events_processed;        // Total events processed
    uint64_t events_dropped;          // Total events dropped
    float total_processing_time;      // Total processing time
    
    // Configuration
    bool enabled;                     // System enabled
    bool logging_enabled;             // Logging enabled
    int max_events_per_frame;         // Maximum events per frame
    float max_processing_time;        // Maximum processing time per frame
    
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
 * Process all pending events
 * @param system Event system
 * @param max_time Maximum processing time
 * @return Number of events processed
 */
int physics_event_system_process_events(PhysicsEventSystem *system, float max_time);

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
 * @param position Object position
 * @param velocity Object velocity
 * @param mass Object mass
 * @param is_active Is active
 * @param is_sleeping Is sleeping
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_lifecycle(PhysicsEventSystem *system,
                                         PhysicsEventType event_type, uint64_t object_id,
                                         uint32_t object_type, const float *position,
                                         const float *velocity, float mass, 
                                         bool is_active, bool is_sleeping);

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
 * Generate simulation event
 * @param system Event system
 * @param event_type Event type
 * @param time_step Time step
 * @param simulation_time Simulation time
 * @param step_count Step count
 * @param active_objects Active objects
 * @param sleeping_objects Sleeping objects
 * @param performance_score Performance score
 * @return Event ID or 0 on failure
 */
uint64_t physics_event_generate_simulation(PhysicsEventSystem *system,
                                           PhysicsEventType event_type, float time_step,
                                           float simulation_time, int step_count,
                                           int active_objects, int sleeping_objects,
                                           float performance_score);

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

// ========================================
// Statistics
// ========================================

/**
 * Get system statistics
 * @param system Event system
 * @param events_generated Events generated
 * @param events_processed Events processed
 * @param events_dropped Events dropped
 * @param total_processing_time Total processing time
 */
void physics_event_get_statistics(PhysicsEventSystem *system,
                                   uint64_t *events_generated, uint64_t *events_processed,
                                   uint64_t *events_dropped, float *total_processing_time);

/**
 * Get queue statistics
 * @param system Event system
 * @param immediate_size Immediate queue size
 * @param normal_size Normal queue size
 * @param background_size Background queue size
 */
void physics_event_get_queue_statistics(PhysicsEventSystem *system,
                                        int *immediate_size, int *normal_size,
                                        int *background_size);

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
 * Enable/disable logging
 * @param system Event system
 * @param enabled Enable logging
 */
void physics_event_set_logging_enabled(PhysicsEventSystem *system, bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_EVENT_SYSTEM_H */
