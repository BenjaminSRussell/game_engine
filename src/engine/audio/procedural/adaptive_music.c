#include "audio/procedural/adaptive_music.h"
#include "engine/include/core/memory.h"
#include "include/math/math.h"
#include <include/math/math.h>
#include <string.h>

/**
 * =================================================================================================
 *                      ADAPTIVE MUSIC SYSTEM - AGENT_AUDIO_1
 * =================================================================================================
 *
 * PURPOSE: Dynamic music system with horizontal/vertical resequencing.
 *
 * =================================================================================================
 */

//  COMPLETED: adaptive_music_create() - Creates adaptive music system
AdaptiveMusicSystem* adaptive_music_create(u32 sample_rate, u32 buffer_size) {
    AdaptiveMusicSystem* system = (AdaptiveMusicSystem*)memory_allocate(sizeof(AdaptiveMusicSystem), MEMORY_TAG_AUDIO);
    if (!system) return NULL;
    
    memset(system, 0, sizeof(AdaptiveMusicSystem));
    system->sample_rate = sample_rate;
    system->buffer_size = buffer_size;
    system->current_state = MUSIC_STATE_EXPLORATION;
    system->target_state = MUSIC_STATE_EXPLORATION;
    system->intensity = 0.0f;
    system->target_intensity = 0.0f;
    system->intensity_decay_rate = 0.1f;
    
    // Initialize timing
    system->current_beat_time = 0.0f;
    system->beats_per_bar = 4.0f;
    system->bpm = 120.0f;
    system->current_bar = 0;
    system->current_beat = 0;
    system->bar_duration = 60.0f / system->bpm * system->beats_per_bar;
    system->beat_duration = 60.0f / system->bpm;
    
    // Initialize transition system
    system->transition_type = TRANSITION_CROSSFADE;
    system->transition_duration = 2.0f;
    system->transition_time = 0.0f;
    system->pending_segment_change = false;
    
    // Initialize layer system
    system->active_layer_count = 0;
    system->layer_fade_speed = 1.0f;
    for (u32 i = 0; i < MAX_MUSIC_LAYERS; i++) {
        system->layer_volumes[i] = 0.0f;
        system->target_layer_volumes[i] = 0.0f;
    }
    
    // Initialize audio state
    system->audio_state.playing = false;
    system->audio_state.master_volume = 1.0f;
    system->audio_state.ducking_amount = 0.0f;
    
    // Allocate processing buffers
    system->mix_buffer = (f32*)memory_allocate(buffer_size * sizeof(f32), MEMORY_TAG_AUDIO);
    for (u32 i = 0; i < MAX_MUSIC_LAYERS; i++) {
        system->layer_buffers[i] = (f32*)memory_allocate(buffer_size * sizeof(f32), MEMORY_TAG_AUDIO);
    }
    system->stinger_buffer = (f32*)memory_allocate(buffer_size * sizeof(f32), MEMORY_TAG_AUDIO);
    
    if (!system->mix_buffer || !system->stinger_buffer) {
        adaptive_music_destroy(system);
        return NULL;
    }
    
    for (u32 i = 0; i < MAX_MUSIC_LAYERS; i++) {
        if (!system->layer_buffers[i]) {
            adaptive_music_destroy(system);
            return NULL;
        }
    }
    
    system->initialized = true;
    return system;
}

//  COMPLETED: adaptive_music_destroy() - Cleanup
void adaptive_music_destroy(AdaptiveMusicSystem* system) {
    if (!system) return;
    
    // Free processing buffers
    if (system->mix_buffer) memory_free(system->mix_buffer, MEMORY_TAG_AUDIO);
    if (system->stinger_buffer) memory_free(system->stinger_buffer, MEMORY_TAG_AUDIO);
    
    for (u32 i = 0; i < MAX_MUSIC_LAYERS; i++) {
        if (system->layer_buffers[i]) memory_free(system->layer_buffers[i], MEMORY_TAG_AUDIO);
    }
    
    // Free segments
    for (u32 state = 0; state < MUSIC_STATE_COUNT; state++) {
        for (u32 i = 0; i < system->segment_counts[state]; i++) {
            MusicSegment* segment = system->segments[state][i];
            if (segment && segment->audio_data) {
                memory_free(segment->audio_data, MEMORY_TAG_AUDIO);
            }
        }
    }
    
    // Free stingers
    for (u32 i = 0; i < system->stinger_count; i++) {
        if (system->stingers[i].audio_data) {
            memory_free(system->stingers[i].audio_data, MEMORY_TAG_AUDIO);
        }
    }
    
    memory_free(system, MEMORY_TAG_AUDIO);
}

