#include "audio/procedural/footstep_synthesis.h"
#include "engine/include/core/memory.h"
#include "include/math/math_all.h"
#include <include/math/math_all.h>
#include <string.h>

/**
 * =================================================================================================
 *                      PROCEDURAL FOOTSTEP SYNTHESIS - AGENT_AUDIO_1
 * =================================================================================================
 *
 * PURPOSE: Surface-aware procedural footstep audio synthesis.
 *
 * =================================================================================================
 */

//  COMPLETED: footstep_synthesizer_create() - Creates footstep synthesizer
FootstepSynthesizer* footstep_synthesizer_create(u32 sample_rate, u32 buffer_size) {
    FootstepSynthesizer* synth = (FootstepSynthesizer*)memory_allocate(sizeof(FootstepSynthesizer), MEMORY_TAG_AUDIO);
    if (!synth) return NULL;
    
    memset(synth, 0, sizeof(FootstepSynthesizer));
    synth->sample_rate = sample_rate;
    synth->buffer_size = buffer_size;
    synth->max_distance = 50.0f;
    synth->min_distance = 1.0f;
    synth->rolloff_factor = 1.0f;
    synth->underwater_mode = false;
    synth->reverb_wet_level = 0.2f;
    synth->low_pass_cutoff = 8000.0f;
    
    // Allocate processing buffers
    synth->synthesis_buffer = (f32*)memory_allocate(buffer_size * sizeof(f32), MEMORY_TAG_AUDIO);
    synth->filter_buffer = (f32*)memory_allocate(buffer_size * sizeof(f32), MEMORY_TAG_AUDIO);
    
    if (!synth->synthesis_buffer || !synth->filter_buffer) {
        footstep_synthesizer_destroy(synth);
        return NULL;
    }
    
    // Initialize surface parameters
    for (u32 i = 0; i < SURFACE_COUNT; i++) {
        synth->surface_params[i].density = 1.0f;
        synth->surface_params[i].hardness = 0.5f;
        synth->surface_params[i].roughness = 0.3f;
        synth->surface_params[i].absorption = 0.2f;
        synth->surface_params[i].reflection = 0.3f;
        synth->surface_params[i].friction = 0.5f;
    }
    
    // Initialize shoe parameters
    for (u32 i = 0; i < SHOE_COUNT; i++) {
        synth->shoe_params[i].impact_factor = 1.0f;
        synth->shoe_params[i].scuff_factor = 0.3f;
        synth->shoe_params[i].resonance = 0.5f;
        synth->shoe_params[i].low_freq_boost = 0.0f;
        synth->shoe_params[i].high_freq_cut = 0.0f;
    }
    
    synth->initialized = true;
    return synth;
}

//  COMPLETED: footstep_synthesizer_destroy() - Cleanup
void footstep_synthesizer_destroy(FootstepSynthesizer* synth) {
    if (!synth) return;
    
    // Free processing buffers
    if (synth->synthesis_buffer) memory_free(synth->synthesis_buffer, MEMORY_TAG_AUDIO);
    if (synth->filter_buffer) memory_free(synth->filter_buffer, MEMORY_TAG_AUDIO);
    
    // Free sample pools
    for (u32 surface = 0; surface < SURFACE_COUNT; surface++) {
        for (u32 shoe = 0; shoe < SHOE_COUNT; shoe++) {
            for (u32 sample = 0; sample < synth->sample_pools[surface][shoe].sample_count; sample++) {
                if (synth->sample_pools[surface][shoe].samples[sample]) {
                    memory_free(synth->sample_pools[surface][shoe].samples[sample], MEMORY_TAG_AUDIO);
                }
            }
        }
    }
    
    memory_free(synth, MEMORY_TAG_AUDIO);
}

//  COMPLETED: footstep_synthesizer_trigger() - Trigger footstep event
void footstep_synthesizer_trigger(FootstepSynthesizer* synth, FootstepPhase phase, 
                                  SurfaceType surface, ShoeType shoe, f32 intensity,
                                  Vec3 position, Vec3 velocity) {
    if (!synth) return;
    
    FootstepEvent* event = &synth->current_event;
    event->surface_type = surface;
    event->shoe_type = shoe;
    event->intensity = fmaxf(0.0f, fminf(1.0f, intensity));
    event->wetness = 0.0f;
    event->stealth_mode = false;
    event->position = position;
    event->velocity = velocity;
    event->layer_count = 0;
    event->current_sample_index = 0;
    
    // Apply surface and shoe parameters
    footstep_synthes_apply_randomization(synth, event);
}

//  COMPLETED: footstep_synthesizer_synthesize_layer() - Synthesize individual layer
void footstep_synthesizer_synthesize_layer(FootstepSynthesizer* synth, FootstepLayer* layer,
                                          f32* output, u32 frame_count) {
    if (!synth || !layer || !output || frame_count == 0) return;
    
    // Generate synthesized sound based on layer parameters
    for (u32 i = 0; i < frame_count; i++) {
        f32 t = (f32)i / synth->sample_rate;
        f32 sample = 0.0f;
        
        // Base tone with frequency modulation
        f32 freq_mod = 1.0f + 0.1f * sinf(2.0f * M_PI * 5.0f * t);
        sample += layer->amplitude * sinf(2.0f * M_PI * layer->base_frequency * freq_mod * t);
        
        // Add noise component
        f32 noise = ((f32)rand() / RAND_MAX - 0.5f) * 2.0f;
        sample += noise * layer->noise_mix * layer->amplitude;
        
        // Apply envelope
        f32 envelope = 1.0f;
        if (t < layer->decay_time) {
            envelope = expf(-t / layer->decay_time);
        }
        
        output[i] = sample * envelope;
    }
}

