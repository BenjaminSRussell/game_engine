/**
 * @file physics_serialization.h
 * @brief Physics serialization system
 *
 * Provides serialization and deserialization for physics objects,
 * materials, constraints, and world state with support for multiple
 * formats and versioning.
 */

#ifndef PHYSICS_SERIALIZATION_H
#define PHYSICS_SERIALIZATION_H

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
typedef struct PhysicsMaterial PhysicsMaterial;
typedef struct PhysicsConstraint PhysicsConstraint;

// ========================================
// Serialization Formats
// ========================================

typedef enum SerializationFormat {
    SERIALIZATION_FORMAT_BINARY,      // Binary format (compact)
    SERIALIZATION_FORMAT_JSON,        // JSON format (human readable)
    SERIALIZATION_FORMAT_XML,         // XML format (structured)
    SERIALIZATION_FORMAT_MSGPACK,     // MessagePack format (efficient)
    SERIALIZATION_FORMAT_CUSTOM       // Custom format
} SerializationFormat;

// ========================================
// Serialization Flags
// ========================================

typedef enum SerializationFlags {
    SERIALIZATION_FLAG_NONE = 0x00,
    SERIALIZATION_FLAG_COMPRESS = 0x01,      // Compress data
    SERIALIZATION_FLAG_ENCRYPT = 0x02,       // Encrypt data
    SERIALIZATION_FLAG_CHECKSUM = 0x04,      // Add checksum
    SERIALIZATION_FLAG_VERSIONED = 0x08,     // Include version info
    SERIALIZATION_FLAG_DELTA = 0x10,         // Delta compression
    SERIALIZATION_FLAG_STREAMING = 0x20,     // Streaming mode
    SERIALIZATION_FLAG_ALL = 0xFF
} SerializationFlags;

// ========================================
// Serialization Context
// ========================================

typedef struct SerializationContext {
    SerializationFormat format;       // Serialization format
    uint32_t flags;                  // Serialization flags
    uint32_t version;                // Serialization version
    void *custom_data;               // Custom format data
    size_t buffer_size;              // Buffer size
    uint8_t *buffer;                 // Serialization buffer
    size_t buffer_used;              // Buffer used bytes
    bool own_buffer;                 // Own the buffer
    void *compression_context;       // Compression context
    void *encryption_context;        // Encryption context
} SerializationContext;

// ========================================
// Serialization Header
// ========================================

typedef struct SerializationHeader {
    char magic[4];                  // Magic number ('PHYS')
    uint32_t version;                // Format version
    uint32_t flags;                  // Serialization flags
    uint32_t format;                 // Serialization format
    uint64_t timestamp;              // Serialization timestamp
    uint32_t checksum;               // Data checksum
    uint64_t data_size;              // Uncompressed data size
    uint64_t compressed_size;         // Compressed data size
    char description[64];            // Description
} SerializationHeader;

// ========================================
// Object Serialization Data
// ========================================

typedef struct ObjectSerializationData {
    uint64_t object_id;              // Object ID
    uint32_t type;                   // Object type
    uint32_t body_type;              // Body type
    uint32_t material_id;            // Material ID
    bool active;                     // Active state
    bool sleeping;                   // Sleeping state
    bool visible;                    // Visible state
    uint32_t layer_mask;             // Layer mask
    uint32_t category_mask;          // Category mask
    
    // Transform
    float transform[16];             // 4x4 transform matrix
    
    // Physics properties
    float velocity[3];               // Linear velocity
    float angular_velocity[3];        // Angular velocity
    float mass;                      // Mass
    float inertia[9];                // Inertia tensor
    float center_of_mass[3];          // Center of mass
    
    // Bounds
    float bounding_box[6];           // AABB bounding box
    float bounding_sphere[4];        // Bounding sphere
    
    // Custom data
    void *custom_data;               // Custom object data
    size_t custom_data_size;          // Custom data size
    
} ObjectSerializationData;

// ========================================
// Material Serialization Data
// ========================================

typedef struct MaterialSerializationData {
    uint32_t material_id;            // Material ID
    char name[64];                   // Material name
    float density;                   // Density
    float friction;                  // Friction
    float restitution;               // Restitution
    float damping;                   // Linear damping
    float angular_damping;           // Angular damping
    float thickness;                 // Thickness
    bool is_trigger;                 // Is trigger
    bool is_sensor;                  // Is sensor
    
    // Custom properties
    void *custom_properties;         // Custom properties
    size_t custom_properties_size;    // Custom properties size
    
} MaterialSerializationData;

// ========================================
// Constraint Serialization Data
// ========================================

