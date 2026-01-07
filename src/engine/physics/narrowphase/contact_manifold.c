/**
 * Contact Manifold Generation Implementation
 */

#include "physics/narrowphase/contact_manifold.h"
#include <include/math/math.h>
#include <string.h>

// Helper to get box face vertices
static void get_box_face(const BoxShape *box, const Transform *tx, const v4f *direction,
                        v4f *face_vertices, int *num_vertices) {
    // Determine which face is most aligned with direction
    v4f local_dir;
    v4f inv_rot = tx->rotation;
    inv_rot.x = -inv_rot.x;
    inv_rot.y = -inv_rot.y;
    inv_rot.z = -inv_rot.z;
    v4f_rotate_quat(&local_dir, direction, &inv_rot);
    
    // Find axis with largest component
    float abs_x = fabsf(local_dir.x);
    float abs_y = fabsf(local_dir.y);
    float abs_z = fabsf(local_dir.z);
    
    v4f he = box->half_extents;
    
    if (abs_x > abs_y && abs_x > abs_z) {
        // X face
        float sign = (local_dir.x > 0) ? 1.0f : -1.0f;
        v4f local[4] = {
            {sign * he.x, -he.y, -he.z, 0},
            {sign * he.x,  he.y, -he.z, 0},
            {sign * he.x,  he.y,  he.z, 0},
            {sign * he.x, -he.y,  he.z, 0}
        };
        
        for (int i = 0; i < 4; i++) {
            v4f_rotate_quat(&face_vertices[i], &local[i], &tx->rotation);
            v4f_add(&face_vertices[i], &face_vertices[i], &tx->position);
        }
        *num_vertices = 4;
    } else if (abs_y > abs_z) {
        // Y face
        float sign = (local_dir.y > 0) ? 1.0f : -1.0f;
        v4f local[4] = {
            {-he.x, sign * he.y, -he.z, 0},
            { he.x, sign * he.y, -he.z, 0},
            { he.x, sign * he.y,  he.z, 0},
            {-he.x, sign * he.y,  he.z, 0}
        };
        
        for (int i = 0; i < 4; i++) {
            v4f_rotate_quat(&face_vertices[i], &local[i], &tx->rotation);
            v4f_add(&face_vertices[i], &face_vertices[i], &tx->position);
        }
        *num_vertices = 4;
    } else {
        // Z face
        float sign = (local_dir.z > 0) ? 1.0f : -1.0f;
        v4f local[4] = {
            {-he.x, -he.y, sign * he.z, 0},
            { he.x, -he.y, sign * he.z, 0},
            { he.x,  he.y, sign * he.z, 0},
            {-he.x,  he.y, sign * he.z, 0}
        };
        
        for (int i = 0; i < 4; i++) {
            v4f_rotate_quat(&face_vertices[i], &local[i], &tx->rotation);
            v4f_add(&face_vertices[i], &face_vertices[i], &tx->position);
        }
        *num_vertices = 4;
    }
}

void get_support_face(const CollisionShape *shape, const Transform *tx,
                     const v4f *direction, v4f *face_vertices, int *num_vertices) {
    switch (shape->type) {
        case SHAPE_BOX:
            get_box_face((const BoxShape*)shape->data, tx, direction, face_vertices, num_vertices);
            break;
        case SHAPE_SPHERE:
            // Sphere has single contact point
            *num_vertices = 1;
            v4f support = get_support(shape, tx, direction);
            face_vertices[0] = support;
            break;
        case SHAPE_CAPSULE:
            // Capsule simplified to single point for now
            *num_vertices = 1;
            face_vertices[0] = get_support(shape, tx, direction);
            break;
        default:
            *num_vertices = 0;
            break;
    }
}

