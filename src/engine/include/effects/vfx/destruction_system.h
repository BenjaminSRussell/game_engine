#ifndef DESTRUCTION_SYSTEM_H
#define DESTRUCTION_SYSTEM_H

#include "include/core/types.h"
#include "../math/vec3.h"
#include "../math/mat4.h"
#include "include/core/memory.h"

// Destruction system configuration
#define DESTRUCTION_MAX_PIECES 10000
#define DESTRUCTION_MAX_VORONOI_SITES 64
#define DESTRUCTION_MAX_FRACTURE_DEPTH 4
#define DESTRUCTION_MIN_PIECE_VOLUME 0.001f
#define DESTRUCTION_SLEEP_THRESHOLD 0.01f
#define DESTRUCTION_CONTACT_THRESHOLD 0.1f

// Fracture types
typedef enum {
    FRACTURE_VORONOI,
    FRACTURE_RECURSIVE,
    FRACTURE_SLICE,
    FRACTURE_PRESPLIT
} FractureType;

// Piece state
typedef enum {
    PIECE_STATE_ACTIVE,
    PIECE_STATE_SLEEPING,
    PIECE_STATIC
} PieceState;

// Voronoi site for fracture generation
typedef struct {
    Vec3 position;
    Vec3 normal;
    f32 weight;
    u32 id;
} VoronoiSite;

// Fracture piece
typedef struct {
    // Geometry
    u32 vertex_count;
    u32 index_count;
    Vec3* vertices;
    u32* indices;
    Vec3* normals;
    Vec2* uvs;
    
    // Physics properties
    Vec3 position;
    Vec3 velocity;
    Vec3 angular_velocity;
    f32 mass;
    Mat4 inertia_tensor;
    Mat4 transform;
    
    // State
    PieceState state;
    u32 piece_id;
    u32 parent_piece_id;
    u32 fracture_depth;
    
    // Material properties
    f32 structural_integrity;
    f32 stress_factor;
    u32 material_type;
    
    // Visual properties
    Vec3 color;
    f32 roughness;
    f32 metalness;
    bool has_inner_surface;
    
    // Timing
    f64 creation_time;
    f64 last_update_time;
    u32 sleep_frames;
    
    // GPU resources
    u32 vertex_buffer_id;
    u32 index_buffer_id;
    u32 material_id;
} FracturePiece;

// Island detection data
typedef struct {
    u32* piece_indices;
    u32 piece_count;
    Vec3 center_of_mass;
    f32 total_mass;
    bool is_supported;
} FractureIsland;

// Structural integrity data
typedef struct {
    u32 support_count;
    u32 connection_count;
    f32 stress_level;
    bool will_collapse;
    Vec3 collapse_direction;
} StructuralIntegrity;

// Breakable constraint between pieces
typedef struct {
    u32 piece_a_id;
    u32 piece_b_id;
    Vec3 connection_point;
    f32 break_threshold;
    f32 current_stress;
    bool is_broken;
} BreakableConstraint;

// Destruction system
typedef struct {
    // Piece management
    FracturePiece* pieces;
    u32 piece_count;
    u32 max_pieces;
    u32 next_piece_id;
    
    // Fracture generation
    VoronoiSite* voronoi_sites;
    u32 voronoi_site_count;
    u32 max_voronoi_sites;
    
    // Island detection
    FractureIsland* islands;
    u32 island_count;
    u32 max_islands;
    
    // Structural integrity
    StructuralIntegrity* integrity_data;
    BreakableConstraint* constraints;
    u32 constraint_count;
    u32 max_constraints;
    
    // Performance
    bool enable_fracture_lod;
    bool enable_piece_aggregation;
    f32 aggregation_threshold;
    
    // Statistics
    u32 total_fractures;
    u32 pieces_generated;
    u64 fracture_time_ms;
    u64 island_detection_time_ms;
} DestructionSystem;

// Core API functions
void destruction_system_init(DestructionSystem* system);
void destruction_system_shutdown(DestructionSystem* system);
void destruction_system_update(DestructionSystem* system, f32 delta_time);

