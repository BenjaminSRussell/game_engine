#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "engine/ui/ui_renderer.h"
#include "engine/ui/ui_atlas.h"
#include "engine/ui/widgets/tab_widget.h"
#include "engine/ui/widgets/widget.h"
#include "engine/ui/widgets/button.h"

// Mock for logging if needed, but we link unified_logger usually or assume macros work
// If core/logger.h is included, it might need implementation.
// Assuming we can link against necessary core files.

int main() {
    printf("Starting UI System Tests...\n");

    // 1. Test Atlas
    printf("Testing UI Atlas...\n");
    UIAtlas* atlas = ui_atlas_create(1024, 1024, 4);
    assert(atlas != NULL);

    UIAtlasRegion region;
    bool alloc_success = ui_atlas_alloc(atlas, 32, 32, &region);
    assert(alloc_success);
    assert(region.width == 32);

    ui_atlas_destroy(atlas);
    printf("UI Atlas Tests Passed.\n");

    // 2. Test Renderer
    printf("Testing UI Renderer...\n");
    UIRenderer renderer;
    bool init_success = ui_renderer_init(&renderer, 800, 600, UI_AA_NONE, UI_GPU_BACKEND_NONE, UI_Z_SORT_NONE);
    assert(init_success);

    // Test Clipping
    Rect clip = {10, 10, 100, 100};
    ui_renderer_push_clip_rect(&renderer, clip);
    // (Internal check: stack top should be 1)

    ui_renderer_pop_clip_rect(&renderer);
    // (Internal check: stack top should be 0)

    ui_renderer_shutdown(&renderer);
    printf("UI Renderer Tests Passed.\n");

    // 3. Test Tab Widget
    printf("Testing Tab Widget...\n");
    TabWidget* tabs = tab_widget_create("MainTabs");
    assert(tabs != NULL);

    // Add tabs
    Widget* page1 = widget_create("Page1");
    Widget* page2 = widget_create("Page2");

    tab_widget_add_tab(tabs, "Tab 1", page1);
    tab_widget_add_tab(tabs, "Tab 2", page2);

    assert(tab_widget_get_active_tab(tabs) == 0);
    assert(page1->visible == true);
    assert(page2->visible == false);

    // Switch tab programmatically
    tab_widget_set_active_tab(tabs, 1);
    assert(tab_widget_get_active_tab(tabs) == 1);
    assert(page1->visible == false);
    assert(page2->visible == true);

    // Simulate layout
    widget_set_size((Widget*)tabs, (Vec2){800, 600});
    // Call layout manually since we don't have a full UI system loop here
    if (tabs->widget.layout) {
        tabs->widget.layout((Widget*)tabs, 800, 600);
    }

    // Verify header placement
    // Header height is 30.
    Vec2 header_size = widget_get_size(tabs->header_container);
    assert(header_size.y == 30.0f);

    widget_destroy((Widget*)tabs);
    printf("Tab Widget Tests Passed.\n");

    printf("All UI System Tests Passed!\n");
    return 0;
}
