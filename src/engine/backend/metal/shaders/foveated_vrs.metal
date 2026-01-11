#include <metal_stdlib>
using namespace metal;

struct VRSParams {
    float2 focus_center;
    float inner_radius;
    float outer_radius;
};

kernel void generate_foveated_vrs(
    texture2d<uint, access::write> rate_map [[texture(0)]],
    constant VRSParams &params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]])
{
    if (gid.x >= rate_map.get_width() || gid.y >= rate_map.get_height()) {
        return;
    }

    float2 uv = float2(gid) / float2(rate_map.get_width(), rate_map.get_height());
    float dist = distance(uv, params.focus_center);

    uint rate;
    if (dist < params.inner_radius) {
        rate = 0; // MTLShadingRate1x1
    } else if (dist < params.outer_radius) {
        rate = 1; // MTLShadingRate2x2 (or custom mapping)
    } else {
        rate = 4; // MTLShadingRate4x4
    }

    rate_map.write(rate, gid);
}
