#ifndef DOCKING_SYSTEM_H
#define DOCKING_SYSTEM_H

#include "core/types.h"
#include "math/vec2.h"
#include "math/vec4.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DOCK_Split_None,
    DOCK_Split_Left,
    DOCK_Split_Right,
    DOCK_Split_Up,
    DOCK_Split_Down,
    DOCK_Split_Tab
} DockSplitDir;

typedef struct DockNode DockNode;

// Represents a window/tab content
typedef struct {
    char title[64];
    u32 id;
    bool is_open;
    bool is_active;
    void* user_data;
    void (*render_callback)(void* user_data);
} DockWindow;

// Represents a node in the docking binary tree
struct DockNode {
    u32 id;
    DockNode* parent;
    DockNode* child_0; // Left/Top
    DockNode* child_1; // Right/Bottom
    DockSplitDir split_dir;
    float split_ratio; // 0.0 - 1.0
    
    // Leaf node data
    DockWindow** windows;
    u32 window_count;
    u32 active_window_index;
    
    // Layout
    Vec2 pos;
    Vec2 size;
};

typedef struct {
    DockNode* root;
    DockNode* active_node;
    float dock_padding;
    bool is_layout_dirty;
} DockContext;

// Global/Context management
void docking_init(DockContext* ctx);
void docking_shutdown(DockContext* ctx);
void docking_update(DockContext* ctx, Vec2 display_size);
void docking_draw(DockContext* ctx); // Main render loop hook

// Window management
DockWindow* docking_create_window(DockContext* ctx, const char* title, void (*render_cb)(void*), void* user_data);
void docking_dock_window(DockContext* ctx, DockWindow* window, DockNode* target_node, DockSplitDir split_dir, float ratio);

// Layout management
DockNode* docking_get_root(DockContext* ctx);
void docking_save_layout(DockContext* ctx, const char* path);
void docking_load_layout(DockContext* ctx, const char* path);

// Helper to start the root dockspace (often called at start of frame)
void docking_begin_dockspace(DockContext* ctx, const char* name);
void docking_end_dockspace(DockContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // DOCKING_SYSTEM_H
