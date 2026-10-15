/* Keep assert() active in every build type. A Release build defines NDEBUG,
 * which would turn these checks into no-ops and let the test pass vacuously. */
#undef NDEBUG

#include "include/animation/ik_system.h"
#include "include/core/logger.h"
#include "include/math/math_all.h"
#include "character/animation/ik/fabrik_solver.h"
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

// Logger stubs. The unified logger .c does not build in this target.
bool unified_logger_init(const LoggerConfig *config) {
    (void)config;
    return true;
}
void unified_logger_shutdown(void) {}
void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    (void)level;
    (void)category;
    (void)file;
    (void)line;
    (void)function;
    va_list args;
    va_start(args, format);
    printf("[LOG] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
void unified_logger_flush(void) { fflush(stdout); }
LoggerConfig unified_logger_get_config(void) {
    LoggerConfig c = {0};
    return c;
}
void unified_logger_set_config(const LoggerConfig *config) { (void)config; }
void unified_logger_set_level(LogLevel level) { (void)level; }
void unified_logger_enable_category(LogCategory category, bool enabled) {
    (void)category;
    (void)enabled;
}

// Simple FABRIK stays compiled into ik_system.c. It has no public header.
void fabrik_init(void);
uint32_t fabrik_create_chain(const Vec3 *positions, uint32_t joint_count);
void fabrik_destroy_chain(uint32_t chain_id);
Vec3 fabrik_get_joint_position(uint32_t chain_id, uint32_t joint_index);

static void straight_chain(Vec3 positions[3], float origin_x) {
    positions[0] = vec3_create(origin_x, 0.0f, 0.0f);
    positions[1] = vec3_create(origin_x + 1.0f, 0.0f, 0.0f);
    positions[2] = vec3_create(origin_x + 2.0f, 0.0f, 0.0f);
}

static void test_root_pin_and_set_positions(void) {
    printf("Testing FABRIK root pin and set_chain_positions...\n");

    assert(animation_fabrik_solver_init() == 0);
    animation_fabrik_solver_handle_t handle;
    animation_fabrik_solver_desc_t desc = {0};
    assert(animation_fabrik_solver_create(&handle, &desc) == 0);

    Vec3 positions[3];
    straight_chain(positions, 0.0f);
    uint32_t chain_id = animation_fabrik_solver_add_chain(handle, positions, 3);
    assert(chain_id != UINT32_MAX);

    Vec3 target = vec3_create(0.0f, 1.2f, 0.0f);
    assert(animation_fabrik_solver_solve_chain(handle, chain_id, &target, 20));

    Vec3 root = animation_fabrik_solver_get_joint_position(handle, chain_id, 0);
    Vec3 end = animation_fabrik_solver_get_joint_position(handle, chain_id, 2);
    printf("Pinned root: (%.4f, %.4f, %.4f) end error %.4f\n", root.x, root.y,
           root.z, vec3_distance(end, target));
    assert(vec3_distance(root, positions[0]) < 1e-4f);
    assert(vec3_distance(end, target) < 0.05f);

    Vec3 far = vec3_create(10.0f, 0.0f, 0.0f);
    assert(animation_fabrik_solver_solve_chain(handle, chain_id, &far, 5));
    root = animation_fabrik_solver_get_joint_position(handle, chain_id, 0);
    end = animation_fabrik_solver_get_joint_position(handle, chain_id, 2);
    assert(vec3_distance(root, positions[0]) < 1e-4f);
    assert(fabsf(end.x - 2.0f) < 1e-3f);
    assert(fabsf(end.y) < 1e-3f);

    Vec3 moved[3];
    straight_chain(moved, 5.0f);
    assert(animation_fabrik_solver_set_chain_positions(handle, chain_id, moved, 3) == 0);
    Vec3 moved_target = vec3_create(5.0f, 1.2f, 0.0f);
    assert(animation_fabrik_solver_solve_chain(handle, chain_id, &moved_target, 20));
    root = animation_fabrik_solver_get_joint_position(handle, chain_id, 0);
    end = animation_fabrik_solver_get_joint_position(handle, chain_id, 2);
    printf("Moved root: (%.4f, %.4f, %.4f) end error %.4f\n", root.x, root.y,
           root.z, vec3_distance(end, moved_target));
    assert(vec3_distance(root, moved[0]) < 1e-4f);
    assert(vec3_distance(end, moved_target) < 0.05f);

    assert(animation_fabrik_solver_set_chain_positions(handle, chain_id, NULL, 3) == -1);
    assert(animation_fabrik_solver_set_chain_positions(handle, chain_id, moved, 1) == -1);
    assert(animation_fabrik_solver_set_chain_positions(handle, 99, moved, 3) == -1);

    animation_fabrik_solver_destroy(handle);
    animation_fabrik_solver_shutdown();
    printf("Root pin passed.\n");
}

static void fill_chain_bones(IKChain *chain, float origin_x) {
    chain->bone_count = 3;
    for (u32 i = 0; i < chain->bone_count; i++) {
        chain->bones[i].position = vec3_create(origin_x + (float)i, 0.0f, 0.0f);
        chain->bones[i].rotation = quat_identity();
        chain->bones[i].length = (i < 2) ? 1.0f : 0.0f;
        chain->bones[i].constraints_enabled = false;
    }
}

static void test_persistent_ik_system_chain(void) {
    printf("Testing persistent FABRIK chain lifecycle...\n");

    IKSystem *system = ik_system_create();
    assert(system != NULL);
    assert(system->fabrik_solver_id != UINT32_MAX);

    u32 id = ik_add_chain(system, "Arm", IK_SOLVER_FABRIK);
    IKChain *chain = &system->chains[id];
    assert(chain->fabrik_chain_id == UINT32_MAX);
    fill_chain_bones(chain, 0.0f);
    chain->max_iterations = 20;

    Vec3 target = vec3_create(0.0f, 1.2f, 0.0f);
    ik_set_target(system, id, target, quat_identity());
    ik_solve(system, id);

    u32 persistent_id = chain->fabrik_chain_id;
    assert(persistent_id != UINT32_MAX);
    assert(vec3_distance(chain->bones[0].position, vec3_create(0.0f, 0.0f, 0.0f)) < 1e-4f);
    float reached = vec3_distance(chain->bones[2].position, target);
    printf("System end error %.4f chain %u\n", reached, persistent_id);
    assert(reached < 0.05f);

    ik_solve(system, id);
    assert(chain->fabrik_chain_id == persistent_id);
    assert(vec3_distance(chain->bones[0].position, vec3_create(0.0f, 0.0f, 0.0f)) < 1e-4f);

    fill_chain_bones(chain, 8.0f);
    Vec3 moved_target = vec3_create(8.0f, 1.2f, 0.0f);
    ik_set_target(system, id, moved_target, quat_identity());
    ik_solve(system, id);
    assert(chain->fabrik_chain_id == persistent_id);
    assert(vec3_distance(chain->bones[0].position, vec3_create(8.0f, 0.0f, 0.0f)) < 1e-4f);
    assert(vec3_distance(chain->bones[2].position, moved_target) < 0.05f);

    Vec3 simple_positions[2] = {vec3_create(1.0f, 2.0f, 3.0f), vec3_create(2.0f, 2.0f, 3.0f)};
    uint32_t simple_id = fabrik_create_chain(simple_positions, 2);
    assert(simple_id != UINT32_MAX);
    Vec3 simple_root = fabrik_get_joint_position(simple_id, 0);
    assert(vec3_distance(simple_root, simple_positions[0]) < 1e-4f);
    fabrik_destroy_chain(simple_id);

    ik_system_destroy(system);
    printf("Persistent chain passed.\n");
}

int main(void) {
    setbuf(stdout, NULL);
    test_root_pin_and_set_positions();
    test_persistent_ik_system_chain();
    printf("All FABRIK tests passed.\n");
    return 0;
}
