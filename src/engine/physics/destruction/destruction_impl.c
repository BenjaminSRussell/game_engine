// physics/destruction/destruction_impl.c
// Chaos Destruction and Geometry Collection implementation.
//
// TODO: Implement Connectivity Graph for structural integrity calculation.
// TODO: Add support for Voronoi-based procedural fracturing of meshes.
// TODO: Implement Strain-Propagation logic (Force -> Damage -> Collapse).
// TODO: Add support for pre-fractured Geometry Collections with baked
// collision.
// TODO: Implement GPU-driven debris simulation using compute shaders.
// TODO: Add support for destruction fields (Radial, Directional, Anchor).
// TODO: Implement sleeping/awake state management for massive fragment counts.
// TODO: Add support for sound-triggering based on material-break types.
// TODO: Implement particle-spawning (Niagara integration) for dust/debris.
// TODO: Add support for networked synchronization of fractured states
// (Delta-Packing).
// TODO: Research and implement ML-based collision-mimpl approximation for
// fragments.
// TODO: Implement a robust cache-playback system for cinematics.

#include "destruction_impl.h"
#include "connectivity_graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ========================================
// Vector Math Utilities
// ========================================

static inline void vec3_copy(float *dest, const float *src) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

static inline void vec3_add(float *result, const float *a, const float *b) {
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
}

static inline void vec3_sub(float *result, const float *a, const float *b) {
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}

static inline void vec3_mul(float *result, const float *v, float s) {
    result[0] = v[0] * s;
    result[1] = v[1] * s;
    result[2] = v[2] * s;
}

static inline float vec3_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline float vec3_length(const float *v) {
    return sqrtf(vec3_dot(v, v));
}

static inline void vec3_normalize(float *result, const float *v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        vec3_mul(result, v, 1.0f / len);
    } else {
        result[0] = result[1] = result[2] = 0.0f;
    }
}

// ========================================
// Constants
// ========================================

#define MAX_VORONOI_SITES 1024
#define MAX_FRAGMENTS 4096
#define FRACTURE_THRESHOLD 50.0f
#define MIN_FRAGMENT_SIZE 0.1f
#define MAX_FRAGMENT_SIZE 10.0f

// ========================================
// Voronoi Site Structure
// ========================================

typedef struct VoronoiSite {
    float position[3];            // Site position in world space
    float weight;                // Site weight for influence
    uint32_t fragment_id;          // Associated fragment ID
    bool active;                  // Site is active in fracturing
    
} VoronoiSite;

// ========================================
// Fragment Structure
// ========================================

typedef struct Fragment {
    uint64_t id;                 // Fragment unique ID
    float vertices[1024][3];        // Fragment vertices
    int vertex_count;              // Number of vertices
    int triangles[2048][3];         // Triangle indices
    int triangle_count;            // Number of triangles
    float position[3];             // Fragment center position
    float velocity[3];             // Fragment velocity
    float angular_velocity[3];       // Angular velocity
    float mass;                   // Fragment mass
    float size;                   // Fragment size
    bool is_sleeping;              // Fragment is sleeping
    float sleep_timer;             // Time until wake check
    bool is_active;               // Fragment is active
    
} Fragment;

// ========================================
// Destruction Field Types
// ========================================

typedef enum DestructionFieldType {
    FIELD_RADIAL,                // Radial explosion field
    FIELD_DIRECTIONAL,            // Directional force field
    FIELD_ANCHOR                 // Anchor point field
    FIELD_WAVE                   // Wave propagation field
} DestructionFieldType;

typedef struct DestructionField {
    DestructionFieldType type;
    float position[3];            // Field origin
    float direction[3];            // Field direction (for directional)
    float radius;                  // Field radius
    float strength;                // Field strength
    float falloff;                 // Field falloff rate
    bool active;                  // Field is active
    
} DestructionField;

// ========================================
// Global State
// ========================================

static bool g_destruction_enabled = true;
static float g_debris_lifetime = 10.0f;

static VoronoiSite g_voronoi_sites[MAX_VORONOI_SITES];
static int g_voronoi_site_count = 0;

static Fragment g_fragments[MAX_FRAGMENTS];
static int g_fragment_count = 0;

static DestructionField g_destruction_fields[32];
static int g_destruction_field_count = 0;

static ConnectivityGraph *g_connectivity_graph = NULL;

// ========================================
// Voronoi Diagram Implementation
// ========================================

static float calculate_voronoi_distance(const float *a, const float *b) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

