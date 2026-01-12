#include "rendering_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

RenderingTestConfig g_rendering_test_config;
RenderingTestResults g_rendering_test_results;

static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

bool rendering_tests_init(const RenderingTestConfig *config) {
    if (config) {
        g_rendering_test_config = *config;
    } else {
        g_rendering_test_config = (RenderingTestConfig){
            .enable_verbose_output = false,
            .enable_performance_tests = true,
            .enable_visual_validation = false,
            .enable_screenshot_comparison = false,
            .enable_offscreen_rendering = true,
            .framebuffer_width = 512,
            .framebuffer_height = 512,
            .pixel_tolerance = 0.01f,
            .screenshot_dir = "./test_screenshots",
            .output_file = NULL
        };
    }
    
    memset(&g_rendering_test_results, 0, sizeof(g_rendering_test_results));
    
    srand((unsigned int)time(NULL));
    return true;
}

void rendering_tests_shutdown(bool generate_report) {
    if (generate_report) {
        rendering_print_test_summary();
        if (g_rendering_test_config.output_file) {
            rendering_export_results(g_rendering_test_config.output_file);
        }
    }
}

FramebufferData* rendering_create_framebuffer(uint32_t width, uint32_t height) {
    FramebufferData *fb = malloc(sizeof(FramebufferData));
    if (!fb) return NULL;
    
    fb->width = width;
    fb->height = height;
    fb->format = 0; // RGBA8
    fb->pixel_count = (size_t)width * height;
    fb->data_size = fb->pixel_count * 4; // 4 bytes per pixel (RGBA)
    fb->pixels = malloc(fb->data_size);
    
    if (!fb->pixels) {
        free(fb);
        return NULL;
    }
    
    // Initialize to black
    memset(fb->pixels, 0, fb->data_size);
    
    return fb;
}

void rendering_destroy_framebuffer(FramebufferData *framebuffer) {
    if (framebuffer) {
        if (framebuffer->pixels) {
            free(framebuffer->pixels);
        }
        free(framebuffer);
    }
}

void rendering_clear_framebuffer(FramebufferData *framebuffer, float r, float g, float b, float a) {
    if (!framebuffer || !framebuffer->pixels) return;
    
    uint8_t ur = (uint8_t)(r * 255.0f);
    uint8_t ug = (uint8_t)(g * 255.0f);
    uint8_t ub = (uint8_t)(b * 255.0f);
    uint8_t ua = (uint8_t)(a * 255.0f);
    
    for (size_t i = 0; i < framebuffer->pixel_count; i++) {
        size_t offset = i * 4;
        framebuffer->pixels[offset + 0] = ur;
        framebuffer->pixels[offset + 1] = ug;
        framebuffer->pixels[offset + 2] = ub;
        framebuffer->pixels[offset + 3] = ua;
    }
}

void rendering_get_pixel(const FramebufferData *framebuffer, 
                        uint32_t x, uint32_t y,
                        float *r, float *g, float *b, float *a) {
    if (!framebuffer || !framebuffer->pixels || 
        x >= framebuffer->width || y >= framebuffer->height) {
        if (r) *r = 0.0f;
        if (g) *g = 0.0f;
        if (b) *b = 0.0f;
        if (a) *a = 0.0f;
        return;
    }
    
    size_t offset = (y * framebuffer->width + x) * 4;
    if (r) *r = framebuffer->pixels[offset + 0] / 255.0f;
    if (g) *g = framebuffer->pixels[offset + 1] / 255.0f;
    if (b) *b = framebuffer->pixels[offset + 2] / 255.0f;
    if (a) *a = framebuffer->pixels[offset + 3] / 255.0f;
}

