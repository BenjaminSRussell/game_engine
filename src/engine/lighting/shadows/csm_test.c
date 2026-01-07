/*
 * csm_test.c
 * Unit tests for CSM logic
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include <stdio.h>
#include <assert.h>
#include <include/math/math.h>

#include "lighting/cascades/cascade_resolution.h"
#include "lighting/shadows/cascade_splits.h"
#include "lighting/cascades/cascade_selection.h"

void test_resolution_calc(void) {
    uint32_t res = cascade_calculate_resolution(100.0f, 10.0f);
    // 100 * 10 = 1000 -> next POT = 1024
    assert(res == 1024);
    
    res = cascade_calculate_resolution(10.0f, 10.0f);
    // 10 * 10 = 100 -> next POT = 128 -> min clamp = 256
    assert(res == 256);
    
    printf("Resolution calculation tests passed.\n");
}

void test_split_calculation(void) {
    float splits[4];
    // 0.5 lambda (hybrid)
    cascade_calculate_split_depths(1.0f, 100.0f, 0.5f, 4, splits);
    
    // Check ordering
    assert(splits[0] < splits[1]);
    assert(splits[1] < splits[2]);
    assert(splits[2] < splits[3]);
    assert(fabs(splits[3] - 100.0f) < 0.001f); // Last split should be far plane
    
    printf("Split calculation tests passed.\n");
}

int main(void) {
    printf("Running CSM Logic Tests...\n");
    test_resolution_calc();
    test_split_calculation();
    printf("All tests passed!\n");
    return 0;
}
