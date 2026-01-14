/**
 * @file connectivity_graph.h
 * @brief Connectivity graph for structural integrity and destruction
 *
 * Implements a connectivity graph system for tracking structural relationships
 * between mesh elements, calculating stress distribution, and determining fracture
 * propagation paths in destructible objects.
 */

#ifndef CONNECTIVITY_GRAPH_H
#define CONNECTIVITY_GRAPH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct ConnectivityGraph ConnectivityGraph;
typedef struct GraphNode GraphNode;
typedef struct GraphEdge GraphEdge;

// ========================================
// Node Types
// ========================================

typedef enum NodeType {
    NODE_VERTEX,                  // Single vertex node
    NODE_TRIANGLE,                // Triangle face node
    NODE_TETRAHEDRON,            // Tetrahedral volume node
    NODE_CLUSTER,                 // Cluster of elements
    NODE_ANCHOR                  // Fixed anchor point
} NodeType;

// ========================================
// Graph Node
// ========================================

struct GraphNode {
    // Node identification
    uint32_t id;                 // Unique node ID
    NodeType type;               // Node type
    int element_index;           // Index into mesh data
    
    // Physical properties
    float position[3];           // Node position in world space
    float mass;                 // Node mass
    float volume;                // Node volume
    float stress[3];             // Principal stresses
    float strain[3];             // Principal strains
    float damage;                // Accumulated damage (0-1)
    float strength;              // Material strength
    
    // Structural properties
    bool is_broken;              // Node has fractured
    bool is_anchor;              // Node is fixed/anchored
    bool is_active;              // Node participates in simulation
    
    // Connectivity
    uint32_t *neighbors;         // Neighbor node IDs
    int neighbor_count;          // Number of neighbors
    int neighbor_capacity;       // Neighbor array capacity
    
    // Forces
    float force[3];              // Applied forces
    float reaction_force[3];     // Reaction forces from constraints
    
    // User data
    void *user_data;
    
};

// ========================================
// Graph Edge
// ========================================

struct GraphEdge {
    // Edge identification
    uint32_t id;                 // Unique edge ID
    uint32_t node_a;             // First node ID
    uint32_t node_b;             // Second node ID
    
    // Physical properties
    float rest_length;           // Rest length of connection
    float current_length;         // Current length
    float stiffness;             // Connection stiffness
    float damping;               // Damping coefficient
    
    // Stress and strain
    float stress;                // Stress in connection
    float strain;                // Strain in connection
    float damage;                // Accumulated damage (0-1)
    float strength;              // Connection strength
    
    // State
    bool is_broken;              // Connection has fractured
    bool is_active;              // Connection participates in simulation
    
    // Fracture properties
    float fracture_angle;        // Preferred fracture angle
    float fracture_threshold;    // Stress threshold for fracture
    
    // User data
    void *user_data;
    
};

// ========================================
// Graph Configuration
// ========================================

typedef struct GraphConfig {
    // Structural parameters
    float default_stiffness;      // Default connection stiffness
    float default_damping;        // Default damping coefficient
    float default_strength;       // Default material strength
    
    // Fracture parameters
    float fracture_threshold;     // Global fracture threshold
    float damage_accumulation_rate; // Rate of damage accumulation
    float stress_propagation_factor; // Stress propagation between nodes
    
    // Simulation parameters
    int solver_iterations;       // Solver iterations per update
    float time_step;             // Fixed time step
    float gravity[3];            // Gravity vector
    
    // Performance parameters
    bool use_spatial_partitioning; // Use spatial partitioning for queries
    int max_nodes_per_partition;  // Maximum nodes per spatial partition
    float partition_size;        // Size of spatial partitions
    
    // Debug parameters
    bool enable_debug_output;     // Enable debug information
    bool validate_integrity;      // Validate graph integrity
    
} GraphConfig;

// ========================================
// Stress Analysis Data
// ========================================

typedef struct StressAnalysis {
    // Global stress metrics
    float max_stress;            // Maximum stress in graph
    float avg_stress;            // Average stress
    float total_strain_energy;   // Total strain energy
    
    // Fracture prediction
    int critical_nodes_count;    // Number of nodes near fracture
    uint32_t *critical_nodes;    // IDs of critical nodes
    float *fracture_probabilities; // Fracture probabilities for nodes
    
    // Structural integrity
    float structural_integrity;  // Overall structural integrity (0-1)
    float connectivity_factor;   // Connectivity measure
    int weak_points_count;       // Number of identified weak points
    
    // Load paths
    int primary_load_paths;      // Number of primary load paths
    uint32_t **load_paths;       // Load path node sequences
    
} StressAnalysis;