// Fracture generation (TASK_1900-1903)
void destruction_generate_voronoi_fracture(DestructionSystem* system, Vec3 impact_point, Vec3 impact_direction, f32 impact_force);
void destruction_generate_recursive_fracture(DestructionSystem* system, u32 parent_piece_id, Vec3 fracture_point, f32 fracture_energy);
void destruction_load_presplit_mesh(DestructionSystem* system, const char* mesh_path);
void destruction_generate_slice_fracture(DestructionSystem* system, Vec3 slice_origin, Vec3 slice_normal, f32 slice_thickness);

// Island detection (TASK_1910)
void destruction_detect_islands(DestructionSystem* system);
void destruction_update_island_support(DestructionSystem* system);
bool destruction_is_piece_supported(DestructionSystem* system, u32 piece_id);

// Structural integrity (TASK_1911)
void destruction_compute_structural_integrity(DestructionSystem* system);
void destruction_check_collapse_conditions(DestructionSystem* system);
void destruction_apply_collapse_forces(DestructionSystem* system);

// Jointing system (TASK_1912)
void destruction_create_breakable_constraint(DestructionSystem* system, u32 piece_a_id, u32 piece_b_id, Vec3 connection_point, f32 break_threshold);
void destruction_update_constraints(DestructionSystem* system);
void destruction_break_constraint(DestructionSystem* system, u32 constraint_id);

// Dynamic to static transitions (TASK_1913)
void destruction_update_piece_states(DestructionSystem* system);
void destruction_set_piece_sleeping(DestructionSystem* system, u32 piece_id);
void destruction_set_piece_static(DestructionSystem* system, u32 piece_id);

// Inner surface generation (TASK_1920)
void destruction_generate_inner_surfaces(DestructionSystem* system, FracturePiece* piece);
void destruction_triangulate_cut_face(DestructionSystem* system, FracturePiece* piece, Vec3 cut_normal);

// Inner material support (TASK_1921)
void destruction_set_inner_material_properties(DestructionSystem* system, u32 piece_id, Vec3 color, f32 roughness, f32 metalness);
void destruction_apply_inner_material_shader(DestructionSystem* system, u32 piece_id);

// Particle effects (TASK_1922)
void destruction_emit_debris_particles(DestructionSystem* system, Vec3 position, Vec3 normal, f32 intensity);
void destruction_emit_dust_particles(DestructionSystem* system, Vec3 position, f32 volume);

// Edge wear shading (TASK_1923)
void destruction_compute_edge_wear(DestructionSystem* system, FracturePiece* piece);
void destruction_apply_edge_wear_shader(DestructionSystem* system, u32 piece_id);

// Fracture LOD (TASK_1930)
void destruction_set_fracture_lod_level(DestructionSystem* system, u32 lod_level);
u32 destruction_get_piece_lod_level(DestructionSystem* system, u32 piece_id);
void destruction_update_piece_geometry_lod(DestructionSystem* system, u32 piece_id);

// Piece aggregation (TASK_1931)
void destruction_aggregate_small_pieces(DestructionSystem* system);
void destruction_combine_pieces(DestructionSystem* system, u32* piece_indices, u32 count);

// Sleep thresholds (TASK_1932)
void destruction_set_sleep_threshold(DestructionSystem* system, f32 threshold);
bool destruction_should_piece_sleep(DestructionSystem* system, u32 piece_id);

// Network replication (TASK_1940-1941)
void destruction_replicate_fracture_event(DestructionSystem* system, Vec3 impact_point, Vec3 impact_direction, f32 impact_force, u32 seed);
void destruction_synchronize_debris_state(DestructionSystem* system);

// Editor tools (TASK_1950-1951)
void destruction_paint_weak_points(DestructionSystem* system, Vec3 position, f32 radius, f32 weakness);
void destruction_visualize_stress_heatmap(DestructionSystem* system);

// Utility functions
FracturePiece* destruction_get_piece(DestructionSystem* system, u32 piece_id);
void destruction_remove_piece(DestructionSystem* system, u32 piece_id);
bool destruction_is_piece_active(DestructionSystem* system, u32 piece_id);

// Physics integration
void destruction_apply_physics_forces(DestructionSystem* system);
void destruction_handle_piece_collisions(DestructionSystem* system);
void destruction_update_piece_transforms(DestructionSystem* system);

#endif // DESTRUCTION_SYSTEM_H