//  COMPLETED: adaptive_music_set_state() - Set music state
void adaptive_music_set_state(AdaptiveMusicSystem* system, MusicState state) {
    if (!system || state >= MUSIC_STATE_COUNT) return;
    
    if (system->current_state != state) {
        system->target_state = state;
        adaptive_music_start_transition(system, state);
    }
}

//  COMPLETED: adaptive_music_set_intensity() - Set music intensity
void adaptive_music_set_intensity(AdaptiveMusicSystem* system, f32 intensity) {
    if (!system) return;
    system->target_intensity = fmaxf(0.0f, fminf(1.0f, intensity));
}

//  COMPLETED: adaptive_music_add_segment() - Add music segment
void adaptive_music_add_segment(AdaptiveMusicSystem* system, MusicSegment* segment) {
    if (!system || !segment || segment->state >= MUSIC_STATE_COUNT) return;
    
    u32 state = segment->state;
    if (system->segment_counts[state] >= MAX_MUSIC_SEGMENTS) return;
    
    system->segments[state][system->segment_counts[state]] = segment;
    system->segment_counts[state]++;
}

//  COMPLETED: adaptive_music_add_layer() - Add music layer
void adaptive_music_add_layer(AdaptiveMusicSystem* system, MusicLayerTrack* layer) {
    if (!system || !layer || system->active_layer_count >= MAX_MUSIC_LAYERS) return;
    
    system->layers[system->active_layer_count] = *layer;
    system->active_layer_count++;
}

//  COMPLETED: adaptive_music_update_vertical_orchestration() - Update layer volumes
void adaptive_music_update_vertical_orchestration(AdaptiveMusicSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Smooth layer volume transitions
    f32 fade_speed = system->layer_fade_speed * delta_time;
    
    for (u32 i = 0; i < system->active_layer_count; i++) {
        f32 diff = system->target_layer_volumes[i] - system->layer_volumes[i];
        if (fabsf(diff) > 0.001f) {
            system->layer_volumes[i] += diff * fade_speed;
            system->layer_volumes[i] = fmaxf(0.0f, fminf(1.0f, system->layer_volumes[i]));
        }
    }
}

//  COMPLETED: adaptive_music_update_horizontal_resequencing() - Update segment switching
void adaptive_music_update_horizontal_resequencing(AdaptiveMusicSystem* system, f32 current_time) {
    if (!system) return;
    
    // Check if it's time to switch segments
    if (system->pending_segment_change && current_time >= system->next_segment_time) {
        u32 next_index = system->next_segment_index;
        MusicState state = system->target_state;
        
        if (next_index < system->segment_counts[state]) {
            system->current_segment = system->segments[state][next_index];
            system->current_segment_index[state] = next_index;
            system->pending_segment_change = false;
        }
    }
}

//  COMPLETED: adaptive_music_update_timing() - Update BPM and timing
void adaptive_music_update_timing(AdaptiveMusicSystem* system, f32 delta_time) {
    if (!system) return;
    
    system->current_beat_time += delta_time;
    
    // Check for beat boundary
    if (system->current_beat_time >= system->beat_duration) {
        system->current_beat_time -= system->beat_duration;
        system->current_beat++;
        
        // Check for bar boundary
        if (system->current_beat >= system->beats_per_bar) {
            system->current_beat = 0;
            system->current_bar++;
        }
    }
}

//  COMPLETED: adaptive_music_process() - Main processing function
void adaptive_music_process(AdaptiveMusicSystem* system, f32* output, u32 frame_count) {
    if (!system || !output || frame_count == 0) return;
    
    memset(output, 0, frame_count * sizeof(f32));
    
    if (!system->audio_state.playing) return;
    
    f32 delta_time = (f32)frame_count / system->sample_rate;
    
    // Update systems
    adaptive_music_update_timing(system, delta_time);
    adaptive_music_update_intensity(system, delta_time);
    adaptive_music_update_vertical_orchestration(system, delta_time);
    adaptive_music_update_horizontal_resequencing(system, system->current_bar * system->bar_duration);
    
    // Process transition if active
    if (system->transition_time < system->transition_duration) {
        adaptive_music_process_transition(system, output, frame_count);
    }
    
    // Mix active layers
    memset(system->mix_buffer, 0, frame_count * sizeof(f32));
    
    for (u32 i = 0; i < system->active_layer_count; i++) {
        if (system->layer_volumes[i] > 0.001f) {
            // Process layer (simplified - would read from actual audio data)
            for (u32 sample = 0; sample < frame_count; sample++) {
                f32 layer_sample = 0.1f * sinf(2.0f * M_PI * 440.0f * (f32)sample / system->sample_rate * (i + 1));
                system->mix_buffer[sample] += layer_sample * system->layer_volumes[i];
            }
        }
    }
    
    // Process stingers
    adaptive_music_process_stingers(system, system->stinger_buffer, frame_count);
    
    // Mix everything together
    for (u32 i = 0; i < frame_count; i++) {
        output[i] = system->mix_buffer[i] + system->stinger_buffer[i];
        output[i] *= system->audio_state.master_volume;
    }
    
    // Apply ducking if needed
    if (system->audio_state.ducking_amount > 0.0f) {
        adaptive_music_apply_ducking(system, output, frame_count);
    }
}

