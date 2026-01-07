#include <math/mat4.h>
#include <npc/npc_visuals.h>
// #include <math/utils.h> // Does not exist
#include <math.h>
#include <string.h>

// Helper to add a box to the mesh with a given transform and color
static void add_box(Mesh *batch, Vec3 size, Vec3 offset, Mat4 transform,
                    Vertex color_vert) {
  Vec3 half = vec3_mul(size, 0.5f);
  Vec3 center = offset;

  // Define 8 corners of the box relative to center
  Vec3 corners[8] = {
      vec3_sub(center, half), // 0: -x -y -z
      vec3(center.x + half.x, center.y - half.y,
           center.z - half.z), // 1: +x -y -z
      vec3(center.x - half.x, center.y + half.y,
           center.z - half.z), // 2: -x +y -z
      vec3(center.x + half.x, center.y + half.y,
           center.z - half.z), // 3: +x +y -z
      vec3(center.x - half.x, center.y - half.y,
           center.z + half.z), // 4: -x -y +z
      vec3(center.x + half.x, center.y - half.y,
           center.z + half.z), // 5: +x -y +z
      vec3(center.x - half.x, center.y + half.y,
           center.z + half.z), // 6: -x +y +z
      vec3_add(center, half)   // 7: +x +y +z
  };

  // Transform corners
  // Transform corners
  for (int i = 0; i < 8; i++) {
    corners[i] = mat4_transform_point(transform, corners[i]);
  }

  // Faces (using indices for quads)
  // 0: Front (+z), 1: Back (-z), 2: Top (+y), 3: Bottom (-y), 4: Right (+x), 5:
  // Left (-x) Indices for triangles
  u32 base_index = batch->vertex_count;

  // Push vertices (we need distinct vertices if we want flat shading or UVs,
  // but here we reuse for simple color) Actually, for proper normals/UVs we
  // need 4 verts per face = 24 verts. Let's do simplified logic: just push
  // transformed 8 corners? No, normals needed. So 24 vertices.

  Vec3 normals[6] = {vec3(0, 0, 1),  vec3(0, 0, -1), vec3(0, 1, 0),
                     vec3(0, -1, 0), vec3(1, 0, 0),  vec3(-1, 0, 0)};

  // Vertex indices for each face (CCW)
  int face_indices[6][4] = {
      {4, 5, 7, 6}, // Front (+Z)
      {1, 0, 2, 3}, // Back (-Z)
      {2, 3, 7, 6}, // Top (+Y)
      {4, 5, 1, 0}, // Bottom (-Y)
      {5, 1, 3, 7}, // Right (+X)
      {0, 4, 6, 2}  // Left (-X)
  };

  for (int f = 0; f < 6; f++) {
    Vec3 normal = normals[f];
    // Rotate normal
    normal = mat4_transform_vec3(transform, normal);
    normal = vec3_normalize(normal);

    for (int v = 0; v < 4; v++) {
      Vertex vert = color_vert;
      vert.position = corners[face_indices[f][v]];
      vert.normal = normal;
      // UVs could be assigned here if we had textures
      vert.uv = vec2(v & 1 ? 1.0f : 0.0f, v & 2 ? 0.0f : 1.0f);

      if (batch->vertex_count < batch->vertex_capacity) {
        batch->vertices[batch->vertex_count++] = vert;
      }
    }

    // Two triangles per face
    if (batch->index_count + 6 <= batch->index_capacity) {
      u32 start = base_index + f * 4;
      batch->indices[batch->index_count++] = start + 0;
      batch->indices[batch->index_count++] = start + 1;
      batch->indices[batch->index_count++] = start + 2;
      batch->indices[batch->index_count++] = start + 0;
      batch->indices[batch->index_count++] = start + 2;
      batch->indices[batch->index_count++] = start + 3;
    }
  }
}

void npc_visuals_init(void) {
  // No static resources needed yet
}

void npc_visuals_free(void) {
  // Nothing to free
}

