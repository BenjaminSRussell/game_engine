// include/render/material.h
//
// Purpose: Material system for Physically Based Rendering (PBR).
// Defines material properties (albedo, metallic, roughness, normal maps, etc.)
// and provides a material library for efficient lookup and shader variant
// management.
//
#ifndef MATERIAL_H
#define MATERIAL_H

#include <common.h>
#include <math/vec3.h>
#include <math/vec4.h>

// Material flags for effect control
typedef enum {
  MATERIAL_FLAG_NORMAL_MAP = 1 << 0,    // Has normal map texture
  MATERIAL_FLAG_PARALLAX = 1 << 1,      // Has parallax/height map
  MATERIAL_FLAG_METALLIC_MAP = 1 << 2,  // Metallic from texture
  MATERIAL_FLAG_ROUGHNESS_MAP = 1 << 3, // Roughness from texture
  MATERIAL_FLAG_AO_MAP = 1 << 4,        // Ambient occlusion map
  MATERIAL_FLAG_EMISSIVE = 1 << 5,      // Emissive material
  MATERIAL_FLAG_TRANSLUCENT = 1 << 6,   // Transparent/translucent
  MATERIAL_FLAG_ANIMATED = 1 << 7,      // Animated texture
  MATERIAL_FLAG_ANISOTROPIC = 1 << 8,   // Anisotropic material (brushed metal)
  MATERIAL_FLAG_SUBSURFACE = 1 << 9,    // Subsurface scattering
  MATERIAL_FLAG_GLASS = 1 << 10,        // Glass/mirror material
  MATERIAL_FLAG_WATER = 1 << 11,        // Water with special properties
} MaterialFlags;

// Material blend modes
typedef enum {
  BLEND_OPAQUE,      // Standard opaque rendering
  BLEND_MASKED,      // Alpha test (binary transparency)
  BLEND_TRANSPARENT, // Alpha blend
  BLEND_ADDITIVE,    // Additive blending (glow effects)
  BLEND_MULTIPLY,    // Multiplicative blending
  BLEND_SCREEN,      // Screen blending
} BlendMode;

// Texture slots in material
typedef enum {
  MATERIAL_SLOT_ALBEDO = 0,
  MATERIAL_SLOT_NORMAL = 1,
  MATERIAL_SLOT_METALLIC = 2,
  MATERIAL_SLOT_ROUGHNESS = 3,
  MATERIAL_SLOT_AO = 4,
  MATERIAL_SLOT_EMISSIVE = 5,
  MATERIAL_SLOT_HEIGHT = 6,
  MATERIAL_SLOT_COUNT = 8
} MaterialTextureSlot;

// Per-material properties
typedef struct Material {
  // Basic identification
  u32 material_id;
  char name[64];

  // Color properties
  Vec4 albedo;   // Base color (RGB + alpha)
  Vec4 emissive; // Emissive color (RGB) + intensity

  // PBR properties
  f32 metallic;          // 0.0 = dielectric, 1.0 = metal (0.0-1.0)
  f32 roughness;         // 0.0 = mirror, 1.0 = rough (0.0-1.0)
  f32 ambient_occlusion; // 0.0 = fully occluded, 1.0 = fully exposed

  // Surface properties
  f32 normal_strength;   // Normal map intensity (0.0-2.0)
  f32 parallax_height;   // Parallax/height map scale (0.0-0.1)
  f32 subsurface_amount; // Subsurface scattering intensity
  f32 anisotropy;        // Anisotropy amount for brushed materials

  // Transparency & refraction
  f32 ior;          // Index of refraction (1.0-2.5)
  f32 transmission; // Light transmission through material (0.0-1.0)
  f32 alpha_cutoff; // Alpha threshold for masked blending (0.0-1.0)

  // Texture IDs (from texture atlas or loader)
  u32 texture_ids[MATERIAL_SLOT_COUNT];

  // UV properties
  f32 uv_scale;                 // UV coordinate scaling
  f32 uv_offset_x, uv_offset_y; // UV offset

  // Blending and rendering
  BlendMode blend_mode;
  u32 material_flags; // Bitflags from MaterialFlags enum
  bool double_sided;  // Render both sides of faces

  // Animation (if MATERIAL_FLAG_ANIMATED set)
  f32 animation_speed;       // Frames per second
  u32 animation_frame_count; // Number of animation frames

} Material;

