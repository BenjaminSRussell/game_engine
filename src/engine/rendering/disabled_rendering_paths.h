// Disabled Rendering Paths Header
// Provides fallback and debugging rendering modes

#ifndef DISABLED_RENDERING_PATHS_H
#define DISABLED_RENDERING_PATHS_H

#include "core/types.h"
#include "rendering/frame_graph/frame_graph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct DisabledRenderingContext DisabledRenderingContext;

// Rendering path configuration
typedef struct {
    bool forward_rendering_enabled;
    bool deferred_rendering_enabled;
    bool post_processing_enabled;
    bool shadows_enabled;
    bool lighting_enabled;
    bool texturing_enabled;
    bool wireframe_mode;
    bool depth_only_mode;
    bool normal_visualization;
    bool uv_visualization;
    bool performance_mode;
} RenderingPathConfig;

// Initialize disabled rendering system
DisabledRenderingContext* disabled_rendering_init(u32 width, u32 height);

// Shutdown disabled rendering system
void disabled_rendering_shutdown(DisabledRenderingContext *ctx);

// Configure rendering paths
void disabled_rendering_set_config(DisabledRenderingContext *ctx, const RenderingPathConfig *config);

// Render frame with disabled paths
void disabled_rendering_render_frame(DisabledRenderingContext *ctx, RenderGraph *render_graph);

// Get rendering statistics
void disabled_rendering_get_stats(DisabledRenderingContext *ctx, 
                                u64 *frames_rendered, 
                                f64 *average_frame_time, 
                                f64 *memory_usage_mb);

// Reset statistics
void disabled_rendering_reset_stats(DisabledRenderingContext *ctx);

#ifdef __cplusplus
}
#endif

#endif // DISABLED_RENDERING_PATHS_H
