// Rendering Path Manager Implementation
// Manages enabling/disabling different rendering paths and features

#include "rendering_path_manager.h"
#include "core/logger.h"
#include "core/memory.h"
#include "rendering/voxel_renderer.h"
#include "rendering/sprite_3d_renderer.h"
#include <stdlib.h>
#include <string.h>

// Hardware detection functions
#ifdef __APPLE__
#include <Metal/Metal.h>
#include <sys/sysctl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#endif

// Helper functions
static bool detect_metal_raytracing_support(void) {
#ifdef __APPLE__
    if (@available(macOS 11.0, iOS 14.0, *)) {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        bool supported = [device supportsRaytracing];
        [device release];
        return supported;
    }
#endif
    return false;
}

static bool detect_metal_vrs_support(void) {
#ifdef __APPLE__
    if (@available(macOS 12.0, *)) {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        bool supported = [device supportsFamily:MTLGPUFamilyApple7];
        [device release];
        return supported;
    }
#endif
    return false;
}

static bool detect_simd_support(void) {
    bool avx2 = false;
    bool avx512 = false;
    
#ifdef _WIN32
    int cpu_info[4];
    __cpuid(cpu_info, 0);
    if (cpu_info[0] >= 7) {
        __cpuidex(cpu_info, 7, 0);
        avx2 = (cpu_info[1] & (1 << 5)) != 0;
        avx512 = (cpu_info[1] & (1 << 16)) != 0;
    }
#elif defined(__APPLE__)
    // Apple Silicon always has good SIMD support
    avx2 = true;
    // Check for AVX-512 on Intel Macs
    size_t size = sizeof(int);
    sysctlbyname("hw.optional.avx512f", &avx512, &size, NULL, 0);
#endif
    
    return avx2 || avx512;
}

// Create rendering path manager
RenderingPathManager *rendering_path_manager_create(void) {
    RenderingPathManager *manager = malloc(sizeof(RenderingPathManager));
    if (!manager) {
        LOG_ERROR("Failed to allocate rendering path manager");
        return NULL;
    }
    
    memset(manager, 0, sizeof(RenderingPathManager));
    
    // Detect hardware capabilities
    rendering_path_manager_detect_hardware(manager);
    
    // Set default configuration
    manager->config.enabled_paths = RENDERING_PATH_FORWARD;
    manager->config.enabled_features = RENDERING_FEATURE_TAA | RENDERING_FEATURE_BLOOM | RENDERING_FEATURE_FOG;
    
    // Enable advanced features if supported
    if (manager->supports_raytracing) {
        manager->config.enabled_features |= RENDERING_FEATURE_RAYTRACING;
    }
    
    if (manager->supports_vrs) {
        manager->config.enabled_features |= RENDERING_FEATURE_VRS;
    }
    
    if (manager->supports_simd_avx2 || manager->supports_simd_avx512) {
        manager->config.enabled_features |= RENDERING_FEATURE_SIMD_MATH;
    }
    
    // Set fallback paths
    manager->config.fallback_paths = RENDERING_PATH_FORWARD;
    manager->config.disabled_features = 0;
    
    // Auto-select optimal path
    rendering_path_manager_auto_select(manager);
    
    manager->initialized = true;
    LOG_INFO("Rendering path manager created");
    
    return manager;
}

// Destroy rendering path manager
void rendering_path_manager_destroy(RenderingPathManager *manager) {
    if (!manager) return;
    
    free(manager);
    LOG_INFO("Rendering path manager destroyed");
}