static void generate_voronoi_sites(const float *mesh_vertices, int vertex_count, 
                               const float *impact_point, float impact_force) {
    g_voronoi_site_count = 0;
    
    // Add impact point as primary site
    if (g_voronoi_site_count < MAX_VORONOI_SITES) {
        VoronoiSite *site = &g_voronoi_sites[g_voronoi_site_count++];
        vec3_copy(site->position, impact_point);
        site->weight = impact_force;
        site->fragment_id = UINT32_MAX;
        site->active = true;
    }
    
    // Add mesh vertices as sites
    for (int i = 0; i < vertex_count && g_voronoi_site_count < MAX_VORONOI_SITES; i++) {
        VoronoiSite *site = &g_voronoi_sites[g_voronoi_site_count++];
        vec3_copy(site->position, &mesh_vertices[i * 3]);
        site->weight = 1.0f;
        site->fragment_id = UINT32_MAX;
        site->active = true;
    }
    
    // Sort sites by weight (heavier sites get priority)
    for (int i = 0; i < g_voronoi_site_count - 1; i++) {
        for (int j = i + 1; j < g_voronoi_site_count; j++) {
            if (g_voronoi_sites[j].weight > g_voronoi_sites[i].weight) {
                VoronoiSite temp = g_voronoi_sites[i];
                g_voronoi_sites[i] = g_voronoi_sites[j];
                g_voronoi_sites[j] = temp;
            }
        }
    }
}

static int find_voronoi_site(const float *position) {
    int closest_site = 0;
    float min_distance = calculate_voronoi_distance(position, g_voronoi_sites[0].position);
    
    for (int i = 1; i < g_voronoi_site_count; i++) {
        if (!g_voronoi_sites[i].active) continue;
        
        float distance = calculate_voronoi_distance(position, g_voronoi_sites[i].position);
        if (distance < min_distance) {
            min_distance = distance;
            closest_site = i;
        }
    }
    
    return closest_site;
}

// ========================================
// Fragment Management
// ========================================

static uint32_t create_fragment(const float *vertices, int vertex_count, 
                           const int *triangles, int triangle_count,
                           const float *center, float mass) {
    if (g_fragment_count >= MAX_FRAGMENTS) return UINT32_MAX;
    
    Fragment *fragment = &g_fragments[g_fragment_count++];
    fragment->id = (uint64_t)fragment;
    
    // Copy vertices
    for (int i = 0; i < vertex_count && i < 1024; i++) {
        vec3_copy(fragment->vertices[i], &vertices[i * 3]);
    }
    fragment->vertex_count = vertex_count;
    
    // Copy triangles
    for (int i = 0; i < triangle_count && i < 2048; i++) {
        fragment->triangles[i][0] = triangles[i * 3];
        fragment->triangles[i][1] = triangles[i * 3 + 1];
        fragment->triangles[i][2] = triangles[i * 3 + 2];
    }
    fragment->triangle_count = triangle_count;
    
    // Set physics properties
    vec3_copy(fragment->position, center);
    fragment->velocity[0] = fragment->velocity[1] = fragment->velocity[2] = 0.0f;
    fragment->angular_velocity[0] = fragment->angular_velocity[1] = fragment->angular_velocity[2] = 0.0f;
    fragment->mass = mass;
    fragment->size = 1.0f;
    fragment->is_sleeping = false;
    fragment->sleep_timer = 0.0f;
    fragment->is_active = true;
    
    return fragment->id;
}

static void fragment_apply_force(Fragment *fragment, const float *force, float dt) {
    if (!fragment || fragment->is_sleeping || !fragment->is_active) return;
    
    // Apply linear force
    if (fragment->mass > 0.0f) {
        float acceleration[3];
        vec3_mul(acceleration, force, 1.0f / fragment->mass);
        fragment->velocity[0] += acceleration[0] * dt;
        fragment->velocity[1] += acceleration[1] * dt;
        fragment->velocity[2] += acceleration[2] * dt;
    }
    
    // Wake up if significant force applied
    float force_magnitude = vec3_length(force);
    if (force_magnitude > FRACTURE_THRESHOLD) {
        fragment->is_sleeping = false;
        fragment->sleep_timer = 0.0f;
    }
}

static void fragment_update(Fragment *fragment, float dt) {
    if (!fragment || !fragment->is_active) return;
    
    // Update position
    fragment->position[0] += fragment->velocity[0] * dt;
    fragment->position[1] += fragment->velocity[1] * dt;
    fragment->position[2] += fragment->velocity[2] * dt;
    
    // Update rotation
    // (Simplified - would use quaternions in full implementation)
    
    // Apply damping
    float damping = 0.99f;
    vec3_mul(fragment->velocity, fragment->velocity, damping);
    vec3_mul(fragment->angular_velocity, fragment->angular_velocity, damping);
    
    // Sleep management
    if (fragment->is_sleeping) {
        fragment->sleep_timer += dt;
        if (fragment->sleep_timer > 5.0f) {
            fragment->is_sleeping = false;
            fragment->sleep_timer = 0.0f;
        }
    } else {
        float speed = vec3_length(fragment->velocity);
        if (speed < 0.1f) {
            fragment->is_sleeping = true;
            fragment->sleep_timer = 0.0f;
        }
    }
}