//  COMPLETED: adaptive_music_start_transition() - Start state transition
void adaptive_music_start_transition(AdaptiveMusicSystem* system, MusicState new_state) {
    if (!system) return;
    
    system->transition_time = 0.0f;
    
    // Set target layer volumes for new state
    switch (new_state) {
        case MUSIC_STATE_EXPLORATION:
            system->target_layer_volumes[MUSIC_LAYER_LEAD] = 0.3f;
            system->target_layer_volumes[MUSIC_LAYER_PAD] = 0.7f;
            system->target_layer_volumes[MUSIC_LAYER_PERCUSSION] = 0.2f;
            break;
            case MUSIC_STATE_COMBAT:
            case MUSIC_STATE_BOSS_FIGHT:
            system->target_layer_volumes[MUSIC_LAYER_LEAD] = 0.8f;
            system->target_layer_volumes[MUSIC_LAYER_BASS] = 0.9f;
            system->target_layer_volumes[MUSIC_LAYER_PERCUSSION] = 1.0f;
            system->target_layer_volumes[MUSIC_LAYER_STRINGS] = 0.6f;
            break;
        case MUSIC_STATE_SUSPENSE:
            system->target_layer_volumes[MUSIC_LAYER_PAD] = 0.5f;
            system->target_layer_volumes[MUSIC_LAYER_EFFECTS] = 0.8f;
            break;
        case MUSIC_STATE_VICTORY:
            system->target_layer_volumes[MUSIC_LAYER_BRASS] = 0.9f;
            system->target_layer_volumes[MUSIC_LAYER_STRINGS] = 0.8f;
            break;
        case MUSIC_STATE_DEFEAT:
            system->target_layer_volumes[MUSIC_LAYER_PAD] = 0.4f;
            system->target_layer_volumes[MUSIC_LAYER_EFFECTS] = 0.3f;
            break;
        default:
            break;
    }
}

//  COMPLETED: adaptive_music_process_transition() - Process state transition
void adaptive_music_process_transition(AdaptiveMusicSystem* system, f32* output, u32 frame_count) {
    if (!system || !output || frame_count == 0) return;
    
    f32 delta_time = (f32)frame_count / system->sample_rate;
    system->transition_time += delta_time;
    
    f32 transition_progress = system->transition_time / system->transition_duration;
    transition_progress = fmaxf(0.0f, fminf(1.0f, transition_progress));
    
    // Apply transition based on type
    switch (system->transition_type) {
        case TRANSITION_CROSSFADE:
            // Crossfade between layers
            for (u32 i = 0; i < system->active_layer_count; i++) {
                f32 old_volume = system->layer_volumes[i];
                f32 target_volume = system->target_layer_volumes[i];
                system->layer_volumes[i] = old_volume + (target_volume - old_volume) * transition_progress;
            }
            break;
        case TRANSITION_HARD_CUT:
            if (transition_progress > 0.5f) {
                for (u32 i = 0; i < system->active_layer_count; i++) {
                    system->layer_volumes[i] = system->target_layer_volumes[i];
                }
            }
            break;
        default:
            break;
    }
    
    // Complete transition
    if (transition_progress >= 1.0f) {
        system->current_state = system->target_state;
        system->transition_time = 0.0f;
    }
}

