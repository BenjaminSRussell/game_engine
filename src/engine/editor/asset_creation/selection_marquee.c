/**
 * =================================================================================================
 *                          SELECTION MARQUEE
 * =================================================================================================
 */

#include "selection_marquee.h"
#include <string.h>

void marquee_init(MarqueeSelection *marquee) {
    memset(marquee, 0, sizeof(MarqueeSelection));
    marquee->require_full_containment = false;
    marquee->occlusion_test = false;
}

void marquee_begin(MarqueeSelection *marquee, int x, int y, SelectionMode mode) {
    marquee->active = true;
    marquee->start_x = x;
    marquee->start_y = y;
    marquee->end_x = x;
    marquee->end_y = y;
    marquee->mode = mode;
}

void marquee_update(MarqueeSelection *marquee, int x, int y) {
    if (!marquee->active) return;
    marquee->end_x = x;
    marquee->end_y = y;
}

void marquee_end(MarqueeSelection *marquee) {
    marquee->active = false;
}

bool marquee_test_rect(MarqueeSelection *marquee, int min_x, int min_y, int max_x, int max_y) {
    int mar_min_x = marquee->start_x < marquee->end_x ? marquee->start_x : marquee->end_x;
    int mar_max_x = marquee->start_x > marquee->end_x ? marquee->start_x : marquee->end_x;
    int mar_min_y = marquee->start_y < marquee->end_y ? marquee->start_y : marquee->end_y;
    int mar_max_y = marquee->start_y > marquee->end_y ? marquee->start_y : marquee->end_y;
    
    if (marquee->require_full_containment) {
        // Object must be fully inside marquee
        return (min_x >= mar_min_x && max_x <= mar_max_x &&
                min_y >= mar_min_y && max_y <= mar_max_y);
    } else {
        // Partial overlap is ok
        return !(max_x < mar_min_x || min_x > mar_max_x ||
                 max_y < mar_min_y || min_y > mar_max_y);
    }
}

void marquee_render(MarqueeSelection *marquee, int viewport_width, int viewport_height) {
    if (!marquee->active) return;
    
    // Render semi-transparent selection box
    // draw_rect_outline(marquee->start_x, marquee->start_y, marquee->end_x, marquee->end_y, color);
    // draw_rect_filled(marquee->start_x, marquee->start_y, marquee->end_x, marquee->end_y, color_translucent);
}
