/**
 * @file connectivity_graph.c
 * @brief Connectivity graph implementation for structural integrity and destruction
 */

#include "connectivity_graph.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// ========================================
// Constants
// ========================================

#define DEFAULT_MAX_NODES 4096
#define DEFAULT_MAX_EDGES 8192
#define MIN_STRESS_THRESHOLD 0.001f
#define FRACTURE_QUEUE_INITIAL_SIZE 64

// ========================================
// Vector Math Utilities
// ========================================

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
// Internal Helper Functions
// ========================================

static uint32_t graph_allocate_node_id(ConnectivityGraph *graph) {
    if (graph->node_count >= graph->node_capacity) {
        // Double capacity
        uint32_t new_capacity = graph->node_capacity * 2;
        GraphNode *new_nodes = (GraphNode *)realloc(graph->nodes, new_capacity * sizeof(GraphNode));
        if (!new_nodes) return UINT32_MAX;
        
        graph->nodes = new_nodes;
        graph->node_capacity = new_capacity;
    }
    
    return graph->node_count++;
}

static uint32_t graph_allocate_edge_id(ConnectivityGraph *graph) {
    if (graph->edge_count >= graph->edge_capacity) {
        // Double capacity
        uint32_t new_capacity = graph->edge_capacity * 2;
        GraphEdge *new_edges = (GraphEdge *)realloc(graph->edges, new_capacity * sizeof(GraphEdge));
        if (!new_edges) return UINT32_MAX;
        
        graph->edges = new_edges;
        graph->edge_capacity = new_capacity;
    }
    
    return graph->edge_count++;
}

static void graph_update_node_neighbors(ConnectivityGraph *graph, uint32_t node_id) {
    GraphNode *node = &graph->nodes[node_id];
    
    // Clear existing neighbors
    node->neighbor_count = 0;
    
    // Find all connected edges
    for (int i = 0; i < graph->edge_count; i++) {
        GraphEdge *edge = &graph->edges[i];
        if (!edge->is_active) continue;
        
        uint32_t neighbor_id = UINT32_MAX;
        if (edge->node_a == node_id) {
            neighbor_id = edge->node_b;
        } else if (edge->node_b == node_id) {
            neighbor_id = edge->node_a;
        }
        
        if (neighbor_id != UINT32_MAX) {
            // Add neighbor if we have capacity
            if (node->neighbor_count < node->neighbor_capacity) {
                node->neighbors[node->neighbor_count++] = neighbor_id;
            }
        }
    }
}

static void graph_calculate_edge_stress(GraphEdge *edge, const GraphNode *node_a, const GraphNode *node_b) {
    // Calculate current length
    float delta[3];
    vec3_sub(delta, node_b->position, node_a->position);
    float current_length = vec3_length(delta);
    
    if (current_length < 0.0001f) {
        edge->stress = 0.0f;
        edge->strain = 0.0f;
        return;
    }
    
    edge->current_length = current_length;
    
    // Calculate strain
    edge->strain = (current_length - edge->rest_length) / edge->rest_length;
    
    // Calculate stress (simplified)
    edge->stress = fabsf(edge->strain) * edge->stiffness;
    
    // Accumulate damage
    if (edge->stress > edge->strength) {
        float damage_rate = (edge->stress - edge->strength) / edge->strength;
        edge->damage += damage_rate * 0.016f; // Assuming 60 FPS
        
        if (edge->damage >= 1.0f) {
            edge->is_broken = true;
            edge->is_active = false;
        }
    }
}

