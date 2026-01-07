/*
 * lod_generator.c
 * Automatic LOD mesh generation
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "lod_generator.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
// #include <core/memory.h> // Removing non-standard headers for stability
// #include <core/logger.h> 

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct vec3 { float x, y, z; } vec3_t;

// Symmetric 4x4 Matrix for Quadric Error Metric
typedef struct quadric {
    double a2, ab, ac, ad;
    double     b2, bc, bd;
    double         c2, cd;
    double             d2;
} quadric_t;

typedef struct lod_vertex {
    vec3_t position;
    vec3_t normal; // Used for boundary/feature detection
    uint32_t original_index;
    int32_t replacement_index; // -1 if active, else index of vertex it collapsed to
    quadric_t q;
    bool boundary;
    uint32_t adjacent_faces_count; // For ref count
} lod_vertex_t;

typedef struct lod_face {
    uint32_t v[3];
    vec3_t normal;
    bool removed;
} lod_face_t;

typedef struct lod_edge {
    uint32_t v0, v1;
    double error;
    vec3_t target_pos;
    bool valid;
    bool boundary; // Edge is a boundary (only 1 face)
} lod_edge_t;

typedef struct lod_mesh {
    lod_vertex_t* vertices;
    uint32_t vertex_count;
    lod_face_t* faces;
    uint32_t face_count;
    lod_edge_t* edges;
    uint32_t edge_count;
    uint32_t edge_capacity;
} lod_mesh_t;

// Min-Heap for edges
typedef struct edge_heap {
    uint32_t* indices; // Indices into lod_mesh->edges
    uint32_t count;
    uint32_t capacity;
    lod_edge_t* edge_store; // Reference
} edge_heap_t;

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static void quadric_zero(quadric_t* q) {
    memset(q, 0, sizeof(quadric_t));
}

static void quadric_init_plane(quadric_t* q, vec3_t n, double d) {
    q->a2 = n.x * n.x; q->ab = n.x * n.y; q->ac = n.x * n.z; q->ad = n.x * d;
    q->b2 = n.y * n.y; q->bc = n.y * n.z; q->bd = n.y * d;
    q->c2 = n.z * n.z; q->cd = n.z * d;
    q->d2 = d * d;
}

static void quadric_add(quadric_t* r, const quadric_t* a, const quadric_t* b) {
    r->a2 = a->a2 + b->a2; r->ab = a->ab + b->ab; r->ac = a->ac + b->ac; r->ad = a->ad + b->ad;
    r->b2 = a->b2 + b->b2; r->bc = a->bc + b->bc; r->bd = a->bd + b->bd;
    r->c2 = a->c2 + b->c2; r->cd = a->cd + b->cd;
    r->d2 = a->d2 + b->d2;
}

static double quadric_error(const quadric_t* q, vec3_t v) {
    return q->a2 * v.x * v.x + 2.0 * q->ab * v.x * v.y + 2.0 * q->ac * v.x * v.z + 2.0 * q->ad * v.x
         + q->b2 * v.y * v.y + 2.0 * q->bc * v.y * v.z + 2.0 * q->bd * v.y
         + q->c2 * v.z * v.z + 2.0 * q->cd * v.z
         + q->d2;
}

static vec3_t vec3_sub(vec3_t a, vec3_t b) { return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z}; }
static vec3_t vec3_cross(vec3_t a, vec3_t b) { return (vec3_t){a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
static float vec3_dot(vec3_t a, vec3_t b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static float vec3_length(vec3_t a) { return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z); }
static vec3_t vec3_normalize(vec3_t a) {
    float len = vec3_length(a);
    if (len < 1e-6f) return (vec3_t){0, 0, 0};
    return (vec3_t){a.x/len, a.y/len, a.z/len};
}

/* ============================================================================
 * HEAP IMPLEMENTATION
 * ============================================================================ */

static void heap_init(edge_heap_t* heap, uint32_t capacity, lod_edge_t* edge_store) {
    heap->indices = (uint32_t*)malloc(capacity * sizeof(uint32_t));
    heap->count = 0;
    heap->capacity = capacity;
    heap->edge_store = edge_store;
}

static void heap_destroy(edge_heap_t* heap) {
    free(heap->indices);
}

static void heap_swap(edge_heap_t* heap, uint32_t i, uint32_t j) {
    uint32_t temp = heap->indices[i];
    heap->indices[i] = heap->indices[j];
    heap->indices[j] = temp;
}

