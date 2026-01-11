#include "geometry/mesh.h"
#include "geometry/mesh_primitives.h"
#include "geometry/mesh_utils.h"
#include "geometry/mesh_optimize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Simple test framework
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

static void test_mesh_creation() {
    printf("\n=== Testing Mesh Creation ===\n");
    
    // Test basic mesh creation
    mesh_t* mesh = mesh_create("TestMesh");
    TEST_ASSERT(mesh != NULL, "mesh_create returns non-NULL");
    TEST_ASSERT(strcmp(mesh->name, "TestMesh") == 0, "mesh name is set correctly");
    TEST_ASSERT(mesh->vertex_count == 0, "new mesh has 0 vertices");
    TEST_ASSERT(mesh->index_count == 0, "new mesh has 0 indices");
    TEST_ASSERT(mesh->ref_count == 1, "new mesh has ref count 1");
    
    // Test buffer allocation
    mesh_allocate_buffers(mesh, 100, 150);
    TEST_ASSERT(mesh->vertex_capacity == 100, "vertex capacity set correctly");
    TEST_ASSERT(mesh->index_capacity == 150, "index capacity set correctly");
    TEST_ASSERT(mesh->vertices != NULL, "vertex buffer allocated");
    TEST_ASSERT(mesh->indices != NULL, "index buffer allocated");
    
    mesh_destroy(mesh);
    printf("Mesh creation tests completed\n");
}

static void test_primitive_creation() {
    printf("\n=== Testing Primitive Creation ===\n");
    
    // Test cube creation
    mesh_t* cube = mesh_create_cube(2.0f);
    TEST_ASSERT(cube != NULL, "cube creation returns non-NULL");
    TEST_ASSERT(cube->vertex_count == 24, "cube has 24 vertices");
    TEST_ASSERT(cube->index_count == 36, "cube has 36 indices");
    TEST_ASSERT(cube->submesh_count == 1, "cube has 1 submesh");
    
    // Test cube bounds
    mesh_calculate_bounds(cube);
    TEST_ASSERT_FLOAT_EQUAL(cube->bounds.min.x, -1.0f, 0.001f, "cube min X is -1");
    TEST_ASSERT_FLOAT_EQUAL(cube->bounds.max.x, 1.0f, 0.001f, "cube max X is 1");
    TEST_ASSERT_FLOAT_EQUAL(cube->bounds.min.y, -1.0f, 0.001f, "cube min Y is -1");
    TEST_ASSERT_FLOAT_EQUAL(cube->bounds.max.y, 1.0f, 0.001f, "cube max Y is 1");
    TEST_ASSERT_FLOAT_EQUAL(cube->bounds.min.z, -1.0f, 0.001f, "cube min Z is -1");
    TEST_ASSERT_FLOAT_EQUAL(cube->bounds.max.z, 1.0f, 0.001f, "cube max Z is 1");
    
    // Test sphere creation
    mesh_t* sphere = mesh_create_sphere(1.0f, 8);
    TEST_ASSERT(sphere != NULL, "sphere creation returns non-NULL");
    TEST_ASSERT(sphere->vertex_count > 0, "sphere has vertices");
    TEST_ASSERT(sphere->index_count > 0, "sphere has indices");
    
    // Test sphere bounds
    mesh_calculate_bounds(sphere);
    TEST_ASSERT_FLOAT_EQUAL(sphere->bounds.sphere_radius, 1.0f, 0.1f, "sphere radius is approximately 1");
    
    // Test plane creation
    mesh_t* plane = mesh_create_plane(2.0f, 2.0f, 2);
    TEST_ASSERT(plane != NULL, "plane creation returns non-NULL");
    TEST_ASSERT(plane->vertex_count == 9, "2x2 plane has 9 vertices");
    TEST_ASSERT(plane->index_count == 12, "2x2 plane has 12 indices");
    
    mesh_destroy(cube);
    mesh_destroy(sphere);
    mesh_destroy(plane);
    printf("Primitive creation tests completed\n");
}

