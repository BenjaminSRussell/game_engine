// Quantum-Inspired Advanced Physics System
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Quat;

Vec3 vec3(float x, float y, float z) { return (Vec3){x, y, z}; }
Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 vec3_mul(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float vec3_length(Vec3 v) { return sqrtf(vec3_dot(v, v)); }
Vec3 vec3_normalize(Vec3 v) { float l = vec3_length(v); return l > 0 ? vec3_mul(v, 1.0f/l) : vec3(0,0,0); }
Vec3 vec3_cross(Vec3 a, Vec3 b) { return (Vec3){a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }

// Quantum-Inspired Particle System
typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    Vec3 phase; // Quantum phase
    float frequency;
    float amplitude;
    float mass;
    float charge;
    int entanglement_partner;
    bool is_entangled;
    Vec3 color;
} QuantumParticle;

// Wave Function
typedef struct {
    Vec3 center;
    float wavelength;
    float amplitude;
    float phase_velocity;
    float frequency;
    float time;
    bool active;
} WaveFunction;

// Quantum Field
typedef struct {
    Vec3 position;
    float strength;
    float radius;
    float frequency;
    float phase;
    bool active;
} QuantumField;

// Wormhole (Exotic Geometry)
typedef struct {
    Vec3 entrance;
    Vec3 exit;
    float radius;
    float stability;
    bool active;
    float event_horizon;
} Wormhole;

#define MAX_QUANTUM_PARTICLES 64
#define MAX_WAVE_FUNCTIONS 8
#define MAX_QUANTUM_FIELDS 4
#define MAX_WORMHOLES 2

static QuantumParticle particles[MAX_QUANTUM_PARTICLES];
static WaveFunction waves[MAX_WAVE_FUNCTIONS];
static QuantumField fields[MAX_QUANTUM_FIELDS];
static Wormhole wormholes[MAX_WORMHOLES];
static int particle_count = 0;
static int wave_count = 0;
static int field_count = 0;
static int wormhole_count = 0;

// Quantum Mechanics Functions
float calculate_wave_function(WaveFunction* wave, Vec3 position, float time) {
    Vec3 r = vec3_sub(position, wave->center);
    float distance = vec3_length(r);
    float k = 2.0f * M_PI / wave->wavelength;
    float phase = k * distance - wave->frequency * time + wave->phase_velocity;
    return wave->amplitude * cosf(phase) / (distance + 1.0f);
}

Vec3 calculate_quantum_force(QuantumParticle* particle, WaveFunction* wave, float time) {
    if (!wave->active) return vec3(0,0,0);
    
    float psi = calculate_wave_function(wave, particle->position, time);
    float psi_gradient = 0.01f; // Simplified gradient
    
    Vec3 force_direction = vec3_normalize(vec3_sub(particle->position, wave->center));
    return vec3_mul(force_direction, psi * psi_gradient * particle->charge);
}

Vec3 calculate_tunneling_force(QuantumParticle* particle, Wormhole* wormhole) {
    if (!wormhole->active) return vec3(0,0,0);
    
    Vec3 to_entrance = vec3_sub(wormhole->entrance, particle->position);
    Vec3 to_exit = vec3_sub(wormhole->exit, particle->position);
    float dist_entrance = vec3_length(to_entrance);
    float dist_exit = vec3_length(to_exit);
    
    if (dist_entrance < wormhole->event_horizon) {
        // Teleport to exit
        return vec3_mul(vec3_normalize(to_exit), 100.0f);
    }
    
    if (dist_entrance < wormhole->radius * 3.0f) {
        // Strong attraction near entrance
        return vec3_mul(vec3_normalize(to_entrance), 50.0f / (dist_entrance + 1.0f));
    }
    
    return vec3(0,0,0);
}

Vec3 calculate_entanglement_force(QuantumParticle* p1, QuantumParticle* p2) {
    if (!p1->is_entangled || !p2->is_entangled) return vec3(0,0,0);
    if (p1->entanglement_partner != p2 - particles) return vec3(0,0,0);
    
    // Quantum entanglement creates correlated behavior
    Vec3 correlation = vec3_sub(p2->position, p1->position);
    float correlation_strength = 10.0f;
    
    // Particles tend to maintain specific quantum relationships
    float phase_diff = p1->phase.x - p2->phase.x + p1->phase.y - p2->phase.y + p1->phase.z - p2->phase.z;
    Vec3 phase_force = vec3_mul(correlation, correlation_strength * sinf(phase_diff) * 0.1f);
    
    return phase_force;
}

void init_quantum_scene(void) {
    particle_count = 0;
    wave_count = 0;
    field_count = 0;
    wormhole_count = 0;
    
    // Create quantum particles with wave properties
    for (int i = 0; i < 16; i++) {
        particles[particle_count++] = (QuantumParticle){
            .position = vec3(-10 + i * 1.2f, 5 + (i % 4) * 1.5f, (i % 3) * 1.5f - 1.5f),
            .velocity = vec3((i % 5 - 2) * 0.8f, 0, (i % 3 - 1) * 0.6f),
            .acceleration = vec3(0,0,0),
            .phase = vec3((i % 7) * 0.9f, (i % 5) * 1.1f, (i % 3) * 1.3f),
            .frequency = 2.0f + i * 0.3f,
            .amplitude = 0.5f + (i % 4) * 0.2f,
            .mass = 0.3f + (i % 5) * 0.2f,
            .charge = (i % 2 == 0) ? 1.0f : -1.0f,
            .entanglement_partner = -1,
            .is_entangled = false,
            .color = vec3(0.3f + (i % 7) * 0.1f, 0.6f, 0.9f - (i % 5) * 0.15f)
        };
    }
    
    // Create entangled pairs
    for (int i = 0; i < 6; i++) {
        int idx1 = i * 2;
        int idx2 = i * 2 + 1;
        if (idx2 < particle_count) {
            particles[idx1].entanglement_partner = idx2;
            particles[idx2].entanglement_partner = idx1;
            particles[idx1].is_entangled = true;
            particles[idx2].is_entangled = true;
        }
    }
    
    // Create wave functions
    for (int i = 0; i < 4; i++) {
        waves[wave_count++] = (WaveFunction){
            .center = vec3((i % 3 - 1) * 8.0f, 3.0f, (i % 2 - 0.5f) * 6.0f),
            .wavelength = 2.0f + i * 0.5f,
            .amplitude = 1.0f + i * 0.3f,
            .phase_velocity = i * M_PI / 4.0f,
            .frequency = 1.0f + i * 0.2f,
            .time = 0.0f,
            .active = true
        };
    }
    
    // Create quantum fields
    for (int i = 0; i < 3; i++) {
        fields[field_count++] = (QuantumField){
            .position = vec3((i % 3 - 1) * 10.0f, 2.0f + i * 2.0f, (i % 2) * 8.0f - 4.0f),
            .strength = 20.0f + i * 10.0f,
            .radius = 3.0f + i * 1.0f,
            .frequency = 1.0f + i * 0.5f,
            .phase = i * M_PI / 3.0f,
            .active = true
        };
    }
    
    // Create wormholes
    wormholes[wormhole_count++] = (Wormhole){
        .entrance = vec3(-8.0f, 5.0f, 0.0f),
        .exit = vec3(8.0f, 8.0f, 0.0f),
        .radius = 1.5f,
        .stability = 0.8f,
        .active = true,
        .event_horizon = 1.0f
    };
    
    if (rand() % 2 == 0) {
        wormholes[wormhole_count++] = (Wormhole){
            .entrance = vec3(0.0f, 10.0f, -5.0f),
            .exit = vec3(0.0f, 0.0f, 5.0f),
            .radius = 1.2f,
            .stability = 0.6f,
            .active = true,
            .event_horizon = 0.8f
        };
    }
}

void update_quantum_physics(float dt) {
    static float quantum_time = 0.0f;
    quantum_time += dt;
    
    // Clear accelerations
    for (int i = 0; i < particle_count; i++) {
        particles[i].acceleration = vec3(0, -9.81f, 0); // Classical gravity
    }
    
    // Wave function forces
    for (int i = 0; i < particle_count; i++) {
        for (int j = 0; j < wave_count; j++) {
            Vec3 wave_force = calculate_quantum_force(&particles[i], &waves[j], quantum_time);
            particles[i].acceleration = vec3_add(particles[i].acceleration,
                vec3_mul(wave_force, 1.0f / particles[i].mass));
        }
    }
    
    // Quantum field forces
    for (int i = 0; i < particle_count; i++) {
        for (int j = 0; j < field_count; j++) {
            if (!fields[j].active) continue;
            
            Vec3 to_field = vec3_sub(fields[j].position, particles[i].position);
            float dist = vec3_length(to_field);
            
            if (dist < fields[j].radius * 2.0f) {
                float field_strength = fields[j].strength * 
                    (1.0f - dist / (fields[j].radius * 2.0f)) *
                    sinf(fields[j].frequency * quantum_time + fields[j].phase);
                
                Vec3 field_direction = vec3_normalize(to_field);
                Vec3 field_force = vec3_mul(field_direction, field_strength);
                
                particles[i].acceleration = vec3_add(particles[i].acceleration,
                    vec3_mul(field_force, particles[i].charge * 0.1f));
            }
        }
    }
    
    // Entanglement forces
    for (int i = 0; i < particle_count; i++) {
        if (particles[i].is_entangled && particles[i].entanglement_partner >= 0) {
            QuantumParticle* partner = &particles[particles[i].entanglement_partner];
            Vec3 entangle_force = calculate_entanglement_force(&particles[i], partner);
            particles[i].acceleration = vec3_add(particles[i].acceleration,
                vec3_mul(entangle_force, 1.0f / particles[i].mass));
        }
    }
    
    // Wormhole tunneling
    for (int i = 0; i < particle_count; i++) {
        for (int j = 0; j < wormhole_count; j++) {
            Vec3 tunnel_force = calculate_tunneling_force(&particles[i], &wormholes[j]);
            particles[i].acceleration = vec3_add(particles[i].acceleration,
                vec3_mul(tunnel_force, 1.0f / particles[i].mass));
        }
    }
    
    // Update particles (Verlet integration for quantum stability)
    for (int i = 0; i < particle_count; i++) {
        Vec3 new_velocity = vec3_add(particles[i].velocity,
            vec3_mul(particles[i].acceleration, dt));
        
        // Quantum damping
        particles[i].velocity = vec3_mul(new_velocity, 0.97f);
        particles[i].position = vec3_add(particles[i].position,
            vec3_mul(particles[i].velocity, dt));
        
        // Update quantum phase
        particles[i].phase.x += particles[i].frequency * dt;
        particles[i].phase.y += particles[i].frequency * dt * 1.1f;
        particles[i].phase.z += particles[i].frequency * dt * 0.9f;
        
        // Boundary conditions
        if (particles[i].position.y < -5.0f) {
            particles[i].position.y = -5.0f;
            particles[i].velocity.y *= -0.6f;
        }
        
        if (fabsf(particles[i].position.x) > 15.0f) {
            particles[i].velocity.x *= -0.7f;
            particles[i].position.x = (particles[i].position.x > 0) ? 15.0f : -15.0f;
        }
    }
    
    // Update wave functions
    for (int i = 0; i < wave_count; i++) {
        waves[i].time = quantum_time;
        waves[i].phase_velocity += dt * 0.5f;
    }
    
    // Update quantum fields
    for (int i = 0; i < field_count; i++) {
        fields[i].phase += fields[i].frequency * dt;
        fields[i].strength = 20.0f + 10.0f * sinf(quantum_time * 0.3f + i);
    }
    
    // Update wormholes
    for (int i = 0; i < wormhole_count; i++) {
        wormholes[i].stability = 0.8f + 0.2f * sinf(quantum_time * 0.2f + i * M_PI / 2.0f);
        wormholes[i].event_horizon = 1.0f + 0.3f * sinf(quantum_time * 0.4f);
    }
}

void render_quantum_system(void) {
    printf("\033[2J\033[H");
    
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 80; x++) {
            char ch = ' ';
            
            // Render quantum particles
            for (int i = 0; i < particle_count; i++) {
                int px = (int)(particles[i].position.x * 2.5f + 40);
                int py = (int)(-particles[i].position.y * 1.2f + 15);
                
                if (abs(x - px) < 2 && abs(y - py) < 1) {
                    if (particles[i].is_entangled) ch = 'E';
                    else if (particles[i].charge > 0) ch = '+';
                    else ch = '-';
                }
            }
            
            // Render wave functions
            for (int i = 0; i < wave_count; i++) {
                if (!waves[i].active) continue;
                
                int wx = (int)(waves[i].center.x * 2.5f + 40);
                int wy = (int)(-waves[i].center.y * 1.2f + 15);
                int wr = (int)(waves[i].wavelength * 2.5f);
                
                int dist_sq = (x - wx) * (x - wx) + (y - wy) * (y - wy);
                if (dist_sq < wr * wr && dist_sq > 0) {
                    float intensity = (1.0f + sinf(waves[i].phase_velocity)) * 0.5f;
                    if (intensity > 0.3f) ch = '~';
                    else if (intensity > 0.0f) ch = '.';
                }
            }
            
            // Render quantum fields
            for (int i = 0; i < field_count; i++) {
                if (!fields[i].active) continue;
                
                int fx = (int)(fields[i].position.x * 2.5f + 40);
                int fy = (int)(-fields[i].position.y * 1.2f + 15);
                int fr = (int)(fields[i].radius * 2.5f);
                
                int dist_sq = (x - fx) * (x - fx) + (y - fy) * (y - fy);
                if (dist_sq < fr * fr && dist_sq > 0) {
                    float field_intensity = (1.0f + sinf(fields[i].phase)) * 0.5f;
                    if (field_intensity > 0.4f) ch = '@';
                    else if (field_intensity > 0.2f) ch = '*';
                    else if (field_intensity > 0.0f) ch = '.';
                }
            }
            
            // Render wormholes
            for (int i = 0; i < wormhole_count; i++) {
                if (!wormholes[i].active) continue;
                
                // Entrance
                int ex = (int)(wormholes[i].entrance.x * 2.5f + 40);
                int ey = (int)(-wormholes[i].entrance.y * 1.2f + 15);
                int er = (int)(wormholes[i].radius * 2.5f);
                
                int dist_e_sq = (x - ex) * (x - ex) + (y - ey) * (y - ey);
                if (dist_e_sq < er * er && dist_e_sq > 0) {
                    ch = 'O';
                }
                
                // Exit
                int xx = (int)(wormholes[i].exit.x * 2.5f + 40);
                int xy = (int)(-wormholes[i].exit.y * 1.2f + 15);
                int xr = (int)(wormholes[i].radius * 2.0f);
                
                int dist_x_sq = (x - xx) * (x - xx) + (y - xy) * (y - xy);
                if (dist_x_sq < xr * xr && dist_x_sq > 0) {
                    ch = 'X';
                }
            }
            
            putchar(ch);
        }
        putchar('\n');
    }
    
    printf("\n=== QUANTUM PHYSICS SYSTEM ===\n");
    printf("Particles: %d | Waves: %d | Fields: %d | Wormholes: %d\n", 
           particle_count, wave_count, field_count, wormhole_count);
    printf("Systems: Wave Functions, Quantum Fields, Entanglement, Wormholes\n");
    printf("Features: Quantum Tunneling, Phase Evolution, Correlated States\n");
}