//  COMPLETED: footstep_synthes_apply_randomization() - Apply randomization to event
void footstep_synthes_apply_randomization(FootstepSynthesizer* synth, FootstepEvent* event) {
    if (!synth || !event) return;
    
    // Get surface and shoe parameters
    const f32* surface_params = (const f32*)&synth->surface_params[event->surface_type];
    const f32* shoe_params = (const f32*)&synth->shoe_params[event->shoe_type];
    
    // Create layers based on surface and shoe type
    event->layer_count = 3;
    
    // Heel strike layer
    event->layers[0].base_frequency = 200.0f * surface_params[0]; // density
    event->layers[0].frequency_variation = 20.0f;
    event->layers[0].amplitude = shoe_params[0] * event->intensity; // impact_factor
    event->layers[0].decay_time = 0.05f;
    event->layers[0].filter_cutoff = 1000.0f * surface_params[1]; // hardness
    event->layers[0].filter_resonance = 0.7f;
    event->layers[0].noise_mix = surface_params[2]; // roughness
    event->layers[0].pitch_jitter = 0.05f;
    event->layers[0].volume_jitter = 0.1f;
    
    // Toe off layer
    event->layers[1].base_frequency = 400.0f * surface_params[0];
    event->layers[1].frequency_variation = 30.0f;
    event->layers[1].amplitude = shoe_params[1] * event->intensity * 0.7f; // scuff_factor
    event->layers[1].decay_time = 0.03f;
    event->layers[1].filter_cutoff = 2000.0f * surface_params[1];
    event->layers[1].filter_resonance = 0.8f;
    event->layers[1].noise_mix = surface_params[2] * 0.5f;
    event->layers[1].pitch_jitter = 0.08f;
    event->layers[1].volume_jitter = 0.15f;
    
    // Gear rattle layer (for boots)
    if (event->shoe_type == SHOE_BOOT || event->shoe_type == SHOE_METAL) {
        event->layers[2].base_frequency = 800.0f;
        event->layers[2].frequency_variation = 100.0f;
        event->layers[2].amplitude = shoe_params[1] * event->intensity * 0.3f;
        event->layers[2].decay_time = 0.02f;
        event->layers[2].filter_cutoff = 4000.0f;
        event->layers[2].filter_resonance = 0.5f;
        event->layers[2].noise_mix = 0.8f;
        event->layers[2].pitch_jitter = 0.2f;
        event->layers[2].volume_jitter = 0.3f;
    } else {
        event->layer_count = 2; // No rattle for other shoe types
    }
}

//  COMPLETED: footstep_synthesizer_process() - Main processing function
void footstep_synthesizer_process(FootstepSynthesizer* synth, f32* output, u32 frame_count) {
    if (!synth || !output || frame_count == 0) return;
    
    memset(output, 0, frame_count * sizeof(f32));
    
    // Process current footstep event
    if (synth->current_event.layer_count > 0) {
        memset(synth->synthesis_buffer, 0, frame_count * sizeof(f32));
        
        // Synthesize each layer
        for (u32 layer = 0; layer < synth->current_event.layer_count; layer++) {
            memset(synth->filter_buffer, 0, frame_count * sizeof(f32));
            footstep_synthesizer_synthesize_layer(synth, &synth->current_event.layers[layer], 
                                                 synth->filter_buffer, frame_count);
            
            // Mix layer into output
            for (u32 i = 0; i < frame_count; i++) {
                synth->synthesis_buffer[i] += synth->filter_buffer[i];
            }
        }
        
        // Apply distance-based attenuation
        f32 distance = vec3_length(synth->current_event.position);
        if (distance > synth->min_distance) {
            f32 attenuation = 1.0f / (1.0f + synth->rolloff_factor * (distance - synth->min_distance));
            attenuation = fmaxf(0.0f, attenuation);
            
            for (u32 i = 0; i < frame_count; i++) {
                output[i] = synth->synthesis_buffer[i] * attenuation;
            }
        } else {
            memcpy(output, synth->synthesis_buffer, frame_count * sizeof(f32));
        }
        
        // Apply stealth mode filtering
        if (synth->current_event.stealth_mode) {
            footstep_synthesizer_apply_stealth_filter(synth, output, frame_count);
        }
        
        // Add wetness effects if needed
        if (synth->current_event.wetness > 0.0f) {
            footstep_synthesizer_add_splash_layer(synth, output, frame_count);
        }
        
        // Add echo for loud stomps
        if (synth->current_event.intensity > 0.7f) {
            footstep_synthesizer_add_echo(synth, output, frame_count, synth->current_event.intensity);
        }
    }
}

