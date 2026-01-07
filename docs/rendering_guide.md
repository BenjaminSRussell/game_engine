# Rendering System User Guide

## Overview

The rendering system provides photorealistic rendering with:
- **PBR Materials**: Physically-based materials
- **Dynamic Lighting**: Point, spot, directional, and area lights
- **Shadows**: High-quality shadow maps with CSM
- **Post-Processing**: Color grading, bloom, SSAO, DoF, tone mapping
- **Environment**: Skybox, IBL, time of day, weather

---

## PBR Materials

### Creating Materials

```c
#include "renderer/pbr/pbr_brdf.h"

// Create a material
PBRMaterial mat = pbr_material_default();

// Set properties
mat.albedo[0] = 0.8f;  // R
mat.albedo[1] = 0.2f;  // G
mat.albedo[2] = 0.2f;  // B
mat.metallic = 0.0f;   // 0 = dielectric, 1 = metal
mat.roughness = 0.4f;  // 0 = smooth, 1 = rough
```

### Material Presets

```c
// Metallic materials
PBRMaterial metal = pbr_material_metal(0.2f); // roughness

// Plastic
PBRMaterial plastic = pbr_material_plastic(1.0f, 0.0f, 0.0f, 0.5f);

// Skin with subsurface scattering
PBRMaterial skin = pbr_material_skin();

// Car paint with clearcoat
PBRMaterial paint = pbr_material_car_paint(1.0f, 0.0f, 0.0f);
```

### Advanced Material Features

**Clearcoat** (glossy layer on top):
```c
mat.clearcoat = 1.0f;
mat.clearcoat_roughness = 0.05f;
```

**Subsurface Scattering** (for skin, wax, etc.):
```c
mat.subsurface = 0.5f;
mat.subsurface_color[0] = 1.0f;
mat.subsurface_color[1] = 0.3f;
mat.subsurface_color[2] = 0.2f;
```

**Anisotropic Reflections** (brushed metal):
```c
mat.anisotropy = 0.8f;
```

---

## Lighting System

### Creating Lights

```c
#include "renderer/lighting/light_system.h"

// Create light system
LightSystem* lights = light_system_create();

// Add a point light
uint32_t point_id = light_system_add_light(lights, LIGHT_TYPE_POINT);
Light* point = light_system_get_light(lights, point_id);

light_set_position(point, 0.0f, 5.0f, 0.0f);
light_set_color(point, 1.0f, 1.0f, 1.0f);
light_set_intensity(point, 10.0f);
light_set_range(point, 20.0f);
```

### Light Types

**Point Light** (omnidirectional):
```c
uint32_t id = light_system_add_light(lights, LIGHT_TYPE_POINT);
Light* light = light_system_get_light(lights, id);
light_set_position(light, x, y, z);
light_set_range(light, 15.0f);
```

**Spot Light** (cone):
```c
uint32_t id = light_system_add_light(lights, LIGHT_TYPE_SPOT);
Light* light = light_system_get_light(lights, id);
light_set_position(light, x, y, z);
light_set_direction(light, 0.0f, -1.0f, 0.0f);
light_set_spot_angles(light, 0.4f, 0.6f); // inner, outer (radians)
```

**Directional Light** (sun/moon):
```c
uint32_t id = light_system_add_light(lights, LIGHT_TYPE_DIRECTIONAL);
Light* light = light_system_get_light(lights, id);
light_set_direction(light, 0.3f, -0.7f, 0.2f);
light_set_intensity(light, 5.0f);
```

**Area Light** (soft lighting):
```c
uint32_t id = light_system_add_light(lights, LIGHT_TYPE_AREA);
Light* light = light_system_get_light(lights, id);
light_set_position(light, x, y, z);
light_set_area_size(light, 2.0f, 2.0f); // width, height
light_set_area_shape(light, LIGHT_SHAPE_RECTANGULAR);
```

### Shadow Configuration

```c
light_set_shadow_enabled(light, true);
light_set_shadow_resolution(light, 2048);
light_set_shadow_bias(light, 0.005f, 0.01f); // bias, normal_bias
```

---

## Shadow System

### Shadow Manager

```c
#include "renderer/shadows/shadow_manager.h"

// Create shadow manager
ShadowManager* shadows = shadow_manager_create(4096); // atlas size

// Allocate shadow map for a light
uint32_t shadow_id = shadow_manager_allocate_shadow_map(
    shadows, 
    SHADOW_MAP_CASCADE,  // for directional light
    2048                 // resolution
);
```

### LOD System

```c
// Set LOD distances
float lod_distances[] = {50.0f, 100.0f, 200.0f, 500.0f};
shadow_manager_set_lod_distances(shadows, lod_distances, 4);

// Get appropriate LOD for distance
uint32_t lod = shadow_manager_get_lod_for_distance(shadows, 150.0f);
```

---

## Post-Processing

### Color Grading

```c
#include "renderer/post_processing/color_grading.h"

// Create color grading params
ColorGradingParams grading = color_grading_default();

// Adjust lift (shadows)
grading.lift[0] = 0.0f;
grading.lift[1] = 0.0f;
grading.lift[2] = 0.05f; // Slight blue lift

// Adjust gamma (midtones)
grading.gamma[0] = 1.0f;
grading.gamma[1] = 1.0f;
grading.gamma[2] = 1.0f;

// Adjust gain (highlights)
grading.gain[0] = 1.1f; // Boost red in highlights
grading.gain[1] = 1.0f;
grading.gain[2] = 0.9f; // Reduce blue in highlights

// Saturation and contrast
grading.saturation = 1.2f;
grading.contrast = 1.1f;

// Temperature (warm/cool)
grading.temperature = 0.2f; // Slightly warm

// Apply to a pixel
float color[3] = {0.5f, 0.5f, 0.5f};
apply_color_grading(color, &grading);
```