// Material library
#define MAX_MATERIALS 512

typedef struct {
  Material materials[MAX_MATERIALS];
  u32 material_count;
  bool initialized;

  // Material name to ID lookup
  struct {
    char names[MAX_MATERIALS][64];
    u32 ids[MAX_MATERIALS];
    u32 count;
  } name_index;
} MaterialLibrary;

// ==============================================================================
// Material Management Functions
// ==============================================================================

// Initialize material library
void material_library_init(MaterialLibrary *library);

// Shutdown material library
void material_library_shutdown(MaterialLibrary *library);

// Create or update a material
u32 material_create(MaterialLibrary *library, const char *name,
                    Material *material);

// Get material by ID
Material *material_get(MaterialLibrary *library, u32 material_id);

// Get material by name
Material *material_get_by_name(MaterialLibrary *library, const char *name);

// Delete material
void material_delete(MaterialLibrary *library, u32 material_id);

// Create default materials (stone, wood, glass, dirt, water, etc.)
void material_create_defaults(MaterialLibrary *library);

// ==============================================================================
// Shader Variant Management
// ==============================================================================

// Shader variant key based on material flags and properties
typedef struct {
  u32 material_flags; // Which effects are enabled
  bool use_normal_map;
  bool use_parallax;
  bool use_metallic_map;
  bool use_roughness_map;
  bool use_ao_map;
  bool use_emissive;
  bool use_transmission; // For glass/transparent
  bool use_anisotropic;
  bool use_subsurface;
  BlendMode blend_mode;
} ShaderVariantKey;

// Compute shader variant key from material properties
ShaderVariantKey material_compute_variant_key(Material *material);

// Variant comparison for caching
bool shader_variant_equals(ShaderVariantKey a, ShaderVariantKey b);

// ==============================================================================
// Pre-configured Material Templates
// ==============================================================================

// Create standard Minecraft block materials
Material material_stone(void);
Material material_dirt(void);
Material material_grass(void);
Material material_wood(void);
Material material_leaves(void);
Material material_water(void);
Material material_glass(void);
Material material_sand(void);
Material material_iron_ore(void);
Material material_gold_ore(void);
Material material_diamond_ore(void);
Material material_lava(void);
Material material_obsidian(void);
Material material_ice(void);

// Create special materials for effects
Material material_transparent(Vec4 color);
Material material_emissive(Vec4 color, f32 intensity);
Material material_metal(f32 roughness);

// ==============================================================================
// Texture Assignment
// ==============================================================================

// Assign texture to material slot
void material_set_texture(Material *material, MaterialTextureSlot slot,
                          u32 texture_id);

// Get texture from material slot
u32 material_get_texture(Material *material, MaterialTextureSlot slot);

// ==============================================================================
// Material Properties Query
// ==============================================================================

// Check if material has a specific property enabled
bool material_has_property(Material *material, MaterialFlags flag);

// Enable/disable material properties
void material_set_property(Material *material, MaterialFlags flag,
                           bool enabled);

// Get material's blend mode
BlendMode material_get_blend_mode(Material *material);

// Set material's blend mode
void material_set_blend_mode(Material *material, BlendMode mode);

// Check if material is transparent
bool material_is_transparent(Material *material);

// Check if material is double-sided
bool material_is_double_sided(Material *material);

// ==============================================================================
// Serialization
// ==============================================================================

// Export material to bytes for storage
bool material_serialize(Material *material, u8 *buffer, u32 buffer_size,
                        u32 *out_size);

// Bind material for rendering (OpenGL/Backend specific)
void material_bind(const Material *material);

// Import material from bytes
bool material_deserialize(u8 *buffer, u32 size, Material *out_material);

#endif // MATERIAL_H
