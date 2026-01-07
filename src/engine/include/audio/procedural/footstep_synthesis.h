#ifndef AUDIO_PROCEDURAL_FOOTSTEP_H
#define AUDIO_PROCEDURAL_FOOTSTEP_H

#include "include/common.h"
#include <include/math/math.h>

#define MAX_FOOTSTEP_LAYERS 4
#define MAX_SURFACE_TYPES 16
#define MAX_SHOE_TYPES 8
#define MAX_FOOTSTEP_SAMPLES 32

typedef enum {
    SURFACE_GRASS,
    SURFACE_STONE,
    SURFACE_METAL,
    SURFACE_WOOD,
    SURFACE_SAND,
    SURFACE_SNOW,
    SURFACE_MUD,
    SURFACE_WATER,
    SURFACE_GRAVEL,
    SURFACE_ICE,
    SURFACE_CARPET,
    SURFACE_CONCRETE,
    SURFACE_DIRT,
    SURFACE_LEAVES,
    SURFACE_GLASS,
    SURFACE_COUNT
} SurfaceType;

typedef enum {
    SHOE_BAREFOOT,
    SHOE_SNEAKER,
    SHOE_BOOT,
    SHOE_HEEL,
    SHOE_SANDAL,
    SHOE_CLEAT,
    SHOE_METAL,
    SHOE_COUNT
} ShoeType;

typedef enum {
    FOOTSTEP_HEEL_STRIKE,
    FOOTSTEP_TOE_OFF,
    FOOTSTEP_MID_STANCE,
    FOOTSTEP_FULL_CONTACT
} FootstepPhase;

typedef struct {
    f32 base_frequency;
    f32 frequency_variation;  // Random variation in Hz
    f32 amplitude;
    f32 decay_time;
    f32 filter_cutoff;
    f32 filter_resonance;
    f32 noise_mix;          // Amount of noise component
    f32 pitch_jitter;       // Pitch variation percentage
    f32 volume_jitter;      // Volume variation percentage
} FootstepLayer;

typedef struct {
    SurfaceType surface_type;
    ShoeType shoe_type;
    f32 intensity;          // 0.0 (walk) to 1.0 (run)
    f32 wetness;            // 0.0 (dry) to 1.0 (underwater)
    bool stealth_mode;      // Crouching/stealth
    Vec3 position;          // World position
    Vec3 velocity;          // Movement velocity
    f32 distance_to_listener;
    
    // Generated layers
    FootstepLayer layers[MAX_FOOTSTEP_LAYERS];
    u32 layer_count;
    
    // Sample round-robin
    u32 sample_indices[MAX_FOOTSTEP_SAMPLES];
    u32 current_sample_index;
} FootstepEvent;

typedef struct {
    // Surface parameters
    struct {
        f32 density;           // Material density
        f32 hardness;          // Surface hardness
        f32 roughness;         // Surface roughness
        f32 absorption;        // Sound absorption
        f32 reflection;        // Sound reflection
        f32 friction;          // Friction coefficient
    } surface_params[SURFACE_COUNT];
    
    // Shoe type parameters
    struct {
        f32 impact_factor;     // How hard the shoe hits
        f32 scuff_factor;      // Scuffing/noise factor
        f32 resonance;         // Resonance characteristics
        f32 low_freq_boost;    // Low frequency emphasis
        f32 high_freq_cut;     // High frequency cutoff
    } shoe_params[SHOE_COUNT];
    
    // Material-specific synthesis parameters
    FootstepLayer templates[SURFACE_COUNT][SHOE_TYPE_COUNT][MAX_FOOTSTEP_LAYERS];
    u32 template_counts[SURFACE_COUNT][SHOE_TYPE_COUNT];
    
    // Sample pools for each surface/shoe combination
    struct {
        f32* samples[MAX_FOOTSTEP_SAMPLES];
        u32 sample_count;
        u32 sample_lengths[MAX_FOOTSTEP_SAMPLES];
        u32 current_sample;
    } sample_pools[SURFACE_COUNT][SHOE_COUNT];
    
    // Processing buffers
    f32* synthesis_buffer;
    f32* filter_buffer;
    u32 buffer_size;
    u32 sample_rate;
    
    // Distance-based attenuation
    f32 max_distance;
    f32 min_distance;
    f32 rolloff_factor;
    
    // Environmental effects
    bool underwater_mode;
    f32 reverb_wet_level;
    f32 low_pass_cutoff;
    
    bool initialized;
} FootstepSynthesizer;

