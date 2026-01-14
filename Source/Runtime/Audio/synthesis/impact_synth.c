/**
 * =================================================================================================
 *                      PHYSICALLY-MODELED IMPACT SOUND SYNTHESIS
 * =================================================================================================
 * 
 * PURPOSE: Generate impact sounds from physics events instead of playing samples
 * APPROACH: Modal synthesis - model objects as collections of resonant modes
 */

#include <include/math/math_all.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_MODES 32
#define SAMPLE_RATE 48000
#define MAX_ACTIVE_IMPACTS 64

// =================================================================================================
// MATERIAL PROPERTIES DATABASE
// =================================================================================================

typedef enum {
    MATERIAL_WOOD,
    MATERIAL_METAL,
    MATERIAL_STONE,
    MATERIAL_GLASS,
    MATERIAL_PLASTIC,
    MATERIAL_CLOTH,
    MATERIAL_COUNT
} MaterialType;

typedef struct {
    float density;           // kg/m
    float youngs_modulus;    // Stiffness (Pa)
    float damping;           // Energy loss per second
    float hardness;          // Surface hardness (affects contact time)
} MaterialProperties;

static MaterialProperties g_materials[MATERIAL_COUNT] = {
    // density, youngs_modulus, damping, hardness
    {600.0f,  11e9f,  0.3f, 0.5f},   // Wood
    {7800.0f, 200e9f, 0.05f, 0.9f},  // Metal (steel)
    {2400.0f, 50e9f,  0.1f, 0.8f},   // Stone
    {2500.0f, 70e9f,  0.01f, 0.95f}, // Glass
    {1200.0f, 3e9f,   0.2f, 0.6f},   // Plastic
    {300.0f,  0.5e9f, 0.9f, 0.1f}    // Cloth
};

// =================================================================================================
// MODAL SYNTHESIS
// =================================================================================================

typedef struct {
    float frequency;    // Mode frequency (Hz)
    float amplitude;    // Mode amplitude
    float damping;      // Decay rate
    float phase;        // Current phase
} ResonantMode;

typedef struct {
    ResonantMode modes[MAX_MODES];
    uint32_t mode_count;
    
    float excitation_time;
    float excitation_strength;
    bool active;
} ModalObject;

// =================================================================================================
// IMPACT EVENT
// =================================================================================================

typedef struct {
    MaterialType material_a;
    MaterialType material_b;
    
    float impact_velocity;  // m/s
    float contact_area;     // m
    float mass_a, mass_b;   // kg
    
    ModalObject resonator;
    float time_active;
    float duration;
} ImpactSound;

// =================================================================================================
// SYNTHESIS ENGINE
// =================================================================================================

typedef struct {
    ImpactSound impacts[MAX_ACTIVE_IMPACTS];
    uint32_t active_count;
    
    float* output_buffer;
    uint32_t buffer_size;
    uint32_t write_pos;
} SynthesisEngine;

// =================================================================================================
// INITIALIZATION
// =================================================================================================

SynthesisEngine* synthesis_create(uint32_t buffer_size) {
    SynthesisEngine* synth = (SynthesisEngine*)calloc(1, sizeof(SynthesisEngine));
    
    synth->buffer_size = buffer_size;
    synth->output_buffer = (float*)calloc(buffer_size, sizeof(float));
    
    return synth;
}

void synthesis_destroy(SynthesisEngine* synth) {
    if (synth) {
        free(synth->output_buffer);
        free(synth);
    }
}

// =================================================================================================
// MODE GENERATION FROM MATERIAL PROPERTIES
// =================================================================================================

static void generate_modes_for_material(ModalObject* obj, MaterialType material, 
                                       float object_size, float impact_force) {
    MaterialProperties* props = &g_materials[material];
    
    // Calculate fundamental frequency based on size and material
    // f = (v / 2L) where v = sqrt(E/), E = Young's modulus,  = density
    float wave_speed = sqrtf(props->youngs_modulus / props->density);
    float fundamental = wave_speed / (2.0f * object_size);
    
    // Generate harmonic modes
    obj->mode_count = 0;
    for (uint32_t i = 1; i <= MAX_MODES && obj->mode_count < MAX_MODES; i++) {
        ResonantMode* mode = &obj->modes[obj->mode_count++];
        
        mode->frequency = fundamental * i;
        
        // Amplitude falls off with frequency
        float energy_fraction = impact_force / (float)i;
        mode->amplitude = energy_fraction * (1.0f / sqrtf(i));
        
        // Higher modes decay faster
        mode->damping = props->damping * (1.0f + 0.5f * i);
        
        mode->phase = 0.0f;
        
        // Stop when amplitude becomes negligible
        if (mode->amplitude < 0.001f) break;
    }
}

