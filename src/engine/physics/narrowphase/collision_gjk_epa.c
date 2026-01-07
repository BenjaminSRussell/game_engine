/**
 * GJK/EPA Implementation
 * 
 * Support functions, GJK algorithm, and EPA algorithm.
 */

#include "physics/narrowphase/collision_gjk_epa.h"
#include <float.h>
#include <include/math/math.h>
#include <stdio.h>

#define GJK_MAX_ITERATIONS 64
#define EPA_MAX_ITERATIONS 64
#define EPA_TOLERANCE 0.0001f
#define EPA_MAX_FACES 64
#define EPA_MAX_VERTICES 64

// ========================================
// Support Functions
// ========================================

static v4f support_sphere(const void *data, const v4f *dir) {
    const SphereShape *sphere = (const SphereShape*)data;
    v4f result;
    
    float len = v4f_length(dir);
    if (len > 1e-6f) {
        v4f_scale(&result, dir, sphere->radius / len);
    } else {
        result = (v4f){sphere->radius, 0, 0, 0};
    }
    return result;
}

static v4f support_box(const void *data, const v4f *dir) {
    const BoxShape *box = (const BoxShape*)data;
    v4f result;
    // Sign of dir components * half_extents
    result.x = (dir->x > 0.0f ? 1.0f : -1.0f) * box->half_extents.x;
    result.y = (dir->y > 0.0f ? 1.0f : -1.0f) * box->half_extents.y;
    result.z = (dir->z > 0.0f ? 1.0f : -1.0f) * box->half_extents.z;
    result.w = 0.0f;
    return result;
}

static v4f support_capsule(const void *data, const v4f *dir) {
    const CapsuleShape *capsule = (const CapsuleShape*)data;
    
    // Cylinder axis is usually Y.
    float half_height = capsule->height * 0.5f;
    v4f result;
    
    // Point on the axis
    float sign_y = (dir->y > 0.0f) ? 1.0f : -1.0f;
    result.x = 0;
    result.y = sign_y * half_height;
    result.z = 0;
    result.w = 0;
    
    // Add sphere support
    float len = v4f_length(dir);
    if (len > 1e-6f) {
        v4f sphere_support;
        v4f_scale(&sphere_support, dir, capsule->radius / len);
        v4f_add(&result, &result, &sphere_support);
    } else {
        result.x += capsule->radius;
    }
    
    return result;
}

v4f get_support(const CollisionShape *shape, const Transform *tx, const v4f *dir) {
    // 1. Transform dir to local space: R^T * dir
    // Conjugate quaternion for inverse rotation
    v4f inv_rot = tx->rotation;
    inv_rot.x = -inv_rot.x;
    inv_rot.y = -inv_rot.y;
    inv_rot.z = -inv_rot.z;
    
    v4f local_dir;
    v4f_rotate_quat(&local_dir, dir, &inv_rot);
    
    // 2. Get local support
    v4f local_support;
    switch (shape->type) {
        case SHAPE_SPHERE: local_support = support_sphere(shape->data, &local_dir); break;
        case SHAPE_BOX: local_support = support_box(shape->data, &local_dir); break;
        case SHAPE_CAPSULE: local_support = support_capsule(shape->data, &local_dir); break;
        default: local_support = (v4f){0,0,0,0}; break;
    }
    
    // Support includes center offset
    v4f_add(&local_support, &local_support, &shape->center);
    
    // 3. Transform support to world space: R * local_support + t
    v4f world_support;
    v4f_rotate_quat(&world_support, &local_support, &tx->rotation);
    v4f_add(&world_support, &world_support, &tx->position);
    
    return world_support;
}

// ========================================
// GJK Algorithm
// ========================================

// Minkowski difference support function
static v4f support_minkowski(const CollisionShape *sa, const Transform *ta,
                            const CollisionShape *sb, const Transform *tb,
                            const v4f *dir) {
    v4f sup_a = get_support(sa, ta, dir);
    
    v4f neg_dir;
    v4f_scale(&neg_dir, dir, -1.0f);
    v4f sup_b = get_support(sb, tb, &neg_dir);
    
    v4f result;
    v4f_sub(&result, &sup_a, &sup_b);
    return result;
}

