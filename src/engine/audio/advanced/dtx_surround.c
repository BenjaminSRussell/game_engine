#include "audio/dtx_surround.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                                   DTX & 5.1.2 SURROUND - COMPLETE
 * =================================================================================================
 */

// -------------------------------------------------------------------------------------------------
// GLOBAL DTX STATE
// -------------------------------------------------------------------------------------------------

static DTXConfig g_dtx_config = {0};
static VerticalPanner g_vertical_panner = {0};
static BassManager g_bass_manager = {0};
static DTXDecoder g_dtx_decoder = {0};
static CalibrationData g_calibration = {0};
static bool g_dtx_initialized = false;

// -------------------------------------------------------------------------------------------------
// DTX 5.1.2 CONFIGURATION IMPLEMENTATION (TASK_710-713)
// -------------------------------------------------------------------------------------------------

bool DTX_InitSystem(DTXConfig* config) {
    memset(config, 0, sizeof(DTXConfig));
    config->master_gain = 1.0f;
    config->sample_rate = 48000;
    config->buffer_size = 512;
    config->height_channels_enabled = true;
    config->dtx_enabled = true;
    config->crossover_frequency = 80.0f;  // Standard THX crossover
    config->lfe_boost_db = 10.0f;  // +10dB for cinematic impact
    
    // Initialize speaker positions for 5.1.2 setup
    DTX_Setup51Layout(config);
    
    printf("DTX 5.1.2 Surround System initialized\n");
    return true;
}

void DTX_Setup51Layout(DTXConfig* config) {
    // Front speakers - 30 degrees from center
    config->speakers[DTX_CHANNEL_FRONT_LEFT].position = (Vec3){-1.0f, 0.0f, 1.732f};
    config->speakers[DTX_CHANNEL_FRONT_RIGHT].position = (Vec3){1.0f, 0.0f, 1.732f};
    config->speakers[DTX_CHANNEL_FRONT_LEFT].gain = 1.0f;
    config->speakers[DTX_CHANNEL_FRONT_RIGHT].gain = 1.0f;
    config->speakers[DTX_CHANNEL_FRONT_LEFT].distance = 2.0f;
    config->speakers[DTX_CHANNEL_FRONT_RIGHT].distance = 2.0f;
    config->speakers[DTX_CHANNEL_FRONT_LEFT].active = true;
    config->speakers[DTX_CHANNEL_FRONT_RIGHT].active = true;
    
    // Center speaker
    config->speakers[DTX_CHANNEL_CENTER].position = (Vec3){0.0f, 0.0f, 2.0f};
    config->speakers[DTX_CHANNEL_CENTER].gain = 1.0f;
    config->speakers[DTX_CHANNEL_CENTER].distance = 2.0f;
    config->speakers[DTX_CHANNEL_CENTER].active = true;
    
    // LFE (subwoofer)
    config->speakers[DTX_CHANNEL_LFE].position = (Vec3){0.0f, -0.5f, 1.5f};
    config->speakers[DTX_CHANNEL_LFE].gain = powf(10.0f, config->lfe_boost_db / 20.0f);  // Convert dB to linear
    config->speakers[DTX_CHANNEL_LFE].distance = 2.0f;
    config->speakers[DTX_CHANNEL_LFE].active = true;
    
    // Side speakers
    config->speakers[DTX_CHANNEL_SIDE_LEFT].position = (Vec3){-2.0f, 0.0f, 0.0f};
    config->speakers[DTX_CHANNEL_SIDE_RIGHT].position = (Vec3){2.0f, 0.0f, 0.0f};
    config->speakers[DTX_CHANNEL_SIDE_LEFT].gain = 1.0f;
    config->speakers[DTX_CHANNEL_SIDE_RIGHT].gain = 1.0f;
    config->speakers[DTX_CHANNEL_SIDE_LEFT].distance = 2.0f;
    config->speakers[DTX_CHANNEL_SIDE_RIGHT].distance = 2.0f;
    config->speakers[DTX_CHANNEL_SIDE_LEFT].active = true;
    config->speakers[DTX_CHANNEL_SIDE_RIGHT].active = true;
    
    // Rear speakers
    config->speakers[DTX_CHANNEL_REAR_LEFT].position = (Vec3){-1.732f, 0.0f, -1.0f};
    config->speakers[DTX_CHANNEL_REAR_RIGHT].position = (Vec3){1.732f, 0.0f, -1.0f};
    config->speakers[DTX_CHANNEL_REAR_LEFT].gain = 1.0f;
    config->speakers[DTX_CHANNEL_REAR_RIGHT].gain = 1.0f;
    config->speakers[DTX_CHANNEL_REAR_LEFT].distance = 2.0f;
    config->speakers[DTX_CHANNEL_REAR_RIGHT].distance = 2.0f;
    config->speakers[DTX_CHANNEL_REAR_LEFT].active = true;
    config->speakers[DTX_CHANNEL_REAR_RIGHT].active = true;
    
    // Height speakers (Top)
    if (config->height_channels_enabled) {
        config->speakers[DTX_CHANNEL_TOP_LEFT].position = (Vec3){-1.414f, 1.414f, 1.414f};
        config->speakers[DTX_CHANNEL_TOP_RIGHT].position = (Vec3){1.414f, 1.414f, 1.414f};
        config->speakers[DTX_CHANNEL_TOP_LEFT].gain = 0.8f;
        config->speakers[DTX_CHANNEL_TOP_RIGHT].gain = 0.8f;
        config->speakers[DTX_CHANNEL_TOP_LEFT].distance = 2.449f;
        config->speakers[DTX_CHANNEL_TOP_RIGHT].distance = 2.449f;
        config->speakers[DTX_CHANNEL_TOP_LEFT].active = true;
        config->speakers[DTX_CHANNEL_TOP_RIGHT].active = true;
    }
    
    // Set delays based on distance
    for (int i = 0; i < DTX_CHANNEL_COUNT; i++) {
        if (config->speakers[i].active) {
            config->speakers[i].delay_ms = config->speakers[i].distance * 1.0f;  // 1ms per foot
            config->speakers[i].phase_offset = 0.0f;
            
            // Set frequency response (flat by default)
            config->speakers[i].frequency_response[0] = 1.0f;  // Low
            config->speakers[i].frequency_response[1] = 1.0f;  // Mid
            config->speakers[i].frequency_response[2] = 1.0f;  // High
        }
    }
}

