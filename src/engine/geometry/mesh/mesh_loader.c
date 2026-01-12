#include "mesh_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Dynamic Array Implementation
typedef struct {
    void* data;
    size_t element_size;
    size_t size;
    size_t capacity;
} vector_t;

static void vector_init(vector_t* vec, size_t element_size) {
    vec->data = NULL;
    vec->element_size = element_size;
    vec->size = 0;
    vec->capacity = 0;
}

static void vector_push(vector_t* vec, const void* element) {
    if (vec->size >= vec->capacity) {
        vec->capacity = vec->capacity == 0 ? 1024 : vec->capacity * 2;
        vec->data = realloc(vec->data, vec->capacity * vec->element_size);
    }
    memcpy((char*)vec->data + (vec->size * vec->element_size), element, vec->element_size);
    vec->size++;
}

static void vector_free(vector_t* vec) {
    if (vec->data) free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}

static void* vector_get(vector_t* vec, size_t index) {
    if (index >= vec->size) return NULL;
    return (char*)vec->data + (index * vec->element_size);
}

// Subsystem Stubs
int geometry_mesh_loader_init(void) { return 0; }
void geometry_mesh_loader_shutdown(void) {}
int geometry_mesh_loader_create(geometry_mesh_loader_handle_t* out_handle, const geometry_mesh_loader_desc_t* desc) { return 0; }
void geometry_mesh_loader_destroy(geometry_mesh_loader_handle_t handle) {}
int geometry_mesh_loader_update(geometry_mesh_loader_handle_t handle, const void* data, size_t size) { return 0; }
bool geometry_mesh_loader_is_valid(geometry_mesh_loader_handle_t handle) { return false; }
int geometry_mesh_loader_get_info(geometry_mesh_loader_handle_t handle, geometry_mesh_loader_info_t* out_info) { return 0; }
void geometry_mesh_loader_mark_dirty(geometry_mesh_loader_handle_t handle) {}
int geometry_mesh_loader_process_pending(void) { return 0; }
int geometry_mesh_loader_load_async(geometry_mesh_loader_handle_t handle, const char* path, geometry_mesh_loader_callback_t callback, void* user_data) { return 0; }
uint32_t geometry_mesh_loader_get_count(void) { return 0; }
size_t geometry_mesh_loader_get_memory_usage(void) { return 0; }
void geometry_mesh_loader_debug_print(void) {}

// Vertex Deduplication
typedef struct {
    int p_idx;
    int t_idx;
    int n_idx;
    u32 final_index;
    struct vertex_node_t* next;
} vertex_node_t;

#define HASH_TABLE_SIZE 10007

static u32 hash_vertex(int p, int t, int n) {
    // Simple hash combining indices
    u32 h = 0;
    h = h * 31 + (u32)p;
    h = h * 31 + (u32)t;
    h = h * 31 + (u32)n;
    return h % HASH_TABLE_SIZE;
}

