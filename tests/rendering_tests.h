/**
 * @file rendering_tests.h
 * @brief Rendering system tests with framebuffer validation
 *
 * Comprehensive test suite for rendering systems including framebuffer
 * contents validation, shader correctness, and rendering pipeline tests.
 */

#ifndef RENDERING_TESTS_H
#define RENDERING_TESTS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Rendering Test Configuration
// ========================================

typedef struct RenderingTestConfig {
    bool enable_verbose_output;         // Enable verbose test output
    bool enable_performance_tests;       // Enable performance benchmarks
    bool enable_visual_validation;      // Enable visual validation
    bool enable_screenshot_comparison;  // Enable screenshot comparison
    bool enable_offscreen_rendering;    // Enable offscreen rendering tests
    uint32_t framebuffer_width;         // Test framebuffer width
    uint32_t framebuffer_height;        // Test framebuffer height
    float pixel_tolerance;              // Pixel comparison tolerance
    const char *screenshot_dir;         // Screenshot directory
    const char *output_file;            // Output file for results
} RenderingTestConfig;

// ========================================
// Rendering Test Results
// ========================================

typedef struct RenderingTestResults {
    uint32_t total_tests;               // Total tests run
    uint32_t passed_tests;              // Tests that passed
    uint32_t failed_tests;              // Tests that failed
    uint32_t skipped_tests;             // Tests that were skipped
    double total_time_ms;               // Total execution time
    double average_frame_time_ms;        // Average frame time
    double max_frame_time_ms;            // Maximum frame time
    double min_frame_time_ms;            // Minimum frame time
    uint32_t pixels_tested;             // Total pixels tested
    uint32_t pixel_differences;         // Number of pixel differences
    float max_pixel_difference;         // Maximum pixel difference found
    char error_messages[4096];          // Accumulated error messages
    char performance_report[2048];      // Performance report
} RenderingTestResults;

// ========================================
// Framebuffer Data
// ========================================

typedef struct FramebufferData {
    uint32_t width;                     // Framebuffer width
    uint32_t height;                    // Framebuffer height
    uint32_t format;                    // Pixel format
    uint8_t *pixels;                    // Pixel data
    size_t pixel_count;                 // Number of pixels
    size_t data_size;                   // Size of pixel data in bytes
} FramebufferData;

// ========================================
// Test Pattern Types
// ========================================

typedef enum TestPattern {
    TEST_PATTERN_SOLID_COLOR,           // Solid color pattern
    TEST_PATTERN_GRADIENT,              // Gradient pattern
    TEST_PATTERN_CHECKERBOARD,          // Checkerboard pattern
    TEST_PATTERN_NOISE,                 // Noise pattern
    TEST_PATTERN_GEOMETRIC,             // Geometric shapes
    TEST_PATTERN_TEXT,                  // Text rendering
    TEST_PATTERN_TEXTURED,              // Textured pattern
    TEST_PATTERN_DEPTH_TEST,            // Depth test pattern
    TEST_PATTERN_STENCIL_TEST,          // Stencil test pattern
    TEST_PATTERN_BLEND_TEST             // Blending test pattern
} TestPattern;

// ========================================
// Global Test State
// ========================================

extern RenderingTestConfig g_rendering_test_config;
extern RenderingTestResults g_rendering_test_results;

// ========================================
// Test Suite Functions
// ========================================

/**
 * Initialize rendering test suite
 * @param config Test configuration
 * @return True if initialization successful
 */
bool rendering_tests_init(const RenderingTestConfig *config);

/**
 * Shutdown rendering test suite
 * @param generate_report Whether to generate final report
 */
void rendering_tests_shutdown(bool generate_report);

/**
 * Run all rendering tests
 * @return True if all tests pass
 */
bool rendering_run_all_tests(void);

/**
 * Get test results
 * @return Test results
 */
RenderingTestResults rendering_get_test_results(void);

/**
 * Print test summary
 */
void rendering_print_test_summary(void);

/**
 * Export test results to file
 * @param filename Output filename
 * @return True if export successful
 */
bool rendering_export_results(const char *filename);

// ========================================
// Framebuffer Management
// ========================================

/**
 * Create test framebuffer
 * @param width Framebuffer width
 * @param height Framebuffer height
 * @return Framebuffer data or NULL on failure
 */
FramebufferData* rendering_create_framebuffer(uint32_t width, uint32_t height);

/**
 * Destroy framebuffer
 * @param framebuffer Framebuffer to destroy
 */
