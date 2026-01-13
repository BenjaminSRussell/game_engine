// src/engine/rendering/post_processing/post_processing_test.h
//
// Purpose: End-to-end testing for post-processing effects header
// Tests TAA, bloom, SSAO, and SSR compute shader implementations

#ifndef POST_PROCESSING_TEST_H
#define POST_PROCESSING_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MAIN TEST FUNCTION
// ============================================================================

// Run all post-processing tests
// Returns true if all tests pass, false otherwise
bool run_post_processing_tests(void);

// ============================================================================
// INDIVIDUAL TEST FUNCTIONS
// ============================================================================

// Test TAA compute shader
bool test_taa_compute(void);

// Test bloom compute shader
bool test_bloom_compute(void);

// Test SSAO compute shader
bool test_ssao_compute(void);

// Test SSR compute shader
bool test_ssr_compute(void);

// Test post-processing integration
bool test_post_processing_integration(void);

#ifdef __cplusplus
}
#endif

#endif // POST_PROCESSING_TEST_H