// =================================================================================================
// IMPACT TRIGGERING
// =================================================================================================

void synthesis_trigger_impact(SynthesisEngine* synth, MaterialType mat_a, MaterialType mat_b,
                              float velocity, float object_size) {
    if (synth->active_count >= MAX_ACTIVE_IMPACTS) return;
    
    ImpactSound* impact = &synth->impacts[synth->active_count++];
    
    impact->material_a = mat_a;
    impact->material_b = mat_b;
    impact->impact_velocity = velocity;
    
    // Calculate impact force (simplified)
    MaterialProperties* props_a = &g_materials[mat_a];
    MaterialProperties* props_b = &g_materials[mat_b];
    
    float combined_hardness = (props_a->hardness + props_b->hardness) * 0.5f;
    float impact_force = velocity * combined_hardness * 100.0f;
    
    // Generate resonant modes for the primary object
    generate_modes_for_material(&impact->resonator, mat_a, object_size, impact_force);
    
    impact->resonator.excitation_strength = impact_force;
    impact->resonator.excitation_time = 0.0f;
    impact->resonator.active = true;
    
    impact->time_active = 0.0f;
    impact->duration = 2.0f; // 2 seconds max
}

// =================================================================================================
// AUDIO SYNTHESIS
// =================================================================================================

static float synthesize_modal_object(ModalObject* obj, float dt) {
    float sample = 0.0f;
    
    // Excitation envelope (sharp attack, exponential decay)
    float excitation = obj->excitation_strength * expf(-obj->excitation_time * 30.0f);
    obj->excitation_time += dt;
    
    // Sum all modes
    for (uint32_t i = 0; i < obj->mode_count; i++) {
        ResonantMode* mode = &obj->modes[i];
        
        // Damped sinusoid
        float decay = expf(-mode->damping * obj->excitation_time);
        float oscillation = sinf(mode->phase);
        
        sample += mode->amplitude * decay * oscillation * excitation;
        
        // Update phase
        mode->phase += 2.0f * M_PI * mode->frequency * dt;
        if (mode->phase > 2.0f * M_PI) {
            mode->phase -= 2.0f * M_PI;
        }
    }
    
    return sample;
}

void synthesis_process(SynthesisEngine* synth, float dt) {
    // Clear buffer
    memset(synth->output_buffer, 0, synth->buffer_size * sizeof(float));
    
    // Process all active impacts
    for (uint32_t i = 0; i < synth->active_count; i++) {
        ImpactSound* impact = &synth->impacts[i];
        
        if (!impact->resonator.active) continue;
        
        // Generate samples
        uint32_t samples_to_generate = (uint32_t)(dt * SAMPLE_RATE);
        float sample_dt = 1.0f / (float)SAMPLE_RATE;
        
        for (uint32_t s = 0; s < samples_to_generate && s < synth->buffer_size; s++) {
            float sample = synthesize_modal_object(&impact->resonator, sample_dt);
            synth->output_buffer[s] += sample * 0.3f; // Mix with attenuation
        }
        
        impact->time_active += dt;
        
        // Deactivate finished impacts
        if (impact->time_active > impact->duration) {
            impact->resonator.active = false;
            
            // Remove from active list (swap with last)
            synth->impacts[i] = synth->impacts[synth->active_count - 1];
            synth->active_count--;
            i--;
        }
    }
}

// =================================================================================================
// HELPER: Physics Integration Hook
// =================================================================================================

void synthesis_on_collision(SynthesisEngine* synth, void* body_a, void* body_b, 
                            float contact_velocity[3]) {
    // Extract material types from physics bodies
    // For now, use placeholder materials
    MaterialType mat_a = MATERIAL_WOOD;
    MaterialType mat_b = MATERIAL_STONE;
    
    float velocity_mag = sqrtf(
        contact_velocity[0] * contact_velocity[0] +
        contact_velocity[1] * contact_velocity[1] +
        contact_velocity[2] * contact_velocity[2]
    );
    
    // Only synthesize for significant impacts
    if (velocity_mag > 0.5f) {
        synthesis_trigger_impact(synth, mat_a, mat_b, velocity_mag, 0.2f);
    }
}

// =================================================================================================
// OUTPUT
// =================================================================================================

float* synthesis_get_output_buffer(SynthesisEngine* synth) {
    return synth->output_buffer;
}

uint32_t synthesis_get_buffer_size(SynthesisEngine* synth) {
    return synth->buffer_size;
}