static void graph_calculate_node_stress(GraphNode *node, ConnectivityGraph *graph) {
    if (node->is_broken || !node->is_active) return;
    
    // Reset stress
    node->stress[0] = node->stress[1] = node->stress[2] = 0.0f;
    
    // Accumulate stress from connected edges
    float total_stress = 0.0f;
    int active_connections = 0;
    
    for (int i = 0; i < node->neighbor_count; i++) {
        uint32_t neighbor_id = node->neighbors[i];
        if (neighbor_id >= graph->node_count) continue;
        
        GraphNode *neighbor = &graph->nodes[neighbor_id];
        if (neighbor->is_broken || !neighbor->is_active) continue;
        
        // Find edge between nodes
        for (int j = 0; j < graph->edge_count; j++) {
            GraphEdge *edge = &graph->edges[j];
            if (!edge->is_active) continue;
            
            if ((edge->node_a == node->id && edge->node_b == neighbor_id) ||
                (edge->node_b == node->id && edge->node_a == neighbor_id)) {
                total_stress += edge->stress;
                active_connections++;
                break;
            }
        }
    }
    
    // Calculate average stress
    if (active_connections > 0) {
        float avg_stress = total_stress / active_connections;
        node->stress[0] = node->stress[1] = node->stress[2] = avg_stress;
        
        // Accumulate damage
        if (avg_stress > node->strength) {
            float damage_rate = (avg_stress - node->strength) / node->strength;
            node->damage += damage_rate * 0.016f; // Assuming 60 FPS
            
            if (node->damage >= 1.0f) {
                node->is_broken = true;
                node->is_active = false;
            }
        }
    }
}

// ========================================
// Public API Implementation
// ========================================

GraphConfig connectivity_graph_get_default_config(void) {
    GraphConfig config = {
        .default_stiffness = 1000.0f,
        .default_damping = 0.1f,
        .default_strength = 50.0f,
        .fracture_threshold = 100.0f,
        .damage_accumulation_rate = 1.0f,
        .stress_propagation_factor = 0.8f,
        .solver_iterations = 5,
        .time_step = 1.0f / 60.0f,
        .gravity = {0.0f, -9.81f, 0.0f},
        .use_spatial_partitioning = true,
        .max_nodes_per_partition = 64,
        .partition_size = 2.0f,
        .enable_debug_output = false,
        .validate_integrity = true
    };
    return config;
}

ConnectivityGraph* connectivity_graph_create(int max_nodes, int max_edges, 
                                           const GraphConfig *config) {
    if (max_nodes <= 0) max_nodes = DEFAULT_MAX_NODES;
    if (max_edges <= 0) max_edges = DEFAULT_MAX_EDGES;
    
    ConnectivityGraph *graph = (ConnectivityGraph *)calloc(1, sizeof(ConnectivityGraph));
    if (!graph) return NULL;
    
    graph->nodes = (GraphNode *)calloc(max_nodes, sizeof(GraphNode));
    graph->edges = (GraphEdge *)calloc(max_edges, sizeof(GraphEdge));
    graph->fracture_queue = (uint32_t *)malloc(FRACTURE_QUEUE_INITIAL_SIZE * sizeof(uint32_t));
    
    if (!graph->nodes || !graph->edges || !graph->fracture_queue) {
        connectivity_graph_destroy(graph);
        return NULL;
    }
    
    graph->node_capacity = max_nodes;
    graph->edge_capacity = max_edges;
    graph->fracture_queue_capacity = FRACTURE_QUEUE_INITIAL_SIZE;
    
    if (config) {
        graph->config = *config;
    } else {
        graph->config = connectivity_graph_get_default_config();
    }
    
    // Initialize node neighbor arrays
    for (int i = 0; i < max_nodes; i++) {
        graph->nodes[i].neighbor_capacity = 16; // Default neighbor capacity
        graph->nodes[i].neighbors = (uint32_t *)malloc(16 * sizeof(uint32_t));
        if (!graph->nodes[i].neighbors) {
            connectivity_graph_destroy(graph);
            return NULL;
        }
    }
    
    return graph;
}

void connectivity_graph_destroy(ConnectivityGraph *graph) {
    if (graph) {
        if (graph->nodes) {
            for (int i = 0; i < graph->node_capacity; i++) {
                free(graph->nodes[i].neighbors);
            }
            free(graph->nodes);
        }
        
        free(graph->edges);
        free(graph->fracture_queue);
        
        if (graph->analysis.critical_nodes) {
            free(graph->analysis.critical_nodes);
            free(graph->analysis.fracture_probabilities);
        }
        
        free(graph);
    }
}

