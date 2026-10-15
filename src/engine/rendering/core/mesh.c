#include <common.h>
#include <rendering/mesh.h>
#include <rendering/renderer.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Basic Mesh system implementation

void mesh_init(Mesh *mesh, u32 vertex_capacity, u32 index_capacity) {
    if (!mesh) return;

    memset(mesh, 0, sizeof(Mesh));
    
    mesh->vertex_capacity = vertex_capacity;
    mesh->index_capacity = index_capacity;

    if (vertex_capacity > 0) {
        mesh->vertices = (Vertex*)calloc(vertex_capacity, sizeof(Vertex));
    }
    
    if (index_capacity > 0) {
        mesh->indices = (u32*)calloc(index_capacity, sizeof(u32));
    }

    mesh->is_uploaded = false;
}

void mesh_free(Mesh *mesh) {
    if (!mesh) return;

    if (mesh->vertices) {
        free(mesh->vertices);
        mesh->vertices = NULL;
    }
    
    if (mesh->indices) {
        free(mesh->indices);
        mesh->indices = NULL;
    }

    mesh->vertex_count = 0;
    mesh->vertex_capacity = 0;
    mesh->index_count = 0;
    mesh->index_capacity = 0;
    mesh->is_uploaded = false;
}

// Post-transform cache size that Forsyth's scoring is tuned for.
#define MESH_VCACHE_SIZE 32

static bool mesh_vcache_contains(const u32 *list, u32 count, u32 value) {
    for (u32 i = 0; i < count; i++) {
        if (list[i] == value) return true;
    }
    return false;
}

// Forsyth's vertex score ("Linear-Speed Vertex Cache Optimisation", 2006).
// Vertices near the front of the cache score high. Vertices with few unemitted
// triangles left get a boost, so they are finished early.
static f32 mesh_vcache_vertex_score(i32 cache_pos, u32 active_tris) {
    if (active_tris == 0) return -1.0f;

    f32 score = 0.0f;
    if (cache_pos >= 0) {
        if (cache_pos < 3) {
            score = 0.75f;  // vertex of the triangle emitted last
        } else {
            score = 1.0f - (f32)(cache_pos - 3) / (f32)(MESH_VCACHE_SIZE - 3);
            score += score * sqrtf(score);  // cache decay: score^1.5
        }
    }
    return score + 2.0f / sqrtf((f32)active_tris);  // valence boost: 2 * active^-0.5
}