void rendering_set_pixel(FramebufferData *framebuffer, 
                        uint32_t x, uint32_t y,
                        float r, float g, float b, float a) {
    if (!framebuffer || !framebuffer->pixels || 
        x >= framebuffer->width || y >= framebuffer->height) {
        return;
    }
    
    size_t offset = (y * framebuffer->width + x) * 4;
    framebuffer->pixels[offset + 0] = (uint8_t)(r * 255.0f);
    framebuffer->pixels[offset + 1] = (uint8_t)(g * 255.0f);
    framebuffer->pixels[offset + 2] = (uint8_t)(b * 255.0f);
    framebuffer->pixels[offset + 3] = (uint8_t)(a * 255.0f);
}

bool rendering_validate_pixel(const FramebufferData *framebuffer, 
                             uint32_t x, uint32_t y,
                             float expected_r, float expected_g, 
                             float expected_b, float expected_a,
                             float tolerance) {
    float actual_r, actual_g, actual_b, actual_a;
    rendering_get_pixel(framebuffer, x, y, &actual_r, &actual_g, &actual_b, &actual_a);
    
    float dr = fabsf(actual_r - expected_r);
    float dg = fabsf(actual_g - expected_g);
    float db = fabsf(actual_b - expected_b);
    float da = fabsf(actual_a - expected_a);
    
    return (dr <= tolerance) && (dg <= tolerance) && (db <= tolerance) && (da <= tolerance);
}

bool rendering_validate_region(const FramebufferData *framebuffer,
                            uint32_t x, uint32_t y,
                            uint32_t width, uint32_t height,
                            float expected_color[4],
                            float tolerance) {
    if (!framebuffer || !expected_color) return false;
    
    for (uint32_t py = y; py < y + height && py < framebuffer->height; py++) {
        for (uint32_t px = x; px < x + width && px < framebuffer->width; px++) {
            if (!rendering_validate_pixel(framebuffer, px, py,
                                         expected_color[0], expected_color[1],
                                         expected_color[2], expected_color[3],
                                         tolerance)) {
                return false;
            }
        }
    }
    
    return true;
}

bool rendering_compare_framebuffers(const FramebufferData *fb1, 
                                  const FramebufferData *fb2, 
                                  float tolerance, 
                                  float *max_difference) {
    if (!fb1 || !fb2 || 
        fb1->width != fb2->width || 
        fb1->height != fb2->height) {
        return false;
    }
    
    float max_diff = 0.0f;
    uint32_t pixels_tested = 0;
    uint32_t pixel_differences = 0;
    
    for (uint32_t y = 0; y < fb1->height; y++) {
        for (uint32_t x = 0; x < fb1->width; x++) {
            float r1, g1, b1, a1;
            float r2, g2, b2, a2;
            
            rendering_get_pixel(fb1, x, y, &r1, &g1, &b1, &a1);
            rendering_get_pixel(fb2, x, y, &r2, &g2, &b2, &a2);
            
            float dr = fabsf(r1 - r2);
            float dg = fabsf(g1 - g2);
            float db = fabsf(b1 - b2);
            float da = fabsf(a1 - a2);
            
            float diff = fmaxf(fmaxf(dr, dg), fmaxf(db, da));
            max_diff = fmaxf(max_diff, diff);
            
            if (diff > tolerance) {
                pixel_differences++;
            }
            
            pixels_tested++;
        }
    }
    
    g_rendering_test_results.pixels_tested += pixels_tested;
    g_rendering_test_results.pixel_differences += pixel_differences;
    g_rendering_test_results.max_pixel_difference = fmaxf(g_rendering_test_results.max_pixel_difference, max_diff);
    
    if (max_difference) {
        *max_difference = max_diff;
    }
    
    return max_diff <= tolerance;
}

