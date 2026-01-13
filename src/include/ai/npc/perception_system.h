#ifndef PERCEPTION_SYSTEM_H
#define PERCEPTION_SYSTEM_H

#include <core/types.h>
#include <core/logger.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Perception System - AI sensory processing for Visual, Auditory, Olfactory, Tactile stimuli

// Forward declarations
typedef struct PerceptionSystem PerceptionSystem;
typedef struct PerceptualAgent PerceptualAgent;
typedef struct Stimulus Stimulus;
typedef struct SensoryInput SensoryInput;
typedef struct PerceivedEntity PerceivedEntity;

// Stimulus types
typedef enum {
    STIMULUS_VISUAL,        // Visual stimulus (sight)
    STIMULUS_AUDITORY,      // Auditory stimulus (sound)
    STIMULUS_OLFACTORY,     // Olfactory stimulus (smell)
    STIMULUS_TACTILE,       // Tactile stimulus (touch)
    STIMULUS_COUNT
} StimulusType;

// Visual stimulus data
typedef struct {
    Vec3 position;           // World position of visual stimulus
    Vec3 direction;          // Direction from observer to stimulus
    f32 distance;            // Distance to stimulus
    f32 brightness;          // Visual brightness (0.0 - 1.0)
    f32 size;               // Visual size (angular size)
    u32 color;              // RGB color value
    f32 motion_speed;        // Motion speed
    bool is_moving;          // Whether stimulus is moving
    f32 contrast;            // Visual contrast
    bool is_in_fov;         // Whether in field of view
    f32 occlusion_factor;   // How much is occluded (0.0 = clear, 1.0 = fully occluded)
} VisualStimulus;

// Auditory stimulus data
typedef struct {
    Vec3 position;           // World position of sound source
    f32 volume;              // Sound volume (0.0 - 1.0)
    f32 frequency;           // Dominant frequency (Hz)
    f32 pitch;              // Sound pitch
    char sound_type[64];     // Type of sound (footstep, voice, explosion, etc.)
    f32 attenuation;         // Distance-based attenuation
    f64 duration;            // Sound duration
    bool is_looping;         // Whether sound loops
    f32 doppler_shift;       // Doppler effect shift
    bool is_direct;          // Direct line of sight to source
} AuditoryStimulus;

// Olfactory stimulus data
typedef struct {
    Vec3 position;           // World position of odor source
    f32 intensity;           // Odor intensity (0.0 - 1.0)
    char odor_type[64];       // Type of odor (blood, food, smoke, etc.)
    f32 spread_radius;        // How far the odor spreads
    f64 persistence_time;     // How long odor persists
    f32 wind_effect;         // Wind effect on odor spread
    Vec3 wind_direction;      // Wind direction
    f32 concentration;       // Current concentration at agent location
} OlfactoryStimulus;

// Tactile stimulus data
typedef struct {
    Vec3 contact_point;      // Point of contact
    Vec3 force_direction;    // Direction of force
    f32 force_magnitude;      // Magnitude of force
    f32 pressure;            // Pressure at contact point
    char texture_type[64];    // Texture type (rough, smooth, hot, cold, etc.)
    f32 temperature;         // Temperature at contact point
    f64 duration;            // Contact duration
    bool is_painful;        // Whether contact causes pain
} TactileStimulus;

// Combined sensory input
struct SensoryInput {
    StimulusType type;        // Type of stimulus
    EntityID source_entity;    // Entity that generated stimulus
    f64 timestamp;           // When stimulus occurred
    Vec3 position;           // Stimulus position
    f32 base_intensity;      // Base intensity (0.0 - 1.0)
    
    // Type-specific data
    union {
        VisualStimulus visual;
        AuditoryStimulus auditory;
        OlfactoryStimulus olfactory;
        TactileStimulus tactile;
    } data;
    
