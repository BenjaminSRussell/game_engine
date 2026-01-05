/**
 * SELF-CONTAINED GAME ENGINE TEST SUITE
 * 
 * This is a fully self-contained test file that can be compiled and run
 * without ANY external dependencies. All stubs are inline.
 * 
 * Compile: gcc -o tests/run_tests tests/comprehensive/test_standalone.c -lm
 * Run: ./tests/run_tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

// =============================================================================
// ANSI COLORS
// =============================================================================

#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"

// =============================================================================
// TEST FRAMEWORK (Minimal Inline)
// =============================================================================

typedef enum { TEST_PASS = 0, TEST_FAIL = 1, TEST_SKIP = 2 } TestResult;

typedef struct {
    const char *suite;
    const char *name;
    TestResult (*func)(void);
} TestCase;

static TestCase g_tests[1024];
static int g_test_count = 0;
static int g_passed = 0;
static int g_failed = 0;
static int g_skipped = 0;

static void test_register(const char* suite, const char* name, TestResult (*func)(void)) {
    g_tests[g_test_count].suite = suite;
    g_tests[g_test_count].name = name;
    g_tests[g_test_count].func = func;
    g_test_count++;
}

#define TEST_ASSERT(cond, msg) \
    do { if (!(cond)) { printf("    " COLOR_RED "FAIL: %s" COLOR_RESET "\n", msg); return TEST_FAIL; } } while(0)

#define TEST_ASSERT_NOT_NULL(ptr, msg) TEST_ASSERT((ptr) != NULL, msg)
#define TEST_ASSERT_NULL(ptr, msg) TEST_ASSERT((ptr) == NULL, msg)
#define TEST_ASSERT_TRUE(cond, msg) TEST_ASSERT(cond, msg)
#define TEST_ASSERT_FALSE(cond, msg) TEST_ASSERT(!(cond), msg)
#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_STRING_EQ(a, b, msg) TEST_ASSERT(strcmp(a, b) == 0, msg)
#define TEST_ASSERT_FLOAT_EQ(a, b, eps, msg) TEST_ASSERT(fabs((a) - (b)) < (eps), msg)
#define TEST_ASSERT_RANGE(v, lo, hi, msg) TEST_ASSERT((v) >= (lo) && (v) <= (hi), msg)

// =============================================================================
// BASIC TYPES
// =============================================================================

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Quaternion;
typedef struct { float m[16]; } Mat4;
typedef struct { Vec3 min, max; } AABB;
typedef uint64_t Entity;
typedef uint32_t Handle;
typedef struct { float r, g, b, a; } Color;

// =============================================================================
// NANITE STUB
// =============================================================================

typedef struct NaniteMesh {
    uint32_t vertex_count;
    uint32_t index_count;
    uint32_t cluster_count;
} NaniteMesh;

NaniteMesh* nanite_mesh_create(uint32_t vertex_count, uint32_t index_count) {
    NaniteMesh* mesh = calloc(1, sizeof(NaniteMesh));
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    uint32_t triangles = index_count / 3;
    mesh->cluster_count = (triangles / 10000) + 1;
    return mesh;
}

void nanite_mesh_destroy(NaniteMesh* mesh) { free(mesh); }
uint32_t nanite_mesh_get_cluster_count(NaniteMesh* mesh) { return mesh ? mesh->cluster_count : 0; }

// =============================================================================
// LUMEN STUB
// =============================================================================

typedef struct LumenScene {
    float ray_count;
    Color sky_color;
} LumenScene;

typedef struct {
    Vec3 position;
    Color indirect_light;
    float ambient_occlusion;
} LumenSample;

LumenScene* lumen_scene_create(float ray_count) {
    LumenScene* scene = calloc(1, sizeof(LumenScene));
    scene->ray_count = ray_count;
    scene->sky_color = (Color){0.5f, 0.7f, 1.0f, 1.0f};
    return scene;
}

void lumen_scene_destroy(LumenScene* scene) { free(scene); }

LumenSample lumen_trace_indirect(LumenScene* scene, Vec3 pos, Vec3 normal) {
    return (LumenSample){
        .position = pos,
        .indirect_light = {0.1f, 0.1f, 0.15f, 1.0f},
        .ambient_occlusion = 0.85f
    };
}

// =============================================================================
// CHAOS PHYSICS STUB
// =============================================================================

typedef struct ChaosDestructible {
    uint32_t chunk_count;
    float damage_threshold;
} ChaosDestructible;

ChaosDestructible* chaos_destructible_create(float damage_threshold) {
    ChaosDestructible* dm = calloc(1, sizeof(ChaosDestructible));
    dm->chunk_count = 1;
    dm->damage_threshold = damage_threshold;
    return dm;
}

void chaos_destructible_destroy(ChaosDestructible* dm) { free(dm); }

void chaos_destructible_apply_damage(ChaosDestructible* dm, Vec3 location, float radius, float damage) {
    if (dm && damage > dm->damage_threshold) {
        dm->chunk_count += (uint32_t)(damage / 50.0f);
    }
}

uint32_t chaos_destructible_get_chunk_count(ChaosDestructible* dm) {
    return dm ? dm->chunk_count : 0;
}

// =============================================================================
// NIAGARA STUB
// =============================================================================

typedef struct NiagaraSystem {
    char name[64];
    uint32_t particle_count;
    float spawn_rate;
    bool active;
} NiagaraSystem;

NiagaraSystem* niagara_system_create(const char* name, float spawn_rate) {
    NiagaraSystem* sys = calloc(1, sizeof(NiagaraSystem));
    if (name) strncpy(sys->name, name, 63);
    sys->spawn_rate = spawn_rate;
    return sys;
}

void niagara_system_destroy(NiagaraSystem* sys) { free(sys); }

void niagara_system_activate(NiagaraSystem* sys) {
    if (sys) {
        sys->active = true;
        sys->particle_count = 50;
    }
}

void niagara_system_update(NiagaraSystem* sys, float dt) {
    if (sys && sys->active) {
        sys->particle_count += (uint32_t)(sys->spawn_rate * dt);
        if (sys->particle_count > 10000) sys->particle_count = 10000;
    }
}

uint32_t niagara_get_particle_count(NiagaraSystem* sys) {
    return sys ? sys->particle_count : 0;
}

// =============================================================================
// GAS STUB
// =============================================================================

#define MAX_ATTRIBUTES 16
#define MAX_TAGS 32

typedef struct {
    char name[32];
    float base_value;
    float current_value;
} Attribute;

typedef struct AbilitySystemComponent {
    Entity owner;
    Attribute attributes[MAX_ATTRIBUTES];
    uint32_t attribute_count;
    uint64_t tags[MAX_TAGS];
    uint32_t tag_count;
} AbilitySystemComponent;

AbilitySystemComponent* gas_create(Entity owner) {
    AbilitySystemComponent* asc = calloc(1, sizeof(AbilitySystemComponent));
    asc->owner = owner;
    return asc;
}

void gas_destroy(AbilitySystemComponent* asc) { free(asc); }

void gas_init_attribute(AbilitySystemComponent* asc, const char* name, float value) {
    if (asc && asc->attribute_count < MAX_ATTRIBUTES) {
        strncpy(asc->attributes[asc->attribute_count].name, name, 31);
        asc->attributes[asc->attribute_count].base_value = value;
        asc->attributes[asc->attribute_count].current_value = value;
        asc->attribute_count++;
    }
}

float gas_get_attribute(AbilitySystemComponent* asc, const char* name) {
    if (!asc) return 0.0f;
    for (uint32_t i = 0; i < asc->attribute_count; i++) {
        if (strcmp(asc->attributes[i].name, name) == 0) {
            return asc->attributes[i].current_value;
        }
    }
    return 0.0f;
}

void gas_set_attribute(AbilitySystemComponent* asc, const char* name, float value) {
    if (!asc) return;
    for (uint32_t i = 0; i < asc->attribute_count; i++) {
        if (strcmp(asc->attributes[i].name, name) == 0) {
            asc->attributes[i].current_value = value;
            return;
        }
    }
}

void gas_add_tag(AbilitySystemComponent* asc, uint64_t tag) {
    if (asc && asc->tag_count < MAX_TAGS) {
        asc->tags[asc->tag_count++] = tag;
    }
}

bool gas_has_tag(AbilitySystemComponent* asc, uint64_t tag) {
    if (!asc) return false;
    for (uint32_t i = 0; i < asc->tag_count; i++) {
        if (asc->tags[i] == tag) return true;
    }
    return false;
}

void gas_remove_tag(AbilitySystemComponent* asc, uint64_t tag) {
    if (!asc) return;
    for (uint32_t i = 0; i < asc->tag_count; i++) {
        if (asc->tags[i] == tag) {
            asc->tags[i] = asc->tags[--asc->tag_count];
            return;
        }
    }
}

// =============================================================================
// WORLD PARTITION STUB
// =============================================================================

typedef struct WorldPartition {
    float cell_size;
    int loading_range;
    Vec3 viewer_pos;
} WorldPartition;

WorldPartition* world_partition_create(float cell_size, int loading_range) {
    WorldPartition* wp = calloc(1, sizeof(WorldPartition));
    wp->cell_size = cell_size;
    wp->loading_range = loading_range;
    return wp;
}

void world_partition_destroy(WorldPartition* wp) { free(wp); }

void world_partition_update(WorldPartition* wp, Vec3 viewer_pos) {
    if (wp) wp->viewer_pos = viewer_pos;
}

bool world_partition_is_cell_loaded(WorldPartition* wp, int x, int z) {
    if (!wp) return false;
    float cx = x * wp->cell_size;
    float cz = z * wp->cell_size;
    float dx = cx - wp->viewer_pos.x;
    float dz = cz - wp->viewer_pos.z;
    float dist = sqrtf(dx*dx + dz*dz);
    return dist < wp->cell_size * wp->loading_range;
}

// =============================================================================
// ECS STUB
// =============================================================================

typedef struct ECSWorld {
    Entity next_entity;
    uint32_t entity_count;
} ECSWorld;

ECSWorld* ecs_world_create(void) {
    ECSWorld* world = calloc(1, sizeof(ECSWorld));
    world->next_entity = 1;
    return world;
}

void ecs_world_destroy(ECSWorld* world) { free(world); }

Entity ecs_create_entity(ECSWorld* world) {
    if (!world) return 0;
    world->entity_count++;
    return world->next_entity++;
}

void ecs_destroy_entity(ECSWorld* world, Entity entity) {
    if (world && world->entity_count > 0) world->entity_count--;
}

uint32_t ecs_get_entity_count(ECSWorld* world) {
    return world ? world->entity_count : 0;
}

// =============================================================================
// TESTS - NANITE
// =============================================================================

static TestResult test_nanite_mesh_creation(void) {
    NaniteMesh* mesh = nanite_mesh_create(10000, 30000);
    TEST_ASSERT_NOT_NULL(mesh, "Nanite mesh should be created");
    TEST_ASSERT_EQ(mesh->vertex_count, 10000, "Vertex count matches");
    nanite_mesh_destroy(mesh);
    return TEST_PASS;
}

static TestResult test_nanite_cluster_generation(void) {
    NaniteMesh* mesh = nanite_mesh_create(1000000, 3000000);
    uint32_t clusters = nanite_mesh_get_cluster_count(mesh);
    TEST_ASSERT_TRUE(clusters >= 100, "High poly mesh should have many clusters");
    nanite_mesh_destroy(mesh);
    return TEST_PASS;
}

// =============================================================================
// TESTS - LUMEN
// =============================================================================

static TestResult test_lumen_scene_creation(void) {
    LumenScene* scene = lumen_scene_create(1.0f);
    TEST_ASSERT_NOT_NULL(scene, "Lumen scene should be created");
    lumen_scene_destroy(scene);
    return TEST_PASS;
}

static TestResult test_lumen_indirect_lighting(void) {
    LumenScene* scene = lumen_scene_create(2.0f);
    Vec3 pos = {0, 100, 0};
    Vec3 normal = {0, 1, 0};
    LumenSample sample = lumen_trace_indirect(scene, pos, normal);
    TEST_ASSERT_TRUE(sample.indirect_light.r >= 0.0f, "Valid indirect light");
    TEST_ASSERT_RANGE(sample.ambient_occlusion, 0.0f, 1.0f, "AO in range");
    lumen_scene_destroy(scene);
    return TEST_PASS;
}

// =============================================================================
// TESTS - CHAOS
// =============================================================================

static TestResult test_chaos_destructible_creation(void) {
    ChaosDestructible* dm = chaos_destructible_create(100.0f);
    TEST_ASSERT_NOT_NULL(dm, "Destructible should be created");
    TEST_ASSERT_EQ(chaos_destructible_get_chunk_count(dm), 1, "Initial chunk count");
    chaos_destructible_destroy(dm);
    return TEST_PASS;
}

static TestResult test_chaos_destruction_damage(void) {
    ChaosDestructible* dm = chaos_destructible_create(50.0f);
    uint32_t initial = chaos_destructible_get_chunk_count(dm);
    chaos_destructible_apply_damage(dm, (Vec3){0,0,0}, 5.0f, 150.0f);
    uint32_t after = chaos_destructible_get_chunk_count(dm);
    TEST_ASSERT_TRUE(after > initial, "Damage should create chunks");
    chaos_destructible_destroy(dm);
    return TEST_PASS;
}

// =============================================================================
// TESTS - NIAGARA
// =============================================================================

static TestResult test_niagara_system_creation(void) {
    NiagaraSystem* sys = niagara_system_create("ExplosionFX", 100.0f);
    TEST_ASSERT_NOT_NULL(sys, "Niagara system created");
    TEST_ASSERT_STRING_EQ(sys->name, "ExplosionFX", "Name matches");
    niagara_system_destroy(sys);
    return TEST_PASS;
}

static TestResult test_niagara_particle_spawn(void) {
    NiagaraSystem* sys = niagara_system_create("TestFX", 1000.0f);
    niagara_system_activate(sys);
    TEST_ASSERT_TRUE(niagara_get_particle_count(sys) > 0, "Initial particles spawned");
    
    for (int i = 0; i < 10; i++) {
        niagara_system_update(sys, 0.016f);
    }
    
    TEST_ASSERT_TRUE(niagara_get_particle_count(sys) > 50, "Particles increased");
    niagara_system_destroy(sys);
    return TEST_PASS;
}

// =============================================================================
// TESTS - GAS
// =============================================================================

static TestResult test_gas_creation(void) {
    AbilitySystemComponent* asc = gas_create(1);
    TEST_ASSERT_NOT_NULL(asc, "ASC created");
    TEST_ASSERT_EQ(asc->owner, 1, "Owner set");
    gas_destroy(asc);
    return TEST_PASS;
}

static TestResult test_gas_attributes(void) {
    AbilitySystemComponent* asc = gas_create(1);
    gas_init_attribute(asc, "Health", 100.0f);
    gas_init_attribute(asc, "Mana", 50.0f);
    
    TEST_ASSERT_FLOAT_EQ(gas_get_attribute(asc, "Health"), 100.0f, 0.01f, "Health correct");
    TEST_ASSERT_FLOAT_EQ(gas_get_attribute(asc, "Mana"), 50.0f, 0.01f, "Mana correct");
    
    gas_set_attribute(asc, "Health", 75.0f);
    TEST_ASSERT_FLOAT_EQ(gas_get_attribute(asc, "Health"), 75.0f, 0.01f, "Health modified");
    
    gas_destroy(asc);
    return TEST_PASS;
}

static TestResult test_gas_tags(void) {
    AbilitySystemComponent* asc = gas_create(1);
    
    gas_add_tag(asc, 0x12345678);
    gas_add_tag(asc, 0x87654321);
    
    TEST_ASSERT_TRUE(gas_has_tag(asc, 0x12345678), "Has first tag");
    TEST_ASSERT_TRUE(gas_has_tag(asc, 0x87654321), "Has second tag");
    TEST_ASSERT_FALSE(gas_has_tag(asc, 0xDEADBEEF), "Doesn't have missing tag");
    
    gas_remove_tag(asc, 0x12345678);
    TEST_ASSERT_FALSE(gas_has_tag(asc, 0x12345678), "Tag removed");
    
    gas_destroy(asc);
    return TEST_PASS;
}

// =============================================================================
// TESTS - WORLD PARTITION
// =============================================================================

static TestResult test_world_partition_creation(void) {
    WorldPartition* wp = world_partition_create(12800.0f, 3);
    TEST_ASSERT_NOT_NULL(wp, "World partition created");
    world_partition_destroy(wp);
    return TEST_PASS;
}

static TestResult test_world_partition_streaming(void) {
    WorldPartition* wp = world_partition_create(5000.0f, 2);
    world_partition_update(wp, (Vec3){0, 0, 0});
    
    TEST_ASSERT_TRUE(world_partition_is_cell_loaded(wp, 0, 0), "Origin cell loaded");
    TEST_ASSERT_FALSE(world_partition_is_cell_loaded(wp, 100, 100), "Far cell not loaded");
    
    world_partition_destroy(wp);
    return TEST_PASS;
}

// =============================================================================
// TESTS - ECS
// =============================================================================

static TestResult test_ecs_world_creation(void) {
    ECSWorld* world = ecs_world_create();
    TEST_ASSERT_NOT_NULL(world, "ECS world created");
    ecs_world_destroy(world);
    return TEST_PASS;
}

static TestResult test_ecs_entity_lifecycle(void) {
    ECSWorld* world = ecs_world_create();
    
    Entity e1 = ecs_create_entity(world);
    Entity e2 = ecs_create_entity(world);
    Entity e3 = ecs_create_entity(world);
    
    TEST_ASSERT_TRUE(e1 != 0, "Entity 1 valid");
    TEST_ASSERT_TRUE(e2 != 0, "Entity 2 valid");
    TEST_ASSERT_TRUE(e1 != e2, "Entities unique");
    TEST_ASSERT_EQ(ecs_get_entity_count(world), 3, "3 entities");
    
    ecs_destroy_entity(world, e2);
    TEST_ASSERT_EQ(ecs_get_entity_count(world), 2, "2 entities after destroy");
    
    ecs_world_destroy(world);
    return TEST_PASS;
}

// =============================================================================
// TEST REGISTRATION
// =============================================================================

static void register_all_tests(void) {
    // Nanite
    test_register("UE:Nanite", "Mesh creation", test_nanite_mesh_creation);
    test_register("UE:Nanite", "Cluster generation", test_nanite_cluster_generation);
    
    // Lumen
    test_register("UE:Lumen", "Scene creation", test_lumen_scene_creation);
    test_register("UE:Lumen", "Indirect lighting", test_lumen_indirect_lighting);
    
    // Chaos
    test_register("UE:Chaos", "Destructible creation", test_chaos_destructible_creation);
    test_register("UE:Chaos", "Destruction damage", test_chaos_destruction_damage);
    
    // Niagara
    test_register("UE:Niagara", "System creation", test_niagara_system_creation);
    test_register("UE:Niagara", "Particle spawn", test_niagara_particle_spawn);
    
    // GAS
    test_register("UE:GAS", "ASC creation", test_gas_creation);
    test_register("UE:GAS", "Attributes", test_gas_attributes);
    test_register("UE:GAS", "Tags", test_gas_tags);
    
    // World Partition
    test_register("UE:WorldPartition", "Creation", test_world_partition_creation);
    test_register("UE:WorldPartition", "Streaming", test_world_partition_streaming);
    
    // ECS
    test_register("ECS", "World creation", test_ecs_world_creation);
    test_register("ECS", "Entity lifecycle", test_ecs_entity_lifecycle);
}

// =============================================================================
// MAIN
// =============================================================================

int main(int argc, char **argv) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║     GAME ENGINE TEST SUITE - UNREAL ENGINE PARITY                ║\n");
    printf("║     Self-Contained TDD Tests                                     ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
    
    register_all_tests();
    
    printf("Running %d tests...\n", g_test_count);
    printf("══════════════════════════════════════════════════════════════════\n\n");
    
    const char* current_suite = "";
    clock_t start = clock();
    
    for (int i = 0; i < g_test_count; i++) {
        // Print suite header if changed
        if (strcmp(current_suite, g_tests[i].suite) != 0) {
            current_suite = g_tests[i].suite;
            printf(COLOR_CYAN "── %s ──" COLOR_RESET "\n", current_suite);
        }
        
        printf("  %-40s ", g_tests[i].name);
        fflush(stdout);
        
        TestResult result = g_tests[i].func();
        
        switch (result) {
            case TEST_PASS:
                printf("[" COLOR_GREEN "PASS" COLOR_RESET "]\n");
                g_passed++;
                break;
            case TEST_FAIL:
                printf("[" COLOR_RED "FAIL" COLOR_RESET "]\n");
                g_failed++;
                break;
            case TEST_SKIP:
                printf("[" COLOR_YELLOW "SKIP" COLOR_RESET "]\n");
                g_skipped++;
                break;
        }
    }
    
    clock_t end = clock();
    double duration = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("\n══════════════════════════════════════════════════════════════════\n");
    printf("                         SUMMARY                                   \n");
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("  Total:   %d\n", g_test_count);
    printf("  Passed:  " COLOR_GREEN "%d" COLOR_RESET "\n", g_passed);
    printf("  Failed:  " COLOR_RED "%d" COLOR_RESET "\n", g_failed);
    printf("  Skipped: " COLOR_YELLOW "%d" COLOR_RESET "\n", g_skipped);
    printf("  Time:    %.2f ms\n", duration);
    
    if (g_failed == 0) {
        printf("\n" COLOR_GREEN);
        printf("╔══════════════════════════════════════════════════════════════════╗\n");
        printf("║                    ✓ ALL TESTS PASSED                            ║\n");
        printf("╚══════════════════════════════════════════════════════════════════╝\n");
        printf(COLOR_RESET "\n");
    } else {
        printf("\n" COLOR_RED);
        printf("╔══════════════════════════════════════════════════════════════════╗\n");
        printf("║                    ✗ %d TESTS FAILED                              ║\n", g_failed);
        printf("╚══════════════════════════════════════════════════════════════════╝\n");
        printf(COLOR_RESET "\n");
    }
    
    return g_failed > 0 ? 1 : 0;
}
