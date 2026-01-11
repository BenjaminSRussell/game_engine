#include "geometry/mesh_material.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_MATERIAL_CHANGES 64

// Internal material tracking data (would be stored in mesh in a real implementation)
typedef struct mesh_material_data_t {
    mesh_material_change_t changes[MAX_MATERIAL_CHANGES];
    u32 change_count;
    u64 last_material_frame;
} mesh_material_data_t;

bool mesh_validate_material_slot(const mesh_t* mesh, u32 slot) {
    if (!mesh) {
        printf("Error: Invalid mesh for material slot validation\n");
        return false;
    }
    
    if (slot >= MESH_MAX_MATERIALS) {
        printf("Error: Material slot %u exceeds maximum %u\n", slot, MESH_MAX_MATERIALS);
        return false;
    }
    
    if (slot >= mesh->material_count) {
        printf("Warning: Material slot %u not assigned (mesh has %u materials)\n", 
               slot, mesh->material_count);
        return false;
    }
    
    return true;
}

void mesh_set_default_material(mesh_t* mesh, u32 material_id) {
    if (!mesh) {
        printf("Error: Invalid mesh for default material assignment\n");
        return;
    }
    
    // Set material in slot 0 (default slot)
    mesh_set_material(mesh, 0, material_id);
    
    printf("Set default material %u for mesh '%s'\n", material_id, mesh->name);
}

void mesh_clear_materials(mesh_t* mesh) {
    if (!mesh) {
        return;
    }
    
    // Clear all material slots
    memset(mesh->material_ids, 0, sizeof(mesh->material_ids));
    mesh->material_count = 0;
    
    printf("Cleared all materials from mesh '%s'\n", mesh->name);
}

void mesh_track_material_change(mesh_t* mesh, u32 slot, u32 new_material, u64 current_frame) {
    if (!mesh || !mesh_validate_material_slot(mesh, slot)) {
        return;
    }
    
    u32 old_material = mesh->material_ids[slot];
    
    // Skip if no actual change
    if (old_material == new_material) {
        return;
    }
    
    // TODO: Store change tracking data in mesh
    // For now, just print the change
    printf("Material change tracked for mesh '%s': slot %u, %u -> %u (frame %llu)\n", 
           mesh->name, slot, old_material, new_material, current_frame);
    
    // Update the material
    mesh_set_material(mesh, slot, new_material);
}

void mesh_apply_material_params(mesh_t* mesh, u32 slot, const mesh_material_params_t* params) {
    if (!mesh || !params || !mesh_validate_material_slot(mesh, slot)) {
        return;
    }
    
    // TODO: This would interface with the material system to apply parameters
    // For now, we just validate and print what would happen
    
    printf("Applying material params to mesh '%s', slot %u, material %u:\n", 
           mesh->name, slot, mesh->material_ids[slot]);
    printf("  Metallic: %.3f\n", params->metallic);
    printf("  Roughness: %.3f\n", params->roughness);
    printf("  AO: %.3f\n", params->ao);
    printf("  Emissive: %.3f\n", params->emissive_strength);
    
    // Validate parameter ranges
    if (params->metallic < 0.0f || params->metallic > 1.0f) {
        printf("Warning: Metallic value %.3f out of range [0,1]\n", params->metallic);
    }
    
    if (params->roughness < 0.0f || params->roughness > 1.0f) {
        printf("Warning: Roughness value %.3f out of range [0,1]\n", params->roughness);
    }
    
    if (params->ao < 0.0f || params->ao > 1.0f) {
        printf("Warning: AO value %.3f out of range [0,1]\n", params->ao);
    }
    
    if (params->emissive_strength < 0.0f) {
        printf("Warning: Emissive strength %.3f should be non-negative\n", params->emissive_strength);
    }
}
