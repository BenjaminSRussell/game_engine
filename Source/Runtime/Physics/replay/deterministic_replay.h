#ifndef DETERMINISTIC_REPLAY_H
#define DETERMINISTIC_REPLAY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct replay_context_t replay_context_t;
typedef struct replay_frame_t replay_frame_t;
typedef struct replay_state_t replay_state_t;

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    float x, y, z, w;
} quat_t;

typedef struct {
    vec3_t position;
    quat_t rotation;
    vec3_t velocity;
    vec3_t angular_velocity;
    uint32_t body_id;
    uint32_t timestamp;
} body_state_t;

typedef struct {
    uint32_t frame_number;
    uint32_t timestamp;
    uint32_t body_count;
    body_state_t* body_states;
    uint32_t input_count;
    uint8_t* input_data;
    uint32_t checksum;
} replay_frame_t;

typedef struct {
    uint32_t magic_number;
    uint16_t version;
    uint16_t header_size;
    uint32_t frame_count;
    uint32_t body_count;
    uint32_t duration_ms;
    uint32_t compression_type;
    uint32_t checksum;
} replay_header_t;

typedef enum {
    REPLAY_SUCCESS = 0,
    REPLAY_ERROR_INVALID_PARAM = -1,
    REPLAY_ERROR_OUT_OF_MEMORY = -2,
    REPLAY_ERROR_FILE_NOT_FOUND = -3,
    REPLAY_ERROR_INVALID_FORMAT = -4,
    REPLAY_ERROR_CHECKSUM_MISMATCH = -5,
    REPLAY_ERROR_NOT_INITIALIZED = -6,
    REPLAY_ERROR_RECORDING = -7,
    REPLAY_ERROR_PLAYBACK = -8
} replay_error_t;

replay_error_t replay_init(replay_context_t** context);
void replay_shutdown(replay_context_t* context);

replay_error_t replay_start_recording(replay_context_t* context, uint32_t body_count);
replay_error_t replay_stop_recording(replay_context_t* context);
replay_error_t replay_save_frame(replay_context_t* context, const body_state_t* bodies, uint32_t body_count);

replay_error_t replay_start_playback(replay_context_t* context, const char* filename);
replay_error_t replay_stop_playback(replay_context_t* context);
replay_error_t replay_get_frame(replay_context_t* context, uint32_t frame_number, replay_frame_t* frame);
replay_error_t replay_step_frame(replay_context_t* context, replay_frame_t* frame);

replay_error_t replay_save_to_file(replay_context_t* context, const char* filename);
replay_error_t replay_load_from_file(replay_context_t* context, const char* filename);

replay_error_t replay_set_deterministic_mode(replay_context_t* context, bool enabled);
replay_error_t replay_set_fixed_timestep(replay_context_t* context, float timestep);
replay_error_t replay_validate_replay(replay_context_t* context, bool* is_valid);

#ifdef __cplusplus
}
#endif

#endif
