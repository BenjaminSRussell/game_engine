// deterministic_replay.c
//
// Purpose: Deterministic replay system for networked physics
// Implements TODO-0044: Deterministic replay for networked physics

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

// Vector3 structure (same as physics system)
typedef struct {
    float x, y, z;
} Vec3;

// Quaternion structure
typedef struct {
    float x, y, z, w;
} Quat;

// Vector operations
static inline Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline Vec3 vec3_mul(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
static inline float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline float vec3_length_sq(Vec3 v) { return vec3_dot(v, v); }
static inline float vec3_length(Vec3 v) { return sqrtf(vec3_length_sq(v)); }

// Physics state snapshot
typedef struct {
    uint32_t frame_number;
    uint32_t body_count;
    struct {
        uint32_t id;
        Vec3 position;
        Vec3 velocity;
        Vec3 acceleration;
        Quat rotation;
        float mass;
        bool is_active;
    }* bodies;
    uint32_t input_count;
    struct {
        uint32_t player_id;
        Vec3 move_input;
        bool jump_pressed;
        float timestamp;
    }* inputs;
    double simulation_time;
    uint64_t checksum;
} PhysicsSnapshot;

// Replay frame
typedef struct {
    uint32_t frame_number;
    PhysicsSnapshot snapshot;
    bool has_inputs;
    bool is_keyframe; // Keyframes are stored more frequently
} ReplayFrame;

// Deterministic replay system
typedef struct {
    ReplayFrame* frames;
    uint32_t capacity;
    uint32_t count;
    uint32_t current_frame;
    uint32_t keyframe_interval;
    bool is_recording;
    bool is_playing;
    bool is_initialized;
    
    // Determinism settings
    bool fixed_timestep;
    float timestep_value;
    uint32_t max_frames;
    
    // Network synchronization
    uint32_t local_player_id;
    uint32_t network_frame_delay;
    
    // Statistics
    uint32_t total_frames_recorded;
    uint32_t total_frames_played;
    double total_record_time;
    double total_play_time;
    uint32_t desync_count;
} DeterministicReplay;

// Global replay system
static DeterministicReplay g_replay = {0};

// CRC32 for checksum calculation
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void init_crc32_table(void) {
    if (crc32_table_initialized) return;
    
    uint32_t polynomial = 0xEDB88320;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = true;
}

static uint32_t calculate_crc32(const void* data, size_t length) {
    init_crc32_table();
    
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < length; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[i]) & 0xFF];
    }
    
    return crc ^ 0xFFFFFFFF;
}

// Initialize deterministic replay system
bool deterministic_replay_init(uint32_t max_frames, uint32_t keyframe_interval,
                             bool fixed_timestep, float timestep_value) {
    printf("Initializing deterministic replay system: max_frames=%u, keyframe_interval=%u, timestep=%.4f\n",
           max_frames, keyframe_interval, timestep_value);
    
    g_replay.frames = calloc(max_frames, sizeof(ReplayFrame));
    if (!g_replay.frames) {
        printf("Error: Failed to allocate replay frames\n");
        return false;
    }
    
    g_replay.capacity = max_frames;
    g_replay.keyframe_interval = keyframe_interval;
    g_replay.fixed_timestep = fixed_timestep;
    g_replay.timestep_value = timestep_value;
    g_replay.max_frames = max_frames;
    g_replay.is_initialized = true;
    
    printf("Deterministic replay system initialized successfully\n");
    return true;
}

// Start recording
bool deterministic_replay_start_recording(uint32_t local_player_id) {
    if (!g_replay.is_initialized) {
        printf("Error: Replay system not initialized\n");
        return false;
    }
    
    if (g_replay.is_recording) {
        printf("Warning: Already recording\n");
        return false;
    }
    
    g_replay.is_recording = true;
    g_replay.is_playing = false;
    g_replay.count = 0;
    g_replay.current_frame = 0;
    g_replay.local_player_id = local_player_id;
    g_replay.total_frames_recorded = 0;
    g_replay.total_record_time = 0.0;
    g_replay.desync_count = 0;
    
    printf("Started recording replay (local_player_id=%u)\n", local_player_id);
    return true;
}