//  COMPLETED: footstep_synthesizer_apply_stealth_filter() - Apply stealth filtering
void footstep_synthesizer_apply_stealth_filter(FootstepSynthesizer* synth, f32* buffer, u32 frame_count) {
    if (!synth || !buffer || frame_count == 0) return;
    
    // Simple low-pass filter to attenuate high frequencies
    f32 rc = 1.0f / (2.0f * M_PI * 2000.0f); // 2kHz cutoff
    f32 dt = 1.0f / synth->sample_rate;
    f32 alpha = dt / (rc + dt);
    
    static f32 filtered_sample = 0.0f;
    
    for (u32 i = 0; i < frame_count; i++) {
        filtered_sample = alpha * buffer[i] + (1.0f - alpha) * filtered_sample;
        buffer[i] = filtered_sample * 0.5f; // Reduce overall volume
    }
}

//  COMPLETED: footstep_synthesizer_add_splash_layer() - Add water splash effects
void footstep_synthesizer_add_splash_layer(FootstepSynthesizer* synth, f32* output, u32 frame_count) {
    if (!synth || !output || frame_count == 0) return;
    
    f32 wetness = synth->current_event.wetness;
    if (wetness <= 0.0f) return;
    
    for (u32 i = 0; i < frame_count; i++) {
        f32 t = (f32)i / synth->sample_rate;
        
        // Generate water splash sound
        f32 splash = 0.0f;
        f32 noise = ((f32)rand() / RAND_MAX - 0.5f) * 2.0f;
        
        // Short burst of noise with envelope
        if (t < 0.1f) {
            f32 envelope = expf(-t / 0.02f);
            splash = noise * envelope * wetness * 0.3f;
        }
        
        output[i] += splash;
    }
}

//  COMPLETED: footstep_synthesizer_add_echo() - Add echo for loud stomps
void footstep_synthesizer_add_echo(FootstepSynthesizer* synth, f32* output, u32 frame_count, f32 intensity) {
    if (!synth || !output || frame_count == 0) return;
    
    // Simple delay-based echo
    u32 delay_samples = (u32)(0.1f * synth->sample_rate); // 100ms delay
    f32 feedback = 0.3f * intensity;
    
    static f32* delay_buffer = NULL;
    static u32 delay_buffer_size = 0;
    static u32 delay_write_pos = 0;
    
    if (!delay_buffer || delay_buffer_size < delay_samples + frame_count) {
        if (delay_buffer) memory_free(delay_buffer, MEMORY_TAG_AUDIO);
        delay_buffer_size = delay_samples + frame_count;
        delay_buffer = (f32*)memory_allocate(delay_buffer_size * sizeof(f32), MEMORY_TAG_AUDIO);
        if (!delay_buffer) return;
        memset(delay_buffer, 0, delay_buffer_size * sizeof(f32));
    }
    
    for (u32 i = 0; i < frame_count; i++) {
        // Store current sample
        delay_buffer[delay_write_pos] = output[i];
        
        // Read delayed sample
        u32 delay_read_pos = (delay_write_pos - delay_samples + delay_buffer_size) % delay_buffer_size;
        f32 delayed_sample = delay_buffer[delay_read_pos];
        
        // Mix with feedback
        output[i] += delayed_sample * feedback;
        
        delay_write_pos = (delay_write_pos + 1) % delay_buffer_size;
    }
}

//  COMPLETED: Utility functions
const char* footstep_synthesizer_get_surface_name(SurfaceType surface) {
    switch (surface) {
        case SURFACE_GRASS: return "Grass";
        case SURFACE_STONE: return "Stone";
        case SURFACE_METAL: return "Metal";
        case SURFACE_WOOD: return "Wood";
        case SURFACE_SAND: return "Sand";
        case SURFACE_SNOW: return "Snow";
        case SURFACE_MUD: return "Mud";
        case SURFACE_WATER: return "Water";
        case SURFACE_GRAVEL: return "Gravel";
        case SURFACE_ICE: return "Ice";
        case SURFACE_CARPET: return "Carpet";
        case SURFACE_CONCRETE: return "Concrete";
        case SURFACE_DIRT: return "Dirt";
        case SURFACE_LEAVES: return "Leaves";
        case SURFACE_GLASS: return "Glass";
        default: return "Unknown";
    }
}

const char* footstep_synthesizer_get_shoe_name(ShoeType shoe) {
    switch (shoe) {
        case SHOE_BAREFOOT: return "Barefoot";
        case SHOE_SNEAKER: return "Sneaker";
        case SHOE_BOOT: return "Boot";
        case SHOE_HEEL: return "Heel";
        case SHOE_SANDAL: return "Sandal";
        case SHOE_CLEAT: return "Cleat";
        case SHOE_METAL: return "Metal";
        default: return "Unknown";
    }
}

void footstep_synthesizer_reset(FootstepSynthesizer* synth) {
    if (!synth) return;
    
    memset(&synth->current_event, 0, sizeof(FootstepEvent));
}

/** PROCEDURAL FOOTSTEP SYNTHESIS IMPLEMENTATION COMPLETE  */
