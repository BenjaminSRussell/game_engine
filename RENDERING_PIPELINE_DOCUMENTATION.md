# Rendering Pipeline Documentation

## Overview

This document provides comprehensive documentation for the advanced 3D rendering pipeline implemented in the Minecraft v2 game engine. The pipeline supports modern graphics features including GPU-driven rendering, deferred shading, atmospheric scattering, and real-time global illumination.

## Architecture

### Core Components

#### 1. Core Renderer System (`core/renderer.c`)
- **Purpose**: Main rendering pipeline orchestrator
- **Features**: Frame management, Metal integration, statistics tracking
- **API**: `renderer_create()`, `renderer_begin_frame()`, `renderer_end_frame()`

#### 2. Render Graph System (`core/render_graph.c`)
- **Purpose**: Dependency-based render pass management
- **Features**: Automatic barrier insertion, resource lifecycle, circular dependency detection
- **API**: `render_graph_create()`, `render_graph_add_pass()`, `render_graph_execute()`

#### 3. Culling System (`core/culling.c`)
- **Purpose**: Efficient visibility determination
- **Features**: Frustum culling, distance culling, occlusion culling
- **API**: `culling_system_create()`, `culling_system_perform()`, `culling_system_get_visible()`

#### 4. Material System (`core/material.c`)
- **Purpose**: PBR material management
- **Features**: Material instances, texture management, hot-reloading
- **API**: `material_create()`, `material_set_albedo()`, `material_bind()`

#### 5. GPU-Driven Rendering (`gpu_driven/draw_command_gen.c`)
- **Purpose**: GPU-side culling and command generation
- **Features**: Compute shader culling, indirect rendering, LOD selection
- **API**: `draw_command_system_create()`, `draw_command_system_perform_culling()`

### Rendering Paths

#### Primary: GPU-Driven Rendering
- **Target**: Modern GPUs with compute capability
- **Features**: GPU culling, indirect drawing, massive scene support
- **Performance**: Scales to millions of objects

#### Secondary: Deferred Rendering
- **Target**: Complex lighting scenarios
- **Features**: G-buffer, screen-space effects, decal support
- **Use Case**: Many dynamic lights, post-processing

#### Tertiary: Forward Rendering
- **Target**: Mobile/older hardware
- **Features**: Simpler architecture, transparency support
- **Use Case**: Fallback compatibility

## Rendering Pipeline Flow

```
Scene Input → Culling → G-Buffer → Lighting → Post-Process → Output
     ↓         ↓        ↓         ↓           ↓
GPU Compute  Frustum  PBR       Bloom      FXAA
Indirect     Distance Shadows   Tone Map  Color Grade
Commands     Occlusion Caustics  Vignette  Chromatic
```

## Advanced Features

### Lighting System (`lighting/lighting_system.c`)
- **Clustered Shading**: Efficient light culling in 3D grid
- **Light Types**: Directional, point, spot, area lights
- **Shadow Support**: Cascaded shadow maps, PCF filtering
- **API**: `lighting_create_light()`, `lighting_update_lights()`

### Shadow Mapping (`shadows/shadow_mapping.c`)
- **Cascaded Shadow Maps**: 4 cascades for directional lights
- **Filtering**: PCF 2x2, 3x3, 5x5, PCSS support
- **Optimization**: Shadow atlas, frustum fitting
- **API**: `shadow_mapping_render_shadows()`, `shadow_mapping_bind_cascade_textures()`

### Post-Processing (`post_process/post_process_pipeline.c`)
- **Effects**: Bloom, tone mapping, FXAA, SMAA, TAA
- **Color Grading**: Contrast, saturation, brightness control
- **Quality Settings**: Adjustable effect parameters
- **API**: `post_process_execute()`, `post_process_enable_effect()`

### Particle System (`particles/particle_system.c`)
- **Physics**: Gravity, wind, turbulence, drag
- **Emission**: Point, sphere, cone, box emission shapes
- **Rendering**: Billboards, meshes, ribbons
- **API**: `particle_system_create_emitter()`, `particle_system_update()`

### Sky Rendering (`sky/sky_rendering.c`)
- **Atmospheric Scattering**: Rayleigh and Mie scattering
- **Time System**: Day/night cycle with celestial bodies
- **Weather**: Dynamic clouds, wind effects
- **API**: `sky_system_set_time_of_day()`, `sky_system_render()`

### Water Rendering (`water/water_rendering.c`)
- **Wave Simulation**: Gerstner waves with multiple components
- **Visual Effects**: Reflections, refractions, foam, caustics
- **Interaction**: Ripple system, wind influence
- **API**: `water_system_add_ripple()`, `water_system_get_wave_height()`

## Developer Tools

### Shader Hot-Reloading (`shader/shader_hot_reload.c`)
- **File Monitoring**: Automatic shader recompilation
- **Error Handling**: Compilation error reporting
- **Cross-Platform**: macOS FSEventStream support
- **API**: `shader_hot_reload_add_shader()`, `shader_hot_reload_update()`

### Render Statistics (`core/render_stats.c`)
- **Performance Metrics**: FPS, frame time, memory usage
- **Detailed Tracking**: Draw calls, triangles, culling stats
- **Historical Data**: Frame history, averages
- **API**: `render_stats_begin_frame()`, `render_stats_print_summary()`

### Texture Streaming (`texture/texture_streaming.c`)
- **Async Loading**: Background texture loading
- **Memory Management**: Budget-based streaming
- **Priority System**: Distance-based loading
- **API**: `texture_streaming_add_texture()`, `texture_streaming_get_texture()`

## Memory Management

### GPU Memory Management
- **Texture Streaming**: Automatic load/unload based on distance
- **Buffer Pooling**: Reusable GPU buffers
- **Resource Cleanup**: Automatic resource destruction