// Simplex solver helpers
static bool handle_simplex(v4f *simplex, int *dim, v4f *dir) {
    // A is the last point added (simplex[*dim - 1])
    v4f a = simplex[*dim - 1];
    v4f ao;
    v4f_scale(&ao, &a, -1.0f); // Vector from A to Origin
    
    if (*dim == 2) {
        // Line case (B, A)
        v4f b = simplex[0];
        v4f ab;
        v4f_sub(&ab, &b, &a);
        
        // AB . AO
        if (v4f_dot(&ab, &ao) > 0) {
            // Region AB: New dir is cross(cross(ab, ao), ab)
            // But for line, it's just normal to line towards origin
             // Triple cross product expansion: (A x B) x C = (A.C)B - (B.C)A
            // Here: (AB x AO) x AB = AB x (AO x AB)
            // Efficient way: rejection of AO on AB? No, component perpendicular.
            // Or simpler: triple cross in 3D.
            v4f temp;
            v4f_cross(&temp, &ab, &ao);
            v4f_cross(dir, &temp, &ab);
        } else {
            // Region A (closest to A)
            // Reduce to point A
            *dim = 1;
            *dir = ao;
        }
    } else if (*dim == 3) {
        // Triangle case (C, B, A)
        v4f b = simplex[1];
        v4f c = simplex[0];
        v4f ab, ac, abc;
        
        v4f_sub(&ab, &b, &a);
        v4f_sub(&ac, &c, &a);
        v4f_cross(&abc, &ab, &ac);
        
        v4f ab_abc, abc_ac;
        v4f_cross(&ab_abc, &ab, &abc);
        v4f_cross(&abc_ac, &abc, &ac); // Actually cross(abc, ac) points OUT of AC edge away from triangle
        
        if (v4f_dot(&abc_ac, &ao) > 0) {
            if (v4f_dot(&ac, &ao) > 0) {
                // Region AC
                *dim = 2;
                simplex[0] = c;
                simplex[1] = a;
                v4f temp;
                v4f_cross(&temp, &ac, &ao);
                v4f_cross(dir, &temp, &ac); // Normal to AC towards origin
            } else {
                // Region AB or Star (A)
                // Need to check AB logic akin to line case
                if (v4f_dot(&ab, &ao) > 0) {
                     *dim = 2;
                     simplex[0] = b;
                     simplex[1] = a;
                     v4f temp;
                     v4f_cross(&temp, &ab, &ao);
                     v4f_cross(dir, &temp, &ab);
                } else {
                    *dim = 1;
                    *dir = ao;
                }
            }
        } else {
            if (v4f_dot(&ab_abc, &ao) > 0) {
                 if (v4f_dot(&ab, &ao) > 0) {
                     // Region AB
                     *dim = 2;
                     simplex[0] = b;
                     simplex[1] = a;
                     v4f temp;
                     v4f_cross(&temp, &ab, &ao);
                     v4f_cross(dir, &temp, &ab);
                } else {
                    *dim = 1;
                    *dir = ao;
                }
            } else {
                // Region ABC (inside triangle or above/below)
                if (v4f_dot(&abc, &ao) > 0) {
                    *dir = abc;
                } else {
                    v4f_scale(dir, &abc, -1.0f);
                    // Standard GJK winding order usually not guaranteed, so flips can happen.
                    // Important: Ensure we keep winding consistent for EPA if we pass this simplex.
                    // For now, just correct normal.
                }
            }
        }
    } else if (*dim == 4) {
        // Tetrahedron case (D, C, B, A)
        v4f b = simplex[2];
        v4f c = simplex[1];
        v4f d = simplex[0];
        
        v4f ab, ac, ad;
        v4f_sub(&ab, &b, &a);
        v4f_sub(&ac, &c, &a);
        v4f_sub(&ad, &d, &a);
        
        v4f abc, acd, adb;
        v4f_cross(&abc, &ab, &ac);
        v4f_cross(&acd, &ac, &ad);
        v4f_cross(&adb, &ad, &ab);
        
        // Check normals of faces to see if origin is outside
        if (v4f_dot(&abc, &ao) > 0) {
            // Outside ABC
            *dim = 3;
            simplex[0] = c; simplex[1] = b; simplex[2] = a;
            *dir = abc;
        } else if (v4f_dot(&acd, &ao) > 0) {
            // Outside ACD
            *dim = 3;
            simplex[0] = d; simplex[1] = c; simplex[2] = a;
            *dir = acd;
        } else if (v4f_dot(&adb, &ao) > 0) {
            // Outside ADB
            *dim = 3;
            simplex[0] = b; simplex[1] = d; simplex[2] = a;
            *dir = adb;
        } else {
            // Origin is inside!
            return true;
        }
    }
    
    return false;
}

