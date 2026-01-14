#ifndef DTX_SURROUND_H
#define DTX_SURROUND_H

#include <common.h>
#include "engine/include/math/math_all.h"

// -------------------------------------------------------------------------------------------------
// DTX 5.1.2 CHANNEL CONFIGURATION
// -------------------------------------------------------------------------------------------------

typedef enum {
    DTX_CHANNEL_FRONT_LEFT = 0,
    DTX_CHANNEL_FRONT_RIGHT,
    DTX_CHANNEL_CENTER,
    DTX_CHANNEL_LFE,
    DTX_CHANNEL_SIDE_LEFT,
    DTX_CHANNEL_SIDE_RIGHT,
    DTX_CHANNEL_REAR_LEFT,
    DTX_CHANNEL_REAR_RIGHT,
    DTX_CHANNEL_TOP_LEFT,
    DTX_CHANNEL_TOP_RIGHT,
    DTX_CHANNEL_COUNT
} DTXChannel;

typedef struct {
    Vec3 position;
    f32 gain;
    f32 distance;
    f32 delay_ms;
    f32 phase_offset;
    bool active;
    f32 frequency_response[3];  // Low, Mid, High
} DTXSpeakerConfig;

typedef struct {
    DTXSpeakerConfig speakers[DTX_CHANNEL_COUNT];
    f32 master_gain;
    u32 sample_rate;
    u32 buffer_size;
    bool height_channels_enabled;
    bool dtx_enabled;
    f32 crossover_frequency;
    f32 lfe_boost;
} DTXConfig;

// -------------------------------------------------------------------------------------------------
// VERTICAL PANNING SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 overhead_threshold;
    f32 vertical_sensitivity;
    f32 height_spread;
    f32 top_channel_gain;
    f32 front_to_top_ratio;
} VerticalPanningConfig;

typedef struct {
    VerticalPanningConfig config;
    f32 height_gains[DTX_CHANNEL_COUNT];
    Vec3 source_position;
    f32 elevation_angle;
    f32 azimuth_angle;
} VerticalPanner;

// -------------------------------------------------------------------------------------------------
// BASS MANAGEMENT SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 crossover_frequency;
    f32 lfe_boost_db;
    f32 main_channel_lowcut;
    f32 lfe_highcut;
    bool bass_redirect_enabled;
    f32 speaker_size_compensation[DTX_CHANNEL_COUNT];
} BassManagementConfig;

typedef struct {
    BassManagementConfig config;
    f32 crossover_filter_state[DTX_CHANNEL_COUNT][4];  // Biquad state
    f32 lfe_filter_state[4];
    bool filters_initialized;
} BassManager;

// -------------------------------------------------------------------------------------------------
// DTX:X IMMERSIVE DECODER
// -------------------------------------------------------------------------------------------------

typedef struct {
    bool enabled;
    f32 upmix_gain;
    f32 center_width;
    f32 surround_gain;
    f32 height_gain;
    f32 dialog_enhancement;
    f32 night_mode_reduction;
} DTXDecoderConfig;

typedef struct {
    DTXDecoderConfig config;
    f32 decoder_matrix[DTX_CHANNEL_COUNT][6];  // 5.1 to 7.1.2 matrix
    f32 dynamic_range_compressor;
    bool decoder_active;
} DTXDecoder;

// -------------------------------------------------------------------------------------------------
// PHASE COMPENSATION SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 phase_compensation_delay[DTX_CHANNEL_COUNT];
    f32 distance_based_phase[DTX_CHANNEL_COUNT];
    f32 phase_correction_factor;
    bool phase_correction_enabled;
} PhaseCompensation;

// -------------------------------------------------------------------------------------------------
// MULTICHANNEL REVERB SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 room_size;
    f32 decay_time;
    f32 damping;
    f32 early_reflections_gain;
    f32 late_reverb_gain;
    f32 stereo_spread;
    f32 height_spread;
} MultichannelReverbConfig;

