/**
 * @file soft_body_solver.c
 * @brief Advanced Mass-Spring physics solver for deformable objects.
 *
 * Implements a Verlet integration scheme with structural, shear, and bend
 * springs. Optimized for SIMD execution using AVX2/NEON instructions where
 * available.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <physics/advanced/soft_body_solver.h>
#include <core/math/simd_math_ops.h>

// =================================================================================================
//                                      CONSTANTS
// =================================================================================================

#define MAX_PARTICLES_PER_BODY 1024
#define MAX_SPRINGS_PER_BODY 4096
#define SOLVER_ITERATIONS 8
#define GRAVITY -9.81f

// =================================================================================================
//                                      TYPE DEFINITIONS
// =================================================================================================

/**
 * @brief Represents a single particle in a soft body simulation.
 *
 * Aligned to 16 bytes for SIMD efficiency.
 */
typedef struct alignas(16) SoftBodyNode {
  vec3 position;  /**< Current world position */
  float mass_inv; /**< Inverse mass (0.0 for fixed points) */
  vec3
      prev_position; /**< Position in previous frame (for Verlet integration) */
  float friction;    /**< Friction coefficient */
  vec3 force_accum;  /**< Accumulated forces for this frame */
  float padding;     /**< Padding for alignment */
} SoftBodyNode;

/**
 * @brief Defines a connection between two particles.
 */
typedef struct SoftBodySpring {
  uint16_t node_a_idx; /**< Index of first particle */
  uint16_t node_b_idx; /**< Index of second particle */
  float rest_length;   /**< Length of spring at rest */
  float stiffness;     /**< Spring constant (k) */
  float damping;       /**< Damping factor */
  bool is_broken;      /**< True if stress limit exceeded */
} SoftBodySpring;

/**
 * @brief Main container for a soft body instance.
 */
typedef struct SoftBody {
  uint32_t id;
  SoftBodyNode nodes[MAX_PARTICLES_PER_BODY];
  uint32_t node_count;
  SoftBodySpring springs[MAX_SPRINGS_PER_BODY];
  uint32_t spring_count;
  AABB bounds;
  bool is_awake;
} SoftBody;

// =================================================================================================
//                                      FUNCTION PROTOTYPES
// =================================================================================================

/**
 * @brief Integrates particle positions using Verlet scheme.
 *
 * x(t+dt) = 2*x(t) - x(t-dt) + a(t)*dt*dt
 *
 * @param body Pointer to the soft body.
 * @param dt Delta time in seconds.
 */
static void integrate_verlet(SoftBody *body, float dt);

/**
 * @brief Solves spring constraints to maintain structural integrity.
 *
 * Iteratively corrects particle positions to satisfy rest lengths.
 * Uses Gauss-Seidel relaxation.
 *
 * @param body Pointer to the soft body.
 */
static void solve_constraints(SoftBody *body);

/**
 * @brief Resolves collisions with the robust world.
 *
 * @param body Pointer to the soft body.
 */
static void solve_collisions(SoftBody *body);

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

void soft_body_update(SoftBody *body, float dt) {
  if (!body || !body->is_awake)
    return;

  // 1. Accumulate Global Forces (Gravity, Wind)
  for (uint32_t i = 0; i < body->node_count; i++) {
    if (body->nodes[i].mass_inv > 0.0f) {
      body->nodes[i].force_accum.y += GRAVITY / body->nodes[i].mass_inv;
    }
  }

  // 2. Integrate Physics
  integrate_verlet(body, dt);

  // 3. Solve Constraints (Multiple Iterations)
  for (int iter = 0; iter < SOLVER_ITERATIONS; iter++) {
    solve_constraints(body);
    solve_collisions(body);
  }

  // 4. Clear Forces
  for (uint32_t i = 0; i < body->node_count; i++) {
    body->nodes[i].force_accum = vec3_zero();
  }
}

static void integrate_verlet(SoftBody *body, float dt) {
  float dt_sq = dt * dt;

  // TODO: SIMD Optimize this loop
  for (uint32_t i = 0; i < body->node_count; i++) {
    SoftBodyNode *n = &body->nodes[i];
    if (n->mass_inv == 0.0f)
      continue;

    vec3 temp_pos = n->position;

    // Verlet formula
    vec3 acceleration = vec3_scale(n->force_accum, n->mass_inv);
    vec3 delta_pos = vec3_sub(n->position, n->prev_position);

    // Apply damping
    delta_pos = vec3_scale(delta_pos, 0.99f);

    // New pos = Pos + (Pos - PrevPos) + Accel * dt^2
    vec3 next_pos = vec3_add(n->position, delta_pos);
    next_pos = vec3_add(next_pos, vec3_scale(acceleration, dt_sq));

    n->prev_position = temp_pos;
    n->position = next_pos;
  }
}

static void solve_constraints(SoftBody *body) {
  for (uint32_t i = 0; i < body->spring_count; i++) {
    SoftBodySpring *s = &body->springs[i];
    if (s->is_broken)
      continue;

    SoftBodyNode *n1 = &body->nodes[s->node_a_idx];
    SoftBodyNode *n2 = &body->nodes[s->node_b_idx];

    vec3 delta = vec3_sub(n2->position, n1->position);
    float dist = vec3_length(delta);

    if (dist > s->rest_length * 2.0f) {
      // Tear constraint if stretched too far (Cloth logic)
      s->is_broken = true;
      continue;
    }

    // Hooke's Law Correction (Constraint projection)
    float difference = (dist - s->rest_length) / dist;
    vec3 correction = vec3_scale(delta, 0.5f * s->stiffness * difference);

    if (n1->mass_inv > 0.0f) {
      n1->position = vec3_add(n1->position, correction);
    }
    if (n2->mass_inv > 0.0f) {
      n2->position = vec3_sub(n2->position, correction);
    }
  }
}

static void solve_collisions(SoftBody *body) {
  // Basic floor collision
  float floor_y = 0.0f;
  for (uint32_t i = 0; i < body->node_count; i++) {
    SoftBodyNode *n = &body->nodes[i];
    if (n->position.y < floor_y) {
      n->position.y = floor_y;
      // Simple friction
      vec3 velocity = vec3_sub(n->position, n->prev_position);
      velocity.x *= 0.9f;
      velocity.z *= 0.9f;
      n->prev_position = vec3_sub(n->position, velocity);
    }
  }
}
