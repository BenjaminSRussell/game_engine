//
// batch_generation.metal
// GPU-driven batch command generation compute shader
//
// Part of the Geometry subsystem
// Advanced 3D Rendering Engine
//

#include <metal_stdlib>
using namespace metal;

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

// Indirect draw arguments (Metal format)
struct MTLDrawIndexedPrimitivesIndirectArguments {
    uint indexCount;
    uint instanceCount;
    uint indexStart;
    int  baseVertex;
    uint baseInstance;
};

// Mesh descriptor
struct MeshDescriptor {
    uint index_count;
    uint vertex_count;
    uint index_offset;
    uint vertex_offset;
    uint material_id;
    uint padding[3];
};

// Batch statistics
struct BatchStats {
    atomic_uint total_batches;
    atomic_uint total_draw_commands;
    atomic_uint total_instances_rendered;
    atomic_uint state_changes;
};

/* ============================================================================
 * MAIN BATCH GENERATION KERNEL
 * ============================================================================ */

kernel void batch_generation_kernel(
    constant uint* visible_instance_ids [[buffer(0)]],
    constant uint visible_count [[buffer(1)]],
    constant MeshDescriptor* mesh_descriptors [[buffer(2)]],
    constant uint* instance_mesh_ids [[buffer(3)]],
    constant uint* instance_material_ids [[buffer(4)]],
    device MTLDrawIndexedPrimitivesIndirectArguments* draw_commands [[buffer(5)]],
    device atomic_uint* command_count [[buffer(6)]],
    device BatchStats* stats [[buffer(7)]],
    uint gid [[thread_position_in_grid]])
{
    // Each thread processes one visible instance
    if (gid >= visible_count) {
        return;
    }
    
    // Get visible instance ID
    uint instance_id = visible_instance_ids[gid];
    uint mesh_id = instance_mesh_ids[instance_id];
    uint material_id = instance_material_ids[instance_id];
    
    // Get mesh descriptor
    constant MeshDescriptor& mesh = mesh_descriptors[mesh_id];
    
    // For now, create one draw command per instance
    // TODO: Batch instances with same mesh+material
    uint command_idx = atomic_fetch_add_explicit(command_count, 1, memory_order_relaxed);
    
    // Fill draw command
    draw_commands[command_idx].indexCount = mesh.index_count;
    draw_commands[command_idx].instanceCount = 1;
    draw_commands[command_idx].indexStart = mesh.index_offset;
    draw_commands[command_idx].baseVertex = mesh.vertex_offset;
    draw_commands[command_idx].baseInstance = instance_id;
    
    // Update statistics
    atomic_fetch_add_explicit(&stats->total_draw_commands, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&stats->total_instances_rendered, 1, memory_order_relaxed);
}

/* ============================================================================
 * ADVANCED BATCHING KERNEL (Material Sorting)
 * ============================================================================ */

// Sort key: material_id (high 16 bits) + mesh_id (low 16 bits)
kernel void generate_sort_keys(
    constant uint* visible_instance_ids [[buffer(0)]],
    constant uint visible_count [[buffer(1)]],
    constant uint* instance_mesh_ids [[buffer(2)]],
    constant uint* instance_material_ids [[buffer(3)]],
    device uint* sort_keys [[buffer(4)]],
    device uint* sort_values [[buffer(5)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= visible_count) {
        return;
    }
    
    uint instance_id = visible_instance_ids[gid];
    uint mesh_id = instance_mesh_ids[instance_id];
    uint material_id = instance_material_ids[instance_id];
    
    // Create sort key (material first for state change reduction)
    uint sort_key = (material_id << 16) | (mesh_id & 0xFFFF);
    
    sort_keys[gid] = sort_key;
    sort_values[gid] = instance_id;
}

/* ============================================================================
 * COMPACTION KERNEL (Remove gaps after sorting)
 * ============================================================================ */

kernel void compact_batches(
    constant uint* sorted_instance_ids [[buffer(0)]],
    constant uint visible_count [[buffer(1)]],
    constant MeshDescriptor* mesh_descriptors [[buffer(2)]],
    constant uint* instance_mesh_ids [[buffer(3)]],
    constant uint* instance_material_ids [[buffer(4)]],
    device MTLDrawIndexedPrimitivesIndirectArguments* draw_commands [[buffer(5)]],
    device atomic_uint* command_count [[buffer(6)]],
    device BatchStats* stats [[buffer(7)]],
    uint gid [[thread_position_in_grid]],
    uint lid [[thread_position_in_threadgroup]],
    threadgroup uint* shared_batch_start [[threadgroup(0)]],
    threadgroup uint* shared_batch_count [[threadgroup(1)]])
{
    if (gid >= visible_count) {
        return;
    }
    
    uint instance_id = sorted_instance_ids[gid];
    uint mesh_id = instance_mesh_ids[instance_id];
    uint material_id = instance_material_ids[instance_id];
    
    // Check if this starts a new batch
    bool new_batch = (gid == 0);
    
    if (gid > 0) {
        uint prev_instance_id = sorted_instance_ids[gid - 1];
        uint prev_mesh_id = instance_mesh_ids[prev_instance_id];
        uint prev_material_id = instance_material_ids[prev_instance_id];
        
        // Start new batch if mesh or material changed
        new_batch = (mesh_id != prev_mesh_id) || (material_id != prev_material_id);
    }
    
    // If new batch, emit draw command
    if (new_batch) {
        constant MeshDescriptor& mesh = mesh_descriptors[mesh_id];
        
        // Count instances in this batch
        uint batch_instance_count = 1;
        for (uint i = gid + 1; i < visible_count; i++) {
            uint next_instance_id = sorted_instance_ids[i];
            uint next_mesh_id = instance_mesh_ids[next_instance_id];
            uint next_material_id = instance_material_ids[next_instance_id];
            
            if (next_mesh_id != mesh_id || next_material_id != material_id) {
                break;
            }
            batch_instance_count++;
        }
        
        // Allocate draw command
        uint command_idx = atomic_fetch_add_explicit(command_count, 1, memory_order_relaxed);
        
        // Fill draw command
        draw_commands[command_idx].indexCount = mesh.index_count;
        draw_commands[command_idx].instanceCount = batch_instance_count;
        draw_commands[command_idx].indexStart = mesh.index_offset;
        draw_commands[command_idx].baseVertex = mesh.vertex_offset;
        draw_commands[command_idx].baseInstance = instance_id;
        
        // Update statistics
        atomic_fetch_add_explicit(&stats->total_batches, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&stats->total_draw_commands, 1, memory_order_relaxed);
        atomic_fetch_add_explicit(&stats->total_instances_rendered, batch_instance_count, memory_order_relaxed);
    }
}

/* ============================================================================
 * STATS RESET KERNEL
 * ============================================================================ */

kernel void reset_batch_stats(
    device BatchStats* stats [[buffer(0)]],
    device atomic_uint* command_count [[buffer(1)]])
{
    atomic_store_explicit(&stats->total_batches, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->total_draw_commands, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->total_instances_rendered, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->state_changes, 0, memory_order_relaxed);
    
    atomic_store_explicit(command_count, 0, memory_order_relaxed);
}
