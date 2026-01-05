/**
 * Unit Tests for Dynamic AABB Tree
 * 
 * Validates tree operations, SAH cost function, and query correctness.
 */

#include "../src/engine/physics/broadphase/aabb_tree.h"
#include "../src/engine/core/simd/simd_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define EPSILON 1e-5f

// ========================================
// AABB Utility Tests
// ========================================

void test_aabb_overlap() {
    printf("Testing AABB overlap... ");
    
    AABB a = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(1.0f, 1.0f, 1.0f)
    };
    
    AABB b = {
        .min = V4F_VECTOR(0.5f, 0.5f, 0.5f),
        .max = V4F_VECTOR(1.5f, 1.5f, 1.5f)
    };
    
    assert(aabb_overlap(&a, &b));  // Should overlap
    
    // Non-overlapping case
    AABB c = {
        .min = V4F_VECTOR(2.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(3.0f, 1.0f, 1.0f)
    };
    
    assert(!aabb_overlap(&a, &c));  // Should not overlap
    
    printf("PASSED\n");
}

void test_aabb_union() {
    printf("Testing AABB union... ");
    
    AABB a = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(1.0f, 1.0f, 1.0f)
    };
    
    AABB b = {
        .min = V4F_VECTOR(0.5f, 0.5f, 0.5f),
        .max = V4F_VECTOR(2.0f, 2.0f, 2.0f)
    };
    
    AABB result;
    aabb_union(&result, &a, &b);
    
    // Union should be (0, 0, 0) to (2, 2, 2)
    assert(fabsf(result.min.x - 0.0f) < EPSILON);
    assert(fabsf(result.max.x - 2.0f) < EPSILON);
    assert(fabsf(result.max.y - 2.0f) < EPSILON);
    
    printf("PASSED\n");
}

void test_aabb_surface_area() {
    printf("Testing AABB surface area... ");
    
    // Unit cube: SA = 2 * (1*1 + 1*1 + 1*1) = 6
    AABB cube = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(1.0f, 1.0f, 1.0f)
    };
    
    float area = aabb_surface_area(&cube);
    assert(fabsf(area - 6.0f) < EPSILON);
    
    printf("PASSED\n");
}

void test_aabb_fatten() {
    printf("Testing AABB fattening... ");
    
    AABB tight = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(1.0f, 1.0f, 1.0f)
    };
    
    AABB fat;
    float margin = 0.1f;
    aabb_fatten(&fat, &tight, margin, NULL);
    
    // Should be expanded by 0.1 on all sides
    assert(fabsf(fat.min.x - (-0.1f)) < EPSILON);
    assert(fabsf(fat.max.x - 1.1f) < EPSILON);
    
    // Test with displacement
    v4f displacement = V4F_VECTOR(1.0f, 0.0f, 0.0f);
    aabb_fatten(&fat, &tight, margin, &displacement);
    
    // Should extend max.x by 1.0 (displacement)
    assert(fabsf(fat.max.x - 2.1f) < EPSILON);  // 1.0 + 0.1 + 1.0
    
    printf("PASSED\n");
}

// ========================================
// Callback Functions
// ========================================

static bool query_callback(int32_t proxy_id, uint32_t body_id, void *user_data) {
    int *count = (int*)user_data;
    (*count)++;
    return true;
}

static bool count_callback(int32_t proxy_id, uint32_t body_id, void *user_data) {
    (*(int*)user_data)++;
    return true;
}

// ========================================
// Tree Operation Tests
// ========================================

void test_tree_create_destroy() {
    printf("Testing tree create/destroy... ");
    
    AABBTree *tree = aabb_tree_create(100, 0.1f);
    assert(tree != NULL);
    assert(tree->node_capacity == 100);
    assert(tree->node_count == 0);
    assert(tree->root == AABB_NULL_NODE);
    
    aabb_tree_destroy(tree);
    
    printf("PASSED\n");
}

void test_tree_insert_single() {
    printf("Testing single insertion... ");
    
    AABBTree *tree = aabb_tree_create(100, 0.1f);
    
    AABB aabb = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(1.0f, 1.0f, 1.0f)
    };
    
    int32_t proxy_id = aabb_tree_insert(tree, &aabb, 42);
    assert(proxy_id != AABB_NULL_NODE);
    assert(tree->root == proxy_id);
    assert(tree->node_count == 1);
    assert(tree->nodes[proxy_id].is_leaf);
    assert(tree->nodes[proxy_id].body_id == 42);
    
    aabb_tree_destroy(tree);
    
    printf("PASSED\n");
}

