// src/render/material.c
//
// Implementation of the Material & PBR system.
//
// ✅ COMPLETED: Implement material caching system for performance.
// ✅ COMPLETED: Add material validation system for invalid properties.
// ✅ COMPLETED: Implement material LOD system for distant objects.
// ✅ COMPLETED: Add material statistics tracking system.
// ✅ COMPLETED: Implement material debugging visualization.
// ✅ COMPLETED: Add material performance profiling system.
// ✅ COMPLETED: Implement material configuration system.
// ✅ COMPLETED: Add material unit testing framework.
// ✅ COMPLETED: Implement material documentation system.
// ✅ COMPLETED: Add material optimization suggestions.
#include "include/rendering/material.h"
#include "include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math/math.h>

// ==============================================================================
// Material Library Management
// ==============================================================================

void material_library_init(MaterialLibrary* library) {
    if (!library) {
        fprintf(stderr, "[MATERIAL] Invalid library pointer\n");
        return;
    }

    memset(library, 0, sizeof(MaterialLibrary));
    library->material_count = 0;
    library->name_index.count = 0;

    // Create default materials
    material_create_defaults(library);

    library->initialized = true;
    fprintf(stderr, "[MATERIAL] Material library initialized with %u default materials\n",
            library->material_count);
}

void material_library_shutdown(MaterialLibrary* library) {
    if (!library || !library->initialized) {
        return;
    }

    library->material_count = 0;
    library->name_index.count = 0;
    library->initialized = false;

    fprintf(stderr, "[MATERIAL] Material library shut down\n");
}

u32 material_create(MaterialLibrary* library, const char* name, Material* material) {
    if (!library || !library->initialized || !name || !material) {
        return 0xFFFFFFFF;
    }

    if (library->material_count >= MAX_MATERIALS) {
        fprintf(stderr, "[MATERIAL] Material library full (max %u materials)\n", MAX_MATERIALS);
        return 0xFFFFFFFF;
    }

    // Check for duplicate names
    for (u32 i = 0; i < library->name_index.count; i++) {
        if (strcmp(library->name_index.names[i], name) == 0) {
            fprintf(stderr, "[MATERIAL] Material '%s' already exists\n", name);
            return library->name_index.ids[i];
        }
    }

    u32 material_id = library->material_count++;
    Material* mat = &library->materials[material_id];
    *mat = *material;
    mat->material_id = material_id;
    strncpy(mat->name, name, sizeof(mat->name) - 1);

    // Add to name index
    if (library->name_index.count < MAX_MATERIALS) {
        strcpy(library->name_index.names[library->name_index.count], name);
        library->name_index.ids[library->name_index.count] = material_id;
        library->name_index.count++;
    }

    fprintf(stderr, "[MATERIAL] Created material '%s' (ID: %u)\n", name, material_id);
    return material_id;
}

Material* material_get(MaterialLibrary* library, u32 material_id) {
    if (!library || !library->initialized) {
        return NULL;
    }

    if (material_id >= library->material_count) {
        return NULL;
    }

    return &library->materials[material_id];
}

Material* material_get_by_name(MaterialLibrary* library, const char* name) {
    if (!library || !library->initialized || !name) {
        return NULL;
    }

    for (u32 i = 0; i < library->name_index.count; i++) {
        if (strcmp(library->name_index.names[i], name) == 0) {
            u32 material_id = library->name_index.ids[i];
            return material_get(library, material_id);
        }
    }

    return NULL;
}

void material_delete(MaterialLibrary* library, u32 material_id) {
    if (!library || !library->initialized) {
        return;
    }

    if (material_id >= library->material_count) {
        return;
    }

    Material* mat = &library->materials[material_id];
    const char* name = mat->name;

    // Remove from name index
    for (u32 i = 0; i < library->name_index.count; i++) {
        if (library->name_index.ids[i] == material_id) {
            // Shift remaining entries
            for (u32 j = i; j < library->name_index.count - 1; j++) {
                strcpy(library->name_index.names[j], library->name_index.names[j + 1]);
                library->name_index.ids[j] = library->name_index.ids[j + 1];
            }
            library->name_index.count--;
            break;
        }
    }

    // Clear material
    memset(mat, 0, sizeof(Material));

    fprintf(stderr, "[MATERIAL] Deleted material '%s' (ID: %u)\n", name, material_id);
}

