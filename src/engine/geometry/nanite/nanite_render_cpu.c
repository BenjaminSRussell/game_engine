#include "geometry/nanite/nanite_render_cpu.h"

#include "core/logger.h"

#include <math.h>
#include <stdlib.h>

static void link_child(ClusterHierarchyNode *parent, u32 child_id) {
  u32 new_count = parent->child_count + 1;
  u32 *grown =
      (u32 *)realloc(parent->child_ids, (size_t)new_count * sizeof(u32));
  if (!grown) {
    return;
  }
  grown[parent->child_count] = child_id;
  parent->child_ids = grown;
  parent->child_count = new_count;
}

static void mark_internal(ClusterHierarchyBuilder *builder, u32 node_id) {
  ClusterHierarchyNode *node;
  u32 i;

  if (!builder || node_id >= builder->node_count) {
    return;
  }
  node = &builder->nodes[node_id];
  if (!node->is_leaf) {
    return;
  }
  node->is_leaf = false;
  for (i = 0; i < builder->leaf_count; i++) {
    if (builder->leaf_nodes[i] == node_id) {
      builder->leaf_nodes[i] = builder->leaf_nodes[builder->leaf_count - 1];
      builder->leaf_count--;
      break;
    }
  }
}

static void cluster_vertex_bounds(const NaniteMesh *mesh,
                                  const NaniteCluster *cluster, Vec3 *center_out,
                                  f32 *radius_out) {
  u32 available;
  u32 count;
  u32 v;
  Vec3 center = vec3_zero();
  f32 max_dist = 0.0f;

  *center_out = vec3_zero();
  *radius_out = 0.0f;
  if (!mesh || !mesh->positions || !cluster || cluster->vertex_count == 0 ||
      mesh->vertex_count == 0 || cluster->vertex_offset >= mesh->vertex_count) {
    return;
  }

  available = mesh->vertex_count - cluster->vertex_offset;
  count = cluster->vertex_count;
  if (count > available) {
    count = available;
  }
  if (count == 0) {
    return;
  }

  for (v = 0; v < count; v++) {
    center = vec3_add(center, mesh->positions[cluster->vertex_offset + v]);
  }
  center = vec3_mul(center, 1.0f / (f32)count);
  for (v = 0; v < count; v++) {
    f32 dist = vec3_distance(mesh->positions[cluster->vertex_offset + v], center);
    if (dist > max_dist) {
      max_dist = dist;
    }
  }
  *center_out = center;
  *radius_out = max_dist;
}

ClusterHierarchyBuilder *cluster_hierarchy_builder_create(u32 max_nodes) {
  ClusterHierarchyBuilder *builder;

  if (max_nodes == 0) {
    return NULL;
  }
  builder = (ClusterHierarchyBuilder *)calloc(1, sizeof(ClusterHierarchyBuilder));
  if (!builder) {
    return NULL;
  }
  builder->nodes =
      (ClusterHierarchyNode *)calloc(max_nodes, sizeof(ClusterHierarchyNode));
  builder->leaf_nodes = (u32 *)calloc(max_nodes, sizeof(u32));
  if (!builder->nodes || !builder->leaf_nodes) {
    free(builder->nodes);
    free(builder->leaf_nodes);
    free(builder);
    return NULL;
  }
  builder->max_nodes = max_nodes;
  builder->root_node_id = UINT32_MAX;
  LOG_INFO_CAT(LOG_CAT_GRAPHICS,
               "Cluster hierarchy builder created for %u nodes", max_nodes);
  return builder;
}

