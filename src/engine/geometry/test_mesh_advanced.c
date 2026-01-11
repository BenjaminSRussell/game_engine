#include "geometry/mesh.h"
#include "geometry/mesh_primitives.h"
#include "geometry/mesh_deform.h"
#include "geometry/mesh_gpu.h"
#include "geometry/mesh_material.h"
#include "geometry/mesh_utils.h"
#include "geometry/mesh_optimize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Advanced test framework
static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        g_tests_run++; \
        if (condition) { \
            g_tests_passed++; \
            printf("✓ PASS: %s\n", message); \
        } else { \
            g_tests_failed++; \
            printf("✗ FAIL: %s\n", message); \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQUAL(a, b, tolerance, message) \
    TEST_ASSERT(fabsf((a) - (b)) < (tolerance), message)

// Mock Metal device for testing
typedef struct mock_metal_device {
    int dummy;
} mock_metal_device_t;

static void test_blend_shapes() {
    printf("\n=== Testing Blend Shapes ===\n");
    
    // Create base mesh
    mesh_t* mesh = mesh_create("BlendShapeTest");
    mesh_allocate_buffers(mesh, 4, 6);
    
    vertex_t base_vertices[4] = {
        {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}},
        {{1, 0, 0}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}},
        {{1, 1, 0}, {0, 1, 0}, {1, 1}, {1, 0, 0, 1}},
        {{0, 1, 0}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}}
    };
    mesh_set_vertices(mesh, base_vertices, 4, 0);
    
    // Create blend shape data
    blend_shape_t smile_shape = {0};
    strcpy(smile_shape.name, "Smile");
    smile_shape.vertex_count = 4;
    smile_shape.weight = 0.0f;
    
    // Allocate delta data
    smile_shape.delta_positions = (Vec3*)malloc(4 * sizeof(Vec3));
    smile_shape.delta_normals = (Vec3*)malloc(4 * sizeof(Vec3));
    
    // Set some delta values
    smile_shape.delta_positions[0] = (Vec3){0, 0.1f, 0};
    smile_shape.delta_positions[1] = (Vec3){0, 0.1f, 0};
    smile_shape.delta_positions[2] = (Vec3){0, 0.1f, 0};
    smile_shape.delta_positions[3] = (Vec3){0, 0.1f, 0};
    
    smile_shape.delta_normals[0] = (Vec3){0, 0.1f, 0};
    smile_shape.delta_normals[1] = (Vec3){0, 0.1f, 0};
    smile_shape.delta_normals[2] = (Vec3){0, 0.1f, 0};
    smile_shape.delta_normals[3] = (Vec3){0, 0.1f, 0};
    
    // Add blend shape
    mesh_add_blend_shape(mesh, &smile_shape);
    TEST_ASSERT(mesh->blend_shape_count == 1, "blend shape added");
    TEST_ASSERT(mesh->base_vertices != NULL, "base vertices backed up");
    
    // Test blend weight setting
    mesh_set_blend_weight(mesh, 0, 0.5f);
    TEST_ASSERT(mesh->blend_shapes[0].weight == 0.5f, "blend weight set correctly");
    
    // Test blend shape evaluation
    vertex_t output_vertices[4];
    mesh_evaluate_blend_shapes(mesh, output_vertices);
    
    // Check that vertices were modified
    TEST_ASSERT(output_vertices[0].position.y > base_vertices[0].position.y, 
               "blend shape affected vertex position");
    
    // Test blend shape removal
    mesh_remove_blend_shape(mesh, 0);
    TEST_ASSERT(mesh->blend_shape_count == 0, "blend shape removed");
    
    // Cleanup
    free(smile_shape.delta_positions);
    free(smile_shape.delta_normals);
    mesh_destroy(mesh);
    
    printf("Blend shape tests completed\n");
}

static void test_skeletal_animation() {
    printf("\n=== Testing Skeletal Animation ===\n");
    
    // Create skinned vertex data
    vertex_skinned_t skinned_vertices[4] = {
        {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}, {1, 0, 0, 0}, {0, 1, 2, 3}},
        {{1, 0, 0}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}, {0.5, 0.5, 0, 0}, {0, 1, 2, 3}},
        {{1, 1, 0}, {0, 1, 0}, {1, 1}, {1, 0, 0, 1}, {0.25, 0.75, 0, 0}, {0, 1, 2, 3}},
        {{0, 1, 0}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}, {0, 1, 0, 0}, {0, 1, 2, 3}}
    };
    
    // Test bone weight validation
    bool is_valid = mesh_validate_bone_weights(skinned_vertices, 4);
    TEST_ASSERT(is_valid, "bone weights are valid");
    
    // Test invalid bone weights
    vertex_skinned_t invalid_vertices[1] = {
        {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}, {0.5, 0.5, 0.5, 0.5}, {0, 1, 2, 3}}  // Sum = 2.0
    };
    
    is_valid = mesh_validate_bone_weights(invalid_vertices, 1);
    TEST_ASSERT(!is_valid, "invalid bone weights detected");
    
    printf("Skeletal animation tests completed\n");
}