static void test_mesh_operations() {
    printf("\n=== Testing Mesh Operations ===\n");
    
    // Create a simple test mesh
    mesh_t* mesh = mesh_create("TestOps");
    mesh_allocate_buffers(mesh, 4, 6);
    
    // Add test vertices
    vertex_t vertices[4] = {
        {{-1, -1, 0}, {0, 0, 1}, {0, 0}, {1, 0, 0, 1}},
        {{ 1, -1, 0}, {0, 0, 1}, {1, 0}, {1, 0, 0, 1}},
        {{ 1,  1, 0}, {0, 0, 1}, {1, 1}, {1, 0, 0, 1}},
        {{-1,  1, 0}, {0, 0, 1}, {0, 1}, {1, 0, 0, 1}}
    };
    mesh_set_vertices(mesh, vertices, 4, 0);
    TEST_ASSERT(mesh->vertex_count == 4, "vertices set correctly");
    
    // Add test indices
    u32 indices[6] = {0, 1, 2, 0, 2, 3};
    mesh_set_indices(mesh, indices, 6, 0);
    TEST_ASSERT(mesh->index_count == 6, "indices set correctly");
    
    // Test bounds calculation
    mesh_calculate_bounds(mesh);
    TEST_ASSERT_FLOAT_EQUAL(mesh->bounds.min.x, -1.0f, 0.001f, "bounds min X correct");
    TEST_ASSERT_FLOAT_EQUAL(mesh->bounds.max.x, 1.0f, 0.001f, "bounds max X correct");
    TEST_ASSERT_FLOAT_EQUAL(mesh->bounds.min.y, -1.0f, 0.001f, "bounds min Y correct");
    TEST_ASSERT_FLOAT_EQUAL(mesh->bounds.max.y, 1.0f, 0.001f, "bounds max Y correct");
    
    // Test normal calculation
    mesh_calculate_normals(mesh);
    TEST_ASSERT_FLOAT_EQUAL(mesh->vertices[0].normal.z, 1.0f, 0.001f, "normal calculation correct");
    
    // Test tangent calculation
    mesh_calculate_tangents(mesh);
    TEST_ASSERT_FLOAT_EQUAL(mesh->vertices[0].tangent.x, 1.0f, 0.001f, "tangent calculation correct");
    
    // Test submesh operations
    submesh_t submesh = {0, 6, 0, 4, 0};
    mesh_add_submesh(mesh, submesh);
    TEST_ASSERT(mesh->submesh_count == 1, "submesh added correctly");
    
    // Test material operations
    mesh_set_material(mesh, 0, 42);
    TEST_ASSERT(mesh->material_ids[0] == 42, "material set correctly");
    TEST_ASSERT(mesh->material_count == 1, "material count updated");
    
    // Test validation
    bool is_valid = mesh_validate(mesh);
    TEST_ASSERT(is_valid, "mesh validation passes");
    
    mesh_destroy(mesh);
    printf("Mesh operations tests completed\n");
}

static void test_mesh_optimization() {
    printf("\n=== Testing Mesh Optimization ===\n");
    
    // Create a mesh with duplicate vertices for testing deduplication
    mesh_t* mesh = mesh_create("DedupeTest");
    mesh_allocate_buffers(mesh, 6, 6);
    
    // Add vertices with some duplicates
    vertex_t vertices[6] = {
        {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}},  // Unique
        {{1, 0, 0}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}},  // Unique
        {{0, 1, 0}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}},  // Unique
        {{0, 0, 0}, {0, 1, 0}, {0, 0}, {1, 0, 0, 1}},  // Duplicate of 0
        {{1, 0, 0}, {0, 1, 0}, {1, 0}, {1, 0, 0, 1}},  // Duplicate of 1
        {{0, 1, 0}, {0, 1, 0}, {0, 1}, {1, 0, 0, 1}}   // Duplicate of 2
    };
    mesh_set_vertices(mesh, vertices, 6, 0);
    
    u32 indices[6] = {0, 1, 2, 3, 4, 5};
    mesh_set_indices(mesh, indices, 6, 0);
    
    u32 original_vertex_count = mesh->vertex_count;
    
    // Test vertex deduplication
    mesh_deduplicate_vertices(mesh, 0.001f);
    TEST_ASSERT(mesh->vertex_count < original_vertex_count, "vertex deduplication reduced vertex count");
    TEST_ASSERT(mesh->vertex_count == 3, "deduplicated to 3 unique vertices");
    
    // Test UV quantization
    mesh_quantize_uvs(mesh);
    TEST_ASSERT(mesh->vertex_count == 3, "vertex count unchanged after UV quantization");
    
    // Test index optimization
    mesh_optimize_indices(mesh);
    TEST_ASSERT(mesh->index_count == 6, "index count unchanged after optimization");
    
    mesh_destroy(mesh);
    printf("Mesh optimization tests completed\n");
}

