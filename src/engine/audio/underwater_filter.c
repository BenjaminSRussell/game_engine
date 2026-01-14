#include "include/audio/underwater_filter.h"
#include "include/core/logger.h"

#include <include/math/math_all.h>

static bool g_filter_enabled = false;
static f32 g_filter_depth = 0.0f;

static f32 g_lp_state_l = 0.0f;
static f32 g_lp_state_r = 0.0f;

void underwater_filter_init(void) {
  g_filter_enabled = false;
  g_filter_depth = 0.0f;
  g_lp_state_l = 0.0f;
  g_lp_state_r = 0.0f;
}

static f32 underwater_filter_cutoff_hz(void) {
  // 20kHz at surface, down to ~800Hz when deep.
  f32 depth = g_filter_depth;
  if (depth < 0.0f)
    depth = 0.0f;
  if (depth > 1.5f)
    depth = 1.5f;

  f32 t = depth / 1.5f;
  return 20000.0f - t * (20000.0f - 800.0f);
}

void underwater_filter_process_samples(float *samples, u32 sample_count) {
  if (!samples || sample_count == 0) {
    return;
  }
  if (!g_filter_enabled) {
    return;
  }

  // Assume stereo interleaved float32.
  // Use a one-pole low-pass filter per channel: y += a * (x - y)
  const f32 sample_rate = 48000.0f;
  const f32 cutoff = underwater_filter_cutoff_hz();
  const f32 rc = 1.0f / (2.0f * (f32)M_PI * cutoff);
  const f32 dt = 1.0f / sample_rate;
  const f32 a = dt / (rc + dt);

  for (u32 i = 0; i + 1 < sample_count; i += 2) {
    f32 in_l = samples[i + 0];
    f32 in_r = samples[i + 1];

    g_lp_state_l = g_lp_state_l + a * (in_l - g_lp_state_l);
    g_lp_state_r = g_lp_state_r + a * (in_r - g_lp_state_r);

    samples[i + 0] = g_lp_state_l;
    samples[i + 1] = g_lp_state_r;
  }
}

void underwater_filter_set_enabled(bool enabled) {
  g_filter_enabled = enabled;
  if (enabled) {
    LOG_INFO("Underwater audio filter enabled");
  } else {
    LOG_INFO("Underwater audio filter disabled");
  }
}

void underwater_filter_update_depth(f32 depth) {
  g_filter_depth = depth;
  if (g_filter_enabled) {
    LOG_TRACE("Underwater filter depth updated: %.2f", depth);
  }
}

bool underwater_filter_is_enabled(void) { return g_filter_enabled; }
