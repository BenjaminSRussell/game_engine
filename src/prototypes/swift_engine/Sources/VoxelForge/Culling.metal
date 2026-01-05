#include <metal_stdlib>
#include "ShaderTypes.h"

using namespace metal;

struct Frustum {
    float4 planes[6];
};

kernel void frustumCull(uint instanceID [[thread_position_in_grid]],
                        constant InstanceData *allInstances [[buffer(0)]],
                        device InstanceData *visibleInstances [[buffer(1)]],
                        device atomic_uint &visibleCount [[buffer(2)]],
                        constant Frustum &frustum [[buffer(3)]]) {
    
    InstanceData instance = allInstances[instanceID];
    float3 pos = instance.instancePosition;
    float radius = 0.5f; // Half-size of the voxel
    
    bool inside = true;
    for (int i = 0; i < 6; i++) {
        float4 plane = frustum.planes[i];
        float distance = dot(float4(pos, 1.0), plane);
        
        if (distance < -radius) {
            inside = false;
            break;
        }
    }
    
    if (inside) {
        uint index = atomic_fetch_add_explicit(&visibleCount, 1, memory_order_relaxed);
        visibleInstances[index] = instance;
    }
}
