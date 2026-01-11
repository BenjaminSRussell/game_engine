/**
 * @file deformable_mesh.h
 * @brief Deformable mesh data and physics simulation
 *
 * Implements deformable body physics using finite element methods and
 * mass-spring systems. Supports vertex deformation, shape matching,
 * and volume preservation.
 */

#ifndef RENDER_DEFORMABLE_MESH_H
#define RENDER_DEFORMABLE_MESH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct DeformableMesh DeformableMesh;
typedef struct DeformableVertex DeformableVertex;
typedef struct DeformableEdge DeformableEdge;
typedef struct DeformableTriangle DeformableTriangle;

// ========================================
// Deformable Vertex
// ========================================

typedef struct DeformableVertex {
    // Position data
    float position[3];          // Current world position
    float rest_position[3];      // Rest/local position
    float old_position[3];       // Previous frame position
    
    // Velocity and forces
    float velocity[3];
    float force[3];
    
    // Physical properties
    float mass;
    float inv_mass;
    
    // Deformation data
    float displacement[3];       // Displacement from rest position
    float strain[3];            // Principal strains
    float stress[3];            // Principal stresses
    
    // Constraints
    bool pinned;                // Fixed in world space
    bool collision_enabled;
    
    // Skinning data (for animated meshes)
    float bone_weights[4];      // Bone influence weights
    int bone_indices[4];        // Bone indices
    
    // User data
    void *user_data;
    
} DeformableVertex;

// ========================================
// Deformable Edge (Spring)
// ========================================

typedef struct DeformableEdge {
    int vertex_a;               // First vertex index
    int vertex_b;               // Second vertex index
    float rest_length;          // Rest length of edge
    float stiffness;            // Spring stiffness
    float damping;              // Damping coefficient
    bool enabled;
    
} DeformableEdge;

// ========================================
// Deformable Triangle
// ========================================

typedef struct DeformableTriangle {
    int vertices[3];            // Vertex indices
    float rest_normal[3];       // Rest triangle normal
    float rest_area;            // Rest triangle area
    
    // Material properties
    float youngs_modulus;       // Material stiffness
    float poissons_ratio;        // Material compressibility
    float density;              // Material density
    
    // Deformation state
    float current_normal[3];    // Current triangle normal
    float current_area;         // Current triangle area
    float strain_energy;        // Stored strain energy
    
    bool enabled;
    
} DeformableTriangle;

// ========================================
// Deformable Material Properties
// ========================================

typedef struct DeformableMaterial {
    // Elastic properties
    float youngs_modulus;       // Young's modulus (stiffness)
    float poissons_ratio;        // Poisson's ratio (compressibility)
    float density;               // Material density
    float damping;               // Global damping
    
    // Plasticity
    float yield_stress;          // Stress threshold for plastic deformation
    float plastic_strain;        // Accumulated plastic strain
    float hardening_coefficient; // Strain hardening factor
    
    // Fracture
    float fracture_threshold;    // Stress threshold for fracture
    float damage;                // Damage accumulation (0-1)
    
    // Visual properties
    float base_color[4];         // Base material color
    float emissive_color[4];     // Emissive color for heated materials
    
} DeformableMaterial;

// ========================================
// Deformation Model Types
// ========================================

typedef enum DeformationModel {
    DEFORM_MASS_SPRING,         // Mass-spring system
    DEFORM_SHAPE_MATCHING,      // Shape matching
    DEFORM_FEM_LINEAR,          // Linear finite element method
    DEFORM_FEM_NONLINEAR,       // Nonlinear FEM
    DEFORM_POSITION_BASED        // Position based dynamics
} DeformationModel;

// ========================================
// Simulation Configuration
// ========================================

typedef struct DeformableConfig {
    // Simulation parameters
    DeformationModel model;      // Deformation model to use
    int solver_iterations;       // Solver iterations per frame
    float time_step;            // Fixed time step
    float gravity[3];           // Gravity vector
    
    // Constraint parameters
    float position_stiffness;   // Position constraint stiffness
    float shape_stiffness;       // Shape matching stiffness
    float volume_stiffness;     // Volume preservation stiffness
    
    // Collision parameters
    bool self_collision;        // Enable self-collision
    bool world_collision;       // Enable world collision
    float collision_thickness;  // Collision thickness
    float collision_damping;    // Collision response damping
    
    // Performance parameters
    bool use_gpu_acceleration;  // Use GPU for computation
    bool adaptive_tessellation; // Adaptive mesh refinement
    int max_subdivision_level;  // Maximum subdivision level
    
} DeformableConfig;