u32 cluster_hierarchy_add_node(ClusterHierarchyBuilder *builder, u32 parent_id,
                               const Vec3 *center, f32 radius, u32 lod_level,
                               f32 error) {
  u32 node_id;
  ClusterHierarchyNode *node;

  if (!builder || !center || builder->node_count >= builder->max_nodes) {
    return UINT32_MAX;
  }

  node_id = builder->node_count++;
  node = &builder->nodes[node_id];
  node->cluster_id = node_id;
  node->parent_id = parent_id;
  node->child_ids = NULL;
  node->child_count = 0;
  node->bounds_center = *center;
  node->bounds_radius = radius;
  node->lod_level = lod_level;
  node->error_metric = error;
  node->is_leaf = true;

  if (parent_id != UINT32_MAX && parent_id < node_id) {
    ClusterHierarchyNode *parent = &builder->nodes[parent_id];
    link_child(parent, node_id);
    mark_internal(builder, parent_id);
  } else {
    node->parent_id = UINT32_MAX;
    builder->root_node_id = node_id;
  }

  if (node->is_leaf) {
    builder->leaf_nodes[builder->leaf_count++] = node_id;
  }

  builder->total_clusters_processed++;
  builder->average_error =
      (builder->average_error * (builder->total_clusters_processed - 1) +
       error) /
      (f32)builder->total_clusters_processed;
  if (error > builder->max_error) {
    builder->max_error = error;
  }
  return node_id;
}

static void choose_root(ClusterHierarchyBuilder *builder) {
  u32 i;
  builder->root_node_id = UINT32_MAX;
  for (i = 0; i < builder->node_count; i++) {
    if (builder->nodes[i].parent_id == UINT32_MAX) {
      builder->root_node_id = i;
    }
  }
}

void cluster_hierarchy_build(ClusterHierarchyBuilder *builder,
                             NaniteMesh *mesh) {
  u32 current_level_start;
  u32 current_level_count;
  u32 level;

  if (!builder || !mesh) {
    return;
  }

  LOG_INFO_CAT(LOG_CAT_GRAPHICS, "Building cluster hierarchy for %u clusters",
               mesh->cluster_count);

  for (u32 i = 0; i < mesh->cluster_count; i++) {
    NaniteCluster *cluster = &mesh->clusters[i];
    Vec3 center;
    f32 radius;
    if (builder->node_count >= builder->max_nodes) {
      break;
    }
    cluster_vertex_bounds(mesh, cluster, &center, &radius);
    cluster_hierarchy_add_node(builder, UINT32_MAX, &center, radius, 0, 0.01f);
  }

  current_level_start = 0;
  current_level_count = builder->node_count;
  level = 1;

  while (current_level_count > 1 && builder->node_count < builder->max_nodes) {
    u32 level_end = current_level_start + current_level_count;
    u32 next_level_start = builder->node_count;
    u32 groups_to_process = (current_level_count + 3) / 4;
    u32 group;

    if (level_end > next_level_start) {
      level_end = next_level_start;
    }

    for (group = 0;
         group < groups_to_process && builder->node_count < builder->max_nodes;
         group++) {
      Vec3 group_center = vec3_zero();
      f32 group_radius = 0.0f;
      f32 group_error = 0.0f;
      u32 child_ids[4];
      u32 child_n = 0;
      u32 i;
      u32 parent_id;
      ClusterHierarchyNode *parent;

      for (i = 0; i < 4; i++) {
        u32 child_id = current_level_start + group * 4 + i;
        ClusterHierarchyNode *child;
        if (child_id >= level_end) {
          break;
        }
        child = &builder->nodes[child_id];
        group_center = vec3_add(group_center, child->bounds_center);
        group_radius = fmaxf(group_radius, child->bounds_radius);
        group_error = fmaxf(group_error, child->error_metric);
        child_ids[child_n++] = child_id;
      }

      if (child_n == 0) {
        continue;
      }

      group_center = vec3_mul(group_center, 1.0f / (f32)child_n);
      group_radius *= 1.5f;
      group_error *= 1.2f;

      parent_id = cluster_hierarchy_add_node(builder, UINT32_MAX, &group_center,
                                             group_radius, level, group_error);
      if (parent_id == UINT32_MAX) {
        break;
      }

      parent = &builder->nodes[parent_id];
      for (i = 0; i < child_n; i++) {
        ClusterHierarchyNode *child = &builder->nodes[child_ids[i]];
        child->parent_id = parent_id;
        link_child(parent, child_ids[i]);
      }
      if (parent->child_count > 0) {
        mark_internal(builder, parent_id);
      }
    }

    if (builder->node_count == next_level_start) {
      break;
    }
    current_level_start = next_level_start;
    current_level_count = builder->node_count - next_level_start;
    level++;
    LOG_DEBUG_CAT(LOG_CAT_GRAPHICS, "Hierarchy level %u: %u nodes", level,
                  current_level_count);
  }

  choose_root(builder);
  LOG_INFO_CAT(LOG_CAT_GRAPHICS,
               "Cluster hierarchy built: %u total nodes, %u levels",
               builder->node_count, level);
}

