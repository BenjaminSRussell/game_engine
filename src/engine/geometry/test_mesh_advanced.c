#include "geometry/mesh.h"
#include "geometry/mesh_optimize.h"
#include "geometry/mesh_deform.h"
#include "geometry/mesh_gpu.h"
#include <core/logger.h>
#include <core/asserts.h>
#include <math/vec3.h>
#include <math/vec2.h>
#include <stdio.h>

// ----------------------------------------------------------------------------
// Test: Mesh Creation & Destruction
// ----------------------------------------------------------------------------

static void test_mesh_lifecycle() {
    LOG_INFO("=== Test: Mesh Lifecycle ===");
    
    mesh_t* mesh = mesh_create("TestMesh");
    ASSERT_NOT_NULL(mesh);
    ASSERT_EQ(mesh->ref_count, 1);
    
    mesh_destroy(mesh);
    
    LOG_INFO("✓ Mesh lifecycle test passed");
}

// ----------------------------------------------------------------------------
// Test: Bounds Calculations
// ----------------------------------------------------------------------------

static void test_bounds_calculations() {
    LOG_INFO("=== Test: Bounds Calculations ===");
    
    mesh_t* mesh = mesh_create("BoundsTest");
    mesh_allocate_buffers(mesh, 8, 36);
    
    // Create a unit cube [-1, 1]
    vertex_t vertices[8] = {
        {.position = {-1, -1, -1}},
        {.position = { 1, -1, -1}},
        {.position = { 1,  1, -1}},
        {.position = {-1,  1, -1}},
        {.position = {-1, -1,  1}},
        {.position = { 1, -1,  1}},
        {.position = { 1,  1,  1}},
        {.position = {-1,  1,  1}},
    };
    
    mesh_set_vertices(mesh, vertices, 8, 0);
    
    // Test AABB
    mesh_calculate_bounds(mesh);
    ASSERT_FLOAT_EQ(mesh->bounds.min.x, -1.0f, 0.001f);
    ASSERT_FLOAT_EQ(mesh->bounds.max.x,  1.0f, 0.001f);
    
    // Test centroid
    Vec3 centroid = mesh_calculate_centroid(mesh);
    ASSERT_FLOAT_EQ(centroid.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(centroid.y, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(centroid.z, 0.0f, 0.001f);
    
    // Test OBB
    mesh_calculate_obb(mesh);
    ASSERT_FLOAT_EQ(mesh->obb.center.x, 0.0f, 0.001f);
    ASSERT_FLOAT_EQ(mesh->obb.extents.x, 1.0f, 0.001f);
    
    mesh_destroy(mesh);
    LOG_INFO("✓ Bounds calculations test passed");
}

// ----------------------------------------------------------------------------
// Test: Mesh Cloning
// ----------------------------------------------------------------------------

static void test_mesh_cloning() {
    LOG_INFO("=== Test: Mesh Cloning ===");
    
    mesh_t* original = mesh_create("Original");
    mesh_allocate_buffers(original, 3, 3);
    
    vertex_t verts[3] = {
        {.position = {0, 0, 0}},
        {.position = {1, 0, 0}},
        {.position = {0, 1, 0}},
    };
    u32 indices[3] = {0, 1, 2};
    
    mesh_set_vertices(original, verts, 3, 0);
    mesh_set_indices(original, indices, 3, 0);
    
    mesh_t* clone = mesh_clone(original);
    ASSERT_NOT_NULL(clone);
    ASSERT_EQ(clone->vertex_count, original->vertex_count);
    ASSERT_EQ(clone->index_count, original->index_count);
    
    // Ensure deep copy
    ASSERT_NE(clone->vertices, original->vertices);
    
    mesh_destroy(original);
    mesh_destroy(clone);
    
    LOG_INFO("✓ Mesh cloning test passed");
}

// ----------------------------------------------------------------------------
// Test: Validation
// ----------------------------------------------------------------------------

static void test_validation() {
    LOG_INFO("=== Test: Mesh Validation ===");
    
    // Valid mesh
    mesh_t* valid_mesh = mesh_create("Valid");
    mesh_allocate_buffers(valid_mesh, 3, 3);
    
    vertex_t verts[3] = {
        {.position = {0, 0, 0}, .normal = {0, 0, 1}},
        {.position = {1, 0, 0}, .normal = {0, 0, 1}},
        {.position = {0, 1, 0}, .normal = {0, 0, 1}},
    };
    u32 indices[3] = {0, 1, 2};
    
    mesh_set_vertices(valid_mesh, verts, 3, 0);
    mesh_set_indices(valid_mesh, indices, 3, 0);
    
    char error_msg[256];
    mesh_validation_error_e result = mesh_validate_detailed(valid_mesh, error_msg, sizeof(error_msg));
    ASSERT_EQ(result, MESH_VALIDATION_OK);
    
    // Invalid mesh (out of range index)
    mesh_t* invalid_mesh = mesh_create("Invalid");
    mesh_allocate_buffers(invalid_mesh, 2, 3);
    mesh_set_vertices(invalid_mesh, verts, 2, 0);
    
    u32 bad_indices[3] = {0, 1, 5}; // Index 5 is out of range
    mesh_set_indices(invalid_mesh, bad_indices, 3, 0);
    
    result = mesh_validate_detailed(invalid_mesh, error_msg, sizeof(error_msg));
    ASSERT_EQ(result, MESH_VALIDATION_INVALID_INDEX);
    LOG_INFO("  Expected validation error: %s", error_msg);
    
    mesh_destroy(valid_mesh);
    mesh_destroy(invalid_mesh);
    
    LOG_INFO("✓ Mesh validation test passed");
}

// ----------------------------------------------------------------------------
// Test: Vertex Deduplication
// ----------------------------------------------------------------------------

static void test_vertex_deduplication() {
    LOG_INFO("=== Test: Vertex Deduplication ===");
    
    mesh_t* mesh = mesh_create("DedupTest");
    mesh_allocate_buffers(mesh, 6, 6);
    
    // Create 6 vertices, but first 3 are duplicates of last 3
    vertex_t verts[6] = {
        {.position = {0, 0, 0}, .normal = {0, 0, 1}, .uv = {0, 0}},
        {.position = {1, 0, 0}, .normal = {0, 0, 1}, .uv = {1, 0}},
        {.position = {0, 1, 0}, .normal = {0, 0, 1}, .uv = {0, 1}},
        {.position = {0, 0, 0}, .normal = {0, 0, 1}, .uv = {0, 0}}, // dup of 0
        {.position = {1, 0, 0}, .normal = {0, 0, 1}, .uv = {1, 0}}, // dup of 1
        {.position = {0, 1, 0}, .normal = {0, 0, 1}, .uv = {0, 1}}, // dup of 2
    };
    u32 indices[6] = {0, 1, 2, 3, 4, 5};
    
    mesh_set_vertices(mesh, verts, 6, 0);
    mesh_set_indices(mesh, indices, 6, 0);
    
    ASSERT_EQ(mesh->vertex_count, 6);
    
    mesh_deduplicate_vertices(mesh, 0.001f);
    
    ASSERT_EQ(mesh->vertex_count, 3);
    LOG_INFO("  Reduced from 6 to 3 vertices");
    
    mesh_destroy(mesh);
    LOG_INFO("✓ Vertex deduplication test passed");
}

// ----------------------------------------------------------------------------
// Test: Blend Shapes
// ----------------------------------------------------------------------------

static void test_blend_shapes() {
    LOG_INFO("=== Test: Blend Shapes ===");
    
    mesh_t* mesh = mesh_create("BlendTest");
    mesh_allocate_buffers(mesh, 3, 3);
    
    vertex_t base_verts[3] = {
        {.position = {0, 0, 0}, .normal = {0, 0, 1}},
        {.position = {1, 0, 0}, .normal = {0, 0, 1}},
        {.position = {0, 1, 0}, .normal = {0, 0, 1}},
    };
    
    mesh_set_vertices(mesh, base_verts, 3, 0);
    
    // Create blend shape
    blend_shape_t smile;
    strncpy(smile.name, "Smile", sizeof(smile.name));
    smile.vertex_count = 3;
    smile.weight = 1.0f;
    
    Vec3 position_deltas[3] = {
        {0, 0, 0},
        {0, 0.5f, 0},  // Move vertex 1 up
        {0, 0, 0},
    };
    smile.position_deltas = position_deltas;
    smile.normal_deltas = NULL;
    
    mesh_add_blend_shape(mesh, &smile);
    
    ASSERT_NOT_NULL(mesh->morph_targets);
    ASSERT_EQ(mesh->morph_targets->shape_count, 1);
    
    // Evaluate blend shapes
    vertex_t output[3];
    mesh_evaluate_blend_shapes(mesh, output);
    
    // Vertex 1 should have moved up by 0.5
    ASSERT_FLOAT_EQ(output[1].position.y, 0.5f, 0.001f);
    
    mesh_destroy(mesh);
    LOG_INFO("✓ Blend shapes test passed");
}

// ----------------------------------------------------------------------------
// Test: Statistics
// ----------------------------------------------------------------------------

static void test_statistics() {
    LOG_INFO("=== Test: Statistics ===");
    
    mesh_t* mesh = mesh_create("StatsTest");
    mesh_allocate_buffers(mesh, 3, 3);
    
    vertex_t verts[3] = {
        {.position = {0, 0, 0}},
        {.position = {1, 0, 0}},
        {.position = {0, 1, 0}},
    };
    u32 indices[3] = {0, 1, 2};
    
    mesh_set_vertices(mesh, verts, 3, 0);
    mesh_set_indices(mesh, indices, 3, 0);
    
    mesh_calculate_stats(mesh);
    
    ASSERT_EQ(mesh->stats.vertex_count, 3);
    ASSERT_EQ(mesh->stats.triangle_count, 1);
    ASSERT_GT(mesh->stats.surface_area, 0.0f);
    
    u32 tri_count = mesh_get_triangle_count(mesh);
    ASSERT_EQ(tri_count, 1);
    
    u64 mem = mesh_get_memory_usage(mesh);
    ASSERT_GT(mem, 0);
    
    mesh_print_stats(mesh);
    
    mesh_destroy(mesh);
    LOG_INFO("✓ Statistics test passed");
}

// ----------------------------------------------------------------------------
// Main Test Runner
// ----------------------------------------------------------------------------

void test_mesh_advanced() {
    LOG_INFO("========================================");
    LOG_INFO("  Advanced Mesh System Tests");
    LOG_INFO("========================================");
    
    test_mesh_lifecycle();
    test_bounds_calculations();
    test_mesh_cloning();
    test_validation();
    test_vertex_deduplication();
    test_blend_shapes();
    test_statistics();
    
    LOG_INFO("========================================");
    LOG_INFO("  All Tests Passed! ✓");
    LOG_INFO("========================================");
}
