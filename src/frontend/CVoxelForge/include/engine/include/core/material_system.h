// include/core/material_system.h
//
// Purpose: Comprehensive material system inspired by GregTech
// Handles materials, their properties, and material stacks (quantities of materials)
// This is foundational for tech mods and complex resource processing
//
#ifndef MATERIAL_SYSTEM_H
#define MATERIAL_SYSTEM_H

#include "../common.h"
#include "big_int.h"

// Material ID - unique identifier for each material
typedef u32 MaterialID;

// Material properties (GregTech-inspired)
typedef struct {
    f32 density;           // kg/m³
    f32 melting_point;     // Kelvin
    f32 boiling_point;     // Kelvin
    f32 hardness;          // Mohs scale (0-10)
    f32 blast_resistance;  // Resistance to explosions
    f32 conductivity;      // Electrical/thermal conductivity
    f32 radioactivity;     // Radioactivity level (0 = none, 1.0 = highly radioactive)
    u32 color;             // RGB color for rendering
    bool is_gas;           // Gas at room temperature
    bool is_liquid;        // Liquid at room temperature
    bool is_metal;         // Is a metal
    bool is_gem;           // Is a gemstone
    bool is_organic;       // Organic material
    bool is_radioactive;   // Radioactive material
    bool is_combustible;   // Can burn
    f32 burn_time;         // Burn time in seconds (for fuel)
    f32 fuel_value;        // Energy value (EU or RF)
} MaterialProperties;

// Material definition
typedef struct {
    MaterialID id;
    char name[64];                // Internal name (e.g., "iron")
    char display_name[64];        // Display name (e.g., "Iron")
    MaterialProperties properties;
    u32 tier;                     // Material tier (0-16, like GregTech)
    MaterialID *components;       // Chemical composition (for alloys)
    u32 component_count;          // Number of components
    f32 *component_ratios;        // Ratio of each component
    bool registered;
} Material;

// Material stack - quantity of a material (supports big integers)
typedef struct {
    MaterialID material_id;
    BigInt quantity;              // Quantity (can be very large)
    f32 purity;                   // Purity (0.0-1.0) for impure materials
} MaterialStack;

// Material registry
typedef struct {
    Material *materials;
    u32 count;
    u32 capacity;
    void *name_lookup;            // HashMap for name-based lookup
} MaterialRegistry;

// Initialize material system
void material_registry_init(MaterialRegistry *registry, u32 initial_capacity);
void material_registry_free(MaterialRegistry *registry);

// Register materials
MaterialID material_register(MaterialRegistry *registry, const char *name, const char *display_name, const MaterialProperties *props);
bool material_update_properties(MaterialRegistry *registry, MaterialID id, const MaterialProperties *props);

// Query materials
const Material *material_get(const MaterialRegistry *registry, MaterialID id);
const Material *material_get_by_name(const MaterialRegistry *registry, const char *name);
MaterialID material_get_id_by_name(const MaterialRegistry *registry, const char *name);

// Material stack operations
MaterialStack material_stack_create(MaterialID material_id, u64 quantity, f32 purity);
MaterialStack material_stack_create_big(MaterialID material_id, const BigInt *quantity, f32 purity);
MaterialStack material_stack_add(const MaterialStack *a, const MaterialStack *b);
MaterialStack material_stack_sub(const MaterialStack *a, const MaterialStack *b);
MaterialStack material_stack_mul(const MaterialStack *stack, f32 multiplier);
bool material_stack_eq(const MaterialStack *a, const MaterialStack *b);
bool material_stack_same_material(const MaterialStack *a, const MaterialStack *b);

// Material composition (for alloys and compounds)
void material_set_composition(MaterialRegistry *registry, MaterialID material_id, const MaterialID *components, const f32 *ratios, u32 count);
bool material_is_alloy(const MaterialRegistry *registry, MaterialID material_id);
bool material_is_element(const MaterialRegistry *registry, MaterialID material_id);

// Default materials (GregTech-inspired common materials)
void material_registry_init_defaults(MaterialRegistry *registry);

// Material constants
#define MATERIAL_ID_INVALID 0
#define MATERIAL_ID_IRON 1
#define MATERIAL_ID_COPPER 2
#define MATERIAL_ID_TIN 3
#define MATERIAL_ID_LEAD 4
#define MATERIAL_ID_GOLD 5
#define MATERIAL_ID_SILVER 6
#define MATERIAL_ID_NICKEL 7
#define MATERIAL_ID_ZINC 8
#define MATERIAL_ID_ALUMINUM 9
#define MATERIAL_ID_BRONZE 10
#define MATERIAL_ID_STEEL 11
#define MATERIAL_ID_BRASS 12
#define MATERIAL_ID_INVAR 13
#define MATERIAL_ID_ELECTRUM 14
#define MATERIAL_ID_CONSTANTAN 15

#endif // MATERIAL_SYSTEM_H

