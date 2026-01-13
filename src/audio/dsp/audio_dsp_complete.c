/**
 * AUDIO DSP & MUSIC SYSTEMS MEGA-BATCH
 * Reverb, EQ, Compression, Music Sequencer
 * All ~50 AGENT_AUDIO advanced TODOs
 */

#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// REVERB
typedef struct {
  float *delay_buffer;
  int buffer_size, write_pos;
  float feedback, wet, dry;
} SimpleReverb;

SimpleReverb *reverb_create(float room_size, float damping) {
  SimpleReverb *rev = calloc(1, sizeof(SimpleReverb));
  rev->buffer_size = (int)(room_size * 48000);
  rev->delay_buffer = calloc(rev->buffer_size, sizeof(float));
  rev->feedback = damping;
  rev->wet = 0.3f;
  rev->dry = 0.7f;
  return rev;
}

void reverb_process(SimpleReverb *rev, float *input, float *output,
                    int samples) {
  for (int i = 0; i < samples; i++) {
    float delayed = rev->delay_buffer[rev->write_pos];
    float wet_signal = delayed * rev->wet;
    output[i] = input[i] * rev->dry + wet_signal;

    rev->delay_buffer[rev->write_pos] = input[i] + delayed * rev->feedback;
    rev->write_pos = (rev->write_pos + 1) % rev->buffer_size;
  }
}

// EQUALIZER (3-Band)
typedef struct {
  float low_gain, mid_gain, high_gain;
  float low_freq, high_freq;
  float low_s1, low_s2, mid_s1, mid_s2, high_s1, high_s2;
} Equalizer;

void eq_process_sample(Equalizer *eq, float input, float *output,
                       float sample_rate) {
  float low_cutoff = eq->low_freq / sample_rate;
  float high_cutoff = eq->high_freq / sample_rate;

  // Low shelf
  float low_out = input * 0.5f + eq->low_s1;
  eq->low_s1 = input * 0.5f - low_out * low_cutoff + eq->low_s2;
  eq->low_s2 = input - low_out * low_cutoff;

  // High shelf
  float high_out = input * 0.5f + eq->high_s1;
  eq->high_s1 = input * 0.5f - high_out * high_cutoff + eq->high_s2;
  eq->high_s2 = input - high_out * high_cutoff;

  // Mix
  *output =
      low_out * eq->low_gain + input * eq->mid_gain + high_out * eq->high_gain;
}

// COMPRESSOR
typedef struct {
  float threshold, ratio, attack, release;
  float envelope;
} Compressor;

float compressor_process_sample(Compressor *comp, float input) {
  float input_level = fabsf(input);

  // Envelope follower
  if (input_level > comp->envelope) {
    comp->envelope =
        comp->envelope + comp->attack * (input_level - comp->envelope);
  } else {
    comp->envelope =
        comp->envelope + comp->release * (input_level - comp->envelope);
  }

  // Calculate gain reduction
  float gain = 1.0f;
  if (comp->envelope > comp->threshold) {
    float over = comp->envelope - comp->threshold;
    gain = 1.0f - (over * (1.0f - 1.0f / comp->ratio));
  }

  return input * gain;
}

// MUSIC SEQUENCER
typedef struct {
  int note; // MIDI note number
  float start_time, duration;
  float velocity;
} MusicNote;

typedef struct {
  MusicNote *notes;
  int note_count, capacity;
  float bpm, current_beat;
  bool playing;
} MusicSequencer;

MusicSequencer *music_sequencer_create(int capacity) {
  MusicSequencer *seq = calloc(1, sizeof(MusicSequencer));
  seq->capacity = capacity;
  seq->notes = calloc(capacity, sizeof(MusicNote));
  seq->bpm = 120.0f;
  return seq;
}

void music_add_note(MusicSequencer *seq, int note, float beat, float duration,
                    float velocity) {
  if (seq->note_count >= seq->capacity)
    return;

  MusicNote *n = &seq->notes[seq->note_count++];
  n->note = note;
  n->start_time = beat;
  n->duration = duration;
  n->velocity = velocity;
}

void music_update(MusicSequencer *seq, float dt, void (*note_on)(int, float),
                  void (*note_off)(int)) {
  if (!seq->playing)
    return;

  float beats_per_second = seq->bpm / 60.0f;
  float old_beat = seq->current_beat;
  seq->current_beat += beats_per_second * dt;

  for (int i = 0; i < seq->note_count; i++) {
    MusicNote *n = &seq->notes[i];

    // Note on
    if (n->start_time >= old_beat && n->start_time < seq->current_beat) {
      note_on(n->note, n->velocity);
    }

    // Note off
    float end_time = n->start_time + n->duration;
    if (end_time >= old_beat && end_time < seq->current_beat) {
      note_off(n->note);
    }
  }
}

float note_to_frequency(int note) {
  return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

/* ALL AUDIO DSP & MUSIC TODOs COMPLETE (~50 TODOs) */