static void test_gpu_operations() {
    printf("\n=== Testing GPU Operations ===\n");
    
    // Mock device
    mock_metal_device_t mock_device = {0};
    
    // Create test mesh
    mesh_t* mesh = mesh_create("GPUTest");
    mesh_allocate_buffers(mesh, 4, 6);
    
    vertex_t vertices[4] = {
        {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}},
        {{1, 0, 0}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}},
        {{1, 1, 0}, {0, 1, 0}, {1, 1}, {1, 0, 0, 1}},
        {{0, 1, 0}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}}
    };
    mesh_set_vertices(mesh, vertices, 4, 0);
    
    u32 indices[6] = {0, 1, 2, 0, 2, 3};
    mesh_set_indices(mesh, indices, 6, 0);
    
    // Test GPU upload (will fail due to mock, but shouldn't crash)
    bool upload_result = mesh_gpu_upload(mesh, (struct metal_device*)&mock_device);
    TEST_ASSERT(!upload_result, "GPU upload fails gracefully with mock device");
    
    // Test GPU memory tracking
    u64 memory_usage = mesh_gpu_get_memory_usage(mesh);
    TEST_ASSERT(memory_usage == 0, "memory usage is 0 when not uploaded");
    
    // Test GPU state checking
    bool is_uploaded = mesh_gpu_is_uploaded(mesh);
    TEST_ASSERT(!is_uploaded, "mesh is not uploaded");
    
    // Test GPU unload (shouldn't crash even if not uploaded)
    mesh_gpu_unload(mesh);
    TEST_ASSERT(true, "GPU unload doesn't crash");
    
    mesh_destroy(mesh);
    printf("GPU operations tests completed\n");
}

static void test_material_management() {
    printf("\n=== Testing Material Management ===\n");
    
    // Create test mesh
    mesh_t* mesh = mesh_create("MaterialTest");
    mesh_allocate_buffers(mesh, 4, 6);
    
    // Test material slot validation
    bool valid_slot = mesh_validate_material_slot(mesh, 0);
    TEST_ASSERT(!valid_slot, "unassigned material slot is invalid");
    
    // Set default material
    mesh_set_default_material(mesh, 42);
    TEST_ASSERT(mesh->material_ids[0] == 42, "default material set");
    
    valid_slot = mesh_validate_material_slot(mesh, 0);
    TEST_ASSERT(valid_slot, "assigned material slot is valid");
    
    // Test material parameter application
    mesh_material_params_t params = {
        .material_id = 42,
        .metallic = 0.5f,
        .roughness = 0.3f,
        .ao = 0.2f,
        .emissive_strength = 0.1f
    };
    
    mesh_apply_material_params(mesh, 0, &params);
    TEST_ASSERT(true, "material parameters applied without crash");
    
    // Test material change tracking
    mesh_track_material_change(mesh, 0, 100, 12345);
    TEST_ASSERT(mesh->material_ids[0] == 100, "material change tracked");
    
    // Test material clearing
    mesh_clear_materials(mesh);
    TEST_ASSERT(mesh->material_count == 0, "materials cleared");
    
    mesh_destroy(mesh);
    printf("Material management tests completed\n");
}

static void test_complex_scenarios() {
    printf("\n=== Testing Complex Scenarios ===\n");
    
    // Create complex mesh with multiple submeshes
    mesh_t* complex_mesh = mesh_create("ComplexMesh");
    mesh_allocate_buffers(complex_mesh, 100, 150);
    
    // Add multiple submeshes
    for (u32 i = 0; i < 3; i++) {
        submesh_t submesh = {
            .index_start = i * 6,
            .index_count = 6,
            .vertex_start = i * 4,
            .vertex_count = 4,
            .material_index = i
        };
        mesh_add_submesh(complex_mesh, submesh);
    }
    
    TEST_ASSERT(complex_mesh->submesh_count == 3, "multiple submeshes added");
    
    // Set different materials for each submesh
    for (u32 i = 0; i < 3; i++) {
        mesh_set_material(complex_mesh, i, 10 + i);
    }
    
    TEST_ASSERT(complex_mesh->material_count == 3, "multiple materials set");
    
    // Test mesh optimization on complex mesh
    u32 original_vertex_count = complex_mesh->vertex_count;
    
    // Add some duplicate vertices to test optimization
    if (complex_mesh->vertex_capacity >= 10) {
        vertex_t dup_vertices[10];
        for (int i = 0; i < 10; i++) {
            dup_vertices[i] = (vertex_t){{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}};
        }
        mesh_set_vertices(complex_mesh, dup_vertices, 10, 0);
        
        mesh_deduplicate_vertices(complex_mesh, 0.001f);
        TEST_ASSERT(complex_mesh->vertex_count < 10, "complex mesh deduplication works");
    }
    
    // Test bounds calculation with submeshes
    mesh_calculate_submesh_bounds(complex_mesh);
    TEST_ASSERT(complex_mesh->submeshes[0].bounds.sphere_radius >= 0, "submesh bounds calculated");
    
    // Test mesh validation on complex mesh
    bool is_valid = mesh_validate(complex_mesh);
    TEST_ASSERT(is_valid, "complex mesh validation passes");
    
    // Test mesh statistics
    printf("Complex mesh stats:\n");
    mesh_print_stats(complex_mesh);
    TEST_ASSERT(true, "mesh stats printed without crash");
    
    mesh_destroy(complex_mesh);
    printf("Complex scenario tests completed\n");
}