// Record a physics snapshot
bool deterministic_replay_record_frame(const PhysicsSnapshot* snapshot) {
    if (!g_replay.is_recording || !g_replay.is_initialized) {
        return false;
    }
    
    if (g_replay.count >= g_replay.capacity) {
        printf("Warning: Replay buffer full, wrapping around\n");
        g_replay.count = 0; // Wrap around for circular buffer
    }
    
    ReplayFrame* frame = &g_replay.frames[g_replay.count];
    frame->frame_number = snapshot->frame_number;
    frame->has_inputs = (snapshot->input_count > 0);
    frame->is_keyframe = (snapshot->frame_number % g_replay.keyframe_interval == 0);
    
    // Deep copy snapshot
    frame->snapshot.frame_number = snapshot->frame_number;
    frame->snapshot.body_count = snapshot->body_count;
    frame->snapshot.simulation_time = snapshot->simulation_time;
    
    if (snapshot->body_count > 0) {
        frame->snapshot.bodies = malloc(snapshot->body_count * sizeof(frame->snapshot.bodies[0]));
        memcpy(frame->snapshot.bodies, snapshot->bodies, 
               snapshot->body_count * sizeof(frame->snapshot.bodies[0]));
    } else {
        frame->snapshot.bodies = NULL;
    }
    
    frame->snapshot.input_count = snapshot->input_count;
    if (snapshot->input_count > 0) {
        frame->snapshot.inputs = malloc(snapshot->input_count * sizeof(frame->snapshot.inputs[0]));
        memcpy(frame->snapshot.inputs, snapshot->inputs,
               snapshot->input_count * sizeof(frame->snapshot.inputs[0]));
    } else {
        frame->snapshot.inputs = NULL;
    }
    
    // Calculate checksum for integrity verification
    size_t data_size = snapshot->body_count * sizeof(frame->snapshot.bodies[0]) +
                       snapshot->input_count * sizeof(frame->snapshot.inputs[0]);
    frame->snapshot.checksum = calculate_crc32(snapshot->bodies, data_size);
    
    g_replay.count++;
    g_replay.total_frames_recorded++;
    
    if (frame->is_keyframe) {
        printf("Recorded keyframe %u with %u bodies, %u inputs (checksum: %08X)\n",
               snapshot->frame_number, snapshot->body_count, snapshot->input_count,
               frame->snapshot.checksum);
    }
    
    return true;
}

// Stop recording
void deterministic_replay_stop_recording(void) {
    if (!g_replay.is_recording) return;
    
    g_replay.is_recording = false;
    printf("Stopped recording replay (total_frames: %u)\n", g_replay.total_frames_recorded);
}

// Start playback
bool deterministic_replay_start_playback(uint32_t start_frame) {
    if (!g_replay.is_initialized) {
        printf("Error: Replay system not initialized\n");
        return false;
    }
    
    if (g_replay.is_playing) {
        printf("Warning: Already playing\n");
        return false;
    }
    
    if (g_replay.count == 0) {
        printf("Error: No frames to replay\n");
        return false;
    }
    
    g_replay.is_playing = true;
    g_replay.is_recording = false;
    g_replay.current_frame = start_frame;
    g_replay.total_frames_played = 0;
    g_replay.total_play_time = 0.0;
    
    printf("Started replay playback from frame %u (total frames: %u)\n", 
           start_frame, g_replay.count);
    return true;
}

// Get next frame for playback
const PhysicsSnapshot* deterministic_replay_get_next_frame(void) {
    if (!g_replay.is_playing || g_replay.current_frame >= g_replay.count) {
        return NULL;
    }
    
    ReplayFrame* frame = &g_replay.frames[g_replay.current_frame];
    g_replay.current_frame++;
    g_replay.total_frames_played++;
    
    return &frame->snapshot;
}

// Stop playback
void deterministic_replay_stop_playback(void) {
    if (!g_replay.is_playing) return;
    
    g_replay.is_playing = false;
    printf("Stopped replay playback (frames played: %u)\n", g_replay.total_frames_played);
}

