#ifndef AUDIO_SYNTHESIS_H
#define AUDIO_SYNTHESIS_H

#include <common.h>
#include <math/math.h>

// -------------------------------------------------------------------------------------------------
// WAVEFORM GENERATORS
// -------------------------------------------------------------------------------------------------

typedef enum {
    WAVEFORM_SINE,
    WAVEFORM_SQUARE,
    WAVEFORM_SAWTOOTH,
    WAVEFORM_TRIANGLE,
    WAVEFORM_NOISE,
    WAVEFORM_PULSE
} WaveformType;

typedef struct {
    WaveformType type;
    f32 frequency;
    f32 amplitude;
    f32 phase;
    f32 phase_increment;
    f32 duty_cycle;  // For pulse waves
    u32 sample_rate;
} WaveformGenerator;

// -------------------------------------------------------------------------------------------------
// ENVELOPE GENERATORS
// -------------------------------------------------------------------------------------------------

typedef enum {
    ENVELOPE_ADSR,
    ENVELOPE_ASR,
    ENVELOPE_AR
} EnvelopeType;

typedef struct {
    EnvelopeType type;
    f32 attack_time;
    f32 decay_time;
    f32 sustain_level;
    f32 release_time;
    f32 current_level;
    u32 state;  // 0=off, 1=attack, 2=decay, 3=sustain, 4=release
    u32 samples_in_state;
    u32 sample_rate;
} ADSREnvelope;

// -------------------------------------------------------------------------------------------------
// FILTERS
// -------------------------------------------------------------------------------------------------

typedef enum {
    FILTER_LOWPASS,
    FILTER_HIGHPASS,
    FILTER_BANDPASS,
    FILTER_NOTCH,
    FILTER_ALLPASS
} FilterType;

typedef struct {
    FilterType type;
    f32 cutoff_frequency;
    f32 resonance;
    f32 q_factor;
    f32 sample_rate;
    
    // Filter state variables
    f32 x1, x2;  // Input history
    f32 y1, y2;  // Output history
    
    // Filter coefficients
    f32 b0, b1, b2;
    f32 a1, a2;
} BiquadFilter;

// -------------------------------------------------------------------------------------------------
// MODULAR SYNTHESIS NODES
// -------------------------------------------------------------------------------------------------

typedef enum {
    NODE_OSCILLATOR,
    NODE_ENVELOPE,
    NODE_FILTER,
    NODE_MIXER,
    NODE_LFO,
    NODE_DELAY,
    NODE_REVERB,
    NODE_GAIN,
    NODE_INPUT,
    NODE_OUTPUT
} NodeType;

typedef struct SynthNode SynthNode;

typedef struct {
    SynthNode** inputs;
    u32 input_count;
    f32* input_gains;
    f32 output;
    void* parameters;
} NodeConnection;

struct SynthNode {
    NodeType type;
    NodeConnection connections;
    void (*process)(SynthNode* node, f32* output, u32 samples);
    void* node_data;
    bool active;
};

// -------------------------------------------------------------------------------------------------
// SYNTHESIS ENGINE
// -------------------------------------------------------------------------------------------------

typedef struct {
    SynthNode* nodes;
    u32 max_nodes;
    u32 active_nodes;
    u32 sample_rate;
    u32 buffer_size;
    f32* temp_buffer;
    bool initialized;
} SynthesisEngine;

// -------------------------------------------------------------------------------------------------
// FUNCTION DECLARATIONS
// -------------------------------------------------------------------------------------------------

// Waveform generators
void Waveform_Init(WaveformGenerator* gen, WaveformType type, f32 frequency, f32 amplitude, u32 sample_rate);
f32 Waveform_Next(WaveformGenerator* gen);
void Waveform_SetFrequency(WaveformGenerator* gen, f32 frequency);
void Waveform_SetAmplitude(WaveformGenerator* gen, f32 amplitude);
void Waveform_SetDutyCycle(WaveformGenerator* gen, f32 duty_cycle);

// Envelope generators
void ADSR_Init(ADSREnvelope* env, f32 attack, f32 decay, f32 sustain, f32 release, u32 sample_rate);
void ADSR_Trigger(ADSREnvelope* env);
void ADSR_Release(ADSREnvelope* env);
f32 ADSR_Next(ADSREnvelope* env);
bool ADSR_IsActive(const ADSREnvelope* env);

// Filters
void Filter_Init(BiquadFilter* filter, FilterType type, f32 cutoff, f32 resonance, u32 sample_rate);
void Filter_SetCutoff(BiquadFilter* filter, f32 cutoff);
void Filter_SetResonance(BiquadFilter* filter, f32 resonance);
f32 Filter_Process(BiquadFilter* filter, f32 input);
void Filter_CalculateCoefficients(BiquadFilter* filter);

// Modular synthesis
bool SynthEngine_Init(SynthesisEngine* engine, u32 max_nodes, u32 sample_rate, u32 buffer_size);
SynthNode* SynthEngine_CreateNode(SynthesisEngine* engine, NodeType type);
void SynthEngine_DestroyNode(SynthesisEngine* engine, SynthNode* node);
void SynthEngine_Connect(SynthesisEngine* engine, SynthNode* source, SynthNode* destination, f32 gain);
void SynthEngine_Process(SynthesisEngine* engine, f32* output, u32 samples);
void SynthEngine_Shutdown(SynthesisEngine* engine);

// Node creation functions
SynthNode* CreateOscillatorNode(SynthesisEngine* engine, WaveformType type, f32 frequency, f32 amplitude);
SynthNode* CreateEnvelopeNode(SynthesisEngine* engine, f32 attack, f32 decay, f32 sustain, f32 release);
SynthNode* CreateFilterNode(SynthesisEngine* engine, FilterType type, f32 cutoff, f32 resonance);
SynthNode* CreateMixerNode(SynthesisEngine* engine, u32 input_count);
SynthNode* CreateGainNode(SynthesisEngine* engine, f32 gain);

// Utility functions
void Audio_CreateSynth(void);
f32 GenerateNoise(f32 amplitude);
f32 GeneratePulseWave(f32 phase, f32 duty_cycle);
f32 GenerateSawtoothWave(f32 phase);
f32 GenerateTriangleWave(f32 phase);

#endif // AUDIO_SYNTHESIS_H
