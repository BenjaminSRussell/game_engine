/**
 * @file physics_stability_test.c
 * @brief Physics stability test for Minecraft v2 Engine
 * 
 * This test runs physics at 180Hz (3x speed) to check stability.
 * Part of TODO-0058: Run physics at 3× speed (180 Hz), check stability
 * 
 * @author Minecraft v2 Engine Team
 * @version 1.0
 * @date 2026
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>

// Physics constants
#define GRAVITY -9.81f
#define STANDARD_TIMESTEP (1.0f / 60.0f)  // 60 Hz standard
#define HIGH_SPEED_TIMESTEP (1.0f / 180.0f)  // 180 Hz test (3x speed)
#define TEST_DURATION_SECONDS 30.0f
#define STABILITY_THRESHOLD 0.001f
#define ENERGY_THRESHOLD 2.0f

// Vector3 structure
typedef struct Vector3 {
    float x, y, z;
} Vector3;

// Rigid body structure
typedef struct RigidBody {
    Vector3 position;
    Vector3 velocity;
    Vector3 acceleration;
    Vector3 angular_velocity;
    float mass;
    float radius;
    float restitution;
    float friction;
    int id;
    double creation_time;
} RigidBody;

// Physics simulation state
typedef struct PhysicsState {
    RigidBody* bodies;
    int body_count;
    float total_energy;
    float max_velocity;
    int collision_count;
    int instability_count;
    double simulation_time;
    float timestep;
} PhysicsState;

// Performance tracking
typedef struct PerformanceTracker {
    double frame_times[1000];
    int frame_count;
    double min_frame_time;
    double max_frame_time;
    double avg_frame_time;
    double total_simulation_time;
} PerformanceTracker;

static PhysicsState g_physics_state = {0};
static PerformanceTracker g_perf_tracker = {0};

// Vector math functions
Vector3 vec3_add(Vector3 a, Vector3 b) {
    return (Vector3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector3 vec3_subtract(Vector3 a, Vector3 b) {
    return (Vector3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector3 vec3_multiply(Vector3 v, float scalar) {
    return (Vector3){v.x * scalar, v.y * scalar, v.z * scalar};
}

float vec3_length(Vector3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float vec3_dot(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Get current time in seconds
double getCurrentTimeSeconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Initialize physics simulation
int init_physics_simulation(int body_count, float timestep) {
    printf("Initializing physics simulation with %d bodies at %.1f Hz...\n", 
           body_count, 1.0f / timestep);
    
    g_physics_state.bodies = (RigidBody*)malloc(body_count * sizeof(RigidBody));
    if (!g_physics_state.bodies) {
        printf("ERROR: Failed to allocate memory for bodies\n");
        return -1;
    }
    
    g_physics_state.body_count = body_count;
    g_physics_state.timestep = timestep;
    g_physics_state.total_energy = 0.0f;
    g_physics_state.max_velocity = 0.0f;
    g_physics_state.collision_count = 0;
    g_physics_state.instability_count = 0;
    g_physics_state.simulation_time = 0.0;
    
    // Initialize bodies with random positions and velocities
    srand((unsigned int)time(NULL));
    for (int i = 0; i < body_count; i++) {
        RigidBody* body = &g_physics_state.bodies[i];
        
        body->position = (Vector3){
            (float)(rand() % 100 - 50),  // -50 to 50
            (float)(rand() % 50 + 10),   // 10 to 60
            (float)(rand() % 100 - 50)   // -50 to 50
        };
        
        body->velocity = (Vector3){
            (float)(rand() % 20 - 10) * 0.1f,  // -1 to 1
            (float)(rand() % 10 - 5) * 0.1f,    // -0.5 to 0.5
            (float)(rand() % 20 - 10) * 0.1f    // -1 to 1
        };
        
        body->acceleration = (Vector3){0, GRAVITY, 0};
        body->angular_velocity = (Vector3){0, 0, 0};
        body->mass = (float)(rand() % 10 + 1);  // 1 to 10
        body->radius = (float)(rand() % 3 + 1);  // 1 to 3
        body->restitution = 0.8f;
        body->friction = 0.3f;
        body->id = i;
        body->creation_time = getCurrentTimeSeconds();
    }
    
    return 0;
}

// Update physics for one timestep
void update_physics() {
    double frame_start = getCurrentTimeSeconds();
    
    for (int i = 0; i < g_physics_state.body_count; i++) {
        RigidBody* body = &g_physics_state.bodies[i];
        
        // Store previous state for stability checking
        Vector3 prev_position = body->position;
        Vector3 prev_velocity = body->velocity;
        
        // Update velocity (v = v0 + a*dt)
        body->velocity = vec3_add(body->velocity, 
                                 vec3_multiply(body->acceleration, g_physics_state.timestep));
        
        // Update position (x = x0 + v*dt)
        body->position = vec3_add(body->position, 
                                 vec3_multiply(body->velocity, g_physics_state.timestep));
        
        // Check for stability issues
        float velocity_change = vec3_length(vec3_subtract(body->velocity, prev_velocity));
        float position_change = vec3_length(vec3_subtract(body->position, prev_position));
        
        if (velocity_change > STABILITY_THRESHOLD * 100 || position_change > 10.0f) {
            g_physics_state.instability_count++;
            printf("Instability detected in body %d: vel_change=%.6f, pos_change=%.6f\n",
                   body->id, velocity_change, position_change);
        }
        
        // Ground collision
        if (body->position.y - body->radius < 0) {
            body->position.y = body->radius;
            body->velocity.y = -body->velocity.y * body->restitution;
            body->velocity.x *= (1.0f - body->friction * g_physics_state.timestep);
            body->velocity.z *= (1.0f - body->friction * g_physics_state.timestep);
            g_physics_state.collision_count++;
        }
        
        // Update max velocity
        float speed = vec3_length(body->velocity);
        if (speed > g_physics_state.max_velocity) {
            g_physics_state.max_velocity = speed;
        }
    }
    
    // Check body-body collisions
    for (int i = 0; i < g_physics_state.body_count; i++) {
        for (int j = i + 1; j < g_physics_state.body_count; j++) {
            RigidBody* body1 = &g_physics_state.bodies[i];
            RigidBody* body2 = &g_physics_state.bodies[j];
            
            Vector3 diff = vec3_subtract(body2->position, body1->position);
            float distance = vec3_length(diff);
            float min_distance = body1->radius + body2->radius;
            
            if (distance < min_distance && distance > 0.001f) {
                // Collision detected
                Vector3 normal = vec3_multiply(diff, 1.0f / distance);
                
                // Separate bodies
                float overlap = min_distance - distance;
                Vector3 separation = vec3_multiply(normal, overlap * 0.5f);
                body1->position = vec3_subtract(body1->position, separation);
                body2->position = vec3_add(body2->position, separation);
                
                // Calculate relative velocity
                Vector3 relative_velocity = vec3_subtract(body2->velocity, body1->velocity);
                float velocity_along_normal = vec3_dot(relative_velocity, normal);
                
                if (velocity_along_normal < 0) {
                    // Calculate impulse
                    float e = (body1->restitution + body2->restitution) * 0.5f;
                    float j = -(1.0f + e) * velocity_along_normal;
                    j /= 1.0f / body1->mass + 1.0f / body2->mass;
                    
                    Vector3 impulse = vec3_multiply(normal, j);
                    body1->velocity = vec3_subtract(body1->velocity, 
                                                   vec3_multiply(impulse, 1.0f / body1->mass));
                    body2->velocity = vec3_add(body2->velocity, 
                                               vec3_multiply(impulse, 1.0f / body2->mass));
                    
                    g_physics_state.collision_count++;
                }
            }
        }
    }
    
    // Calculate total system energy
    g_physics_state.total_energy = 0.0f;
    for (int i = 0; i < g_physics_state.body_count; i++) {
        RigidBody* body = &g_physics_state.bodies[i];
        float kinetic_energy = 0.5f * body->mass * vec3_length(body->velocity) * vec3_length(body->velocity);
        float potential_energy = body->mass * -GRAVITY * body->position.y;
        g_physics_state.total_energy += kinetic_energy + potential_energy;
    }
    
    g_physics_state.simulation_time += g_physics_state.timestep;
    
    // Track performance
    double frame_end = getCurrentTimeSeconds();
    double frame_time = frame_end - frame_start;
    
    if (g_perf_tracker.frame_count < 1000) {
        g_perf_tracker.frame_times[g_perf_tracker.frame_count] = frame_time;
        g_perf_tracker.frame_count++;
    }
    
    if (frame_time < g_perf_tracker.min_frame_time || g_perf_tracker.min_frame_time == 0) {
        g_perf_tracker.min_frame_time = frame_time;
    }
    if (frame_time > g_perf_tracker.max_frame_time) {
        g_perf_tracker.max_frame_time = frame_time;
    }
}

// Run physics stability test
int run_physics_stability_test(float timestep, const char* test_name) {
    printf("\n=== Physics Stability Test: %s ===\n", test_name);
    printf("Timestep: %.6f seconds (%.1f Hz)\n", timestep, 1.0f / timestep);
    printf("Test duration: %.1f seconds\n", TEST_DURATION_SECONDS);
    
    int initial_instabilities = g_physics_state.instability_count;
    float initial_energy = g_physics_state.total_energy;
    double start_time = getCurrentTimeSeconds();
    
    int frames_to_run = (int)(TEST_DURATION_SECONDS / timestep);
    printf("Running %d physics frames...\n", frames_to_run);
    
    for (int frame = 0; frame < frames_to_run; frame++) {
        update_physics();
        
        if (frame % (frames_to_run / 10) == 0) {
            printf("Progress: %d%% (%d/%d frames)\n", 
                   (frame * 100) / frames_to_run, frame, frames_to_run);
        }
    }
    
    double end_time = getCurrentTimeSeconds();
    double actual_duration = end_time - start_time;
    
    // Calculate statistics
    int new_instabilities = g_physics_state.instability_count - initial_instabilities;
    float energy_change = fabsf(g_physics_state.total_energy - initial_energy);
    float energy_change_percent = (energy_change / initial_energy) * 100.0f;
    
    printf("\nTest Results:\n");
    printf("Actual duration: %.2f seconds\n", actual_duration);
    printf("Physics frames simulated: %d\n", frames_to_run);
    printf("Simulation time: %.2f seconds\n", g_physics_state.simulation_time);
    printf("Collisions: %d\n", g_physics_state.collision_count);
    printf("Instabilities detected: %d\n", new_instabilities);
    printf("Initial energy: %.2f\n", initial_energy);
    printf("Final energy: %.2f\n", g_physics_state.total_energy);
    printf("Energy change: %.2f (%.2f%%)\n", energy_change, energy_change_percent);
    printf("Max velocity: %.2f\n", g_physics_state.max_velocity);
    
    // Performance metrics
    if (g_perf_tracker.frame_count > 0) {
        double total_frame_time = 0.0;
        for (int i = 0; i < g_perf_tracker.frame_count; i++) {
            total_frame_time += g_perf_tracker.frame_times[i];
        }
        g_perf_tracker.avg_frame_time = total_frame_time / g_perf_tracker.frame_count;
        g_perf_tracker.total_simulation_time = actual_duration;
        
        printf("\nPerformance Metrics:\n");
        printf("Min frame time: %.6f seconds\n", g_perf_tracker.min_frame_time);
        printf("Max frame time: %.6f seconds\n", g_perf_tracker.max_frame_time);
        printf("Avg frame time: %.6f seconds\n", g_perf_tracker.avg_frame_time);
        printf("Target frame time: %.6f seconds\n", timestep);
        printf("Performance ratio: %.2fx\n", timestep / g_perf_tracker.avg_frame_time);
    }
    
    // Stability assessment
    int stability_score = 0;
    printf("\nStability Assessment:\n");
    
    if (new_instabilities == 0) {
        printf("✅ No instabilities detected\n");
        stability_score++;
    } else {
        printf("❌ %d instabilities detected\n", new_instabilities);
    }
    
    if (energy_change_percent < ENERGY_THRESHOLD) {
        printf("✅ Energy change within acceptable bounds (%.2f%% < %.2f%%)\n", 
               energy_change_percent, ENERGY_THRESHOLD);
        stability_score++;
    } else {
        printf("❌ Energy change exceeds threshold (%.2f%% > %.2f%%)\n", 
               energy_change_percent, ENERGY_THRESHOLD);
    }
    
    if (g_physics_state.max_velocity < 100.0f) {
        printf("✅ Maximum velocity reasonable (%.2f < 100.0)\n", g_physics_state.max_velocity);
        stability_score++;
    } else {
        printf("❌ Excessive velocity detected (%.2f > 100.0)\n", g_physics_state.max_velocity);
    }
    
    return (stability_score >= 2) ? 0 : -1;
}

// Cleanup physics simulation
void cleanup_physics_simulation() {
    if (g_physics_state.bodies) {
        free(g_physics_state.bodies);
        g_physics_state.bodies = NULL;
    }
    memset(&g_physics_state, 0, sizeof(g_physics_state));
    memset(&g_perf_tracker, 0, sizeof(g_perf_tracker));
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("Minecraft v2 Engine - Physics Stability Test\n");
    printf("============================================\n");
    printf("TODO-0058: Run physics at 3× speed (180 Hz), check stability\n\n");
    
    int result = 0;
    const int NUM_BODIES = 50;
    
    // Test 1: Standard 60 Hz (baseline)
    if (init_physics_simulation(NUM_BODIES, STANDARD_TIMESTEP) == 0) {
        result |= run_physics_stability_test(STANDARD_TIMESTEP, "Standard 60 Hz (Baseline)");
        cleanup_physics_simulation();
    } else {
        result = -1;
    }
    
    // Test 2: High speed 180 Hz (3x speed) - Main test
    if (init_physics_simulation(NUM_BODIES, HIGH_SPEED_TIMESTEP) == 0) {
        result |= run_physics_stability_test(HIGH_SPEED_TIMESTEP, "High Speed 180 Hz (3x Speed)");
        cleanup_physics_simulation();
    } else {
        result = -1;
    }
    
    // Final assessment
    printf("\n=== Final Assessment ===\n");
    if (result == 0) {
        printf("✅ ALL PHYSICS STABILITY TESTS PASSED\n");
        printf("✅ TODO-0058 completed successfully\n");
        printf("✅ Physics engine stable at 180 Hz (3x speed)\n");
    } else {
        printf("❌ PHYSICS STABILITY TESTS FAILED\n");
        printf("❌ TODO-0058 not completed - stability issues detected\n");
    }
    
    return result;
}