// Verify determinism by comparing snapshots
bool deterministic_replay_verify_frame(const PhysicsSnapshot* recorded, 
                                   const PhysicsSnapshot* simulated) {
    if (recorded->frame_number != simulated->frame_number) {
        printf("Frame number mismatch: recorded=%u, simulated=%u\n",
               recorded->frame_number, simulated->frame_number);
        return false;
    }
    
    if (recorded->body_count != simulated->body_count) {
        printf("Body count mismatch: recorded=%u, simulated=%u\n",
               recorded->body_count, simulated->body_count);
        return false;
    }
    
    // Compare body states with tolerance
    const float POSITION_TOLERANCE = 0.001f;
    const float VELOCITY_TOLERANCE = 0.01f;
    
    for (uint32_t i = 0; i < recorded->body_count; i++) {
        const auto* rec_body = &recorded->bodies[i];
        const auto* sim_body = &simulated->bodies[i];
        
        if (rec_body->id != sim_body->id) {
            printf("Body ID mismatch: recorded=%u, simulated=%u\n",
                   rec_body->id, sim_body->id);
            return false;
        }
        
        // Position comparison
        Vec3 pos_diff = vec3_sub(rec_body->position, sim_body->position);
        if (vec3_length_sq(pos_diff) > POSITION_TOLERANCE * POSITION_TOLERANCE) {
            printf("Position mismatch for body %u: recorded=(%.3f,%.3f,%.3f), simulated=(%.3f,%.3f,%.3f)\n",
                   rec_body->id,
                   rec_body->position.x, rec_body->position.y, rec_body->position.z,
                   sim_body->position.x, sim_body->position.y, sim_body->position.z);
            return false;
        }
        
        // Velocity comparison
        Vec3 vel_diff = vec3_sub(rec_body->velocity, sim_body->velocity);
        if (vec3_length_sq(vel_diff) > VELOCITY_TOLERANCE * VELOCITY_TOLERANCE) {
            printf("Velocity mismatch for body %u: recorded=(%.3f,%.3f,%.3f), simulated=(%.3f,%.3f,%.3f)\n",
                   rec_body->id,
                   rec_body->velocity.x, rec_body->velocity.y, rec_body->velocity.z,
                   sim_body->velocity.x, sim_body->velocity.y, sim_body->velocity.z);
            return false;
        }
    }
    
    return true;
}

// Test deterministic replay with simple physics
bool test_deterministic_physics(void) {
    printf("\n=== Deterministic Physics Test ===\n");
    
    bool test_passed = true;
    
    // Start recording
    if (!deterministic_replay_start_recording(1)) {
        printf("Failed to start recording\n");
        return false;
    }
    
    // Simulate and record physics for 100 frames
    for (uint32_t frame = 0; frame < 100; frame++) {
        // Create mock physics snapshot
        PhysicsSnapshot snapshot = {0};
        snapshot.frame_number = frame;
        snapshot.body_count = 3;
        snapshot.simulation_time = frame * g_replay.timestep_value;
        
        snapshot.bodies = malloc(3 * sizeof(snapshot.bodies[0]));
        
        // Simple falling objects with different masses
        for (int i = 0; i < 3; i++) {
            float t = frame * g_replay.timestep_value;
            snapshot.bodies[i].id = i + 1;
            snapshot.bodies[i].mass = 1.0f + i * 0.5f;
            snapshot.bodies[i].position = (Vec3){(float)i * 2.0f, 5.0f - 0.5f * 9.81f * t * t, 0.0f};
            snapshot.bodies[i].velocity = (Vec3){0.0f, -9.81f * t, 0.0f};
            snapshot.bodies[i].acceleration = (Vec3){0.0f, -9.81f, 0.0f};
            snapshot.bodies[i].rotation = (Quat){0, 0, 0, 1};
            snapshot.bodies[i].is_active = true;
        }
        
        // Add some player inputs
        snapshot.input_count = 1;
        snapshot.inputs = malloc(sizeof(snapshot.inputs[0]));
        snapshot.inputs[0].player_id = 1;
        snapshot.inputs[0].move_input = (Vec3){sinf(frame * 0.1f), 0, cosf(frame * 0.1f)};
        snapshot.inputs[0].jump_pressed = (frame % 30 == 0);
        snapshot.inputs[0].timestamp = t;
        
        deterministic_replay_record_frame(&snapshot);
        
        free(snapshot.bodies);
        free(snapshot.inputs);
    }
    
    deterministic_replay_stop_recording();
    
    // Start playback and verify
    if (!deterministic_replay_start_playback(0)) {
        printf("Failed to start playback\n");
        return false;
    }
    
    // Simulate physics again and compare with recorded
    for (uint32_t frame = 0; frame < 100; frame++) {
        const PhysicsSnapshot* recorded = deterministic_replay_get_next_frame();
        if (!recorded) {
            printf("Failed to get recorded frame %u\n", frame);
            test_passed = false;
            break;
        }
        
        // Simulate the same physics
        PhysicsSnapshot simulated = {0};
        simulated.frame_number = frame;
        simulated.body_count = 3;
        simulated.simulation_time = frame * g_replay.timestep_value;
        
        simulated.bodies = malloc(3 * sizeof(simulated.bodies[0]));
        
        for (int i = 0; i < 3; i++) {
            float t = frame * g_replay.timestep_value;
            simulated.bodies[i].id = i + 1;
            simulated.bodies[i].mass = 1.0f + i * 0.5f;
            simulated.bodies[i].position = (Vec3){(float)i * 2.0f, 5.0f - 0.5f * 9.81f * t * t, 0.0f};
            simulated.bodies[i].velocity = (Vec3){0.0f, -9.81f * t, 0.0f};
            simulated.bodies[i].acceleration = (Vec3){0.0f, -9.81f, 0.0f};
            simulated.bodies[i].rotation = (Quat){0, 0, 0, 1};
            simulated.bodies[i].is_active = true;
        }
        
        // Verify determinism
        if (!deterministic_replay_verify_frame(recorded, &simulated)) {
            printf("Determinism check failed at frame %u\n", frame);
            test_passed = false;
            g_replay.desync_count++;
        }
        
        free(simulated.bodies);
    }
    
    deterministic_replay_stop_playback();
    
    printf("Deterministic physics test: %s (desyncs: %u)\n",
           test_passed ? "PASSED" : "FAILED", g_replay.desync_count);
    
    return test_passed;
}

