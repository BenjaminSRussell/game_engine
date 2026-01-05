#ifndef AUDIO_PROCEDURAL_ADAPTIVE_MUSIC_H
#define AUDIO_PROCEDURAL_ADAPTIVE_MUSIC_H

#include "../../common.h"
#include <math.h>

#define MAX_MUSIC_LAYERS 8
#define MAX_MUSIC_SEGMENTS 16
#define MAX_STINGERS 8
#define MAX_BEATS_PER_BAR 8
#define MAX_BARS_PER_SEGMENT 32

typedef enum {
    MUSIC_STATE_EXPLORATION,
    MUSIC_STATE_COMBAT,
    MUSIC_STATE_SUSPENSE,
    MUSIC_STATE_VICTORY,
    MUSIC_STATE_DEFEAT,
    MUSIC_STATE_BOSS_FIGHT,
    MUSIC_STATE_STEALTH,
    MUSIC_STATE_COUNT
} MusicState;

typedef enum {
    MUSIC_LAYER_LEAD,
    MUSIC_LAYER_BASS,
    MUSIC_LAYER_PERCUSSION,
    MUSIC_LAYER_PAD,
    MUSIC_LAYER_STRINGS,
    MUSIC_LAYER_BRASS,
    MUSIC_LAYER_EFFECTS,
    MUSIC_LAYER_COUNT
} MusicLayer;

typedef enum {
    TRANSITION_CROSSFADE,
    TRANSITION_HARD_CUT,
    TRANSITION_BRIDGE,
    TRANSITION_FILTER_SWEEP,
    TRANSITION_RISER,
    TRANSITION_COUNT
} TransitionType;

typedef struct {
    f32* audio_data;
    u32 length;
    u32 sample_rate;
    f32 bpm;
    u32 beats_per_bar;
    u32 bars;
    MusicState state;
    char name[64];
    bool loaded;
} MusicSegment;

typedef struct {
    MusicLayer layer_type;
    f32* audio_data;
    u32 length;
    f32 volume;
    f32 pan;
    bool active;
    f32 fade_in_time;
    f32 fade_out_time;
    char name[64];
} MusicLayerTrack;

typedef struct {
    f32* audio_data;
    u32 length;
    f32 volume;
    f32 pitch;
    MusicState trigger_state;
    f32 priority;
    char name[64];
    bool one_shot;
} MusicStinger;

typedef struct {
    MusicState current_state;
    MusicState target_state;
    f32 intensity;           // 0.0 to 1.0
    f32 target_intensity;
    f32 intensity_decay_rate;
    
    // Current segment and layer information
    MusicSegment* current_segment;
    MusicLayerTrack layers[MAX_MUSIC_LAYERS];
    u32 active_layer_count;
    
    // Segment playlist for each state
    MusicSegment* segments[MAX_MUSIC_STATES][MAX_MUSIC_SEGMENTS];
    u32 segment_counts[MAX_MUSIC_STATES];
    u32 current_segment_index[MAX_MUSIC_STATES];
    
    // Stingers
    MusicStinger stingers[MAX_STINGERS];
    u32 stinger_count;
    
    // Timing and synchronization
    f32 current_beat_time;
    f32 beats_per_bar;
    f32 bpm;
    u32 current_bar;
    u32 current_beat;
    f32 bar_duration;
    f32 beat_duration;
    
    // Transition system
    TransitionType transition_type;
    f32 transition_time;
    f32 transition_duration;
    MusicSegment* bridge_segment;
    
    // Vertical re-orchestration (layer fading)
    f32 layer_volumes[MAX_MUSIC_LAYERS];
    f32 target_layer_volumes[MAX_MUSIC_LAYERS];
    f32 layer_fade_speed;
    
    // Horizontal re-sequencing (segment switching)
    f32 next_segment_time;
    bool pending_segment_change;
    u32 next_segment_index;
    
    // Processing buffers
    f32* mix_buffer;
    f32* layer_buffers[MAX_MUSIC_LAYERS];
    f32* stinger_buffer;
    u32 buffer_size;
    u32 sample_rate;
    
    // Audio system integration
    struct {
        u32 source_id;
        bool playing;
        f32 master_volume;
        f32 ducking_amount;
    } audio_state;
    
    bool initialized;
} AdaptiveMusicSystem;

// Adaptive music system lifecycle
AdaptiveMusicSystem* adaptive_music_create(u32 sample_rate, u32 buffer_size);
void adaptive_music_destroy(AdaptiveMusicSystem* system);

