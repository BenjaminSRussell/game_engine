#include "../../../../include/core/logger.h"
#include "../../../../include/engine/asset_importers.h"
#include <stdlib.h>
#include <string.h>

// Don't define MINIAUDIO_IMPLEMENTATION here, it should be in audio_system.h
#undef PI
#include "../../../../include/vendor/miniaudio.h"

ImportedAudio *asset_importer_load_audio_from_memory(const void *data,
                                                     size_t size) {
  if (!data || size == 0) {
    LOG_ERROR("Invalid audio data");
    return NULL;
  }

  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 48000);

  ma_result result = ma_decoder_init_memory(data, size, &config, &decoder);
  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed to initialize audio decoder from memory: %d", result);
    return NULL;
  }

  ma_uint64 frame_count;
  result = ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);
  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed to get audio length: %d", result);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  size_t size_bytes = (size_t)(frame_count * 2 * sizeof(float));
  void *pcm_data = malloc(size_bytes);
  if (!pcm_data) {
    LOG_ERROR("Failed to allocate memory for PCM data (%zu bytes)", size_bytes);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  ma_uint64 frames_read;
  result =
      ma_decoder_read_pcm_frames(&decoder, pcm_data, frame_count, &frames_read);
  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed to read PCM frames: %d", result);
    free(pcm_data);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  ImportedAudio *audio = (ImportedAudio *)malloc(sizeof(ImportedAudio));
  if (!audio) {
    LOG_ERROR("Failed to allocate ImportedAudio");
    free(pcm_data);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  audio->data = pcm_data;
  audio->frame_count = (u64)frames_read;
  audio->channels = 2;
  audio->sample_rate = 48000;
  audio->size_bytes = (u32)size_bytes;

  ma_decoder_uninit(&decoder);

  LOG_INFO("Loaded audio from memory: %llu frames, %u channels, %u Hz",
           (unsigned long long)audio->frame_count, audio->channels,
           audio->sample_rate);

  return audio;
}

ImportedAudio *asset_importer_load_audio(const char *path) {
  if (!path) {
    LOG_ERROR("Invalid audio path");
    return NULL;
  }

  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 48000);

  ma_result result = ma_decoder_init_file(path, &config, &decoder);
  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed to initialize audio decoder for '%s': %d", path, result);
    return NULL;
  }

  ma_uint64 frame_count;
  result = ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);
  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed to get audio length for '%s': %d", path, result);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  // fallback for formats where length might not be known immediately or is 0
  if (frame_count == 0) {
    LOG_WARN("Audio length is 0 for '%s', possible streaming format or unknown "
             "length",
             path);
    // We could handle this by reading in chunks, but for now we expect fixed
    // length
  }

  size_t size_bytes = (size_t)(frame_count * 2 * sizeof(float));
  void *pcm_data = malloc(size_bytes);
  if (!pcm_data) {
    LOG_ERROR("Failed to allocate memory for PCM data (%zu bytes) for '%s'",
              size_bytes, path);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  ma_uint64 frames_read;
  result =
      ma_decoder_read_pcm_frames(&decoder, pcm_data, frame_count, &frames_read);
  if (result != MA_SUCCESS) {
    LOG_ERROR("Failed to read PCM frames for '%s': %d", path, result);
    free(pcm_data);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  ImportedAudio *audio = (ImportedAudio *)malloc(sizeof(ImportedAudio));
  if (!audio) {
    LOG_ERROR("Failed to allocate ImportedAudio for '%s'", path);
    free(pcm_data);
    ma_decoder_uninit(&decoder);
    return NULL;
  }

  audio->data = pcm_data;
  audio->frame_count = (u64)frames_read;
  audio->channels = 2;
  audio->sample_rate = 48000;
  audio->size_bytes = (u32)size_bytes;

  ma_decoder_uninit(&decoder);

  LOG_INFO("Loaded audio '%s': %llu frames, %u channels, %u Hz", path,
           (unsigned long long)audio->frame_count, audio->channels,
           audio->sample_rate);

  return audio;
}

void asset_importer_free_audio(ImportedAudio *audio) {
  if (!audio)
    return;

  if (audio->data) {
    free(audio->data);
  }

  free(audio);
}