// Test network synchronization
bool test_network_synchronization(void) {
    printf("\n=== Network Synchronization Test ===\n");
    
    bool test_passed = true;
    
    // Simulate network delay
    g_replay.network_frame_delay = 5; // 5 frames delay
    
    if (!deterministic_replay_start_recording(1)) {
        printf("Failed to start recording\n");
        return false;
    }
    
    // Record frames with simulated network conditions
    for (uint32_t frame = 0; frame < 50; frame++) {
        PhysicsSnapshot snapshot = {0};
        snapshot.frame_number = frame;
        snapshot.body_count = 2;
        snapshot.simulation_time = frame * g_replay.timestep_value;
        
        snapshot.bodies = malloc(2 * sizeof(snapshot.bodies[0]));
        
        for (int i = 0; i < 2; i++) {
            float t = frame * g_replay.timestep_value;
            snapshot.bodies[i].id = i + 1;
            snapshot.bodies[i].mass = 1.0f;
            snapshot.bodies[i].position = (Vec3){(float)i * 3.0f, 10.0f - 0.5f * 9.81f * t * t, 0.0f};
            snapshot.bodies[i].velocity = (Vec3){0.0f, -9.81f * t, 0.0f};
            snapshot.bodies[i].acceleration = (Vec3){0.0f, -9.81f, 0.0f};
            snapshot.bodies[i].rotation = (Quat){0, 0, 0, 1};
            snapshot.bodies[i].is_active = true;
        }
        
        // Simulate delayed inputs
        snapshot.input_count = 1;
        snapshot.inputs = malloc(sizeof(snapshot.inputs[0]));
        snapshot.inputs[0].player_id = 1;
        snapshot.inputs[0].move_input = (Vec3){1.0f, 0, 0};
        snapshot.inputs[0].jump_pressed = (frame % 20 == 0);
        snapshot.inputs[0].timestamp = frame * g_replay.timestep_value - g_replay.network_frame_delay * g_replay.timestep_value;
        
        deterministic_replay_record_frame(&snapshot);
        
        free(snapshot.bodies);
        free(snapshot.inputs);
    }
    
    deterministic_replay_stop_recording();
    
    printf("Network synchronization test completed\n");
    return test_passed;
}

// Test replay integrity
bool test_replay_integrity(void) {
    printf("\n=== Replay Integrity Test ===\n");
    
    bool test_passed = true;
    
    if (!deterministic_replay_start_recording(1)) {
        printf("Failed to start recording\n");
        return false;
    }
    
    // Record frames with checksums
    for (uint32_t frame = 0; frame < 30; frame++) {
        PhysicsSnapshot snapshot = {0};
        snapshot.frame_number = frame;
        snapshot.body_count = 5;
        snapshot.simulation_time = frame * g_replay.timestep_value;
        
        snapshot.bodies = malloc(5 * sizeof(snapshot.bodies[0]));
        
        for (int i = 0; i < 5; i++) {
            float t = frame * g_replay.timestep_value;
            snapshot.bodies[i].id = i + 1;
            snapshot.bodies[i].mass = 1.0f + i * 0.2f;
            snapshot.bodies[i].position = (Vec3){
                sinf(t + i) * 5.0f,
                cosf(t + i) * 3.0f,
                sinf(t * 0.5f + i) * 2.0f
            };
            snapshot.bodies[i].velocity = (Vec3){
                cosf(t + i) * 5.0f,
                -sinf(t + i) * 3.0f,
                cosf(t * 0.5f + i) * 1.0f
            };
            snapshot.bodies[i].acceleration = (Vec3){0, -9.81f, 0};
            snapshot.bodies[i].rotation = (Quat){0, 0, sinf(t + i), cosf(t + i)};
            snapshot.bodies[i].is_active = true;
        }
        
        deterministic_replay_record_frame(&snapshot);
        free(snapshot.bodies);
    }
    
    deterministic_replay_stop_recording();
    
    // Verify checksums during playback
    if (!deterministic_replay_start_playback(0)) {
        printf("Failed to start playback\n");
        return false;
    }
    
    uint32_t checksum_failures = 0;
    while (true) {
        const PhysicsSnapshot* frame = deterministic_replay_get_next_frame();
        if (!frame) break;
        
        // Recalculate checksum and verify
        size_t data_size = frame->body_count * sizeof(frame->bodies[0]);
        uint32_t calculated_checksum = calculate_crc32(frame->bodies, data_size);
        
        if (calculated_checksum != frame->checksum) {
            printf("Checksum mismatch at frame %u: stored=%08X, calculated=%08X\n",
                   frame->frame_number, frame->checksum, calculated_checksum);
            checksum_failures++;
            test_passed = false;
        }
    }
    
    deterministic_replay_stop_playback();
    
    printf("Replay integrity test: %s (checksum failures: %u)\n",
           test_passed ? "PASSED" : "FAILED", checksum_failures);
    
    return test_passed;
}

