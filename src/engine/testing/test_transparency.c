/*
 * test_transparency.c
 * Unit tests for transparency system.
 */

#include "../rendering/forward/transparency.h"
#include <core/logger.h>
#include <assert.h>

void test_Transparency_BlendModes(void) {
    transparency_system_init();
    
    transparency_set_blend_state(BLEND_MODE_ALPHA);
    // Verify internal state (if accessible or mocked backend)
    
    transparency_set_blend_state(BLEND_MODE_ADDITIVE);
    assert(transparency_is_order_independent(BLEND_MODE_ADDITIVE) == true);
    
    transparency_set_blend_state(BLEND_MODE_OPAQUE);
    assert(transparency_requires_sorting(BLEND_MODE_OPAQUE) == false);
    
    LOG_INFO("test_Transparency_BlendModes passed");
}

void test_Transparency_Sorting(void) {
    // Test helper function logic
    assert(transparency_requires_sorting(BLEND_MODE_ALPHA) == true);
    assert(transparency_requires_sorting(BLEND_MODE_PREMULTIPLIED) == true);
    
    // In a real integration test, we would submit objects and verify sorting order
    LOG_INFO("test_Transparency_Sorting passed");
}

void register_transparency_tests(void) {
    test_Transparency_BlendModes();
    test_Transparency_Sorting();
}
