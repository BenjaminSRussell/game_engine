#include "engine_coverage_test_framework.h"
#include <math.h>
#include <pthread.h>

// Mock physics system structures and functions
typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    float mass;
    float radius;
    bool is_static;
} RigidBody;

typedef struct {
    Vec3 normal;
    float penetration;
    RigidBody *body_a;
    RigidBody *body_b;
} CollisionContact;

typedef struct {
    RigidBody *bodies;
    uint32_t body_count;
    uint32_t max_bodies;
    CollisionContact *contacts;
    uint32_t contact_count;
    uint32_t max_contacts;
    Vec3 gravity;
    float time_step;
    bool is_initialized;
} PhysicsWorld;

static PhysicsWorld g_physics_world = {0};

// Vector math helpers
static Vec3 vec3_create(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

static Vec3 vec3_add(Vec3 a, Vec3 b) {
    return vec3_create(a.x + b.x, a.y + b.y, a.z + b.z);
}

static Vec3 vec3_subtract(Vec3 a, Vec3 b) {
    return vec3_create(a.x - b.x, a.y - b.y, a.z - b.z);
}

static Vec3 vec3_multiply(Vec3 v, float scalar) {
    return vec3_create(v.x * scalar, v.y * scalar, v.z * scalar);
}

static float vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static float vec3_distance(Vec3 a, Vec3 b) {
    return vec3_length(vec3_subtract(a, b));
}

// Mock physics functions
bool physics_world_init(uint32_t max_bodies, uint32_t max_contacts) {
    if (g_physics_world.is_initialized) {
        return false;
    }
    
    g_physics_world.bodies = MALLOC(sizeof(RigidBody) * max_bodies);
    if (!g_physics_world.bodies) {
        return false;
    }
    
    g_physics_world.contacts = MALLOC(sizeof(CollisionContact) * max_contacts);
    if (!g_physics_world.contacts) {
        FREE(g_physics_world.bodies);
        return false;
    }
    
    g_physics_world.max_bodies = max_bodies;
    g_physics_world.max_contacts = max_contacts;
    g_physics_world.body_count = 0;
    g_physics_world.contact_count = 0;
    g_physics_world.gravity = vec3_create(0.0f, -9.81f, 0.0f);
    g_physics_world.time_step = 1.0f / 60.0f;
    g_physics_world.is_initialized = true;
    
    return true;
}

void physics_world_shutdown(void) {
    if (g_physics_world.bodies) {
        FREE(g_physics_world.bodies);
        g_physics_world.bodies = NULL;
    }
    
    if (g_physics_world.contacts) {
        FREE(g_physics_world.contacts);
        g_physics_world.contacts = NULL;
    }
    
    g_physics_world.is_initialized = false;
}

RigidBody* physics_world_create_body(Vec3 position, float mass, float radius, bool is_static) {
    if (!g_physics_world.is_initialized || g_physics_world.body_count >= g_physics_world.max_bodies) {
        return NULL;
    }
    
    RigidBody *body = &g_physics_world.bodies[g_physics_world.body_count];
    body->position = position;
    body->velocity = vec3_create(0.0f, 0.0f, 0.0f);
    body->acceleration = vec3_create(0.0f, 0.0f, 0.0f);
    body->mass = mass;
    body->radius = radius;
    body->is_static = is_static;
    
    g_physics_world.body_count++;
    return body;
}

void physics_world_destroy_body(RigidBody *body) {
    if (!body || !g_physics_world.is_initialized) {
        return;
    }
    
    // Find the body in the array and remove it
    for (uint32_t i = 0; i < g_physics_world.body_count; i++) {
        if (&g_physics_world.bodies[i] == body) {
            // Move the last body to this position
            if (i < g_physics_world.body_count - 1) {
                g_physics_world.bodies[i] = g_physics_world.bodies[g_physics_world.body_count - 1];
            }
            g_physics_world.body_count--;
            break;
        }
    }
}

void physics_world_set_gravity(Vec3 gravity) {
    g_physics_world.gravity = gravity;
}

void physics_world_set_time_step(float time_step) {
    g_physics_world.time_step = time_step;
}

void physics_world_step(void) {
    if (!g_physics_world.is_initialized) {
        return;
    }
    
    // Clear contacts
    g_physics_world.contact_count = 0;
    
    // Update physics for each body
    for (uint32_t i = 0; i < g_physics_world.body_count; i++) {
        RigidBody *body = &g_physics_world.bodies[i];
        
        if (!body->is_static) {
            // Apply gravity
            body->acceleration = g_physics_world.gravity;
            
            // Update velocity
            body->velocity = vec3_add(body->velocity, vec3_multiply(body->acceleration, g_physics_world.time_step));
            
            // Update position
            body->position = vec3_add(body->position, vec3_multiply(body->velocity, g_physics_world.time_step));
        }
    }
    
    // Simple collision detection
    for (uint32_t i = 0; i < g_physics_world.body_count && g_physics_world.contact_count < g_physics_world.max_contacts; i++) {
        for (uint32_t j = i + 1; j < g_physics_world.body_count && g_physics_world.contact_count < g_physics_world.max_contacts; j++) {
            RigidBody *body_a = &g_physics_world.bodies[i];
            RigidBody *body_b = &g_physics_world.bodies[j];
            
            float distance = vec3_distance(body_a->position, body_b->position);
            float combined_radius = body_a->radius + body_b->radius;
            
            if (distance < combined_radius) {
                CollisionContact *contact = &g_physics_world.contacts[g_physics_world.contact_count];
                contact->body_a = body_a;
                contact->body_b = body_b;
                contact->penetration = combined_radius - distance;
                
                // Calculate collision normal
                Vec3 diff = vec3_subtract(body_b->position, body_a->position);
                if (distance > 0.0f) {
                    contact->normal = vec3_multiply(diff, 1.0f / distance);
                } else {
                    contact->normal = vec3_create(0.0f, 1.0f, 0.0f); // Default up
                }
                
                g_physics_world.contact_count++;
            }
        }
    }
    
    // Simple collision response
    for (uint32_t i = 0; i < g_physics_world.contact_count; i++) {
        CollisionContact *contact = &g_physics_world.contacts[i];
        RigidBody *body_a = contact->body_a;
        RigidBody *body_b = contact->body_b;
        
        if (!body_a->is_static && !body_b->is_static) {
            // Separate bodies
            Vec3 separation = vec3_multiply(contact->normal, contact->penetration * 0.5f);
            body_a->position = vec3_subtract(body_a->position, separation);
            body_b->position = vec3_add(body_b->position, separation);
            
            // Exchange velocities (simplified)
            Vec3 temp = body_a->velocity;
            body_a->velocity = vec3_multiply(body_b->velocity, 0.8f); // Energy loss
            body_b->velocity = vec3_multiply(temp, 0.8f);
        } else if (!body_a->is_static) {
            // Move body_a out of collision
            Vec3 separation = vec3_multiply(contact->normal, contact->penetration);
            body_a->position = vec3_subtract(body_a->position, separation);
            body_a->velocity = vec3_multiply(body_a->velocity, -0.8f); // Bounce
        } else if (!body_b->is_static) {
            // Move body_b out of collision
            Vec3 separation = vec3_multiply(contact->normal, contact->penetration);
            body_b->position = vec3_add(body_b->position, separation);
            body_b->velocity = vec3_multiply(body_b->velocity, -0.8f); // Bounce
        }
    }
}

// Physics system tests
static bool test_physics_world_initialization(void) {
    // Test initialization with valid parameters
    bool result = physics_world_init(1000, 2000);
    ASSERT_TRUE(result);
    ASSERT_TRUE(g_physics_world.is_initialized);
    ASSERT_EQ(1000, g_physics_world.max_bodies);
    ASSERT_EQ(2000, g_physics_world.max_contacts);
    ASSERT_EQ(0, g_physics_world.body_count);
    ASSERT_EQ(0, g_physics_world.contact_count);
    ASSERT_NOT_NULL(g_physics_world.bodies);
    ASSERT_NOT_NULL(g_physics_world.contacts);
    ASSERT_FLOAT_EQ(-9.81f, g_physics_world.gravity.y);
    ASSERT_FLOAT_EQ(1.0f / 60.0f, g_physics_world.time_step);
    
    // Test double initialization
    result = physics_world_init(500, 1000);
    ASSERT_FALSE(result);
    
    physics_world_shutdown();
    return true;
}

static bool test_physics_world_shutdown(void) {
    // Initialize first
    ASSERT_TRUE(physics_world_init(1000, 2000));
    
    // Test shutdown
    physics_world_shutdown();
    ASSERT_FALSE(g_physics_world.is_initialized);
    ASSERT_NULL(g_physics_world.bodies);
    ASSERT_NULL(g_physics_world.contacts);
    
    // Test shutdown when not initialized (should be safe)
    physics_world_shutdown();
    
    return true;
}

static bool test_rigid_body_creation(void) {
    ASSERT_TRUE(physics_world_init(100, 100));
    
    Vec3 position = vec3_create(0.0f, 10.0f, 0.0f);
    RigidBody *body = physics_world_create_body(position, 1.0f, 1.0f, false);
    
    ASSERT_NOT_NULL(body);
    ASSERT_FLOAT_EQ(position.x, body->position.x);
    ASSERT_FLOAT_EQ(position.y, body->position.y);
    ASSERT_FLOAT_EQ(position.z, body->position.z, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, body->velocity.x, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, body->velocity.y, 0.0001f);
    ASSERT_FLOAT_EQ(0.0f, body->velocity.z, 0.0001f);
    ASSERT_FLOAT_EQ(1.0f, body->mass);
    ASSERT_FLOAT_EQ(1.0f, body->radius);
    ASSERT_FALSE(body->is_static);
    ASSERT_EQ(1, g_physics_world.body_count);
    
    physics_world_shutdown();
    return true;
}

static bool test_rigid_body_creation_without_world(void) {
    // Test body creation when physics world is not initialized
    Vec3 position = vec3_create(0.0f, 0.0f, 0.0f);
    RigidBody *body = physics_world_create_body(position, 1.0f, 1.0f, false);
    ASSERT_NULL(body);
    
    return true;
}

static bool test_static_vs_dynamic_bodies(void) {
    ASSERT_TRUE(physics_world_init(100, 100));
    
    // Create static body
    Vec3 static_pos = vec3_create(0.0f, 0.0f, 0.0f);
    RigidBody *static_body = physics_world_create_body(static_pos, 0.0f, 2.0f, true);
    ASSERT_NOT_NULL(static_body);
    ASSERT_TRUE(static_body->is_static);
    ASSERT_FLOAT_EQ(0.0f, static_body->mass);
    
    // Create dynamic body
    Vec3 dynamic_pos = vec3_create(0.0f, 10.0f, 0.0f);
    RigidBody *dynamic_body = physics_world_create_body(dynamic_pos, 1.0f, 1.0f, false);
    ASSERT_NOT_NULL(dynamic_body);
    ASSERT_FALSE(dynamic_body->is_static);
    ASSERT_FLOAT_EQ(1.0f, dynamic_body->mass);
    
    physics_world_shutdown();
    return true;
}

static bool test_gravity_simulation(void) {
    ASSERT_TRUE(physics_world_init(100, 100));
    
    Vec3 position = vec3_create(0.0f, 10.0f, 0.0f);
    RigidBody *body = physics_world_create_body(position, 1.0f, 1.0f, false);
    ASSERT_NOT_NULL(body);
    
    float initial_y = body->position.y;
    
    // Simulate one time step
    physics_world_step();
    
    // Body should have fallen due to gravity
    ASSERT_TRUE(body->position.y < initial_y);
    ASSERT_TRUE(body->velocity.y < 0.0f); // Should be moving downward
    
    physics_world_shutdown();
    return true;
}

static bool test_collision_detection(void) {
    ASSERT_TRUE(physics_world_init(100, 100));
    
    // Create two bodies that should collide
    Vec3 pos_a = vec3_create(0.0f, 5.0f, 0.0f);
    Vec3 pos_b = vec3_create(0.0f, 3.0f, 0.0f);
    RigidBody *body_a = physics_world_create_body(pos_a, 1.0f, 2.0f, false);
    RigidBody *body_b = physics_world_create_body(pos_b, 1.0f, 2.0f, false);
    
    ASSERT_NOT_NULL(body_a);
    ASSERT_NOT_NULL(body_b);
    
    // Bodies are overlapping (distance 2, combined radius 4)
    physics_world_step();
    
    // Should have detected collision
    ASSERT_TRUE(g_physics_world.contact_count > 0);
    
    physics_world_shutdown();
    return true;
}

static bool test_collision_response(void) {
    ASSERT_TRUE(physics_world_init(100, 100));
    
    // Create two bodies moving towards each other
    Vec3 pos_a = vec3_create(-5.0f, 0.0f, 0.0f);
    Vec3 pos_b = vec3_create(5.0f, 0.0f, 0.0f);
    RigidBody *body_a = physics_world_create_body(pos_a, 1.0f, 2.0f, false);
    RigidBody *body_b = physics_world_create_body(pos_b, 1.0f, 2.0f, false);
    
    ASSERT_NOT_NULL(body_a);
    ASSERT_NOT_NULL(body_b);
    
    // Give them initial velocities towards each other
    body_a->velocity = vec3_create(2.0f, 0.0f, 0.0f);
    body_b->velocity = vec3_create(-2.0f, 0.0f, 0.0f);
    
    float initial_vel_a_x = body_a->velocity.x;
    float initial_vel_b_x = body_b->velocity.x;
    
    // Step simulation
    physics_world_step();
    
    // After collision, velocities should have changed (bounced)
    ASSERT_TRUE(body_a->velocity.x != initial_vel_a_x);
    ASSERT_TRUE(body_b->velocity.x != initial_vel_b_x);
    
    physics_world_shutdown();
    return true;
}

static bool test_body_destruction(void) {
    ASSERT_TRUE(physics_world_init(100, 100));
    
    Vec3 position = vec3_create(0.0f, 0.0f, 0.0f);
    RigidBody *body = physics_world_create_body(position, 1.0f, 1.0f, false);
    ASSERT_NOT_NULL(body);
    ASSERT_EQ(1, g_physics_world.body_count);
    
    // Destroy the body
    physics_world_destroy_body(body);
    ASSERT_EQ(0, g_physics_world.body_count);
    
    // Test destroying NULL body
    physics_world_destroy_body(NULL);
    ASSERT_EQ(0, g_physics_world.body_count);
    
    physics_world_shutdown();
    return true;
}

static bool test_physics_performance(void) {
    ASSERT_TRUE(physics_world_init(1000, 2000));
    
    const int num_bodies = 500;
    RigidBody *bodies[num_bodies];
    
    uint64_t start_time, end_time, total_time;
    
    // Measure body creation time
    MEASURE_PERFORMANCE_START();
    for (int i = 0; i < num_bodies; i++) {
        Vec3 pos = vec3_create((float)(i % 10), (float)(i / 10), 0.0f);
        bodies[i] = physics_world_create_body(pos, 1.0f, 0.5f, false);
        ASSERT_NOT_NULL(bodies[i]);
    }
    MEASURE_PERFORMANCE_END(&total_time);
    
    double creation_time_ms = (double)total_time / 1000000.0;
    log_test_message("INFO", "PHYSICS", "Created %d bodies in %.2f ms", num_bodies, creation_time_ms);
    
    // Measure simulation time
    MEASURE_PERFORMANCE_START();
    for (int step = 0; step < 60; step++) { // 1 second at 60 FPS
        physics_world_step();
    }
    MEASURE_PERFORMANCE_END(&total_time);
    
    double simulation_time_ms = (double)total_time / 1000000.0;
    log_test_message("INFO", "PHYSICS", "Simulated 60 steps with %d bodies in %.2f ms", 
                    num_bodies, simulation_time_ms);
    
    // Cleanup
    for (int i = 0; i < num_bodies; i++) {
        physics_world_destroy_body(bodies[i]);
    }
    
    physics_world_shutdown();
    return true;
}

// Thread safety test
static void* physics_thread_function(void *arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < 100; i++) {
        Vec3 pos = vec3_create((float)thread_id, (float)i, 0.0f);
        RigidBody *body = physics_world_create_body(pos, 1.0f, 0.5f, false);
        
        if (body) {
            physics_world_step();
            physics_world_destroy_body(body);
        }
        
        usleep(100);
    }
    
    return NULL;
}