void add_quantum_particle(void) {
    if (particle_count >= MAX_QUANTUM_PARTICLES) return;
    
    particles[particle_count++] = (QuantumParticle){
        .position = vec3(-12 + (rand() % 240) / 10.0f, 8 + (rand() % 40) / 10.0f, (rand() % 30) / 10.0f - 1.5f),
        .velocity = vec3((rand() % 100 - 50) / 25.0f, (rand() % 50 - 25) / 25.0f, (rand() % 100 - 50) / 30.0f),
        .acceleration = vec3(0,0,0),
        .phase = vec3((rand() % 100) / 10.0f, (rand() % 100) / 10.0f, (rand() % 100) / 10.0f),
        .frequency = 1.0f + (rand() % 300) / 100.0f,
        .amplitude = 0.3f + (rand() % 70) / 100.0f,
        .mass = 0.2f + (rand() % 60) / 100.0f,
        .charge = (rand() % 2 == 0) ? 1.0f : -1.0f,
        .entanglement_partner = -1,
        .is_entangled = false,
        .color = vec3(0.2f + (rand() % 80) / 100.0f, 0.4f + (rand() % 60) / 100.0f, 0.6f + (rand() % 40) / 100.0f)
    };
}

int main(void) {
    printf("Starting QUANTUM PHYSICS SYSTEM\n");
    printf("Exotic Features:\n");
    printf("- Wave Function Mechanics\n");
    printf("- Quantum Entanglement\n");
    printf("- Quantum Field Effects\n");
    printf("- Wormhole Tunneling\n");
    printf("- Phase Evolution\n");
    printf("- Quantum Correlations\n\n");
    
    srand((unsigned int)time(NULL));
    init_quantum_scene();
    
    float last_time = (float)clock() / CLOCKS_PER_SEC;
    bool running = true;
    int frame_count = 0;
    
    while (running) {
        float current_time = (float)clock() / CLOCKS_PER_SEC;
        float dt = current_time - last_time;
        last_time = current_time;
        
        if (dt > 0.03f) dt = 0.03f;
        
        if (frame_count % 40 == 0) {
            add_quantum_particle();
        }
        
        if (frame_count % 60 == 0) {
            // Randomly activate/deactivate waves
            for (int i = 0; i < wave_count; i++) {
                waves[i].active = (rand() % 100 < 70);
            }
        }
        
        if (frame_count % 80 == 0) {
            // Randomly modify field strengths
            for (int i = 0; i < field_count; i++) {
                fields[i].strength = 10.0f + (rand() % 300) / 10.0f;
            }
        }
        
        update_quantum_physics(dt);
        render_quantum_system();
        
        frame_count++;
        
        struct timespec ts = {0, 40000000};
        nanosleep(&ts, NULL);
        
        if (frame_count > 600) running = false;
    }
    
    printf("\nQuantum Physics Demo Complete!\n");
    printf("Demonstrated: Wave Functions, Entanglement, Quantum Fields, Wormholes\n");
    return 0;
}
