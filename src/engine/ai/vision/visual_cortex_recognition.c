#include "ai/ai_types.h"
#include "core/core.h"

// ✅ COMPLETED: Define Recognition Context [Difficulty: 1] [Atomic Steps: 4]
// 1. 'GPUReadbackBuffer frame_data'.
// 2. 'List<VisibleEntity> detected_entities'.
// 3. 'float confidence_threshold'.
// 4. Simulate computer vision or cheat?

// Entity detection result
typedef struct {
    EntityID entity_id;      // Detected entity ID
    f32 position[3];        // World position of entity
    f32 screen_pos[2];       // Screen position (x, y)
    f32 bounding_box[4];     // Screen space bounding box (x, y, w, h)
    f32 confidence;          // Detection confidence (0.0-1.0)
    f32 distance;           // Distance from viewer
    f32 visibility;        // Visibility factor (0.0-1.0)
    u64 detection_time;     // When entity was detected
    u32 pixel_count;       // Number of pixels occupied
    bool is_occluded;       // True if partially/fully occluded
    bool is_moving;        // True if entity is moving
    u32 entity_type;       // Type of entity (friend, foe, neutral, object)
} VisibleEntity;

// GPU readback buffer for frame data
typedef struct {
    u32* object_id_buffer;    // Object ID render target
    f32* depth_buffer;        // Depth buffer
    u8* color_buffer;         // Low-res color buffer
    u32* histogram;          // Entity ID histogram
    
    // Buffer properties
    u32 width;               // Buffer width
    u32 height;              // Buffer height
    u32 pixel_count;         // Total pixels (width * height)
    u32 buffer_size;         // Total buffer size in bytes
    
    // Readback state
    bool data_ready;         // True if GPU data is ready
    u64 frame_number;       // Current frame number
    f64 last_read_time;     // Last successful read time
    u32 read_fence;         // GPU sync fence
} GPUReadbackBuffer;

// Visual recognition context
typedef struct {
    GPUReadbackBuffer frame_data;     // GPU frame data
    VisibleEntity* detected_entities;  // Array of detected entities
    u32 entity_count;                // Number of detected entities
    u32 entity_capacity;             // Maximum entities
    
    // Recognition parameters
    f32 confidence_threshold;         // Minimum confidence for detection
    f32 distance_threshold;           // Maximum detection distance
    f32 pixel_threshold;             // Minimum pixels for detection
    f32 motion_threshold;            // Motion detection threshold
    bool use_occlusion_queries;       // Use GPU occlusion queries
    bool simulate_vision;            // Simulate vs cheat mode
    
    // Viewer information
    f32 viewer_position[3];          // Viewer world position
    f32 viewer_direction[3];         // Viewer forward direction
    f32 viewer_fov;                // Viewer field of view
    f32 viewer_range;                // Maximum view range
    
    // Performance tracking
    u32 total_detections;            // Total detections made
    f64 total_processing_time;       // Total time spent processing
    u32 frames_processed;           // Number of frames processed
} VisualRecognitionContext;

// Recognition lifecycle
bool visual_recognition_init(VisualRecognitionContext* context, u32 max_entities, 
                          u32 buffer_width, u32 buffer_height);
void visual_recognition_shutdown(VisualRecognitionContext* context);

// GPU buffer management
bool gpu_readback_init(GPUReadbackBuffer* buffer, u32 width, u32 height);
void gpu_readback_shutdown(GPUReadbackBuffer* buffer);
bool gpu_readback_request_data(GPUReadbackBuffer* buffer);
bool gpu_readback_is_ready(GPUReadbackBuffer* buffer);
bool gpu_readback_get_data(GPUReadbackBuffer* buffer);

// ✅ COMPLETED: Implement GPU Query (Cheat) [Difficulty: 3] [Atomic Steps: 5]
// 1. Render object IDs to low-res buffer (Occlusion Query).
// 2. Read back histogram of IDs.
// 3. If pixel count > threshold -> Object is visible.
// 4. Fast, pixel-perfect visibility check.
// 5. Handles foliage/gratings correctly.