static void test_mesh_utilities() {
    printf("\n=== Testing Mesh Utilities ===\n");
    
    // Create test mesh
    mesh_t* mesh = mesh_create("UtilsTest");
    mesh_allocate_buffers(mesh, 4, 6);
    
    vertex_t vertices[4] = {
        {{-1, -1, 0}, {0, 0, 1}, {0, 0}, {1, 0, 0, 1}},
        {{ 1, -1, 0}, {0, 0, 1}, {1, 0}, {1, 0, 0, 1}},
        {{ 1,  1, 0}, {0, 0, 1}, {1, 1}, {1, 0, 0, 1}},
        {{-1,  1, 0}, {0, 0, 1}, {0, 1}, {1, 0, 0, 1}}
    };
    mesh_set_vertices(mesh, vertices, 4, 0);
    
    u32 indices[6] = {0, 1, 2, 0, 2, 3};
    mesh_set_indices(mesh, indices, 6, 0);
    
    // Test submesh bounds calculation
    submesh_t submesh = {0, 6, 0, 4, 0};
    mesh_add_submesh(mesh, submesh);
    mesh_calculate_submesh_bounds(mesh);
    
    TEST_ASSERT(mesh->submeshes[0].bounds.min.x <= -0.9f, "submesh bounds min X correct");
    TEST_ASSERT(mesh->submeshes[0].bounds.max.x >= 0.9f, "submesh bounds max X correct");
    
    // Test convex hull calculation
    Vec3* hull_vertices = NULL;
    u32 hull_count = 0;
    mesh_calculate_convex_hull(mesh, &hull_vertices, &hull_count);
    
    TEST_ASSERT(hull_vertices != NULL, "convex hull vertices allocated");
    TEST_ASSERT(hull_count > 0, "convex hull has vertices");
    TEST_ASSERT(hull_count <= mesh->vertex_count, "convex hull doesn't exceed original vertex count");
    
    if (hull_vertices) {
        free(hull_vertices);
    }
    
    // Test transform bounds update
    float transform[16] = {
        2, 0, 0, 0,  // Scale by 2
        0, 2, 0, 0,
        0, 0, 2, 0,
        1, 0, 0, 1   // Translate by (1, 0, 0)
    };
    
    mesh_calculate_bounds(mesh);  // Calculate original bounds first
    mesh_bounds_t original_bounds = mesh->bounds;
    mesh_update_bounds_transform_matrix(mesh, transform);
    
    TEST_ASSERT(mesh->bounds.min.x > original_bounds.min.x, "transformed bounds moved");
    TEST_ASSERT(mesh->bounds.max.x > original_bounds.max.x, "transformed bounds scaled");
    
    mesh_destroy(mesh);
    printf("Mesh utilities tests completed\n");
}

static void test_error_handling() {
    printf("\n=== Testing Error Handling ===\n");
    
    // Test NULL pointer handling
    mesh_destroy(NULL);  // Should not crash
    TEST_ASSERT(true, "mesh_destroy(NULL) doesn't crash");
    
    mesh_calculate_bounds(NULL);
    TEST_ASSERT(true, "mesh_calculate_bounds(NULL) doesn't crash");
    
    mesh_validate(NULL);
    TEST_ASSERT(!mesh_validate(NULL), "mesh_validate(NULL) returns false");
    
    // Test invalid operations
    mesh_t* mesh = mesh_create("ErrorTest");
    
    // Test operations on empty mesh
    mesh_calculate_bounds(mesh);
    TEST_ASSERT(true, "bounds calculation on empty mesh doesn't crash");
    
    mesh_calculate_normals(mesh);
    TEST_ASSERT(true, "normal calculation on empty mesh doesn't crash");
    
    // Test invalid buffer operations
    mesh_set_vertices(mesh, NULL, 0, 0);
    TEST_ASSERT(true, "setting NULL vertices doesn't crash");
    
    mesh_set_indices(mesh, NULL, 0, 0);
    TEST_ASSERT(true, "setting NULL indices doesn't crash");
    
    // Test invalid material slot
    mesh_set_material(mesh, 100, 42);  // Invalid slot
    TEST_ASSERT(true, "invalid material slot doesn't crash");
    
    mesh_destroy(mesh);
    printf("Error handling tests completed\n");
}

// Main test runner
int main() {
    printf("Starting Geometry System Tests\n");
    printf("==============================\n");
    
    test_mesh_creation();
    test_primitive_creation();
    test_mesh_operations();
    test_mesh_optimization();
    test_mesh_utilities();
    test_error_handling();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", g_tests_run);
    printf("Tests passed: %d\n", g_tests_passed);
    printf("Tests failed: %d\n", g_tests_failed);
    printf("Success rate: %.1f%%\n", (float)g_tests_passed / g_tests_run * 100.0f);
    
    if (g_tests_failed == 0) {
        printf("\n🎉 All tests passed! Geometry system is working correctly.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Please review the geometry system.\n");
        return 1;
    }
}
