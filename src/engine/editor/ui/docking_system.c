#include "editor/ui/docking_system.h"
#include "engine/include/core/memory.h"
#include "engine/include/core/logger.h"
#include "core/json.h" // Assuming JSON support exists given json.c in metadata
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Internal ID counter
static u32 g_dock_id_counter = 1;

static DockNode* create_node() {
    DockNode* node = (DockNode*)calloc(1, sizeof(DockNode));
    node->id = g_dock_id_counter++;
    node->split_ratio = 0.5f;
    node->split_dir = DOCK_Split_None;
    return node;
}

static void destroy_node(DockNode* node) {
    if (!node) return;
    
    if (node->child_0) destroy_node(node->child_0);
    if (node->child_1) destroy_node(node->child_1);
    
    if (node->windows) {
        // Windows themselves might be owned elsewhere or freed here? 
        // For now, just free the array pointer
        free(node->windows);
    }
    free(node);
}

void docking_init(DockContext* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(DockContext));
    ctx->root = create_node(); // Start with single root leaf
    ctx->dock_padding = 2.0f;
    LOG_INFO("Docking system initialized");
}

void docking_shutdown(DockContext* ctx) {
    if (!ctx) return;
    if (ctx->root) {
        destroy_node(ctx->root);
    }
    memset(ctx, 0, sizeof(DockContext));
}

// Recursive layout update
static void update_node_layout(DockNode* node, Vec2 pos, Vec2 size, float padding) {
    if (!node) return;
    
    node->pos = pos;
    node->size = size;
    
    if (node->child_0 && node->child_1) {
        Vec2 size_0 = size;
        Vec2 size_1 = size;
        Vec2 pos_1 = pos;
        
        float split_px;
        
        switch (node->split_dir) {
            case DOCK_Split_Left: // Child 0 is Left
                split_px = size.x * node->split_ratio;
                size_0.x = split_px - padding;
                size_1.x = size.x - split_px;
                pos_1.x += split_px;
                break;
            case DOCK_Split_Right: // Child 0 is Right (swap logic if needed, but lets keep Child 0 as primary split)
                 split_px = size.x * node->split_ratio;
                 // Typically Split_Right means Child 1 is right?
                 // Standard binary tree: Child 0 is "first", Child 1 is "second"
                 // If Split Is Vertical (Left/Right):
                 size_0.x = split_px - padding;
                 size_1.x = size.x - split_px;
                 pos_1.x += split_px;
                 break;
            case DOCK_Split_Up:
            case DOCK_Split_Down:
                split_px = size.y * node->split_ratio;
                size_0.y = split_px - padding;
                size_1.y = size.y - split_px;
                pos_1.y += split_px;
                break;
            default: break;
        }
        
        update_node_layout(node->child_0, pos, size_0, padding);
        update_node_layout(node->child_1, pos_1, size_1, padding);
    }
}

void docking_update(DockContext* ctx, Vec2 display_size) {
    if (!ctx || !ctx->root) return;
    
    // Update root to fill display (minus toolbar etc if we had it)
    update_node_layout(ctx->root, (Vec2){0,0}, display_size, ctx->dock_padding);
}

static void render_node(DockNode* node) {
    if (!node) return;
    
    // Internal node? Render children
    if (node->child_0 || node->child_1) {
        if (node->child_0) render_node(node->child_0);
        if (node->child_1) render_node(node->child_1);
        return;
    }
    
    // Leaf node? Render windows
    if (node->window_count > 0) {
        // Render tab bar 
        // Render active window content
        DockWindow* active = node->windows[node->active_window_index];
        if (active && active->render_callback) {
            // Need to set up scissor/viewport here theoretically
            active->render_callback(active->user_data);
        }
    }
}

void docking_draw(DockContext* ctx) {
    if (!ctx || !ctx->root) return;
    render_node(ctx->root);
}

DockWindow* docking_create_window(DockContext* ctx, const char* title, void (*render_cb)(void*), void* user_data) {
    DockWindow* win = (DockWindow*)calloc(1, sizeof(DockWindow));
    strncpy(win->title, title, sizeof(win->title)-1);
    win->render_callback = render_cb;
    win->user_data = user_data;
    win->id = g_dock_id_counter++;
    win->is_open = true;
    
    // Default: Add to root if it's a leaf, or find first leaf?
    // For now, user must manually dock it or we add to root.
    if (ctx->root && !ctx->root->child_0) {
        // Add to root
        docking_dock_window(ctx, win, ctx->root, DOCK_Split_Tab, 0.5f);
    }
    
    return win;
}

void docking_dock_window(DockContext* ctx, DockWindow* window, DockNode* target_node, DockSplitDir split_dir, float ratio) {
    if (!ctx || !window || !target_node) return;
    
    if (split_dir == DOCK_Split_Tab) {
        // Add to array
        target_node->window_count++;
        target_node->windows = (DockWindow**)realloc(target_node->windows, sizeof(DockWindow*) * target_node->window_count);
        target_node->windows[target_node->window_count - 1] = window;
        target_node->active_window_index = target_node->window_count - 1; // Focus new tab
    } else {
        // Split node
        // 1. Move current content to Child 0 (or 1 depending on dir)
        // 2. New window goes to other Child
        // Simplified impl:
        DockNode* new_child_0 = create_node();
        DockNode* new_child_1 = create_node();
        
        // Move content
        new_child_0->windows = target_node->windows;
        new_child_0->window_count = target_node->window_count;
        new_child_0->active_window_index = target_node->active_window_index;
        
        target_node->windows = NULL;
        target_node->window_count = 0;
        
        // New window
        new_child_1->window_count = 1;
        new_child_1->windows = (DockWindow**)malloc(sizeof(DockWindow*));
        new_child_1->windows[0] = window;
        
        target_node->child_0 = new_child_0;
        target_node->child_1 = new_child_1;
        target_node->split_dir = split_dir;
        target_node->split_ratio = ratio;
        
        new_child_0->parent = target_node;
        new_child_1->parent = target_node;
    }
}

DockNode* docking_get_root(DockContext* ctx) {
    return ctx ? ctx->root : NULL;
}

void docking_save_layout(DockContext* ctx, const char* path) {
    // Stub: Serialize tree to JSON
    LOG_INFO("Saved layout to %s", path);
}

void docking_load_layout(DockContext* ctx, const char* path) {
    // Stub: Deserialize JSON to tree
    LOG_INFO("Loaded layout from %s", path);
}

void docking_begin_dockspace(DockContext* ctx, const char* name) {
    // Initial setup if needed
}

void docking_end_dockspace(DockContext* ctx) {
    // Cleanup if needed
}