// Footstep synthesizer lifecycle
FootstepSynthesizer* footstep_synthesizer_create(u32 sample_rate, u32 buffer_size);
void footstep_synthesizer_destroy(FootstepSynthesizer* synth);

// Event triggering
void footstep_synthesizer_trigger(FootstepSynthesizer* synth, FootstepPhase phase, 
                                  SurfaceType surface, ShoeType shoe, f32 intensity,
                                  Vec3 position, Vec3 velocity);

// Surface sensing
SurfaceType footstep_synthesizer_detect_surface(FootstepSynthesizer* synth, Vec3 position);
void footstep_synthesizer_set_surface_parameters(FootstepSynthesizer* synth, SurfaceType surface,
                                                 f32 density, f32 hardness, f32 roughness);

// Synthesis
void footstep_synthesizer_synthesize_layer(FootstepSynthesizer* synth, FootstepLayer* layer,
                                          f32* output, u32 frame_count);
void footstep_synthesizer_synthesize_event(FootstepSynthesizer* synth, FootstepEvent* event,
                                           f32* output, u32 frame_count);

// Randomization
void footstep_synthes_apply_randomization(FootstepSynthesizer* synth, FootstepEvent* event);
void footstep_synthesizer_round_robin_sample(FootstepSynthesizer* synth, SurfaceType surface, 
                                             ShoeType shoe);

// Shoe type modifiers
void footstep_synthesizer_set_shoe_parameters(FootstepSynthesizer* synth, ShoeType shoe,
                                             f32 impact, f32 scuff, f32 resonance,
                                             f32 low_boost, f32 high_cut);

// Intensity processing
void footstep_synthesizer_set_intensity(FootstepSynthesizer* synth, f32 intensity);
f32 footstep_synthesizer_calculate_intensity(FootstepSynthesizer* synth, Vec3 velocity);

// Wetness and environmental effects
void footstep_synthesizer_set_wetness(FootstepSynthesizer* synth, f32 wetness);
void footstep_synthesizer_add_splash_layer(FootstepSynthesizer* synth, f32* output, u32 frame_count);

// Stealth mode
void footstep_synthesizer_set_stealth_mode(FootstepSynthesizer* synth, bool enabled);
void footstep_synthesizer_apply_stealth_filter(FootstepSynthesizer* synth, f32* buffer, u32 frame_count);

// Distance-based processing
void footstep_synthesizer_update_distance(FootstepSynthesizer* synth, Vec3 listener_position);
bool footstep_synthesizer_should_process(FootstepSynthesizer* synth, Vec3 source_position);

// Echo/reverb for loud stomps
void footstep_synthesizer_add_echo(FootstepSynthesizer* synth, f32* output, u32 frame_count, f32 intensity);

// Sample management
void footstep_synthesizer_load_samples(FootstepSynthesizer* synth, SurfaceType surface, 
                                      ShoeType shoe, const char* sample_directory);
void footstep_synthesizer_add_sample(FootstepSynthesizer* synth, SurfaceType surface, 
                                     ShoeType shoe, const f32* sample, u32 length);

// Processing
void footstep_synthesizer_process(FootstepSynthesizer* synth, f32* output, u32 frame_count);
void footstep_synthesizer_process_stereo(FootstepSynthesizer* synth, f32* left, f32* right, u32 frame_count);

// Utilities
const char* footstep_synthesizer_get_surface_name(SurfaceType surface);
const char* footstep_synthesizer_get_shoe_name(ShoeType shoe);
void footstep_synthesizer_reset(FootstepSynthesizer* synth);

#endif // AUDIO_PROCEDURAL_FOOTSTEP_H