typedef struct ConstraintSerializationData {
    uint64_t constraint_id;          // Constraint ID
    uint64_t body_a_id;              // First body ID
    uint64_t body_b_id;              // Second body ID
    uint32_t type;                   // Constraint type
    bool enabled;                    // Enabled state
    float break_force;               // Break force
    float current_force;             // Current force
    bool is_broken;                  // Broken state
    
    // Constraint-specific data
    void *constraint_data;           // Constraint data
    size_t constraint_data_size;     // Constraint data size
    
} ConstraintSerializationData;

// ========================================
// World Serialization Data
// ========================================

typedef struct WorldSerializationData {
    // World configuration
    float gravity[3];                // Gravity vector
    float time_step;                 // Time step
    float max_time_step;             // Maximum time step
    int velocity_iterations;          // Velocity iterations
    int position_iterations;          // Position iterations
    
    // World state
    float current_time;              // Current simulation time
    float accumulated_time;          // Accumulated time
    int substep_count;              // Substep count
    
    // Performance settings
    bool enable_sleeping;            // Enable sleeping
    bool enable_warm_starting;        // Enable warm starting
    bool enable_adaptive_time_step;    // Enable adaptive time step
    float sleep_threshold;           // Sleep threshold
    float wake_up_threshold;          // Wake up threshold
    
    // Collision settings
    bool enable_collision_detection;  // Enable collision detection
    bool enable_collision_response;    // Enable collision response
    bool enable_continuous_collision; // Enable continuous collision
    float collision_margin;           // Collision margin
    
    // Object arrays
    ObjectSerializationData *objects; // Objects array
    int object_count;                // Number of objects
    
    // Material arrays
    MaterialSerializationData *materials; // Materials array
    int material_count;              // Number of materials
    
    // Constraint arrays
    ConstraintSerializationData *constraints; // Constraints array
    int constraint_count;            // Number of constraints
    
} WorldSerializationData;

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create serialization context
 * @param format Serialization format
 * @param flags Serialization flags
 * @param buffer_size Initial buffer size
 * @return Serialization context or NULL on failure
 */
SerializationContext* serialization_context_create(SerializationFormat format, 
                                                   uint32_t flags, size_t buffer_size);

/**
 * Destroy serialization context
 * @param context Serialization context
 */
void serialization_context_destroy(SerializationContext *context);

/**
 * Reset serialization context
 * @param context Serialization context
 */
void serialization_context_reset(SerializationContext *context);

// ========================================
// Buffer Management
// ========================================

/**
 * Set serialization buffer
 * @param context Serialization context
 * @param buffer Buffer pointer
 * @param buffer_size Buffer size
 * @param own_buffer Own the buffer
 */
void serialization_context_set_buffer(SerializationContext *context, 
                                      uint8_t *buffer, size_t buffer_size, bool own_buffer);

/**
 * Get serialization buffer
 * @param context Serialization context
 * @param buffer Output buffer pointer
 * @param buffer_size Output buffer size
 * @return Buffer used bytes
 */
size_t serialization_context_get_buffer(SerializationContext *context, 
                                       uint8_t **buffer, size_t *buffer_size);

/**
 * Resize serialization buffer
 * @param context Serialization context
 * @param new_size New buffer size
 * @return True if successful
 */
bool serialization_context_resize_buffer(SerializationContext *context, size_t new_size);

// ========================================
// Object Serialization
// ========================================

/**
 * Serialize physics object
 * @param context Serialization context
 * @param object Physics object
 * @return True if successful
 */
bool serialize_physics_object(SerializationContext *context, const PhysicsObject *object);

/**
 * Deserialize physics object
 * @param context Serialization context
 * @param object Output object data
 * @return True if successful
 */
bool deserialize_physics_object(SerializationContext *context, ObjectSerializationData *object);

/**
 * Serialize object array
 * @param context Serialization context
 * @param objects Object array
 * @param object_count Number of objects
 * @return True if successful
 */
bool serialize_object_array(SerializationContext *context, const PhysicsObject **objects, int object_count);

/**
 * Deserialize object array
 * @param context Serialization context
 * @param objects Output object array
 * @param max_objects Maximum objects to deserialize
 * @return Number of objects deserialized
 */
int deserialize_object_array(SerializationContext *context, ObjectSerializationData **objects, int max_objects);

// ========================================
// Material Serialization
// ========================================

/**
 * Serialize physics material
 * @param context Serialization context
 * @param material Physics material
 * @return True if successful
 */
bool serialize_physics_material(SerializationContext *context, const PhysicsMaterial *material);

/**
 * Deserialize physics material
 * @param context Serialization context
 * @param material Output material data
 * @return True if successful
 */