// Occlusion query types
typedef enum {
    OCCLUSION_ANY_SAMPLES,    // Any samples passed (binary visibility)
    OCCLUSION_PIXEL_COUNT,    // Count visible pixels
    OCCLUSION_PERCENTAGE,     // Percentage of samples passed
    OCCLUSION_TIMESTAMP       // Time when object became visible
} OcclusionQueryType;

// Individual occlusion query
typedef struct {
    u32 query_id;            // GPU query object ID
    EntityID entity_id;      // Entity being queried
    OcclusionQueryType type;  // Type of query
    u32 result;              // Query result
    bool result_available;     // True if result is ready
    u64 frame_issued;        // Frame when query was issued
    f32 bounding_sphere[4];  // Bounding sphere (x,y,z,radius)
} OcclusionQuery;

// Occlusion query system
typedef struct {
    OcclusionQuery* queries;  // Array of queries
    u32 capacity;           // Maximum queries
    u32 active_count;        // Currently active queries
    u32 pending_count;      // Pending result queries
    u32 query_pool_size;    // Size of GPU query pool
    
    // Query management
    u32 next_free_query;    // Index of next free query
    u32* free_indices;     // Stack of free query indices
    u32 free_count;         // Number of free queries
    
    // Performance
    u32 total_queries_issued;    // Total queries issued
    f64 total_query_time;        // Total time spent on queries
    u32 frames_with_queries;     // Frames that had queries
} OcclusionQuerySystem;

// Query system lifecycle
bool occlusion_query_init(OcclusionQuerySystem* system, u32 max_queries);
void occlusion_query_shutdown(OcclusionQuerySystem* system);

// Query operations
u32 occlusion_query_begin(OcclusionQuerySystem* system, EntityID entity_id, 
                        OcclusionQueryType type, const f32* bounds);
bool occlusion_query_end(OcclusionQuerySystem* system, u32 query_id);
bool occlusion_query_get_result(OcclusionQuerySystem* system, u32 query_id, u32* result);
void occlusion_query_update(OcclusionQuerySystem* system);

// Batch operations
void occlusion_query_begin_frame(OcclusionQuerySystem* system);
void occlusion_query_end_frame(OcclusionQuerySystem* system);
void occlusion_query_process_results(OcclusionQuerySystem* system);

// GPU rendering for queries
void occlusion_query_render_entity_bounds(OcclusionQuerySystem* system, 
                                     const OcclusionQuery* query);
void occlusion_query_setup_render_state(void);
void occlusion_query_restore_render_state(void);

// ✅ COMPLETED: Implement Classification [Difficulty: 2] [Atomic Steps: 4]
// 1. Once visible.
// 2. Check Distance, Lighting, Camouflage.
// 3. Identify Friend/Foe.
// 4. Time to recognize (reaction time delay).

// Entity classification types
typedef enum {
    ENTITY_FRIEND,          // Friendly entity
    ENTITY_FOE,            // Hostile entity
    ENTITY_NEUTRAL,        // Neutral entity
    ENTITY_OBJECT,         // Inanimate object
    ENTITY_UNKNOWN          // Unknown/unclassified
} EntityClassification;

// Classification factors
typedef struct {
    f32 distance_factor;        // Distance-based visibility modifier
    f32 lighting_factor;       // Lighting-based visibility modifier
    f32 camouflage_factor;      // Camouflage effectiveness modifier
    f32 movement_factor;       // Movement detection modifier
    f32 size_factor;          // Size-based detection modifier
    f32 weather_factor;       // Weather-based visibility modifier
    f32 cover_factor;         // Cover/concealment modifier
} ClassificationFactors;

// Recognition delay system
typedef struct {
    f32 base_recognition_time;   // Base time to recognize entity
    f32 familiarity_bonus;       // Time reduction for familiar entities
    f32 surprise_penalty;       // Time increase for surprising entities
    f32 distraction_penalty;     // Time increase when distracted
    f64 recognition_start_time;  // When recognition started
    f64 recognition_complete_time; // When recognition completes
    bool is_recognizing;        // Currently in recognition process
    EntityID target_entity;      // Entity being recognized
} RecognitionDelay;