// ========================================
// Deformable Mesh Structure
// ========================================

struct DeformableMesh {
    // Mesh data
    DeformableVertex *vertices;
    int vertex_count;
    int vertex_capacity;
    
    DeformableEdge *edges;
    int edge_count;
    int edge_capacity;
    
    DeformableTriangle *triangles;
    int triangle_count;
    int triangle_capacity;
    
    // Material
    DeformableMaterial material;
    
    // Configuration
    DeformableConfig config;
    
    // Simulation state
    float total_energy;         // Total system energy
    float kinetic_energy;       // Kinetic energy
    float potential_energy;     // Potential energy
    bool simulation_active;
    
    // Bounding volume
    float bounds_min[3];        // Mesh bounding box min
    float bounds_max[3];        // Mesh bounding box max
    bool bounds_dirty;
    
    // Collision data
    void *collision_shape;      // Optimized collision representation
    bool collision_dirty;
    
    // GPU resources (if enabled)
    void *gpu_vertex_buffer;
    void *gpu_index_buffer;
    bool gpu_data_dirty;
};

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create deformable mesh with specified capacity
 */
DeformableMesh* deformable_mesh_create(int max_vertices, int max_triangles, 
                                       const DeformableConfig *config);

/**
 * Destroy deformable mesh and free resources
 */
void deformable_mesh_destroy(DeformableMesh *mesh);

// ========================================
// Mesh Building
// ========================================

/**
 * Add vertex to mesh
 */
int deformable_mesh_add_vertex(DeformableMesh *mesh, const float *position, float mass);

/**
 * Add triangle to mesh
 */
int deformable_mesh_add_triangle(DeformableMesh *mesh, int v0, int v1, int v2);

/**
 * Create edges from triangles
 */
void deformable_mesh_build_edges(DeformableMesh *mesh);

/**
 * Create rectangular grid mesh
 */
void deformable_mesh_create_grid(DeformableMesh *mesh, int width, int height, 
                                float spacing, const float *position, float mass);

/**
 * Create sphere mesh
 */
void deformable_mesh_create_sphere(DeformableMesh *mesh, float radius, int segments, 
                                  const float *position, float mass);

/**
 * Load mesh from vertex and index data
 */
void deformable_mesh_load_data(DeformableMesh *mesh, 
                              const float *vertices, int vertex_count,
                              const int *indices, int index_count, float mass);

// ========================================
// Material Management
// ========================================

/**
 * Set mesh material properties
 */
void deformable_mesh_set_material(DeformableMesh *mesh, const DeformableMaterial *material);

/**
 * Get mesh material
 */
DeformableMaterial* deformable_mesh_get_material(DeformableMesh *mesh);

/**
 * Apply material preset (rubber, metal, flesh, etc.)
 */
void deformable_mesh_apply_material_preset(DeformableMesh *mesh, const char *preset_name);

// ========================================
// Simulation Interface
// ========================================

/**
 * Update deformable mesh simulation
 */
void deformable_mesh_update(DeformableMesh *mesh, float dt);

/**
 * Apply external force to vertex
 */
void deformable_mesh_apply_vertex_force(DeformableMesh *mesh, int vertex_id, const float *force);

/**
 * Apply force to all vertices
 */
void deformable_mesh_apply_force(DeformableMesh *mesh, const float *force);

/**
 * Apply impulse to vertex
 */
void deformable_mesh_apply_impulse(DeformableMesh *mesh, int vertex_id, const float *impulse);

/**
 * Apply pressure to mesh surface
 */
void deformable_mesh_apply_pressure(DeformableMesh *mesh, float pressure);

/**
 * Apply wind force
 */
void deformable_mesh_apply_wind(DeformableMesh *mesh, const float *wind_direction, float strength);

// ========================================
// Constraint Management
// ========================================

/**
 * Pin vertex in world space
 */
void deformable_mesh_pin_vertex(DeformableMesh *mesh, int vertex_id, bool pinned);

/**
 * Pin multiple vertices
 */
