#pragma once

#include <core/types.h>
#include <math/vec3.h>
#include <math/mat4.h>
#include <math/quat.h>

// Forward declarations
struct GBuffer;
struct Camera;

typedef enum {
    DECAL_BLEND_MODE_DEFERRED,   // Blend into G-Buffer (Normal, Albedo, Material)
    DECAL_BLEND_MODE_EMISSIVE,   // Add to Emissive buffer
    DECAL_BLEND_MODE_STAIN,      // Multiply Albedo only (e.g. dirt, scorch)
    DECAL_BLEND_MODE_NORMAL_ONLY // Only modify normals
} DecalBlendMode;

typedef struct {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
    
    // Texture IDs (Resource Handles)
    u32 albedo_texture_id;
    u32 normal_texture_id;
    u32 material_texture_id; // Metallic(R), Roughness(G), AO(B)
    
    // Appearance
    Vec4 color_tint; // RGBA
    DecalBlendMode blend_mode;
    
    // Lifetime
    f32 lifetime;       // Remaining time in seconds
    f32 start_lifetime; // Initial lifetime for fade calculations
    f32 fade_duration;  // Seconds to fade out before death
    
    b8 active;
} Decal;

typedef struct DecalSystem DecalSystem;

// -- API --

/**
 * Creates and initializes the Decal System.
 */
DecalSystem* decal_system_create(void);

/**
 * Shuts down the Decal System and frees resources.
 */
void decal_system_destroy(DecalSystem* system);

/**
 * Updates decal lifetimes and manages internal state.
 * @param dt Delta time in seconds.
 */
void decal_system_update(DecalSystem* system, f32 dt);

/**
 * Renders active decals into the G-Buffer.
 * Should be called after the main G-Buffer geometry pass.
 */
void decal_system_render(DecalSystem* system, struct GBuffer* gbuffer, struct Camera* camera);

/**
 * Renders debug visualization (wireframe bounds).
 */
void decal_system_render_debug(DecalSystem* system, struct Camera* camera);

/**
 * Adds a new decal to the world.
 * @return Handle/ID of the decal, or 0 if failed.
 */
u32 decal_system_add_decal(DecalSystem* system, Decal decal);

/**
 * Removes a decal by ID.
 */
void decal_system_remove_decal(DecalSystem* system, u32 decal_id);

/**
 * Clears all active decals.
 */
void decal_system_clear(DecalSystem* system);

/**
 * Sets the G-Buffer geometry pass depth texture for reconstruction.
 */
void decal_system_set_depth_texture(DecalSystem* system, u32 depth_texture_id);