### Presets

```c
ColorGradingParams warm = color_grading_preset_warm();
ColorGradingParams cool = color_grading_preset_cool();
ColorGradingParams cinematic = color_grading_preset_cinematic();
ColorGradingParams vibrant = color_grading_preset_vibrant();
```

---

## Environment System

### Skybox

```c
#include "renderer/environment/skybox.h"

Skybox* sky = skybox_create();

// Set procedural sky
skybox_set_procedural(sky, SKY_MODEL_HOSEK_WILKIE);
skybox_set_turbidity(sky, 2.5f); // Atmospheric clarity

// Time of day (0-24 hours)
skybox_set_time_of_day(sky, 14.5f); // 2:30 PM

// Stars and moon
skybox_enable_stars(sky, true);
skybox_enable_moon(sky, true);
skybox_set_moon_phase(sky, 0.75f); // Waning gibbous

// Exposure and tint
skybox_set_exposure(sky, 1.2f);
skybox_set_tint(sky, 1.0f, 1.0f, 0.95f); // Slight blue reduction
```

### IBL (Image-Based Lighting)

```c
#include "renderer/environment/ibl.h"

IBLSystem* ibl = ibl_system_create();

// Add environment probe
uint32_t probe_id = ibl_system_add_probe(ibl);
IBLProbe* probe = ibl_system_get_probe(ibl, probe_id);

// Configure probe
ibl_probe_set_position(probe, 0.0f, 0.0f, 0.0f);

// Set bounding box for parallax correction
float box_min[3] = {-10.0f, 0.0f, -10.0f};
float box_max[3] = {10.0f, 5.0f, 10.0f};
ibl_probe_set_box(probe, box_min, box_max);

// Set intensity
ibl_probe_set_intensity(probe, 1.0f);

// Capture and process (in actual renderer)
ibl_probe_capture(probe, position);
ibl_probe_convolve_diffuse(probe);
ibl_probe_prefilter_specular(probe);

// Set as global probe
ibl_system_set_global_probe(ibl, probe_id);
```

### Environment Manager

```c
#include "renderer/environment/environment_manager.h"

// Create unified environment
EnvironmentManager* env = environment_manager_create();

// Set time of day
environment_set_time_of_day(env, 12.0f); // Noon

// Access subsystems
Skybox* sky = environment_get_skybox(env);
IBLSystem* ibl = environment_get_ibl_system(env);

// Update environment (call each frame)
environment_update(env, delta_time);
```

---

## Complete Scene Setup Example

```c
#include "renderer/lighting/light_system.h"
#include "renderer/shadows/shadow_manager.h"
#include "renderer/environment/environment_manager.h"
#include "renderer/post_processing/color_grading.h"

void setup_scene(void) {
    // Environment
    EnvironmentManager* env = environment_manager_create();
    environment_set_time_of_day(env, 16.0f); // 4 PM
    
    // Lighting
    LightSystem* lights = light_system_create();
    
    // Sun (directional)
    uint32_t sun = light_system_add_light(lights, LIGHT_TYPE_DIRECTIONAL);
    Light* sun_light = light_system_get_light(lights, sun);
    light_set_direction(sun_light, 0.3f, -0.7f, 0.2f);
    light_set_color(sun_light, 1.0f, 0.95f, 0.9f); // Warm sunlight
    light_set_intensity(sun_light, 5.0f);
    light_set_shadow_enabled(sun_light, true);
    
    // Fill light (area)
    uint32_t fill = light_system_add_light(lights, LIGHT_TYPE_AREA);
    Light* fill_light = light_system_get_light(lights, fill);
    light_set_position(fill_light, -5.0f, 3.0f, 5.0f);
    light_set_area_size(fill_light, 3.0f, 3.0f);
    light_set_color(fill_light, 0.7f, 0.8f, 1.0f); // Cool fill
    light_set_intensity(fill_light, 2.0f);
    
    // Shadows
    ShadowManager* shadows = shadow_manager_create(4096);
    
    // Post-processing
    ColorGradingParams grading = color_grading_preset_cinematic();
    grading.saturation = 1.1f;
    
    printf("Scene setup complete!\n");
}
```

---

## Performance Tips

### Lighting
- Use **light culling** - only render visible lights
- Limit to **256 visible lights** maximum
- Use **shadow LOD** for distant objects
- **Bake static lighting** when possible

### Shadows
- Use **shadow atlas** to share memory
- Set **appropriate resolutions** (1024-2048 typical)
- Use **CSM** only for main directional light
- Enable **shadow caching** for static objects

### Materials
- **Reuse materials** where possible
- Use **instancing** for identical materials
- **Precompute** expensive operations
- Use **LOD** for distant objects

### Post-Processing
- Apply effects in **order of cost** (cheap first)
- Use **half-resolution** for expensive effects
- **Temporal filtering** reduces noise
- **Skip effects** for low-end hardware

---

## Troubleshooting

**Materials look flat**
- Check PBR parameters (metallic/roughness)
- Verify lighting is present
- Enable IBL for ambient

**Shadows have artifacts**
- Increase shadow bias
- Use higher resolution shadow maps
- Enable PCSS for softer shadows

**Performance is low**
- Reduce number of lights
- Lower shadow resolution
- Disable expensive post-effects
- Use LOD system

**Colors look wrong**
- Check tone mapping settings
- Verify color grading parameters
- Ensure proper gamma correction
- Check exposure values

---

## Conclusion

The rendering system provides all the tools needed for photorealistic real-time rendering. Start with simple setups and gradually add complexity as needed. Always profile performance and optimize based on your target hardware.