// ========================================
// Connectivity Graph Structure
// ========================================

struct ConnectivityGraph {
    // Graph data
    GraphNode *nodes;            // Node array
    int node_count;              // Number of nodes
    int node_capacity;           // Node array capacity
    
    GraphEdge *edges;            // Edge array
    int edge_count;              // Number of edges
    int edge_capacity;           // Edge array capacity
    
    // Configuration
    GraphConfig config;
    
    // Spatial partitioning
    void *spatial_grid;          // Spatial partitioning grid
    bool spatial_dirty;          // Spatial data needs update
    
    // Simulation state
    float total_mass;            // Total mass of system
    float center_of_mass[3];     // Center of mass
    bool simulation_active;
    
    // Analysis data
    StressAnalysis analysis;     // Current stress analysis
    bool analysis_dirty;         // Analysis needs update
    
    // Fracture data
    uint32_t *fracture_queue;    // Queue of nodes to fracture
    int fracture_queue_count;    // Number in fracture queue
    int fracture_queue_capacity; // Fracture queue capacity
    
    // Performance metrics
    float update_time_ms;        // Time spent updating (milliseconds)
    float analysis_time_ms;      // Time spent analyzing (milliseconds)
    int nodes_updated;           // Number of nodes updated per frame
    int edges_updated;           // Number of edges updated per frame
    
};

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create connectivity graph with specified capacity
 */
ConnectivityGraph* connectivity_graph_create(int max_nodes, int max_edges, 
                                           const GraphConfig *config);

/**
 * Destroy connectivity graph and free resources
 */
void connectivity_graph_destroy(ConnectivityGraph *graph);

// ========================================
// Graph Building
// ========================================

/**
 * Add node to graph
 */
uint32_t connectivity_graph_add_node(ConnectivityGraph *graph, NodeType type, 
                                   const float *position, float mass, int element_index);

/**
 * Add edge to graph
 */
uint32_t connectivity_graph_add_edge(ConnectivityGraph *graph, uint32_t node_a, uint32_t node_b,
                                   float stiffness, float damping);

/**
 * Build graph from mesh data
 */
void connectivity_graph_build_from_mesh(ConnectivityGraph *graph,
                                       const float *vertices, int vertex_count,
                                       const int *triangles, int triangle_count,
                                       float node_mass);

/**
 * Build tetrahedral graph from volume data
 */
void connectivity_graph_build_tetrahedral(ConnectivityGraph *graph,
                                          const float *vertices, int vertex_count,
                                          const int *tetrahedra, int tetrahedron_count,
                                          float node_mass);

// ========================================
// Graph Operations
// ========================================

/**
 * Update graph simulation
 */
void connectivity_graph_update(ConnectivityGraph *graph, float dt);

/**
 * Apply force to node
 */
void connectivity_graph_apply_node_force(ConnectivityGraph *graph, uint32_t node_id, 
                                       const float *force);

/**
 * Apply impulse to node
 */
void connectivity_graph_apply_node_impulse(ConnectivityGraph *graph, uint32_t node_id, 
                                          const float *impulse);

/**
 * Apply force to all nodes
 */
void connectivity_graph_apply_force(ConnectivityGraph *graph, const float *force);

/**
 * Set node as anchor
 */
void connectivity_graph_set_anchor(ConnectivityGraph *graph, uint32_t node_id, bool anchored);

/**
 * Break connection between nodes
 */
void connectivity_graph_break_connection(ConnectivityGraph *graph, uint32_t edge_id);

/**
 * Break node (fracture)
 */
void connectivity_graph_break_node(ConnectivityGraph *graph, uint32_t node_id);

// ========================================
// Stress Analysis
// ========================================

/**
 * Perform stress analysis
 */
void connectivity_graph_analyze_stress(ConnectivityGraph *graph);

/**
 * Get stress analysis results
 */
const StressAnalysis* connectivity_graph_get_stress_analysis(ConnectivityGraph *graph);

/**
 * Calculate structural integrity
 */
float connectivity_graph_calculate_integrity(ConnectivityGraph *graph);

/**
 * Find weak points in structure
 */
int connectivity_graph_find_weak_points(ConnectivityGraph *graph, uint32_t *weak_points, 
                                       int max_points);

/**
 * Predict fracture propagation
 */