static void heap_bubble_up(edge_heap_t* heap, uint32_t index) {
    while (index > 0) {
        uint32_t parent = (index - 1) / 2;
        if (heap->edge_store[heap->indices[index]].error < heap->edge_store[heap->indices[parent]].error) {
            heap_swap(heap, index, parent);
            index = parent;
        } else {
            break;
        }
    }
}

static void heap_bubble_down(edge_heap_t* heap, uint32_t index) {
    while (true) {
        uint32_t left = 2 * index + 1;
        uint32_t right = 2 * index + 2;
        uint32_t smallest = index;

        if (left < heap->count && 
            heap->edge_store[heap->indices[left]].error < heap->edge_store[heap->indices[smallest]].error) {
            smallest = left;
        }
        if (right < heap->count && 
            heap->edge_store[heap->indices[right]].error < heap->edge_store[heap->indices[smallest]].error) {
            smallest = right;
        }

        if (smallest != index) {
            heap_swap(heap, index, smallest);
            index = smallest;
        } else {
            break;
        }
    }
}

static void heap_push(edge_heap_t* heap, uint32_t edge_index) {
    if (heap->count >= heap->capacity) return;
    heap->indices[heap->count] = edge_index;
    heap_bubble_up(heap, heap->count);
    heap->count++;
}

static uint32_t heap_pop(edge_heap_t* heap) {
    if (heap->count == 0) return UINT32_MAX;
    uint32_t result = heap->indices[0];
    heap->count--;
    heap->indices[0] = heap->indices[heap->count];
    heap_bubble_down(heap, 0);
    return result;
}

/* ============================================================================
 * MESH PROCESSING
 * ============================================================================ */

lod_generation_config_t lod_get_default_config(void) {
    lod_generation_config_t config = {
        .reduction_ratios = {0.75f, 0.50f, 0.25f, 0.10f},
        .quality_weights = {1.0f, 0.1f, 0.1f}, // High weight on position
        .preserve_boundaries = true,
        .preserve_seams = false,
        .preserve_features = true,
        .feature_angle = 60.0f,
        .weld_distance = 0.0001f,
        .normal_smoothing = 0.5f // Default to partial smoothing
    };
    return config;
}

// Compute initial quadrics for all vertices
static void compute_vertex_quadrics(lod_mesh_t* mesh, const lod_generation_config_t* config) {
    for (uint32_t i = 0; i < mesh->face_count; i++) {
        lod_face_t* f = &mesh->faces[i];
        
        vec3_t v0 = mesh->vertices[f->v[0]].position;
        vec3_t v1 = mesh->vertices[f->v[1]].position;
        vec3_t v2 = mesh->vertices[f->v[2]].position;
        
        vec3_t n = vec3_normalize(vec3_cross(vec3_sub(v1, v0), vec3_sub(v2, v0)));
        f->normal = n; // Store face normal
        double d = -vec3_dot(n, v0);
        
        quadric_t q;
        quadric_init_plane(&q, n, d);
        
        // Add triangle quadric to all 3 vertices (weighted by area for better results?)
        // Standard QEM just adds them.
        for (int j = 0; j < 3; j++) {
            quadric_add(&mesh->vertices[f->v[j]].q, &mesh->vertices[f->v[j]].q, &q);
            mesh->vertices[f->v[j]].adjacent_faces_count++;
        }
    }
    
    // Feature preservation: Add boundary constraints
    if (config->preserve_boundaries) {
        // Simple O(N^2) boundary finding for demo - Production needs half-edge or hash map
        // For simplicity in this step, we'll mark boundaries if edges belong to 1 face
        // TODO: Use more efficient topology structure (e.g. half-edge)
    }
}

static double compute_edge_error(lod_mesh_t* mesh, uint32_t v0, uint32_t v1, vec3_t* out_target, const lod_generation_config_t* config) {
    quadric_t q_bar;
    quadric_add(&q_bar, &mesh->vertices[v0].q, &mesh->vertices[v1].q);
    
    // Simple midpoint strategy for efficiency (Optimal placement involves solving system of linear equations)
    // For AAA, we'd solve for x,y,z where Gradient(Q(x,y,z)) = 0.
    // Fallback: try v0, v1, and midpoint.
    
    vec3_t p0 = mesh->vertices[v0].position;
    vec3_t p1 = mesh->vertices[v1].position;
    vec3_t mid = (vec3_t){(p0.x+p1.x)*0.5f, (p0.y+p1.y)*0.5f, (p0.z+p1.z)*0.5f};
    
    double e0 = quadric_error(&q_bar, p0);
    double e1 = quadric_error(&q_bar, p1);
    double em = quadric_error(&q_bar, mid);
    
    double min_error = em;
    *out_target = mid;
    
    if (e0 < min_error) { min_error = e0; *out_target = p0; }
    if (e1 < min_error) { min_error = e1; *out_target = p1; }
    
    // Penalize boundary/feature edges if configured
    if (config->preserve_features) {
         // Logic for feature preservation would check normal deviation
    }
    
    return min_error;
}