bool rendering_generate_pattern(TestPattern pattern, FramebufferData *framebuffer) {
    if (!framebuffer) return false;
    
    switch (pattern) {
        case TEST_PATTERN_SOLID_COLOR: {
            float color[4] = {1.0f, 0.0f, 0.0f, 1.0f}; // Red
            rendering_clear_framebuffer(framebuffer, color[0], color[1], color[2], color[3]);
            break;
        }
        
        case TEST_PATTERN_GRADIENT: {
            for (uint32_t y = 0; y < framebuffer->height; y++) {
                for (uint32_t x = 0; x < framebuffer->width; x++) {
                    float r = (float)x / (float)framebuffer->width;
                    float g = (float)y / (float)framebuffer->height;
                    float b = 0.0f;
                    float a = 1.0f;
                    rendering_set_pixel(framebuffer, x, y, r, g, b, a);
                }
            }
            break;
        }
        
        case TEST_PATTERN_CHECKERBOARD: {
            uint32_t checker_size = 16;
            for (uint32_t y = 0; y < framebuffer->height; y++) {
                for (uint32_t x = 0; x < framebuffer->width; x++) {
                    bool white = ((x / checker_size) + (y / checker_size)) % 2 == 0;
                    float color = white ? 1.0f : 0.0f;
                    rendering_set_pixel(framebuffer, x, y, color, color, color, 1.0f);
                }
            }
            break;
        }
        
        case TEST_PATTERN_NOISE: {
            for (uint32_t y = 0; y < framebuffer->height; y++) {
                for (uint32_t x = 0; x < framebuffer->width; x++) {
                    float noise = (float)rand() / (float)RAND_MAX;
                    rendering_set_pixel(framebuffer, x, y, noise, noise, noise, 1.0f);
                }
            }
            break;
        }
        
        default:
            return false;
    }
    
    return true;
}

bool test_rendering_solid_color(void) {
    FramebufferData *fb = rendering_create_framebuffer(
        g_rendering_test_config.framebuffer_width,
        g_rendering_test_config.framebuffer_height);
    if (!fb) return false;
    
    // Generate solid red pattern
    if (!rendering_generate_pattern(TEST_PATTERN_SOLID_COLOR, fb)) {
        rendering_destroy_framebuffer(fb);
        return false;
    }
    
    // Validate center pixel is red
    uint32_t cx = fb->width / 2;
    uint32_t cy = fb->height / 2;
    bool valid = rendering_validate_pixel(fb, cx, cy, 1.0f, 0.0f, 0.0f, 1.0f, 
                                       g_rendering_test_config.pixel_tolerance);
    
    rendering_destroy_framebuffer(fb);
    return valid;
}