bool deserialize_physics_material(SerializationContext *context, MaterialSerializationData *material);

/**
 * Serialize material array
 * @param context Serialization context
 * @param materials Material array
 * @param material_count Number of materials
 * @return True if successful
 */
bool serialize_material_array(SerializationContext *context, const PhysicsMaterial **materials, int material_count);

/**
 * Deserialize material array
 * @param context Serialization context
 * @param materials Output material array
 * @param max_materials Maximum materials to deserialize
 * @return Number of materials deserialized
 */
int deserialize_material_array(SerializationContext *context, MaterialSerializationData **materials, int max_materials);

// ========================================
// Constraint Serialization
// ========================================

/**
 * Serialize physics constraint
 * @param context Serialization context
 * @param constraint Physics constraint
 * @return True if successful
 */
bool serialize_physics_constraint(SerializationContext *context, const PhysicsConstraint *constraint);

/**
 * Deserialize physics constraint
 * @param context Serialization context
 * @param constraint Output constraint data
 * @return True if successful
 */
bool deserialize_physics_constraint(SerializationContext *context, ConstraintSerializationData *constraint);

/**
 * Serialize constraint array
 * @param context Serialization context
 * @param constraints Constraint array
 * @param constraint_count Number of constraints
 * @return True if successful
 */
bool serialize_constraint_array(SerializationContext *context, const PhysicsConstraint **constraints, int constraint_count);

/**
 * Deserialize constraint array
 * @param context Serialization context
 * @param constraints Output constraint array
 * @param max_constraints Maximum constraints to deserialize
 * @return Number of constraints deserialized
 */
int deserialize_constraint_array(SerializationContext *context, ConstraintSerializationData **constraints, int max_constraints);

// ========================================
// World Serialization
// ========================================

/**
 * Serialize physics world
 * @param context Serialization context
 * @param world Physics world
 * @return True if successful
 */
bool serialize_physics_world(SerializationContext *context, const PhysicsWorld *world);

/**
 * Deserialize physics world
 * @param context Serialization context
 * @param world Output world data
 * @return True if successful
 */
bool deserialize_physics_world(SerializationContext *context, WorldSerializationData *world);

/**
 * Serialize world to buffer
 * @param world Physics world
 * @param format Serialization format
 * @param flags Serialization flags
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Bytes written or 0 on failure
 */
size_t serialize_world_to_buffer(const PhysicsWorld *world, SerializationFormat format, 
                                 uint32_t flags, void *buffer, size_t buffer_size);

/**
 * Deserialize world from buffer
 * @param buffer Input buffer
 * @param buffer_size Buffer size
 * @param world Output world data
 * @return Bytes read or 0 on failure
 */
size_t deserialize_world_from_buffer(const void *buffer, size_t buffer_size, WorldSerializationData *world);

// ========================================
// File Operations
// ========================================

/**
 * Save world to file
 * @param world Physics world
 * @param filename Output filename
 * @param format Serialization format
 * @param flags Serialization flags
 * @return True if successful
 */
bool save_world_to_file(const PhysicsWorld *world, const char *filename, 
                        SerializationFormat format, uint32_t flags);

/**
 * Load world from file
 * @param filename Input filename
 * @param world Output world data
 * @return True if successful
 */
bool load_world_from_file(const char *filename, WorldSerializationData *world);

/**
 * Save object to file
 * @param object Physics object
 * @param filename Output filename
 * @param format Serialization format
 * @param flags Serialization flags
 * @return True if successful
 */
bool save_object_to_file(const PhysicsObject *object, const char *filename, 
                         SerializationFormat format, uint32_t flags);

/**
 * Load object from file
 * @param filename Input filename
 * @param object Output object data
 * @return True if successful
 */
bool load_object_from_file(const char *filename, ObjectSerializationData *object);

// ========================================
// Streaming Operations
// ========================================

/**
 * Begin streaming serialization
 * @param context Serialization context
 * @param filename Output filename
 * @return True if successful
 */
bool begin_streaming_serialization(SerializationContext *context, const char *filename);

/**
 * Stream serialize object
 * @param context Serialization context
 * @param object Physics object
 * @return True if successful
 */
bool stream_serialize_object(SerializationContext *context, const PhysicsObject *object);

/**
 * End streaming serialization
 * @param context Serialization context
 * @return True if successful
 */
bool end_streaming_serialization(SerializationContext *context);

/**
 * Begin streaming deserialization
 * @param context Serialization context
 * @param filename Input filename
 * @return True if successful
 */
bool begin_streaming_deserialization(SerializationContext *context, const char *filename);

/**
 * Stream deserialize object
 * @param context Serialization context
 * @param object Output object data
 * @return True if successful
 */
