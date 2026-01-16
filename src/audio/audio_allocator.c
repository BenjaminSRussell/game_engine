#include "audio/audio_allocator.h"
#include "core/memory/memory_macros.h"
#include "engine/include/core/logger.h"
#include <string.h>

AudioAllocator *g_audio_allocator = NULL;

// Allocation sizes
#define VOICE_SIZE 256
#define BUFFER_SIZE 4096 // 4KB chunks for streaming
#define EFFECT_SIZE 128

void audio_allocator_init(void) {
  if (g_audio_allocator)
    return;

  g_audio_allocator = ALLOC_TYPE_ZERO(AudioAllocator);

  // Command arena (2MB - sufficient for many commands per frame)
  g_audio_allocator->command_arena =
      unified_memory_arena_create(2 * 1024 * 1024);

  // Voice pool (128 max concurrent voices)
  MemoryPoolConfig voice_config = {
      .block_size = VOICE_SIZE,
      .block_count = 128,
      .auto_expand = false // Hard limit for voices
  };
  g_audio_allocator->voice_pool = unified_memory_pool_create(&voice_config);

  // Buffer pool (256 chunks = 1MB streaming buffer)
  MemoryPoolConfig buffer_config = {
      .block_size = BUFFER_SIZE, .block_count = 256, .auto_expand = true};
  g_audio_allocator->buffer_pool = unified_memory_pool_create(&buffer_config);

  // Effect pool
  MemoryPoolConfig effect_config = {
      .block_size = EFFECT_SIZE, .block_count = 64, .auto_expand = true};
  g_audio_allocator->effect_pool = unified_memory_pool_create(&effect_config);

  LOG_INFO("[Audio] Allocator initialized");
}

void audio_allocator_shutdown(void) {
  if (!g_audio_allocator)
    return;

  audio_allocator_print_stats();

  if (g_audio_allocator->effect_pool)
    unified_memory_pool_destroy(g_audio_allocator->effect_pool);
  if (g_audio_allocator->buffer_pool)
    unified_memory_pool_destroy(g_audio_allocator->buffer_pool);
  if (g_audio_allocator->voice_pool)
    unified_memory_pool_destroy(g_audio_allocator->voice_pool);
  if (g_audio_allocator->command_arena)
    unified_memory_arena_destroy(g_audio_allocator->command_arena);

  FREE(g_audio_allocator);
  g_audio_allocator = NULL;

  LOG_INFO("[Audio] Allocator shutdown");
}

void audio_allocator_reset_commands(void) {
  if (!g_audio_allocator || !g_audio_allocator->command_arena)
    return;

  // In a real implementation, we'd use a double-buffer allocator or ring buffer
  // for commands to handle thread contention. For now, we assume frame-synced
  // resets.

  size_t current =
      unified_memory_arena_get_usage(g_audio_allocator->command_arena);
  if (current > g_audio_allocator->command_arena_peak_usage) {
    g_audio_allocator->command_arena_peak_usage = current;
  }

  unified_memory_arena_reset(g_audio_allocator->command_arena);
}

void *audio_alloc_command(size_t size) {
  if (!g_audio_allocator || !g_audio_allocator->command_arena)
    return NULL;
  // Note: Needs thread safety/locking for production
  return unified_memory_arena_alloc(g_audio_allocator->command_arena, size);
}

void *audio_alloc_voice(void) {
  if (!g_audio_allocator || !g_audio_allocator->voice_pool)
    return NULL;
  void *ptr =
      unified_memory_pool_alloc(g_audio_allocator->voice_pool, VOICE_SIZE);
  if (ptr)
    g_audio_allocator->voices_allocated++;
  return ptr;
}

void audio_free_voice(void *voice) {
  if (!g_audio_allocator || !g_audio_allocator->voice_pool || !voice)
    return;
  unified_memory_pool_free(g_audio_allocator->voice_pool, voice);
  g_audio_allocator->voices_allocated--;
}

void *audio_alloc_buffer(void) {
  if (!g_audio_allocator || !g_audio_allocator->buffer_pool)
    return NULL;
  void *ptr =
      unified_memory_pool_alloc(g_audio_allocator->buffer_pool, BUFFER_SIZE);
  if (ptr)
    g_audio_allocator->buffers_allocated++;
  return ptr;
}

void audio_free_buffer(void *buffer) {
  if (!g_audio_allocator || !g_audio_allocator->buffer_pool || !buffer)
    return;
  unified_memory_pool_free(g_audio_allocator->buffer_pool, buffer);
  g_audio_allocator->buffers_allocated--;
}

void audio_allocator_print_stats(void) {
  if (!g_audio_allocator)
    return;

  printf("\n=== Audio Allocator Statistics ===\n");
  printf("Command Arena Peak: %.2f KB\n",
         g_audio_allocator->command_arena_peak_usage / 1024.0f);
  printf("Voices: %u active\n", g_audio_allocator->voices_allocated);
  printf("Buffers: %u allocated\n", g_audio_allocator->buffers_allocated);
  printf("==================================\n\n");
}