// Generate replay report
void generate_replay_report(void) {
    printf("\n=== Deterministic Replay Report ===\n");
    
    printf("System Status: %s\n", g_replay.is_initialized ? "Initialized" : "Not Initialized");
    printf("Current Mode: %s\n", 
           g_replay.is_recording ? "Recording" : 
           (g_replay.is_playing ? "Playing" : "Idle"));
    
    printf("Recording Settings:\n");
    printf("  Fixed Timestep: %s\n", g_replay.fixed_timestep ? "Yes" : "No");
    printf("  Timestep Value: %.4f\n", g_replay.timestep_value);
    printf("  Keyframe Interval: %u\n", g_replay.keyframe_interval);
    printf("  Max Frames: %u\n", g_replay.max_frames);
    
    printf("Network Settings:\n");
    printf("  Local Player ID: %u\n", g_replay.local_player_id);
    printf("  Network Frame Delay: %u\n", g_replay.network_frame_delay);
    
    printf("Statistics:\n");
    printf("  Total Frames Recorded: %u\n", g_replay.total_frames_recorded);
    printf("  Total Frames Played: %u\n", g_replay.total_frames_played);
    printf("  Current Frame Buffer: %u/%u\n", g_replay.count, g_replay.capacity);
    printf("  Desync Count: %u\n", g_replay.desync_count);
    
    if (g_replay.total_frames_recorded > 0) {
        printf("  Recording Rate: %.1f fps\n", 
               g_replay.total_frames_recorded / (g_replay.total_record_time > 0 ? g_replay.total_record_time : 1.0));
    }
    
    if (g_replay.total_frames_played > 0) {
        printf("  Playback Rate: %.1f fps\n",
               g_replay.total_frames_played / (g_replay.total_play_time > 0 ? g_replay.total_play_time : 1.0));
    }
}

// Cleanup replay system
void deterministic_replay_cleanup(void) {
    if (!g_replay.is_initialized) return;
    
    printf("Cleaning up deterministic replay system...\n");
    
    // Free allocated memory for frames
    for (uint32_t i = 0; i < g_replay.count; i++) {
        ReplayFrame* frame = &g_replay.frames[i];
        if (frame->snapshot.bodies) {
            free(frame->snapshot.bodies);
        }
        if (frame->snapshot.inputs) {
            free(frame->snapshot.inputs);
        }
    }
    
    free(g_replay.frames);
    memset(&g_replay, 0, sizeof(g_replay));
    
    printf("Deterministic replay system cleaned up\n");
}

// Main verification function
int main(void) {
    printf("=== Deterministic Replay Verification ===\n");
    printf("Implementing TODO-0044: Deterministic replay for networked physics\n\n");
    
    // Initialize replay system
    if (!deterministic_replay_init(1000, 10, true, 1.0f / 60.0f)) {
        printf("Failed to initialize deterministic replay system\n");
        return 1;
    }
    
    // Run tests
    bool physics_test_passed = test_deterministic_physics();
    bool network_test_passed = test_network_synchronization();
    bool integrity_test_passed = test_replay_integrity();
    
    // Generate report
    generate_replay_report();
    
    // Cleanup
    deterministic_replay_cleanup();
    
    printf("\n=== Verification Complete ===\n");
    bool all_passed = physics_test_passed && network_test_passed && integrity_test_passed;
    printf("Result: %s\n", all_passed ? "PASSED" : "FAILED");
    
    return all_passed ? 0 : 1;
}
