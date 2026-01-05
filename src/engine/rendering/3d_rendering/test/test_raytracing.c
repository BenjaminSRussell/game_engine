/*
 * test_raytracing.c
 * Integration test for the Ray Tracing subsystem
 */

#include "../acceleration_structures/blas_builder.h"
#include "../acceleration_structures/tlas_builder.h"
#include "../raytracing/shadows_rt/rt_shadow_rays.h"
#include "../raytracing/reflections_rt/rt_reflection_rays.h"
#include "../raytracing/gi_rt/ddgi_probes.h"
#include <stdio.h>
#include <assert.h>

void test_acceleration_structures() {
    printf("Testing Acceleration Structures...\n");
    
    // BLAS
    int res = acceleration_structures_blas_builder_init();
    assert(res == 0);
    printf("BLAS Init OK\n");

    acceleration_structures_blas_builder_handle_t blas_handle = {0};
    acceleration_structures_blas_builder_desc_t blas_desc = {0};
    res = acceleration_structures_blas_builder_create(&blas_handle, &blas_desc);
    assert(res == 0);
    printf("BLAS Create OK\n");

    acceleration_structures_blas_builder_destroy(blas_handle);
    acceleration_structures_blas_builder_shutdown();

    // TLAS
    res = acceleration_structures_tlas_builder_init();
    assert(res == 0);
    printf("TLAS Init OK\n");

    acceleration_structures_tlas_builder_handle_t tlas_handle = {0};
    acceleration_structures_tlas_builder_desc_t tlas_desc = {0};
    res = acceleration_structures_tlas_builder_create(&tlas_handle, &tlas_desc);
    assert(res == 0);
    printf("TLAS Create OK\n");

    acceleration_structures_tlas_builder_destroy(tlas_handle);
    acceleration_structures_tlas_builder_shutdown();
}

void test_rt_shadows() {
    printf("Testing RT Shadows...\n");

    int res = raytracing_rt_shadow_rays_init();
    assert(res == 0);
    printf("Shadow Rays Init OK\n");

    raytracing_rt_shadow_rays_handle_t handle = {0};
    raytracing_rt_shadow_rays_desc_t desc = {0};
    res = raytracing_rt_shadow_rays_create(&handle, &desc);
    assert(res == 0);
    printf("Shadow Rays Context Create OK\n");

    raytracing_rt_shadow_rays_destroy(handle);
    raytracing_rt_shadow_rays_shutdown();
}

void test_rt_reflections() {
    printf("Testing RT Reflections...\n");

    int res = raytracing_rt_reflection_rays_init();
    assert(res == 0);
    printf("Reflection Rays Init OK\n");

    raytracing_rt_reflection_rays_handle_t handle = {0};
    raytracing_rt_reflection_rays_desc_t desc = {0};
    res = raytracing_rt_reflection_rays_create(&handle, &desc);
    assert(res == 0);
    printf("Reflection Rays Context Create OK\n");

    raytracing_rt_reflection_rays_destroy(handle);
    raytracing_rt_reflection_rays_shutdown();
}

void test_ddgi() {
    printf("Testing DDGI...\n");

    int res = raytracing_ddgi_probes_init();
    assert(res == 0);
    printf("DDGI Probes Init OK\n");

    raytracing_ddgi_probes_handle_t handle = {0};
    raytracing_ddgi_probes_desc_t desc = {0};
    res = raytracing_ddgi_probes_create(&handle, &desc);
    assert(res == 0);
    printf("DDGI Probes Create OK\n");

    raytracing_ddgi_probes_destroy(handle);
    raytracing_ddgi_probes_shutdown();
}

int main() {
    printf("=== Starting Ray Tracing Integration Tests ===\n");
    test_acceleration_structures();
    test_rt_shadows();
    test_rt_reflections();
    test_ddgi();
    printf("=== All Ray Tracing Tests Passed ===\n");
    return 0;
}