uint32_t connectivity_graph_add_node(ConnectivityGraph *graph, NodeType type, 
                                   const float *position, float mass, int element_index) {
    if (!graph || !position) return UINT32_MAX;
    
    uint32_t node_id = graph_allocate_node_id(graph);
    if (node_id == UINT32_MAX) return UINT32_MAX;
    
    GraphNode *node = &graph->nodes[node_id];
    
    node->id = node_id;
    node->type = type;
    node->element_index = element_index;
    
    vec3_copy(node->position, position);
    node->mass = mass;
    node->volume = mass / graph->config.default_strength; // Simplified
    
    node->stress[0] = node->stress[1] = node->stress[2] = 0.0f;
    node->strain[0] = node->strain[1] = node->strain[2] = 0.0f;
    node->damage = 0.0f;
    node->strength = graph->config.default_strength;
    
    node->is_broken = false;
    node->is_anchor = false;
    node->is_active = true;
    
    node->force[0] = node->force[1] = node->force[2] = 0.0f;
    node->reaction_force[0] = node->reaction_force[1] = node->reaction_force[2] = 0.0f;
    
    graph->total_mass += mass;
    graph->analysis_dirty = true;
    
    return node_id;
}

uint32_t connectivity_graph_add_edge(ConnectivityGraph *graph, uint32_t node_a, uint32_t node_b,
                                   float stiffness, float damping) {
    if (!graph || node_a >= graph->node_count || node_b >= graph->node_count) {
        return UINT32_MAX;
    }
    
    uint32_t edge_id = graph_allocate_edge_id(graph);
    if (edge_id == UINT32_MAX) return UINT32_MAX;
    
    GraphEdge *edge = &graph->edges[edge_id];
    
    edge->id = edge_id;
    edge->node_a = node_a;
    edge->node_b = node_b;
    
    // Calculate rest length
    GraphNode *node_a_ptr = &graph->nodes[node_a];
    GraphNode *node_b_ptr = &graph->nodes[node_b];
    float delta[3];
    vec3_sub(delta, node_b_ptr->position, node_a_ptr->position);
    edge->rest_length = vec3_length(delta);
    edge->current_length = edge->rest_length;
    
    edge->stiffness = stiffness > 0.0f ? stiffness : graph->config.default_stiffness;
    edge->damping = damping >= 0.0f ? damping : graph->config.default_damping;
    
    edge->stress = 0.0f;
    edge->strain = 0.0f;
    edge->damage = 0.0f;
    edge->strength = graph->config.default_strength;
    
    edge->is_broken = false;
    edge->is_active = true;
    
    edge->fracture_angle = 0.0f;
    edge->fracture_threshold = graph->config.fracture_threshold;
    
    // Update neighbor lists
    graph_update_node_neighbors(graph, node_a);
    graph_update_node_neighbors(graph, node_b);
    
    return edge_id;
}

void connectivity_graph_update(ConnectivityGraph *graph, float dt) {
    if (!graph || !graph->simulation_active) return;
    
    // Apply forces to nodes
    for (int i = 0; i < graph->node_count; i++) {
        GraphNode *node = &graph->nodes[i];
        if (!node->is_active || node->is_broken || node->is_anchor) continue;
        
        // Apply gravity
        vec3_add(node->force, node->force, graph->config.gravity);
    }
    
    // Update edge stresses
    for (int i = 0; i < graph->edge_count; i++) {
        GraphEdge *edge = &graph->edges[i];
        if (!edge->is_active || edge->is_broken) continue;
        
        uint32_t node_a = edge->node_a;
        uint32_t node_b = edge->node_b;
        
        if (node_a < graph->node_count && node_b < graph->node_count) {
            graph_calculate_edge_stress(edge, &graph->nodes[node_a], &graph->nodes[node_b]);
        }
    }
    
    // Update node stresses
    for (int i = 0; i < graph->node_count; i++) {
        graph_calculate_node_stress(&graph->nodes[i], graph);
    }
    
    // Process fractures
    connectivity_graph_process_fractures(graph);
    
    // Mark analysis as dirty
    graph->analysis_dirty = true;
}

void connectivity_graph_apply_node_force(ConnectivityGraph *graph, uint32_t node_id, 
                                       const float *force) {
    if (!graph || !force || node_id >= graph->node_count) return;
    
    GraphNode *node = &graph->nodes[node_id];
    if (node->is_active && !node->is_broken) {
        vec3_add(node->force, node->force, force);
    }
}

