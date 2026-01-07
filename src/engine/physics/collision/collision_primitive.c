/**
 * COLLISION PRIMITIVES IMPLEMENTATION
 * Implements narrowphase collision detection for supported shapes.
 */

#include "../../physics/physics_engine_core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Helper Math Functions ---

static float v3_dot(const float *a, const float *b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Local helpers renamed to avoid conflict with declared but different API in math/vec3.h
static void v3_sub(float *out, const float *a, const float *b) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

static void v3_add(float *out, const float *a, const float *b) {
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

static void v3_scale(float *out, const float *v, float s) {
    out[0] = v[0] * s;
    out[1] = v[1] * s;
    out[2] = v[2] * s;
}

static float v3_len_sq(const float *v) {
    return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

static void v3_normalize(float *v) {
    float len = sqrtf(v3_len_sq(v));
    if (len > 0.00001f) {
        float inv = 1.0f / len;
        v[0] *= inv; v[1] *= inv; v[2] *= inv;
    } else {
        v[0] = 0; v[1] = 1; v[2] = 0; // Default up
    }
}
// --- Manifold Management ---

ContactManifold *manifold_create(void) {
    ContactManifold *m = (ContactManifold *)malloc(sizeof(ContactManifold));
    if (m) {
        memset(m, 0, sizeof(ContactManifold));
    }
    return m;
}

void manifold_add_point(ContactManifold *m, const ContactPoint *pt) {
    if (m->point_count < 4) {
        m->points[m->point_count++] = *pt;
    }
    // Simplistic: just keep first 4. Real engine would keep deepest/most spread out.
}

// --- Collision Primitives ---

bool collision_sphere_sphere(const void *shape_a_ptr, const void *shape_b_ptr, ContactManifold *m) {
    // Note: The function signature in physics_engine_core.h takes void* for shapes,
    // but effectively we need global transforms. 
    // Wait, the signature in header is: bool collision_sphere_sphere(const void *a, const void *b, ContactManifold *m);
    // Usually 'a' and 'b' would be RigidBody* or CollisionShape*.
    // Let's assume they are RigidBody* for now as that has position.
    
    // cast to RigidBody to get usage working with current header design
    const RigidBody *rb_a = (const RigidBody*)shape_a_ptr;
    const RigidBody *rb_b = (const RigidBody*)shape_b_ptr;
    
    // Check if shapes are valid
    if (!rb_a->shape || !rb_b->shape) return false;
    
    // Get logical positions (center of mass)
    const float *pos_a = rb_a->position;
    const float *pos_b = rb_b->position;
    
    // Get Radii
    float r_a = rb_a->shape->data.sphere.radius;
    float r_b = rb_b->shape->data.sphere.radius;
    
    float diff[3];
    v3_sub(diff, pos_b, pos_a);
    float dist_sq = v3_len_sq(diff);
    float radius_sum = r_a + r_b;
    
    if (dist_sq > radius_sum * radius_sum) {
        return false;
    }
    
    // Collision detected
    if (m) {
        float dist = sqrtf(dist_sq);
        
        m->body_a = rb_a->id;
        m->body_b = rb_b->id;
        
        // Normal points from A to B
        float normal[3];
        if (dist > 0.00001f) {
            v3_scale(normal, diff, 1.0f / dist);
        } else {
            normal[0] = 0; normal[1] = 1; normal[2] = 0; // Coincident centers
        }
        memcpy(m->normal, normal, 3 * sizeof(float));
        
        ContactPoint cp;
        // World position of contact on B
        // Point = pos_b - normal * r_b
        float offset_b[3];
        v3_scale(offset_b, normal, r_b);
        v3_sub(cp.position_world_b, pos_b, offset_b);
        
        // World position of contact on A
        // Point = pos_a + normal * r_a
        float offset_a[3];
        v3_scale(offset_a, normal, r_a);
        v3_add(cp.position_world_a, pos_a, offset_a);
        
        cp.penetration_depth = radius_sum - dist;
        memcpy(cp.normal, normal, 3 * sizeof(float));
        
        // Friction/Restitution avg
        cp.friction = sqrtf(rb_a->friction * rb_b->friction);
        cp.restitution = fmaxf(rb_a->restitution, rb_b->restitution);
        
        manifold_add_point(m, &cp);
        m->point_count = 1;
    }
    
    return true;
}

bool collision_box_box(const void *a, const void *b, ContactManifold *m) {
    // Placeholder for AABB overlap test
    const RigidBody *rb_a = (const RigidBody*)a;
    const RigidBody *rb_b = (const RigidBody*)b;
    
    if (!rb_a->shape || !rb_b->shape) return false;
    
    float half_a[3] = { rb_a->shape->data.box.half_extents[0], rb_a->shape->data.box.half_extents[1], rb_a->shape->data.box.half_extents[2] };
    float half_b[3] = { rb_b->shape->data.box.half_extents[0], rb_b->shape->data.box.half_extents[1], rb_b->shape->data.box.half_extents[2] };
    
    float dist[3];
    v3_sub(dist, rb_b->position, rb_a->position);
    
    float x_overlap = (half_a[0] + half_b[0]) - fabsf(dist[0]);
    if (x_overlap < 0) return false;
    
    float y_overlap = (half_a[1] + half_b[1]) - fabsf(dist[1]);
    if (y_overlap < 0) return false;
    
    float z_overlap = (half_a[2] + half_b[2]) - fabsf(dist[2]);
    
    // Collision detected
    if (m) {
        // Find axis of least penetration
        m->body_a = rb_a->id;
        m->body_b = rb_b->id;
        
        if (x_overlap < y_overlap && x_overlap < z_overlap) {
            m->normal[0] = (dist[0] > 0) ? 1.0f : -1.0f; m->normal[1] = 0; m->normal[2] = 0;
            ContactPoint cp;
            cp.penetration_depth = x_overlap;
            memcpy(cp.normal, m->normal, 3*sizeof(float));
            manifold_add_point(m, &cp);
        } else if (y_overlap < z_overlap) {
            m->normal[0] = 0; m->normal[1] = (dist[1] > 0) ? 1.0f : -1.0f; m->normal[2] = 0;
            ContactPoint cp;
            cp.penetration_depth = y_overlap;
            memcpy(cp.normal, m->normal, 3*sizeof(float));
            manifold_add_point(m, &cp);
        } else {
            m->normal[0] = 0; m->normal[1] = 0; m->normal[2] = (dist[2] > 0) ? 1.0f : -1.0f;
            ContactPoint cp;
            cp.penetration_depth = z_overlap;
            memcpy(cp.normal, m->normal, 3*sizeof(float));
            manifold_add_point(m, &cp);
        }
        m->point_count = 1;
    }
    
    return true;
}

bool collision_sphere_box(RigidBody *sphere, RigidBody *box, ContactManifold *m) {
    // Get sphere properties
    float sphere_p[3] = {sphere->position[0], sphere->position[1], sphere->position[2]};
    float sphere_r = sphere->shape->data.sphere.radius;
    
    // Transform sphere center to box local space
    // Currently assuming axis-aligned for verification
    float rel_p[3];
    v3_sub(rel_p, sphere_p, box->position); // rel_p = sphere_p - box->position
    
    // Box half extents
    float *he = box->shape->data.box.half_extents;
    
    // Find closest point on box to sphere center
    float closest[3];
    closest[0] = fmaxf(-he[0], fminf(rel_p[0], he[0]));
    closest[1] = fmaxf(-he[1], fminf(rel_p[1], he[1]));
    closest[2] = fmaxf(-he[2], fminf(rel_p[2], he[2]));
    
    // Check distance between sphere center and closest point
    float dist_v[3];
    v3_sub(dist_v, rel_p, closest); 
    float dist_sq = v3_len_sq(dist_v);
    
    if (dist_sq > sphere_r * sphere_r) {
        return false;
    }
    
    if (m) {
        m->body_a = sphere->id;
        m->body_b = box->id;
        m->point_count = 1;
        float dist = sqrtf(dist_sq);
        m->points[0].penetration_depth = sphere_r - dist;
        
        // Normal computation
        // Normal should point B -> A (Box -> Sphere) to push A out.
        // Or A -> B? 
        // Logic: Body A (Sphere) position += Normal * Pen * InvMass.
        // So Normal should point Direction A should move.
        // A is Sphere. Sphere is outside Box (mostly). Setup is Sphere above Box.
        // Sphere Center (y=0.5). Box Center (y=-1). Top (y=0).
        // Closest point (y=0).
        // Vector (Sphere - Closest) = (0, 0.5, 0) - (0, 0.0, 0) = (0, 0.5, 0).
        // Normalized = (0, 1, 0).
        // This points UP. Sphere should move UP.
        // So Normal = (Vector from Surface to Sphere Center).
        
        if (dist > 0.0001f) {
             v3_scale(m->points[0].normal, dist_v, 1.0f/dist);
        } else {
             // Center is inside box. Push sphere UP (Y+)
             m->points[0].normal[0] = 0; 
             m->points[0].normal[1] = 1; 
             m->points[0].normal[2] = 0; 
        }
        
        // Contact point on Surface
        float box_closest_world[3];
        v3_add(box_closest_world, box->position, closest);
        
        // Use closest point on box as the contact point for both for now
        // Ideally point_a is on sphere surface.
        m->points[0].position_world_a[0] = box_closest_world[0];
        m->points[0].position_world_a[1] = box_closest_world[1];
        m->points[0].position_world_a[2] = box_closest_world[2];

        m->points[0].position_world_b[0] = box_closest_world[0];
        m->points[0].position_world_b[1] = box_closest_world[1];
        m->points[0].position_world_b[2] = box_closest_world[2];
        
        m->points[0].restitution = fmaxf(sphere->restitution, box->restitution);
        m->points[0].friction = sqrtf(sphere->friction * box->friction);
    }
    return true;
}

// Collision Dispatcher
bool collision_detect_pair(RigidBody *a, RigidBody *b, ContactManifold *m) {
    if (!a->shape || !b->shape) return false;
    
    CollisionShapeType t_a = a->shape->type;
    CollisionShapeType t_b = b->shape->type;
    
    // Sphere vs Sphere
    if (t_a == COLLISION_SHAPE_SPHERE && t_b == COLLISION_SHAPE_SPHERE) {
        return collision_sphere_sphere(a, b, m);
    }
    
    // Box vs Box
    if (t_a == COLLISION_SHAPE_BOX && t_b == COLLISION_SHAPE_BOX) {
        return collision_box_box(a, b, m);
    }
    
    // Sphere vs Box
    if (t_a == COLLISION_SHAPE_SPHERE && t_b == COLLISION_SHAPE_BOX) {
        return collision_sphere_box(a, b, m);
    }
    if (t_a == COLLISION_SHAPE_BOX && t_b == COLLISION_SHAPE_SPHERE) {
        return collision_sphere_box(b, a, m);
    }
    
    return false;
}