GJKResult gjk_detect_collision(const CollisionShape *shape_a, const Transform *tx_a,
                              const CollisionShape *shape_b, const Transform *tx_b) {
    GJKResult result;
    result.colliding = false;
    result.simplex_dim = 0;
    
    // Initial direction: Center B - Center A or generic (1,0,0)
    v4f dir; 
    v4f_sub(&dir, &tx_b->position, &tx_a->position); // Try center diff first
    if (v4f_dot(&dir, &dir) < 1e-6f) dir = (v4f){1.0f, 0.0f, 0.0f, 0.0f};
    
    // First point
    v4f point = support_minkowski(shape_a, tx_a, shape_b, tx_b, &dir);
    result.simplex[0] = point;
    result.simplex_dim = 1;
    
    // Next direction towards origin
    v4f_scale(&dir, &point, -1.0f);
    
    for (int i = 0; i < GJK_MAX_ITERATIONS; i++) {
        // Support in direction
        point = support_minkowski(shape_a, tx_a, shape_b, tx_b, &dir);
        
        // Check if we passed the origin
        if (v4f_dot(&point, &dir) < 0) {
            return result; // No collision
        }
        
        result.simplex[result.simplex_dim++] = point;
        
        if (handle_simplex(result.simplex, &result.simplex_dim, &dir)) {
            result.colliding = true;
            return result;
        }
    }
    
    return result;
}

// ========================================
// EPA Algorithm
// ========================================

typedef struct EPAFace {
    v4f n;      // Normal
    float d;    // Distance to origin
    int v[3];   // Indices into vertex array
} EPAFace;

typedef struct EPAEdge {
    int v1, v2;
} EPAEdge;

// Helper function to add face
static void add_face(int a, int b, int c, EPAFace *faces, int *count, v4f *verts) {
     if (*count >= EPA_MAX_FACES) return;
     v4f v1 = verts[a];
     v4f v2 = verts[b];
     v4f v3 = verts[c];
     
     v4f edge1, edge2, n;
     v4f_sub(&edge1, &v2, &v1);
     v4f_sub(&edge2, &v3, &v1);
     v4f_cross(&n, &edge1, &edge2);
     v4f_normalize(&n, &n);
     
     faces[*count].n = n;
     faces[*count].d = v4f_dot(&n, &v1);
     faces[*count].v[0] = a;
     faces[*count].v[1] = b;
     faces[*count].v[2] = c;
     
     // Ensure normal points away from origin.
     if (faces[*count].d < 0) {
          faces[*count].n.x *= -1; faces[*count].n.y *= -1; faces[*count].n.z *= -1;
          faces[*count].d *= -1;
          // Swap indices to keep winding
          int temp = faces[*count].v[0];
          faces[*count].v[0] = faces[*count].v[1];
          faces[*count].v[1] = temp;
     }
     (*count)++;
}

// Helper: Add unique edge (remove duplicates)
static void add_unique_edge(int a, int b, EPAEdge *edges, int *count) {
    for (int i = 0; i < *count; i++) {
        if (edges[i].v1 == b && edges[i].v2 == a) {
            // Found reverse edge, means it's shared by two lit faces -> remove it
            // Swap with last
            edges[i] = edges[--(*count)];
            return;
        }
    }
    edges[*count].v1 = a;
    edges[*count].v2 = b;
    (*count)++;
}