int connectivity_graph_predict_fracture_path(ConnectivityGraph *graph, uint32_t start_node,
                                            uint32_t *path_nodes, int max_path_length);

// ========================================
// Fracture Simulation
// ========================================

/**
 * Process fracture queue
 */
void connectivity_graph_process_fractures(ConnectivityGraph *graph);

/**
 * Trigger fracture at node
 */
void connectivity_graph_trigger_fracture(ConnectivityGraph *graph, uint32_t node_id, 
                                        const float *impact_point, float impact_force);

/**
 * Apply radial fracture
 */
void connectivity_graph_apply_radial_fracture(ConnectivityGraph *graph, const float *center, 
                                            float radius, float force);

/**
 * Apply directional fracture
 */
void connectivity_graph_apply_directional_fracture(ConnectivityGraph *graph, 
                                                const float *direction, float force);

// ========================================
// Query Operations
// ========================================

/**
 * Find nodes in radius
 */
int connectivity_graph_find_nodes_in_radius(ConnectivityGraph *graph, const float *center, 
                                          float radius, uint32_t *node_ids, int max_count);

/**
 * Find connected components
 */
int connectivity_graph_find_components(ConnectivityGraph *graph, uint32_t *component_ids, 
                                      int max_components);

/**
 * Check if nodes are connected
 */
bool connectivity_graph_are_nodes_connected(ConnectivityGraph *graph, uint32_t node_a, 
                                            uint32_t node_b);

/**
 * Find shortest path between nodes
 */
int connectivity_graph_find_path(ConnectivityGraph *graph, uint32_t start_node, 
                                 uint32_t end_node, uint32_t *path_nodes, int max_path_length);

/**
 * Get node neighbors
 */
int connectivity_graph_get_node_neighbors(ConnectivityGraph *graph, uint32_t node_id, 
                                         uint32_t *neighbors, int max_neighbors);

// ========================================
// Utility Functions
// ========================================

/**
 * Get default graph configuration
 */
GraphConfig connectivity_graph_get_default_config(void);

/**
 * Update graph configuration
 */
void connectivity_graph_set_config(ConnectivityGraph *graph, const GraphConfig *config);

/**
 * Get graph statistics
 */
void connectivity_graph_get_stats(ConnectivityGraph *graph, int *node_count, int *edge_count,
                                  int *broken_nodes, int *broken_edges, float *integrity);

/**
 * Validate graph integrity
 */
bool connectivity_graph_validate(ConnectivityGraph *graph);

/**
 * Reset graph state
 */
void connectivity_graph_reset(ConnectivityGraph *graph);

/**
 * Optimize graph performance
 */
void connectivity_graph_optimize(ConnectivityGraph *graph);

// ========================================
// Debug and Visualization
// ========================================

/**
 * Enable debug visualization
 */
void connectivity_graph_enable_debug(ConnectivityGraph *graph, bool enabled);

/**
 * Export graph data to file
 */
bool connectivity_graph_export_data(ConnectivityGraph *graph, const char *filename);

/**
 * Generate graph report
 */
void connectivity_graph_generate_report(ConnectivityGraph *graph, char *buffer, int buffer_size);

/**
 * Visualize stress distribution
 */
void connectivity_graph_visualize_stress(ConnectivityGraph *graph, float *colors, int color_count);

/**
 * Visualize fracture probability
 */
void connectivity_graph_visualize_fracture_probability(ConnectivityGraph *graph, 
                                                     float *colors, int color_count);

// ========================================
// Advanced Features
// ========================================

/**
 * Enable parallel processing
 */
void connectivity_graph_enable_parallel(ConnectivityGraph *graph, bool enabled);

/**
 * Set thread count for parallel processing
 */
void connectivity_graph_set_thread_count(ConnectivityGraph *graph, int thread_count);

/**
 * Save graph state to file
 */
bool connectivity_graph_save_state(ConnectivityGraph *graph, const char *filename);

/**
 * Load graph state from file
 */
bool connectivity_graph_load_state(ConnectivityGraph *graph, const char *filename);

/**
 * Merge two graphs
 */
bool connectivity_graph_merge(ConnectivityGraph *target, ConnectivityGraph *source, 
                             const float *transform);

/**
 * Split graph into components
 */
int connectivity_graph_split(ConnectivityGraph *graph, ConnectivityGraph **components,
                           int max_components);

#ifdef __cplusplus
}
#endif

#endif /* CONNECTIVITY_GRAPH_H */