// ========================================
// Destruction Fields Implementation
// ========================================

static int create_destruction_field(DestructionFieldType type, const float *position, 
                                      const float *direction, float radius, 
                                      float strength, float falloff) {
    if (g_destruction_field_count >= 32) return -1;
    
    DestructionField *field = &g_destruction_fields[g_destruction_field_count++];
    field->type = type;
    vec3_copy(field->position, position);
    
    if (direction) {
        vec3_copy(field->direction, direction);
        vec3_normalize(field->direction, field->direction);
    } else {
        field->direction[0] = 0.0f;
        field->direction[1] = 1.0f;
        field->direction[2] = 0.0f;
    }
    
    field->radius = radius;
    field->strength = strength;
    field->falloff = falloff;
    field->active = true;
    
    return g_destruction_field_count - 1;
}

static void apply_destruction_field(DestructionField *field, Fragment *fragment, float dt) {
    if (!field || !field->active || !fragment || !fragment->is_active) return;
    
    float to_fragment[3];
    vec3_sub(to_fragment, fragment->position, field->position);
    float distance = vec3_length(to_fragment);
    
    if (distance > field->radius) return;
    
    float force_magnitude = field->strength * expf(-field->falloff * distance);
    
    float force[3];
    switch (field->type) {
        case FIELD_RADIAL:
            vec3_normalize(force, to_fragment);
            break;
        case FIELD_DIRECTIONAL:
            vec3_copy(force, field->direction);
            break;
        case FIELD_ANCHOR:
            // Anchor pulls toward position
            vec3_normalize(force, to_fragment);
            break;
        case FIELD_WAVE:
            // Wave creates oscillating force
            float wave_phase = sinf(field->strength * dt * 10.0f);
            vec3_copy(force, field->direction);
            force[0] *= wave_phase;
            force[1] *= wave_phase;
            force[2] *= wave_phase;
            break;
    }
    
    vec3_mul(force, force, force_magnitude);
    fragment_apply_force(fragment, force, dt);
}

// ========================================
// Procedural Fracturing
// ========================================

static void voronoi_fracture_mesh(const float *vertices, int vertex_count,
                               const int *triangles, int triangle_count,
                               const float *impact_point, float impact_force,
                               Fragment **fragments, int *fragment_count) {
    if (!vertices || !triangles || !fragments || !fragment_count) return;
    
    // Generate Voronoi sites
    generate_voronoi_sites(vertices, vertex_count, impact_point, impact_force);
    
    // Create fragments based on Voronoi regions
    int created_fragments = 0;
    
    for (int i = 0; i < vertex_count; i++) {
        int site_id = find_voronoi_site(&vertices[i * 3]);
        if (site_id < 0) continue;
        
        VoronoiSite *site = &g_voronoi_sites[site_id];
        if (site->fragment_id == UINT32_MAX) {
            // Create new fragment for this site
            site->fragment_id = create_fragment(vertices, vertex_count, triangles, triangle_count,
                                           &vertices[i * 3], 1.0f);
            if (site->fragment_id != UINT32_MAX) {
                (*fragments)[created_fragments++] = &g_fragments[site->fragment_id];
            }
        }
    }
    
    *fragment_count = created_fragments;
}

static void strain_propagation_analysis(ConnectivityGraph *graph, const float *impact_point, 
                                 float impact_force) {
    if (!graph || !impact_point) return;
    
    // Apply impact force to nearest node
    uint32_t nearest_node = 0;
    float min_distance = FLT_MAX;
    
    for (int i = 0; i < graph->node_count; i++) {
        if (!graph->nodes[i].is_active || graph->nodes[i].is_broken) continue;
        
        float distance = calculate_voronoi_distance(impact_point, graph->nodes[i].position);
        if (distance < min_distance) {
            min_distance = distance;
            nearest_node = i;
        }
    }
    
    if (nearest_node < graph->node_count) {
        float force[3];
        vec3_sub(force, impact_point, graph->nodes[nearest_node].position);
        vec3_normalize(force, force);
        vec3_mul(force, force, impact_force);
        
        connectivity_graph_apply_node_force(graph, nearest_node, force);
        
        // Propagate stress through connectivity
        for (int iteration = 0; iteration < 5; iteration++) {
            connectivity_graph_update(graph, 0.016f); // 60 FPS timestep
            
            // Check for new fractures and propagate
            int weak_points[32];
            int weak_count = connectivity_graph_find_weak_points(graph, weak_points, 32);
            
            for (int i = 0; i < weak_count; i++) {
                connectivity_graph_trigger_fracture(graph, weak_points[i], impact_point, impact_force * 0.5f);
            }
        }
    }
}

