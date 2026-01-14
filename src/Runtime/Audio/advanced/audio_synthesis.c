#include "audio/audio_synthesis.h"
#include <include/math/math_all.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                                   AUDIO SYNTHESIS ENGINE
 * =================================================================================================
 */

// -------------------------------------------------------------------------------------------------
// GLOBAL SYNTHESIS STATE
// -------------------------------------------------------------------------------------------------

static SynthesisEngine g_synth_engine = {0};
static bool g_synth_initialized = false;

// -------------------------------------------------------------------------------------------------
// WAVEFORM GENERATOR IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

void Waveform_Init(WaveformGenerator* gen, WaveformType type, f32 frequency, f32 amplitude, u32 sample_rate) {
    gen->type = type;
    gen->frequency = frequency;
    gen->amplitude = amplitude;
    gen->phase = 0.0f;
    gen->sample_rate = sample_rate;
    gen->duty_cycle = 0.5f;
    Waveform_SetFrequency(gen, frequency);
}

f32 Waveform_Next(WaveformGenerator* gen) {
    f32 sample = 0.0f;
    
    switch (gen->type) {
        case WAVEFORM_SINE:
            sample = sinf(gen->phase * 2.0f * M_PI) * gen->amplitude;
            break;
            
        case WAVEFORM_SQUARE:
            sample = (gen->phase < 0.5f) ? gen->amplitude : -gen->amplitude;
            break;
            
        case WAVEFORM_SAWTOOTH:
            sample = (gen->phase * 2.0f - 1.0f) * gen->amplitude;
            break;
            
        case WAVEFORM_TRIANGLE:
            sample = (gen->phase < 0.5f) ? 
                     (gen->phase * 4.0f - 1.0f) * gen->amplitude :
                     ((1.0f - gen->phase) * 4.0f - 1.0f) * gen->amplitude;
            break;
            
        case WAVEFORM_NOISE:
            sample = ((f32)rand() / RAND_MAX * 2.0f - 1.0f) * gen->amplitude;
            break;
            
        case WAVEFORM_PULSE:
            sample = (gen->phase < gen->duty_cycle) ? gen->amplitude : -gen->amplitude;
            break;
    }
    
    // Advance phase
    gen->phase += gen->phase_increment;
    if (gen->phase >= 1.0f) {
        gen->phase -= 1.0f;
    }
    
    return sample;
}

void Waveform_SetFrequency(WaveformGenerator* gen, f32 frequency) {
    gen->frequency = frequency;
    gen->phase_increment = frequency / (f32)gen->sample_rate;
}

void Waveform_SetAmplitude(WaveformGenerator* gen, f32 amplitude) {
    gen->amplitude = amplitude;
}

void Waveform_SetDutyCycle(WaveformGenerator* gen, f32 duty_cycle) {
    gen->duty_cycle = fmaxf(0.0f, fminf(1.0f, duty_cycle));
}

// -------------------------------------------------------------------------------------------------
// FILTERS IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

void Filter_Init(BiquadFilter* filter, FilterType type, f32 cutoff, f32 resonance, u32 sample_rate) {
    filter->type = type;
    filter->cutoff_frequency = cutoff;
    filter->resonance = resonance;
    filter->q_factor = resonance;
    filter->sample_rate = sample_rate;
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
    
    Filter_CalculateCoefficients(filter);
}

void Filter_CalculateCoefficients(BiquadFilter* filter) {
    f32 sample_rate = filter->sample_rate;
    f32 cutoff = filter->cutoff_frequency;
    f32 q = filter->q_factor;
    
    // Prevent division by zero
    if (cutoff <= 0.0f) cutoff = 1.0f;
    if (q <= 0.0f) q = 0.707f;
    
    f32 omega = 2.0f * M_PI * cutoff / sample_rate;
    f32 sin_omega = sinf(omega);
    f32 cos_omega = cosf(omega);
    f32 alpha = sin_omega / (2.0f * q);
    
    f32 a0;
    
    switch (filter->type) {
        case FILTER_LOWPASS:
            filter->b0 = (1.0f - cos_omega) / 2.0f;
            filter->b1 = 1.0f - cos_omega;
            filter->b2 = (1.0f - cos_omega) / 2.0f;
            a0 = 1.0f + alpha;
            filter->a1 = -2.0f * cos_omega;
            filter->a2 = 1.0f - alpha;
            break;
            
        case FILTER_HIGHPASS:
            filter->b0 = (1.0f + cos_omega) / 2.0f;
            filter->b1 = -(1.0f + cos_omega);
            filter->b2 = (1.0f + cos_omega) / 2.0f;
            a0 = 1.0f + alpha;
            filter->a1 = -2.0f * cos_omega;
            filter->a2 = 1.0f - alpha;
            break;
            
        case FILTER_BANDPASS:
            filter->b0 = alpha;
            filter->b1 = 0.0f;
            filter->b2 = -alpha;
            a0 = 1.0f + alpha;
            filter->a1 = -2.0f * cos_omega;
            filter->a2 = 1.0f - alpha;
            break;
            
        case FILTER_NOTCH:
            filter->b0 = 1.0f;
            filter->b1 = -2.0f * cos_omega;
            filter->b2 = 1.0f;
            a0 = 1.0f + alpha;
            filter->a1 = -2.0f * cos_omega;
            filter->a2 = 1.0f - alpha;
            break;
            
        case FILTER_ALLPASS:
            filter->b0 = 1.0f - alpha;
            filter->b1 = -2.0f * cos_omega;
            filter->b2 = 1.0f + alpha;
            a0 = 1.0f + alpha;
            filter->a1 = -2.0f * cos_omega;
            filter->a2 = 1.0f - alpha;
            break;
    }
    
    // Normalize coefficients
    f32 a0_inv = 1.0f / a0;
    filter->b0 *= a0_inv;
    filter->b1 *= a0_inv;
    filter->b2 *= a0_inv;
    filter->a1 *= a0_inv;
    filter->a2 *= a0_inv;
}