void connectivity_graph_apply_node_impulse(ConnectivityGraph *graph, uint32_t node_id, 
                                          const float *impulse) {
    if (!graph || !impulse || node_id >= graph->node_count) return;
    
    GraphNode *node = &graph->nodes[node_id];
    if (node->is_active && !node->is_broken && node->mass > 0.0f) {
        float velocity_impulse[3];
        vec3_mul(velocity_impulse, impulse, 1.0f / node->mass);
        // In a full implementation, this would affect node velocity
    }
}

void connectivity_graph_apply_force(ConnectivityGraph *graph, const float *force) {
    if (!graph || !force) return;
    
    for (int i = 0; i < graph->node_count; i++) {
        connectivity_graph_apply_node_force(graph, i, force);
    }
}

void connectivity_graph_set_anchor(ConnectivityGraph *graph, uint32_t node_id, bool anchored) {
    if (!graph || node_id >= graph->node_count) return;
    
    graph->nodes[node_id].is_anchor = anchored;
    graph->analysis_dirty = true;
}

void connectivity_graph_break_connection(ConnectivityGraph *graph, uint32_t edge_id) {
    if (!graph || edge_id >= graph->edge_count) return;
    
    GraphEdge *edge = &graph->edges[edge_id];
    edge->is_broken = true;
    edge->is_active = false;
    
    // Update neighbor lists
    graph_update_node_neighbors(graph, edge->node_a);
    graph_update_node_neighbors(graph, edge->node_b);
    
    graph->analysis_dirty = true;
}

void connectivity_graph_break_node(ConnectivityGraph *graph, uint32_t node_id) {
    if (!graph || node_id >= graph->node_count) return;
    
    GraphNode *node = &graph->nodes[node_id];
    node->is_broken = true;
    node->is_active = false;
    
    // Break all connected edges
    for (int i = 0; i < graph->edge_count; i++) {
        GraphEdge *edge = &graph->edges[i];
        if (edge->is_active && (edge->node_a == node_id || edge->node_b == node_id)) {
            edge->is_broken = true;
            edge->is_active = false;
        }
    }
    
    graph->analysis_dirty = true;
}

void connectivity_graph_analyze_stress(ConnectivityGraph *graph) {
    if (!graph) return;
    
    if (!graph->analysis_dirty) return; // Use cached results
    
    StressAnalysis *analysis = &graph->analysis;
    
    // Calculate stress metrics
    float max_stress = 0.0f;
    float total_stress = 0.0f;
    int active_nodes = 0;
    
    for (int i = 0; i < graph->node_count; i++) {
        GraphNode *node = &graph->nodes[i];
        if (!node->is_active || node->is_broken) continue;
        
        float node_stress = vec3_length(node->stress);
        max_stress = fmaxf(max_stress, node_stress);
        total_stress += node_stress;
        active_nodes++;
    }
    
    analysis->max_stress = max_stress;
    analysis->avg_stress = active_nodes > 0 ? total_stress / active_nodes : 0.0f;
    
    // Calculate structural integrity
    float intact_connections = 0.0f;
    float total_connections = 0.0f;
    
    for (int i = 0; i < graph->edge_count; i++) {
        GraphEdge *edge = &graph->edges[i];
        total_connections += 1.0f;
        if (edge->is_active && !edge->is_broken) {
            intact_connections += 1.0f;
        }
    }
    
    analysis->structural_integrity = total_connections > 0.0f ? 
                                     intact_connections / total_connections : 0.0f;
    
    // Find critical nodes (near fracture)
    int critical_count = 0;
    for (int i = 0; i < graph->node_count; i++) {
        GraphNode *node = &graph->nodes[i];
        if (!node->is_active || node->is_broken) continue;
        
        float node_stress = vec3_length(node->stress);
        if (node_stress > node->strength * 0.8f) { // 80% of strength
            critical_count++;
        }
    }
    
    analysis->critical_nodes_count = critical_count;
    
    graph->analysis_dirty = false;
}

