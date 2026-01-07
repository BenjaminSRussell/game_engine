#include <metal_stdlib>
#include "../core/gpu_types.metal"

using namespace metal;

/* ============================================================================
 * GPU CULLING COMPUTE SHADERS
 * ============================================================================
 * Frustum culling, LOD selection, and compaction kernels
 * ============================================================================ */

/* ============================================================================
 * FRUSTUM CULLING KERNEL
 * ============================================================================
 * Tests each instance AABB against view frustum
 * Sets visible_flag in culling results
 */
kernel void cull_frustum(
    device GPUInstanceData* instances [[buffer(0)]],
    device CullingResult* results [[buffer(1)]],
    constant FrustumData& frustum [[buffer(2)]],
    constant uint& instance_count [[buffer(3)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid >= instance_count) {
        return;
    }

    // Get instance data
    const GPUInstanceData instance = instances[tid];

    // Extract bounds from instance
    float3 bounds_min = instance.bounds_min.xyz;
    float3 bounds_max = instance.bounds_max.xyz;

    // Calculate AABB center and extents in world space
    // TODO: Transform bounds by instance transform matrix
    float3 center = (bounds_min + bounds_max) * 0.5f;
    float3 extents = (bounds_max - bounds_min) * 0.5f;

    // Test AABB against frustum
    uint visible = aabb_in_frustum(center, extents, frustum) ? 1u : 0u;

    // Write culling result
    results[tid].visible_flag = visible;
    results[tid].lod_selected = 0u;  // Will be set by LOD selection kernel
    results[tid].draw_index = tid;   // Will be updated during compaction
}

/* ============================================================================
 * LOD SELECTION KERNEL
 * ============================================================================
 * Selects LOD level based on distance from camera
 * Updates lod_selected in culling results
 */
kernel void select_lod(
    device GPUInstanceData* instances [[buffer(0)]],
    device CullingResult* results [[buffer(1)]],
    constant LODSelectionData& lod_data [[buffer(2)]],
    constant float3& camera_pos [[buffer(3)]],
    constant uint& instance_count [[buffer(4)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid >= instance_count) {
        return;
    }

    // Skip if culled
    if (results[tid].visible_flag == 0u) {
        return;
    }

    // Get instance bounds
    const GPUInstanceData instance = instances[tid];
    float3 bounds_min = instance.bounds_min.xyz;
    float3 bounds_max = instance.bounds_max.xyz;
    float3 center = (bounds_min + bounds_max) * 0.5f;

    // TODO: Transform center by instance transform
    // For now, assume center is in world space

    // Calculate distance to camera
    float3 to_camera = camera_pos - center;
    float distance = length(to_camera);

    // Select LOD
    uint current_lod = results[tid].lod_selected;
    uint new_lod = select_lod_with_hysteresis(distance, current_lod, lod_data);

    results[tid].lod_selected = new_lod;
}

/* ============================================================================
 * VISIBILITY FLAGS KERNEL (OPTIONAL)
 * ============================================================================
 * Can apply additional culling (occlusion, distance fade, etc.)
 * Updates visible_flag in culling results
 */
kernel void cull_additional(
    device CullingResult* results [[buffer(0)]],
    constant uint& instance_count [[buffer(1)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid >= instance_count) {
        return;
    }

    // Can add additional culling here
    // e.g., occlusion, shadow map culling, distance culling
    // For now, keep existing visibility
}

/* ============================================================================
 * COMPACTION KERNEL
 * ============================================================================
 * Compacts visible instances and populates indirect draw args
 * Uses atomic operations to build dense visible list
 */
kernel void compact_visible(
    device CullingResult* results [[buffer(0)]],
    device IndirectDrawArgs* indirect_args [[buffer(1)]],
    device uint* visible_indices [[buffer(2)]],
    device atomic_uint* visible_count [[buffer(3)]],
    constant uint& instance_count [[buffer(4)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid >= instance_count) {
        return;
    }

    const CullingResult result = results[tid];

    // If visible, append to visible list
    if (result.visible_flag != 0u) {
        // Atomically increment visible count and get position
        uint visible_idx = atomic_fetch_add_explicit(
            &visible_count[0], 1u, memory_order_relaxed);

        // Store visible instance index
        visible_indices[visible_idx] = tid;

        // Store draw index for later use
        results[tid].draw_index = visible_idx;
    }
}

/* ============================================================================
 * BATCH BUILDER KERNEL
 * ============================================================================
 * Groups visible instances into batches and generates indirect draw args
 * Assumes instances are sorted by material/mesh before culling
 */
kernel void build_batches(
    device GPUInstanceData* instances [[buffer(0)]],
    device CullingResult* results [[buffer(1)]],
    device uint* visible_indices [[buffer(2)]],
    device IndirectDrawArgs* indirect_args [[buffer(3)]],
    device atomic_uint* batch_count [[buffer(4)]],
    constant uint& visible_count [[buffer(5)]],
    constant uint& instance_count [[buffer(6)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid >= visible_count) {
        return;
    }

    // Get visible instance index
    uint instance_idx = visible_indices[tid];
    const GPUInstanceData instance = instances[instance_idx];

    // TODO: Implement batch building logic
    // This would:
    // 1. Group consecutive instances with same material/mesh
    // 2. Generate one indirect draw args per batch
    // 3. Populate indexCount, indexStart, baseInstance
    // 4. Atomically allocate batch indices
}

/* ============================================================================
 * RESET COUNTERS KERNEL
 * ============================================================================
 * Resets GPU counters for next frame
 */
kernel void reset_counters(
    device atomic_uint* visible_count [[buffer(0)]],
    device atomic_uint* draw_count [[buffer(1)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid == 0) {
        atomic_store_explicit(&visible_count[0], 0u, memory_order_relaxed);
        atomic_store_explicit(&draw_count[0], 0u, memory_order_relaxed);
    }
}

/* ============================================================================
 * INDIRECT ARGS POPULATION KERNEL
 * ============================================================================
 * Fills in final indirect draw arguments from batch information
 */
kernel void populate_indirect_args(
    device IndirectDrawArgs* indirect_args [[buffer(0)]],
    device uint* batch_instance_counts [[buffer(1)]],
    device uint* batch_index_offsets [[buffer(2)]],
    device uint* batch_instance_offsets [[buffer(3)]],
    constant uint& batch_count [[buffer(4)]],
    uint tid [[thread_position_in_grid]])
{
    if (tid >= batch_count) {
        return;
    }

    IndirectDrawArgs args;
    args.indexCount = batch_instance_counts[tid] * 6;  // Assuming 6 indices per instance (2 triangles)
    args.instanceCount = 1;  // We're using instancing with one "instance" per draw
    args.indexStart = batch_index_offsets[tid];
    args.baseVertex = 0;
    args.baseInstance = batch_instance_offsets[tid];

    indirect_args[tid] = args;
}
