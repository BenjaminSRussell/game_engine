#include "audio/audio_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "test_framework.h"
#include <stdio.h>

void test_audio_allocator(void) {
  unified_memory_init(NULL);
  audio_allocator_init();

  AudioAllocator *allocator = g_audio_allocator;
  TEST_ASSERT(allocator != NULL, "Allocator init failed");

  // Test Voice Allocation
  void *v1 = audio_alloc_voice();
  void *v2 = audio_alloc_voice();
  TEST_ASSERT(v1 != NULL && v2 != NULL, "Voice alloc failed");
  TEST_ASSERT(allocator->voices_allocated == 2, "Voice count incorrect");

  audio_free_voice(v1);
  TEST_ASSERT(allocator->voices_allocated == 1, "Voice free failed");

  // Test Buffer Allocation
  void *b1 = audio_alloc_buffer();
  TEST_ASSERT(b1 != NULL, "Buffer alloc failed");
  audio_free_buffer(b1);

  // Test Command Allocation
  void *cmd = audio_alloc_command(64);
  TEST_ASSERT(cmd != NULL, "Command alloc failed");
  audio_allocator_reset_commands();

  audio_allocator_shutdown();
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== Audio Allocator Tests ===\n");
  RUN_TEST(test_audio_allocator);
  printf("Tests Passed\n");
  return 0;
}
