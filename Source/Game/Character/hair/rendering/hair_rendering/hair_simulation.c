// Disabled for build fix
#include <common.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  Vec3 *positions;
  Vec3 *prev_positions;
  Vec3 *velocities;
  uint32_t strand_count;
  uint32_t verts_per_strand;
  float stiffness;
  float damping;
} HairSimSystem;

static HairSimSystem g_hair_sim = {0};

void hair_sim_init(uint32_t strands, uint32_t verts_per_strand) {
  g_hair_sim.strand_count = strands;
  g_hair_sim.verts_per_strand = verts_per_strand;
  uint32_t total_verts = strands * verts_per_strand;

  g_hair_sim.positions = (Vec3 *)malloc(total_verts * sizeof(Vec3));
  g_hair_sim.prev_positions = (Vec3 *)malloc(total_verts * sizeof(Vec3));
  g_hair_sim.velocities = (Vec3 *)malloc(total_verts * sizeof(Vec3));

  // Initialize hair in a vertical rest position
  for (uint32_t s = 0; s < strands; s++) {
    for (uint32_t v = 0; v < verts_per_strand; v++) {
      uint32_t idx = s * verts_per_strand + v;
      g_hair_sim.positions[idx] = vec3(s * 0.01f, -v * 0.05f, 0.0f);
      g_hair_sim.prev_positions[idx] = g_hair_sim.positions[idx];
      g_hair_sim.velocities[idx] = vec3_zero();
    }
  }
}

void hair_sim_update(float delta_time) {
  if (!g_hair_sim.positions)
    return;

  float dt_sq = delta_time * delta_time;
  Vec3 gravity = vec3(0.0f, -9.81f, 0.0f);

  // Simple Verlet integration shell
  for (uint32_t i = 0;
       i < g_hair_sim.strand_count * g_hair_sim.verts_per_strand; i++) {
    // Skip root vertices (assuming index 0 per strand is fixed)
    if (i % g_hair_sim.verts_per_strand == 0)
      continue;

    Vec3 temp = g_hair_sim.positions[i];

    // x_new = x + (x - x_prev) + a * dt^2
    Vec3 velocity =
        vec3_sub(g_hair_sim.positions[i], g_hair_sim.prev_positions[i]);
    Vec3 acceleration = gravity; // Simplified

    g_hair_sim.positions[i] =
        vec3_add(vec3_add(g_hair_sim.positions[i], velocity),
                 vec3_mul(acceleration, dt_sq));
    g_hair_sim.prev_positions[i] = temp;
  }

  // Constraint satisfaction stub (Strand length constraints)
}

void hair_sim_shutdown(void) {
  if (g_hair_sim.positions)
    free(g_hair_sim.positions);
  if (g_hair_sim.prev_positions)
    free(g_hair_sim.prev_positions);
  if (g_hair_sim.velocities)
    free(g_hair_sim.velocities);
  memset(&g_hair_sim, 0, sizeof(g_hair_sim));
}
#include <common.h>
