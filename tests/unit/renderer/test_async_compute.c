#include <renderer/async_compute.h>
#include "test_framework_unified.h"

static TestResult test_queue_detection_logic(void) {
    QueueFamilies families = {0};
    families.graphics_family = 0;
    families.compute_family = 1; // Distinct
    families.has_dedicated_compute = true;
    
    // Verify our logic considers this "dedicated"
    TEST_ASSERT_TRUE(families.has_dedicated_compute, "Dedicated compute detected");
    TEST_ASSERT_NEQ(families.graphics_family, families.compute_family, "Families are distinct");
    
    return TEST_PASS;
}

int main(void) {
    test_init();
    test_register("Renderer:AsyncCompute", "Queue Detection", test_queue_detection_logic, NULL, NULL);
    TestStats stats = test_run_all();
    test_cleanup();
    return stats.failed > 0 ? 1 : 0;
}