// Detect hardware capabilities
void rendering_path_manager_detect_hardware(RenderingPathManager *manager) {
    if (!manager) return;
    
    LOG_INFO("Detecting hardware capabilities...");
    
    // Metal ray tracing support
    manager->supports_raytracing = detect_metal_raytracing_support();
    LOG_INFO("Ray tracing support: %s", manager->supports_raytracing ? "YES" : "NO");
    
    // Metal VRS support
    manager->supports_vrs = detect_metal_vrs_support();
    LOG_INFO("VRS support: %s", manager->supports_vrs ? "YES" : "NO");
    
    // Compute shader support (assumed on modern hardware)
    manager->supports_compute = true;
    LOG_INFO("Compute support: YES");
    
    // SIMD support
    bool simd_support = detect_simd_support();
    manager->supports_simd_avx2 = simd_support;
    manager->supports_simd_avx512 = simd_support && false; // Conservative
    LOG_INFO("SIMD support: AVX2=%s, AVX512=%s", 
             manager->supports_simd_avx2 ? "YES" : "NO",
             manager->supports_simd_avx512 ? "YES" : "NO");
}

// Enable rendering path
void rendering_path_manager_enable_path(RenderingPathManager *manager, RenderingPathFlags paths) {
    if (!manager) return;
    
    manager->config.enabled_paths |= paths;
    LOG_DEBUG("Enabled rendering paths: 0x%04X", paths);
}

// Disable rendering path
void rendering_path_manager_disable_path(RenderingPathManager *manager, RenderingPathFlags paths) {
    if (!manager) return;
    
    manager->config.enabled_paths &= ~paths;
    LOG_DEBUG("Disabled rendering paths: 0x%04X", paths);
}

// Check if path is enabled
bool rendering_path_manager_is_path_enabled(RenderingPathManager *manager, RenderingPathFlags path) {
    if (!manager) return false;
    
    return (manager->config.enabled_paths & path) != 0;
}

// Enable rendering feature
void rendering_path_manager_enable_feature(RenderingPathManager *manager, RenderingFeatureFlags features) {
    if (!manager) return;
    
    // Check if features are supported
    RenderingFeatureFlags supported = 0;
    if (manager->supports_raytracing) supported |= RENDERING_FEATURE_RAYTRACING;
    if (manager->supports_vrs) supported |= RENDERING_FEATURE_VRS;
    if (manager->supports_simd_avx2 || manager->supports_simd_avx512) supported |= RENDERING_FEATURE_SIMD_MATH;
    
    RenderingFeatureFlags can_enable = features & supported;
    manager->config.enabled_features |= can_enable;
    
    if (can_enable != features) {
        LOG_WARN("Some features not supported: 0x%04X", features & ~supported);
    }
    
    LOG_DEBUG("Enabled rendering features: 0x%04X", can_enable);
}

// Disable rendering feature
void rendering_path_manager_disable_feature(RenderingPathManager *manager, RenderingFeatureFlags features) {
    if (!manager) return;
    
    manager->config.enabled_features &= ~features;
    LOG_DEBUG("Disabled rendering features: 0x%04X", features);
}

// Check if feature is enabled
bool rendering_path_manager_is_feature_enabled(RenderingPathManager *manager, RenderingFeatureFlags feature) {
    if (!manager) return false;
    
    return (manager->config.enabled_features & feature) != 0;
}

// Get optimal rendering path
RenderingPathFlags rendering_path_manager_get_optimal_path(RenderingPathManager *manager) {
    if (!manager) return RENDERING_PATH_FORWARD;
    
    // Prefer ray tracing if supported
    if (manager->supports_raytracing && rendering_path_manager_is_feature_enabled(manager, RENDERING_FEATURE_RAYTRACING)) {
        return RENDERING_PATH_RAYTRACED;
    }
    
    // Prefer deferred rendering for advanced features
    if (rendering_path_manager_is_feature_enabled(manager, RENDERING_FEATURE_SSAO | RENDERING_FEATURE_SSR)) {
        return RENDERING_PATH_DEFERRED;
    }
    
    // Default to forward rendering
    return RENDERING_PATH_FORWARD;
}

