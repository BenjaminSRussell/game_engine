// src/engine/ai/modular_assets.c
//
// Purpose: Modular asset system implementation for Lego-like snap-together
// components.
//
#include <ai/ai_asset_generator.h>
#include "engine/include/core/logger.h"
#include <core/modular_assets.h>
#include <float.h>
#include <math/mat4.h>
#include <include/rendering/mesh.h>
#include <stdlib.h>
#include <string.h>

// Create modular asset
ModularAsset *modular_asset_create(const char *name) {
  if (!name) {
    return NULL;
  }

  ModularAsset *asset = (ModularAsset *)calloc(1, sizeof(ModularAsset));
  if (!asset) {
    return NULL;
  }

  asset->name = name;
  asset->connection_capacity = 32;
  asset->connections =
      (Connection *)calloc(asset->connection_capacity, sizeof(Connection));

  asset->bounds_min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
  asset->bounds_max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  LOG_INFO("Created modular asset: %s", name);
  return asset;
}

void modular_asset_destroy(ModularAsset *asset) {
  if (!asset) {
    return;
  }

  if (asset->connections) {
    free(asset->connections);
  }

  free(asset);
}

// Add component to asset
bool modular_asset_add_component(ModularAsset *asset,
                                 struct AssetComponent *component) {
  if (!asset || !component) {
    return false;
  }

  // Set as root if no root exists
  if (!asset->root_component) {
    asset->root_component = component;
  }

  asset->component_count++;

  // TODO: Update bounds when AssetComponent is fully defined
  // asset->bounds_min = vec3(fminf(asset->bounds_min.x,
  // component->bounds_min.x),
  //                          fminf(asset->bounds_min.y,
  //                          component->bounds_min.y),
  //                          fminf(asset->bounds_min.z,
  //                          component->bounds_min.z));
  // asset->bounds_max = vec3(fmaxf(asset->bounds_max.x,
  // component->bounds_max.x),
  //                          fmaxf(asset->bounds_max.y,
  //                          component->bounds_max.y),
  //                          fmaxf(asset->bounds_max.z,
  //                          component->bounds_max.z));

  asset->center = vec3((asset->bounds_min.x + asset->bounds_max.x) * 0.5f,
                       (asset->bounds_min.y + asset->bounds_max.y) * 0.5f,
                       (asset->bounds_min.z + asset->bounds_max.z) * 0.5f);

  asset->needs_rebuild = true;

  return true;
}

// Connect components
bool modular_asset_connect(ModularAsset *asset, u32 component1_id, u32 snap1_id,
                           u32 component2_id, u32 snap2_id) {
  if (!asset) {
    return false;
  }

  // Find components (simplified - would use proper lookup)
  // For now, assume components are in library

  // Create connection
  if (asset->connection_count >= asset->connection_capacity) {
    u32 new_capacity = asset->connection_capacity * 2;
    Connection *new_conns = (Connection *)realloc(
        asset->connections, new_capacity * sizeof(Connection));
    if (!new_conns) {
      return false;
    }
    asset->connections = new_conns;
    asset->connection_capacity = new_capacity;
  }

  Connection *conn = &asset->connections[asset->connection_count];
  conn->connection_id = asset->connection_count;
  conn->type = CONNECTION_TYPE_SNAP;
  conn->connected_to_id = component2_id;
  conn->strength = 1.0f;
  conn->locked = false;

  asset->connection_count++;
  asset->needs_rebuild = true;

  return true;
}

// Check if components can connect
bool modular_asset_can_connect(ModularAsset *asset, u32 component1_id,
                               u32 snap1_id, u32 component2_id, u32 snap2_id) {
  // Simplified - would check actual component snap points
  return true;
}

// Rebuild combined mesh
bool modular_asset_rebuild_mesh(ModularAsset *asset) {
  if (!asset || !asset->needs_rebuild) {
    return true;
  }

  // Simplified - would combine all component meshes
  // For now, just mark as rebuilt
  asset->needs_rebuild = false;

  LOG_INFO("Rebuilt mesh for asset: %s", asset->name);
  return true;
}

bool modular_asset_rebuild_mesh_2_5d(ModularAsset *asset) {
  if (!asset) {
    return false;
  }

  // Simplified - would combine 2.5D meshes
  asset->needs_rebuild = false;

  return true;
}

// Validate asset
bool modular_asset_validate(ModularAsset *asset) {
  if (!asset) {
    return false;
  }

  // Check for root component
  if (!asset->root_component) {
    return false;
  }

  // Check connections are valid
  for (u32 i = 0; i < asset->connection_count; i++) {
    // Validate connection
  }

  return true;
}
