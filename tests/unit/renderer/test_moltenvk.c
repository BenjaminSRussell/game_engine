#include <renderer/moltenvk_config.h>
#include "test_framework_unified.h"

static TestResult test_moltenvk_detection(void) {
    // We can't easily mock VkPhysicalDevice without thorough stubbing,
    // but we can test the config structure defaults
    
    MoltenVKConfig config = {0};
    u64 alignment = moltenvk_get_min_buffer_alignment(&config);
    
    TEST_ASSERT_EQ(alignment, 256, "Default alignment should be conservative 256");
    
    return TEST_PASS;
}

static TestResult test_moltenvk_features(void) {
    MoltenVKConfig config = {0};
    config.supports_geometry_shaders = false;
    
    bool has_geo = moltenvk_supports_feature(&config, "geometry_shaders");
    TEST_ASSERT_FALSE(has_geo, "Reports no geometry shaders");
    
    config.supports_geometry_shaders = true;
    has_geo = moltenvk_supports_feature(&config, "geometry_shaders");
    TEST_ASSERT_TRUE(has_geo, "Reports geometry shaders");
    
    return TEST_PASS;
}

int main(void) {
    test_init();
    test_register("Renderer:MoltenVK", "Detection Defaults", test_moltenvk_detection, NULL, NULL);
    test_register("Renderer:MoltenVK", "Feature Query", test_moltenvk_features, NULL, NULL);
    TestStats stats = test_run_all();
    test_cleanup();
    return stats.failed > 0 ? 1 : 0;
}
