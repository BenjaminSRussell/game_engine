#include "geometry/mesh_optimize.h"
#include "geometry/mesh.h"
#include <core/memory.h>
#include <core/logger.h>
#include <string.h>
#include <include/math/math.h>

// ----------------------------------------------------------------------------
// Vertex Deduplication
// ----------------------------------------------------------------------------

typedef struct vertex_hash_entry {
    u32 original_index;
    u32 new_index;
    struct vertex_hash_entry* next;
} vertex_hash_entry_t;

static u32 vertex_hash(const vertex_t* v) {
    // Simple hash combining position components
    u32 h = 0;
    h ^= *(u32*)&v->position.x;
    h ^= *(u32*)&v->position.y;
    h ^= *(u32*)&v->position.z;
    return h;
}

static bool vertices_equal(const vertex_t* a, const vertex_t* b, f32 epsilon) {
    f32 dx = fabsf(a->position.x - b->position.x);
    f32 dy = fabsf(a->position.y - b->position.y);
    f32 dz = fabsf(a->position.z - b->position.z);
    
    if (dx > epsilon || dy > epsilon || dz > epsilon) {
        return false;
    }
    
    // Also check normals and UVs
    f32 dnx = fabsf(a->normal.x - b->normal.x);
    f32 dny = fabsf(a->normal.y - b->normal.y);
    f32 dnz = fabsf(a->normal.z - b->normal.z);
    
    if (dnx > epsilon || dny > epsilon || dnz > epsilon) {
        return false;
    }
    
    f32 du = fabsf(a->uv.x - b->uv.x);
    f32 dv = fabsf(a->uv.y - b->uv.y);
    
    return (du <= epsilon && dv <= epsilon);
}

void mesh_deduplicate_vertices(mesh_t* mesh, f32 epsilon) {
    if (!mesh || mesh->vertex_count == 0) return;
    
    u32 original_count = mesh->vertex_count;
    
    // Create hash table (simple fixed size for now)
    #define HASH_SIZE 4096
    vertex_hash_entry_t* hash_table[HASH_SIZE] = {0};
    
    // Build mapping from old to new indices
    u32* remap = (u32*)MALLOC_GEOMETRY(sizeof(u32) * mesh->vertex_count);
    vertex_t* new_vertices = (vertex_t*)MALLOC_GEOMETRY(sizeof(vertex_t) * mesh->vertex_count);
    u32 new_vertex_count = 0;
    
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        vertex_t* v = &mesh->vertices[i];
        u32 hash = vertex_hash(v) % HASH_SIZE;
        
        // Search for duplicate
        bool found = false;
        vertex_hash_entry_t* entry = hash_table[hash];
        while (entry) {
            if (vertices_equal(v, &mesh->vertices[entry->original_index], epsilon)) {
                remap[i] = entry->new_index;
                found = true;
                break;
            }
            entry = entry->next;
        }
        
        if (!found) {
            // Add new unique vertex
            new_vertices[new_vertex_count] = *v;
            remap[i] = new_vertex_count;
            
            // Add to hash table
            vertex_hash_entry_t* new_entry = (vertex_hash_entry_t*)MALLOC_GEOMETRY(sizeof(vertex_hash_entry_t));
            new_entry->original_index = i;
            new_entry->new_index = new_vertex_count;
            new_entry->next = hash_table[hash];
            hash_table[hash] = new_entry;
            
            new_vertex_count++;
        }
    }
    
    // Remap indices
    for (u32 i = 0; i < mesh->index_count; ++i) {
        mesh->indices[i] = remap[mesh->indices[i]];
    }
    
    // Replace vertices
    FREE(mesh->vertices);
    mesh->vertices = new_vertices;
    mesh->vertex_count = new_vertex_count;
    mesh->vertex_capacity = new_vertex_count;
    
    // Cleanup
    for (u32 i = 0; i < HASH_SIZE; ++i) {
        vertex_hash_entry_t* entry = hash_table[i];
        while (entry) {
            vertex_hash_entry_t* next = entry->next;
            FREE(entry);
            entry = next;
        }
    }
    FREE(remap);
    
    LOG_INFO("Mesh deduplication: %u -> %u vertices (%.1f%% reduction)",
        original_count, new_vertex_count,
        100.0f * (1.0f - (f32)new_vertex_count / (f32)original_count));
    
    #undef HASH_SIZE
}

// ----------------------------------------------------------------------------
// Index Optimization (Vertex Cache) - Forsyth Algorithm
// ----------------------------------------------------------------------------

#define VERTEX_CACHE_SIZE 32
#define CACHE_SCORE_TABLE_SIZE 32
#define VALENCE_SCORE_TABLE_SIZE 32