void npc_visuals_append_mesh(Mesh *batch, NPCComponent *npc,
                             TransformComponent *transform, f32 delta_time) {
  if (!batch || !npc || !transform)
    return;

  // Base transform
  Mat4 world = mat4_translate(transform->position);
  Mat4 rotation = mat4_mul(mat4_rotate_y(transform->rotation.y),
                           mat4_mul(mat4_rotate_x(transform->rotation.x),
                                    mat4_rotate_z(transform->rotation.z)));
  Mat4 model = mat4_mul(world, rotation);

  // Determines animation phase
  static f32 s_global_time = 0.0f;
  s_global_time += delta_time;

  f32 walk_cycle = 0.0f;
  bool is_moving = false;

  // Check state for movement
  if (npc->state == NPC_STATE_WANDERING || npc->state == NPC_STATE_CHASING ||
      npc->state == NPC_STATE_FLEEING) {
    // Use global time + entity ID to desync
    walk_cycle = (s_global_time * 10.0f) + (npc->type * 1.5f);
    is_moving = true;
  }

  f32 leg_angle = is_moving ? sinf(walk_cycle) * 45.0f * DEG_TO_RAD : 0.0f;
  f32 arm_angle = is_moving ? cosf(walk_cycle) * 45.0f * DEG_TO_RAD : 0.0f;

  // Common colors
  // Vertex struct doesn't have color, only texture_id.
  // We assume default values for other fields.
  // u8 skin_tex = 4; // Wood-like (Villager)
  // u8 clothes_tex = 11; // Wool-like
  // u8 zombie_tex = 3; // Greenish
  // u8 skeleton_tex = 1; // Stone/White
  // u8 creeper_tex = 3; // Green

  u8 skin_tex = 4;
  u8 clothes_tex = 11;
  u8 zombie_tex = 3;
  u8 skeleton_tex = 1;
  u8 creeper_tex = 3;

  Vertex base_vert = {.position = vec3_zero(),
                      .normal = vec3(0, 1, 0),
                      .uv = vec2(0, 0),
                      .ao = 0,
                      .light = 15,
                      .texture_id = 0,
                      .wave_phase = 0.0f};

  // --- Humanoid (Villager, Zombie, Skeleton) ---
  if (npc->type == NPC_TYPE_VILLAGER || npc->type == NPC_TYPE_ZOMBIE ||
      npc->type == NPC_TYPE_SKELETON) {
    f32 y_offset = 1.0f; // Center of body

    u8 head_id = skin_tex;
    u8 body_id = clothes_tex;

    if (npc->type == NPC_TYPE_ZOMBIE) {
      head_id = zombie_tex;
      body_id = clothes_tex;
    }
    if (npc->type == NPC_TYPE_SKELETON) {
      head_id = skeleton_tex;
      body_id = skeleton_tex;
    }

    // Body
    base_vert.texture_id = body_id;
    add_box(batch, vec3(0.5f, 0.75f, 0.25f), vec3(0, 0.75f, 0), model,
            base_vert);

    // Head
    base_vert.texture_id = head_id;
    add_box(batch, vec3(0.5f, 0.5f, 0.5f), vec3(0, 1.375f, 0), model,
            base_vert);

    // Legs
    Mat4 leg_l_m = mat4_mul(model, mat4_translate(vec3(-0.15f, 0.375f, 0)));
    leg_l_m = mat4_mul(leg_l_m, mat4_rotate_x(leg_angle));

    Mat4 leg_r_m = mat4_mul(model, mat4_translate(vec3(0.15f, 0.375f, 0)));
    leg_r_m = mat4_mul(leg_r_m, mat4_rotate_x(-leg_angle));

    base_vert.texture_id = body_id; // Pants
    add_box(batch, vec3(0.2f, 0.75f, 0.2f), vec3(0, -0.375f, 0), leg_l_m,
            base_vert);
    add_box(batch, vec3(0.2f, 0.75f, 0.2f), vec3(0, -0.375f, 0), leg_r_m,
            base_vert);

    // Arms
    Mat4 arm_l_m = mat4_mul(model, mat4_translate(vec3(-0.35f, 1.0f, 0)));
    arm_l_m = mat4_mul(arm_l_m, mat4_rotate_x(arm_angle));

    Mat4 arm_r_m = mat4_mul(model, mat4_translate(vec3(0.35f, 1.0f, 0)));
    arm_r_m = mat4_mul(arm_r_m, mat4_rotate_x(-arm_angle));

    // Zombie arms raised
    if (npc->type == NPC_TYPE_ZOMBIE) {
      arm_l_m = mat4_mul(model, mat4_translate(vec3(-0.35f, 1.0f, 0)));
      arm_l_m = mat4_mul(arm_l_m, mat4_rotate_x(90.0f * DEG_TO_RAD));

      arm_r_m = mat4_mul(model, mat4_translate(vec3(0.35f, 1.0f, 0)));
      arm_r_m = mat4_mul(arm_r_m, mat4_rotate_x(90.0f * DEG_TO_RAD));
    }

    if (npc->type == NPC_TYPE_VILLAGER) {
      // Villager crossed arms
      add_box(batch, vec3(0.2f, 0.6f, 0.2f), vec3(0, -0.2f, 0), arm_l_m,
              base_vert); // Simplified
      add_box(batch, vec3(0.2f, 0.6f, 0.2f), vec3(0, -0.2f, 0), arm_r_m,
              base_vert);
    } else {
      add_box(batch, vec3(0.2f, 0.75f, 0.2f), vec3(0, -0.25f, 0), arm_l_m,
              base_vert);
      add_box(batch, vec3(0.2f, 0.75f, 0.2f), vec3(0, -0.25f, 0), arm_r_m,
              base_vert);
    }
  }
  // --- Creeper ---
  else if (npc->type == NPC_TYPE_CREEPER) {
    base_vert.texture_id = creeper_tex;

    // Body
    add_box(batch, vec3(0.5f, 1.0f, 0.25f), vec3(0, 0.75f, 0), model,
            base_vert);

    // Head
    add_box(batch, vec3(0.5f, 0.5f, 0.5f), vec3(0, 1.5f, 0), model, base_vert);

    // 4 Legs
    Vec3 leg_pos[4] = {{-0.2f, 0.4f, 0.2f},
                       {0.2f, 0.4f, 0.2f},
                       {-0.2f, 0.4f, -0.2f},
                       {0.2f, 0.4f, -0.2f}};

    for (int i = 0; i < 4; i++) {
      Mat4 leg_m = mat4_mul(model, mat4_translate(leg_pos[i]));
      f32 angle = (i % 2 == 0 ? 1 : -1) * leg_angle;
      leg_m = mat4_mul(leg_m, mat4_rotate_x(angle));
      add_box(batch, vec3(0.2f, 0.4f, 0.2f), vec3(0, -0.2f, 0), leg_m,
              base_vert);
    }
  }
  // --- Quadruped (Cow, Pig, Chicken?) ---
  else if (npc->type == NPC_TYPE_COW || npc->type == NPC_TYPE_PIG) {
    u8 body_tex = (npc->type == NPC_TYPE_COW) ? 11 : 6; // Wool vs Pinkish block
    base_vert.texture_id = body_tex;

    // Horizontal Body
    Mat4 body_m = mat4_mul(model, mat4_translate(vec3(0, 0.8f, 0)));
    // Rotate body 90deg? No, cow body is long along Z usually.
    add_box(batch, vec3(0.6f, 0.5f, 0.9f), vec3(0, 0, 0), body_m, base_vert);

    // Head
    Mat4 head_m = mat4_mul(model, mat4_translate(vec3(0, 1.1f, -0.5f)));
    add_box(batch, vec3(0.4f, 0.4f, 0.4f), vec3(0, 0, 0), head_m, base_vert);

    // Legs
    Vec3 leg_pos[4] = {{-0.25f, 0.6f, 0.35f},
                       {0.25f, 0.6f, 0.35f},
                       {-0.25f, 0.6f, -0.35f},
                       {0.25f, 0.6f, -0.35f}};
    for (int i = 0; i < 4; i++) {
      Mat4 leg_m = mat4_mul(model, mat4_translate(leg_pos[i]));
      f32 angle = (i % 2 == 0 ? 1 : -1) * leg_angle;
      leg_m = mat4_mul(leg_m, mat4_rotate_x(angle));
      add_box(batch, vec3(0.2f, 0.6f, 0.2f), vec3(0, -0.3f, 0), leg_m,
              base_vert);
    }
  }
}
