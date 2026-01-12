#ifndef REPLAY_SYSTEM_H
#define REPLAY_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "math/vec3.h"
#include "core/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// Replay System Limits
#define MAX_REPLAY_FRAMES 180000  // 1 hour at 30 FPS
#define MAX_REPLAY_ENTITIES 1024
#define MAX_REPLAY_EVENTS 65536
#define MAX_REPLAY_MARKERS 1024
#define MAX_REPLAY_CAMERAS 16
#define MAX_REPLAY_HIGHLIGHTS 256
#define MAX_REPLAY_SNAPSHOTS 720  // Every 5 seconds for 1 hour
#define MAX_REPLAY_FILE_SIZE (1024 * 1024 * 1024)  // 1GB max
#define MAX_REPLAY_NAME_LENGTH 128
#define MAX_REPLAY_DESCRIPTION_LENGTH 512
#define MAX_REPLAY_TAGS 16
#define MAX_REPLAY_EXPORT_FORMATS 8

// Replay Frame Types
typedef enum {
    FRAME_TYPE_INPUT = 0,
    FRAME_TYPE_STATE,
    FRAME_TYPE_EVENT,
    FRAME_TYPE_CAMERA,
    FRAME_TYPE_AUDIO,
    FRAME_TYPE_VISUAL
} ReplayFrameType;

// Playback States
typedef enum {
    PLAYBACK_STATE_STOPPED = 0,
    PLAYBACK_STATE_PLAYING,
    PLAYBACK_STATE_PAUSED,
    PLAYBACK_STATE_SEEKING,
    PLAYBACK_STATE_RECORDING
} PlaybackState;

// Camera Types
typedef enum {
    CAMERA_TYPE_FREE = 0,
    CAMERA_TYPE_ENTITY,
    CAMERA_TYPE_CINEMATIC,
    CAMERA_TYPE_FIRST_PERSON,
    CAMERA_TYPE_THIRD_PERSON,
    CAMERA_TYPE_ORBIT,
    CAMERA_TYPE_TRACKING
} CameraType;

// Highlight Types
typedef enum {
    HIGHLIGHT_TYPE_KILL = 0,
    HIGHLIGHT_TYPE_DEATH,
    HIGHLIGHT_TYPE_OBJECTIVE,
    HIGHLIGHT_TYPE_ACHIEVEMENT,
    HIGHLIGHT_TYPE_STUNT,
    HIGHLIGHT_TYPE_MULTI_KILL,
    HIGHLIGHT_TYPE_HEADSHOT,
    HIGHLIGHT_TYPE_EXPLOSION,
    HIGHLIGHT_TYPE_CAPUTURE,
    HIGHLIGHT_TYPE_SCORE
} HighlightType;

// Export Formats
typedef enum {
    EXPORT_FORMAT_MP4 = 0,
    EXPORT_FORMAT_WEBM,
    EXPORT_FORMAT_GIF,
    EXPORT_FORMAT_AVI,
    EXPORT_FORMAT_MOV,
    EXPORT_FORMAT_REPLAY_FILE,
    EXPORT_FORMAT_HIGHLIGHTS,
    EXPORT_FORMAT_STATISTICS
} ExportFormat;

// Input Frame Data
typedef struct {
    uint32_t entity_id;
    uint16_t buttons;           // Bitmask of pressed buttons
    uint16_t mouse_buttons;      // Bitmask of mouse buttons
    float mouse_x, mouse_y;      // Normalized mouse position
    float mouse_delta_x, mouse_delta_y;
    float analog_x, analog_y;     // Analog stick position
    float analog_trigger_left, analog_trigger_right;
    uint8_t key_states[32];      // Keyboard state array
} InputFrame;

// State Frame Data
typedef struct {
    uint32_t entity_id;
    Vec3 position;
    Vec3 velocity;
    Vec3 rotation;
    Vec3 angular_velocity;
    uint32_t health;
    uint32_t armor;
    uint32_t ammo;
    uint32_t special;
    uint8_t state_flags;         // Crouching, sprinting, etc.
    uint8_t weapon_id;
    float animation_time;
} StateFrame;

// Event Frame Data
typedef struct {
    uint32_t event_type;
    uint32_t source_entity_id;
    uint32_t target_entity_id;
    Vec3 position;
    Vec3 direction;
    float magnitude;
    uint32_t timestamp;
    uint8_t data[64];           // Event-specific data
} EventFrame;