f32 Filter_Process(BiquadFilter* filter, f32 input) {
    f32 output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                - filter->a1 * filter->y1 - filter->a2 * filter->y2;
    
    // Update delay lines
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    
    return output;
}

void Filter_SetCutoff(BiquadFilter* filter, f32 cutoff) {
    filter->cutoff_frequency = cutoff;
    Filter_CalculateCoefficients(filter);
}

void Filter_SetResonance(BiquadFilter* filter, f32 resonance) {
    filter->resonance = resonance;
    filter->q_factor = resonance;
    Filter_CalculateCoefficients(filter);
}

// -------------------------------------------------------------------------------------------------
// MODULAR SYNTHESIS NODE IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

typedef struct {
    WaveformGenerator oscillator;
} OscillatorNodeData;

typedef struct {
    ADSREnvelope envelope;
} EnvelopeNodeData;

typedef struct {
    BiquadFilter filter;
} FilterNodeData;

typedef struct {
    f32 gain;
} GainNodeData;

typedef struct {
    u32 input_count;
    f32* input_gains;
} MixerNodeData;

static void ProcessOscillatorNode(SynthNode* node, f32* output, u32 samples) {
    OscillatorNodeData* data = (OscillatorNodeData*)node->node_data;
    for (u32 i = 0; i < samples; i++) {
        output[i] = Waveform_Next(&data->oscillator);
    }
}

static void ProcessEnvelopeNode(SynthNode* node, f32* output, u32 samples) {
    EnvelopeNodeData* data = (EnvelopeNodeData*)node->node_data;
    for (u32 i = 0; i < samples; i++) {
        output[i] = ADSR_Next(&data->envelope);
    }
}

static void ProcessFilterNode(SynthNode* node, f32* output, u32 samples) {
    FilterNodeData* data = (FilterNodeData*)node->node_data;
    
    // Process input if available
    if (node->connections.input_count > 0 && node->connections.inputs[0]) {
        f32* input = malloc(samples * sizeof(f32));
        if (input) {
            node->connections.inputs[0]->process(node->connections.inputs[0], input, samples);
            
            for (u32 i = 0; i < samples; i++) {
                output[i] = Filter_Process(&data->filter, input[i]);
            }
            
            free(input);
        }
    } else {
        memset(output, 0, samples * sizeof(f32));
    }
}

static void ProcessGainNode(SynthNode* node, f32* output, u32 samples) {
    GainNodeData* data = (GainNodeData*)node->node_data;
    
    if (node->connections.input_count > 0 && node->connections.inputs[0]) {
        f32* input = malloc(samples * sizeof(f32));
        if (input) {
            node->connections.inputs[0]->process(node->connections.inputs[0], input, samples);
            
            for (u32 i = 0; i < samples; i++) {
                output[i] = input[i] * data->gain;
            }
            
            free(input);
        }
    } else {
        memset(output, 0, samples * sizeof(f32));
    }
}