// Set quality level
void rendering_path_manager_set_quality_level(RenderingPathManager *manager, u32 quality_level) {
    if (!manager || quality_level > 5) return;
    
    LOG_INFO("Setting rendering quality level: %d", quality_level);
    
    // Reset features
    manager->config.enabled_features = 0;
    
    switch (quality_level) {
        case 0: // Low
            manager->config.enabled_paths = RENDERING_PATH_FORWARD;
            manager->config.enabled_features = RENDERING_FEATURE_FOG;
            break;
            
        case 1: // Medium
            manager->config.enabled_paths = RENDERING_PATH_FORWARD;
            manager->config.enabled_features = RENDERING_FEATURE_FOG | RENDERING_FEATURE_BLOOM;
            break;
            
        case 2: // High
            manager->config.enabled_paths = RENDERING_PATH_FORWARD;
            manager->config.enabled_features = RENDERING_FEATURE_FOG | RENDERING_FEATURE_BLOOM | RENDERING_FEATURE_TAA;
            break;
            
        case 3: // Ultra
            manager->config.enabled_paths = RENDERING_PATH_DEFERRED;
            manager->config.enabled_features = RENDERING_FEATURE_FOG | RENDERING_FEATURE_BLOOM | RENDERING_FEATURE_TAA | 
                                           RENDERING_FEATURE_SSAO | RENDERING_FEATURE_SHADOWS;
            break;
            
        case 4: // Extreme
            manager->config.enabled_paths = RENDERING_PATH_DEFERRED;
            manager->config.enabled_features = RENDERING_FEATURE_FOG | RENDERING_FEATURE_BLOOM | RENDERING_FEATURE_TAA | 
                                           RENDERING_FEATURE_SSAO | RENDERING_FEATURE_SSR | RENDERING_FEATURE_SHADOWS |
                                           RENDERING_FEATURE_POST_PROCESSING;
            break;
            
        case 5: // Insane
            manager->config.enabled_paths = RENDERING_PATH_RAYTRACED;
            manager->config.enabled_features = RENDERING_FEATURE_ALL;
            if (!manager->supports_raytracing) {
                manager->config.enabled_paths = RENDERING_PATH_DEFERRED;
                manager->config.enabled_features &= ~RENDERING_FEATURE_RAYTRACING;
            }
            break;
    }
    
    // Add hardware-specific features
    if (manager->supports_vrs) {
        manager->config.enabled_features |= RENDERING_FEATURE_VRS;
    }
    
    if (manager->supports_simd_avx2 || manager->supports_simd_avx512) {
        manager->config.enabled_features |= RENDERING_FEATURE_SIMD_MATH;
    }
}

// Auto-select best rendering path
void rendering_path_manager_auto_select(RenderingPathManager *manager) {
    if (!manager) return;
    
    LOG_INFO("Auto-selecting optimal rendering configuration...");
    
    // Start with all paths enabled
    manager->config.enabled_paths = RENDERING_PATH_FORWARD | RENDERING_PATH_DEFERRED | 
                                   RENDERING_PATH_VOXEL | RENDERING_PATH_SPRITE_3D;
    
    // Add ray tracing if supported
    if (manager->supports_raytracing) {
        manager->config.enabled_paths |= RENDERING_PATH_RAYTRACED;
    }
    
    // Add compute if supported
    if (manager->supports_compute) {
        manager->config.enabled_paths |= RENDERING_PATH_COMPUTE;
    }
    
    // Enable all supported features
    RenderingFeatureFlags all_features = RENDERING_FEATURE_ALL;
    rendering_path_manager_enable_feature(manager, all_features);
    
    // Select optimal current path
    manager->current_path = rendering_path_manager_get_optimal_path(manager);
    
    LOG_INFO("Selected rendering path: 0x%04X", manager->current_path);
    LOG_INFO("Enabled features: 0x%04X", manager->config.enabled_features);
}