// Camera Frame Data
typedef struct {
    CameraType type;
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fov;
    float roll;
    float zoom;
    uint32_t follow_entity_id;
    bool is_smooth;
    float transition_time;
} CameraFrame;

// Audio Frame Data
typedef struct {
    uint32_t sound_id;
    Vec3 position;
    float volume;
    float pitch;
    bool is_3d;
    float radius;
    uint32_t entity_id;
} AudioFrame;

// Visual Frame Data
typedef struct {
    uint32_t effect_type;
    Vec3 position;
    Vec3 direction;
    float intensity;
    float duration;
    uint32_t entity_id;
    uint8_t color[4];
    float scale;
} VisualFrame;

// Replay Frame
typedef struct {
    uint32_t frame_number;
    uint32_t timestamp_ms;
    ReplayFrameType type;
    uint32_t data_size;
    union {
        InputFrame input;
        StateFrame state;
        EventFrame event;
        CameraFrame camera;
        AudioFrame audio;
        VisualFrame visual;
    } data;
} ReplayFrame;

// Replay Snapshot
typedef struct {
    uint32_t timestamp_ms;
    uint32_t frame_number;
    uint32_t entity_count;
    
    // Complete entity states at this point
    struct {
        uint32_t entity_id;
        StateFrame state;
        uint8_t custom_data[256];
    } entities[MAX_REPLAY_ENTITIES];
    
    // Global state
    uint32_t game_time;
    uint32_t round_number;
    uint8_t weather_state;
    float global_lighting[3];
} ReplaySnapshot;

// Replay Marker
typedef struct {
    uint32_t timestamp_ms;
    uint32_t frame_number;
    HighlightType type;
    char description[128];
    uint32_t entity_id;
    Vec3 position;
    float importance;  // 0.0 to 1.0
    bool is_auto_generated;
} ReplayMarker;

// Replay Camera
typedef struct {
    CameraType type;
    char name[64];
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fov;
    float duration;
    bool is_looping;
    uint32_t keyframe_count;
    struct {
        uint32_t timestamp_ms;
        Vec3 position;
        Vec3 target;
        Vec3 up;
        float fov;
    } keyframes[64];
} ReplayCamera;

// Replay Highlight
typedef struct {
    uint32_t start_frame;
    uint32_t end_frame;
    HighlightType type;
    char title[128];
    char description[256];
    uint32_t primary_entity_id;
    uint32_t secondary_entity_id;
    float score;              // Importance score
    bool is_user_created;
} ReplayHighlight;

// Replay Statistics
typedef struct {
    uint32_t total_frames;
    uint32_t total_duration_ms;
    uint32_t entity_count;
    uint32_t event_count;
    uint32_t marker_count;
    uint32_t highlight_count;
    uint32_t file_size_bytes;
    float average_fps;
    uint32_t peak_entity_count;
    uint32_t peak_events_per_second;
} ReplayStatistics;

// Replay Metadata
typedef struct {
    char name[MAX_REPLAY_NAME_LENGTH];
    char description[MAX_REPLAY_DESCRIPTION_LENGTH];
    char version[32];
    char game_version[32];
    char map_name[64];
    char game_mode[32];
    uint32_t creation_time;
    uint32_t duration_ms;
    uint32_t file_size;
    char tags[MAX_REPLAY_TAGS][32];
    uint8_t tag_count;
    bool is_public;
    bool is_compressed;
    bool is_encrypted;
    uint32_t checksum;
} ReplayMetadata;

// Replay Recording Context
typedef struct {
    bool is_recording;
    uint32_t start_time_ms;
    uint32_t current_frame;
    uint32_t frame_skip_counter;
    float recording_fps;
    bool auto_markers_enabled;
    uint32_t snapshot_interval_ms;
    uint32_t last_snapshot_time;
    uint32_t rolling_buffer_size;
    bool use_compression;
    uint8_t quality_level;  // 0-5
} RecordingContext;

// Replay Playback Context
typedef struct {
    PlaybackState state;
    uint32_t current_frame;
    uint32_t target_frame;
    float playback_speed;
    bool is_looping;
    uint32_t loop_start_frame;
    uint32_t loop_end_frame;
    bool auto_camera_enabled;
    uint32_t active_camera_id;
    float interpolation_factor;
    bool show_markers;
    bool show_hud;
    bool is_reversed;
} PlaybackContext;

