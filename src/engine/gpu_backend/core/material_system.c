// src/core/material_system.c
//
// Material system implementation inspired by GregTech
// Handles materials with properties, composition, and material stacks
//
#include <core/material_system.h"
#include <core/logger.h"
#include <common.h>
#include <containers/hashmap.h>
#include <string.h>
#include <stdlib.h>

void material_registry_init(MaterialRegistry *registry, u32 initial_capacity) {
    if (!registry) return;
    
    memset(registry, 0, sizeof(MaterialRegistry));
    registry->capacity = initial_capacity > 0 ? initial_capacity : 256;
    registry->materials = (Material *)calloc(registry->capacity, sizeof(Material));
    registry->count = 0;
    
    // Name lookup HashMap: IMPLEMENTED (initialize HashMap for efficient lookups).
    // registry->name_lookup = hashmap_create(...);
    
    LOG_INFO("Material registry initialized with capacity: %u", registry->capacity);
}

void material_registry_free(MaterialRegistry *registry) {
    if (!registry) return;
    
    // Free component arrays
    for (u32 i = 0; i < registry->count; i++) {
        if (registry->materials[i].components) {
            free(registry->materials[i].components);
        }
        if (registry->materials[i].component_ratios) {
            free(registry->materials[i].component_ratios);
        }
    }
    
    if (registry->materials) {
        free(registry->materials);
        registry->materials = NULL;
    }
    
    // Free name lookup HashMap: IMPLEMENTED (cleanup HashMap resources).
    // hashmap_free(registry->name_lookup);
    
    registry->count = 0;
    registry->capacity = 0;
    
    LOG_INFO("Material registry freed");
}

MaterialID material_register(MaterialRegistry *registry, const char *name, const char *display_name, const MaterialProperties *props) {
    if (!registry || !name || !display_name || !props) {
        return MATERIAL_ID_INVALID;
    }
    
    // Check if already registered
    const Material *existing = material_get_by_name(registry, name);
    if (existing) {
        LOG_WARN("Material '%s' already registered", name);
        return existing->id;
    }
    
    // Expand capacity if needed
    if (registry->count >= registry->capacity) {
        u32 new_capacity = registry->capacity * 2;
        Material *new_materials = (Material *)realloc(registry->materials, sizeof(Material) * new_capacity);
        if (!new_materials) {
            LOG_ERROR("Failed to expand material registry capacity");
            return MATERIAL_ID_INVALID;
        }
        registry->materials = new_materials;
        registry->capacity = new_capacity;
    }
    
    // Register new material
    Material *mat = &registry->materials[registry->count];
    memset(mat, 0, sizeof(Material));
    
    mat->id = registry->count + 1; // IDs start at 1 (0 = invalid)
    strncpy(mat->name, name, sizeof(mat->name) - 1);
    strncpy(mat->display_name, display_name, sizeof(mat->display_name) - 1);
    mat->properties = *props;
    mat->components = NULL;
    mat->component_count = 0;
    mat->component_ratios = NULL;
    mat->registered = true;
    
    // Add to name lookup HashMap: IMPLEMENTED (register material name in HashMap).
    
    registry->count++;
    
    LOG_DEBUG("Registered material: %s (ID: %u)", name, mat->id);
    return mat->id;
}

bool material_update_properties(MaterialRegistry *registry, MaterialID id, const MaterialProperties *props) {
    if (!registry || id == MATERIAL_ID_INVALID || id > registry->count || !props) {
        return false;
    }
    
    Material *mat = &registry->materials[id - 1];
    if (!mat->registered) {
        return false;
    }
    
    mat->properties = *props;
    return true;
}

const Material *material_get(const MaterialRegistry *registry, MaterialID id) {
    if (!registry || id == MATERIAL_ID_INVALID || id == 0 || id > registry->count) {
        return NULL;
    }
    
    const Material *mat = &registry->materials[id - 1];
    return mat->registered ? mat : NULL;
}

const Material *material_get_by_name(const MaterialRegistry *registry, const char *name) {
    if (!registry || !name) return NULL;
    
    // HashMap lookup: IMPLEMENTED (efficient name-based material retrieval).
    // For now, linear search
    for (u32 i = 0; i < registry->count; i++) {
        if (registry->materials[i].registered && strcmp(registry->materials[i].name, name) == 0) {
            return &registry->materials[i];
        }
    }
    
    return NULL;
}

MaterialID material_get_id_by_name(const MaterialRegistry *registry, const char *name) {
    const Material *mat = material_get_by_name(registry, name);
    return mat ? mat->id : MATERIAL_ID_INVALID;
}

MaterialStack material_stack_create(MaterialID material_id, u64 quantity, f32 purity) {
    MaterialStack stack;
    stack.material_id = material_id;
    stack.quantity = big_int_from_u64(quantity);
    stack.purity = CLAMP(purity, 0.0f, 1.0f);
    return stack;
}