static bool test_physics_thread_safety(void) {
    ASSERT_TRUE(physics_world_init(1000, 2000));
    
    pthread_t threads[5];
    int thread_ids[5];
    
    // Create multiple threads
    for (int i = 0; i < 5; i++) {
        thread_ids[i] = i;
        int result = pthread_create(&threads[i], NULL, physics_thread_function, &thread_ids[i]);
        ASSERT_EQ(0, result);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    physics_world_shutdown();
    ASSERT_NO_MEMORY_LEAKS();
    return true;
}

// Main physics test suite
TestSuite engine_run_physics_tests(void) {
    TestSuite suite = {0};
    strcpy(suite.suite_name, "Physics Tests");
    
    const int test_count = 10;
    suite.tests = MALLOC(sizeof(TestResult) * test_count);
    suite.test_count = test_count;
    
    TestCase tests[] = {
        {"World Initialization", test_physics_world_initialization, "Test physics world initialization"},
        {"World Shutdown", test_physics_world_shutdown, "Test physics world shutdown"},
        {"Rigid Body Creation", test_rigid_body_creation, "Test rigid body creation"},
        {"Body Creation Without World", test_rigid_body_creation_without_world, "Test body creation without initialized world"},
        {"Static vs Dynamic Bodies", test_static_vs_dynamic_bodies, "Test static and dynamic body creation"},
        {"Gravity Simulation", test_gravity_simulation, "Test gravity simulation"},
        {"Collision Detection", test_collision_detection, "Test collision detection"},
        {"Collision Response", test_collision_response, "Test collision response"},
        {"Body Destruction", test_body_destruction, "Test rigid body destruction"},
        {"Performance", test_physics_performance, "Test physics system performance"}
    };
    
    suite.total_execution_time_ms = get_time_ms();
    
    for (int i = 0; i < test_count; i++) {
        TestResult *result = &suite.tests[i];
        strcpy(result->test_name, tests[i].name);
        strcpy(result->suite_name, suite.suite_name);
        result->assertions_run = 0;
        result->assertions_passed = 0;
        result->error_message[0] = '\0';
        
        g_current_test = result;
        
        double test_start = get_time_ms();
        result->passed = tests[i].function();
        result->execution_time_ms = get_time_ms() - test_start;
        
        if (result->passed) {
            suite.tests_passed++;
            log_test_message("PASS", suite.suite_name, "%s (%.2f ms, %u assertions)", 
                           result->test_name, result->execution_time_ms, result->assertions_passed);
        } else {
            suite.tests_failed++;
            log_test_message("FAIL", suite.suite_name, "%s (%.2f ms): %s", 
                           result->test_name, result->execution_time_ms, result->error_message);
        }
    }
    
    suite.total_execution_time_ms = get_time_ms() - suite.total_execution_time_ms;
    suite.tests_skipped = 0;
    
    g_current_test = NULL;
    
    return suite;
}