// Replay System
typedef struct {
    // Metadata
    ReplayMetadata metadata;
    ReplayStatistics statistics;
    
    // Data storage
    ReplayFrame frames[MAX_REPLAY_FRAMES];
    uint32_t frame_count;
    
    ReplaySnapshot snapshots[MAX_REPLAY_SNAPSHOTS];
    uint32_t snapshot_count;
    
    ReplayMarker markers[MAX_REPLAY_MARKERS];
    uint32_t marker_count;
    
    ReplayCamera cameras[MAX_REPLAY_CAMERAS];
    uint32_t camera_count;
    
    ReplayHighlight highlights[MAX_REPLAY_HIGHLIGHTS];
    uint32_t highlight_count;
    
    // Contexts
    RecordingContext recording;
    PlaybackContext playback;
    
    // Entity tracking
    struct {
        uint32_t entity_id;
        char name[64];
        bool is_player;
        bool is_active;
        uint32_t first_frame;
        uint32_t last_frame;
    } entity_registry[MAX_REPLAY_ENTITIES];
    uint32_t entity_registry_count;
    
    // Compression and optimization
    bool is_compressed;
    uint8_t compression_level;
    bool is_optimized;
    uint32_t optimization_flags;
} ReplaySystem;

// Export Settings
typedef struct {
    ExportFormat format;
    uint32_t start_frame;
    uint32_t end_frame;
    uint32_t resolution_width;
    uint32_t resolution_height;
    float frame_rate;
    uint32_t bitrate_kbps;
    bool include_audio;
    bool include_hud;
    bool include_markers;
    uint32_t quality_level;
    char output_path[512];
} ExportSettings;

// Core Functions
bool replay_system_init(ReplaySystem* replay);
void replay_system_shutdown(ReplaySystem* replay);
ReplaySystem* replay_system_create(void);
void replay_system_destroy(ReplaySystem* replay);

// Recording Functions
bool replay_start_recording(ReplaySystem* replay, const char* name, float fps);
bool replay_stop_recording(ReplaySystem* replay);
bool replay_pause_recording(ReplaySystem* replay);
bool replay_resume_recording(ReplaySystem* replay);
bool replay_add_frame(ReplaySystem* replay, const ReplayFrame* frame);
bool replay_add_input_frame(ReplaySystem* replay, uint32_t entity_id, const InputFrame* input);
bool replay_add_state_frame(ReplaySystem* replay, uint32_t entity_id, const StateFrame* state);
bool replay_add_event_frame(ReplaySystem* replay, const EventFrame* event);
bool replay_add_camera_frame(ReplaySystem* replay, const CameraFrame* camera);
bool replay_add_audio_frame(ReplaySystem* replay, const AudioFrame* audio);
bool replay_add_visual_frame(ReplaySystem* replay, const VisualFrame* visual);

// Playback Functions
bool replay_start_playback(ReplaySystem* replay);
bool replay_stop_playback(ReplaySystem* replay);
bool replay_pause_playback(ReplaySystem* replay);
bool replay_resume_playback(ReplaySystem* replay);
bool replay_seek_to_frame(ReplaySystem* replay, uint32_t frame_number);
bool replay_seek_to_time(ReplaySystem* replay, uint32_t time_ms);
bool replay_set_playback_speed(ReplaySystem* replay, float speed);
bool replay_set_playback_range(ReplaySystem* replay, uint32_t start_frame, uint32_t end_frame);
bool replay_toggle_reverse_playback(ReplaySystem* replay);
ReplayFrame* replay_get_current_frame(const ReplaySystem* replay);
ReplayFrame* replay_get_frame(const ReplaySystem* replay, uint32_t frame_number);

// Camera Functions
uint32_t replay_add_camera(ReplaySystem* replay, const ReplayCamera* camera);
bool replay_set_active_camera(ReplaySystem* replay, uint32_t camera_id);
bool replay_enable_auto_camera(ReplaySystem* replay, bool enabled);
ReplayCamera* replay_get_camera(const ReplaySystem* replay, uint32_t camera_id);
bool replay_update_camera_position(ReplaySystem* replay, uint32_t camera_id, Vec3 position, Vec3 target);