const StressAnalysis* connectivity_graph_get_stress_analysis(ConnectivityGraph *graph) {
    if (!graph) return NULL;
    
    connectivity_graph_analyze_stress(graph);
    return &graph->analysis;
}

float connectivity_graph_calculate_integrity(ConnectivityGraph *graph) {
    if (!graph) return 0.0f;
    
    connectivity_graph_analyze_stress(graph);
    return graph->analysis.structural_integrity;
}

int connectivity_graph_find_weak_points(ConnectivityGraph *graph, uint32_t *weak_points, 
                                       int max_points) {
    if (!graph || !weak_points || max_points <= 0) return 0;
    
    connectivity_graph_analyze_stress(graph);
    
    int weak_count = 0;
    
    for (int i = 0; i < graph->node_count && weak_count < max_points; i++) {
        GraphNode *node = &graph->nodes[i];
        if (!node->is_active || node->is_broken) continue;
        
        float node_stress = vec3_length(node->stress);
        if (node_stress > node->strength * 0.9f) { // 90% of strength
            weak_points[weak_count++] = i;
        }
    }
    
    return weak_count;
}

void connectivity_graph_trigger_fracture(ConnectivityGraph *graph, uint32_t node_id, 
                                        const float *impact_point, float impact_force) {
    if (!graph || node_id >= graph->node_count) return;
    
    GraphNode *node = &graph->nodes[node_id];
    if (node->is_broken || !node->is_active) return;
    
    // Apply impact force
    float impact_vector[3];
    if (impact_point) {
        vec3_sub(impact_vector, impact_point, node->position);
        vec3_normalize(impact_vector, impact_vector);
    } else {
        impact_vector[0] = 0.0f;
        impact_vector[1] = 1.0f;
        impact_vector[2] = 0.0f;
    }
    
    vec3_mul(impact_vector, impact_vector, impact_force);
    connectivity_graph_apply_node_force(graph, node_id, impact_vector);
    
    // Add to fracture queue
    if (graph->fracture_queue_count < graph->fracture_queue_capacity) {
        graph->fracture_queue[graph->fracture_queue_count++] = node_id;
    }
}

void connectivity_graph_process_fractures(ConnectivityGraph *graph) {
    if (!graph) return;
    
    for (int i = 0; i < graph->fracture_queue_count; i++) {
        uint32_t node_id = graph->fracture_queue[i];
        
        if (node_id < graph->node_count) {
            GraphNode *node = &graph->nodes[node_id];
            
            // Check if node should fracture
            float node_stress = vec3_length(node->stress);
            if (node_stress > node->strength || node->damage >= 1.0f) {
                connectivity_graph_break_node(graph, node_id);
            }
        }
    }
    
    // Clear fracture queue
    graph->fracture_queue_count = 0;
}

void connectivity_graph_get_stats(ConnectivityGraph *graph, int *node_count, int *edge_count,
                                  int *broken_nodes, int *broken_edges, float *integrity) {
    if (!graph) return;
    
    if (node_count) *node_count = graph->node_count;
    if (edge_count) *edge_count = graph->edge_count;
    
    int broken_nodes_count = 0;
    int broken_edges_count = 0;
    
    for (int i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].is_broken) broken_nodes_count++;
    }
    
    for (int i = 0; i < graph->edge_count; i++) {
        if (graph->edges[i].is_broken) broken_edges_count++;
    }
    
    if (broken_nodes) *broken_nodes = broken_nodes_count;
    if (broken_edges) *broken_edges = broken_edges_count;
    if (integrity) *integrity = connectivity_graph_calculate_integrity(graph);
}

bool connectivity_graph_validate(ConnectivityGraph *graph) {
    if (!graph) return false;
    
    // Check node indices in edges
    for (int i = 0; i < graph->edge_count; i++) {
        GraphEdge *edge = &graph->edges[i];
        if (edge->node_a >= graph->node_count || edge->node_b >= graph->node_count) {
            return false;
        }
    }
    
    // Check neighbor lists
    for (int i = 0; i < graph->node_count; i++) {
        GraphNode *node = &graph->nodes[i];
        for (int j = 0; j < node->neighbor_count; j++) {
            if (node->neighbors[j] >= graph->node_count) {
                return false;
            }
        }
    }
    
    return true;
}
