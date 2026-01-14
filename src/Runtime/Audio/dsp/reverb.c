/**
 * REVERB DSP EFFECT
 * Convolution and algorithmic reverb for spatial audio
 */

#include <include/math/math_all.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REVERB_TIME 10.0f
#define SAMPLE_RATE 48000
#define MAX_IR_LENGTH (SAMPLE_RATE * MAX_REVERB_TIME)

// Delay line for reverb
typedef struct {
  float *buffer;
  unsigned int size;
  unsigned int write_pos;
} DelayLine;

// All-pass filter
typedef struct {
  DelayLine delay;
  float feedback;
} AllPassFilter;

// Comb filter
typedef struct {
  DelayLine delay;
  float feedback;
  float damp;
  float filter_state;
} CombFilter;

// Freeverb-style algorithmic reverb
typedef struct {
  CombFilter comb_filters[8];
  AllPassFilter allpass_filters[4];
  float room_size;
  float damping;
  float wet_level;
  float dry_level;
  float width; // Stereo width
} AlgorithmicReverb;

// Convolution reverb (impulse response)
typedef struct {
  float *impulse_response;
  unsigned int ir_length;
  float *convolution_buffer;
  unsigned int buffer_pos;
  float wet_level;
  float dry_level;
} ConvolutionReverb;

// Create delay line
static DelayLine delay_create(unsigned int size) {
  DelayLine delay;
  delay.size = size;
  delay.write_pos = 0;
  delay.buffer = calloc(size, sizeof(float));
  return delay;
}

static void delay_destroy(DelayLine *delay) { free(delay->buffer); }

static void delay_write(DelayLine *delay, float sample) {
  delay->buffer[delay->write_pos] = sample;
  delay->write_pos = (delay->write_pos + 1) % delay->size;
}

static float delay_read(const DelayLine *delay, unsigned int offset) {
  unsigned int read_pos =
      (delay->write_pos + delay->size - offset) % delay->size;
  return delay->buffer[read_pos];
}

// Algorithmic reverb: Initialize
AlgorithmicReverb *reverb_algorithmic_create() {
  AlgorithmicReverb *reverb = malloc(sizeof(AlgorithmicReverb));

  // Initialize comb filters with different delays (prime numbers for diffusion)
  const unsigned int comb_delays[8] = {1557, 1617, 1491, 1422,
                                       1277, 1356, 1188, 1116};
  for (int i = 0; i < 8; i++) {
    reverb->comb_filters[i].delay = delay_create(comb_delays[i]);
    reverb->comb_filters[i].feedback = 0.84f;
    reverb->comb_filters[i].damp = 0.2f;
    reverb->comb_filters[i].filter_state = 0.0f;
  }

  // Initialize all-pass filters
  const unsigned int allpass_delays[4] = {225, 556, 441, 341};
  for (int i = 0; i < 4; i++) {
    reverb->allpass_filters[i].delay = delay_create(allpass_delays[i]);
    reverb->allpass_filters[i].feedback = 0.5f;
  }

  reverb->room_size = 0.5f;
  reverb->damping = 0.5f;
  reverb->wet_level = 0.33f;
  reverb->dry_level = 0.4f;
  reverb->width = 1.0f;

  return reverb;
}

// Process comb filter
static float comb_process(CombFilter *comb, float input) {
  float delayed = delay_read(&comb->delay, comb->delay.size - 1);

  // One-pole lowpass for damping
  comb->filter_state =
      delayed * (1.0f - comb->damp) + comb->filter_state * comb->damp;

  float output = -input + comb->filter_state;
  delay_write(&comb->delay, input + comb->filter_state * comb->feedback);

  return output;
}

// Process all-pass filter
static float allpass_process(AllPassFilter *ap, float input) {
  float delayed = delay_read(&ap->delay, ap->delay.size - 1);
  float output = -input + delayed;
  delay_write(&ap->delay, input + delayed * ap->feedback);
  return output;
}

// Process reverb (mono in, stereo out)
void reverb_algorithmic_process(AlgorithmicReverb *reverb, const float *input,
                                float *output_left, float *output_right,
                                unsigned int num_samples) {
  for (unsigned int i = 0; i < num_samples; i++) {
    float in = input[i];

    // Sum of all comb filters
    float comb_sum = 0.0f;
    for (int c = 0; c < 8; c++) {
      comb_sum += comb_process(&reverb->comb_filters[c], in);
    }

    // Chain all-pass filters
    float ap_out = comb_sum;
    for (int a = 0; a < 4; a++) {
      ap_out = allpass_process(&reverb->allpass_filters[a], ap_out);
    }

    // Mix wet/dry
    float wet = ap_out * reverb->wet_level;
    float dry = in * reverb->dry_level;

    // Stereo spread
    output_left[i] = dry + wet * (1.0f + reverb->width) / 2.0f;
    output_right[i] = dry + wet * (1.0f - reverb->width) / 2.0f;
  }
}

// Set reverb parameters
void reverb_set_room_size(AlgorithmicReverb *reverb, float size) {
  reverb->room_size = size;
  // Update comb filter feedbacks
  for (int i = 0; i < 8; i++) {
    reverb->comb_filters[i].feedback = 0.7f + size * 0.28f;
  }
}