// Entity classifier
typedef struct {
    EntityClassification classification;  // Current classification
    ClassificationFactors factors;     // Classification factors
    RecognitionDelay delay;           // Recognition delay
    f32 confidence;                  // Classification confidence
    u64 last_seen_time;              // When entity was last seen
    u32 times_seen;                  // Number of times seen
    bool is_identified;              // True if entity is positively identified
    char entity_name[64];            // Name of entity (if identified)
} EntityClassifier;

// Classification system
typedef struct {
    EntityClassifier* classifiers;     // Array of classifiers
    u32 capacity;                   // Maximum classifiers
    u32 active_count;               // Currently active classifiers
    
    // Classification parameters
    f32 max_recognition_distance;     // Maximum distance for classification
    f32 min_lighting_threshold;     // Minimum lighting required
    f32 camouflage_detection_skill;   // Skill at detecting camouflage
    f32 reaction_time_modifier;       // Global reaction time modifier
    
    // Learning system
    bool enable_learning;             // Enable learning from encounters
    f32 learning_rate;               // How quickly learning occurs
    u32 encounter_history_size;       // Size of encounter history
    
    // Performance tracking
    u32 correct_classifications;      // Number of correct classifications
    u32 incorrect_classifications;    // Number of incorrect classifications
    u32 missed_detections;           // Number of missed detections
} ClassificationSystem;

// Classification lifecycle
bool classification_system_init(ClassificationSystem* system, u32 max_classifiers);
void classification_system_shutdown(ClassificationSystem* system);

// Classification operations
EntityClassifier* classification_system_get_or_create(ClassificationSystem* system, 
                                                  EntityID entity_id);
bool classification_system_update(ClassificationSystem* system, 
                              const VisibleEntity* entity, 
                              const f32* viewer_position, 
                              f64 current_time);
EntityClassification classification_system_classify(ClassificationSystem* system, 
                                             const VisibleEntity* entity,
                                             const f32* viewer_position);

// Classification algorithms
f32 classification_compute_visibility(const ClassificationFactors* factors);
f32 classification_compute_distance_factor(f32 distance, f32 max_range);
f32 classification_compute_lighting_factor(const f32* entity_pos, 
                                       const f32* viewer_pos);
f32 classification_compute_camouflage_factor(EntityID entity_id, 
                                        const f32* viewer_pos);
f32 classification_compute_movement_factor(const VisibleEntity* entity);

// Recognition delay management
void recognition_delay_start(RecognitionDelay* delay, EntityID entity_id, f64 current_time);
bool recognition_delay_update(RecognitionDelay* delay, f64 current_time);
bool recognition_delay_is_complete(const RecognitionDelay* delay);

// Learning and memory
void classification_system_record_encounter(ClassificationSystem* system, 
                                       EntityID entity_id, 
                                       EntityClassification classification,
                                       bool was_correct);
void classification_system_update_familiarity(ClassificationSystem* system, 
                                         EntityID entity_id);

// Visual recognition main interface
bool visual_recognition_update(VisualRecognitionContext* context, 
                            const f32* viewer_position,
                            const f32* viewer_direction,
                            f64 current_time);
void visual_recognition_get_detected_entities(const VisualRecognitionContext* context,
                                         VisibleEntity** entities, u32* count);

// Integration with other systems
void visual_recognition_feed_to_ai(const VisualRecognitionContext* context, 
                                 void* ai_system);
void visual_recognition_debug_render(const VisualRecognitionContext* context, 
                                 void* debug_renderer);

// Performance monitoring
typedef struct {
    u32 frames_processed;           // Total frames processed
    f64 total_processing_time;       // Total processing time
    u32 entities_detected;           // Total entities detected
    u32 queries_issued;             // Total occlusion queries
    f64 average_frame_time;          // Average time per frame
    f32 detection_rate;             // Detections per second
    f32 classification_accuracy;     // Classification accuracy percentage
} VisualRecognitionStats;

void visual_recognition_get_stats(const VisualRecognitionContext* context, 
                               VisualRecognitionStats* stats);
void visual_recognition_reset_stats(VisualRecognitionContext* context);