int clip_face_against_plane(const v4f *input_vertices, int num_input,
                            v4f *output_vertices,
                            const v4f *plane_normal, float plane_distance) {
    if (num_input == 0) return 0;
    
    int num_output = 0;
    
    for (int i = 0; i < num_input; i++) {
        int next = (i + 1) % num_input;
        
        v4f curr = input_vertices[i];
        v4f next_v = input_vertices[next];
        
        float curr_dist = v4f_dot(&curr, plane_normal) - plane_distance;
        float next_dist = v4f_dot(&next_v, plane_normal) - plane_distance;
        
        // Both inside
        if (curr_dist >= 0 && next_dist >= 0) {
            output_vertices[num_output++] = next_v;
        }
        // Current inside, next outside - add intersection
        else if (curr_dist >= 0 && next_dist < 0) {
            float t = curr_dist / (curr_dist - next_dist);
            v4f edge, intersection;
            v4f_sub(&edge, &next_v, &curr);
            v4f_scale(&edge, &edge, t);
            v4f_add(&intersection, &curr, &edge);
            output_vertices[num_output++] = intersection;
        }
        // Current outside, next inside - add intersection and next
        else if (curr_dist < 0 && next_dist >= 0) {
            float t = curr_dist / (curr_dist - next_dist);
            v4f edge, intersection;
            v4f_sub(&edge, &next_v, &curr);
            v4f_scale(&edge, &edge, t);
            v4f_add(&intersection, &curr, &edge);
            output_vertices[num_output++] = intersection;
            output_vertices[num_output++] = next_v;
        }
        // Both outside - skip
    }
    
    return num_output;
}

bool generate_contact_manifold(ContactManifold *manifold,
                               const CollisionShape *shape_a, const Transform *tx_a,
                               const CollisionShape *shape_b, const Transform *tx_b,
                               const EPAResult *epa) {
    if (!epa->valid || epa->penetration_depth <= 0) {
        manifold->num_points = 0;
        return false;
    }
    
    manifold->normal = epa->normal;
    manifold->num_points = 0;
    
    // For sphere-sphere, single contact point
    if (shape_a->type == SHAPE_SPHERE && shape_b->type == SHAPE_SPHERE) {
        const SphereShape *sa = (const SphereShape*)shape_a->data;
        const SphereShape *sb = (const SphereShape*)shape_b->data;
        
        // Contact point at midpoint between sphere surfaces
        v4f dir_ab;
        v4f_sub(&dir_ab, &tx_b->position, &tx_a->position);
        float dist = v4f_length(&dir_ab);
        v4f_normalize(&dir_ab, &dir_ab);
        
        v4f point_a, point_b;
        v4f_scale(&point_a, &dir_ab, sa->radius);
        v4f_add(&point_a, &point_a, &tx_a->position);
        
        v4f neg_dir;
        v4f_scale(&neg_dir, &dir_ab, -sb->radius);
        v4f_add(&point_b, &neg_dir, &tx_b->position);
        
        manifold->points[0].position_a = point_a;
        manifold->points[0].position_b = point_b;
        manifold->points[0].penetration = epa->penetration_depth;
        manifold->points[0].normal = epa->normal;
        manifold->num_points = 1;
        return true;
    }
    
    // For box-box or mixed shapes, use face clipping
    // Get reference face (shape with face most aligned with normal)
    v4f ref_face[8], inc_face[8];
    int num_ref = 0, num_inc = 0;
    
    v4f neg_normal;
    v4f_scale(&neg_normal, &epa->normal, -1.0f);
    
    get_support_face(shape_a, tx_a, &epa->normal, ref_face, &num_ref);
    get_support_face(shape_b, tx_b, &neg_normal, inc_face, &num_inc);
    
    // Simplified: use incident face vertices as contact points if penetrating
    for (int i = 0; i < num_inc && manifold->num_points < MAX_MANIFOLD_POINTS; i++) {
        // Project onto reference face and check penetration
        float depth = v4f_dot(&inc_face[i], &epa->normal);
        float ref_depth = num_ref > 0 ? v4f_dot(&ref_face[0], &epa->normal) : 0;
        
        if (depth < ref_depth + 0.01f) { // Small tolerance
            manifold->points[manifold->num_points].position_a = inc_face[i];
            manifold->points[manifold->num_points].position_b = inc_face[i];
            manifold->points[manifold->num_points].penetration = ref_depth - depth;
            manifold->points[manifold->num_points].normal = epa->normal;
            manifold->num_points++;
        }
    }
    
    return manifold->num_points > 0;
}
