// Frame Graph - Comprehensive Validation
// Implements Phase 1: Validation with detailed error reporting
#include "rendering/frame_graph/frame_graph_internal.h"
#include "core/logger.h"
#include <string.h>
#include <stdio.h>

// Comprehensive graph validation with detailed error messages
bool rg_validate_graph(RenderGraph *rg, char *error_buffer, u32 buffer_size) {
    if (!rg) {
        if (error_buffer && buffer_size > 0) {
            snprintf(error_buffer, buffer_size, "Null render graph");
        }
        return false;
    }
    
    bool valid = true;
    char temp_error[256];
    
    // Check 1: Resources used but never written
    for (u32 i = 1; i < rg->resource_count; i++) {
        RGResource *res = &rg->resources[i];
        
        if (res->is_imported) continue;  // Imported resources don't need writes
        
        bool has_writer = false;
        for (u32 p = 1; p < rg->pass_count; p++) {
            RGPass *pass = &rg->passes[p];
            for (u32 w = 0; w < pass->write_count; w++) {
                if (pass->writes[w].id == i) {
                    has_writer = true;
                    break;
                }
            }
            if (has_writer) break;
        }
        
        if (!has_writer) {
            snprintf(temp_error, sizeof(temp_error), 
                    "Resource '%s' is used but never written", res->name);
            if (error_buffer && buffer_size > 0) {
                strncat(error_buffer, temp_error, buffer_size - strlen(error_buffer) - 1);
                strncat(error_buffer, "; ", buffer_size - strlen(error_buffer) - 1);
            }
            LOG_ERROR("%s", temp_error);
            valid = false;
        }
    }
    
    // Check 2: Write-after-write without intervening read (potential bug)
    for (u32 i = 1; i < rg->resource_count; i++) {
        u32 last_writer = 0xFFFFFFFF;
        bool had_read_since_write = false;
        
        // Scan passes in execution order (if compiled)
        u32 pass_order_count = rg->is_compiled ? rg->execution_count : rg->pass_count - 1;
        
        for (u32 order_idx = 0; order_idx < pass_order_count; order_idx++) {
            u32 p = rg->is_compiled ? rg->execution_order[order_idx] : (order_idx + 1);
            RGPass *pass = &rg->passes[p];
            
            // Check reads
            for (u32 r = 0; r < pass->read_count; r++) {
                if (pass->reads[r].id == i) {
                    had_read_since_write = true;
                }
            }
            
            // Check writes
            for (u32 w = 0; w < pass->write_count; w++) {
                if (pass->writes[w].id == i) {
                    if (last_writer != 0xFFFFFFFF && !had_read_since_write) {
                        snprintf(temp_error, sizeof(temp_error),
                                "Resource '%s': Write-after-write hazard (Pass '%s' -> '%s' with no read)",
                                rg->resources[i].name,
                                rg->passes[last_writer].name,
                                pass->name);
                        if (error_buffer && buffer_size > 0) {
                            strncat(error_buffer, temp_error, buffer_size - strlen(error_buffer) - 1);
                            strncat(error_buffer, "; ", buffer_size - strlen(error_buffer) - 1);
                        }
                        LOG_WARN("%s", temp_error);
                    }
                    last_writer = p;
                    had_read_since_write = false;
                }
            }
        }
    }
    
    // Check 3: Validate format compatibility for aliased resources
    if (rg->resource_pool) {
        // Will be more useful in Phase 2 when aliasing is more sophisticated
        // For now, basic type checking is done in resource_pool.c
    }
    
    return valid;
}
