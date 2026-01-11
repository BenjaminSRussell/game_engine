/**
 * @file soft_body_constraints.h
 * @brief Soft body constraint rendering and visualization
 *
 * Implements rendering system for soft body constraints including distance
 * constraints, bending constraints, and collision constraints. Supports
 * various visualization modes and GPU-accelerated rendering.
 */

#ifndef RENDER_SOFT_BODY_CONSTRAINTS_H
#define RENDER_SOFT_BODY_CONSTRAINTS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct SoftBodyConstraintRenderer SoftBodyConstraintRenderer;
typedef struct SoftBody SoftBody;
typedef struct Constraint Constraint;

// ========================================
// Constraint Types for Rendering
// ========================================

typedef enum ConstraintRenderType {
    CONSTRAINT_RENDER_DISTANCE,     // Distance constraints (springs)
    CONSTRAINT_RENDER_BENDING,      // Bending constraints
    CONSTRAINT_RENDER_COLLISION,    // Collision constraints
    CONSTRAINT_RENDER_VOLUME,       // Volume preservation constraints
    CONSTRAINT_RENDER_SKINNING      // Skinning constraints
} ConstraintRenderType;

// ========================================
// Visualization Modes
// ========================================

typedef enum VisualizationMode {
    VIZ_NONE,                      // No constraint visualization
    VIZ_WIREFRAME,                 // Wireframe lines
    VIZ_THICK_LINES,               // Thick lines with width based on stress
    VIZ_TUBES,                     // Cylindrical tubes around constraints
    VIZ_STRESS_COLOR,              // Color-coded by stress/strain
    VIZ_DEFORMATION_COLOR,         // Color-coded by deformation
    VIZ_VELOCITY_COLOR,            // Color-coded by velocity
    VIZ_ACTIVE_ONLY,               // Only show active/constrained elements
    VIX_HIGHLIGHT_BROKEN           // Highlight broken/fractured constraints
} VisualizationMode;

// ========================================
// Render Data for Constraint
// ========================================

typedef struct ConstraintRenderData {
    // Geometry
    float start_point[3];          // Start position in world space
    float end_point[3];            // End position in world space
    float mid_point[3];            // Midpoint (for curved constraints)
    
    // Physical properties
    float rest_length;             // Rest length
    float current_length;          // Current length
    float strain;                  // Strain (current_length / rest_length - 1)
    float stress;                  // Stress (force per unit area)
    float stiffness;               // Constraint stiffness
    
    // Velocity
    float velocity[3];             // Relative velocity
    float angular_velocity[3];     // Angular velocity (for bending)
    
    // Visual properties
    float thickness;               // Visual thickness
    float color[4];                // RGBA color
    float emissive_color[4];       // Emissive color for heated constraints
    bool visible;                  // Visibility flag
    bool broken;                   // Broken/fractured flag
    
    // Constraint type
    ConstraintRenderType type;
    
    // User data
    void *user_data;
    
} ConstraintRenderData;

// ========================================
// Rendering Configuration
// ========================================

typedef struct ConstraintRenderConfig {
    // Visualization settings
    VisualizationMode mode;       // Current visualization mode
    float line_width;              // Base line width
    float tube_radius;             // Tube radius for 3D rendering
    bool show_inactive;            // Show inactive constraints
    bool show_broken;              // Show broken constraints
    
    // Color settings
    float default_color[4];        // Default constraint color
    float active_color[4];         // Active constraint color
    float broken_color[4];         // Broken constraint color
    float high_stress_color[4];    // High stress color
    float low_stress_color[4];     // Low stress color
    
    // Stress visualization
    float stress_threshold_low;     // Low stress threshold
    float stress_threshold_high;    // High stress threshold
    bool stress_gradient;          // Use gradient coloring
    
    // Performance settings
    bool use_gpu_instancing;       // Use GPU instancing for rendering
    bool frustum_culling;          // Enable frustum culling
    bool level_of_detail;          // Enable level of detail
    float max_render_distance;     // Maximum render distance
    
    // Animation settings
    bool animate_thickness;        // Animate thickness based on stress
    bool animate_color;            // Animate color based on stress
    float animation_speed;         // Animation speed multiplier
    
} ConstraintRenderConfig;

// ========================================
// Batch Rendering Data
// ========================================

typedef struct ConstraintBatch {
    ConstraintRenderData *constraints; // Constraint data array
    int constraint_count;          // Number of constraints in batch
    int constraint_capacity;       // Capacity of constraint array
    
    // GPU resources
    uint32_t vertex_buffer;        // GPU vertex buffer
    uint32_t index_buffer;         // GPU index buffer
    uint32_t instance_buffer;      // GPU instance buffer
    bool gpu_dirty;                 // GPU data needs update
    
    // Bounding volume
    float bounds_min[3];            // Batch bounding box min
    float bounds_max[3];            // Batch bounding box max
    bool bounds_dirty;              // Bounds need recalculation
    
    // Rendering state
    bool visible;                   // Batch is visible
    float distance_to_camera;       // Distance to camera for LOD
    
} ConstraintBatch;