void cluster_hierarchy_destroy(ClusterHierarchyBuilder *builder) {
  u32 i;
  if (!builder) {
    return;
  }
  if (builder->nodes) {
    for (i = 0; i < builder->node_count; i++) {
      free(builder->nodes[i].child_ids);
    }
  }
  free(builder->nodes);
  free(builder->leaf_nodes);
  free(builder);
}

void nanite_lod_cluster_range(const NaniteMesh *mesh, u32 lod, u32 *start_out,
                              u32 *count_out) {
  u32 start = 0;
  u32 count = 0;

  if (mesh && mesh->clusters && mesh->cluster_count > 0 && mesh->lod_count > 0 &&
      lod < mesh->lod_count && lod < NANITE_MAX_LOD_LEVELS) {
    start = mesh->lod_offsets[lod];
    if (start > mesh->cluster_count) {
      start = mesh->cluster_count;
    }
    if (lod + 1 < mesh->lod_count && (lod + 1) < NANITE_MAX_LOD_LEVELS) {
      u32 next = mesh->lod_offsets[lod + 1];
      if (next > mesh->cluster_count) {
        next = mesh->cluster_count;
      }
      count = (next > start) ? (next - start) : 0;
    } else {
      count = mesh->cluster_count - start;
    }
  }

  if (start_out) {
    *start_out = start;
  }
  if (count_out) {
    *count_out = count;
  }
}

static Vec3 lod_reference_center(const NaniteMesh *mesh) {
  u32 start = 0;
  u32 count = 0;
  u32 end;
  u32 i;
  Vec3 bmin;
  Vec3 bmax;

  nanite_lod_cluster_range(mesh, 0, &start, &count);
  if (count == 0) {
    return vec3_zero();
  }
  end = start + count;
  if (end > mesh->cluster_count) {
    end = mesh->cluster_count;
  }
  bmin = mesh->clusters[start].bounds_min;
  bmax = mesh->clusters[start].bounds_max;
  for (i = start + 1; i < end; i++) {
    bmin = vec3_min(bmin, mesh->clusters[i].bounds_min);
    bmax = vec3_max(bmax, mesh->clusters[i].bounds_max);
  }
  return vec3_mul(vec3_add(bmin, bmax), 0.5f);
}

u32 nanite_select_lod(const NaniteMesh *mesh, const Vec3 *camera_pos,
                      f32 error_threshold_px, f32 view_height_px) {
  u32 lods;
  u32 step;
  Vec3 center;
  f32 dist;

  if (!mesh || !mesh->clusters || mesh->cluster_count == 0 || mesh->lod_count == 0 ||
      !camera_pos || !(error_threshold_px >= 0.0f) || !(view_height_px > 0.0f)) {
    return 0;
  }

  lods = mesh->lod_count;
  if (lods > NANITE_MAX_LOD_LEVELS) {
    lods = NANITE_MAX_LOD_LEVELS;
  }

  center = lod_reference_center(mesh);
  dist = vec3_distance(center, *camera_pos);
  if (dist < 1.0e-4f) {
    dist = 1.0e-4f;
  }

  for (step = 0; step < lods; step++) {
    u32 lod = lods - 1 - step;
    u32 start = 0;
    u32 count = 0;
    u32 i;
    f32 max_error = 0.0f;
    f32 screen_error;

    nanite_lod_cluster_range(mesh, lod, &start, &count);
    if (count == 0) {
      continue;
    }
    for (i = 0; i < count; i++) {
      f32 err = mesh->clusters[start + i].lod_error;
      if (err > max_error) {
        max_error = err;
      }
    }
    screen_error = (max_error * view_height_px) / dist;
    if (screen_error <= error_threshold_px) {
      return lod;
    }
  }
  return 0;
}