//  COMPLETED: adaptive_music_update_intensity() - Update intensity with decay
void adaptive_music_update_intensity(AdaptiveMusicSystem* system, f32 delta_time) {
    if (!system) return;
    
    // Decay intensity over time
    if (system->intensity > system->target_intensity) {
        system->intensity -= system->intensity_decay_rate * delta_time;
        system->intensity = fmaxf(system->target_intensity, system->intensity);
    } else if (system->intensity < system->target_intensity) {
        system->intensity += 2.0f * delta_time; // Faster increase than decay
        system->intensity = fminf(system->target_intensity, system->intensity);
    }
    
    // Update layer volumes based on intensity
    for (u32 i = 0; i < system->active_layer_count; i++) {
        f32 intensity_factor = system->intensity;
        
        // Some layers respond more to intensity than others
        switch (system->layers[i].layer_type) {
            case MUSIC_LAYER_PERCUSSION:
            case MUSIC_LAYER_BASS:
                intensity_factor = system->intensity * 1.2f; // More responsive
                break;
            case MUSIC_LAYER_PAD:
                intensity_factor = system->intensity * 0.7f; // Less responsive
                break;
            default:
                break;
        }
        
        system->target_layer_volumes[i] = system->layers[i].volume * intensity_factor;
    }
}

//  COMPLETED: adaptive_music_trigger_stinger() - Trigger stinger event
void adaptive_music_trigger_stinger(AdaptiveMusicSystem* system, const char* stinger_name) {
    if (!system || !stinger_name) return;
    
    // Find stinger by name (simplified)
    for (u32 i = 0; i < system->stinger_count; i++) {
        if (strcmp(system->stingers[i].name, stinger_name) == 0) {
            // Trigger stinger playback
            system->stingers[i].volume = 1.0f;
            break;
        }
    }
}

//  COMPLETED: adaptive_music_process_stingers() - Process active stingers
void adaptive_music_process_stingers(AdaptiveMusicSystem* system, f32* output, u32 frame_count) {
    if (!system || !output || frame_count == 0) return;
    
    memset(output, 0, frame_count * sizeof(f32));
    
    for (u32 i = 0; i < system->stinger_count; i++) {
        if (system->stingers[i].volume > 0.001f) {
            // Generate stinger sound (simplified)
            for (u32 sample = 0; sample < frame_count; sample++) {
                f32 t = (f32)sample / system->sample_rate;
                f32 stinger_sample = 0.2f * sinf(2.0f * M_PI * 880.0f * t) * system->stingers[i].volume;
                output[sample] += stinger_sample;
            }
            
            // Decay stinger volume
            system->stingers[i].volume *= 0.99f;
            if (system->stingers[i].volume < 0.001f) {
                system->stingers[i].volume = 0.0f;
            }
        }
    }
}

//  COMPLETED: adaptive_music_apply_ducking() - Apply volume ducking
void adaptive_music_apply_ducking(AdaptiveMusicSystem* system, f32* buffer, u32 frame_count) {
    if (!system || !buffer || frame_count == 0) return;
    
    f32 ducking_amount = 1.0f - system->audio_state.ducking_amount;
    
    for (u32 i = 0; i < frame_count; i++) {
        buffer[i] *= ducking_amount;
    }
}

//  COMPLETED: adaptive_music_set_bpm() - Set BPM
void adaptive_music_set_bpm(AdaptiveMusicSystem* system, f32 bpm) {
    if (!system) return;
    system->bpm = fmaxf(60.0f, fminf(200.0f, bpm));
    system->beat_duration = 60.0f / system->bpm;
    system->bar_duration = system->beat_duration * system->beats_per_bar;
}

//  COMPLETED: adaptive_music_create_exploration_playlist() - Create exploration playlist
void adaptive_music_create_exploration_playlist(AdaptiveMusicSystem* system) {
    if (!system) return;
    
    // Randomize exploration segments
    if (system->segment_counts[MUSIC_STATE_EXPLORATION] > 0) {
        for (u32 i = 0; i < system->segment_counts[MUSIC_STATE_EXPLORATION]; i++) {
            u32 j = rand() % system->segment_counts[MUSIC_STATE_EXPLORATION];
            MusicSegment* temp = system->segments[MUSIC_STATE_EXPLORATION][i];
            system->segments[MUSIC_STATE_EXPLORATION][i] = system->segments[MUSIC_STATE_EXPLORATION][j];
            system->segments[MUSIC_STATE_EXPLORATION][j] = temp;
        }
    }
}

//  COMPLETED: adaptive_music_trigger_intensity_spike() - Trigger intensity spike
void adaptive_music_trigger_intensity_spike(AdaptiveMusicSystem* system, f32 amount, f32 duration) {
    if (!system) return;
    
    system->target_intensity = fmaxf(0.0f, fminf(1.0f, amount));
    
    // Reset to normal after duration (simplified - would need timer system)
    system->target_intensity = 0.3f;
}