// Get final index resolving collapses
static uint32_t get_final_index(lod_mesh_t* mesh, uint32_t idx) {
    while (mesh->vertices[idx].replacement_index != -1) {
        idx = mesh->vertices[idx].replacement_index;
    }
    return idx;
}

mesh_data_t* qem_simplify_advanced(const mesh_data_t* source, 
                                  float target_ratio, 
                                  const lod_generation_config_t* config,
                                  float* out_error) {
    if (!source || target_ratio >= 1.0f) return NULL;
    
    // 1. Build internal mesh structure
    lod_mesh_t lmesh = {0};
    lmesh.vertex_count = source->vertex_count;
    lmesh.vertices = (lod_vertex_t*)calloc(lmesh.vertex_count, sizeof(lod_vertex_t));
    if (!lmesh.vertices) return NULL;
    
    // Parse source vertices (Assuming packed format for now, better to use vertex_format accessors)
    const uint8_t* src_verts = (const uint8_t*)source->vertex_data;
    for (uint32_t i = 0; i < lmesh.vertex_count; i++) {
        // Hack: Assuming Position is first float3 (standard)
        // Production: Use geometry_vertex_format_find_attribute
        const float* pos = (const float*)(src_verts + i * source->vertex_stride);
        lmesh.vertices[i].position = (vec3_t){pos[0], pos[1], pos[2]};
        lmesh.vertices[i].original_index = i;
        lmesh.vertices[i].replacement_index = -1;
    }
    
    // Parse indices
    lmesh.face_count = source->index_count / 3;
    lmesh.faces = (lod_face_t*)malloc(lmesh.face_count * sizeof(lod_face_t));
    
    if (source->index_type == INDEX_TYPE_UINT32) {
        const uint32_t* indices = (const uint32_t*)source->index_data;
        for (uint32_t i = 0; i < lmesh.face_count; i++) {
            lmesh.faces[i].v[0] = indices[i*3+0];
            lmesh.faces[i].v[1] = indices[i*3+1];
            lmesh.faces[i].v[2] = indices[i*3+2];
            lmesh.faces[i].removed = false;
        }
    } else {
        const uint16_t* indices = (const uint16_t*)source->index_data;
        for (uint32_t i = 0; i < lmesh.face_count; i++) {
            lmesh.faces[i].v[0] = indices[i*3+0];
            lmesh.faces[i].v[1] = indices[i*3+1];
            lmesh.faces[i].v[2] = indices[i*3+2];
            lmesh.faces[i].removed = false;
        }
    }
    
    // 2. Compute Quadrics
    compute_vertex_quadrics(&lmesh, config);
    
    // 3. Select Valid Edges & Compute Errors
    // Naive: All unique edges from faces.
    // For 100k tris, we can have 150k edges.
    uint32_t estimated_edges = lmesh.face_count * 3;
    lmesh.edges = (lod_edge_t*)malloc(estimated_edges * sizeof(lod_edge_t));
    edge_heap_t heap;
    heap_init(&heap, estimated_edges, lmesh.edges);
    
    for (uint32_t i = 0; i < lmesh.face_count; i++) {
        lod_face_t* f = &lmesh.faces[i];
        for (int j = 0; j < 3; j++) {
            uint32_t v0 = f->v[j];
            uint32_t v1 = f->v[(j+1)%3];
            if (v0 > v1) { uint32_t t=v0; v0=v1; v1=t; } // Canonical order
            
            // Check if edge already exists (Naive loop - slow for huge meshes, use Hash Map in prod)
            bool exists = false;
            // skipping check for demo speed, duplicates will just add redundancy not logic failure
            
            vec3_t target;
            double error = compute_edge_error(&lmesh, v0, v1, &target, config);
            
            lmesh.edges[lmesh.edge_count].v0 = v0;
            lmesh.edges[lmesh.edge_count].v1 = v1;
            lmesh.edges[lmesh.edge_count].error = error;
            lmesh.edges[lmesh.edge_count].target_pos = target;
            lmesh.edges[lmesh.edge_count].valid = true;
            
            heap_push(&heap, lmesh.edge_count);
            lmesh.edge_count++;
        }
    }
    
    // 4. Decimate
    uint32_t target_face_count = (uint32_t)(lmesh.face_count * target_ratio);
    uint32_t current_face_count = lmesh.face_count;
    
    while (current_face_count > target_face_count && heap.count > 0) {
        uint32_t edge_idx = heap_pop(&heap);
        lod_edge_t* edge = &lmesh.edges[edge_idx];
        
        if (!edge->valid) continue;
        
        // Find current roots
        uint32_t r0 = get_final_index(&lmesh, edge->v0);
        uint32_t r1 = get_final_index(&lmesh, edge->v1);
        
        if (r0 == r1) continue; // Already collapsed
        
        // Perform collapse r1 -> r0
        lmesh.vertices[r1].replacement_index = r0;
        lmesh.vertices[r0].position = edge->target_pos;
        quadric_add(&lmesh.vertices[r0].q, &lmesh.vertices[r0].q, &lmesh.vertices[r1].q);
        
        // Invalidate faces
        // (Expensive scan - in prod maintain vertex->face adjacency list)
        // Here we just count removed valid faces post-pass or lazily
        current_face_count -= 2; // Approximation, usually 2 triangles share an edge
    }
    
    // 5. Rebuild Result Mesh
    mesh_data_t* result = (mesh_data_t*)calloc(1, sizeof(mesh_data_t));
    result->vertex_stride = source->vertex_stride; // Keep same layout
    result->flags = source->flags;
    result->vertex_format = source->vertex_format;
    
    // Compact vertices (lazy approach for demo - keep all and update positions, or remove unused)
    // AAA standard: Remove unused vertices.
    // Map old_index -> new_index
    uint32_t* index_map = (uint32_t*)malloc(lmesh.vertex_count * sizeof(uint32_t));
    memset(index_map, 0xFF, lmesh.vertex_count * sizeof(uint32_t));
    
    uint32_t new_vertex_count = 0;
    // Iterate faces to find used vertices
    for (uint32_t i = 0; i < lmesh.face_count; i++) {
        lod_face_t* f = &lmesh.faces[i];
        uint32_t v0 = get_final_index(&lmesh, f->v[0]);
        uint32_t v1 = get_final_index(&lmesh, f->v[1]);
        uint32_t v2 = get_final_index(&lmesh, f->v[2]);
        
        if (v0 != v1 && v1 != v2 && v2 != v0) { // Degenerate check
            if (index_map[v0] == UINT32_MAX) index_map[v0] = new_vertex_count++;
            if (index_map[v1] == UINT32_MAX) index_map[v1] = new_vertex_count++;
            if (index_map[v2] == UINT32_MAX) index_map[v2] = new_vertex_count++;
        } else {
            f->removed = true;
        }
    }
    
    result->vertex_count = new_vertex_count;
    result->vertex_data_size = new_vertex_count * result->vertex_stride;
    result->vertex_data = malloc(result->vertex_data_size);
    
    // Fill vertex buffer
    for (uint32_t i = 0; i < lmesh.vertex_count; i++) {
        if (index_map[i] != UINT32_MAX) {
             // Copy old data first
             uint32_t original = lmesh.vertices[i].original_index; // Actually i
             memcpy((uint8_t*)result->vertex_data + index_map[i] * result->vertex_stride,
                    (uint8_t*)source->vertex_data + original * source->vertex_stride,
                    result->vertex_stride);
             
             // Update Position
             float* pos = (float*)((uint8_t*)result->vertex_data + index_map[i] * result->vertex_stride);
             pos[0] = lmesh.vertices[i].position.x;
             pos[1] = lmesh.vertices[i].position.y;
             pos[2] = lmesh.vertices[i].position.z;
        }
    }
    
    // Fill index buffer
    // Count valid faces first
    uint32_t final_faces = 0;
    for (uint32_t i = 0; i < lmesh.face_count; i++) { if (!lmesh.faces[i].removed) final_faces++; }
    
    result->index_count = final_faces * 3;
    result->index_type = source->index_type;
    size_t idx_size = (result->index_type == INDEX_TYPE_UINT32) ? 4 : 2;
    result->index_data_size = result->index_count * idx_size;
    result->index_data = malloc(result->index_data_size);
    
    uint32_t idx_ptr = 0;
    for (uint32_t i = 0; i < lmesh.face_count; i++) {
        lod_face_t* f = &lmesh.faces[i];
        if (f->removed) continue;
        
        uint32_t v0 = index_map[get_final_index(&lmesh, f->v[0])];
        uint32_t v1 = index_map[get_final_index(&lmesh, f->v[1])];
        uint32_t v2 = index_map[get_final_index(&lmesh, f->v[2])];
        
        if (result->index_type == INDEX_TYPE_UINT32) {
            ((uint32_t*)result->index_data)[idx_ptr++] = v0;
            ((uint32_t*)result->index_data)[idx_ptr++] = v1;
            ((uint32_t*)result->index_data)[idx_ptr++] = v2;
        } else {
            ((uint16_t*)result->index_data)[idx_ptr++] = (uint16_t)v0;
            ((uint16_t*)result->index_data)[idx_ptr++] = (uint16_t)v1;
            ((uint16_t*)result->index_data)[idx_ptr++] = (uint16_t)v2;
        }
    }
    
    // Cleanup
    free(lmesh.vertices);
    free(lmesh.faces);
    free(lmesh.edges);
    heap_destroy(&heap);
    free(index_map);
    
    return result;
}