### CPU Memory Management
- **Object Pooling**: Reusable particle and light objects
- **Smart Pointers**: Reference counting for shared resources
- **Leak Detection**: Memory tracking and reporting

## Performance Optimizations

### GPU-Side Optimizations
- **Indirect Rendering**: Reduce CPU overhead
- **Compute Culling**: GPU-side visibility determination
- **Clustered Shading**: Efficient light management
- **Texture Atlasing**: Reduce texture switches

### CPU-Side Optimizations
- **Spatial Partitioning**: Efficient culling
- **Batching**: Group similar draw calls
- **LOD System**: Distance-based detail reduction
- **Multithreading**: Parallel resource loading

## Quality Settings

### Rendering Quality Levels
- **Ultra**: Maximum visual quality, 4K+ resolution
- **High**: High quality, 1440p-4K resolution
- **Medium**: Good quality, 1080p-1440p resolution
- **Low**: Basic quality, 720p-1080p resolution

### Adaptive Quality
- **Dynamic Resolution**: Scale based on performance
- **Effect Scaling**: Adjust post-processing quality
- **LOD Bias**: Aggressive distance culling

## Integration Guide

### Initialization Sequence
```c
// 1. Initialize core systems
renderer_create(width, height);
render_graph_create();
culling_system_create(max_objects);

// 2. Initialize advanced systems
lighting_system_init(max_lights, true);
shadow_mapping_init(4, 2048, 4096, SHADOW_FILTER_PCF_3X3);
post_process_init(width, height, true);

// 3. Initialize environmental systems
sky_system_init(true, true);
water_system_init(WATER_QUALITY_HIGH, true);
particle_system_init(32, 100000);

// 4. Initialize developer tools
shader_hot_reload_init();
render_stats_init(300);
texture_streaming_init(512); // 512MB budget
```

### Frame Rendering Loop
```c
void render_frame(const Scene* scene, const Camera* camera) {
    // Begin frame
    renderer_begin_frame();
    render_stats_begin_frame();
    
    // Update systems
    culling_system_update_frustum(camera->view, camera->proj);
    lighting_update_lights(camera->view, camera->proj);
    particle_system_update(dt);
    sky_system_update(dt);
    water_system_update(dt, wind_dir, wind_speed);
    
    // Render shadows
    shadow_mapping_render_shadows(lights, light_count, 
                                  camera->view, camera->proj, near, far);
    
    // Render main scene
    render_graph_execute();
    
    // Render environmental effects
    sky_system_render(camera->view, camera->proj, camera->position);
    water_system_render(camera->view, camera->proj, camera->position);
    particle_system_render(camera->view, camera->proj);
    
    // Post-processing
    post_process_set_input_texture(scene_color_texture);
    post_process_execute();
    
    // End frame
    render_stats_end_frame();
    renderer_end_frame();
}
```

## API Reference

### Core Functions
- `Renderer* renderer_create(uint32_t width, uint32_t height)`
- `void renderer_destroy(Renderer* renderer)`
- `void renderer_begin_frame(Renderer* renderer)`
- `void renderer_end_frame(Renderer* renderer)`

### Material Functions
- `Material* material_create(const char* name)`
- `void material_set_albedo(Material* material, float r, float g, float b)`
- `void material_bind(Material* material)`

### Lighting Functions
- `Light* lighting_create_light(const char* name, LightType type)`
- `void lighting_set_light_position(Light* light, float x, float y, float z)`
- `void lighting_update_lights(const float* view, const float* proj)`

### Post-Processing Functions
- `bool post_process_init(uint32_t width, uint32_t height, bool hdr)`
- `void post_process_execute(void)`
- `void post_process_enable_bloom(bool enabled, float threshold, float intensity)`

## Performance Guidelines

### Best Practices
1. **Batch Similar Objects**: Group objects with same material
2. **Use LOD Systems**: Reduce detail for distant objects
3. **Enable Culling**: Remove invisible objects early
4. **Stream Textures**: Load textures asynchronously
5. **Profile Regularly**: Monitor performance metrics

### Common Pitfalls
1. **Too Many Draw Calls**: Batch when possible
2. **Large Textures**: Use texture atlases
3. **Expensive Shaders**: Use LOD for shader complexity
4. **Memory Leaks**: Ensure proper cleanup
5. **Synchronization Issues**: Use proper threading

## Troubleshooting

### Common Issues
- **Black Screen**: Check shader compilation and resource loading
- **Poor Performance**: Enable statistics and identify bottlenecks
- **Visual Artifacts**: Verify normal maps and texture coordinates
- **Memory Issues**: Check texture streaming and buffer sizes

### Debug Tools
- **Render Statistics**: `render_stats_print_summary()`
- **Shader Logs**: Check hot-reload error callbacks
- **Memory Tracking**: Monitor texture streaming stats
- **Performance Profiling**: Use built-in timing measurements

## Future Enhancements

### Planned Features
- **Ray Tracing**: Hybrid rasterization + ray tracing
- **Global Illumination**: DDGI and ReSTIR integration
- **Mesh Shaders**: Geometry pipeline optimization
- **Variable Rate Shading**: Adaptive rendering quality
- **VR Support**: Stereo rendering optimizations

### Extension Points
- **Custom Shaders**: Plug-in shader system
- **Custom Effects**: Post-processing effect framework
- **Custom Materials**: Material property extensions
- **Custom Culling**: Pluggable culling algorithms

## Conclusion

This rendering pipeline provides a comprehensive, modern graphics solution suitable for both indie and AAA game development. The modular architecture allows for easy customization and extension while maintaining high performance across different hardware configurations.

The implementation follows industry best practices and provides a solid foundation for creating visually stunning 3D games and applications.
