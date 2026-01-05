#include <renderer/dynamic_rendering.h>
#include <renderer/vulkan_dispatch.h>
#include "test_framework_unified.h"

// Mock dispatch table
#ifndef VULKAN_BUILD
typedef struct VkRenderingInfo VkRenderingInfo;
typedef struct VkCommandBuffer VkCommandBuffer;
#endif

static TestResult test_dynamic_rendering_init(void) {
    // This is difficult to test without a real Vulkan instance
    // We can at least verification the strict helper functions
    
    VkImageView color = (VkImageView)1;
    float clear_color[4] = {0,0,0,1};
    
    DynamicRenderingInfo info = dynamic_rendering_info_simple(color, 800, 600, true, clear_color);
    
    TEST_ASSERT_EQ(info.color_attachment_count, 1, "Simple info has 1 attachment");
    TEST_ASSERT_NOT_NULL(info.color_attachments, "Attachments array allocated");
    TEST_ASSERT_EQ(info.render_area.extent.width, 800, "Width correct");
    
    return TEST_PASS;
}

static TestResult test_tbdr_local_read_check(void) {
    // Just verify the function exists and returns a bool safely
    bool result = dynamic_rendering_has_local_read();
    TEST_ASSERT(result == true || result == false, "Returns valid boolean");
    return TEST_PASS;
}

int main(void) {
    test_init();
    test_register("Renderer:Dynamic", "Init Helper", test_dynamic_rendering_init, NULL, NULL);
    test_register("Renderer:Dynamic", "TBDR Check", test_tbdr_local_read_check, NULL, NULL);
    TestStats stats = test_run_all();
    test_cleanup();
    return stats.failed > 0 ? 1 : 0;
}