    // Processing metadata
    f32 processed_intensity; // Final intensity after processing
    f32 confidence;          // Confidence in stimulus detection
    bool is_valid;           // Whether stimulus is valid
};

// Perceived entity information
struct PerceivedEntity {
    EntityID entity_id;        // Unique entity identifier
    char name[128];            // Entity name
    Vec3 last_known_position;  // Last known position
    Vec3 velocity;             // Estimated velocity
    f64 last_seen_time;        // When entity was last seen
    f64 last_heard_time;       // When entity was last heard
    f64 last_felt_time;       // When entity was last felt
    f64 last_smelled_time;     // When entity was last smelled
    f32 visual_confidence;     // Confidence in visual perception
    f32 auditory_confidence;   // Confidence in auditory perception
    f32 olfactory_confidence;   // Confidence in olfactory perception
    f32 tactile_confidence;     // Confidence in tactile perception
    f32 overall_confidence;    // Combined confidence
    bool is_visible;            // Currently visible
    bool is_audible;            // Currently audible
    bool is_detectable;        // Currently detectable by any sense
    u32 detection_flags;        // Bit flags for detected senses
    f64 last_update_time;      // When this data was last updated
};

// Agent sensory configuration
typedef struct {
    // Visual sensor configuration
    struct {
        f32 fov_horizontal;      // Horizontal field of view (degrees)
        f32 fov_vertical;        // Vertical field of view (degrees)
        f32 max_distance;        // Maximum visual range
        f32 min_brightness;      // Minimum brightness threshold
        f32 motion_sensitivity;   // Motion detection sensitivity
        bool color_vision;        // Whether agent can see color
        f32 resolution;          // Visual resolution (pixels per degree)
        f32 occlusion_handling;   // How well agent handles occlusion
    } visual;
    
    // Auditory sensor configuration
    struct {
        f32 max_distance;        // Maximum hearing range
        f32 min_volume;          // Minimum audible volume
        f32 frequency_range[2];  // Min/max frequency range (Hz)
        f32 direction_accuracy;    // Direction accuracy (degrees)
        bool can_localize;       // Whether agent can localize sounds
        f32 background_noise;     // Background noise level
    } auditory;
    
    // Olfactory sensor configuration
    struct {
        f32 max_distance;        // Maximum smell range
        f32 min_concentration;   // Minimum detectable concentration
        f32 sensitivity;         // Smell sensitivity
        char preferred_odors[8][32]; // Preferred odor types
        u32 preferred_count;     // Number of preferred odors
    } olfactory;
    
    // Tactile sensor configuration
    struct {
        f32 max_distance;        // Maximum touch range
        f32 min_pressure;         // Minimum detectable pressure
        f32 temperature_sensitivity; // Temperature sensitivity
        f32 vibration_sensitivity; // Vibration sensitivity
        bool pain_response;       // Whether agent responds to pain
    } tactile;
    
    // General perception settings
    struct {
        f32 alertness_level;      // Alertness (0.0 = relaxed, 1.0 = high alert)
        f32 attention_focus;      // Current attention focus (0.0 - 1.0)
        f32 fatigue_level;        // Fatigue level (0.0 = fresh, 1.0 = exhausted)
        bool spatial_partitioning;  // Use spatial partitioning for optimization
        f32 update_frequency;     // How often to update perception (Hz)
    } general;
} SensoryConfig;

// Perception system configuration
typedef struct {
    u32 max_agents;             // Maximum number of perceptual agents
    u32 max_stimuli_per_frame; // Maximum stimuli to process per frame
    u32 max_perceived_entities;  // Maximum entities to track per agent
    f32 spatial_grid_size;       // Spatial partitioning grid size
    bool enable_occlusion;        // Enable occlusion testing
    bool enable_doppler;          // Enable Doppler effect
    bool enable_wind_effects;       // Enable wind effects on odors
    f64 memory_decay_time;         // Time for perceived entity memory decay
    bool debug_mode;               // Enable debug visualization
} PerceptionSystemConfig;