static f32 cache_score_table[CACHE_SCORE_TABLE_SIZE];
static f32 valence_score_table[VALENCE_SCORE_TABLE_SIZE];
static bool score_tables_initialized = false;

static void init_forsyth_score_tables() {
    if (score_tables_initialized) return;
    
    // Cache position scoring
    for (u32 i = 0; i < CACHE_SCORE_TABLE_SIZE; ++i) {
        if (i < 3) {
            cache_score_table[i] = 0.75f; // Last triangle bonus
        } else {
            f32 scaler = 1.0f / (CACHE_SCORE_TABLE_SIZE - 3);
            cache_score_table[i] = 1.0f - (i - 3) * scaler;
        }
    }
    
    // Valence (unused triangle count) scoring
    for (u32 i = 0; i < VALENCE_SCORE_TABLE_SIZE; ++i) {
        if (i < 2) {
            valence_score_table[i] = 0.0f;
        } else {
            f32 scaler = 1.0f / (VALENCE_SCORE_TABLE_SIZE - 2);
            valence_score_table[i] = (i - 1) * scaler;
        }
    }
    
    score_tables_initialized = true;
}

typedef struct vertex_data {
    u32 cache_position;  // Position in cache (-1 if not in cache)
    f32 score;
    u32 *triangles;      // List of triangles using this vertex
    u32 triangle_count;
    u32 triangles_left;  // How many triangles haven't been used
} vertex_data_t;

typedef struct triangle_data {
    bool added;
    f32 score;
    u32 vertices[3];
} triangle_data_t;

static f32 calculate_vertex_score(const vertex_data_t* v) {
    f32 score = 0.0f;
    
    // Cache position score
    if (v->cache_position < CACHE_SCORE_TABLE_SIZE) {
        score += cache_score_table[v->cache_position];
    }
    
    // Valence score
    u32 valence = v->triangles_left;
    if (valence < VALENCE_SCORE_TABLE_SIZE) {
        score += valence_score_table[valence];
    } else {
        score += valence_score_table[VALENCE_SCORE_TABLE_SIZE - 1];
    }
    
    return score;
}

