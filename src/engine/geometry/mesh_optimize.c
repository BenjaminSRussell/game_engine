#include "mesh_optimize.h"
#include "mesh.h"
#include <core/memory.h>
#include <core/logger.h>
#include <string.h>
#include <math.h>

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
// Index Optimization (Vertex Cache)
// ----------------------------------------------------------------------------

// Simplified Forsyth vertex cache optimization
void mesh_optimize_vertex_cache(mesh_t* mesh) {
    if (!mesh || mesh->index_count == 0) return;
    
    // For now, implement a simple linear ordering
    // Full Forsyth algorithm is complex - this is a placeholder
    // TODO: Implement full Forsyth/Tipsify algorithm
    
    LOG_INFO("Vertex cache optimization applied (placeholder)");
}

void mesh_optimize_indices(mesh_t* mesh) {
    mesh_optimize_vertex_cache(mesh);
}

// ----------------------------------------------------------------------------
// Normal Packing (Octahedral Encoding)
// ----------------------------------------------------------------------------

void mesh_pack_normals_oct8(mesh_t* mesh) {
    // TODO: Implement octahedral encoding for normals
    // This reduces normal storage from 12 bytes to 2 bytes
    LOG_INFO("Normal packing (oct8) not yet implemented");
}

void mesh_pack_normals_oct16(mesh_t* mesh) {
    // TODO: Implement octahedral encoding for normals (16-bit version)
    LOG_INFO("Normal packing (oct16) not yet implemented");
}

// ----------------------------------------------------------------------------
// UV Quantization
// ----------------------------------------------------------------------------

void mesh_quantize_uvs(mesh_t* mesh) {
    // TODO: Quantize UVs to 16-bit integers
    // This reduces UV storage from 8 bytes to 4 bytes
    LOG_INFO("UV quantization not yet implemented");
}