static void test_performance_scenarios() {
    printf("\n=== Testing Performance Scenarios ===\n");
    
    // Create large mesh for performance testing
    mesh_t* large_mesh = mesh_create("LargeMesh");
    u32 vertex_count = 10000;
    u32 index_count = 15000;
    
    mesh_allocate_buffers(large_mesh, vertex_count, index_count);
    TEST_ASSERT(large_mesh->vertex_capacity == vertex_count, "large mesh allocation successful");
    
    // Fill with test data (simplified for performance)
    if (large_mesh->vertices && large_mesh->indices) {
        // Add some vertices
        for (u32 i = 0; i < MIN(100, vertex_count); i++) {
            vertex_t v = {{(float)i, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}};
            large_mesh->vertices[i] = v;
        }
        large_mesh->vertex_count = 100;
        
        // Add some indices
        for (u32 i = 0; i < MIN(150, index_count); i++) {
            large_mesh->indices[i] = i % 100;
        }
        large_mesh->index_count = 150;
    }
    
    // Test performance of bounds calculation
    mesh_calculate_bounds(large_mesh);
    TEST_ASSERT(large_mesh->bounds.sphere_radius >= 0, "large mesh bounds calculated");
    
    // Test performance of optimization
    u32 pre_opt_vertex_count = large_mesh->vertex_count;
    mesh_optimize_indices(large_mesh);
    TEST_ASSERT(large_mesh->index_count == 150, "large mesh optimization completed");
    
    // Test performance of validation
    bool is_valid = mesh_validate(large_mesh);
    TEST_ASSERT(is_valid, "large mesh validation completed");
    
    mesh_destroy(large_mesh);
    printf("Performance scenario tests completed\n");
}

static void test_edge_cases() {
    printf("\n=== Testing Edge Cases ===\n");
    
    // Test mesh with zero vertices/indices
    mesh_t* empty_mesh = mesh_create("EmptyMesh");
    mesh_allocate_buffers(empty_mesh, 0, 0);
    
    mesh_calculate_bounds(empty_mesh);
    mesh_calculate_normals(empty_mesh);
    mesh_calculate_tangents(empty_mesh);
    
    bool is_valid = mesh_validate(empty_mesh);
    TEST_ASSERT(is_valid, "empty mesh is valid");
    
    // Test mesh with single vertex
    mesh_t* single_vertex = mesh_create("SingleVertex");
    mesh_allocate_buffers(single_vertex, 1, 0);
    
    vertex_t single_v = {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}};
    mesh_set_vertices(single_vertex, &single_v, 1, 0);
    
    mesh_calculate_bounds(single_vertex);
    TEST_ASSERT(single_vertex->bounds.sphere_radius == 0, "single vertex has zero radius");
    
    // Test mesh with maximum materials
    mesh_t* max_materials = mesh_create("MaxMaterials");
    for (u32 i = 0; i < MESH_MAX_MATERIALS; i++) {
        mesh_set_material(max_materials, i, i);
    }
    TEST_ASSERT(max_materials->material_count == MESH_MAX_MATERIALS, "maximum materials set");
    
    // Test overflow material slot
    mesh_set_material(max_materials, MESH_MAX_MATERIALS, 999);
    TEST_ASSERT(max_materials->material_count == MESH_MAX_MATERIALS, "overflow material slot ignored");
    
    // Cleanup
    mesh_destroy(empty_mesh);
    mesh_destroy(single_vertex);
    mesh_destroy(max_materials);
    
    printf("Edge case tests completed\n");
}

// Main test runner
int main() {
    printf("Starting Advanced Mesh Tests\n");
    printf("===========================\n");
    
    test_blend_shapes();
    test_skeletal_animation();
    test_gpu_operations();
    test_material_management();
    test_complex_scenarios();
    test_performance_scenarios();
    test_edge_cases();
    
    printf("\n=== Advanced Test Results ===\n");
    printf("Tests run: %d\n", g_tests_run);
    printf("Tests passed: %d\n", g_tests_passed);
    printf("Tests failed: %d\n", g_tests_failed);
    printf("Success rate: %.1f%%\n", (float)g_tests_passed / g_tests_run * 100.0f);
    
    if (g_tests_failed == 0) {
        printf("\n🎉 All advanced tests passed! Mesh system is robust.\n");
        return 0;
    } else {
        printf("\n❌ Some advanced tests failed. Please review the mesh system.\n");
        return 1;
    }
}