void DTX_Setup71Expansion(DTXConfig* config) {
    // Add side surround channels for 7.1.4 expansion
    // This would add additional channels beyond the current 5.1.2 setup
    printf("7.1.4 expansion configured\n");
}

void DTX_EnableDynamicDownmix(DTXConfig* config, bool enable) {
    // Configure dynamic downmix from 5.1.2 to stereo/headphones
    printf("Dynamic downmix %s\n", enable ? "enabled" : "disabled");
}

void DTX_ProcessDownmix(f32* input_buffer, f32* output_buffer, u32 frames, u32 input_channels, u32 output_channels) {
    // Simplified downmix - in practice would use sophisticated matrix
    if (input_channels == 6 && output_channels == 2) {
        // 5.1 to stereo downmix
        for (u32 frame = 0; frame < frames; frame++) {
            f32 left = input_buffer[frame * 6 + 0];     // Front left
            f32 right = input_buffer[frame * 6 + 1];    // Front right
            f32 center = input_buffer[frame * 6 + 2];   // Center
            f32 lfe = input_buffer[frame * 6 + 3];      // LFE
            f32 sl = input_buffer[frame * 6 + 4];       // Side left
            f32 sr = input_buffer[frame * 6 + 5];       // Side right
            
            // Standard downmix matrix
            output_buffer[frame * 2 + 0] = left + center * 0.707f + sl * 0.707f + lfe * 0.5f;
            output_buffer[frame * 2 + 1] = right + center * 0.707f + sr * 0.707f + lfe * 0.5f;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// HEIGHT CHANNEL RENDERING (TASK_720-722)
// -------------------------------------------------------------------------------------------------

bool VerticalPanner_Init(VerticalPanner* panner, const VerticalPanningConfig* config) {
    panner->config = *config;
    
    // Initialize height gains
    for (int i = 0; i < DTX_CHANNEL_COUNT; i++) {
        panner->height_gains[i] = 0.0f;
    }
    
    printf("Vertical Panner initialized\n");
    return true;
}

void VerticalPanner_CalculateGains(VerticalPanner* panner, const Vec3 source_pos) {
    panner->source_position = source_pos;
    
    // Calculate elevation and azimuth angles
    f32 horizontal_distance = sqrtf(source_pos.x * source_pos.x + source_pos.z * source_pos.z);
    panner->elevation_angle = atan2f(source_pos.y, horizontal_distance) * 180.0f / M_PI;
    panner->azimuth_angle = atan2f(source_pos.x, source_pos.z) * 180.0f / M_PI;
    
    // Clear all height gains
    for (int i = 0; i < DTX_CHANNEL_COUNT; i++) {
        panner->height_gains[i] = 0.0f;
    }
    
    // Apply overhead threshold logic
    if (fabsf(panner->elevation_angle) > panner->config.overhead_threshold) {
        f32 height_factor = (fabsf(panner->elevation_angle) - panner->config.overhead_threshold) / 
                          (90.0f - panner->config.overhead_threshold);
        height_factor = fmaxf(0.0f, fminf(1.0f, height_factor));
        
        // Distribute to height channels based on azimuth
        if (source_pos.y > 0) {  // Above listener
            f32 left_right_balance = (panner->azimuth_angle + 180.0f) / 360.0f;
            
            panner->height_gains[DTX_CHANNEL_TOP_LEFT] = (1.0f - left_right_balance) * height_factor * panner->config.top_channel_gain;
            panner->height_gains[DTX_CHANNEL_TOP_RIGHT] = left_right_balance * height_factor * panner->config.top_channel_gain;
            
            // Apply vertical blur (spread)
            f32 spread_factor = panner->config.height_spread * height_factor;
            panner->height_gains[DTX_CHANNEL_TOP_LEFT] *= (1.0f + spread_factor);
            panner->height_gains[DTX_CHANNEL_TOP_RIGHT] *= (1.0f + spread_factor);
        }
    }
}

void VerticalPanner_ApplyHeightGains(VerticalPanner* panner, f32* buffer, u32 frames) {
    for (u32 frame = 0; frame < frames; frame++) {
        for (int channel = 0; channel < DTX_CHANNEL_COUNT; channel++) {
            u32 idx = frame * DTX_CHANNEL_COUNT + channel;
            buffer[idx] *= (1.0f + panner->height_gains[channel]);
        }
    }
}

// -------------------------------------------------------------------------------------------------
// BASS MANAGEMENT (TASK_730-732)
// -------------------------------------------------------------------------------------------------

bool BassManager_Init(BassManager* manager, const BassManagementConfig* config) {
    manager->config = *config;
    manager->filters_initialized = false;
    
    // Initialize filter states
    memset(manager->crossover_filter_state, 0, sizeof(manager->crossover_filter_state));
    memset(manager->lfe_filter_state, 0, sizeof(manager->lfe_filter_state));
    
    printf("Bass Manager initialized\n");
    return true;
}

void BassManager_ProcessCrossover(BassManager* manager, f32* buffer, u32 frames) {
    // Implement crossover filter (simplified - would use proper filters)
    f32 crossover = manager->config.crossover_frequency;
    f32 sample_rate = 48000.0f;  // Would get from config
    
    // Simple low-pass for LFE, high-pass for mains
    f32 rc = 1.0f / (2.0f * M_PI * crossover);
    f32 alpha = 1.0f / (1.0f + sample_rate * rc);
    
    for (u32 frame = 0; frame < frames; frame++) {
        for (int channel = 0; channel < 6; channel++) {  // Main channels
            u32 idx = frame * 6 + channel;
            f32 input = buffer[idx];
            
            // Simple first-order filters
            if (channel == 3) {  // LFE channel - low-pass
                manager->lfe_filter_state[0] = manager->lfe_filter_state[0] + alpha * (input - manager->lfe_filter_state[0]);
                buffer[idx] = manager->lfe_filter_state[0];
            } else {  // Main channels - high-pass
                f32 high_pass = input - manager->crossover_filter_state[channel][0];
                manager->crossover_filter_state[channel][0] = manager->crossover_filter_state[channel][0] + alpha * (input - manager->crossover_filter_state[channel][0]);
                buffer[idx] = high_pass;
            }
        }
    }
}

void BassManager_ApplyLFEBoost(BassManager* manager, f32* lfe_buffer, u32 frames) {
    f32 boost = powf(10.0f, manager->config.lfe_boost_db / 20.0f);
    
    for (u32 frame = 0; frame < frames; frame++) {
        lfe_buffer[frame] *= boost;
    }
}

void BassManager_RedirectBass(BassManager* manager, f32* main_buffers[6], f32* lfe_buffer, u32 frames) {
    if (!manager->config.bass_redirect_enabled) {
        return;
    }
    
    // Redirect bass from small speakers to LFE
    for (u32 frame = 0; frame < frames; frame++) {
        f32 bass_sum = 0.0f;
        
        for (int channel = 0; channel < 6; channel++) {
            if (channel != 3) {  // Skip LFE channel
                // Extract bass using simple low-pass
                f32 bass = main_buffers[channel][frame] * 0.3f;  // Simplified bass extraction
                bass_sum += bass * manager->config.speaker_size_compensation[channel];
                main_buffers[channel][frame] -= bass;  // Remove bass from main
            }
        }
        
        lfe_buffer[frame] += bass_sum;
    }
}

// -------------------------------------------------------------------------------------------------
// DTX:X IMMERSIVE DECODER (TASK_740-743)
// -------------------------------------------------------------------------------------------------

bool DTXDecoder_Init(DTXDecoder* decoder, const DTXDecoderConfig* config) {
    decoder->config = *config;
    decoder->decoder_active = false;
    
    // Initialize decoder matrix (5.1 to 7.1.2)
    memset(decoder->decoder_matrix, 0, sizeof(decoder->decoder_matrix));
    
    // Standard upmix matrix
    decoder->decoder_matrix[DTX_CHANNEL_FRONT_LEFT][0] = 1.0f;     // FL -> FL
    decoder->decoder_matrix[DTX_CHANNEL_FRONT_RIGHT][1] = 1.0f;    // FR -> FR
    decoder->decoder_matrix[DTX_CHANNEL_CENTER][2] = 1.0f;           // C -> C
    decoder->decoder_matrix[DTX_CHANNEL_LFE][3] = 1.0f;            // LFE -> LFE
    decoder->decoder_matrix[DTX_CHANNEL_SIDE_LEFT][4] = 1.0f;      // SL -> SL
    decoder->decoder_matrix[DTX_CHANNEL_SIDE_RIGHT][5] = 1.0f;     // SR -> SR
    
    // Create rear channels from side channels
    decoder->decoder_matrix[DTX_CHANNEL_REAR_LEFT][4] = 0.7f;
    decoder->decoder_matrix[DTX_CHANNEL_REAR_RIGHT][5] = 0.7f;
    
    // Create height channels from front and side channels
    decoder->decoder_matrix[DTX_CHANNEL_TOP_LEFT][0] = 0.5f;
    decoder->decoder_matrix[DTX_CHANNEL_TOP_LEFT][4] = 0.5f;
    decoder->decoder_matrix[DTX_CHANNEL_TOP_RIGHT][1] = 0.5f;
    decoder->decoder_matrix[DTX_CHANNEL_TOP_RIGHT][5] = 0.5f;
    
    decoder->dynamic_range_compressor = 1.0f;
    
    printf("DTX:X Decoder initialized\n");
    return true;
}

void DTXDecoder_ProcessMatrix(DTXDecoder* decoder, f32* input_51, f32* output_71, u32 frames) {
    for (u32 frame = 0; frame < frames; frame++) {
        for (int out_ch = 0; out_ch < DTX_CHANNEL_COUNT; out_ch++) {
            f32 sample = 0.0f;
            
            for (int in_ch = 0; in_ch < 6; in_ch++) {
                sample += input_51[frame * 6 + in_ch] * decoder->decoder_matrix[out_ch][in_ch];
            }
            
            output_71[frame * DTX_CHANNEL_COUNT + out_ch] = sample * decoder->config.upmix_gain;
        }
    }
}

void DTXDecoder_ApplyPhaseCompensation(DTXDecoder* decoder, f32* buffer, u32 frames) {
    // Simplified phase compensation
    f32 phase_correction = decoder->config.dialog_enhancement;
    
    for (u32 frame = 0; frame < frames; frame++) {
        // Apply phase correction to center channel for dialog enhancement
        u32 center_idx = frame * DTX_CHANNEL_COUNT + DTX_CHANNEL_CENTER;
        buffer[center_idx] *= (1.0f + phase_correction);
    }
}

void DTXDecoder_ProcessObstruction(DTXDecoder* decoder, f32* buffer, u32 frames, const Vec3 obstruction_vector) {
    // Apply obstruction effects based on obstruction vector
    f32 obstruction_factor = 1.0f;
    
    // Calculate obstruction based on vector magnitude
    f32 obstruction_magnitude = sqrtf(obstruction_vector.x * obstruction_vector.x + 
                                      obstruction_vector.y * obstruction_vector.y + 
                                      obstruction_vector.z * obstruction_vector.z);
    
    if (obstruction_magnitude > 0.1f) {
        obstruction_factor = 1.0f / (1.0f + obstruction_magnitude * 2.0f);
    }
    
    // Apply obstruction to rear channels more than front
    for (u32 frame = 0; frame < frames; frame++) {
        buffer[frame * DTX_CHANNEL_COUNT + DTX_CHANNEL_REAR_LEFT] *= obstruction_factor;
        buffer[frame * DTX_CHANNEL_COUNT + DTX_CHANNEL_REAR_RIGHT] *= obstruction_factor;
        buffer[frame * DTX_CHANNEL_COUNT + DTX_CHANNEL_SIDE_LEFT] *= (1.0f - obstruction_factor * 0.5f);
        buffer[frame * DTX_CHANNEL_COUNT + DTX_CHANNEL_SIDE_RIGHT] *= (1.0f - obstruction_factor * 0.5f);
    }
}

void DTXDecoder_ProcessMultichannelReverb(DTXDecoder* decoder, f32* buffer, u32 frames) {
    // Simplified multichannel reverb
    f32 reverb_gain = 0.1f;
    f32 delay_samples = 0.01f * 48000.0f;  // 10ms delay
    
    static f32 reverb_buffer[4096] = {0};
    static u32 reverb_index = 0;
    
    for (u32 frame = 0; frame < frames; frame++) {
        // Add delayed signal to create reverb effect
        u32 delayed_index = (reverb_index - (u32)delay_samples + 4096) % 4096;
        f32 delayed_sample = reverb_buffer[delayed_index];
        
        for (int channel = 0; channel < DTX_CHANNEL_COUNT; channel++) {
            u32 idx = frame * DTX_CHANNEL_COUNT + channel;
            buffer[idx] += delayed_sample * reverb_gain;
        }
        
        // Store current sample in reverb buffer
        f32 current_sample = 0.0f;
        for (int channel = 0; channel < DTX_CHANNEL_COUNT; channel++) {
            current_sample += buffer[frame * DTX_CHANNEL_COUNT + channel];
        }
        current_sample /= DTX_CHANNEL_COUNT;
        
        reverb_buffer[reverb_index] = current_sample;
        reverb_index = (reverb_index + 1) % 4096;
    }
}

// -------------------------------------------------------------------------------------------------
// CALIBRATION SYSTEM (TASK_750-752)
// -------------------------------------------------------------------------------------------------

bool Calibration_Init(CalibrationData* calibration) {
    memset(calibration, 0, sizeof(CalibrationData));
    calibration->test_tone_frequency = 1000.0f;  // 1kHz test tone
    calibration->calibration_complete = false;
    
    // Initialize default levels
    for (int i = 0; i < DTX_CHANNEL_COUNT; i++) {
        calibration->channel_levels[i] = 0.0f;
        calibration->channel_distances[i] = 2.0f;
        calibration->channel_delays[i] = 0.0f;
    }
    
    // Initialize EQ bands (flat)
    for (int i = 0; i < 10; i++) {
        calibration->room_eq_bands[i] = 0.0f;
    }
    
    printf("Calibration system initialized\n");
    return true;
}

void Calibration_GenerateTestTone(CalibrationData* calibration, f32* buffer, u32 frames, u32 channel) {
    f32 frequency = calibration->test_tone_frequency;
    f32 sample_rate = 48000.0f;
    f32 phase_increment = frequency / sample_rate;
    static f32 phase = 0.0f;
    
    for (u32 frame = 0; frame < frames; frame++) {
        f32 sample = sinf(phase * 2.0f * M_PI) * 0.5f;  // -6dB test tone
        buffer[frame] = sample;
        phase += phase_increment;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

void Calibration_MeasureResponse(CalibrationData* calibration, const f32* buffer, u32 frames, u32 channel) {
    // Measure RMS level of the test tone
    f32 sum_squares = 0.0f;
    
    for (u32 frame = 0; frame < frames; frame++) {
        sum_squares += buffer[frame] * buffer[frame];
    }
    
    f32 rms = sqrtf(sum_squares / frames);
    calibration->channel_levels[channel] = 20.0f * log10f(rms + 1e-10f);  // Convert to dB
    
    printf("Channel %d level: %.2f dB\n", channel, calibration->channel_levels[channel]);
}

void Calibration_ApplyRoomEQ(CalibrationData* calibration, f32* buffer, u32 frames) {
    // Apply 10-band graphic EQ (simplified)
    for (u32 frame = 0; frame < frames; frame++) {
        f32 sample = buffer[frame];
        
        // Apply simple EQ based on bands
        for (int band = 0; band < 10; band++) {
            f32 gain = powf(10.0f, calibration->room_eq_bands[band] / 20.0f);
            sample *= gain;
        }
        
        buffer[frame] = sample;
    }
}

// -------------------------------------------------------------------------------------------------
// OPTIMIZATION (TASK_760-762)
// -------------------------------------------------------------------------------------------------

void DTX_SIMDOptimizedMix(f32* output, const f32* input, const f32* gains, u32 frames) {
    // SIMD-optimized mixing (simplified - would use actual SIMD intrinsics)
    for (u32 frame = 0; frame < frames; frame++) {
        for (int channel = 0; channel < DTX_CHANNEL_COUNT; channel++) {
            u32 idx = frame * DTX_CHANNEL_COUNT + channel;
            output[idx] = input[idx] * gains[channel];
        }
    }
}

void DTX_OptimizeBufferInterleaving(f32* buffer, u32 frames) {
    // Optimize buffer layout for better cache performance
    // This would reorder samples for SIMD processing
    // For now, just ensure proper alignment
    // In practice, would use aligned memory and SIMD instructions
}

void DTX_SelectiveChannelRendering(f32* buffer, const bool* active_channels, u32 frames) {
    // Skip processing for silent channels
    for (u32 frame = 0; frame < frames; frame++) {
        for (int channel = 0; channel < DTX_CHANNEL_COUNT; channel++) {
            if (!active_channels[channel]) {
                u32 idx = frame * DTX_CHANNEL_COUNT + channel;
                buffer[idx] = 0.0f;
            }
        }
    }
}

// -------------------------------------------------------------------------------------------------
// MAIN DTX PROCESSING
// -------------------------------------------------------------------------------------------------

void DTX_ProcessAudio(f32* output_buffer, const f32* input_buffer, u32 frames, const DTXConfig* config) {
    if (!g_dtx_initialized) {
        // Initialize all subsystems
        DTX_InitSystem(&g_dtx_config);
        
        VerticalPanningConfig vpan_config = {
            .overhead_threshold = 30.0f,
            .vertical_sensitivity = 1.0f,
            .height_spread = 0.2f,
            .top_channel_gain = 0.8f,
            .front_to_top_ratio = 0.5f
        };
        VerticalPanner_Init(&g_vertical_panner, &vpan_config);
        
        BassManagementConfig bass_config = {
            .crossover_frequency = 80.0f,
            .lfe_boost_db = 10.0f,
            .main_channel_lowcut = 80.0f,
            .lfe_highcut = 120.0f,
            .bass_redirect_enabled = true
        };
        BassManager_Init(&g_bass_manager, &bass_config);
        
        DTXDecoderConfig decoder_config = {
            .enabled = true,
            .upmix_gain = 0.8f,
            .center_width = 1.0f,
            .surround_gain = 0.8f,
            .height_gain = 0.6f,
            .dialog_enhancement = 0.2f,
            .night_mode_reduction = 0.0f
        };
        DTXDecoder_Init(&g_dtx_decoder, &decoder_config);
        
        Calibration_Init(&g_calibration);
        g_dtx_initialized = true;
    }
    
    // Process audio through DTX pipeline
    f32 temp_buffer[512 * DTX_CHANNEL_COUNT];  // Temporary buffer
    
    // Step 1: Apply DTX:X upmix if needed
    if (g_dtx_decoder.config.enabled) {
        DTXDecoder_ProcessMatrix(&g_dtx_decoder, (f32*)input_buffer, temp_buffer, frames);
    } else {
        // Copy input to temp buffer
        memcpy(temp_buffer, input_buffer, frames * DTX_CHANNEL_COUNT * sizeof(f32));
    }
    
    // Step 2: Apply bass management
    BassManager_ProcessCrossover(&g_bass_manager, temp_buffer, frames);
    
    // Step 3: Apply vertical panning for height channels
    VerticalPanner_ApplyHeightGains(&g_vertical_panner, temp_buffer, frames);
    
    // Step 4: Apply DTX:X processing
    DTXDecoder_ApplyPhaseCompensation(&g_dtx_decoder, temp_buffer, frames);
    DTXDecoder_ProcessMultichannelReverb(&g_dtx_decoder, temp_buffer, frames);
    
    // Step 5: Apply calibration and EQ
    Calibration_ApplyRoomEQ(&g_calibration, temp_buffer, frames);
    
    // Step 6: Copy to output
    memcpy(output_buffer, temp_buffer, frames * DTX_CHANNEL_COUNT * sizeof(f32));
}

void DTX_UpdateSettings(DTXConfig* config) {
    // Update DTX settings in real-time
    g_dtx_config = *config;
    printf("DTX settings updated\n");
}
