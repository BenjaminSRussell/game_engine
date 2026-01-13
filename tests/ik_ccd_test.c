#include "include/animation/ik_system.h"
#include "include/core/logger.h"
#include "include/math/math.h"
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

// ============================================================================
// STUBS
// ============================================================================

// Logger stubs
bool unified_logger_init(const LoggerConfig *config) { return true; }
void unified_logger_shutdown(void) {}
void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
    va_list args;
    va_start(args, format);
    // Print everything
    printf("[LOG] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
void unified_logger_flush(void) { fflush(stdout); }
LoggerConfig unified_logger_get_config(void) { LoggerConfig c = {0}; return c; }
void unified_logger_set_config(const LoggerConfig *config) {}
void unified_logger_set_level(LogLevel level) {}
void unified_logger_enable_category(LogCategory category, bool enabled) {}

// FABRIK solver stubs
int animation_fabrik_solver_init(void) { return 0; }
void animation_fabrik_solver_shutdown(void) {}

// ============================================================================
// TESTS
// ============================================================================

static void test_ccd_solver_basic(void) {
    printf("Testing CCD Solver Basic Reach...\n");

    IKSystem *system = ik_system_create();
    assert(system != NULL);

    u32 chain_id = ik_add_chain(system, "TestChain", IK_SOLVER_CCD);
    IKChain *chain = &system->chains[chain_id];

    chain->bone_count = 3;
    // Bone 0: (0,0,0) length 1
    chain->bones[0].position = vec3_create(0,0,0);
    chain->bones[0].rotation = quat_identity();
    chain->bones[0].length = 1.0f;
    chain->bones[0].constraints_enabled = false;

    // Bone 1: (1,0,0) length 1
    chain->bones[1].position = vec3_create(1,0,0);
    chain->bones[1].rotation = quat_identity();
    chain->bones[1].length = 1.0f;
    chain->bones[1].constraints_enabled = false;

    // Bone 2: (2,0,0) - End effector
    chain->bones[2].position = vec3_create(2,0,0);
    chain->bones[2].rotation = quat_identity();
    chain->bones[2].length = 0.0f;
    chain->bones[2].constraints_enabled = false;

    Vec3 target = vec3_create(1.0f, 1.0f, 0.0f);
    ik_set_target(system, chain_id, target, quat_identity());

    chain->max_iterations = 20;
    chain->precision = 0.01f;
    ik_solve(system, chain_id);

    Vec3 end_effector = chain->bones[2].position;
    float dist = vec3_distance(end_effector, target);
    printf("Target: (%.2f, %.2f, %.2f), Actual: (%.2f, %.2f, %.2f), Error: %.4f\n",
           target.x, target.y, target.z,
           end_effector.x, end_effector.y, end_effector.z, dist);

    assert(dist < 0.05f);

    ik_system_destroy(system);
    printf("Basic Reach Passed.\n");
}

static void test_ccd_solver_constraints(void) {
    printf("Testing CCD Solver Constraints...\n");

    IKSystem *system = ik_system_create();
    u32 chain_id = ik_add_chain(system, "ConstraintChain", IK_SOLVER_CCD);
    IKChain *chain = &system->chains[chain_id];

    chain->bone_count = 3;
    chain->bones[0].position = vec3_create(0,0,0);
    chain->bones[0].rotation = quat_identity();
    chain->bones[0].length = 1.0f;

    chain->bones[1].position = vec3_create(1,0,0);
    chain->bones[1].rotation = quat_identity();
    chain->bones[1].length = 1.0f;

    chain->bones[2].position = vec3_create(2,0,0);
    chain->bones[2].length = 0.0f;

    // Enable constraints on Bone 0 (Root)
    // Limit rotation around all axes heavily
    chain->bones[0].constraints_enabled = true;
    chain->bones[0].min_angles = vec3_create(-0.1f, -0.1f, -0.1f);
    chain->bones[0].max_angles = vec3_create(0.1f, 0.1f, 0.1f);

    Vec3 target = vec3_create(0.0f, 1.0f, 0.0f);
    ik_set_target(system, chain_id, target, quat_identity());

    ik_solve(system, chain_id);

    Vec3 end_effector = chain->bones[2].position;
    printf("Target: (0, 1, 0), Actual: (%.2f, %.2f, %.2f)\n",
           end_effector.x, end_effector.y, end_effector.z);

    float dist = vec3_distance(end_effector, target);
    printf("Distance: %.4f\n", dist);

    // If constraints work, it should NOT reach (0,1,0).
    // If Bone 0 is locked near X axis, best reach is ~0.4 distance away.
    // If constraints were broken, it would reach dist ~ 0.0.
    assert(dist > 0.2f);

    ik_system_destroy(system);
    printf("Constraints Passed.\n");
}

static void test_ccd_solver_pole_vector(void) {
    printf("Testing CCD Solver Pole Vector...\n");

    IKSystem *system = ik_system_create();
    u32 chain_id = ik_add_chain(system, "PoleChain", IK_SOLVER_CCD);
    IKChain *chain = &system->chains[chain_id];

    chain->bone_count = 3;
    chain->bones[0].position = vec3_create(0,0,0);
    chain->bones[0].rotation = quat_identity();
    chain->bones[0].length = 1.0f;

    chain->bones[1].position = vec3_create(1,0,0);
    chain->bones[1].rotation = quat_identity();
    chain->bones[1].length = 1.0f;

    chain->bones[2].position = vec3_create(2,0,0);

    // Target is closer, so knee must bend
    Vec3 target = vec3_create(1.5f, 0.0f, 0.0f);
    ik_set_target(system, chain_id, target, quat_identity());

    // Set Pole Vector to UP (0, 1, 0)
    // The knee (joint 1) should pop UP (positive Y)
    chain->pole_vector_enabled = true;
    chain->pole_vector = vec3_create(1.0f, 1.0f, 0.0f);

    ik_solve(system, chain_id);

    Vec3 knee_pos = chain->bones[1].position;
    printf("Pole: (1, 1, 0), Knee: (%.2f, %.2f, %.2f)\n",
           knee_pos.x, knee_pos.y, knee_pos.z);

    // Knee should have positive Y
    assert(knee_pos.y > 0.1f);
    // Knee should be roughly on the plane Z=0 (since Pole is on Z=0, Start/End on Z=0)
    assert(fabsf(knee_pos.z) < 0.1f);

    // Now change Pole Vector to (1, 0, 1) (Forward Z)
    // Knee should pop Z+
    chain->pole_vector = vec3_create(1.0f, 0.0f, 1.0f);

    // Reset chain somewhat? CCD iterates from current.
    ik_solve(system, chain_id);

    knee_pos = chain->bones[1].position;
    printf("Pole: (1, 0, 1), Knee: (%.2f, %.2f, %.2f)\n",
           knee_pos.x, knee_pos.y, knee_pos.z);

    assert(knee_pos.z > 0.1f);
    assert(fabsf(knee_pos.y) < 0.1f); // Should be flat on Y

    ik_system_destroy(system);
    printf("Pole Vector Passed.\n");
}

int main(void) {
    setbuf(stdout, NULL);
    test_ccd_solver_basic();
    test_ccd_solver_constraints();
    test_ccd_solver_pole_vector();
    printf("All tests passed.\n");
    return 0;
}
