// Barrier merging - append to end of compile_execute.c

// Phase 1: Barrier merging optimization  
// Merges adjacent barriers to the same resource to reduce overhead
static void merge_adjacent_barriers(RenderGraph *rg) {
    if (!rg || rg->barrier_count <= 1) return;
    
    // Greedy merging: scan array, merge consecutive barriers to same resource
    u32 write_idx = 0;
    
    for (u32 read_idx = 0; read_idx < rg->barrier_count; read_idx++) {
        bool merged = false;
        
        // Try to merge with previous barrier
        if (write_idx > 0) {
            RGBarrier *prev = &rg->barriers[write_idx - 1];
            RGBarrier *curr = &rg->barriers[read_idx];
            
            // Can merge if same resource and compatible stages
            if (prev->resource.id == curr->resource.id &&
                prev->dst_stage == curr->src_stage) {
                // Merge: chain the transitions
                prev->new_state = curr->new_state;
                prev->dst_stage = curr->dst_stage;
                merged = true;
            }
        }
        
        if (!merged) {
            // Keep this barrier
            if (read_idx != write_idx) {
                rg->barriers[write_idx] = rg->barriers[read_idx];
            }
            write_idx++;
        }
    }
    
    rg->barrier_count = write_idx;
}
