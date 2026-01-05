#include "../../include/engine/asset_importers.h"
#undef PI
#define MINIAUDIO_IMPLEMENTATION
#include "../../include/vendor/miniaudio.h"
#ifndef PI
#define PI 3.14159265358979323846f
#endif
#include "../../include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Minimal 16-bit Mono 44.1kHz WAV with 1 sample (0)
const unsigned char TINY_WAV[] = {
    'R',  'I',  'F',  'F',  0x28, 0x00, 0x00, 0x00, // Size: 44 - 8 = 36
    'W',  'A',  'V',  'E',  'f',  'm',  't',  ' ',
    0x10, 0x00, 0x00, 0x00, // fmt chunk size: 16
    0x01, 0x00,             // format: 1 (PCM)
    0x01, 0x00,             // channels: 1
    0x44, 0xAC, 0x00, 0x00, // sample rate: 44100
    0x88, 0x58, 0x01, 0x00, // byte rate: 44100 * 2 = 88200
    0x02, 0x00,             // block align: 2
    0x10, 0x00,             // bits per sample: 16
    'd',  'a',  't',  'a',  0x02, 0x00, 0x00, 0x00, // data size: 2
    0x00, 0x00                                      // 1 sample: 0
};

int main() {
  logger_init(LOG_LEVEL_INFO, LOG_TARGET_CONSOLE, NULL);
  LOG_INFO("Starting Audio Importer Test...");

  // Test: Load minimal WAV from memory
  LOG_INFO("\n=== Test: Load Tiny WAV ===");
  ImportedAudio *audio =
      asset_importer_load_audio_from_memory(TINY_WAV, sizeof(TINY_WAV));

  if (audio) {
    LOG_INFO("✓ Successfully loaded audio!");
    LOG_INFO("  Frames: %llu", (unsigned long long)audio->frame_count);
    LOG_INFO("  Channels: %u", audio->channels);
    LOG_INFO("  Sample Rate: %u Hz", audio->sample_rate);
    LOG_INFO("  Size: %u bytes", audio->size_bytes);

    // Check if we got something sensible
    if (audio->channels == 2 && audio->sample_rate == 48000) {
      LOG_INFO("✓ Resampled to engine standards (2 ch, 48kHz)!");
    }

    asset_importer_free_audio(audio);
    LOG_INFO("✓ Freed audio memory.");
  } else {
    LOG_ERROR("✗ Failed to load audio!");
    return 1;
  }

  LOG_INFO("\n=== Test Complete ===");
  return 0;
}