// Writes the triangles of `indices` to `out` in cache-friendly order. Each triangle
// keeps its vertex order, so winding and geometry are unchanged. Returns false if
// the temporary arrays cannot be allocated; `out` is then not written.
static bool mesh_vcache_reorder(const u32 *indices, u32 *out, u32 vertex_count,
                                u32 tri_count) {
    const size_t index_count = (size_t)tri_count * 3;
    u32 *adj_begin = calloc((size_t)vertex_count + 1, sizeof(u32));
    u32 *adj_fill = malloc((size_t)vertex_count * sizeof(u32));
    u32 *adj_tris = malloc(index_count * sizeof(u32));
    u32 *active = malloc((size_t)vertex_count * sizeof(u32));
    f32 *vscore = malloc((size_t)vertex_count * sizeof(f32));
    bool *emitted = calloc(tri_count, sizeof(bool));
    const bool ok = adj_begin && adj_fill && adj_tris && active && vscore && emitted;

    if (ok) {
        // Triangles touching each vertex, as ranges of adj_tris.
        for (size_t i = 0; i < index_count; i++) adj_begin[indices[i] + 1]++;
        for (u32 v = 0; v < vertex_count; v++) adj_begin[v + 1] += adj_begin[v];
        memcpy(adj_fill, adj_begin, (size_t)vertex_count * sizeof(u32));
        for (size_t i = 0; i < index_count; i++) {
            adj_tris[adj_fill[indices[i]]++] = (u32)(i / 3);
        }
        for (u32 v = 0; v < vertex_count; v++) {
            active[v] = adj_begin[v + 1] - adj_begin[v];
            vscore[v] = mesh_vcache_vertex_score(-1, active[v]);
        }

        // Greedy emission: the best-scoring triangle that touches the cache. When
        // none is left, fall back to the lowest-numbered unemitted triangle.
        u32 cache[MESH_VCACHE_SIZE];  // most recently used first
        u32 cache_len = 0;
        u32 cursor = 0;
        u32 next = 0;
        f32 best = 0.0f;
        for (u32 t = 0; t < tri_count; t++) {
            const u32 *tri = indices + 3 * (size_t)t;
            const f32 s = vscore[tri[0]] + vscore[tri[1]] + vscore[tri[2]];
            if (t == 0 || s > best) {
                best = s;
                next = t;
            }
        }

        size_t written = 0;
        for (u32 done = 0; done < tri_count; done++) {
            const u32 t = next;
            const u32 *tri = indices + 3 * (size_t)t;
            emitted[t] = true;
            for (u32 k = 0; k < 3; k++) {
                out[written++] = tri[k];
                active[tri[k]]--;
            }

            // The emitted triangle's vertices move to the front of the cache. Older
            // entries follow until the cache is full; the rest are evicted.
            u32 fresh[MESH_VCACHE_SIZE];
            u32 fresh_len = 0;
            for (u32 k = 0; k < 3; k++) {
                if (!mesh_vcache_contains(fresh, fresh_len, tri[k])) fresh[fresh_len++] = tri[k];
            }
            for (u32 j = 0; j < cache_len; j++) {
                const u32 v = cache[j];
                if (mesh_vcache_contains(tri, 3, v)) continue;
                if (fresh_len < MESH_VCACHE_SIZE) {
                    fresh[fresh_len++] = v;
                } else {
                    vscore[v] = mesh_vcache_vertex_score(-1, active[v]);
                }
            }
            cache_len = fresh_len;
            memcpy(cache, fresh, cache_len * sizeof(u32));
            for (u32 j = 0; j < cache_len; j++) {
                vscore[cache[j]] = mesh_vcache_vertex_score((i32)j, active[cache[j]]);
            }

            // Best unemitted triangle that touches a cached vertex.
            bool found = false;
            for (u32 j = 0; j < cache_len; j++) {
                const u32 v = cache[j];
                for (u32 p = adj_begin[v]; p < adj_begin[v + 1]; p++) {
                    const u32 u = adj_tris[p];
                    if (emitted[u]) continue;
                    const u32 *ut = indices + 3 * (size_t)u;
                    const f32 s = vscore[ut[0]] + vscore[ut[1]] + vscore[ut[2]];
                    if (!found || s > best) {
                        found = true;
                        best = s;
                        next = u;
                    }
                }
            }
            if (!found) {
                while (cursor < tri_count && emitted[cursor]) cursor++;
                next = cursor;
            }
        }
    }

    free(adj_begin);
    free(adj_fill);
    free(adj_tris);
    free(active);
    free(vscore);
    free(emitted);
    return ok;
}

// Reorders the mesh's triangles for GPU post-transform vertex cache locality.
// Call before mesh_upload_to_gpu(): the uploaded index buffer is not updated here.
// The mesh is left unchanged if its indices are not whole triangles, reference a
// vertex at or past vertex_count, or memory cannot be allocated.
void mesh_optimize_vertex_cache(Mesh *mesh) {
    if (!mesh || !mesh->indices || mesh->vertex_count == 0 || mesh->index_count < 3 ||
        mesh->index_count % 3 != 0) {
        return;
    }
    for (u32 i = 0; i < mesh->index_count; i++) {
        if (mesh->indices[i] >= mesh->vertex_count) return;
    }

    u32 *out = malloc((size_t)mesh->index_count * sizeof(u32));
    if (!out) return;
    if (mesh_vcache_reorder(mesh->indices, out, mesh->vertex_count, mesh->index_count / 3)) {
        memcpy(mesh->indices, out, (size_t)mesh->index_count * sizeof(u32));
    }
    free(out);
}