void rendering_destroy_framebuffer(FramebufferData *framebuffer);

/**
 * Clear framebuffer with color
 * @param framebuffer Framebuffer to clear
 * @param r Red component (0-1)
 * @param g Green component (0-1)
 * @param b Blue component (0-1)
 * @param a Alpha component (0-1)
 */
void rendering_clear_framebuffer(FramebufferData *framebuffer, float r, float g, float b, float a);

/**
 * Read framebuffer contents
 * @param framebuffer Framebuffer to read
 * @return True if read successful
 */
bool rendering_read_framebuffer(FramebufferData *framebuffer);

/**
 * Save framebuffer to file
 * @param framebuffer Framebuffer to save
 * @param filename Output filename
 * @return True if save successful
 */
bool rendering_save_framebuffer(const FramebufferData *framebuffer, const char *filename);

// ========================================
// Pattern Generation Tests
// ========================================

/**
 * Test solid color rendering
 * @return True if test passes
 */
bool test_rendering_solid_color(void);

/**
 * Test gradient rendering
 * @return True if test passes
 */
bool test_rendering_gradient(void);

/**
 * Test checkerboard pattern rendering
 * @return True if test passes
 */
bool test_rendering_checkerboard(void);

/**
 * Test geometric shape rendering
 * @return True if test passes
 */
bool test_rendering_geometric_shapes(void);

/**
 * Test text rendering
 * @return True if test passes
 */
bool test_rendering_text(void);

/**
 * Test texture rendering
 * @return True if test passes
 */
bool test_rendering_textures(void);

// ========================================
// Shader Tests
// ========================================

/**
 * Test basic vertex shader
 * @return True if test passes
 */
bool test_shader_vertex_basic(void);

/**
 * Test basic fragment shader
 * @return True if test passes
 */
bool test_shader_fragment_basic(void);

/**
 * Test shader uniforms
 * @return True if test passes
 */
bool test_shader_uniforms(void);

/**
 * Test shader attributes
 * @return True if test passes
 */
bool test_shader_attributes(void);

/**
 * Test shader textures
 * @return True if test passes
 */
bool test_shader_textures(void);

/**
 * Test shader lighting
 * @return True if test passes
 */
bool test_shader_lighting(void);

// ========================================
// Rendering Pipeline Tests
// ========================================

/**
 * Test depth buffer functionality
 * @return True if test passes
 */
bool test_rendering_depth_buffer(void);

/**
 * Test stencil buffer functionality
 * @return True if test passes
 */
bool test_rendering_stencil_buffer(void);

/**
 * Test blending operations
 * @return True if test passes
 */
bool test_rendering_blending(void);

/**
 * Test culling operations
 * @return True if test passes
 */
bool test_rendering_culling(void);

/**
 * Test scissor test
 * @return True if test passes
 */
bool test_rendering_scissor_test(void);

/**
 * Test viewport operations
 * @return True if test passes
 */
bool test_rendering_viewport(void);

// ========================================
// Framebuffer Validation Tests
// ========================================

/**
 * Validate framebuffer contents against expected pattern
 * @param framebuffer Framebuffer to validate
 * @param pattern Expected pattern type
 * @param tolerance Validation tolerance
 * @return True if validation passes
 */
bool rendering_validate_framebuffer_pattern(const FramebufferData *framebuffer, 
                                         TestPattern pattern, 
                                         float tolerance);

/**
 * Compare two framebuffers
 * @param fb1 First framebuffer
 * @param fb2 Second framebuffer
 * @param tolerance Comparison tolerance
 * @param max_difference Output maximum difference
 * @return True if framebuffers match within tolerance
 */
bool rendering_compare_framebuffers(const FramebufferData *fb1, 
                                  const FramebufferData *fb2, 
                                  float tolerance, 
                                  float *max_difference);

/**
 * Validate pixel at specific coordinates
 * @param framebuffer Framebuffer to check
 * @param x X coordinate
 * @param y Y coordinate
 * @param expected_r Expected red component
 * @param expected_g Expected green component
 * @param expected_b Expected blue component
 * @param expected_a Expected alpha component
 * @param tolerance Comparison tolerance
 * @return True if pixel matches expected values
 */
bool rendering_validate_pixel(const FramebufferData *framebuffer, 
                             uint32_t x, uint32_t y,
                             float expected_r, float expected_g, 
                             float expected_b, float expected_a,
                             float tolerance);