void test_tree_insert_multiple() {
    printf("Testing multiple insertions... ");
    
    AABBTree *tree = aabb_tree_create(100, 0.1f);
    
    // Insert 5 AABBs
    for (uint32_t i = 0; i < 5; i++) {
        AABB aabb = {
            .min = V4F_VECTOR((float)i, 0.0f, 0.0f),
            .max = V4F_VECTOR((float)i + 1.0f, 1.0f, 1.0f)
        };
        
        int32_t proxy_id = aabb_tree_insert(tree, &aabb, i);
        assert(proxy_id != AABB_NULL_NODE);
    }
    
    // Should have 5 leaves + 4 internal nodes = 9 total
    assert(tree->node_count == 9);
    
    aabb_tree_destroy(tree);
    
    printf("PASSED\n");
}

void test_tree_remove() {
    printf("Testing removal... ");
    
    AABBTree *tree = aabb_tree_create(100, 0.1f);
    
    AABB aabb1 = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(1.0f, 1.0f, 1.0f)
    };
    
    AABB aabb2 = {
        .min = V4F_VECTOR(2.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(3.0f, 1.0f, 1.0f)
    };
    
    int32_t proxy1 = aabb_tree_insert(tree, &aabb1, 1);
    int32_t proxy2 = aabb_tree_insert(tree, &aabb2, 2);
    
    assert(tree->node_count == 3);  // 2 leaves + 1 branch
    
    // Remove first
    aabb_tree_remove(tree, proxy1);
    assert(tree->node_count == 1);  // Only proxy2 left
    assert(tree->root == proxy2);
    
    aabb_tree_destroy(tree);
    
    printf("PASSED\n");
}

void test_tree_query() {
    printf("Testing tree query... ");
    
    AABBTree *tree = aabb_tree_create(100, 0.1f);
    
    // Insert several AABBs
    AABB aabbs[5];
    for (int i = 0; i < 5; i++) {
        aabbs[i].min = V4F_VECTOR((float)i * 2.0f, 0.0f, 0.0f);
        aabbs[i].max = V4F_VECTOR((float)i * 2.0f + 1.0f, 1.0f, 1.0f);
        aabb_tree_insert(tree, &aabbs[i], i);
    }
    
    // Query region that should overlap with AABBs 0, 1, 2
    AABB query_aabb = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(5.0f, 1.0f, 1.0f)
    };
    
    int hit_count = 0;
    aabb_tree_query(tree, &query_aabb, query_callback, &hit_count);
    
    // Should hit at least 3 AABBs (0, 1, 2)
    assert(hit_count >= 3);
    
    aabb_tree_destroy(tree);
    
    printf("PASSED\n");
}

// ========================================
// Performance Test
// ========================================

void test_tree_performance() {
    printf("\nPerformance test: 1000 bodies...\n");
    
    AABBTree *tree = aabb_tree_create(2000, 0.1f);
    
    // Insert 1000 bodies
    int32_t *proxies = (int32_t*)malloc(sizeof(int32_t) * 1000);
    
    for (int i = 0; i < 1000; i++) {
        float x = (float)(i % 10);
        float y = (float)(i / 10);  // Integer division intentional for grid layout
        
        AABB aabb = {
            .min = V4F_VECTOR(x, y, 0.0f),
            .max = V4F_VECTOR(x + 0.5f, y + 0.5f, 0.5f)
        };
        
        proxies[i] = aabb_tree_insert(tree, &aabb, i);
    }
    
    printf("  Inserted 1000 bodies\n");
    printf("  Node count: %u\n", tree->node_count);
    printf("  Tree height: %d\n", tree->nodes[tree->root].height);
    
    // Expected height for balanced tree: log2(1000) ≈ 10
    // Allow some imbalance: height should be < 20
    assert(tree->nodes[tree->root].height < 20);
    
    // Query test
    AABB query = {
        .min = V4F_VECTOR(0.0f, 0.0f, 0.0f),
        .max = V4F_VECTOR(5.0f, 5.0f, 1.0f)
    };
    
    int hits = 0;
    aabb_tree_query(tree, &query, count_callback, &hits);
    printf("  Query hits: %d\n", hits);
    
    free(proxies);
    aabb_tree_destroy(tree);
    
    printf("  Performance test PASSED\n");
}

// ========================================
// Main
// ========================================

int main() {
    printf("=== AABB Tree Unit Tests ===\n\n");
    
    printf("AABB Utility Tests:\n");
    test_aabb_overlap();
    test_aabb_union();
    test_aabb_surface_area();
    test_aabb_fatten();
    
    printf("\nTree Operation Tests:\n");
    test_tree_create_destroy();
    test_tree_insert_single();
    test_tree_insert_multiple();
    test_tree_remove();
    test_tree_query();
    
    test_tree_performance();
    
    printf("\n=== ALL TESTS PASSED ===\n");
    return 0;
}