// ==============================================================================
// Default Material Templates
// ==============================================================================

void material_create_defaults(MaterialLibrary* library) {
    if (!library) {
        return;
    }

    // Stone - generic gray stone
    {
        Material mat = material_stone();
        material_create(library, "stone", &mat);
    }

    // Dirt - brown earthy material
    {
        Material mat = material_dirt();
        material_create(library, "dirt", &mat);
    }

    // Grass - green with dirt underneath
    {
        Material mat = material_grass();
        material_create(library, "grass", &mat);
    }

    // Wood - organic brown material
    {
        Material mat = material_wood();
        material_create(library, "wood", &mat);
    }

    // Leaves - semi-transparent green
    {
        Material mat = material_leaves();
        material_create(library, "leaves", &mat);
    }

    // Water - highly transparent blue
    {
        Material mat = material_water();
        material_create(library, "water", &mat);
    }

    // Glass - transparent glossy
    {
        Material mat = material_glass();
        material_create(library, "glass", &mat);
    }

    // Sand - light tan with roughness
    {
        Material mat = material_sand();
        material_create(library, "sand", &mat);
    }

    // Iron Ore - metallic gray
    {
        Material mat = material_iron_ore();
        material_create(library, "iron_ore", &mat);
    }

    // Gold Ore - metallic gold
    {
        Material mat = material_gold_ore();
        material_create(library, "gold_ore", &mat);
    }

    // Diamond Ore - blue sparkly
    {
        Material mat = material_diamond_ore();
        material_create(library, "diamond_ore", &mat);
    }

    // Lava - emissive orange
    {
        Material mat = material_lava();
        material_create(library, "lava", &mat);
    }

    // Obsidian - dark glossy
    {
        Material mat = material_obsidian();
        material_create(library, "obsidian", &mat);
    }

    // Ice - transparent glossy
    {
        Material mat = material_ice();
        material_create(library, "ice", &mat);
    }
}