typedef struct {
    MultichannelReverbConfig config;
    f32 delay_lines[DTX_CHANNEL_COUNT][4096];
    u32 delay_indices[DTX_CHANNEL_COUNT];
    f32 filter_states[DTX_CHANNEL_COUNT][4];
    bool reverb_active;
} MultichannelReverb;

// -------------------------------------------------------------------------------------------------
// CALIBRATION SYSTEM
// -------------------------------------------------------------------------------------------------

typedef struct {
    f32 channel_levels[DTX_CHANNEL_COUNT];
    f32 channel_distances[DTX_CHANNEL_COUNT];
    f32 channel_delays[DTX_CHANNEL_COUNT];
    f32 room_eq_bands[10];  // 10-band graphic EQ
    bool calibration_complete;
    f32 test_tone_frequency;
} CalibrationData;

// -------------------------------------------------------------------------------------------------
// FUNCTION DECLARATIONS
// -------------------------------------------------------------------------------------------------

// DTX 5.1.2 Configuration (TASK_710-713)
bool DTX_InitSystem(DTXConfig* config);
void DTX_Setup51Layout(DTXConfig* config);
void DTX_Setup71Expansion(DTXConfig* config);
void DTX_EnableDynamicDownmix(DTXConfig* config, bool enable);
void DTX_ProcessDownmix(f32* input_buffer, f32* output_buffer, u32 frames, u32 input_channels, u32 output_channels);

// Height Channel Rendering (TASK_720-722)
bool VerticalPanner_Init(VerticalPanner* panner, const VerticalPanningConfig* config);
void VerticalPanner_CalculateGains(VerticalPanner* panner, const Vec3 source_pos);
void VerticalPanner_ApplyHeightGains(VerticalPanner* panner, f32* buffer, u32 frames);

// Bass Management (TASK_730-732)
bool BassManager_Init(BassManager* manager, const BassManagementConfig* config);
void BassManager_ProcessCrossover(BassManager* manager, f32* buffer, u32 frames);
void BassManager_ApplyLFEBoost(BassManager* manager, f32* lfe_buffer, u32 frames);
void BassManager_RedirectBass(BassManager* manager, f32* main_buffers[DTX_CHANNEL_COUNT], f32* lfe_buffer, u32 frames);

// DTX:X Processing (TASK_740-743)
bool DTXDecoder_Init(DTXDecoder* decoder, const DTXDecoderConfig* config);
void DTXDecoder_ProcessMatrix(DTXDecoder* decoder, f32* input_51, f32* output_71, u32 frames);
void DTXDecoder_ApplyPhaseCompensation(DTXDecoder* decoder, f32* buffer, u32 frames);
void DTXDecoder_ProcessObstruction(DTXDecoder* decoder, f32* buffer, u32 frames, const Vec3 obstruction_vector);
void DTXDecoder_ProcessMultichannelReverb(DTXDecoder* decoder, f32* buffer, u32 frames);

// Calibration (TASK_750-752)
bool Calibration_Init(CalibrationData* calibration);
void Calibration_GenerateTestTone(CalibrationData* calibration, f32* buffer, u32 frames, u32 channel);
void Calibration_MeasureResponse(CalibrationData* calibration, const f32* buffer, u32 frames, u32 channel);
void Calibration_ApplyRoomEQ(CalibrationData* calibration, f32* buffer, u32 frames);

// Optimization (TASK_760-762)
void DTX_SIMDOptimizedMix(f32* output, const f32* input, const f32* gains, u32 frames);
void DTX_OptimizeBufferInterleaving(f32* buffer, u32 frames);
void DTX_SelectiveChannelRendering(f32* buffer, const bool* active_channels, u32 frames);

// Main DTX processing
void DTX_ProcessAudio(f32* output_buffer, const f32* input_buffer, u32 frames, const DTXConfig* config);
void DTX_UpdateSettings(DTXConfig* config);

#endif // DTX_SURROUND_H