// ========================================
// Renderer Structure
// ========================================

struct SoftBodyConstraintRenderer {
    // Rendering batches
    ConstraintBatch *batches;
    int batch_count;
    int batch_capacity;
    
    // Configuration
    ConstraintRenderConfig config;
    
    // Camera data
    float camera_position[3];       // Camera position
    float camera_view_matrix[16];   // View matrix
    float camera_proj_matrix[16];   // Projection matrix
    float camera_frustum[6][4];     // Camera frustum planes
    
    // Rendering statistics
    int total_constraints_rendered;  // Total constraints rendered this frame
    int total_batches_rendered;     // Total batches rendered
    int culled_constraints;         // Constraints culled by frustum
    float render_time_ms;           // Time spent rendering (milliseconds)
    
    // Material system
    uint32_t default_material;      // Default material ID
    uint32_t wireframe_material;    // Wireframe material ID
    uint32_t tube_material;         // Tube material ID
    uint32_t stress_material;       // Stress-based material ID
    
    // Shader programs
    uint32_t line_shader;           // Line rendering shader
    uint32_t tube_shader;           // Tube rendering shader
    uint32_t stress_shader;         // Stress visualization shader
    
    // GPU resources
    uint32_t global_uniform_buffer; // Global uniform buffer
    bool gpu_resources_initialized;  // GPU resources are ready
    
    // Debug visualization
    bool debug_mode;                // Debug visualization enabled
    float debug_color[4];           // Debug overlay color
    bool show_constraint_ids;        // Show constraint IDs
    bool show_stress_values;         // Show stress values
    
};

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create soft body constraint renderer
 */
SoftBodyConstraintRenderer* soft_body_constraint_renderer_create(int max_batches, 
                                                               const ConstraintRenderConfig *config);

/**
 * Destroy constraint renderer
 */
void soft_body_constraint_renderer_destroy(SoftBodyConstraintRenderer *renderer);

// ========================================
// Soft Body Integration
// ========================================

/**
 * Add soft body to renderer
 */
int soft_body_constraint_renderer_add_body(SoftBodyConstraintRenderer *renderer, 
                                         const SoftBody *body);

/**
 * Remove soft body from renderer
 */
void soft_body_constraint_renderer_remove_body(SoftBodyConstraintRenderer *renderer, int body_id);

/**
 * Update soft body constraint data
 */
void soft_body_constraint_renderer_update_body(SoftBodyConstraintRenderer *renderer, 
                                             int body_id, const SoftBody *body);

/**
 * Update all soft bodies
 */
void soft_body_constraint_renderer_update_all(SoftBodyConstraintRenderer *renderer);

// ========================================
// Constraint Management
// ========================================

/**
 * Add individual constraint for rendering
 */
int soft_body_constraint_renderer_add_constraint(SoftBodyConstraintRenderer *renderer, 
                                               const ConstraintRenderData *constraint);

/**
 * Remove constraint from rendering
 */
void soft_body_constraint_renderer_remove_constraint(SoftBodyConstraintRenderer *renderer, 
                                                   int constraint_id);

/**
 * Update constraint data
 */
void soft_body_constraint_renderer_update_constraint(SoftBodyConstraintRenderer *renderer, 
                                                   int constraint_id, 
                                                   const ConstraintRenderData *constraint);

/**
 * Get constraint render data
 */
ConstraintRenderData* soft_body_constraint_renderer_get_constraint(SoftBodyConstraintRenderer *renderer, 
                                                               int constraint_id);

// ========================================
// Rendering Interface
// ========================================

/**
 * Render all constraints
 */
void soft_body_constraint_renderer_render(SoftBodyConstraintRenderer *renderer);

/**
 * Render specific batch
 */
void soft_body_constraint_renderer_render_batch(SoftBodyConstraintRenderer *renderer, int batch_id);

/**
 * Render constraints for specific soft body
 */
void soft_body_constraint_renderer_render_body(SoftBodyConstraintRenderer *renderer, int body_id);

/**
 * Render wireframe view
 */
void soft_body_constraint_renderer_render_wireframe(SoftBodyConstraintRenderer *renderer);

/**
 * Render stress visualization
 */
void soft_body_constraint_renderer_render_stress(SoftBodyConstraintRenderer *renderer);

// ========================================
// Camera and View Management
// ========================================

/**
 * Update camera matrices
 */
void soft_body_constraint_renderer_set_camera(SoftBodyConstraintRenderer *renderer, 
                                            const float *view_matrix, const float *proj_matrix,
                                            const float *camera_pos);

/**
 * Update frustum for culling
 */
void soft_body_constraint_renderer_update_frustum(SoftBodyConstraintRenderer *renderer);

/**
 * Check if constraint is visible
 */
bool soft_body_constraint_renderer_is_constraint_visible(SoftBodyConstraintRenderer *renderer, 
                                                       const ConstraintRenderData *constraint);

// ========================================
// Configuration Management
// ========================================

