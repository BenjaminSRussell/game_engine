/**
 * =================================================================================================
 *                          AUDIO DSP EFFECTS
 *                          Phase 6: Audio Excellence
 * =================================================================================================
 *
 * PURPOSE: Digital Signal Processing (EQ, Delay, Distortion)
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdint.h>
#include <stdlib.h> // NULL

typedef struct {
  float buffer[44100]; // 1 second delay at 44.1kHz
  int cursor;
  int length;
  float feedback;
  float mix;
} DelayEffect;

void audio_dsp_process_delay(DelayEffect *delay, float *buffer, int samples) {
  if (!delay)
    return;

  for (int i = 0; i < samples; i++) {
    float input = buffer[i];

    // Read from delay line
    int read_pos = (delay->cursor - delay->length + 44100) % 44100;
    float delayed = delay->buffer[read_pos];

    // Write to delay line (with feedback)
    delay->buffer[delay->cursor] = input + delayed * delay->feedback;

    // Output mix
    buffer[i] = input * (1.0f - delay->mix) + delayed * delay->mix;

    // Advance cursor
    delay->cursor = (delay->cursor + 1) % 44100;
  }
}

void audio_dsp_process_distortion(float *buffer, int samples, float drive) {
  for (int i = 0; i < samples; i++) {
    // Soft clipper
    float x = buffer[i] * drive;
    if (x > 1.0f)
      x = 1.0f;
    else if (x < -1.0f)
      x = -1.0f;
    else
      x = 1.5f * x - 0.5f * x * x * x; // Polynomial soft clip

    buffer[i] = x;
  }
}