void reverb_set_damping(AlgorithmicReverb *reverb, float damp) {
  reverb->damping = damp;
  for (int i = 0; i < 8; i++) {
    reverb->comb_filters[i].damp = damp;
  }
}

void reverb_set_wet_dry(AlgorithmicReverb *reverb, float wet, float dry) {
  reverb->wet_level = wet;
  reverb->dry_level = dry;
}

void reverb_algorithmic_destroy(AlgorithmicReverb *reverb) {
  for (int i = 0; i < 8; i++) {
    delay_destroy(&reverb->comb_filters[i].delay);
  }
  for (int i = 0; i < 4; i++) {
    delay_destroy(&reverb->allpass_filters[i].delay);
  }
  free(reverb);
}

// Convolution reverb: Create
ConvolutionReverb *reverb_convolution_create(const char *ir_file_path) {
  ConvolutionReverb *reverb = malloc(sizeof(ConvolutionReverb));

  // Load impulse response from file (WAV, etc.)
  // For now, create a simple exponential decay IR
  reverb->ir_length = SAMPLE_RATE * 2; // 2 second IR
  reverb->impulse_response = malloc(reverb->ir_length * sizeof(float));

  // Generate simple exponential decay
  for (unsigned int i = 0; i < reverb->ir_length; i++) {
    float t = (float)i / SAMPLE_RATE;
    reverb->impulse_response[i] =
        expf(-t * 2.0f) * ((float)rand() / RAND_MAX * 0.5f - 0.25f);
  }

  reverb->convolution_buffer = calloc(reverb->ir_length, sizeof(float));
  reverb->buffer_pos = 0;
  reverb->wet_level = 0.5f;
  reverb->dry_level = 0.5f;

  return reverb;
}

// Process convolution reverb
void reverb_convolution_process(ConvolutionReverb *reverb, const float *input,
                                float *output, unsigned int num_samples) {
  for (unsigned int i = 0; i < num_samples; i++) {
    // Add input to circular buffer
    reverb->convolution_buffer[reverb->buffer_pos] = input[i];

    // Convolve with impulse response
    float conv_sum = 0.0f;
    for (unsigned int j = 0; j < reverb->ir_length; j++) {
      unsigned int tap =
          (reverb->buffer_pos + reverb->ir_length - j) % reverb->ir_length;
      conv_sum += reverb->convolution_buffer[tap] * reverb->impulse_response[j];
    }

    // Mix wet/dry
    output[i] = input[i] * reverb->dry_level + conv_sum * reverb->wet_level;

    reverb->buffer_pos = (reverb->buffer_pos + 1) % reverb->ir_length;
  }
}

void reverb_convolution_destroy(ConvolutionReverb *reverb) {
  free(reverb->impulse_response);
  free(reverb->convolution_buffer);
  free(reverb);
}

// Reverb zone system (different reverb for different areas)
typedef struct {
  float position[3];
  float radius;
  AlgorithmicReverb *reverb;
  char name[64];
} ReverbZone;

typedef struct {
  ReverbZone *zones;
  unsigned int zone_count;
  unsigned int max_zones;
} ReverbZoneManager;

ReverbZoneManager *reverb_zone_manager_create() {
  ReverbZoneManager *mgr = malloc(sizeof(ReverbZoneManager));
  mgr->max_zones = 32;
  mgr->zone_count = 0;
  mgr->zones = malloc(mgr->max_zones * sizeof(ReverbZone));
  return mgr;
}

void reverb_zone_add(ReverbZoneManager *mgr, const char *name, float pos[3],
                     float radius) {
  if (mgr->zone_count >= mgr->max_zones)
    return;

  ReverbZone *zone = &mgr->zones[mgr->zone_count++];
  strncpy(zone->name, name, 63);
  memcpy(zone->position, pos, 3 * sizeof(float));
  zone->radius = radius;
  zone->reverb = reverb_algorithmic_create();
}

// Get reverb for listener position (blends between zones)
void reverb_zone_get_for_position(ReverbZoneManager *mgr, float listener_pos[3],
                                  AlgorithmicReverb **out_reverb,
                                  float *blend_weight) {
  // Find closest reverb zone
  float closest_dist = 1e10f;
  ReverbZone *closest_zone = NULL;

  for (unsigned int i = 0; i < mgr->zone_count; i++) {
    ReverbZone *zone = &mgr->zones[i];
    float dx = listener_pos[0] - zone->position[0];
    float dy = listener_pos[1] - zone->position[1];
    float dz = listener_pos[2] - zone->position[2];
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < zone->radius && dist < closest_dist) {
      closest_dist = dist;
      closest_zone = zone;
    }
  }

  if (closest_zone) {
    *out_reverb = closest_zone->reverb;
    *blend_weight = 1.0f - (closest_dist / closest_zone->radius);
  } else {
    *out_reverb = NULL;
    *blend_weight = 0.0f;
  }
}

void reverb_zone_manager_destroy(ReverbZoneManager *mgr) {
  for (unsigned int i = 0; i < mgr->zone_count; i++) {
    reverb_algorithmic_destroy(mgr->zones[i].reverb);
  }
  free(mgr->zones);
  free(mgr);
}