/**
 * Get default rendering configuration
 */
ConstraintRenderConfig soft_body_constraint_renderer_get_default_config(void);

/**
 * Update rendering configuration
 */
void soft_body_constraint_renderer_set_config(SoftBodyConstraintRenderer *renderer, 
                                             const ConstraintRenderConfig *config);

/**
 * Get current configuration
 */
ConstraintRenderConfig soft_body_constraint_renderer_get_config(SoftBodyConstraintRenderer *renderer);

/**
 * Set visualization mode
 */
void soft_body_constraint_renderer_set_mode(SoftBodyConstraintRenderer *renderer, 
                                           VisualizationMode mode);

// ========================================
// Material and Color Management
// ========================================

/**
 * Update constraint colors based on stress
 */
void soft_body_constraint_renderer_update_stress_colors(SoftBodyConstraintRenderer *renderer);

/**
 * Update constraint colors based on deformation
 */
void soft_body_constraint_renderer_update_deformation_colors(SoftBodyConstraintRenderer *renderer);

/**
 * Update constraint colors based on velocity
 */
void soft_body_constraint_renderer_update_velocity_colors(SoftBodyConstraintRenderer *renderer);

/**
 * Set custom color for constraint type
 */
void soft_body_constraint_renderer_set_type_color(SoftBodyConstraintRenderer *renderer, 
                                                 ConstraintRenderType type, const float *color);

// ========================================
// GPU Resource Management
// ========================================

/**
 * Initialize GPU resources
 */
bool soft_body_constraint_renderer_init_gpu(SoftBodyConstraintRenderer *renderer);

/**
 * Update GPU buffers
 */
void soft_body_constraint_renderer_update_gpu_buffers(SoftBodyConstraintRenderer *renderer);

/**
 * Cleanup GPU resources
 */
void soft_body_constraint_renderer_cleanup_gpu(SoftBodyConstraintRenderer *renderer);

/**
 * Check if GPU resources are available
 */
bool soft_body_constraint_renderer_gpu_available(SoftBodyConstraintRenderer *renderer);

// ========================================
// Utility Functions
// ========================================

/**
 * Calculate constraint stress
 */
float soft_body_constraint_calculate_stress(const ConstraintRenderData *constraint);

/**
 * Calculate constraint strain
 */
float soft_body_constraint_calculate_strain(const ConstraintRenderData *constraint);

/**
 * Get color for stress value
 */
void soft_body_constraint_stress_to_color(float stress, float min_stress, float max_stress, 
                                          float *color);

/**
 * Generate tube geometry for constraint
 */
void soft_body_constraint_generate_tube_geometry(const ConstraintRenderData *constraint, 
                                               int segments, float radius,
                                               float **vertices, int *vertex_count,
                                               int **indices, int *index_count);

/**
 * Get rendering statistics
 */
void soft_body_constraint_renderer_get_stats(SoftBodyConstraintRenderer *renderer, 
                                            int *total_constraints, int *rendered_constraints,
                                            int *culled_constraints, float *render_time);

/**
 * Reset rendering statistics
 */
void soft_body_constraint_renderer_reset_stats(SoftBodyConstraintRenderer *renderer);

/**
 * Validate renderer state
 */
bool soft_body_constraint_renderer_validate(SoftBodyConstraintRenderer *renderer);

// ========================================
// Debug and Analysis
// ========================================

/**
 * Enable debug visualization
 */
void soft_body_constraint_renderer_enable_debug(SoftBodyConstraintRenderer *renderer, bool enabled);

/**
 * Render debug information
 */
void soft_body_constraint_renderer_render_debug(SoftBodyConstraintRenderer *renderer);

/**
 * Export constraint data to file
 */
bool soft_body_constraint_renderer_export_data(SoftBodyConstraintRenderer *renderer, 
                                              const char *filename);

/**
 * Generate constraint report
 */
void soft_body_constraint_renderer_generate_report(SoftBodyConstraintRenderer *renderer, 
                                                 char *buffer, int buffer_size);

// ========================================
// Advanced Features
// ========================================

/**
 * Enable level of detail system
 */
void soft_body_constraint_renderer_enable_lod(SoftBodyConstraintRenderer *renderer, bool enabled);

/**
 * Set LOD levels and distances
 */
void soft_body_constraint_renderer_set_lod_levels(SoftBodyConstraintRenderer *renderer, 
                                                int levels, const float *distances);

/**
 * Enable instanced rendering
 */
void soft_body_constraint_renderer_enable_instancing(SoftBodyConstraintRenderer *renderer, bool enabled);

/**
 * Set instancing parameters
 */
void soft_body_constraint_renderer_set_instancing_params(SoftBodyConstraintRenderer *renderer, 
                                                       int instances_per_batch, bool gpu_update);

/**
 * Capture constraint rendering to texture
 */
uint32_t soft_body_constraint_renderer_capture_to_texture(SoftBodyConstraintRenderer *renderer, 
                                                          int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_SOFT_BODY_CONSTRAINTS_H */