// OBJ Loader Implementation
mesh_t* mesh_load_obj(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Failed to open OBJ file: %s\n", filename);
        return NULL;
    }

    vector_t positions; vector_init(&positions, sizeof(Vec3));
    vector_t texcoords; vector_init(&texcoords, sizeof(Vec2));
    vector_t normals;   vector_init(&normals, sizeof(Vec3));
    vector_t final_vertices; vector_init(&final_vertices, sizeof(vertex_t));
    vector_t final_indices;  vector_init(&final_indices, sizeof(u32));

    vertex_node_t** hash_table = (vertex_node_t**)calloc(HASH_TABLE_SIZE, sizeof(vertex_node_t*));

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char* token = strtok(line, " \t\r\n");
        if (!token) continue;

        if (strcmp(token, "v") == 0) {
            Vec3 p;
            char* x = strtok(NULL, " \t\r\n");
            char* y = strtok(NULL, " \t\r\n");
            char* z = strtok(NULL, " \t\r\n");
            if (x && y && z) {
                p.x = strtof(x, NULL);
                p.y = strtof(y, NULL);
                p.z = strtof(z, NULL);
                vector_push(&positions, &p);
            }
        } else if (strcmp(token, "vt") == 0) {
            Vec2 uv;
            char* u = strtok(NULL, " \t\r\n");
            char* v = strtok(NULL, " \t\r\n");
            if (u && v) {
                uv.x = strtof(u, NULL);
                uv.y = strtof(v, NULL);
                vector_push(&texcoords, &uv);
            }
        } else if (strcmp(token, "vn") == 0) {
            Vec3 n;
            char* x = strtok(NULL, " \t\r\n");
            char* y = strtok(NULL, " \t\r\n");
            char* z = strtok(NULL, " \t\r\n");
            if (x && y && z) {
                n.x = strtof(x, NULL);
                n.y = strtof(y, NULL);
                n.z = strtof(z, NULL);
                vector_push(&normals, &n);
            }
        } else if (strcmp(token, "f") == 0) {
            // Triangulate face (fan triangulation)
            int p_indices[32], t_indices[32], n_indices[32];
            int count = 0;

            while ((token = strtok(NULL, " \t\r\n")) && count < 32) {
                int p = 0, t = 0, n = 0;

                // Parse v/vt/vn format manually to handle missing components
                char* slash1 = strchr(token, '/');
                if (slash1) {
                    *slash1 = '\0';
                    p = atoi(token);

                    char* next = slash1 + 1;
                    char* slash2 = strchr(next, '/');
                    if (slash2) {
                        *slash2 = '\0';
                        if (slash2 > next) t = atoi(next); // v/vt/vn
                        n = atoi(slash2 + 1);
                    } else {
                        if (*next) t = atoi(next); // v/vt
                    }
                } else {
                    p = atoi(token); // v
                }

                // OBJ indices are 1-based, handle negatives
                if (p < 0) p = (int)positions.size + p + 1;
                if (t < 0) t = (int)texcoords.size + t + 1;
                if (n < 0) n = (int)normals.size + n + 1;

                p_indices[count] = p;
                t_indices[count] = t;
                n_indices[count] = n;
                count++;
            }

            // Triangulate
            for (int i = 2; i < count; i++) {
                int indices[3] = {0, i - 1, i};

                for (int j = 0; j < 3; j++) {
                    int idx = indices[j];
                    int p = p_indices[idx];
                    int t = t_indices[idx];
                    int n = n_indices[idx];

                    // Bounds Check
                    if (p < 1 || p > (int)positions.size) {
                        printf("Error: Invalid position index %d (max %zu)\n", p, positions.size);
                        // cleanup and return NULL or skip
                        continue;
                    }
                    if (t > (int)texcoords.size) t = 0; // Ignore invalid UVs
                    if (n > (int)normals.size) n = 0;   // Ignore invalid normals

                    // Deduplication
                    u32 hash = hash_vertex(p, t, n);
                    vertex_node_t* node = hash_table[hash];
                    u32 final_idx = (u32)-1;

                    while (node) {
                        if (node->p_idx == p && node->t_idx == t && node->n_idx == n) {
                            final_idx = node->final_index;
                            break;
                        }
                        node = (vertex_node_t*)node->next;
                    }

                    if (final_idx == (u32)-1) {
                        // Create new vertex
                        vertex_t v = {0};

                        Vec3* pos_ptr = (Vec3*)vector_get(&positions, p - 1);
                        if (pos_ptr) v.position = *pos_ptr;

                        if (t > 0) {
                            Vec2* uv_ptr = (Vec2*)vector_get(&texcoords, t - 1);
                            if (uv_ptr) v.uv = *uv_ptr;
                        }

                        if (n > 0) {
                            Vec3* n_ptr = (Vec3*)vector_get(&normals, n - 1);
                            if (n_ptr) v.normal = *n_ptr;
                        }

                        v.tangent = (Vec4){0, 0, 0, 1}; // Default

                        final_idx = (u32)final_vertices.size;
                        vector_push(&final_vertices, &v);

                        // Add to hash table
                        vertex_node_t* new_node = (vertex_node_t*)malloc(sizeof(vertex_node_t));
                        new_node->p_idx = p;
                        new_node->t_idx = t;
                        new_node->n_idx = n;
                        new_node->final_index = final_idx;
                        new_node->next = (struct vertex_node_t*)hash_table[hash];
                        hash_table[hash] = new_node;
                    }

                    vector_push(&final_indices, &final_idx);
                }
            }
        }
    }

    fclose(file);

    // Create Mesh
    mesh_t* mesh = mesh_create(filename);
    if (!mesh) {
        // Cleanup on failure
    } else {
        mesh_allocate_buffers(mesh, (u32)final_vertices.size, (u32)final_indices.size);
        mesh_set_vertices(mesh, (vertex_t*)final_vertices.data, (u32)final_vertices.size, 0);
        mesh_set_indices(mesh, (u32*)final_indices.data, (u32)final_indices.size, 0);

        submesh_t sub = {0};
        sub.index_count = (u32)final_indices.size;
        sub.vertex_count = (u32)final_vertices.size;
        mesh_add_submesh(mesh, sub);

        mesh_calculate_bounds(mesh);
        // Only calculate tangents if UVs are present, otherwise might produce garbage?
        // Actually mesh_calculate_tangents handles it safely (checks UVs)
        mesh_calculate_tangents(mesh);
    }

    // Cleanup
    vector_free(&positions);
    vector_free(&texcoords);
    vector_free(&normals);
    vector_free(&final_vertices);
    vector_free(&final_indices);

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        vertex_node_t* node = hash_table[i];
        while (node) {
            vertex_node_t* next = (vertex_node_t*)node->next;
            free(node);
            node = next;
        }
    }
    free(hash_table);

    return mesh;
}
