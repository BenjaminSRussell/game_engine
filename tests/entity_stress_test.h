/**
 * @file entity_stress_test.h
 * @brief Entity system stress tests
 *
 * Comprehensive stress tests for entity systems including loading
 * large numbers of entities and running simulation frames.
 */

#ifndef ENTITY_STRESS_TEST_H
#define ENTITY_STRESS_TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Entity Stress Test Configuration
// ========================================

typedef struct EntityStressTestConfig {
    bool enable_verbose_output;         // Enable verbose test output
    bool enable_memory_tracking;        // Enable memory usage tracking
    bool enable_performance_monitoring; // Enable performance monitoring
    uint32_t entity_count;             // Number of entities to create
    uint32_t frame_count;              // Number of frames to simulate
    float time_step;                   // Simulation time step
    uint32_t components_per_entity;     // Number of components per entity
    bool enable_physics;               // Enable physics simulation
    bool enable_rendering;              // Enable rendering simulation
    bool enable_ai;                    // Enable AI simulation
    const char *output_file;            // Output file for results
} EntityStressTestConfig;

// ========================================
// Entity Stress Test Results
// ========================================

typedef struct EntityStressTestResults {
    uint32_t total_entities;           // Total entities created
    uint32_t active_entities;           // Currently active entities
    uint32_t frames_simulated;         // Total frames simulated
    double total_time_ms;               // Total execution time
    double average_frame_time_ms;        // Average frame time
    double max_frame_time_ms;            // Maximum frame time
    double min_frame_time_ms;            // Minimum frame time
    size_t peak_memory_usage;          // Peak memory usage
    size_t current_memory_usage;        // Current memory usage
    uint32_t memory_allocations;        // Total memory allocations
    uint32_t memory_deallocations;      // Total memory deallocations
    double entities_per_second;         // Entity processing rate
    double frames_per_second;           // Frame rate achieved
    bool test_passed;                   // Whether test passed
    char error_messages[4096];          // Accumulated error messages
    char performance_report[2048];      // Performance report
} EntityStressTestResults;

// ========================================
// Entity Component Types
// ========================================

typedef enum ComponentType {
    COMPONENT_TRANSFORM,               // Transform component
    COMPONENT_PHYSICS,                 // Physics component
    COMPONENT_RENDERING,               // Rendering component
    COMPONENT_AI,                      // AI component
    COMPONENT_ANIMATION,                // Animation component
    COMPONENT_AUDIO,                   // Audio component
    COMPONENT_SCRIPT,                  // Script component
    COMPONENT_CUSTOM,                  // Custom component
    COMPONENT_COUNT                    // Total component types
} ComponentType;

// ========================================
// Entity Structure
// ========================================

typedef struct Entity {
    uint32_t id;                        // Unique entity ID
    char name[64];                      // Entity name
    bool is_active;                     // Whether entity is active
    void *components[COMPONENT_COUNT];   // Component pointers
    uint32_t component_mask;             // Component presence mask
    float position[3];                  // Position
    float velocity[3];                  // Velocity
    float rotation[4];                  // Rotation (quaternion)
    float scale[3];                     // Scale
    uint64_t creation_time;              // Creation timestamp
    uint64_t last_update_time;          // Last update timestamp
    uint32_t update_count;               // Update count
} Entity;

// ========================================
// Component Base Structure
// ========================================

typedef struct Component {
    ComponentType type;                  // Component type
    uint32_t entity_id;                 // Owner entity ID
    bool is_enabled;                    // Whether component is enabled
    void *data;                         // Component data
    size_t data_size;                   // Data size
} Component;

// ========================================
// Transform Component
// ========================================

typedef struct TransformComponent {
    float position[3];                  // Position
    float rotation[4];                  // Rotation (quaternion)
    float scale[3];                     // Scale
    float matrix[16];                   // Transformation matrix
    bool is_dirty;                      // Whether transform needs update
} TransformComponent;