EPAResult epa_compute_penetration(const CollisionShape *shape_a, const Transform *tx_a,
                                 const CollisionShape *shape_b, const Transform *tx_b,
                                 const v4f *gjk_simplex, int gjk_dim) {
    EPAResult result;
    result.valid = false;
    
    // 0. Fallback if GJK didn't return a tetrahedron
    // In practice, if collision is found but dim < 4, we need to handle handling degeneracy or building a tetra.
    // For this implementation, we assume GJK returns tetra or generic 4 points containing origin.
    // NOTE: Simplex winding matters.
    
    v4f polytope[EPA_MAX_VERTICES];
    int num_verts = gjk_dim;
    for(int i=0; i<num_verts; i++) polytope[i] = gjk_simplex[i];
    
    // Expand simplex to tetrahedron if necessary
    if (num_verts == 1) {
        // Point case: Add any point in some direction
        v4f dir = {1, 0, 0, 0};
        v4f p = support_minkowski(shape_a, tx_a, shape_b, tx_b, &dir);
        if (v4f_dot(&p, &dir) - v4f_dot(&polytope[0], &dir) < 1e-4f) {
           // Try other directions if 1.0,0,0 failed to find new point
           dir = (v4f){0, 1, 0, 0};
           p = support_minkowski(shape_a, tx_a, shape_b, tx_b, &dir);
        }
        polytope[num_verts++] = p;
    }
    
    if (num_verts == 2) {
        // Line case: Add point in direction orthogonal to line
        v4f line;
        v4f_sub(&line, &polytope[1], &polytope[0]);
        
        // Find normal
        v4f dir = {0};
        if (fabsf(line.x) < fabsf(line.y)) dir.x = 1; else dir.y = 1;
        v4f n;
        v4f_cross(&n, &line, &dir);
        v4f_normalize(&n, &n);
        
        // 3rd point
        v4f p = support_minkowski(shape_a, tx_a, shape_b, tx_b, &n);
        polytope[num_verts++] = p;
        
        // 4th point opposite side? Or triangle expansion handles it.
    }
    
    if (num_verts == 3) {
        // Triangle case: Add point in normal direction
        v4f v1v0, v2v0;
        v4f_sub(&v1v0, &polytope[1], &polytope[0]);
        v4f_sub(&v2v0, &polytope[2], &polytope[0]);
        
        v4f n;
        v4f_cross(&n, &v1v0, &v2v0);
        v4f_normalize(&n, &n);
        
        // Support along normal
        v4f p = support_minkowski(shape_a, tx_a, shape_b, tx_b, &n);
        
        // Check if we found a new point (if close to triangle, try opposite normal)
        float d = v4f_dot(&n, &p);
        float d_tri = v4f_dot(&n, &polytope[0]);
        
        if (fabsf(d - d_tri) < 1e-4f) {
             v4f_scale(&n, &n, -1.0f);
             p = support_minkowski(shape_a, tx_a, shape_b, tx_b, &n);
        }
        
        polytope[num_verts++] = p;
    }
    
    if (num_verts < 4) {
        // Still invalid? Should not happen if shapes have volume.
        return result;
    }
    
    EPAFace faces[EPA_MAX_FACES];
    int num_faces = 0;
    
    // Initial Tetrahedron faces (0,1,2), (0,2,1)? 
    // Need correct winding. We check d anyway.
    add_face(0, 1, 2, faces, &num_faces, polytope);
    add_face(0, 2, 3, faces, &num_faces, polytope);
    add_face(2, 1, 3, faces, &num_faces, polytope);
    add_face(1, 0, 3, faces, &num_faces, polytope);
    
    for (int iter = 0; iter < EPA_MAX_ITERATIONS; iter++) {
        // Find closest face
        float min_dist = FLT_MAX;
        int closest_face_idx = -1;
        
        for (int i = 0; i < num_faces; i++) {
            if (faces[i].d < min_dist) {
                min_dist = faces[i].d;
                closest_face_idx = i;
            }
        }
        
        if (closest_face_idx == -1) {
             printf("EPA: No closest face found\n");
             break;
        }
        
        v4f n = faces[closest_face_idx].n;
        
        // Support in normal direction
        v4f p = support_minkowski(shape_a, tx_a, shape_b, tx_b, &n);
        
        float d = v4f_dot(&p, &n);
        
        // Convergence check
        if (d - min_dist < EPA_TOLERANCE) {
            result.valid = true;
            result.penetration_depth = d;
            result.normal = n; // Normal on B pointing to A? 
             v4f_scale(&result.normal, &result.normal, -1.0f); // Default to B->A often
            return result;
        }
        
        // Expansion: Remove lit faces
        EPAEdge edges[EPA_MAX_FACES * 3]; // Rough upper bound
        int num_edges = 0;
        
        // But first, we need to add point p to polytope
        if (num_verts >= EPA_MAX_VERTICES) break;
        polytope[num_verts] = p;
        int p_idx = num_verts++;
        
        // Iterate all faces to see if lit
        // Lit if dot(n, p) - d > 0
        
        EPAFace next_faces[EPA_MAX_FACES]; // Temp buffer
        int next_count = 0;
        
        for (int i = 0; i < num_faces; i++) {
            if (v4f_dot(&faces[i].n, &p) - faces[i].d > 0) {
                 // Lit: add edges
                 add_unique_edge(faces[i].v[0], faces[i].v[1], edges, &num_edges);
                 add_unique_edge(faces[i].v[1], faces[i].v[2], edges, &num_edges);
                 add_unique_edge(faces[i].v[2], faces[i].v[0], edges, &num_edges);
            } else {
                 // Keep face
                 next_faces[next_count++] = faces[i];
            }
        }
        
        // Reconstruct from edges
        for (int i = 0; i < num_edges; i++) {
             add_face(edges[i].v1, edges[i].v2, p_idx, next_faces, &next_count, polytope);
        }
        
        // Copy back
        for(int i=0; i<next_count; i++) faces[i] = next_faces[i];
        num_faces = next_count;
    }
    
    return result;
}