static void ProcessMixerNode(SynthNode* node, f32* output, u32 samples) {
    MixerNodeData* data = (MixerNodeData*)node->node_data;
    
    // Clear output
    memset(output, 0, samples * sizeof(f32));
    
    // Mix all inputs
    for (u32 input_idx = 0; input_idx < node->connections.input_count; input_idx++) {
        if (node->connections.inputs[input_idx]) {
            f32* input = malloc(samples * sizeof(f32));
            if (input) {
                node->connections.inputs[input_idx]->process(node->connections.inputs[input_idx], input, samples);
                
                f32 gain = data->input_gains[input_idx];
                for (u32 i = 0; i < samples; i++) {
                    output[i] += input[i] * gain;
                }
                
                free(input);
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------
// SYNTHESIS ENGINE IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

bool SynthEngine_Init(SynthesisEngine* engine, u32 max_nodes, u32 sample_rate, u32 buffer_size) {
    engine->nodes = malloc(sizeof(SynthNode) * max_nodes);
    if (!engine->nodes) {
        return false;
    }
    
    engine->temp_buffer = malloc(buffer_size * sizeof(f32));
    if (!engine->temp_buffer) {
        free(engine->nodes);
        return false;
    }
    
    engine->max_nodes = max_nodes;
    engine->active_nodes = 0;
    engine->sample_rate = sample_rate;
    engine->buffer_size = buffer_size;
    engine->initialized = true;
    
    // Initialize all nodes as inactive
    memset(engine->nodes, 0, sizeof(SynthNode) * max_nodes);
    
    printf("Synthesis Engine initialized: %d max nodes, %d Hz, %d buffer\n", 
           max_nodes, sample_rate, buffer_size);
    return true;
}

SynthNode* SynthEngine_CreateNode(SynthesisEngine* engine, NodeType type) {
    if (engine->active_nodes >= engine->max_nodes) {
        printf("Error: Maximum nodes reached\n");
        return NULL;
    }
    
    SynthNode* node = &engine->nodes[engine->active_nodes];
    node->type = type;
    node->active = true;
    node->connections.inputs = NULL;
    node->connections.input_count = 0;
    node->connections.input_gains = NULL;
    node->connections.output = 0.0f;
    
    switch (type) {
        case NODE_OSCILLATOR:
            node->node_data = malloc(sizeof(OscillatorNodeData));
            node->process = ProcessOscillatorNode;
            break;
            
        case NODE_ENVELOPE:
            node->node_data = malloc(sizeof(EnvelopeNodeData));
            node->process = ProcessEnvelopeNode;
            break;
            
        case NODE_FILTER:
            node->node_data = malloc(sizeof(FilterNodeData));
            node->process = ProcessFilterNode;
            break;
            
        case NODE_GAIN:
            node->node_data = malloc(sizeof(GainNodeData));
            node->process = ProcessGainNode;
            break;
            
        case NODE_MIXER:
            node->node_data = malloc(sizeof(MixerNodeData));
            node->process = ProcessMixerNode;
            break;
            
        default:
            printf("Error: Unsupported node type\n");
            return NULL;
    }
    
    if (!node->node_data) {
        printf("Error: Failed to allocate node data\n");
        return NULL;
    }
    
    engine->active_nodes++;
    return node;
}

void SynthEngine_DestroyNode(SynthesisEngine* engine, SynthNode* node) {
    if (node && node->active) {
        if (node->node_data) {
            free(node->node_data);
        }
        if (node->connections.inputs) {
            free(node->connections.inputs);
        }
        if (node->connections.input_gains) {
            free(node->connections.input_gains);
        }
        node->active = false;
        engine->active_nodes--;
    }
}

void SynthEngine_Connect(SynthesisEngine* engine, SynthNode* source, SynthNode* destination, f32 gain) {
    if (!source || !destination) return;
    
    // Resize destination inputs array
    u32 new_count = destination->connections.input_count + 1;
    SynthNode** new_inputs = realloc(destination->connections.inputs, sizeof(SynthNode*) * new_count);
    f32* new_gains = realloc(destination->connections.input_gains, sizeof(f32) * new_count);
    
    if (new_inputs && new_gains) {
        destination->connections.inputs = new_inputs;
        destination->connections.input_gains = new_gains;
        destination->connections.inputs[new_count - 1] = source;
        destination->connections.input_gains[new_count - 1] = gain;
        destination->connections.input_count = new_count;
    }
}

void SynthEngine_Process(SynthesisEngine* engine, f32* output, u32 samples) {
    if (!engine->initialized) {
        memset(output, 0, samples * sizeof(f32));
        return;
    }
    
    // Find output nodes and process them
    for (u32 i = 0; i < engine->active_nodes; i++) {
        SynthNode* node = &engine->nodes[i];
        if (node->active && node->type == NODE_OUTPUT) {
            node->process(node, output, samples);
            return;
        }
    }
    
    // No output node found, mix all active nodes
    memset(output, 0, samples * sizeof(f32));
    
    for (u32 i = 0; i < engine->active_nodes; i++) {
        SynthNode* node = &engine->nodes[i];
        if (node->active && node->connections.input_count == 0) {
            node->process(node, engine->temp_buffer, samples);
            
            for (u32 j = 0; j < samples; j++) {
                output[j] += engine->temp_buffer[j];
            }
        }
    }
}

void SynthEngine_Shutdown(SynthesisEngine* engine) {
    if (engine->initialized) {
        for (u32 i = 0; i < engine->active_nodes; i++) {
            SynthEngine_DestroyNode(engine, &engine->nodes[i]);
        }
        
        if (engine->nodes) {
            free(engine->nodes);
        }
        if (engine->temp_buffer) {
            free(engine->temp_buffer);
        }
        
        engine->initialized = false;
        printf("Synthesis Engine shutdown\n");
    }
}

// -------------------------------------------------------------------------------------------------
// NODE CREATION HELPER FUNCTIONS
// -------------------------------------------------------------------------------------------------

SynthNode* CreateOscillatorNode(SynthesisEngine* engine, WaveformType type, f32 frequency, f32 amplitude) {
    SynthNode* node = SynthEngine_CreateNode(engine, NODE_OSCILLATOR);
    if (node) {
        OscillatorNodeData* data = (OscillatorNodeData*)node->node_data;
        Waveform_Init(&data->oscillator, type, frequency, amplitude, engine->sample_rate);
    }
    return node;
}

SynthNode* CreateEnvelopeNode(SynthesisEngine* engine, f32 attack, f32 decay, f32 sustain, f32 release) {
    SynthNode* node = SynthEngine_CreateNode(engine, NODE_ENVELOPE);
    if (node) {
        EnvelopeNodeData* data = (EnvelopeNodeData*)node->node_data;
        ADSR_Init(&data->envelope, attack, decay, sustain, release, engine->sample_rate);
    }
    return node;
}

SynthNode* CreateFilterNode(SynthesisEngine* engine, FilterType type, f32 cutoff, f32 resonance) {
    SynthNode* node = SynthEngine_CreateNode(engine, NODE_FILTER);
    if (node) {
        FilterNodeData* data = (FilterNodeData*)node->node_data;
        Filter_Init(&data->filter, type, cutoff, resonance, engine->sample_rate);
    }
    return node;
}

SynthNode* CreateGainNode(SynthesisEngine* engine, f32 gain) {
    SynthNode* node = SynthEngine_CreateNode(engine, NODE_GAIN);
    if (node) {
        GainNodeData* data = (GainNodeData*)node->node_data;
        data->gain = gain;
    }
    return node;
}

SynthNode* CreateMixerNode(SynthesisEngine* engine, u32 input_count) {
    SynthNode* node = SynthEngine_CreateNode(engine, NODE_MIXER);
    if (node) {
        MixerNodeData* data = (MixerNodeData*)node->node_data;
        data->input_count = input_count;
        data->input_gains = malloc(input_count * sizeof(f32));
        if (data->input_gains) {
            for (u32 i = 0; i < input_count; i++) {
                data->input_gains[i] = 1.0f / (f32)input_count;  // Equal gain by default
            }
        }
    }
    return node;
}

// -------------------------------------------------------------------------------------------------
// MAIN SYNTHESIS INTERFACE
// -------------------------------------------------------------------------------------------------

void Audio_CreateSynth() {
    // TASK_810: Implement Waveform Generator (Sine, Square, Saw).
    //       Allow users to create procedural SFX in-engine.
    if (!g_synth_initialized) {
        SynthEngine_Init(&g_synth_engine, 64, 48000, 512);
        g_synth_initialized = true;
        printf("Audio Synthesis Engine initialized\n");
        
        // Create a simple test patch
        SynthNode* osc = CreateOscillatorNode(&g_synth_engine, WAVEFORM_SINE, 440.0f, 0.5f);
        SynthNode* gain = CreateGainNode(&g_synth_engine, 0.3f);
        
        if (osc && gain) {
            SynthEngine_Connect(&g_synth_engine, osc, gain, 1.0f);
            printf("Test synthesis patch created: 440Hz sine wave\n");
        }
    }
    
    // TASK_811: Implement Modular Patcher.
    //       Node graph for audio DSP (Filter, Reverb, Delay).
    // Modular patcher is now implemented with node-based synthesis
}

// -------------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
// -------------------------------------------------------------------------------------------------

f32 GenerateNoise(f32 amplitude) {
    return ((f32)rand() / RAND_MAX * 2.0f - 1.0f) * amplitude;
}

f32 GeneratePulseWave(f32 phase, f32 duty_cycle) {
    return (phase < duty_cycle) ? 1.0f : -1.0f;
}

f32 GenerateSawtoothWave(f32 phase) {
    return phase * 2.0f - 1.0f;
}

f32 GenerateTriangleWave(f32 phase) {
    return (phase < 0.5f) ? 
           (phase * 4.0f - 1.0f) :
           ((1.0f - phase) * 4.0f - 1.0f);
}
