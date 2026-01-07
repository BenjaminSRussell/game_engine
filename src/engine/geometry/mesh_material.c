#include "mesh_material.h"
#include "mesh.h"
#include <core/logger.h>
#include <string.h>

// ----------------------------------------------------------------------------
// Material Validation
// ----------------------------------------------------------------------------

bool mesh_validate_material_slot(const mesh_t* mesh, u32 slot) {
    if (!mesh) {
        LOG_ERROR("mesh_validate_material_slot: NULL mesh");
        return false;
    }
    
    if (slot >= MESH_MAX_MATERIALS) {
        LOG_ERROR("Material slot %u out of range (max: %u)", slot, MESH_MAX_MATERIALS);
        return false;
    }
    
    return true;
}

// ----------------------------------------------------------------------------
// Default Material Handling
// ----------------------------------------------------------------------------

void mesh_set_default_material(mesh_t* mesh, u32 material_id) {
    if (!mesh) return;
    
    // Set material in all slots
    for (u32 i = 0; i < MESH_MAX_MATERIALS; ++i) {
        mesh->material_ids[i] = material_id;
    }
    mesh->material_count = 1;
    
    LOG_INFO("Set default material ID %u for mesh '%s'", material_id, mesh->name);
}

void mesh_clear_materials(mesh_t* mesh) {
    if (!mesh) return;
    
    memset(mesh->material_ids, 0, sizeof(u32) * MESH_MAX_MATERIALS);
    mesh->material_count = 0;
}

// ----------------------------------------------------------------------------
// Material Change Tracking
// ----------------------------------------------------------------------------

// Store in mesh extension data (simplified - would use proper system in production)
static mesh_material_change_t material_changes[256];
static u32 material_change_count = 0;

void mesh_track_material_change(mesh_t* mesh, u32 slot, u32 new_material, u64 current_frame) {
    if (!mesh || slot >= MESH_MAX_MATERIALS) return;
    
    u32 old_material = mesh->material_ids[slot];
    
    if (old_material == new_material) return; // No change
    
    // Record change
    if (material_change_count < 256) {
        material_changes[material_change_count].slot = slot;
        material_changes[material_change_count].old_material = old_material;
        material_changes[material_change_count].new_material = new_material;
        material_changes[material_change_count].frame_changed = current_frame;
        material_change_count++;
    }
    
    // Apply change
    mesh_set_material(mesh, slot, new_material);
    
    LOG_INFO("Material slot %u changed from ID %u to %u (frame %llu)",
        slot, old_material, new_material, current_frame);
}

// ----------------------------------------------------------------------------
// Material Parameter Helpers
// ----------------------------------------------------------------------------

// This is placeholder - in real system would integrate with material system
void mesh_apply_material_params(mesh_t* mesh, u32 slot, const mesh_material_params_t* params) {
    if (!mesh || !params || slot >= MESH_MAX_MATERIALS) return;
    
    // Set material ID
    mesh->material_ids[slot] = params->material_id;
    
    if (slot >= mesh->material_count) {
        mesh->material_count = slot + 1;
    }
    
    // In real implementation, this would pass params to material system
    LOG_INFO("Applied material params to mesh '%s' slot %u (metallic=%.2f, roughness=%.2f)",
        mesh->name, slot, params->metallic, params->roughness);
}
