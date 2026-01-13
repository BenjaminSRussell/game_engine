/**
 * @file metal_compatibility_test.c
 * @brief macOS 10.15+ Metal backend compatibility test
 * Part of TODO-0069
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

int test_metal_compatibility() {
    printf("Testing Metal compatibility...\n");
    
#ifdef __APPLE__
    if (@available(macOS 10.15, *)) {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device) {
            printf("✅ Metal compatibility verified\n");
            return 0;
        }
    }
    printf("❌ Metal not available or macOS < 10.15\n");
#else
    printf("⚠️  Metal only available on macOS\n");
#endif
    return -1;
}

int main() {
    return test_metal_compatibility();
}