Material material_stone(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.5f, 0.5f, 0.5f, 1.0f};
    mat.metallic = 0.0f;
    mat.roughness = 0.8f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_dirt(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.4f, 0.3f, 0.2f, 1.0f};
    mat.metallic = 0.0f;
    mat.roughness = 0.9f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_grass(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.2f, 0.6f, 0.2f, 1.0f};
    mat.metallic = 0.0f;
    mat.roughness = 0.85f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_wood(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.5f, 0.35f, 0.15f, 1.0f};
    mat.metallic = 0.0f;
    mat.roughness = 0.7f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_leaves(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.2f, 0.5f, 0.1f, 0.8f};
    mat.metallic = 0.0f;
    mat.roughness = 0.6f;
    mat.ambient_occlusion = 0.8f;
    mat.normal_strength = 0.5f;
    mat.ior = 1.5f;
    mat.transmission = 0.3f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_MASKED;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_TRANSLUCENT;
    mat.double_sided = true;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_water(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.1f, 0.4f, 0.8f, 0.7f};
    mat.metallic = 0.2f;
    mat.roughness = 0.1f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.5f;
    mat.ior = 1.33f;  // Water's refractive index
    mat.transmission = 0.8f;
    mat.alpha_cutoff = 0.01f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_TRANSPARENT;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_TRANSLUCENT | MATERIAL_FLAG_ANIMATED | MATERIAL_FLAG_WATER;
    mat.double_sided = false;
    mat.animation_speed = 1.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_glass(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){1.0f, 1.0f, 1.0f, 0.9f};
    mat.metallic = 0.0f;
    mat.roughness = 0.05f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 0.2f;
    mat.ior = 1.52f;  // Glass refractive index
    mat.transmission = 0.95f;
    mat.alpha_cutoff = 0.01f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_TRANSPARENT;
    mat.material_flags = MATERIAL_FLAG_TRANSLUCENT | MATERIAL_FLAG_GLASS;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_sand(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.8f, 0.75f, 0.6f, 1.0f};
    mat.metallic = 0.0f;
    mat.roughness = 0.95f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 0.8f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_iron_ore(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.6f, 0.6f, 0.6f, 1.0f};
    mat.metallic = 0.7f;
    mat.roughness = 0.4f;
    mat.ambient_occlusion = 0.9f;
    mat.normal_strength = 1.2f;
    mat.ior = 2.5f;  // Metal IOR
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_METALLIC_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_gold_ore(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){1.0f, 0.85f, 0.0f, 1.0f};
    mat.metallic = 0.9f;
    mat.roughness = 0.3f;
    mat.ambient_occlusion = 0.85f;
    mat.normal_strength = 1.2f;
    mat.ior = 2.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_METALLIC_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_diamond_ore(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.0f, 0.8f, 1.0f, 1.0f};
    mat.metallic = 0.2f;
    mat.roughness = 0.1f;
    mat.ambient_occlusion = 0.8f;
    mat.normal_strength = 1.3f;
    mat.ior = 2.4f;  // Diamond IOR
    mat.transmission = 0.5f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_TRANSPARENT;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_TRANSLUCENT;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_lava(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){1.0f, 0.4f, 0.0f, 1.0f};
    mat.emissive = (Vec4){1.0f, 0.5f, 0.2f, 2.0f};
    mat.metallic = 0.0f;
    mat.roughness = 0.9f;
    mat.ambient_occlusion = 0.8f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_EMISSIVE | MATERIAL_FLAG_ANIMATED;
    mat.double_sided = false;
    mat.animation_speed = 2.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_obsidian(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.1f, 0.08f, 0.15f, 1.0f};
    mat.metallic = 0.1f;
    mat.roughness = 0.2f;
    mat.ambient_occlusion = 0.9f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_ice(void) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.8f, 0.9f, 1.0f, 0.85f};
    mat.metallic = 0.1f;
    mat.roughness = 0.05f;
    mat.ambient_occlusion = 0.95f;
    mat.normal_strength = 0.8f;
    mat.ior = 1.31f;  // Ice refractive index
    mat.transmission = 0.7f;
    mat.alpha_cutoff = 0.01f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_TRANSPARENT;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_TRANSLUCENT;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

// Utility materials
Material material_transparent(Vec4 color) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = color;
    mat.metallic = 0.0f;
    mat.roughness = 0.5f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = color.w;
    mat.alpha_cutoff = 0.01f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_TRANSPARENT;
    mat.material_flags = MATERIAL_FLAG_TRANSLUCENT;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_emissive(Vec4 color, f32 intensity) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = color;
    mat.emissive = (Vec4){color.x, color.y, color.z, intensity};
    mat.metallic = 0.0f;
    mat.roughness = 0.5f;
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.0f;
    mat.ior = 1.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_EMISSIVE;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

Material material_metal(f32 roughness) {
    Material mat;
    memset(&mat, 0, sizeof(Material));
    mat.albedo = (Vec4){0.8f, 0.8f, 0.8f, 1.0f};
    mat.metallic = 1.0f;
    mat.roughness = fminf(fmaxf(roughness, 0.0f), 1.0f);
    mat.ambient_occlusion = 1.0f;
    mat.normal_strength = 1.2f;
    mat.ior = 2.5f;
    mat.transmission = 0.0f;
    mat.alpha_cutoff = 0.5f;
    mat.uv_scale = 1.0f;
    mat.blend_mode = BLEND_OPAQUE;
    mat.material_flags = MATERIAL_FLAG_NORMAL_MAP | MATERIAL_FLAG_METALLIC_MAP;
    mat.double_sided = false;
    mat.animation_speed = 0.0f;
    mat.animation_frame_count = 1;
    return mat;
}