bool test_rendering_gradient(void) {
    FramebufferData *fb = rendering_create_framebuffer(
        g_rendering_test_config.framebuffer_width,
        g_rendering_test_config.framebuffer_height);
    if (!fb) return false;
    
    // Generate gradient pattern
    if (!rendering_generate_pattern(TEST_PATTERN_GRADIENT, fb)) {
        rendering_destroy_framebuffer(fb);
        return false;
    }
    
    // Validate corners
    bool valid = true;
    
    // Top-left should be black
    valid &= rendering_validate_pixel(fb, 0, 0, 0.0f, 0.0f, 0.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    
    // Top-right should be red
    valid &= rendering_validate_pixel(fb, fb->width - 1, 0, 1.0f, 0.0f, 0.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    
    // Bottom-left should be green
    valid &= rendering_validate_pixel(fb, 0, fb->height - 1, 0.0f, 1.0f, 0.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    
    // Bottom-right should be yellow
    valid &= rendering_validate_pixel(fb, fb->width - 1, fb->height - 1, 1.0f, 1.0f, 0.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    
    rendering_destroy_framebuffer(fb);
    return valid;
}

bool test_rendering_checkerboard(void) {
    FramebufferData *fb = rendering_create_framebuffer(
        g_rendering_test_config.framebuffer_width,
        g_rendering_test_config.framebuffer_height);
    if (!fb) return false;
    
    // Generate checkerboard pattern
    if (!rendering_generate_pattern(TEST_PATTERN_CHECKERBOARD, fb)) {
        rendering_destroy_framebuffer(fb);
        return false;
    }
    
    // Validate pattern
    bool valid = true;
    
    // Check alternating pattern
    valid &= rendering_validate_pixel(fb, 0, 0, 1.0f, 1.0f, 1.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    valid &= rendering_validate_pixel(fb, 16, 0, 0.0f, 0.0f, 0.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    valid &= rendering_validate_pixel(fb, 0, 16, 0.0f, 0.0f, 0.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    valid &= rendering_validate_pixel(fb, 16, 16, 1.0f, 1.0f, 1.0f, 1.0f,
                                    g_rendering_test_config.pixel_tolerance);
    
    rendering_destroy_framebuffer(fb);
    return valid;
}

bool test_framebuffer_contents_validation(void) {
    // Create two identical framebuffers
    FramebufferData *fb1 = rendering_create_framebuffer(256, 256);
    FramebufferData *fb2 = rendering_create_framebuffer(256, 256);
    
    if (!fb1 || !fb2) {
        rendering_destroy_framebuffer(fb1);
        rendering_destroy_framebuffer(fb2);
        return false;
    }
    
    // Generate same pattern in both
    rendering_generate_pattern(TEST_PATTERN_GRADIENT, fb1);
    rendering_generate_pattern(TEST_PATTERN_GRADIENT, fb2);
    
    // Compare them
    float max_diff;
    bool identical = rendering_compare_framebuffers(fb1, fb2, 
                                                  g_rendering_test_config.pixel_tolerance,
                                                  &max_diff);
    
    // Modify one pixel in fb2
    rendering_set_pixel(fb2, 128, 128, 1.0f, 1.0f, 1.0f, 1.0f);
    
    // Compare again - should detect difference
    bool different = !rendering_compare_framebuffers(fb1, fb2,
                                                   g_rendering_test_config.pixel_tolerance,
                                                   &max_diff);
    
    rendering_destroy_framebuffer(fb1);
    rendering_destroy_framebuffer(fb2);
    
    return identical && different;
}

bool test_performance_many_draw_calls(void) {
    FramebufferData *fb = rendering_create_framebuffer(
        g_rendering_test_config.framebuffer_width,
        g_rendering_test_config.framebuffer_height);
    if (!fb) return false;
    
    uint64_t start_time = get_time_ms();
    
    // Simulate many draw calls by setting pixels
    for (int i = 0; i < 10000; i++) {
        uint32_t x = rand() % fb->width;
        uint32_t y = rand() % fb->height;
        float r = (float)rand() / (float)RAND_MAX;
        float g = (float)rand() / (float)RAND_MAX;
        float b = (float)rand() / (float)RAND_MAX;
        rendering_set_pixel(fb, x, y, r, g, b, 1.0f);
    }
    
    uint64_t end_time = get_time_ms();
    double execution_time = (double)(end_time - start_time);
    
    g_rendering_test_results.average_frame_time_ms = execution_time;
    g_rendering_test_results.max_frame_time_ms = execution_time;
    g_rendering_test_results.min_frame_time_ms = execution_time;
    
    rendering_destroy_framebuffer(fb);
    return true;
}

bool rendering_run_all_tests(void) {
    uint64_t start_time = get_time_ms();
    
    bool (*tests[])(void) = {
        test_rendering_solid_color,
        test_rendering_gradient,
        test_rendering_checkerboard,
        test_framebuffer_contents_validation,
        test_performance_many_draw_calls
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < num_tests; i++) {
        g_rendering_test_results.total_tests++;
        
        uint64_t test_start = get_time_ms();
        bool passed = tests[i]();
        uint64_t test_end = get_time_ms();
        
        if (passed) {
            g_rendering_test_results.passed_tests++;
        } else {
            g_rendering_test_results.failed_tests++;
        }
        
        // Update frame time statistics
        double test_time = (double)(test_end - test_start);
        if (g_rendering_test_results.min_frame_time_ms == 0.0) {
            g_rendering_test_results.min_frame_time_ms = test_time;
        }
        g_rendering_test_results.min_frame_time_ms = fmin(g_rendering_test_results.min_frame_time_ms, test_time);
        g_rendering_test_results.max_frame_time_ms = fmax(g_rendering_test_results.max_frame_time_ms, test_time);
    }
    
    g_rendering_test_results.total_time_ms = get_time_ms() - start_time;
    g_rendering_test_results.average_frame_time_ms = 
        g_rendering_test_results.total_time_ms / g_rendering_test_results.total_tests;
    
    return g_rendering_test_results.failed_tests == 0;
}

void rendering_print_test_summary(void) {
    printf("\n=== Rendering Test Summary ===\n");
    printf("Total Tests: %u\n", g_rendering_test_results.total_tests);
    printf("Passed: %u\n", g_rendering_test_results.passed_tests);
    printf("Failed: %u\n", g_rendering_test_results.failed_tests);
    printf("Skipped: %u\n", g_rendering_test_results.skipped_tests);
    printf("Total Time: %.2f ms\n", g_rendering_test_results.total_time_ms);
    printf("Average Frame Time: %.3f ms\n", g_rendering_test_results.average_frame_time_ms);
    printf("Max Frame Time: %.3f ms\n", g_rendering_test_results.max_frame_time_ms);
    printf("Min Frame Time: %.3f ms\n", g_rendering_test_results.min_frame_time_ms);
    printf("Pixels Tested: %u\n", g_rendering_test_results.pixels_tested);
    printf("Pixel Differences: %u\n", g_rendering_test_results.pixel_differences);
    printf("Max Pixel Difference: %.6f\n", g_rendering_test_results.max_pixel_difference);
    
    if (g_rendering_test_results.failed_tests > 0) {
        printf("\nFailed Tests:\n%s\n", g_rendering_test_results.error_messages);
    }
    
    printf("============================\n");
}

RenderingTestResults rendering_get_test_results(void) {
    return g_rendering_test_results;
}

bool rendering_export_results(const char *filename) {
    if (!filename) return false;
    
    FILE *file = fopen(filename, "w");
    if (!file) return false;
    
    fprintf(file, "Rendering Test Results\n");
    fprintf(file, "====================\n\n");
    
    fprintf(file, "Configuration:\n");
    fprintf(file, "  Framebuffer: %ux%u\n", 
            g_rendering_test_config.framebuffer_width,
            g_rendering_test_config.framebuffer_height);
    fprintf(file, "  Pixel Tolerance: %.6f\n\n", g_rendering_test_config.pixel_tolerance);
    
    fprintf(file, "Results:\n");
    fprintf(file, "  Total Tests: %u\n", g_rendering_test_results.total_tests);
    fprintf(file, "  Passed: %u\n", g_rendering_test_results.passed_tests);
    fprintf(file, "  Failed: %u\n", g_rendering_test_results.failed_tests);
    fprintf(file, "  Skipped: %u\n", g_rendering_test_results.skipped_tests);
    fprintf(file, "  Total Time: %.2f ms\n", g_rendering_test_results.total_time_ms);
    fprintf(file, "  Average Frame Time: %.3f ms\n", g_rendering_test_results.average_frame_time_ms);
    fprintf(file, "  Max Frame Time: %.3f ms\n", g_rendering_test_results.max_frame_time_ms);
    fprintf(file, "  Min Frame Time: %.3f ms\n", g_rendering_test_results.min_frame_time_ms);
    fprintf(file, "  Pixels Tested: %u\n", g_rendering_test_results.pixels_tested);
    fprintf(file, "  Pixel Differences: %u\n", g_rendering_test_results.pixel_differences);
    fprintf(file, "  Max Pixel Difference: %.6f\n\n", g_rendering_test_results.max_pixel_difference);
    
    if (g_rendering_test_results.failed_tests > 0) {
        fprintf(file, "Failed Tests:\n%s\n", g_rendering_test_results.error_messages);
    }
    
    fprintf(file, "Performance Report:\n%s\n", g_rendering_test_results.performance_report);
    
    fclose(file);
    return true;
}
