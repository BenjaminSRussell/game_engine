/*
 * cascade_resolution_test.c
 * Shadow-cascade map size, texel density, and quality presets.
 * Compiles cascade_resolution.c directly and does not link Engine.
 */

#include "lighting/cascades/cascade_resolution.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static int g_fails;

static void expect_u32(const char *label, uint32_t got, uint32_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s got %u want %u\n", label, got, want);
        g_fails++;
        return;
    }
    printf("PASS: %s (%u)\n", label, got);
}

static void expect_true(const char *label, int cond) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", label);
        g_fails++;
        return;
    }
    printf("PASS: %s\n", label);
}

static int is_pow2(uint32_t v) {
    return v != 0 && (v & (v - 1)) == 0;
}

static void test_resolution(void) {
    expect_u32("non-positive density", cascade_calculate_resolution(128.0f, 0.0f), 1024);
    expect_u32("negative density", cascade_calculate_resolution(4096.0f, -3.0f), 1024);
    expect_u32("zero world size clamps to min", cascade_calculate_resolution(0.0f, 4.0f), 256);
    expect_u32("tiny request clamps to min", cascade_calculate_resolution(10.0f, 1.0f), 256);
    expect_u32("exact min power of two", cascade_calculate_resolution(256.0f, 1.0f), 256);
    expect_u32("rounds up to next power of two", cascade_calculate_resolution(257.0f, 1.0f), 512);
    expect_u32("fractional world size ceils first", cascade_calculate_resolution(300.2f, 1.0f), 512);
    expect_u32("density scales the request", cascade_calculate_resolution(100.0f, 1.5f), 256);
    expect_u32("exact 4096", cascade_calculate_resolution(4096.0f, 1.0f), 4096);
    expect_u32("just over 4096", cascade_calculate_resolution(4097.0f, 1.0f), 8192);
    expect_u32("exact max", cascade_calculate_resolution(8192.0f, 1.0f), 8192);
    expect_u32("above max clamps", cascade_calculate_resolution(8193.0f, 1.0f), 8192);
    expect_u32("huge request clamps to max", cascade_calculate_resolution(100000.0f, 2.0f), 8192);

    {
        uint32_t res = cascade_calculate_resolution(180.0f, 3.0f);
        float density = cascade_calculate_texel_density(res, 180.0f);
        expect_true("unclamped resolution is a power of two", is_pow2(res));
        expect_true("unclamped resolution meets texel target", density + 0.001f >= 3.0f);
        expect_true("unclamped resolution stays inside the map limits",
                    res >= 256 && res <= 8192);
    }

    {
        uint32_t res = cascade_calculate_resolution(100000.0f, 4.0f);
        float density = cascade_calculate_texel_density(res, 100000.0f);
        expect_u32("over-max stays at the cap", res, 8192);
        expect_true("capped map cannot meet an impossible density", density < 4.0f);
    }
}

static void test_texel_density(void) {
    float density;

    expect_true("non-positive world size",
                cascade_calculate_texel_density(2048, 0.0f) == 0.0f);
    expect_true("world size at the epsilon cutoff",
                cascade_calculate_texel_density(2048, 0.001f) == 0.0f);
    expect_true("negative world size",
                cascade_calculate_texel_density(512, -8.0f) == 0.0f);

    density = cascade_calculate_texel_density(1024, 64.0f);
    expect_true("1024 texels over 64 units", fabsf(density - 16.0f) < 0.001f);

    density = cascade_calculate_texel_density(512, 0.0011f);
    expect_true("just above the cutoff still divides", density > 400000.0f);
    expect_true("just above the cutoff matches resolution / size",
                fabsf(density - (512.0f / 0.0011f)) < 1.0f);
}

static void expect_uniform(const char *label, cascade_quality_preset_t preset,
                           uint32_t count, uint32_t want) {
    uint32_t values[8];
    uint32_t i;
    char item[96];

    for (i = 0; i < 8; i++) {
        values[i] = 0xA5A5A5A5u;
    }
    cascade_apply_quality_preset(preset, values, count);
    for (i = 0; i < count; i++) {
        snprintf(item, sizeof item, "%s[%u]", label, i);
        expect_u32(item, values[i], want);
    }
    for (i = count; i < 8; i++) {
        snprintf(item, sizeof item, "%s leaves slot %u", label, i);
        expect_u32(item, values[i], 0xA5A5A5A5u);
    }
}

static void test_quality_preset(void) {
    uint32_t sentinel = 0x11111111u;

    cascade_apply_quality_preset(CASCADE_QUALITY_HIGH, NULL, 4);
    printf("PASS: null output is a no-op\n");

    cascade_apply_quality_preset(CASCADE_QUALITY_ULTRA, &sentinel, 0);
    expect_u32("zero cascade count is a no-op", sentinel, 0x11111111u);

    expect_uniform("low", CASCADE_QUALITY_LOW, 3, 512);
    expect_uniform("medium", CASCADE_QUALITY_MEDIUM, 1, 1024);
    expect_uniform("high", CASCADE_QUALITY_HIGH, 4, 2048);
    expect_uniform("ultra", CASCADE_QUALITY_ULTRA, 5, 4096);
    expect_uniform("unknown preset", (cascade_quality_preset_t)99, 2, 1024);
}

int main(void) {
    test_resolution();
    test_texel_density();
    test_quality_preset();
    if (g_fails != 0) {
        fprintf(stderr, "FAIL: %d cascade resolution checks failed\n", g_fails);
        return 1;
    }
    printf("PASS: cascade resolution\n");
    return 0;
}
