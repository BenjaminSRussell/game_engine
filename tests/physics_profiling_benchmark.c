#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include "src/engine/physics/solver/physics_solver_system.h"
#include "src/engine/physics/solver/xpbd_solver.h"
#include "src/engine/physics/solver/sequential_impulse.h"

// Helper to get time in milliseconds
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main(void) {
    printf("==============================================================\n");
    printf("PHYSICS SYSTEM PERFORMANCE PROFILING (TODO-0045)\n");
    printf("Target: 1000+ rigid bodies @ 60Hz\n");
    printf("==============================================================\n\n");

    // Configuration
    const int BODY_COUNT = 1000;
    const int SIMULATION_FRAMES = 600; // 10 seconds at 60Hz
    const float DT = 1.0f / 60.0f;

    // Setup Physics System
    physics_solver_config config = physics_solver_get_default_config();
    config.max_rigid_bodies = BODY_COUNT + 100; // Extra buffer
    config.time_step = DT;

    physics_solver_system *system = physics_solver_create(&config);
    if (!system) {
        fprintf(stderr, "Error: Failed to create physics solver system.\n");
        return 1;
    }

    printf("Initializing %d rigid bodies...\n", BODY_COUNT);

    // Create 1000 bodies
    // Arranged in a grid or stack to ensure collisions happen, but not exploding immediately
    int dim = (int)ceil(pow((double)BODY_COUNT, 1.0/3.0)); // Cube root for 3D grid

    int added_count = 0;
    for (int i = 0; i < BODY_COUNT; i++) {
        int x = i % dim;
        int y = (i / dim) % dim;
        int z = i / (dim * dim);

        RigidBody body;
        memset(&body, 0, sizeof(RigidBody));

        body.position[0] = (float)x * 2.0f - (float)dim; // Spaced 2.0 apart
        body.position[1] = (float)y * 2.0f + 5.0f;       // Start 5.0 units high
        body.position[2] = (float)z * 2.0f - (float)dim;

        body.orientation[3] = 1.0f; // Identity quaternion

        body.mass = 1.0f;
        body.inertia = 1.0f; // Simplified
        body.restitution = 0.5f;
        body.static_friction = 0.5f;
        body.kinetic_friction = 0.3f;
        body.is_static = false;

        // Randomize initial velocity slightly to break symmetry
        body.linear_velocity[0] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        body.linear_velocity[1] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        body.linear_velocity[2] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;

        if (physics_solver_add_rigid_body(system, &body) < 0) {
            fprintf(stderr, "Warning: Failed to add body %d\n", i);
        } else {
            added_count++;
        }
    }

    printf("Successfully added %d rigid bodies.\n", added_count);

    // Create a static ground plane (infinite mass/static body)
    RigidBody ground;
    memset(&ground, 0, sizeof(RigidBody));
    ground.position[0] = 0.0f;
    ground.position[1] = -5.0f;
    ground.position[2] = 0.0f;
    ground.orientation[3] = 1.0f;
    ground.is_static = true;
    ground.mass = 0.0f; // Infinite mass implies static usually, but is_static flag is better

    physics_solver_add_rigid_body(system, &ground);
    printf("Added ground plane.\n\n");

    printf("Starting simulation (%d frames)...\n", SIMULATION_FRAMES);

    double start_time = get_time_ms();

    double min_frame_time = 1e9;
    double max_frame_time = 0;
    double total_frame_time = 0;

    for (int frame = 0; frame < SIMULATION_FRAMES; frame++) {
        double frame_start = get_time_ms();

        physics_solver_step(system, DT);

        double frame_end = get_time_ms();
        double frame_dt = frame_end - frame_start;

        if (frame_dt < min_frame_time) min_frame_time = frame_dt;
        if (frame_dt > max_frame_time) max_frame_time = frame_dt;
        total_frame_time += frame_dt;

        if ((frame + 1) % 60 == 0) {
            printf("Frame %d/%d completed (avg: %.2f ms)\r", frame + 1, SIMULATION_FRAMES, total_frame_time / (frame + 1));
            fflush(stdout);
        }
    }

    double end_time = get_time_ms();
    printf("\n\n");

    // Gather Stats
    physics_solver_stats stats;
    physics_solver_get_stats(system, &stats);

    double total_time_ms = end_time - start_time;
    double avg_frame_time = total_frame_time / SIMULATION_FRAMES;

    // Output Report
    printf("==============================================================\n");
    printf("PROFILING RESULTS\n");
    printf("==============================================================\n");
    printf("Total Execution Time: %.2f ms\n", total_time_ms);
    printf("Frames Simulated:     %d\n", SIMULATION_FRAMES);
    printf("Body Count:           %d\n", added_count);
    printf("--------------------------------------------------------------\n");
    printf("Average Frame Time:   %.2f ms\n", avg_frame_time);
    printf("Min Frame Time:       %.2f ms\n", min_frame_time);
    printf("Max Frame Time:       %.2f ms\n", max_frame_time);
    printf("--------------------------------------------------------------\n");
    printf("Internal Stats:\n");
    printf("  Rigid Bodies:       %d\n", stats.rigid_body_count);
    printf("  Contacts Solved:    %d\n", stats.contact_count);
    printf("==============================================================\n");

    // Save to file
    FILE *fp = fopen("PHYSICS_PROFILING_REPORT.txt", "w");
    if (fp) {
        fprintf(fp, "PHYSICS SYSTEM PERFORMANCE PROFILING REPORT\n");
        fprintf(fp, "Date: %s", "2024-XX-XX"); // Simplified
        fprintf(fp, "\nConfiguration:\n");
        fprintf(fp, "  Rigid Bodies: %d\n", added_count);
        fprintf(fp, "  Duration: %d frames (%.2f s)\n", SIMULATION_FRAMES, SIMULATION_FRAMES * DT);
        fprintf(fp, "  Time Step: %.4f s\n", DT);
        fprintf(fp, "\nResults:\n");
        fprintf(fp, "  Total Real Time: %.2f ms\n", total_time_ms);
        fprintf(fp, "  Avg Frame Time:  %.2f ms\n", avg_frame_time);
        fprintf(fp, "  Min Frame Time:  %.2f ms\n", min_frame_time);
        fprintf(fp, "  Max Frame Time:  %.2f ms\n", max_frame_time);
        fprintf(fp, "\nPerformance Grade: %s\n", (avg_frame_time < 16.6f) ? "PASS (60+ FPS)" : "FAIL (< 60 FPS)");
        fclose(fp);
        printf("Report saved to PHYSICS_PROFILING_REPORT.txt\n");
    }

    physics_solver_destroy(system);
    return 0;
}