// Update performance metrics
void rendering_path_manager_update_metrics(RenderingPathManager *manager, RenderingPathFlags path, f32 render_time) {
    if (!manager) return;
    
    switch (path) {
        case RENDERING_PATH_FORWARD:
            manager->forward_render_time = render_time;
            break;
        case RENDERING_PATH_DEFERRED:
            manager->deferred_render_time = render_time;
            break;
        case RENDERING_PATH_RAYTRACED:
            manager->raytraced_render_time = render_time;
            break;
        case RENDERING_PATH_VOXEL:
            manager->voxel_render_time = render_time;
            break;
        case RENDERING_PATH_SPRITE_3D:
            manager->sprite_3d_render_time = render_time;
            break;
        default:
            break;
    }
}

// Get configuration
RenderingPathConfig rendering_path_manager_get_config(RenderingPathManager *manager) {
    if (!manager) {
        RenderingPathConfig empty = {0};
        return empty;
    }
    
    return manager->config;
}

// Apply configuration to renderer
void rendering_path_manager_apply_config(RenderingPathManager *manager, IRenderer *renderer) {
    if (!manager || !renderer) return;
    
    LOG_INFO("Applying rendering configuration to renderer");
    
    // Apply path-specific settings
    if (rendering_path_manager_is_path_enabled(manager, RENDERING_PATH_RAYTRACED)) {
        // Enable ray tracing features
        LOG_DEBUG("Enabling ray tracing features");
    }
    
    if (rendering_path_manager_is_path_enabled(manager, RENDERING_PATH_DEFERRED)) {
        // Enable deferred rendering features
        LOG_DEBUG("Enabling deferred rendering features");
    }
    
    // Apply feature-specific settings
    if (rendering_path_manager_is_feature_enabled(manager, RENDERING_FEATURE_TAA)) {
        LOG_DEBUG("Enabling TAA");
    }
    
    if (rendering_path_manager_is_feature_enabled(manager, RENDERING_FEATURE_SSAO)) {
        LOG_DEBUG("Enabling SSAO");
    }
    
    if (rendering_path_manager_is_feature_enabled(manager, RENDERING_FEATURE_SSR)) {
        LOG_DEBUG("Enabling SSR");
    }
    
    if (rendering_path_manager_is_feature_enabled(manager, RENDERING_FEATURE_VRS)) {
        LOG_DEBUG("Enabling VRS");
    }
    
    if (rendering_path_manager_is_feature_enabled(manager, RENDERING_FEATURE_SIMD_MATH)) {
        LOG_DEBUG("Enabling SIMD optimizations");
    }
}

// Check if feature is supported
bool rendering_path_manager_is_feature_supported(RenderingPathManager *manager, RenderingFeatureFlags feature) {
    if (!manager) return false;
    
    switch (feature) {
        case RENDERING_FEATURE_RAYTRACING:
            return manager->supports_raytracing;
        case RENDERING_FEATURE_VRS:
            return manager->supports_vrs;
        case RENDERING_FEATURE_SIMD_MATH:
            return manager->supports_simd_avx2 || manager->supports_simd_avx512;
        case RENDERING_FEATURE_COMPUTE:
            return manager->supports_compute;
        default:
            return true; // Other features are generally supported
    }
}

// Get fallback path
RenderingPathFlags rendering_path_manager_get_fallback_path(RenderingPathManager *manager, RenderingPathFlags primary_path) {
    if (!manager) return RENDERING_PATH_FORWARD;
    
    switch (primary_path) {
        case RENDERING_PATH_RAYTRACED:
            return RENDERING_PATH_DEFERRED;
        case RENDERING_PATH_DEFERRED:
            return RENDERING_PATH_FORWARD;
        case RENDERING_PATH_VOXEL:
            return RENDERING_PATH_FORWARD;
        case RENDERING_PATH_SPRITE_3D:
            return RENDERING_PATH_FORWARD;
        default:
            return manager->config.fallback_paths;
    }
}