/**
 * Validate framebuffer region
 * @param framebuffer Framebuffer to check
 * @param x Region X coordinate
 * @param y Region Y coordinate
 * @param width Region width
 * @param height Region height
 * @param expected_color Expected color
 * @param tolerance Comparison tolerance
 * @return True if region matches expected color
 */
bool rendering_validate_region(const FramebufferData *framebuffer,
                            uint32_t x, uint32_t y,
                            uint32_t width, uint32_t height,
                            float expected_color[4],
                            float tolerance);

// ========================================
// Performance Tests
// ========================================

/**
 * Performance test with many draw calls
 * @return True if test passes
 */
bool test_performance_many_draw_calls(void);

/**
 * Performance test with large textures
 * @return True if test passes
 */
bool test_performance_large_textures(void);

/**
 * Performance test with complex shaders
 * @return True if test passes
 */
bool test_performance_complex_shaders(void);

/**
 * Performance test with high resolution
 * @return True if test passes
 */
bool test_performance_high_resolution(void);

/**
 * Performance test with multiple framebuffers
 * @return True if test passes
 */
bool test_performance_multiple_framebuffers(void);

// ========================================
// Stress Tests
// ========================================

/**
 * Stress test with rapid framebuffer switches
 * @return True if test passes
 */
bool stress_test_rapid_framebuffer_switches(void);

/**
 * Stress test with memory pressure
 * @return True if test passes
 */
bool stress_test_memory_pressure(void);

/**
 * Stress test with shader compilation
 * @return True if test passes
 */
bool stress_test_shader_compilation(void);

/**
 * Stress test with texture loading
 * @return True if test passes
 */
bool stress_test_texture_loading(void);

// ========================================
// Utility Functions
// ========================================

/**
 * Generate test pattern
 * @param pattern Pattern type
 * @param framebuffer Output framebuffer
 * @return True if generation successful
 */
bool rendering_generate_pattern(TestPattern pattern, FramebufferData *framebuffer);

/**
 * Load test texture
 * @param filename Texture filename
 * @return Texture handle or 0 on failure
 */
uint32_t rendering_load_test_texture(const char *filename);

/**
 * Create test shader
 * @param vertex_source Vertex shader source
 * @param fragment_source Fragment shader source
 * @return Shader program handle or 0 on failure
 */
uint32_t rendering_create_test_shader(const char *vertex_source, const char *fragment_source);

/**
 * Get pixel color from framebuffer
 * @param framebuffer Framebuffer
 * @param x X coordinate
 * @param y Y coordinate
 * @param r Output red component
 * @param g Output green component
 * @param b Output blue component
 * @param a Output alpha component
 */
void rendering_get_pixel(const FramebufferData *framebuffer, 
                        uint32_t x, uint32_t y,
                        float *r, float *g, float *b, float *a);

/**
 * Set pixel color in framebuffer
 * @param framebuffer Framebuffer
 * @param x X coordinate
 * @param y Y coordinate
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @param a Alpha component
 */
void rendering_set_pixel(FramebufferData *framebuffer, 
                        uint32_t x, uint32_t y,
                        float r, float g, float b, float a);

/**
 * Calculate framebuffer checksum
 * @param framebuffer Framebuffer
 * @return Checksum value
 */
uint32_t rendering_calculate_checksum(const FramebufferData *framebuffer);

/**
 * Validate framebuffer integrity
 * @param framebuffer Framebuffer to validate
 * @return True if framebuffer is valid
 */
bool rendering_validate_framebuffer_integrity(const FramebufferData *framebuffer);

// ========================================
// Screenshot and Comparison Functions
// ========================================

/**
 * Take screenshot of current framebuffer
 * @param filename Output filename
 * @return True if screenshot successful
 */
bool rendering_take_screenshot(const char *filename);

/**
 * Compare screenshot with reference
 * @param screenshot_file Screenshot file
 * @param reference_file Reference file
 * @param tolerance Comparison tolerance
 * @return True if screenshots match within tolerance
 */
bool rendering_compare_screenshot(const char *screenshot_file, 
                                 const char *reference_file, 
                                 float tolerance);

/**
 * Generate difference image
 * @param file1 First image file
 * @param file2 Second image file
 * @param output_file Output difference image file
 * @return True if difference image generated successfully
 */
bool rendering_generate_difference_image(const char *file1, 
                                         const char *file2, 
                                         const char *output_file);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_TESTS_H */