mesh_data_t* qem_simplify(const mesh_data_t* source, float target_ratio) {
    lod_generation_config_t config = lod_get_default_config();
    return qem_simplify_advanced(source, target_ratio, &config, NULL);
}

/* ============================================================================
 * GPU UPLOAD (PLACEHOLDER)
 * ============================================================================ */

static mesh_gpu_data_t* mesh_upload_to_gpu_internal(const mesh_data_t* mesh) {
    if (!mesh) return NULL;
    mesh_gpu_data_t* gpu_data = (mesh_gpu_data_t*)calloc(1, sizeof(mesh_gpu_data_t));
    if (!gpu_data) return NULL;
    
    gpu_data->vertex_count = mesh->vertex_count;
    gpu_data->index_count = mesh->index_count;
    gpu_data->vertex_stride = mesh->vertex_stride;
    gpu_data->vertex_buffer = mesh->vertex_data;
    gpu_data->index_buffer = mesh->index_data;
    
    return gpu_data;
}

static void mesh_gpu_data_destroy(mesh_gpu_data_t* gpu_data) {
    if (!gpu_data) return;
    free(gpu_data);
}


/* ============================================================================
 * LOD CHAIN GENERATION
 * ============================================================================ */

lod_chain_t* lod_generate_chain_advanced(const mesh_data_t* source, 
                                        const lod_generation_config_t* config) {
    if (!source || !config) return NULL;
    
    lod_chain_t* chain = (lod_chain_t*)calloc(1, sizeof(lod_chain_t));
    if (!chain) return NULL;
    
    chain->config = *config;
    
    // LOD 0
    chain->lods[0] = mesh_upload_to_gpu_internal(source);
    chain->switch_distances[0] = 1.0f;
    chain->geometric_errors[0] = 0.0f;
    chain->lod_count = 1;
    
    // Generated LODs
    for (int i = 0; i < 4; i++) {
        float ratio = config->reduction_ratios[i];
        if (ratio <= 0.0f || ratio >= 1.0f) continue;
        
        float error = 0.0f;
        mesh_data_t* simplified = qem_simplify_advanced(source, ratio, config, &error);
        if (simplified) {
             chain->lods[chain->lod_count] = mesh_upload_to_gpu_internal(simplified);
             chain->switch_distances[chain->lod_count] = ratio * 0.5f; // Heuristic
             chain->geometric_errors[chain->lod_count] = error;
             chain->lod_count++;
             
             // In proper engine, ownership of buffers would be passed or copied
             // Here we simulate it
             free(simplified);
        }
    }
    
    return chain;
}


lod_chain_t* lod_generate_chain(const mesh_data_t* source, const float* ratios, uint32_t count) {
    lod_generation_config_t config = lod_get_default_config();
    // Override ratios
    for (uint32_t i = 0; i < count && i < 4; i++) {
        config.reduction_ratios[i] = ratios[i];
    }
    return lod_generate_chain_advanced(source, &config);
}


void lod_chain_destroy(lod_chain_t* chain) {
    if (!chain) return;
    for (uint32_t i = 0; i < chain->lod_count; i++) {
        // Free buffers that were allocated during simplification
        if (i > 0) {
            free(chain->lods[i]->vertex_buffer);
            free(chain->lods[i]->index_buffer);
        }
        mesh_gpu_data_destroy(chain->lods[i]);
    }
    free(chain);
}
