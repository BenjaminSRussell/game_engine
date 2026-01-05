/**
 * COMPREHENSIVE GAME ENGINE TEST SUITE - MASTER RUNNER
 * Full Unreal Engine 5.2 Feature Parity Testing
 * 
 * This file registers and runs ALL test suites covering:
 * - Core Engine Systems (Rendering, Physics, Animation, Audio, ECS, AI, Networking)
 * - Unreal Engine Parity Features (Nanite, Lumen, Chaos, Niagara, GAS, etc.)
 * 
 * Run with: ./run_all_tests [--suite <name>] [--list] [--verbose]
 */

#include "../test_framework_unified.h"
#include <stdio.h>
#include <string.h>

// =============================================================================
// FORWARD DECLARATIONS - Core Engine Test Suites
// =============================================================================

void register_rendering_tests(void);
void register_physics_tests(void);
void register_animation_tests(void);
void register_audio_tests(void);
void register_ecs_tests(void);
void register_ai_tests(void);
void register_networking_tests(void);

// Stub test suites
void register_input_tests(void);
void register_scripting_tests(void);
void register_gameplay_tests(void);
void register_world_tests(void);
void register_environment_tests(void);
void register_editor_tests(void);
void register_asset_tests(void);
void register_platform_tests(void);
void register_performance_tests(void);
void register_accessibility_tests(void);

// =============================================================================
// FORWARD DECLARATIONS - Unreal Engine Parity Test Suites
// =============================================================================

void register_ue_nanite_lumen_tests(void);  // Nanite, Lumen, VSM, TSR
void register_ue_chaos_tests(void);          // Destruction, Cloth, Vehicles
void register_ue_niagara_tests(void);        // Particle System
void register_ue_gas_tests(void);            // Gameplay Ability System
void register_ue_world_tests(void);          // World Partition, Water, Landscape, Foliage, PCG
void register_ue_ai_tests(void);             // Mass AI, State Tree, Smart Objects
void register_ue_media_tests(void);          // MetaSounds, Sequencer, Movie Render, Input, Control Rig

// =============================================================================
// COMMAND LINE PARSING
// =============================================================================

typedef struct {
    const char *suite_filter;
    bool list_only;
    bool verbose;
    bool help;
} TestOptions;

static void print_usage(const char *program) {
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║     GAME ENGINE TEST SUITE - USAGE                               ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
    printf("Usage: %s [OPTIONS]\n\n", program);
    printf("Options:\n");
    printf("  --suite <name>   Run only tests in the specified suite\n");
    printf("  --list           List all registered tests without running\n");
    printf("  --verbose        Show detailed test output\n");
    printf("  --help           Show this help message\n");
    printf("\n");
    printf("Core Engine Suites:\n");
    printf("  Rendering        Pipeline, materials, post-processing, LOD\n");
    printf("  Physics          Rigid bodies, collision, joints, vehicles\n");
    printf("  Animation        Skeletal, blend trees, IK, retargeting\n");
    printf("  Audio            Spatial audio, effects, music system\n");
    printf("  ECS              Entities, components, systems, queries\n");
    printf("  AI               NavMesh, behavior trees, GOAP, perception\n");
    printf("  Networking       Transport, RPC, replication, matchmaking\n");
    printf("\n");
    printf("Unreal Engine Parity Suites:\n");
    printf("  UE:Nanite        Virtualized geometry system\n");
    printf("  UE:Lumen         Global illumination\n");
    printf("  UE:VSM           Virtual shadow maps\n");
    printf("  UE:TSR           Temporal super resolution\n");
    printf("  UE:Chaos         Destruction, cloth, vehicles\n");
    printf("  UE:Niagara       Advanced particle system\n");
    printf("  UE:GAS           Gameplay Ability System\n");
    printf("  UE:WorldPartition Open world streaming\n");
    printf("  UE:Water         Water body simulation\n");
    printf("  UE:Landscape     Terrain system\n");
    printf("  UE:Foliage       Vegetation instancing\n");
    printf("  UE:PCG           Procedural content generation\n");
    printf("  UE:MassAI        Crowd AI system\n");
    printf("  UE:StateTree     AI state management\n");
    printf("  UE:SmartObjects  AI interaction points\n");
    printf("  UE:MetaSounds    Procedural audio\n");
    printf("  UE:Sequencer     Cinematics system\n");
    printf("  UE:MovieRender   Offline rendering\n");
    printf("  UE:EnhancedInput Advanced input system\n");
    printf("  UE:ControlRig    Animation rigging\n");
}