// Main Perception System API
PerceptionSystem* perception_system_create(const PerceptionSystemConfig* config);
void perception_system_destroy(PerceptionSystem* system);
bool perception_system_initialize(PerceptionSystem* system);
void perception_system_shutdown(PerceptionSystem* system);

// Agent management
PerceptualAgent* perception_system_add_agent(PerceptionSystem* system, EntityID entity_id, 
                                               const SensoryConfig* config);
bool perception_system_remove_agent(PerceptionSystem* system, EntityID entity_id);
PerceptualAgent* perception_system_get_agent(PerceptionSystem* system, EntityID entity_id);
void perception_system_update_agent_config(PerceptionSystem* system, EntityID entity_id, 
                                       const SensoryConfig* config);

// Stimulus processing
void perception_system_add_stimulus(PerceptionSystem* system, const SensoryInput* stimulus);
void perception_system_process_frame(PerceptionSystem* system, f64 delta_time);
void perception_system_update_spatial_partition(PerceptionSystem* system);

// Perception queries
PerceivedEntity* perception_system_get_perceived_entities(PerceptionSystem* system, 
                                                     EntityID agent_id, u32* count);
PerceivedEntity* perception_system_get_entity_by_id(PerceptionSystem* system, 
                                                   EntityID agent_id, EntityID entity_id);
bool perception_system_can_see_entity(PerceptionSystem* system, EntityID observer_id, 
                                   EntityID target_id);
bool perception_system_can_hear_entity(PerceptionSystem* system, EntityID observer_id, 
                                    EntityID target_id);
f32 perception_system_get_detection_confidence(PerceptionSystem* system, EntityID observer_id, 
                                            EntityID target_id, StimulusType sense);

// Spatial queries
PerceivedEntity* perception_system_get_entities_in_radius(PerceptionSystem* system, 
                                                       EntityID agent_id, const Vec3* center, 
                                                       f32 radius, u32* count);
PerceivedEntity* perception_system_get_entities_in_fov(PerceptionSystem* system, 
                                                    EntityID agent_id, u32* count);
PerceivedEntity* perception_system_get_visible_entities(PerceptionSystem* system, 
                                                     EntityID agent_id, u32* count);

// Memory and learning
void perception_system_update_agent_memory(PerceptionSystem* system, EntityID agent_id, 
                                        EntityID target_id, StimulusType sense, 
                                        f32 confidence);
void perception_system_decay_memories(PerceptionSystem* system, f64 delta_time);
void perception_system_learn_from_experience(PerceptionSystem* system, EntityID agent_id, 
                                         const SensoryInput* stimulus, 
                                         f32 outcome_confidence);

// Debug and visualization
void perception_system_set_debug_mode(PerceptionSystem* system, bool enabled);
void perception_system_draw_debug_info(PerceptionSystem* system, EntityID agent_id);
void perception_system_print_statistics(PerceptionSystem* system);

// Utility functions
const char* stimulus_type_to_string(StimulusType type);
f32 perception_system_calculate_visibility(const Vec3* observer_pos, const Vec3* target_pos, 
                                       const Vec3* observer_forward, f32 fov, f32 max_range);
f32 perception_system_calculate_audibility(const Vec3* observer_pos, const Vec3* source_pos, 
                                        f32 volume, f32 max_range);
bool perception_system_line_of_sight_test(const Vec3* start, const Vec3* end);

// Advanced perception features
void perception_system_enable_alertness_mode(PerceptionSystem* system, EntityID agent_id, f32 level);
void perception_system_set_attention_focus(PerceptionSystem* system, EntityID agent_id, 
                                      const Vec3* focus_point);
void perception_system_apply_fatigue_effects(PerceptionSystem* system, EntityID agent_id, f32 fatigue);

#ifdef __cplusplus
}
#endif

#endif // PERCEPTION_SYSTEM_H
