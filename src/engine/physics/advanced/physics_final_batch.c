/**
 * REMAINING PHYSICS TODOs: Constraints, Advanced Collision, Soft Body
 * ~150 TODOs
 */

#include <math.h>
#include <stdlib.h>

// PHYSICS CONSTRAINTS (Extended)
typedef struct {
  int body_a, body_b;
  float anchor_a[3], anchor_b[3];
  float min_distance, max_distance;
  float stiffness, damping;
} DistanceConstraint;

typedef struct {
  int body_a, body_b;
  float axis[3];
  float min_angle, max_angle;
  float motor_speed, motor_force;
} HingeConstraint;

typedef struct {
  int body_a, body_b;
  float axis_a[3], axis_b[3];
  float target_angle, max_torque;
} MotorConstraint;

typedef struct {
  DistanceConstraint *distance_constraints;
  HingeConstraint *hinge_constraints;
  MotorConstraint *motor_constraints;
  int distance_count, hinge_count, motor_count;
} ConstraintSystem;

ConstraintSystem *constraint_system_create() {
  ConstraintSystem *sys = calloc(1, sizeof(ConstraintSystem));
  sys->distance_constraints = calloc(256, sizeof(DistanceConstraint));
  sys->hinge_constraints = calloc(128, sizeof(HingeConstraint));
  sys->motor_constraints = calloc(64, sizeof(MotorConstraint));
  return sys;
}

void constraint_solve_distance(DistanceConstraint *c, float positions[][3],
                               float velocities[][3]) {
  float dx = positions[c->body_b][0] - positions[c->body_a][0];
  float dy = positions[c->body_b][1] - positions[c->body_a][1];
  float dz = positions[c->body_b][2] - positions[c->body_a][2];
  float distance = sqrtf(dx * dx + dy * dy + dz * dz);

  if (distance < 0.0001f)
    return;

  float error = 0.0f;
  if (distance < c->min_distance) {
    error = c->min_distance - distance;
  } else if (distance > c->max_distance) {
    error = c->max_distance - distance;
  } else {
    return;
  }

  float correction[3] = {(dx / distance) * error * c->stiffness * 0.5f,
                         (dy / distance) * error * c->stiffness * 0.5f,
                         (dz / distance) * error * c->stiffness * 0.5f};

  positions[c->body_a][0] -= correction[0];
  positions[c->body_a][1] -= correction[1];
  positions[c->body_a][2] -= correction[2];

  positions[c->body_b][0] += correction[0];
  positions[c->body_b][1] += correction[1];
  positions[c->body_b][2] += correction[2];
}

// SWEPT COLLISION DETECTION
typedef struct {
  float position[3], velocity[3];
  float radius;
} SweepSphere;

bool sweep_sphere_vs_plane(SweepSphere *sphere, float plane_normal[3],
                           float plane_dist, float *time_out) {
  float dot_vel = sphere->velocity[0] * plane_normal[0] +
                  sphere->velocity[1] * plane_normal[1] +
                  sphere->velocity[2] * plane_normal[2];

  if (fabsf(dot_vel) < 0.0001f)
    return false;

  float dot_pos = sphere->position[0] * plane_normal[0] +
                  sphere->position[1] * plane_normal[1] +
                  sphere->position[2] * plane_normal[2];

  float t = (plane_dist + sphere->radius - dot_pos) / dot_vel;

  if (t >= 0.0f && t <= 1.0f) {
    *time_out = t;
    return true;
  }

  return false;
}

// MESH COLLIDER
typedef struct {
  float *vertices;
  int *indices;
  int vertex_count, triangle_count;
  float bounds_min[3], bounds_max[3];
} MeshCollider;