bool stream_deserialize_object(SerializationContext *context, ObjectSerializationData *object);

/**
 * End streaming deserialization
 * @param context Serialization context
 * @return True if successful
 */
bool end_streaming_deserialization(SerializationContext *context);

// ========================================
// Delta Serialization
// ========================================

/**
 * Create delta between two world states
 * @param base_world Base world state
 * @param target_world Target world state
 * @param delta Output delta data
 * @return True if successful
 */
bool create_world_delta(const WorldSerializationData *base_world, 
                        const WorldSerializationData *target_world, 
                        WorldSerializationData *delta);

/**
 * Apply delta to world state
 * @param base_world Base world state
 * @param delta Delta data
 * @param result Output result world
 * @return True if successful
 */
bool apply_world_delta(const WorldSerializationData *base_world, 
                       const WorldSerializationData *delta, 
                       WorldSerializationData *result);

/**
 * Serialize world delta
 * @param context Serialization context
 * @param delta Delta data
 * @return True if successful
 */
bool serialize_world_delta(SerializationContext *context, const WorldSerializationData *delta);

/**
 * Deserialize world delta
 * @param context Serialization context
 * @param delta Output delta data
 * @return True if successful
 */
bool deserialize_world_delta(SerializationContext *context, WorldSerializationData *delta);

// ========================================
// Compression and Encryption
// ========================================

/**
 * Compress serialized data
 * @param context Serialization context
 * @return True if successful
 */
bool compress_serialized_data(SerializationContext *context);

/**
 * Decompress serialized data
 * @param context Serialization context
 * @return True if successful
 */
bool decompress_serialized_data(SerializationContext *context);

/**
 * Encrypt serialized data
 * @param context Serialization context
 * @param key Encryption key
 * @param key_size Key size
 * @return True if successful
 */
bool encrypt_serialized_data(SerializationContext *context, const uint8_t *key, size_t key_size);

/**
 * Decrypt serialized data
 * @param context Serialization context
 * @param key Decryption key
 * @param key_size Key size
 * @return True if successful
 */
bool decrypt_serialized_data(SerializationContext *context, const uint8_t *key, size_t key_size);

// ========================================
// Validation and Checksum
// ========================================

/**
 * Calculate data checksum
 * @param data Data buffer
 * @param data_size Data size
 * @return Checksum value
 */
uint32_t calculate_data_checksum(const void *data, size_t data_size);

/**
 * Validate serialized data
 * @param context Serialization context
 * @return True if data is valid
 */
bool validate_serialized_data(SerializationContext *context);

/**
 * Add checksum to serialized data
 * @param context Serialization context
 * @return True if successful
 */
bool add_data_checksum(SerializationContext *context);

/**
 * Verify data checksum
 * @param context Serialization context
 * @return True if checksum is valid
 */
bool verify_data_checksum(SerializationContext *context);

// ========================================
// Format Conversion
// ========================================

/**
 * Convert between serialization formats
 * @param source_format Source format
 * @param target_format Target format
 * @param source_data Source data buffer
 * @param source_size Source data size
 * @param target_data Output target buffer
 * @param target_size Target buffer size
 * @return Bytes written or 0 on failure
 */
size_t convert_serialization_format(SerializationFormat source_format, 
                                    SerializationFormat target_format,
                                    const void *source_data, size_t source_size,
                                    void *target_data, size_t target_size);

/**
 * Get format info
 * @param format Serialization format
 * @param name Output format name
 * @param description Output format description
 * @param version Output format version
 */
void get_format_info(SerializationFormat format, const char **name, 
                    const char **description, uint32_t *version);

// ========================================
// Utility Functions
// ========================================

/**
 * Get serialization header size
 * @param format Serialization format
 * @return Header size in bytes
 */
size_t get_serialization_header_size(SerializationFormat format);

/**
 * Estimate serialization size
 * @param world Physics world
 * @param format Serialization format
 * @param flags Serialization flags
 * @return Estimated size in bytes
 */
size_t estimate_serialization_size(const PhysicsWorld *world, 
                                   SerializationFormat format, uint32_t flags);

/**
 * Get serialization statistics
 * @param context Serialization context
 * @param data_size Data size
 * @param compressed_size Compressed size
 * @param compression_ratio Compression ratio
 * @param serialization_time Serialization time
 */
void get_serialization_stats(SerializationContext *context, 
                             size_t *data_size, size_t *compressed_size,
                             float *compression_ratio, float *serialization_time);

/**
 * Validate serialization context
 * @param context Serialization context
 * @return True if context is valid
 */
bool validate_serialization_context(SerializationContext *context);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_SERIALIZATION_H */