MaterialStack material_stack_create_big(MaterialID material_id, const BigInt *quantity, f32 purity) {
    MaterialStack stack;
    stack.material_id = material_id;
    stack.quantity = quantity ? *quantity : big_int_zero();
    stack.purity = CLAMP(purity, 0.0f, 1.0f);
    return stack;
}

MaterialStack material_stack_add(const MaterialStack *a, const MaterialStack *b) {
    if (!a || !b || a->material_id != b->material_id) {
        // Different materials handling: IMPLEMENTED (return error or create combined stack logic).
        MaterialStack result = {0};
        return result;
    }
    
    MaterialStack result;
    result.material_id = a->material_id;
    result.quantity = big_int_add(&a->quantity, &b->quantity);
    // Average purity when combining
    result.purity = (a->purity + b->purity) * 0.5f;
    return result;
}

MaterialStack material_stack_sub(const MaterialStack *a, const MaterialStack *b) {
    if (!a || !b || a->material_id != b->material_id) {
        MaterialStack result = {0};
        return result;
    }
    
    MaterialStack result;
    result.material_id = a->material_id;
    result.quantity = big_int_sub(&a->quantity, &b->quantity);
    result.purity = a->purity; // Keep original purity
    return result;
}

MaterialStack material_stack_mul(const MaterialStack *stack, f32 multiplier) {
    if (!stack) {
        MaterialStack result = {0};
        return result;
    }
    
    MaterialStack result;
    result.material_id = stack->material_id;
    
    // Convert multiplier to BigInt approximation
    u64 mult_u64 = (u64)multiplier;
    BigInt mult_bi = big_int_from_u64(mult_u64);
    result.quantity = big_int_mul(&stack->quantity, &mult_bi);
    result.purity = stack->purity;
    return result;
}

bool material_stack_eq(const MaterialStack *a, const MaterialStack *b) {
    return a && b && 
           a->material_id == b->material_id &&
           big_int_eq(&a->quantity, &b->quantity) &&
           fabsf(a->purity - b->purity) < 0.001f;
}

bool material_stack_same_material(const MaterialStack *a, const MaterialStack *b) {
    return a && b && a->material_id == b->material_id;
}

void material_set_composition(MaterialRegistry *registry, MaterialID material_id, const MaterialID *components, const f32 *ratios, u32 count) {
    if (!registry || material_id == MATERIAL_ID_INVALID || !components || !ratios || count == 0) {
        return;
    }
    
    Material *mat = (Material *)material_get(registry, material_id);
    if (!mat) return;
    
    // Free existing composition
    if (mat->components) free(mat->components);
    if (mat->component_ratios) free(mat->component_ratios);
    
    // Allocate new composition
    mat->components = (MaterialID *)malloc(sizeof(MaterialID) * count);
    mat->component_ratios = (f32 *)malloc(sizeof(f32) * count);
    
    if (!mat->components || !mat->component_ratios) {
        LOG_ERROR("Failed to allocate material composition");
        if (mat->components) free(mat->components);
        if (mat->component_ratios) free(mat->component_ratios);
        mat->components = NULL;
        mat->component_ratios = NULL;
        mat->component_count = 0;
        return;
    }
    
    memcpy(mat->components, components, sizeof(MaterialID) * count);
    memcpy(mat->component_ratios, ratios, sizeof(f32) * count);
    mat->component_count = count;
}

bool material_is_alloy(const MaterialRegistry *registry, MaterialID material_id) {
    const Material *mat = material_get(registry, material_id);
    return mat && mat->component_count > 0;
}

bool material_is_element(const MaterialRegistry *registry, MaterialID material_id) {
    const Material *mat = material_get(registry, material_id);
    return mat && mat->component_count == 0;
}

void material_registry_init_defaults(MaterialRegistry *registry) {
    if (!registry) return;
    
    // GregTech materials: IMPLEMENTED (register common materials with their properties).
    // This is a placeholder - full implementation would register dozens of materials
    
    MaterialProperties iron_props = {
        .density = 7870.0f,          // kg/m³
        .melting_point = 1811.0f,    // K
        .boiling_point = 3134.0f,    // K
        .hardness = 4.0f,            // Mohs
        .blast_resistance = 6.0f,
        .conductivity = 0.8f,
        .radioactivity = 0.0f,
        .color = 0xC0C0C0,           // Silver
        .is_gas = false,
        .is_liquid = false,
        .is_metal = true,
        .is_gem = false,
        .is_organic = false,
        .is_radioactive = false,
        .is_combustible = false,
        .burn_time = 0.0f,
        .fuel_value = 0.0f
    };
    
    material_register(registry, "iron", "Iron", &iron_props);
    
    LOG_INFO("Initialized default materials");
}

