#pragma once

#include <stdbool.h>

typedef enum {
    SELECT_MODE_REPLACE,    // Default
    SELECT_MODE_ADD,        // Shift
    SELECT_MODE_REMOVE      // Ctrl
} SelectionMode;

typedef struct {
    bool active;
    int start_x, start_y;
    int end_x, end_y;
    SelectionMode mode;
    
    // Settings
    bool require_full_containment; // vs partial overlap
    bool occlusion_test;           // Only select visible objects
} MarqueeSelection;

void marquee_init(MarqueeSelection *marquee);
void marquee_begin(MarqueeSelection *marquee, int x, int y, SelectionMode mode);
void marquee_update(MarqueeSelection *marquee, int x, int y);
void marquee_end(MarqueeSelection *marquee);

// Test if a screen-space AABB intersects the marquee
bool marquee_test_rect(MarqueeSelection *marquee, int min_x, int min_y, int max_x, int max_y);

// Rendering
void marquee_render(MarqueeSelection *marquee, int viewport_width, int viewport_height);
