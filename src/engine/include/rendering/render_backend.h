// Rendering Backend Abstraction
// C-based polymorphism using function pointer tables (V-Tables)
// Unifies Vulkan and Metal backends without C++ overhead

#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

#include <common.h>

// Forward declarations
typedef struct RenderBackend RenderBackend;
typedef struct BufferHandle BufferHandle;
typedef struct PipelineHandle PipelineHandle;
typedef struct TextureHandle TextureHandle;

// Buffer descriptor
typedef struct {
    u64 size;
    u32 usage_flags; // Vertex, Index, Uniform, Storage, etc.
    bool device_local;
    bool host_visible;
    const void* initial_data;
} BufferDesc;

// Pipeline descriptor  
typedef struct {
    const char* vertex_shader;
    const char* fragment_shader;
    u32 vertex_stride;
    // ... additional pipeline state
} PipelineDesc;

// Render pass info for Dynamic Rendering
typedef struct {
    TextureHandle* color_attachments;
    u32 color_attachment_count;
    TextureHandle* depth_attachment;
    bool clear_color;
    bool clear_depth;
    float clear_color_value[4];
    float clear_depth_value;
} PassInfo;

// Rendering backend V-Table
typedef struct {
    // Lifecycle
    bool (*init)(void* platform_data);
    void (*shutdown)(void);
    
    // Resource creation
    BufferHandle* (*create_buffer)(const BufferDesc* desc);
    void (*destroy_buffer)(BufferHandle* buffer);
    
    PipelineHandle* (*create_pipeline)(const PipelineDesc* desc);
    void (*destroy_pipeline)(PipelineHandle* pipeline);
    
    // Command recording
    void (*begin_frame)(void);
    void (*end_frame)(void);
    
    void (*begin_pass)(const PassInfo* info);
    void (*end_pass)(void);
    
    void (*bind_pipeline)(PipelineHandle* pipeline);
    void (*cmd_draw)(u32 vertex_count, u32 instance_count);
    void (*cmd_draw_indexed)(u32 index_count, u32 instance_count);
    
    // Submission and presentation
    void (*submit)(void);
    void (*present)(void);
    
    // Debug
    const char* (*get_backend_name)(void);
    void (*print_stats)(void);
    
} RenderBackendVTable;

// Render backend instance
struct RenderBackend {
    const RenderBackendVTable* vtable;
    void* backend_context; // Vulkan or Metal specific data
    const char* name;
};

// Global render backend instance
extern RenderBackend* g_render_backend;

// Initialize backend (platform-specific)
bool render_backend_init(void* platform_data);
void render_backend_shutdown(void);

// Inline wrappers for V-Table calls
static inline bool render_backend_create(void* platform_data) {
    return g_render_backend && g_render_backend->vtable->init(platform_data);
}

static inline BufferHandle* render_backend_create_buffer(const BufferDesc* desc) {
    return g_render_backend ? g_render_backend->vtable->create_buffer(desc) : NULL;
}

static inline void render_backend_begin_pass(const PassInfo* info) {
    if (g_render_backend) {
        g_render_backend->vtable->begin_pass(info);
    }
}

static inline void render_backend_cmd_draw(u32 vertex_count, u32 instance_count) {
    if (g_render_backend) {
        g_render_backend->vtable->cmd_draw(vertex_count, instance_count);
    }
}

static inline void render_backend_submit(void) {
    if (g_render_backend) {
        g_render_backend->vtable->submit();
    }
}

static inline const char* render_backend_get_name(void) {
    return g_render_backend ? g_render_backend->vtable->get_backend_name() : "None";
}

#endif // RENDER_BACKEND_H