void deformable_mesh_pin_vertices(DeformableMesh *mesh, const int *vertex_ids, int count, bool pinned);

/**
 * Set vertex position (for pinned vertices)
 */
void deformable_mesh_set_vertex_position(DeformableMesh *mesh, int vertex_id, const float *position);

/**
 * Add edge constraint
 */
int deformable_mesh_add_edge_constraint(DeformableMesh *mesh, int vertex_a, int vertex_b, 
                                       float stiffness, float damping);

/**
 * Remove edge constraint
 */
void deformable_mesh_remove_edge_constraint(DeformableMesh *mesh, int edge_id);

// ========================================
// Collision Interface
// ========================================

/**
 * Process world collisions
 */
void deformable_mesh_process_world_collision(DeformableMesh *mesh);

/**
 * Process self-collisions
 */
void deformable_mesh_process_self_collision(DeformableMesh *mesh);

/**
 * Add collision constraint
 */
void deformable_mesh_add_collision_constraint(DeformableMesh *mesh, int vertex_id, 
                                             const float *normal, float penetration, float friction);

// ========================================
// Deformation Analysis
// ========================================

/**
 * Calculate mesh deformation metrics
 */
void deformable_mesh_calculate_deformation(DeformableMesh *mesh);

/**
 * Get maximum displacement in mesh
 */
float deformable_mesh_get_max_displacement(const DeformableMesh *mesh);

/**
 * Get total strain energy
 */
float deformable_mesh_get_strain_energy(const DeformableMesh *mesh);

/**
 * Calculate stress at vertex
 */
void deformable_mesh_get_vertex_stress(const DeformableMesh *mesh, int vertex_id, float *stress);

/**
 * Check if mesh has exceeded fracture threshold
 */
bool deformable_mesh_should_fracture(const DeformableMesh *mesh);

// ========================================
// Mesh Manipulation
// ========================================

/**
 * Transform mesh (rotation, translation, scale)
 */
void deformable_mesh_transform(DeformableMesh *mesh, const float *matrix);

/**
 * Reset mesh to rest position
 */
void deformable_mesh_reset(DeformableMesh *mesh);

/**
 * Subdivide mesh (increase detail)
 */
void deformable_mesh_subdivide(DeformableMesh *mesh, int level);

/**
 * Simplify mesh (reduce detail)
 */
void deformable_mesh_simplify(DeformableMesh *mesh, float error_threshold);

/**
 * Weld nearby vertices
 */
void deformable_mesh_weld_vertices(DeformableMesh *mesh, float threshold);

// ========================================
// Utility Functions
// ========================================

/**
 * Get default deformable configuration
 */
DeformableConfig deformable_mesh_get_default_config(void);

/**
 * Update mesh configuration
 */
void deformable_mesh_set_config(DeformableMesh *mesh, const DeformableConfig *config);

/**
 * Get mesh statistics
 */
void deformable_mesh_get_stats(const DeformableMesh *mesh, int *vertex_count, 
                              int *edge_count, int *triangle_count, float *total_energy);

/**
 * Update mesh bounds
 */
void deformable_mesh_update_bounds(DeformableMesh *mesh);

/**
 * Get mesh bounding box
 */
void deformable_mesh_get_bounds(const DeformableMesh *mesh, float *min_bounds, float *max_bounds);

/**
 * Validate mesh integrity
 */
bool deformable_mesh_validate(const DeformableMesh *mesh);

/**
 * Optimize mesh for performance
 */
void deformable_mesh_optimize(DeformableMesh *mesh);

// ========================================
// Rendering Support
// ========================================

/**
 * Get vertex positions for rendering
 */
const float* deformable_mesh_get_vertex_positions(const DeformableMesh *mesh);

/**
 * Get vertex normals for rendering
 */
const float* deformable_mesh_get_vertex_normals(const DeformableMesh *mesh);

/**
 * Get triangle indices for rendering
 */
const int* deformable_mesh_get_triangle_indices(const DeformableMesh *mesh);

/**
 * Update vertex normals from current deformation
 */
void deformable_mesh_update_normals(DeformableMesh *mesh);

/**
 * Get deformed mesh data for GPU upload
 */
void deformable_mesh_get_gpu_data(const DeformableMesh *mesh, void **vertex_data, int *vertex_count,
                                  void **index_data, int *index_count);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_DEFORMABLE_MESH_H */