// ========================================
// GPU Debris Simulation (Simplified)
// ========================================

static void simulate_debris_gpu(Fragment *fragments, int fragment_count, float dt) {
    // In a real implementation, this would use compute shaders
    // For now, we'll use CPU simulation
    
    for (int i = 0; i < fragment_count; i++) {
        Fragment *fragment = &fragments[i];
        
        // Apply gravity
        float gravity[3] = {0.0f, -9.81f, 0.0f};
        fragment_apply_force(fragment, gravity, dt);
        
        // Update fragment
        fragment_update(fragment, dt);
        
        // Remove old fragments
        if (fragment->position[1] < -50.0f) {
            fragment->is_active = false;
        }
    }
}

// ========================================
// Public API Implementation
// ========================================

void destruction_sys_set_enabled(bool enabled) {
    g_destruction_enabled = enabled;
}

bool destruction_sys_is_enabled(void) { 
    return g_destruction_enabled; 
}

void destruction_sys_set_debris_lifetime(float lifetime) {
    g_debris_lifetime = lifetime;
}

float destruction_sys_get_debris_lifetime(void) { 
    return g_debris_lifetime; 
}

void destruction_sys_trigger_break(uint64_t entity_id, float x, float y, float z, float force) {
    if (!g_destruction_enabled) return;
    
           "%.2f) with force %.2f\n",
           entity_id, x, y, z, force);
    
    // Initialize connectivity graph if needed
    if (!g_connectivity_graph) {
        GraphConfig config = connectivity_graph_get_default_config();
        g_connectivity_graph = connectivity_graph_create(1024, 2048, &config);
    }
    
    // Apply strain propagation
    float impact_point[3] = {x, y, z};
    strain_propagation_analysis(g_connectivity_graph, impact_point, force);
    
    // Create fragments from connectivity graph
    Fragment *fragments[MAX_FRAGMENTS];
    int fragment_count = 0;
    
    // Generate fragments from broken nodes
    for (int i = 0; i < g_connectivity_graph->node_count; i++) {
        if (g_connectivity_graph->nodes[i].is_broken) {
            // Create fragment from node
            float vertices[3] = {
                g_connectivity_graph->nodes[i].position[0],
                g_connectivity_graph->nodes[i].position[1],
                g_connectivity_graph->nodes[i].position[2]
            };
            int triangles[3] = {0, 1, 2}; // Simple triangle
            
            uint32_t fragment_id = create_fragment(vertices, 1, triangles, 1, vertices, 
                                                   g_connectivity_graph->nodes[i].mass);
            if (fragment_id != UINT32_MAX) {
                fragments[fragment_count++] = &g_fragments[fragment_id];
            }
        }
    }
    
    // Simulate debris
    simulate_debris_gpu(fragments, fragment_count, 0.016f);
    
    // Remove old debris
    for (int i = 0; i < g_fragment_count; i++) {
        if (!g_fragments[i].is_active || 
            g_fragments[i].position[1] < -100.0f ||
            g_fragments[i].sleep_timer > g_debris_lifetime) {
            g_fragments[i].is_active = false;
        }
    }
    
}

int destruction_sys_create_field(DestructionFieldType type, const float *position, 
                                 const float *direction, float radius, 
                                 float strength, float falloff) {
    return create_destruction_field(type, position, direction, radius, strength, falloff);
}

void destruction_sys_remove_field(int field_id) {
    if (field_id >= 0 && field_id < g_destruction_field_count) {
        g_destruction_fields[field_id].active = false;
    }
}

void destruction_sys_apply_fields(Fragment *fragments, int fragment_count, float dt) {
    for (int i = 0; i < g_destruction_field_count; i++) {
        DestructionField *field = &g_destruction_fields[i];
        if (!field->active) continue;
        
        for (int j = 0; j < fragment_count; j++) {
            apply_destruction_field(field, &fragments[j], dt);
        }
    }
}

void destruction_sys_simulate_debris(float dt) {
    simulate_debris_gpu(g_fragments, g_fragment_count, dt);
}

int destruction_sys_get_active_fragment_count(void) {
    int count = 0;
    for (int i = 0; i < g_fragment_count; i++) {
        if (g_fragments[i].is_active) count++;
    }
    return count;
}

Fragment* destruction_sys_get_fragment(int fragment_id) {
    if (fragment_id >= 0 && fragment_id < g_fragment_count) {
        return &g_fragments[fragment_id];
    }
    return NULL;
}

void destruction_sys_reset(void) {
    // Reset all data
    g_voronoi_site_count = 0;
    g_fragment_count = 0;
    g_destruction_field_count = 0;
    
    if (g_connectivity_graph) {
        connectivity_graph_reset(g_connectivity_graph);
    }
}