static TestOptions parse_args(int argc, char **argv) {
    TestOptions opts = {
        .suite_filter = NULL,
        .list_only = false,
        .verbose = false,
        .help = false
    };
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--suite") == 0 && i + 1 < argc) {
            opts.suite_filter = argv[++i];
        } else if (strcmp(argv[i], "--list") == 0) {
            opts.list_only = true;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            opts.verbose = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            opts.help = true;
        }
    }
    
    return opts;
}

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(int argc, char **argv) {
    TestOptions opts = parse_args(argc, argv);
    
    if (opts.help) {
        print_usage(argv[0]);
        return 0;
    }
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║     COMPREHENSIVE GAME ENGINE TEST SUITE                         ║\n");
    printf("║     Unreal Engine 5.2 Feature Parity Validation                  ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
    
    // Initialize test framework
    test_init();
    test_enable_leak_detection();
    
    printf("Registering test suites...\n");
    printf("──────────────────────────────────────────────────────────────────\n");
    
    // Core Engine Systems
    printf("  [Core] Rendering, Physics, Animation, Audio, ECS, AI, Networking\n");
    register_rendering_tests();
    register_physics_tests();
    register_animation_tests();
    register_audio_tests();
    register_ecs_tests();
    register_ai_tests();
    register_networking_tests();
    
    // Stub systems
    printf("  [Stub] Input, Scripting, Gameplay, World, Editor, Platform\n");
    register_input_tests();
    register_scripting_tests();
    register_gameplay_tests();
    register_world_tests();
    register_environment_tests();
    register_editor_tests();
    register_asset_tests();
    register_platform_tests();
    register_performance_tests();
    register_accessibility_tests();
    
    // Unreal Engine Parity
    printf("  [UE5]  Nanite, Lumen, Chaos, Niagara, GAS, World Systems\n");
    register_ue_nanite_lumen_tests();
    register_ue_chaos_tests();
    register_ue_niagara_tests();
    register_ue_gas_tests();
    register_ue_world_tests();
    printf("  [UE5]  Mass AI, State Tree, Smart Objects, MetaSounds, Sequencer\n");
    register_ue_ai_tests();
    register_ue_media_tests();
    
    printf("──────────────────────────────────────────────────────────────────\n");
    
    // List tests if requested
    if (opts.list_only) {
        test_list_all();
        test_cleanup();
        return 0;
    }
    
    // Run tests
    printf("\nRunning tests...\n");
    printf("══════════════════════════════════════════════════════════════════\n");
    
    TestStats stats;
    if (opts.suite_filter) {
        printf("Filtering by suite: %s\n", opts.suite_filter);
        stats = test_run_suite(opts.suite_filter);
    } else {
        stats = test_run_all();
    }
    
    // Print summary
    printf("\n══════════════════════════════════════════════════════════════════\n");
    printf("                           TEST SUMMARY                            \n");
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("  Total:   %u tests\n", stats.total);
    printf("  Passed:  \033[32m%u\033[0m\n", stats.passed);
    printf("  Failed:  \033[31m%u\033[0m\n", stats.failed);
    printf("  Skipped: \033[33m%u\033[0m\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    
    // Check for memory leaks
    if (test_check_leaks()) {
        printf("\n\033[31m⚠ Memory leaks detected!\033[0m\n");
    }
    
    // Print pass/fail banner
    if (stats.failed == 0) {
        printf("\n\033[32m");
        printf("╔══════════════════════════════════════════════════════════════════╗\n");
        printf("║                    ✓ ALL TESTS PASSED                            ║\n");
        printf("╚══════════════════════════════════════════════════════════════════╝\n");
        printf("\033[0m\n");
    } else {
        printf("\n\033[31m");
        printf("╔══════════════════════════════════════════════════════════════════╗\n");
        printf("║                    ✗ %u TESTS FAILED                              ║\n", stats.failed);
        printf("╚══════════════════════════════════════════════════════════════════╝\n");
        printf("\033[0m\n");
    }
    
    test_cleanup();
    return stats.failed > 0 ? 1 : 0;
}
