#ifndef AUDIO_ALLOCATOR_H
#define AUDIO_ALLOCATOR_H

#include "core/memory/unified_memory_allocator.h"
#include "core/types.h"

/**
 * Audio Allocator - Memory management for audio subsystem
 *
 * Strategy:
 * - Command Arena: Lock-free command queue buffer (reset when processed)
 * - Pools: Voices (active sounds), Buffers (streaming chunks)
 *
 * Thread Safety:
 * - Allocator functions must be thread-safe or called from owner thread
 * - Command arena is single-producer (Game), single-consumer (Audio)
 */

typedef struct {
  ArenaAllocator *command_arena; // For audio commands
  MemoryPool *voice_pool;        // Active voices
  MemoryPool *buffer_pool;       // PCM buffers/chunks
  MemoryPool *effect_pool;       // DSP effect instances

  // Statistics
  u64 command_arena_peak_usage;
  u32 voices_allocated;
  u32 buffers_allocated;
} AudioAllocator;

// Global audio allocator
extern AudioAllocator *g_audio_allocator;

/**
 * Initialize audio allocator
 */
void audio_allocator_init(void);

/**
 * Shutdown audio allocator
 */
void audio_allocator_shutdown(void);

/**
 * Reset command arena
 * Call after processing commands in audio thread
 */
void audio_allocator_reset_commands(void);

/**
 * Allocate audio command
 * Thread-safe for Game Thread
 */
void *audio_alloc_command(size_t size);

/**
 * Allocate audio voice
 */
void *audio_alloc_voice(void);

/**
 * Free audio voice
 */
void audio_free_voice(void *voice);

/**
 * Allocate buffer (chunk)
 */
void *audio_alloc_buffer(void);

/**
 * Free buffer (chunk)
 */
void audio_free_buffer(void *buffer);

/**
 * Get stats
 */
void audio_allocator_print_stats(void);

#endif // AUDIO_ALLOCATOR_H