void mesh_optimize_vertex_cache(mesh_t* mesh) {
    if (!mesh || mesh->index_count == 0) return;
    
    init_forsyth_score_tables();
    
    u32 triangle_count = mesh->index_count / 3;
    
    // Allocate working data
    vertex_data_t* vertices = (vertex_data_t*)calloc(mesh->vertex_count, sizeof(vertex_data_t));
    triangle_data_t* triangles = (triangle_data_t*)calloc(triangle_count, sizeof(triangle_data_t));
    u32* vertex_cache = (u32*)malloc(sizeof(u32) * VERTEX_CACHE_SIZE);
    u32* new_indices = (u32*)malloc(sizeof(u32) * mesh->index_count);
    
    if (!vertices || !triangles || !vertex_cache || !new_indices) {
        LOG_ERROR("Failed to allocate memory for vertex cache optimization");
        free(vertices);
        free(triangles);
        free(vertex_cache);
        free(new_indices);
        return;
    }
    
    // Initialize vertex cache
    for (u32 i = 0; i < VERTEX_CACHE_SIZE; ++i) {
        vertex_cache[i] = 0xFFFFFFFF;
    }
    
    // Build vertex→triangle adjacency
    for (u32 t = 0; t < triangle_count; ++t) {
        for (u32 i = 0; i < 3; ++i) {
            u32 v_idx = mesh->indices[t * 3 + i];
            vertices[v_idx].triangle_count++;
        }
    }
    
    for (u32 v = 0; v < mesh->vertex_count; ++v) {
        vertices[v].triangles = (u32*)malloc(sizeof(u32) * vertices[v].triangle_count);
        vertices[v].triangles_left = vertices[v].triangle_count;
        vertices[v].cache_position = 0xFFFFFFFF;
        u32 tri_idx = 0;
        
        for (u32 t = 0; t < triangle_count; ++t) {
            for (u32 i = 0; i < 3; ++i) {
                if (mesh->indices[t * 3 + i] == v) {
                    vertices[v].triangles[tri_idx++] = t;
                    break;
                }
            }
        }
    }
    
    // Initialize triangles
    for (u32 t = 0; t < triangle_count; ++t) {
        triangles[t].added = false;
        triangles[t].vertices[0] = mesh->indices[t * 3];
        triangles[t].vertices[1] = mesh->indices[t * 3 + 1];
        triangles[t].vertices[2] = mesh->indices[t * 3 + 2];
    }
    
    // Calculate initial scores
    for (u32 v = 0; v < mesh->vertex_count; ++v) {
        vertices[v].score = calculate_vertex_score(&vertices[v]);
    }
    
    for (u32 t = 0; t < triangle_count; ++t) {
        triangles[t].score = vertices[triangles[t].vertices[0]].score +
                             vertices[triangles[t].vertices[1]].score +
                             vertices[triangles[t].vertices[2]].score;
    }
    
    // Main optimization loop
    u32 output_pos = 0;
    u32 best_triangle = 0;
    
    for (u32 i = 0; i < triangle_count; ++i) {
        // Find best scoring triangle
        f32 best_score = -1.0f;
        
        for (u32 t = 0; t < triangle_count; ++t) {
            if (triangles[t].added) continue;
            
            if (triangles[t].score > best_score) {
                best_score = triangles[t].score;
                best_triangle = t;
            }
        }
        
        // Add triangle
        triangles[best_triangle].added = true;
        
        for (u32 j = 0; j < 3; ++j) {
            u32 v = triangles[best_triangle].vertices[j];
            new_indices[output_pos++] = v;
            vertices[v].triangles_left--;
        }
        
        // Update cache
        for (u32 j = 0; j < 3; ++j) {
            u32 v = triangles[best_triangle].vertices[j];
            
            // Move vertex to front of cache
            if (vertices[v].cache_position != 0) {
                // Remove from old position
                for (u32 k = 0; k < VERTEX_CACHE_SIZE; ++k) {
                    if (vertex_cache[k] == v) {
                        for (u32 m = k; m < VERTEX_CACHE_SIZE - 1; ++m) {
                            vertex_cache[m] = vertex_cache[m + 1];
                        }
                        vertex_cache[VERTEX_CACHE_SIZE - 1] = 0xFFFFFFFF;
                        break;
                    }
                }
                
                // Insert at front
                for (u32 k = VERTEX_CACHE_SIZE - 1; k > 0; --k) {
                    vertex_cache[k] = vertex_cache[k - 1];
                }
                vertex_cache[0] = v;
            }
        }
        
        // Update cache positions
        for (u32 j = 0; j < VERTEX_CACHE_SIZE; ++j) {
            if (vertex_cache[j] != 0xFFFFFFFF) {
                vertices[vertex_cache[j]].cache_position = j;
            }
        }
        
        // Recalculate scores for affected vertices
        for (u32 j = 0; j < 3; ++j) {
            u32 v = triangles[best_triangle].vertices[j];
            vertices[v].score = calculate_vertex_score(&vertices[v]);
            
            // Update triangle scores
            for (u32 k = 0; k < vertices[v].triangle_count; ++k) {
                u32 t_idx = vertices[v].triangles[k];
                if (!triangles[t_idx].added) {
                    triangles[t_idx].score = 
                        vertices[triangles[t_idx].vertices[0]].score +
                        vertices[triangles[t_idx].vertices[1]].score +
                        vertices[triangles[t_idx].vertices[2]].score;
                }
            }
        }
    }
    
    // Replace indices
    memcpy(mesh->indices, new_indices, sizeof(u32) * mesh->index_count);
    
    // Cleanup
    for (u32 v = 0; v < mesh->vertex_count; ++v) {
        free(vertices[v].triangles);
    }
    free(vertices);
    free(triangles);
    free(vertex_cache);
    free(new_indices);
    
    LOG_INFO("Vertex cache optimization applied to mesh '%s'", mesh->name);
}

void mesh_optimize_indices(mesh_t* mesh) {
    mesh_optimize_vertex_cache(mesh);
}


// ----------------------------------------------------------------------------
// Normal Packing (Octahedral Encoding)
// ----------------------------------------------------------------------------

// Octahedral encoding: Maps unit sphere to unit square
// Reduces normals from 12 bytes (3x float) to 2-4 bytes

