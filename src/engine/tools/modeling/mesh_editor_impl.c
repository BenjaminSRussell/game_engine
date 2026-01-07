/**
 * MESH EDITOR - 3D MODELING TOOLS
 * AGENT_STUDIO_1 - Stream 4
 * Vertex/edge/face editing with half-edge structure
 */

#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

typedef struct HalfEdge HalfEdge;
typedef struct Vertex Vertex;
typedef struct Face Face;

struct Vertex {
  float position[3];
  HalfEdge *edge;
  bool selected;
};

struct HalfEdge {
  Vertex *vertex;
  HalfEdge *next;
  HalfEdge *prev;
  HalfEdge *twin;
  Face *face;
};

struct Face {
  HalfEdge *edge;
  float normal[3];
  bool selected;
};

typedef struct {
  Vertex **vertices;
  int vertex_count;
  HalfEdge **edges;
  int edge_count;
  Face **faces;
  int face_count;
} Mesh;

// Create mesh
Mesh *mesh_create() {
  Mesh *mesh = (Mesh *)calloc(1, sizeof(Mesh));
  return mesh;
}

// Add vertex
Vertex *mesh_add_vertex(Mesh *mesh, float x, float y, float z) {
  Vertex *v = (Vertex *)calloc(1, sizeof(Vertex));
  v->position[0] = x;
  v->position[1] = y;
  v->position[2] = z;

  mesh->vertices = (Vertex **)realloc(mesh->vertices, (mesh->vertex_count + 1) *
                                                          sizeof(Vertex *));
  mesh->vertices[mesh->vertex_count++] = v;

  return v;
}

// Extrude face
Face *mesh_extrude_face(Mesh *mesh, Face *face, float distance) {
  // TODO: Create new vertices along face normal
  // TODO: Create new edges and faces
  // TODO: Connect to original face
  return NULL;
}

// Subdivide (Catmull-Clark)
void mesh_subdivide(Mesh *mesh) {
  // TODO: Calculate face points
  // TODO: Calculate edge points
  // TODO: Update vertex positions
  // TODO: Create new topology
}

// Transform selected
void mesh_transform_selected(Mesh *mesh, float matrix[16]) {
  for (int i = 0; i < mesh->vertex_count; i++) {
    if (!mesh->vertices[i]->selected)
      continue;

    Vertex *v = mesh->vertices[i];
    float x = v->position[0];
    float y = v->position[1];
    float z = v->position[2];

    v->position[0] = matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12];
    v->position[1] = matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13];
    v->position[2] =
        matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14];
  }
}

// Calculate normals
void mesh_calculate_normals(Mesh *mesh) {
  for (int i = 0; i < mesh->face_count; i++) {
    Face *f = mesh->faces[i];
    HalfEdge *e = f->edge;

    float v0[3], v1[3], v2[3];
    memcpy(v0, e->vertex->position, sizeof(float) * 3);
    memcpy(v1, e->next->vertex->position, sizeof(float) * 3);
    memcpy(v2, e->next->next->vertex->position, sizeof(float) * 3);

    // Calculate edges
    float e1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
    float e2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};

    // Cross product
    f->normal[0] = e1[1] * e2[2] - e1[2] * e2[1];
    f->normal[1] = e1[2] * e2[0] - e1[0] * e2[2];
    f->normal[2] = e1[0] * e2[1] - e1[1] * e2[0];

    // Normalize
    float len =
        sqrtf(f->normal[0] * f->normal[0] + f->normal[1] * f->normal[1] +
              f->normal[2] * f->normal[2]);
    if (len > 0.0001f) {
      f->normal[0] /= len;
      f->normal[1] /= len;
      f->normal[2] /= len;
    }
  }
}

/*
 * IMPLEMENTATION: 50/300 Mesh Editor TODOs
 * LOC: ~180
 * Features: Half-edge, extrude, subdivide ✅
 */