// ========================================
// Physics Component
// ========================================

typedef struct PhysicsComponent {
    float velocity[3];                  // Velocity
    float acceleration[3];              // Acceleration
    float mass;                         // Mass
    float radius;                       // Bounding radius
    bool is_static;                     // Whether object is static
    bool has_gravity;                   // Whether affected by gravity
} PhysicsComponent;

// ========================================
// Rendering Component
// ========================================

typedef struct RenderingComponent {
    uint32_t mesh_id;                   // Mesh ID
    uint32_t material_id;                // Material ID
    bool is_visible;                    // Whether visible
    float color[4];                     // Color tint
    uint32_t vertex_count;              // Vertex count
    uint32_t triangle_count;            // Triangle count
} RenderingComponent;

// ========================================
// AI Component
// ========================================

typedef struct AIComponent {
    uint32_t behavior_tree_id;           // Behavior tree ID
    float think_interval;               // Think interval
    uint64_t last_think_time;           // Last think timestamp
    uint32_t current_state;             // Current AI state
    float health;                       // Health points
    float max_health;                   // Maximum health
} AIComponent;

// ========================================
// Global Test State
// ========================================

extern EntityStressTestConfig g_entity_stress_test_config;
extern EntityStressTestResults g_entity_stress_test_results;

// ========================================
// Test Suite Functions
// ========================================

/**
 * Initialize entity stress test
 * @param config Test configuration
 * @return True if initialization successful
 */
bool entity_stress_test_init(const EntityStressTestConfig *config);

/**
 * Shutdown entity stress test
 * @param generate_report Whether to generate final report
 */
void entity_stress_test_shutdown(bool generate_report);

/**
 * Run entity stress test
 * @return True if test passes
 */
bool entity_stress_test_run(void);

/**
 * Get test results
 * @return Test results
 */
EntityStressTestResults entity_stress_test_get_results(void);

/**
 * Print test summary
 */
void entity_stress_test_print_summary(void);

/**
 * Export test results to file
 * @param filename Output filename
 * @return True if export successful
 */
bool entity_stress_test_export_results(const char *filename);

// ========================================
// Entity Management
// ========================================

/**
 * Create entity
 * @param name Entity name
 * @return Created entity or NULL on failure
 */
Entity* entity_create(const char *name);

/**
 * Destroy entity
 * @param entity Entity to destroy
 */
void entity_destroy(Entity *entity);

/**
 * Add component to entity
 * @param entity Entity
 * @param type Component type
 * @param component Component data
 * @return True if addition successful
 */
bool entity_add_component(Entity *entity, ComponentType type, void *component);

/**
 * Remove component from entity
 * @param entity Entity
 * @param type Component type
 * @return True if removal successful
 */
bool entity_remove_component(Entity *entity, ComponentType type);

/**
 * Get component from entity
 * @param entity Entity
 * @param type Component type
 * @return Component pointer or NULL if not found
 */
void* entity_get_component(const Entity *entity, ComponentType type);

/**
 * Update entity
 * @param entity Entity to update
 * @param dt Time delta
 */
void entity_update(Entity *entity, float dt);

// ========================================
// Component Creation
// ========================================

/**
 * Create transform component
 * @param position Initial position
 * @param rotation Initial rotation
 * @param scale Initial scale
 * @return Created component or NULL on failure
 */
TransformComponent* create_transform_component(const float position[3],
                                            const float rotation[4],
                                            const float scale[3]);

/**
 * Create physics component
 * @param mass Object mass
 * @param radius Bounding radius
 * @param is_static Whether static
 * @return Created component or NULL on failure
 */
PhysicsComponent* create_physics_component(float mass, float radius, bool is_static);

/**
 * Create rendering component
 * @param mesh_id Mesh ID
 * @param material_id Material ID
 * @param color Color tint
 * @return Created component or NULL on failure
 */
