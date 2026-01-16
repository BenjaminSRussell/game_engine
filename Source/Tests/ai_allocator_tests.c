#include "ai/ai_allocator.h"
#include "core/memory/unified_memory_allocator.h"
#include "test_framework.h"
#include <stdio.h>

void test_ai_allocator(void) {
  unified_memory_init(NULL);
  ai_allocator_init();

  AIAllocator *allocator = g_ai_allocator;
  TEST_ASSERT(allocator != NULL, "Allocator init failed");

  // Test Agent
  void *a1 = ai_alloc_agent();
  TEST_ASSERT(a1 != NULL, "Agent alloc failed");
  TEST_ASSERT(allocator->agents_allocated == 1, "Agent count error");
  ai_free_agent(a1);

  // Test Path
  void *p1 = ai_alloc_path();
  TEST_ASSERT(p1 != NULL, "Path alloc failed");
  ai_free_path(p1);

  // Test Temp Node (Arena)
  void *node = ai_alloc_path_node(64);
  TEST_ASSERT(node != NULL, "Node alloc failed");
  ai_allocator_reset_path_arena();

  ai_allocator_shutdown();
  unified_memory_shutdown();
}

int main(void) {
  printf("\n=== AI Allocator Tests ===\n");
  RUN_TEST(test_ai_allocator);
  printf("Tests Passed\n");
  return 0;
}
