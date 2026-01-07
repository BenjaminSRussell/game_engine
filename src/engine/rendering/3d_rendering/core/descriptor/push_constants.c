#include "push_constants.h"
#include <string.h>

bool push_constants_validate(const push_constant_layout_t* layout, uint32_t offset, uint32_t size) {
    if (!layout) return false;
    if (size == 0 || (offset + size) > MAX_PUSH_CONSTANT_SIZE) return false;

    // Check if the update falls within any defined range
    for (uint32_t i = 0; i < layout->range_count; ++i) {
        const push_constant_range_t* range = &layout->ranges[i];
        
        // Check for containment or overlap that is allowed
        // Usually we want precise matching or containment
        if (offset >= range->offset && (offset + size) <= (range->offset + range->size)) {
            return true;
        }
    }
    
    return false;
}