RenderingComponent* create_rendering_component(uint32_t mesh_id,
                                             uint32_t material_id,
                                             const float color[4]);

/**
 * Create AI component
 * @param behavior_tree_id Behavior tree ID
 * @param health Health points
 * @return Created component or NULL on failure
 */
AIComponent* create_ai_component(uint32_t behavior_tree_id, float health);

// ========================================
// System Updates
// ========================================

/**
 * Update physics system
 * @param entities Entity array
 * @param entity_count Number of entities
 * @param dt Time delta
 */
void update_physics_system(Entity *entities, uint32_t entity_count, float dt);

/**
 * Update rendering system
 * @param entities Entity array
 * @param entity_count Number of entities
 * @param dt Time delta
 */
void update_rendering_system(Entity *entities, uint32_t entity_count, float dt);

/**
 * Update AI system
 * @param entities Entity array
 * @param entity_count Number of entities
 * @param dt Time delta
 */
void update_ai_system(Entity *entities, uint32_t entity_count, float dt);

/**
 * Update animation system
 * @param entities Entity array
 * @param entity_count Number of entities
 * @param dt Time delta
 */
void update_animation_system(Entity *entities, uint32_t entity_count, float dt);

// ========================================
// Stress Test Functions
// ========================================

/**
 * Create test entities
 * @param count Number of entities to create
 * @return True if creation successful
 */
bool create_test_entities(uint32_t count);

/**
 * Destroy all test entities
 */
void destroy_test_entities(void);

/**
 * Simulate frames
 * @param frame_count Number of frames to simulate
 * @return True if simulation successful
 */
bool simulate_frames(uint32_t frame_count);

/**
 * Validate entity state
 * @return True if entity state is valid
 */
bool validate_entity_state(void);

/**
 * Measure memory usage
 * @return Current memory usage in bytes
 */
size_t measure_memory_usage(void);

/**
 * Validate memory integrity
 * @return True if memory is valid
 */
bool validate_memory_integrity(void);

// ========================================
// Performance Monitoring
// ========================================

/**
 * Start performance monitoring
 */
void start_performance_monitoring(void);

/**
 * Stop performance monitoring
 */
void stop_performance_monitoring(void);

/**
 * Get frame time statistics
 * @param avg_time Output average time
 * @param max_time Output maximum time
 * @param min_time Output minimum time
 */
void get_frame_time_statistics(double *avg_time, double *max_time, double *min_time);

/**
 * Get memory statistics
 * @param current_usage Output current usage
 * @param peak_usage Output peak usage
 * @param allocation_count Output allocation count
 */
void get_memory_statistics(size_t *current_usage, size_t *peak_usage, uint32_t *allocation_count);

// ========================================
// Utility Functions
// ========================================

/**
 * Generate random position
 * @param position Output position
 * @param bounds Position bounds
 */
void generate_random_position(float position[3], const float bounds[3]);

/**
 * Generate random velocity
 * @param velocity Output velocity
 * @param max_speed Maximum speed
 */
void generate_random_velocity(float velocity[3], float max_speed);

/**
 * Generate random color
 * @param color Output color
 */
void generate_random_color(float color[4]);

/**
 * Calculate entity processing rate
 * @param entity_count Number of entities
 * @param frame_time Frame time in milliseconds
 * @return Entities per second
 */
double calculate_entity_processing_rate(uint32_t entity_count, double frame_time);

/**
 * Validate entity count
 * @param expected Expected count
 * @param actual Actual count
 * @return True if count is valid
 */
bool validate_entity_count(uint32_t expected, uint32_t actual);

/**
 * Check for memory leaks
 * @return True if memory leaks detected
 */
bool check_memory_leaks(void);

/**
 * Generate performance report
 * @param report Output report string
 * @param max_size Maximum report size
 * @return True if report generated
 */
bool generate_performance_report(char *report, size_t max_size);

#ifdef __cplusplus
}
#endif

#endif /* ENTITY_STRESS_TEST_H */