// Marker Functions
uint32_t replay_add_marker(ReplaySystem* replay, const ReplayMarker* marker);
bool replay_remove_marker(ReplaySystem* replay, uint32_t marker_id);
bool replay_add_auto_marker(ReplaySystem* replay, HighlightType type, uint32_t entity_id, const char* description);
ReplayMarker* replay_get_marker(const ReplaySystem* replay, uint32_t marker_id);
ReplayMarker* replay_find_marker_at_time(const ReplaySystem* replay, uint32_t time_ms);

// Highlight Functions
uint32_t replay_add_highlight(ReplaySystem* replay, const ReplayHighlight* highlight);
bool replay_remove_highlight(ReplaySystem* replay, uint32_t highlight_id);
bool replay_auto_generate_highlights(ReplaySystem* replay);
ReplayHighlight* replay_get_highlight(const ReplaySystem* replay, uint32_t highlight_id);
ReplayHighlight* replay_get_best_highlight(const ReplaySystem* replay, HighlightType type);

// Snapshot Functions
bool replay_create_snapshot(ReplaySystem* replay);
bool replay_restore_snapshot(ReplaySystem* replay, uint32_t snapshot_id);
ReplaySnapshot* replay_get_snapshot(const ReplaySystem* replay, uint32_t snapshot_id);
ReplaySnapshot* replay_find_nearest_snapshot(const ReplaySystem* replay, uint32_t time_ms);

// Entity Management
bool replay_register_entity(ReplaySystem* replay, uint32_t entity_id, const char* name, bool is_player);
bool replay_unregister_entity(ReplaySystem* replay, uint32_t entity_id);
bool replay_set_entity_active(ReplaySystem* replay, uint32_t entity_id, bool active);
const char* replay_get_entity_name(const ReplaySystem* replay, uint32_t entity_id);
bool replay_is_entity_player(const ReplaySystem* replay, uint32_t entity_id);

// File I/O Functions
bool replay_save_to_file(const ReplaySystem* replay, const char* filename);
bool replay_load_from_file(ReplaySystem* replay, const char* filename);
bool replay_export_to_file(const ReplaySystem* replay, const ExportSettings* settings);
bool replay_compress_replay(ReplaySystem* replay, uint8_t compression_level);
bool replay_decompress_replay(ReplaySystem* replay);

// Analysis Functions
ReplayStatistics replay_calculate_statistics(const ReplaySystem* replay);
bool replay_analyze_performance(const ReplaySystem* replay, float* avg_fps, float* peak_fps);
uint32_t replay_count_events_of_type(const ReplaySystem* replay, uint32_t event_type);
bool replay_find_peak_action_time(const ReplaySystem* replay, uint32_t* time_ms);

// Utility Functions
uint32_t replay_get_frame_count(const ReplaySystem* replay);
uint32_t replay_get_duration_ms(const ReplaySystem* replay);
float replay_get_fps(const ReplaySystem* replay);
uint32_t replay_get_current_time_ms(const ReplaySystem* replay);
bool replay_is_recording(const ReplaySystem* replay);
bool replay_is_playing(const ReplaySystem* replay);
PlaybackState replay_get_playback_state(const ReplaySystem* replay);

// Validation Functions
bool replay_validate(const ReplaySystem* replay);
bool replay_validate_frame(const ReplayFrame* frame);
bool replay_validate_metadata(const ReplayMetadata* metadata);

// Debug Functions
void replay_debug_print_info(const ReplaySystem* replay);
void replay_debug_print_frame(const ReplayFrame* frame);
void replay_debug_print_markers(const ReplaySystem* replay);
void replay_debug_print_statistics(const ReplaySystem* replay);

// Multiplayer Support
bool replay_merge_replays(ReplaySystem* target, const ReplaySystem* source, uint32_t time_offset_ms);
bool replay_extract_player_perspective(const ReplaySystem* source, ReplaySystem* target, uint32_t player_id);
bool replay_create_server_replay(const ReplaySystem* client_replays[], uint32_t count, ReplaySystem* server_replay);

// Streaming Support
bool replay_enable_streaming_mode(ReplaySystem* replay, bool enabled);
bool replay_stream_frame_to_network(const ReplaySystem* replay, uint32_t frame_number);
bool replay_receive_streamed_frame(ReplaySystem* replay, const ReplayFrame* frame);

#ifdef __cplusplus
}
#endif

#endif // REPLAY_SYSTEM_H