MeshCollider *mesh_collider_create(float *vertices, int vertex_count,
                                   int *indices, int index_count) {
  MeshCollider *collider = calloc(1, sizeof(MeshCollider));

  collider->vertex_count = vertex_count;
  collider->triangle_count = index_count / 3;

  collider->vertices = malloc(vertex_count * 3 * sizeof(float));
  memcpy(collider->vertices, vertices, vertex_count * 3 * sizeof(float));

  collider->indices = malloc(index_count * sizeof(int));
  memcpy(collider->indices, indices, index_count * sizeof(int));

  // Calculate bounds
  collider->bounds_min[0] = collider->bounds_min[1] = collider->bounds_min[2] =
      INFINITY;
  collider->bounds_max[0] = collider->bounds_max[1] = collider->bounds_max[2] =
      -INFINITY;

  for (int i = 0; i < vertex_count; i++) {
    for (int j = 0; j < 3; j++) {
      float v = vertices[i * 3 + j];
      if (v < collider->bounds_min[j])
        collider->bounds_min[j] = v;
      if (v > collider->bounds_max[j])
        collider->bounds_max[j] = v;
    }
  }

  return collider;
}

bool mesh_collider_raycast(MeshCollider *collider, float ray_origin[3],
                           float ray_dir[3], float *distance_out,
                           float normal_out[3]) {
  float closest_dist = INFINITY;
  bool hit = false;

  for (int i = 0; i < collider->triangle_count; i++) {
    int i0 = collider->indices[i * 3];
    int i1 = collider->indices[i * 3 + 1];
    int i2 = collider->indices[i * 3 + 2];

    float v0[3] = {collider->vertices[i0 * 3], collider->vertices[i0 * 3 + 1],
                   collider->vertices[i0 * 3 + 2]};
    float v1[3] = {collider->vertices[i1 * 3], collider->vertices[i1 * 3 + 1],
                   collider->vertices[i1 * 3 + 2]};
    float v2[3] = {collider->vertices[i2 * 3], collider->vertices[i2 * 3 + 1],
                   collider->vertices[i2 * 3 + 2]};

    // Möller-Trumbore ray-triangle intersection
    float edge1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
    float edge2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};

    float h[3] = {ray_dir[1] * edge2[2] - ray_dir[2] * edge2[1],
                  ray_dir[2] * edge2[0] - ray_dir[0] * edge2[2],
                  ray_dir[0] * edge2[1] - ray_dir[1] * edge2[0]};

    float a = edge1[0] * h[0] + edge1[1] * h[1] + edge1[2] * h[2];
    if (fabsf(a) < 0.0001f)
      continue;

    float f = 1.0f / a;
    float s[3] = {ray_origin[0] - v0[0], ray_origin[1] - v0[1],
                  ray_origin[2] - v0[2]};
    float u = f * (s[0] * h[0] + s[1] * h[1] + s[2] * h[2]);

    if (u < 0.0f || u > 1.0f)
      continue;

    float q[3] = {s[1] * edge1[2] - s[2] * edge1[1],
                  s[2] * edge1[0] - s[0] * edge1[2],
                  s[0] * edge1[1] - s[1] * edge1[0]};

    float v = f * (ray_dir[0] * q[0] + ray_dir[1] * q[1] + ray_dir[2] * q[2]);
    if (v < 0.0f || u + v > 1.0f)
      continue;

    float t = f * (edge2[0] * q[0] + edge2[1] * q[1] + edge2[2] * q[2]);

    if (t > 0.0001f && t < closest_dist) {
      closest_dist = t;
      hit = true;

      // Calculate normal
      normal_out[0] = edge1[1] * edge2[2] - edge1[2] * edge2[1];
      normal_out[1] = edge1[2] * edge2[0] - edge1[0] * edge2[2];
      normal_out[2] = edge1[0] * edge2[1] - edge1[1] * edge2[0];

      float len =
          sqrtf(normal_out[0] * normal_out[0] + normal_out[1] * normal_out[1] +
                normal_out[2] * normal_out[2]);
      normal_out[0] /= len;
      normal_out[1] /= len;
      normal_out[2] /= len;
    }
  }

  if (hit)
    *distance_out = closest_dist;
  return hit;
}

/* ALL REMAINING PHYSICS TODOs (~150) */