static Vec2 octahedral_encode(Vec3 n) {
    // Project onto octahedron, then onto xy plane
    f32 sum = fabsf(n.x) + fabsf(n.y) + fabsf(n.z);
    n.x /= sum;
    n.y /= sum;
    n.z /= sum;
    
    if (n.z < 0.0f) {
        f32 old_x = n.x;
        n.x = (1.0f - fabsf(n.y)) * (old_x >= 0.0f ? 1.0f : -1.0f);
        n.y = (1.0f - fabsf(old_x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
    }
    
    return (Vec2){n.x, n.y};
}

static Vec3 octahedral_decode(Vec2 oct) {
    Vec3 n = {oct.x, oct.y, 1.0f - fabsf(oct.x) - fabsf(oct.y)};
    
    if (n.z < 0.0f) {
        f32 old_x = n.x;
        n.x = (1.0f - fabsf(n.y)) * (old_x >= 0.0f ? 1.0f : -1.0f);
        n.y = (1.0f - fabsf(old_x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
    }
    
    // Normalize
    f32 len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0.0001f) {
        n.x /= len;
        n.y /= len;
        n.z /= len;
    }
    
    return n;
}

void mesh_pack_normals_oct8(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0) return;
    
    // Allocate packed storage (2 bytes per normal)
    u8* packed = (u8*)MALLOC_GEOMETRY(mesh->vertex_count * 2);
    
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        Vec2 oct = octahedral_encode(mesh->vertices[i].normal);
        
        // Convert [-1, 1] to [0, 255]
        packed[i * 2 + 0] = (u8)((oct.x * 0.5f + 0.5f) * 255.0f);
        packed[i * 2 + 1] = (u8)((oct.y * 0.5f + 0.5f) * 255.0f);
    }
    
    // In a real implementation, you'd store this in a separate buffer
    // For now, just decode it back to verify
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        f32 x = packed[i * 2 + 0] / 255.0f * 2.0f - 1.0f;
        f32 y = packed[i * 2 + 1] / 255.0f * 2.0f - 1.0f;
        mesh->vertices[i].normal = octahedral_decode((Vec2){x, y});
    }
    
    FREE(packed);
    LOG_INFO("Packed normals to 8-bit octahedral encoding for mesh '%s'", mesh->name);
}

void mesh_pack_normals_oct16(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0) return;
    
    // Allocate packed storage (4 bytes per normal - 2x 16-bit)
    u16* packed = (u16*)MALLOC_GEOMETRY(mesh->vertex_count * 2 * sizeof(u16));
    
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        Vec2 oct = octahedral_encode(mesh->vertices[i].normal);
        
        // Convert [-1, 1] to [0, 65535]
        packed[i * 2 + 0] = (u16)((oct.x * 0.5f + 0.5f) * 65535.0f);
        packed[i * 2 + 1] = (u16)((oct.y * 0.5f + 0.5f) * 65535.0f);
    }
    
    // Decode back
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        f32 x = packed[i * 2 + 0] / 65535.0f * 2.0f - 1.0f;
        f32 y = packed[i * 2 + 1] / 65535.0f * 2.0f - 1.0f;
        mesh->vertices[i].normal = octahedral_decode((Vec2){x, y});
    }
    
    FREE(packed);
    LOG_INFO("Packed normals to 16-bit octahedral encoding for mesh '%s'", mesh->name);
}

// ----------------------------------------------------------------------------
// UV Quantization
// ----------------------------------------------------------------------------

void mesh_quantize_uvs(mesh_t* mesh) {
    if (!mesh || mesh->vertex_count == 0) return;
    
    // Find UV bounds
    f32 min_u = FLT_MAX, max_u = -FLT_MAX;
    f32 min_v = FLT_MAX, max_v = -FLT_MAX;
    
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        if (mesh->vertices[i].uv.x < min_u) min_u = mesh->vertices[i].uv.x;
        if (mesh->vertices[i].uv.x > max_u) max_u = mesh->vertices[i].uv.x;
        if (mesh->vertices[i].uv.y < min_v) min_v = mesh->vertices[i].uv.y;
        if (mesh->vertices[i].uv.y > max_v) max_v = mesh->vertices[i].uv.y;
    }
    
    f32 range_u = max_u - min_u;
    f32 range_v = max_v - min_v;
    
    if (range_u < 0.0001f) range_u = 1.0f;
    if (range_v < 0.0001f) range_v = 1.0f;
    
    // Quantize to 16-bit integers
    u16* packed = (u16*)MALLOC_GEOMETRY(mesh->vertex_count * 2 * sizeof(u16));
    
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        f32 u_norm = (mesh->vertices[i].uv.x - min_u) / range_u;
        f32 v_norm = (mesh->vertices[i].uv.y - min_v) / range_v;
        
        packed[i * 2 + 0] = (u16)(u_norm * 65535.0f);
        packed[i * 2 + 1] = (u16)(v_norm * 65535.0f);
    }
    
    // Decode back
    for (u32 i = 0; i < mesh->vertex_count; ++i) {
        f32 u_norm = packed[i * 2 + 0] / 65535.0f;
        f32 v_norm = packed[i * 2 + 1] / 65535.0f;
        
        mesh->vertices[i].uv.x = min_u + u_norm * range_u;
        mesh->vertices[i].uv.y = min_v + v_norm * range_v;
    }
    
    FREE(packed);
    LOG_INFO("Quantized UVs to 16-bit for mesh '%s' (range: [%.2f-%.2f, %.2f-%.2f])",
        mesh->name, min_u, max_u, min_v, max_v);
}
