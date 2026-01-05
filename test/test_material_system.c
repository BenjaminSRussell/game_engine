/**
 * Material System Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(expr) do { \
    tests_run++; \
    if (expr) { \
        tests_passed++; \
        printf("  ✓ %s\n", #expr); \
    } else { \
        printf("  ✗ FAILED: %s (line %d)\n", #expr, __LINE__); \
    } \
} while(0)

// Mock Structures
typedef struct {
    char name[64];
    float values[16];
    int texture_ids[8];
} Material;

typedef struct {
    char vert_path[128];
    char frag_path[128];
    int program_id;
} Shader;

typedef struct {
    Shader *shader;
    Material *material;
} MaterialInstance;

// Mock API
Material* material_create(const char* name) {
    Material* mat = calloc(1, sizeof(Material));
    strncpy(mat->name, name, 63);
    return mat;
}

void material_destroy(Material* mat) {
    free(mat);
}

void material_set_float(Material* mat, int index, float value) {
    if (index >= 0 && index < 16) mat->values[index] = value;
}

float material_get_float(Material* mat, int index) {
    if (index >= 0 && index < 16) return mat->values[index];
    return 0.0f;
}

void material_set_texture(Material* mat, int slot, int texture_id) {
    if (slot >= 0 && slot < 8) mat->texture_ids[slot] = texture_id;
}

int material_get_texture(Material* mat, int slot) {
    if (slot >= 0 && slot < 8) return mat->texture_ids[slot];
    return -1;
}

// Tests
void test_material_creation() {
    printf("Testing material creation...\n");
    Material* mat = material_create("StoneMaterial");
    TEST_ASSERT(mat != NULL);
    TEST_ASSERT(strcmp(mat->name, "StoneMaterial") == 0);
    material_destroy(mat);
}

void test_material_properties() {
    printf("Testing material properties...\n");
    Material* mat = material_create("PropsTest");
    
    material_set_float(mat, 0, 1.5f);
    material_set_float(mat, 5, 3.14f);
    
    TEST_ASSERT(material_get_float(mat, 0) == 1.5f);
    TEST_ASSERT(material_get_float(mat, 5) == 3.14f);
    TEST_ASSERT(material_get_float(mat, 2) == 0.0f); // Default
    
    material_destroy(mat);
}

void test_material_textures() {
    printf("Testing material textures...\n");
    Material* mat = material_create("TextureTest");
    
    material_set_texture(mat, 0, 101);
    material_set_texture(mat, 7, 202);
    
    TEST_ASSERT(material_get_texture(mat, 0) == 101);
    TEST_ASSERT(material_get_texture(mat, 7) == 202);
    TEST_ASSERT(material_get_texture(mat, 1) == 0);
    
    material_destroy(mat);
}

int main() {
    printf("=== Material System Tests ===\n\n");
    
    test_material_creation();
    test_material_properties();
    test_material_textures();
    
    printf("\n=== Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    return (tests_run == tests_passed) ? 0 : 1;
}