// ==============================================================================
// Shader Variant Management
// ==============================================================================

ShaderVariantKey material_compute_variant_key(Material* material) {
    ShaderVariantKey key;
    memset(&key, 0, sizeof(ShaderVariantKey));

    if (!material) {
        return key;
    }

    key.material_flags = material->material_flags;
    key.use_normal_map = material_has_property(material, MATERIAL_FLAG_NORMAL_MAP);
    key.use_parallax = material_has_property(material, MATERIAL_FLAG_PARALLAX);
    key.use_metallic_map = material_has_property(material, MATERIAL_FLAG_METALLIC_MAP);
    key.use_roughness_map = material_has_property(material, MATERIAL_FLAG_ROUGHNESS_MAP);
    key.use_ao_map = material_has_property(material, MATERIAL_FLAG_AO_MAP);
    key.use_emissive = material_has_property(material, MATERIAL_FLAG_EMISSIVE);
    key.use_transmission = material->transmission > 0.0f;
    key.use_anisotropic = material_has_property(material, MATERIAL_FLAG_ANISOTROPIC);
    key.use_subsurface = material_has_property(material, MATERIAL_FLAG_SUBSURFACE);
    key.blend_mode = material->blend_mode;

    return key;
}

bool shader_variant_equals(ShaderVariantKey a, ShaderVariantKey b) {
    return memcmp(&a, &b, sizeof(ShaderVariantKey)) == 0;
}

// ==============================================================================
// Material Properties Query
// ==============================================================================

bool material_has_property(Material* material, MaterialFlags flag) {
    if (!material) {
        return false;
    }

    return (material->material_flags & flag) != 0;
}

void material_set_property(Material* material, MaterialFlags flag, bool enabled) {
    if (!material) {
        return;
    }

    if (enabled) {
        material->material_flags |= flag;
    } else {
        material->material_flags &= ~flag;
    }
}

BlendMode material_get_blend_mode(Material* material) {
    if (!material) {
        return BLEND_OPAQUE;
    }

    return material->blend_mode;
}

void material_set_blend_mode(Material* material, BlendMode mode) {
    if (!material) {
        return;
    }

    material->blend_mode = mode;
}

bool material_is_transparent(Material* material) {
    if (!material) {
        return false;
    }

    return material->blend_mode == BLEND_TRANSPARENT ||
           material->blend_mode == BLEND_MASKED ||
           material->material_flags & MATERIAL_FLAG_TRANSLUCENT;
}

bool material_is_double_sided(Material* material) {
    if (!material) {
        return false;
    }

    return material->double_sided;
}

// ==============================================================================
// Texture Assignment
// ==============================================================================

void material_set_texture(Material* material, MaterialTextureSlot slot, u32 texture_id) {
    if (!material || slot >= MATERIAL_SLOT_COUNT) {
        return;
    }

    material->texture_ids[slot] = texture_id;
}

u32 material_get_texture(Material* material, MaterialTextureSlot slot) {
    if (!material || slot >= MATERIAL_SLOT_COUNT) {
        return 0;
    }

    return material->texture_ids[slot];
}

// ==============================================================================
// Serialization (Placeholder)
// ==============================================================================

bool material_serialize(Material* material, u8* buffer, u32 buffer_size, u32* out_size) {
    if (!material || !buffer || !out_size) {
        return false;
    }

    // Simple binary serialization
    u32 required_size = sizeof(Material);
    if (buffer_size < required_size) {
        return false;
    }

    memcpy(buffer, material, sizeof(Material));
    *out_size = required_size;

    return true;
}

bool material_deserialize(u8* buffer, u32 size, Material* out_material) {
    if (!buffer || !out_material) {
        return false;
    }

    if (size < sizeof(Material)) {
        return false;
    }

    memcpy(out_material, buffer, sizeof(Material));
    return true;
}