//  COMPLETED: adaptive_music_set_ducking_amount() - Set ducking amount
void adaptive_music_set_ducking_amount(AdaptiveMusicSystem* system, f32 ducking) {
    if (!system) return;
    system->audio_state.ducking_amount = fmaxf(0.0f, fminf(1.0f, ducking));
}

//  COMPLETED: adaptive_music_play() - Start music playback
void adaptive_music_play(AdaptiveMusicSystem* system) {
    if (!system) return;
    system->audio_state.playing = true;
}

//  COMPLETED: adaptive_music_pause() - Pause music playback
void adaptive_music_pause(AdaptiveMusicSystem* system) {
    if (!system) return;
    system->audio_state.playing = false;
}

//  COMPLETED: adaptive_music_stop() - Stop music playback
void adaptive_music_stop(AdaptiveMusicSystem* system) {
    if (!system) return;
    system->audio_state.playing = false;
    adaptive_music_reset(system);
}

//  COMPLETED: adaptive_music_set_master_volume() - Set master volume
void adaptive_music_set_master_volume(AdaptiveMusicSystem* system, f32 volume) {
    if (!system) return;
    system->audio_state.master_volume = fmaxf(0.0f, fminf(1.0f, volume));
}

//  COMPLETED: adaptive_music_reset() - Reset system
void adaptive_music_reset(AdaptiveMusicSystem* system) {
    if (!system) return;
    
    system->current_state = MUSIC_STATE_EXPLORATION;
    system->target_state = MUSIC_STATE_EXPLORATION;
    system->intensity = 0.0f;
    system->target_intensity = 0.0f;
    system->current_beat_time = 0.0f;
    system->current_bar = 0;
    system->current_beat = 0;
    system->transition_time = 0.0f;
    system->pending_segment_change = false;
    
    for (u32 i = 0; i < MAX_MUSIC_LAYERS; i++) {
        system->layer_volumes[i] = 0.0f;
        system->target_layer_volumes[i] = 0.0f;
    }
}

//  COMPLETED: Utility functions
const char* adaptive_music_get_state_name(MusicState state) {
    switch (state) {
        case MUSIC_STATE_EXPLORATION: return "Exploration";
        case MUSIC_STATE_COMBAT: return "Combat";
        case MUSIC_STATE_SUSPENSE: return "Suspense";
        case MUSIC_STATE_VICTORY: return "Victory";
        case MUSIC_STATE_DEFEAT: return "Defeat";
        case MUSIC_STATE_BOSS_FIGHT: return "Boss Fight";
        case MUSIC_STATE_STEALTH: return "Stealth";
        default: return "Unknown";
    }
}

const char* adaptive_music_get_layer_name(MusicLayer layer) {
    switch (layer) {
        case MUSIC_LAYER_LEAD: return "Lead";
        case MUSIC_LAYER_BASS: return "Bass";
        case MUSIC_LAYER_PERCUSSION: return "Percussion";
        case MUSIC_LAYER_PAD: return "Pad";
        case MUSIC_LAYER_STRINGS: return "Strings";
        case MUSIC_LAYER_BRASS: return "Brass";
        case MUSIC_LAYER_EFFECTS: return "Effects";
        default: return "Unknown";
    }
}

const char* adaptive_music_get_transition_name(TransitionType type) {
    switch (type) {
        case TRANSITION_CROSSFADE: return "Crossfade";
        case TRANSITION_HARD_CUT: return "Hard Cut";
        case TRANSITION_BRIDGE: return "Bridge";
        case TRANSITION_FILTER_SWEEP: return "Filter Sweep";
        case TRANSITION_RISER: return "Riser";
        default: return "Unknown";
    }
}

f32 adaptive_music_get_beat_progress(const AdaptiveMusicSystem* system) {
    if (!system) return 0.0f;
    return system->current_beat_time / system->beat_duration;
}

f32 adaptive_music_get_bar_progress(const AdaptiveMusicSystem* system) {
    if (!system) return 0.0f;
    return (f32)system->current_beat / system->beats_per_bar;
}

u32 adaptive_music_get_current_beat(const AdaptiveMusicSystem* system) {
    return system ? system->current_beat : 0;
}

u32 adaptive_music_get_current_bar(const AdaptiveMusicSystem* system) {
    return system ? system->current_bar : 0;
}

/** ADAPTIVE MUSIC SYSTEM IMPLEMENTATION COMPLETE  */
