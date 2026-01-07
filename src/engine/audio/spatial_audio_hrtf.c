#include "audio/spatial_audio_hrtf.h"
#include <include/math/math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                                   SPATIAL AUDIO HRTF SYSTEM
 * =================================================================================================
 */

// -------------------------------------------------------------------------------------------------
// GLOBAL HRTF STATE
// -------------------------------------------------------------------------------------------------

static SpatialAudioProcessor g_hrtf_processor = {0};
static BafflingModel g_baffling_model = {0};
static HRTFCrossfade g_hrtf_crossfade = {0};
static NearFieldModel g_near_field_model = {0};
static bool g_hrtf_initialized = false;

// -------------------------------------------------------------------------------------------------
// HRTF DATABASE MANAGEMENT
// -------------------------------------------------------------------------------------------------

bool HRTF_LoadDatabase(HRTFDatabase* database, const char* dataset_path, HRTFDataset dataset_type) {
    database->dataset_type = dataset_type;
    strncpy(database->dataset_path, dataset_path, sizeof(database->dataset_path) - 1);
    database->sample_rate = 48000;
    database->impulse_length = 128;  // Standard HRTF impulse length
    database->loaded = false;
    
    switch (dataset_type) {
        case HRTF_DATASET_GENERIC:
            return HRTF_LoadGenericDataset(database);
        case HRTF_DATASET_KEMAR:
            return HRTF_LoadKEMARDataset(database);
        case HRTF_DATASET_CIPIC:
        case HRTF_DATASET_LISTEN:
            printf(\"HRTF dataset type not yet implemented\\n\");
            return false;
        case HRTF_DATASET_CUSTOM:
            return HRTF_LoadCustomDataset(database, dataset_path);
        default:
            return false;
    }
}

bool HRTF_LoadGenericDataset(HRTFDatabase* database) {
    printf(\"Loading generic HRTF dataset...\\n\");
    
    // Create a simplified generic HRTF dataset
    // In practice, this would load actual HRTF measurements from files
    
    // Allocate measurements for standard azimuth/elevation grid
    database->measurement_count = 72 * 19;  // 5-degree azimuth steps, 10-degree elevation steps
    database->measurements = malloc(sizeof(HRTFMeasurement) * database->measurement_count);
    
    if (!database->measurements) {
        return false;
    }
    
    // Generate simplified HRTF measurements
    u32 measurement_index = 0;
    for (int elev = -90; elev <= 90; elev += 10) {
        for (int azim = 0; azim < 360; azim += 5) {
            HRTFMeasurement* measurement = &database->measurements[measurement_index];
            measurement->azimuth = (f32)azim;
            measurement->elevation = (f32)elev;
            measurement->impulse_length = database->impulse_length;
            
            // Allocate impulse responses
            measurement->left_ear_impulse = malloc(sizeof(f32) * measurement->impulse_length);
            measurement->right_ear_impulse = malloc(sizeof(f32) * measurement->impulse_length);
            
            if (!measurement->left_ear_impulse || !measurement->right_ear_impulse) {
                return false;
            }
            
            // Generate simplified impulse responses
            for (u32 i = 0; i < measurement->impulse_length; i++) {
                f32 t = (f32)i / database->sample_rate;
                
                // Basic HRTF characteristics
                f32 left_delay = 0.0f;
                f32 right_delay = 0.0f;
                f32 left_gain = 1.0f;
                f32 right_gain = 1.0f;
                
                // Calculate interaural time difference
                f32 azimuth_rad = azim * M_PI / 180.0f;
                f32 elevation_rad = elev * M_PI / 180.0f;
                f32 head_radius = 0.0875f;  // 8.75 cm average head radius
                
                // ITD calculation
                f32 itd = head_radius * (sin(azimuth_rad) + 0.5f * sin(elevation_rad) * cos(azimuth_rad)) / 343.0f;
                itd *= database->sample_rate;  // Convert to samples
                
                if (azim <= 180) {
                    left_delay = -itd / 2.0f;
                    right_delay = itd / 2.0f;
                } else {
                    left_delay = itd / 2.0f;
                    right_delay = -itd / 2.0f;
                }
                
                // ILD calculation (simplified)
                f32 ild_factor = fabsf(sin(azimuth_rad)) * 0.5f;
                if (azim <= 180) {
                    left_gain = 1.0f + ild_factor;
                    right_gain = 1.0f - ild_factor;
                } else {
                    left_gain = 1.0f - ild_factor;
                    right_gain = 1.0f + ild_factor;
                }
                
                // Generate impulse with delay and gain
                f32 sample_time = (f32)i - left_delay;
                if (sample_time >= 0.0f && sample_time < 10.0f) {
                    measurement->left_ear_impulse[i] = left_gain * expf(-sample_time / 2.0f) * sinf(2.0f * M_PI * 1000.0f * sample_time / database->sample_rate);
                } else {
                    measurement->left_ear_impulse[i] = 0.0f;
                }
                
                sample_time = (f32)i - right_delay;
                if (sample_time >= 0.0f && sample_time < 10.0f) {
                    measurement->right_ear_impulse[i] = right_gain * expf(-sample_time / 2.0f) * sinf(2.0f * M_PI * 1000.0f * sample_time / database->sample_rate);
                } else {
                    measurement->right_ear_impulse[i] = 0.0f;
                }
            }
            
            measurement->delay_left = 0.0f;
            measurement->delay_right = 0.0f;
            
            measurement_index++;
        }
    }
    
    database->loaded = true;
    printf(\"Generic HRTF dataset loaded: %d measurements\\n\", database->measurement_count);
    return true;
}

bool HRTF_LoadKEMARDataset(HRTFDatabase* database) {
    printf(\"Loading KEMAR HRTF dataset...\\n\");
    // Simplified KEMAR dataset loading
    return HRTF_LoadGenericDataset(database);  // Use generic for now
}

bool HRTF_LoadCustomDataset(HRTFDatabase* database, const char* file_path) {
    printf(\"Loading custom HRTF dataset from %s...\\n\", file_path);
    // Would load custom HRTF measurements from file
    return HRTF_LoadGenericDataset(database);  // Use generic for now
}

void HRTF_UnloadDatabase(HRTFDatabase* database) {
    if (database->measurements) {
        for (u32 i = 0; i < database->measurement_count; i++) {
            if (database->measurements[i].left_ear_impulse) {
                free(database->measurements[i].left_ear_impulse);
            }
            if (database->measurements[i].right_ear_impulse) {
                free(database->measurements[i].right_ear_impulse);
            }
        }
        free(database->measurements);
        database->measurements = NULL;
    }
    database->loaded = false;
}

HRTFMeasurement* HRTF_FindMeasurement(const HRTFDatabase* database, f32 azimuth, f32 elevation) {
    if (!database->loaded) {
        return NULL;
    }
    
    // Find closest measurement in the dataset
    f32 min_distance = 1000.0f;
    HRTFMeasurement* closest = NULL;
    
    for (u32 i = 0; i < database->measurement_count; i++) {
        HRTFMeasurement* measurement = &database->measurements[i];
        
        f32 azim_diff = fabsf(measurement->azimuth - azimuth);
        if (azim_diff > 180.0f) azim_diff = 360.0f - azim_diff;
        
        f32 elev_diff = fabsf(measurement->elevation - elevation);
        
        f32 distance = sqrtf(azim_diff * azim_diff + elev_diff * elev_diff);
        
        if (distance < min_distance) {
            min_distance = distance;
            closest = measurement;
        }
    }
    
    return closest;
}

// ✅ COMPLETED: HRTF interpolation implementation
void HRTF_InterpolateMeasurements(const HRTFDatabase* database, f32 azimuth, f32 elevation, 
                                  HRTFMeasurement* result) {
    if (!database->loaded || !result) {
        return;
    }
    
    // Find the 4 nearest measurements for bilinear interpolation
    HRTFMeasurement* nearest[4] = {NULL};
    f32 distances[4] = {1000.0f, 1000.0f, 1000.0f, 1000.0f};
    
    // Find closest measurements
    for (u32 i = 0; i < database->measurement_count; i++) {
        HRTFMeasurement* measurement = &database->measurements[i];
        
        f32 azim_diff = fabsf(measurement->azimuth - azimuth);
        if (azim_diff > 180.0f) azim_diff = 360.0f - azim_diff;
        
        f32 elev_diff = fabsf(measurement->elevation - elevation);
        f32 distance = sqrtf(azim_diff * azim_diff + elev_diff * elev_diff);
        
        // Insert into sorted list if closer than existing entries
        for (u32 j = 0; j < 4; j++) {
            if (distance < distances[j]) {
                // Shift existing entries down
                for (u32 k = 3; k > j; k--) {
                    nearest[k] = nearest[k-1];
                    distances[k] = distances[k-1];
                }
                nearest[j] = measurement;
                distances[j] = distance;
                break;
            }
        }
    }
    
    // If we have at least one measurement, use the closest
    if (nearest[0]) {
        // For simplicity, use the closest measurement if we don't have enough for interpolation
        if (!nearest[1] || distances[0] < 5.0f) {
            // Use single measurement
            result->azimuth = azimuth;
            result->elevation = elevation;
            result->impulse_length = nearest[0]->impulse_length;
            result->delay_left = nearest[0]->delay_left;
            result->delay_right = nearest[0]->delay_right;
            
            // Copy impulse responses
            if (result->left_ear_impulse && result->right_ear_impulse) {
                memcpy(result->left_ear_impulse, nearest[0]->left_ear_impulse, 
                       result->impulse_length * sizeof(f32));
                memcpy(result->right_ear_impulse, nearest[0]->right_ear_impulse, 
                       result->impulse_length * sizeof(f32));
            }
        } else {
            // Perform bilinear interpolation with available measurements
            f32 total_weight = 0.0f;
            f32 weights[4];
            
            // Calculate weights based on inverse distance
            for (u32 i = 0; i < 4 && nearest[i]; i++) {
                weights[i] = 1.0f / (distances[i] + 0.001f);  // Add small epsilon to avoid division by zero
                total_weight += weights[i];
            }
            
            // Normalize weights
            for (u32 i = 0; i < 4 && nearest[i]; i++) {
                weights[i] /= total_weight;
            }
            
            // Interpolate impulse responses
            result->azimuth = azimuth;
            result->elevation = elevation;
            result->impulse_length = nearest[0]->impulse_length;
            
            // Weighted average of delays
            result->delay_left = 0.0f;
            result->delay_right = 0.0f;
            for (u32 i = 0; i < 4 && nearest[i]; i++) {
                result->delay_left += nearest[i]->delay_left * weights[i];
                result->delay_right += nearest[i]->delay_right * weights[i];
            }
            
            // Interpolate impulse responses
            if (result->left_ear_impulse && result->right_ear_impulse) {
                for (u32 i = 0; i < result->impulse_length; i++) {
                    result->left_ear_impulse[i] = 0.0f;
                    result->right_ear_impulse[i] = 0.0f;
                    
                    for (u32 j = 0; j < 4 && nearest[j]; j++) {
                        result->left_ear_impulse[i] += nearest[j]->left_ear_impulse[i] * weights[j];
                        result->right_ear_impulse[i] += nearest[j]->right_ear_impulse[i] * weights[j];
                    }
                }
            }
        }
    }
}

// ✅ COMPLETED: Barycentric interpolation for triangular HRTF mesh
void HRTF_BarycentricInterpolation(const HRTFMeasurement* v1, const HRTFMeasurement* v2, const HRTFMeasurement* v3,
                                  f32 w1, f32 w2, f32 w3, HRTFMeasurement* result) {
    if (!v1 || !v2 || !v3 || !result) {
        return;
    }
    
    // Normalize weights
    f32 total_weight = w1 + w2 + w3;
    if (total_weight <= 0.0f) {
        return;
    }
    
    w1 /= total_weight;
    w2 /= total_weight;
    w3 /= total_weight;
    
    // Interpolate position
    result->azimuth = v1->azimuth * w1 + v2->azimuth * w2 + v3->azimuth * w3;
    result->elevation = v1->elevation * w1 + v2->elevation * w2 + v3->elevation * w3;
    
    // Use the maximum impulse length from the vertices
    result->impulse_length = v1->impulse_length;
    if (v2->impulse_length > result->impulse_length) result->impulse_length = v2->impulse_length;
    if (v3->impulse_length > result->impulse_length) result->impulse_length = v3->impulse_length;
    
    // Interpolate delays
    result->delay_left = v1->delay_left * w1 + v2->delay_left * w2 + v3->delay_left * w3;
    result->delay_right = v1->delay_right * w1 + v2->delay_right * w2 + v3->delay_right * w3;
    
    // Interpolate impulse responses
    if (result->left_ear_impulse && result->right_ear_impulse) {
        for (u32 i = 0; i < result->impulse_length; i++) {
            f32 left_sample = 0.0f;
            f32 right_sample = 0.0f;
            
            if (i < v1->impulse_length) {
                left_sample += v1->left_ear_impulse[i] * w1;
                right_sample += v1->right_ear_impulse[i] * w1;
            }
            if (i < v2->impulse_length) {
                left_sample += v2->left_ear_impulse[i] * w2;
                right_sample += v2->right_ear_impulse[i] * w2;
            }
            if (i < v3->impulse_length) {
                left_sample += v3->left_ear_impulse[i] * w3;
                right_sample += v3->right_ear_impulse[i] * w3;
            }
            
            result->left_ear_impulse[i] = left_sample;
            result->right_ear_impulse[i] = right_sample;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// SPATIAL AUDIO PROCESSOR IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

bool SpatialAudio_Init(SpatialAudioProcessor* processor, u32 sample_rate, u32 buffer_size) {
    processor->sample_rate = sample_rate;
    processor->convolution_buffer_size = buffer_size + 128;  // Extra space for convolution
    
    // Initialize HRTF database
    if (!HRTF_LoadDatabase(&processor->hrtf_database, \"data/hrtf/\", HRTF_DATASET_GENERIC)) {
        printf(\"Failed to load HRTF database\\n\");
        return false;
    }
    
    // Allocate convolution buffers
    processor->left_convolution_buffer = malloc(sizeof(f32) * processor->convolution_buffer_size);
    processor->right_convolution_buffer = malloc(sizeof(f32) * processor->convolution_buffer_size);
    
    if (!processor->left_convolution_buffer || !processor->right_convolution_buffer) {
        return false;
    }
    
    // Allocate filter states
    processor->left_filter_state = calloc(128, sizeof(f32));  // Zero-initialize
    processor->right_filter_state = calloc(128, sizeof(f32));
    
    if (!processor->left_filter_state || !processor->right_filter_state) {
        return false;
    }
    
    // Initialize spatial parameters
    processor->params.listener_position = (Vec3){0.0f, 0.0f, 0.0f};
    processor->params.listener_orientation = (Quat){0.0f, 0.0f, 0.0f, 1.0f};
    processor->params.source_position = (Vec3){1.0f, 0.0f, 0.0f};
    processor->params.gain = 1.0f;
    processor->params.occlusion_factor = 0.0f;
    processor->params.is_behind_listener = false;
    
    // Initialize distance model
    processor->min_distance = 0.5f;
    processor->max_distance = 50.0f;
    processor->rolloff_factor = 1.0f;
    
    processor->initialized = true;
    printf(\"Spatial Audio Processor initialized\\n\");
    return true;
}

void SpatialAudio_Shutdown(SpatialAudioProcessor* processor) {
    if (processor->initialized) {
        HRTF_UnloadDatabase(&processor->hrtf_database);
        
        if (processor->left_convolution_buffer) free(processor->left_convolution_buffer);
        if (processor->right_convolution_buffer) free(processor->right_convolution_buffer);
        if (processor->left_filter_state) free(processor->left_filter_state);
        if (processor->right_filter_state) free(processor->right_filter_state);
        
        processor->initialized = false;
        printf(\"Spatial Audio Processor shutdown\\n\");
    }
}

void SpatialAudio_SetListener(SpatialAudioProcessor* processor, const Vec3 position, const Quat orientation) {
    processor->params.listener_position = position;
    processor->params.listener_orientation = orientation;
}

void SpatialAudio_SetSource(SpatialAudioProcessor* processor, const Vec3 position, f32 gain) {
    processor->params.source_position = position;
    processor->params.gain = gain;
    
    // Calculate spatial parameters
    HRTF_CalculateParameters(position, processor->params.listener_position, 
                            processor->params.listener_orientation,
                            &processor->params.azimuth, &processor->params.elevation, 
                            &processor->params.distance);
    
    // Find appropriate HRTF measurements
    processor->current_hrtf_left = HRTF_FindMeasurement(&processor->hrtf_database, 
                                                       processor->params.azimuth, 
                                                       processor->params.elevation);
    processor->current_hrtf_right = processor->current_hrtf_left;  // Same measurement for both ears
    
    // Check if source is behind listener
    processor->params.is_behind_listener = SpatialAudio_IsSourceBehind(position, processor->params.listener_orientation);
}

void SpatialAudio_SetDistanceModel(SpatialAudioProcessor* processor, f32 min_distance, f32 max_distance, f32 rolloff) {
    processor->min_distance = min_distance;
    processor->max_distance = max_distance;
    processor->rolloff_factor = rolloff;
}

void SpatialAudio_SetOcclusion(SpatialAudioProcessor* processor, f32 occlusion_factor) {
    processor->params.occlusion_factor = fmaxf(0.0f, fminf(1.0f, occlusion_factor));
}

void SpatialAudio_Process(SpatialAudioProcessor* processor, const f32* input, f32* left_output, f32* right_output, u32 frames) {
    if (!processor->initialized || !processor->current_hrtf_left) {
        // No HRTF available, just copy input to both outputs
        memcpy(left_output, input, frames * sizeof(f32));
        memcpy(right_output, input, frames * sizeof(f32));
        return;
    }
    
    // Apply distance attenuation
    f32 distance_gain = SpatialAudio_CalculateDistanceAttenuation(
        processor->params.distance, 
        processor->min_distance, 
        processor->max_distance, 
        processor->rolloff_factor
    );
    
    // Apply occlusion
    f32 occlusion_gain = 1.0f - processor->params.occlusion_factor * 0.8f;  // Max 80% reduction
    
    // Apply source gain
    f32 total_gain = processor->params.gain * distance_gain * occlusion_gain;
    
    // Create temporary input buffer with gain applied
    f32* gain_input = malloc(frames * sizeof(f32));
    if (!gain_input) {
        return;
    }
    
    for (u32 i = 0; i < frames; i++) {
        gain_input[i] = input[i] * total_gain;
    }
    
    // Convolve with HRTF impulses
    HRTF_ConvolveWithImpulse(gain_input, processor->current_hrtf_left->left_ear_impulse, 
                           left_output, frames, processor->current_hrtf_left->impulse_length);
    HRTF_ConvolveWithImpulse(gain_input, processor->current_hrtf_right->right_ear_impulse, 
                           right_output, frames, processor->current_hrtf_right->impulse_length);
    
    // Apply baffling effects
    Baffling_ApplyHeadShadow(&g_baffling_model, left_output, right_output, 
                           processor->params.azimuth, frames);
    
    // Apply near-field effects if close
    if (processor->params.distance < 2.0f) {
        NearField_ApplyProximityEffect(&g_near_field_model, left_output, right_output, 
                                     processor->params.distance, frames);
    }
    
    free(gain_input);
}

// -------------------------------------------------------------------------------------------------
// HRTF PROCESSING IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

void HRTF_CalculateParameters(const Vec3 source_pos, const Vec3 listener_pos, const Quat listener_orient, f32* azimuth, f32* elevation, f32* distance) {
    // Calculate relative position
    Vec3 relative_pos = {
        source_pos.x - listener_pos.x,
        source_pos.y - listener_pos.y,
        source_pos.z - listener_pos.z
    };
    
    // Calculate distance
    *distance = sqrtf(relative_pos.x * relative_pos.x + 
                    relative_pos.y * relative_pos.y + 
                    relative_pos.z * relative_pos.z);
    
    // Normalize to unit vector
    if (*distance > 0.001f) {
        relative_pos.x /= *distance;
        relative_pos.y /= *distance;
        relative_pos.z /= *distance;
    } else {
        relative_pos = (Vec3){1.0f, 0.0f, 0.0f};  // Default direction
    }
    
    // Convert to spherical coordinates
    *azimuth = atan2f(relative_pos.x, relative_pos.z) * 180.0f / M_PI;
    *elevation = asinf(relative_pos.y) * 180.0f / M_PI;
    
    // Normalize azimuth to [0, 360)
    if (*azimuth < 0.0f) {
        *azimuth += 360.0f;
    }
}

void HRTF_ConvolveWithImpulse(const f32* input, const f32* impulse, f32* output, u32 input_frames, u32 impulse_length) {
    // Simple convolution implementation
    for (u32 i = 0; i < input_frames; i++) {
        output[i] = 0.0f;
        
        for (u32 j = 0; j < impulse_length && j <= i; j++) {
            output[i] += input[i - j] * impulse[j];
        }
    }
}

void HRTF_ApplyInterauralTimeDifference(f32* left_output, f32* right_output, f32 itd_samples, u32 frames) {
    // Apply ITD by delaying one channel relative to the other
    // This is a simplified implementation
    
    if (fabsf(itd_samples) < 1.0f) {
        return;  // ITD too small to matter
    }
    
    // For positive ITD, delay right channel
    // For negative ITD, delay left channel
    if (itd_samples > 0.0f) {
        // Delay right channel
        for (u32 i = frames - 1; i >= (u32)itd_samples; i--) {
            right_output[i] = right_output[i - (u32)itd_samples];
        }
        for (u32 i = 0; i < (u32)itd_samples && i < frames; i++) {
            right_output[i] = 0.0f;
        }
    } else {
        // Delay left channel
        u32 delay_samples = (u32)(-itd_samples);
        for (u32 i = frames - 1; i >= delay_samples; i--) {
            left_output[i] = left_output[i - delay_samples];
        }
        for (u32 i = 0; i < delay_samples && i < frames; i++) {
            left_output[i] = 0.0f;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// BAFFLING AND SHADOWING IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

void Baffling_Init(BafflingModel* model) {
    // Standard anthropometric measurements
    model->anthropometry.head_radius = 0.0875f;      // 8.75 cm
    model->anthropometry.shoulder_width = 0.43f;      // 43 cm
    model->anthropometry.torso_height = 0.6f;         // 60 cm
    model->anthropometry.pinna_offset = 0.02f;        // 2 cm
    
    model->head_shadow_gain = 0.3f;
    model->shoulder_shadow_gain = 0.2f;
    model->torso_shadow_gain = 0.1f;
    model->pinna_notch_freq = 8000.0f;
    model->pinna_boost_freq = 3000.0f;
    
    printf(\"Baffling model initialized\\n\");
}

void Baffling_ApplyHeadShadow(BafflingModel* model, f32* left_output, f32* right_output, f32 azimuth, u32 frames) {
    // Apply head shadowing effect (high-frequency attenuation)
    f32 shadow_factor = 1.0f;
    
    if (azimuth >= 90.0f && azimuth <= 270.0f) {
        // Source on the right side, shadow left ear
        shadow_factor = 1.0f - model->head_shadow_gain * fabsf(sinf((azimuth - 180.0f) * M_PI / 180.0f));
        for (u32 i = 0; i < frames; i++) {
            left_output[i] *= shadow_factor;
        }
    } else {
        // Source on the left side, shadow right ear
        shadow_factor = 1.0f - model->head_shadow_gain * fabsf(sinf(azimuth * M_PI / 180.0f));
        for (u32 i = 0; i < frames; i++) {
            right_output[i] *= shadow_factor;
        }
    }
}

void Baffling_ApplyShoulderShadow(BafflingModel* model, f32* left_output, f32* right_output, f32 azimuth, f32 elevation, u32 frames) {
    // Apply shoulder shadowing (mid-frequency attenuation)
    if (elevation < 0.0f) {  // Only for sources below ear level
        f32 shoulder_factor = 1.0f - model->shoulder_shadow_gain * (1.0f + elevation / 90.0f);
        
        if (azimuth >= 90.0f && azimuth <= 270.0f) {
            for (u32 i = 0; i < frames; i++) {
                left_output[i] *= shoulder_factor;
            }
        } else {
            for (u32 i = 0; i < frames; i++) {
                right_output[i] *= shoulder_factor;
            }
        }
    }
}

void Baffling_ApplyPinnaEffects(BafflingModel* model, f32* left_output, f32* right_output, f32 elevation, u32 frames) {
    // Apply pinna effects (spectral shaping by outer ear)
    // Simplified implementation - would use proper filters in practice
    
    f32 boost_factor = 1.0f + 0.2f * (elevation + 90.0f) / 90.0f;  // Boost for elevated sources
    
    for (u32 i = 0; i < frames; i++) {
        left_output[i] *= boost_factor;
        right_output[i] *= boost_factor;
    }
}

// -------------------------------------------------------------------------------------------------
// NEAR-FIELD EFFECTS IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

void NearField_Init(NearFieldModel* model) {
    model->near_field_threshold = 1.0f;    // 1 meter
    model->near_field_gain_boost = 0.5f;    // 50% boost
    model->low_freq_boost = 0.3f;           // 30% low-freq boost
    model->proximity_effect = 0.4f;          // Proximity effect strength
    
    printf(\"Near-field model initialized\\n\");
}

void NearField_ApplyProximityEffect(NearFieldModel* model, f32* left_output, f32* right_output, f32 distance, u32 frames) {
    if (distance < model->near_field_threshold) {
        f32 proximity_factor = 1.0f + model->near_field_gain_boost * (1.0f - distance / model->near_field_threshold);
        
        for (u32 i = 0; i < frames; i++) {
            left_output[i] *= proximity_factor;
            right_output[i] *= proximity_factor;
        }
    }
}

void NearField_ApplyLowFreqBoost(NearFieldModel* model, f32* left_output, f32* right_output, f32 distance, u32 frames) {
    if (distance < model->near_field_threshold) {
        f32 boost_factor = 1.0f + model->low_freq_boost * (1.0f - distance / model->near_field_threshold);
        
        // Apply more boost to low frequencies (simplified)
        for (u32 i = 0; i < frames; i++) {
            // This would be a proper low-pass filter in practice
            left_output[i] *= boost_factor;
            right_output[i] *= boost_factor;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
// -------------------------------------------------------------------------------------------------

f32 SpatialAudio_CalculateDistanceAttenuation(f32 distance, f32 min_distance, f32 max_distance, f32 rolloff_factor) {
    if (distance <= min_distance) {
        return 1.0f;
    }
    
    if (distance >= max_distance) {
        return 0.0f;
    }
    
    // Inverse distance attenuation
    return min_distance / (min_distance + rolloff_factor * (distance - min_distance));
}

f32 SpatialAudio_CalculateInterauralTimeDifference(f32 azimuth, f32 head_radius) {
    f32 azimuth_rad = azimuth * M_PI / 180.0f;
    return head_radius * sinf(azimuth_rad) / 343.0f;  // Speed of sound = 343 m/s
}

void SpatialAudio_CalculateInterauralLevelDifference(f32 azimuth, f32* left_gain, f32* right_gain) {
    f32 azimuth_rad = azimuth * M_PI / 180.0f;
    f32 ild_factor = fabsf(sinf(azimuth_rad)) * 0.5f;
    
    if (azimuth <= 180.0f) {
        *left_gain = 1.0f + ild_factor;
        *right_gain = 1.0f - ild_factor;
    } else {
        *left_gain = 1.0f - ild_factor;
        *right_gain = 1.0f + ild_factor;
    }
}

bool SpatialAudio_IsSourceBehind(const Vec3 source_pos, const Quat listener_orient) {
    // Transform source position to listener's local coordinate system
    // This is a simplified implementation
    
    Vec3 forward = (Vec3){0.0f, 0.0f, 1.0f};  // Assuming listener faces forward in local space
    Vec3 to_source = {
        source_pos.x,
        source_pos.y,
        source_pos.z
    };
    
    // Normalize
    f32 length = sqrtf(to_source.x * to_source.x + to_source.y * to_source.y + to_source.z * to_source.z);
    if (length > 0.001f) {
        to_source.x /= length;
        to_source.y /= length;
        to_source.z /= length;
    }
    
    // Check if behind (dot product < 0)
    f32 dot_product = to_source.x * forward.x + to_source.y * forward.y + to_source.z * forward.z;
    return dot_product < 0.0f;
}

// -------------------------------------------------------------------------------------------------
// CROSSFADE SYSTEM IMPLEMENTATION
// -------------------------------------------------------------------------------------------------

void HRTF_InitCrossfade(HRTFCrossfade* crossfade, f32 duration) {
    if (!crossfade) return;
    
    crossfade->crossfade_duration = duration;
    crossfade->current_crossfade_time = 0.0f;
    crossfade->prev_hrtf_left = NULL;
    crossfade->prev_hrtf_right = NULL;
    crossfade->target_hrtf_left = NULL;
    crossfade->target_hrtf_right = NULL;
    crossfade->crossfading = false;
}

void HRTF_StartCrossfade(HRTFCrossfade* crossfade, HRTFMeasurement* prev_left, HRTFMeasurement* prev_right, 
                        HRTFMeasurement* target_left, HRTFMeasurement* target_right) {
    if (!crossfade || !prev_left || !prev_right || !target_left || !target_right) {
        return;
    }
    
    crossfade->prev_hrtf_left = prev_left;
    crossfade->prev_hrtf_right = prev_right;
    crossfade->target_hrtf_left = target_left;
    crossfade->target_hrtf_right = target_right;
    crossfade->current_crossfade_time = 0.0f;
    crossfade->crossfading = true;
}

void HRTF_UpdateCrossfade(HRTFCrossfade* crossfade, f32* left_output, f32* right_output, u32 frames) {
    if (!crossfade || !crossfade->crossfading || !left_output || !right_output) {
        return;
    }
    
    f32 dt = (f32)frames / 48000.0f;  // Assuming 48kHz sample rate
    crossfade->current_crossfade_time += dt;
    
    if (crossfade->current_crossfade_time >= crossfade->crossfade_duration) {
        // Crossfade complete
        crossfade->crossfading = false;
        return;
    }
    
    // Calculate crossfade factor (0.0 = all prev, 1.0 = all target)
    f32 crossfade_factor = crossfade->current_crossfade_time / crossfade->crossfade_duration;
    
    // Apply smooth crossfade curve (cosine interpolation)
    crossfade_factor = (1.0f - cosf(crossfade_factor * M_PI)) * 0.5f;
    
    // Create temporary buffers for prev and target HRTF outputs
    f32* prev_left = malloc(frames * sizeof(f32));
    f32* prev_right = malloc(frames * sizeof(f32));
    f32* target_left = malloc(frames * sizeof(f32));
    f32* target_right = malloc(frames * sizeof(f32));
    
    if (!prev_left || !prev_right || !target_left || !target_right) {
        // Memory allocation failed, skip crossfade
        if (prev_left) free(prev_left);
        if (prev_right) free(prev_right);
        if (target_left) free(target_left);
        if (target_right) free(target_right);
        return;
    }
    
    // Process with previous HRTF (simplified - would need input buffer)
    // This is a placeholder - in practice you'd convolve with both HRTFs
    for (u32 i = 0; i < frames; i++) {
        prev_left[i] = left_output[i] * (1.0f - crossfade_factor);
        prev_right[i] = right_output[i] * (1.0f - crossfade_factor);
        target_left[i] = left_output[i] * crossfade_factor;
        target_right[i] = right_output[i] * crossfade_factor;
    }
    
    // Mix the crossfaded signals
    for (u32 i = 0; i < frames; i++) {
        left_output[i] = prev_left[i] + target_left[i];
        right_output[i] = prev_right[i] + target_right[i];
    }
    
    free(prev_left);
    free(prev_right);
    free(target_left);
    free(target_right);
}

bool HRTF_IsCrossfading(const HRTFCrossfade* crossfade) {
    return crossfade ? crossfade->crossfading : false;
}

// -------------------------------------------------------------------------------------------------
// MAIN HRTF INTERFACE
// -------------------------------------------------------------------------------------------------

void SpatialAudio_CreateHRTFSystem(void) {
    if (!g_hrtf_initialized) {
        // Initialize spatial audio processor
        SpatialAudio_Init(&g_hrtf_processor, 48000, 512);
        
        // Initialize baffling model
        Baffling_Init(&g_baffling_model);
        
        // Initialize crossfade system
        HRTF_InitCrossfade(&g_hrtf_crossfade, 0.1f);  // 100ms crossfade
        
        // Initialize near-field model
        NearField_Init(&g_near_field_model);
        
        g_hrtf_initialized = true;
        printf(\"HRTF Spatial Audio System created\\n\");
    }
}

void SpatialAudio_Process3DAudio(f32* input_buffer, f32* left_output, f32* right_output, u32 frames) {
    if (!g_hrtf_initialized) {
        SpatialAudio_CreateHRTFSystem();
    }
    
    // Process through HRTF system
    SpatialAudio_Process(&g_hrtf_processor, input_buffer, left_output, right_output, frames);
}