// State management
void adaptive_music_set_state(AdaptiveMusicSystem* system, MusicState state);
void adaptive_music_set_intensity(AdaptiveMusicSystem* system, f32 intensity);
void adaptive_music_trigger_stinger(AdaptiveMusicSystem* system, const char* stinger_name);

// Segment management
void adaptive_music_add_segment(AdaptiveMusicSystem* system, MusicSegment* segment);
void adaptive_music_load_segment_from_file(AdaptiveMusicSystem* system, const char* filepath, 
                                           MusicState state);
void adaptive_music_set_segment_playlist(AdaptiveMusicSystem* system, MusicState state, 
                                         MusicSegment** segments, u32 count);

// Layer management (Vertical Re-orchestration)
void adaptive_music_add_layer(AdaptiveMusicSystem* system, MusicLayerTrack* layer);
void adaptive_music_set_layer_volume(AdaptiveMusicSystem* system, MusicLayer layer, f32 volume);
void adaptive_music_fade_layer_in(AdaptiveMusicSystem* system, MusicLayer layer, f32 fade_time);
void adaptive_music_fade_layer_out(AdaptiveMusicSystem* system, MusicLayer layer, f32 fade_time);
void adaptive_music_update_vertical_orchestration(AdaptiveMusicSystem* system, f32 delta_time);

// Horizontal re-sequencing
void adaptive_music_switch_segment_at_bar(AdaptiveMusicSystem* system, u32 segment_index);
void adaptive_music_switch_segment_at_beat(AdaptiveMusicSystem* system, u32 segment_index);
void adaptive_music_randomize_next_segment(AdaptiveMusicSystem* system);
void adaptive_music_update_horizontal_resequencing(AdaptiveMusicSystem* system, f32 current_time);

// Stingers
void adaptive_music_add_stinger(AdaptiveMusicSystem* system, MusicStinger* stinger);
void adaptive_music_trigger_state_stinger(AdaptiveMusicSystem* system, MusicState state);
void adaptive_music_process_stingers(AdaptiveMusicSystem* system, f32* output, u32 frame_count);

// BPM synchronization
void adaptive_music_set_bpm(AdaptiveMusicSystem* system, f32 bpm);
void adaptive_music_update_timing(AdaptiveMusicSystem* system, f32 delta_time);
f32 adaptive_music_get_beat_progress(const AdaptiveMusicSystem* system);
f32 adaptive_music_get_bar_progress(const AdaptiveMusicSystem* system);
u32 adaptive_music_get_current_beat(const AdaptiveMusicSystem* system);
u32 adaptive_music_get_current_bar(const AdaptiveMusicSystem* system);

// Transitions
void adaptive_music_set_transition_type(AdaptiveMusicSystem* system, TransitionType type);
void adaptive_music_set_transition_duration(AdaptiveMusicSystem* system, f32 duration);
void adaptive_music_start_transition(AdaptiveMusicSystem* system, MusicState new_state);
void adaptive_music_process_transition(AdaptiveMusicSystem* system, f32* output, u32 frame_count);

// Intensity logic
void adaptive_music_update_intensity(AdaptiveMusicSystem* system, f32 delta_time);
void adaptive_music_set_intensity_decay_rate(AdaptiveMusicSystem* system, f32 decay_rate);
void adaptive_music_trigger_intensity_spike(AdaptiveMusicSystem* system, f32 amount, f32 duration);

// Playlist management
void adaptive_music_create_exploration_playlist(AdaptiveMusicSystem* system);
void adaptive_music_randomize_exploration_tracks(AdaptiveMusicSystem* system);

// Ducking (for dialogue)
void adaptive_music_set_ducking_amount(AdaptiveMusicSystem* system, f32 ducking);
void adaptive_music_apply_ducking(AdaptiveMusicSystem* system, f32* buffer, u32 frame_count);

// Processing
void adaptive_music_process(AdaptiveMusicSystem* system, f32* output, u32 frame_count);
void adaptive_music_process_stereo(AdaptiveMusicSystem* system, f32* left, f32* right, u32 frame_count);

// Utilities
const char* adaptive_music_get_state_name(MusicState state);
const char* adaptive_music_get_layer_name(MusicLayer layer);
const char* adaptive_music_get_transition_name(TransitionType type);
void adaptive_music_reset(AdaptiveMusicSystem* system);

// Audio system integration
void adaptive_music_play(AdaptiveMusicSystem* system);
void adaptive_music_pause(AdaptiveMusicSystem* system);
void adaptive_music_stop(AdaptiveMusicSystem* system);
void adaptive_music_set_master_volume(AdaptiveMusicSystem* system, f32 volume);

#endif // AUDIO_PROCEDURAL_ADAPTIVE_MUSIC_H
