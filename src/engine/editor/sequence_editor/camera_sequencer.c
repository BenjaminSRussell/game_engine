#include "editor/sequence_editor/camera_sequencer.h"
#include <stdlib.h>
#include <string.h>

// Struct is defined in header

CameraSequencer* camera_sequencer_create(void) {
    CameraSequencer *seq = (CameraSequencer*)calloc(1, sizeof(CameraSequencer));
    seq->playback_speed = 1.0f;
    return seq;
}

void camera_sequencer_init(void) {
    // Stub
}

void camera_sequencer_destroy(CameraSequencer *seq) {
    if (seq) {
        free(seq);
    }
}

void camera_sequencer_play(CameraSequencer *seq) {
    if (seq) seq->is_playing = true;
}

void camera_sequencer_pause(CameraSequencer *seq) {
    if (seq) seq->is_playing = false;
}

void camera_sequencer_stop(CameraSequencer *seq) {
    if (seq) {
        seq->is_playing = false;
        seq->current_time = 0.0f;
    }
}

void camera_sequencer_seek(CameraSequencer *seq, float time) {
    if (seq) seq->current_time = time;
}

void camera_sequencer_update(CameraSequencer *seq, float delta_time) {
    if (seq && seq->is_playing) {
        seq->current_time += delta_time * seq->playback_speed;
    }
}

void camera_sequencer_set_playback_speed(CameraSequencer *seq, float speed) {
    if (seq) {
        seq->playback_speed = speed;
    }
}
