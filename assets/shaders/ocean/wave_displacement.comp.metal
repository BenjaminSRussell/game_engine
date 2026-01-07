//
// Wave Displacement Assembly
// Combines IFFT outputs into final displacement map
//

#include <metal_stdlib>
#include "fft_utils.metal"

using namespace metal;

kernel void wave_displacement_assembly(
    texture2d<float, access::read> ifft_height [[texture(0)]],     // IFFT output (height)
    texture2d<float, access::read> ifft_disp_x [[texture(1)]],     // IFFT output (Dx)
    texture2d<float, access::read> ifft_disp_z [[texture(2)]],     // IFFT output (Dz)
    texture2d<float, access::write> displacement_map [[texture(3)]], // Output: (Dx, height, Dz)
    constant SpectrumParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 size = uint2(displacement_map.get_width(), displacement_map.get_height());
    
    if (gid.x >= size.x || gid.y >= size.y) {
        return;
    }
    
    // Read IFFT outputs (real parts only, as IFFT of real spectrum)
    float height = ifft_height.read(gid).r;
    float disp_x = ifft_disp_x.read(gid).r;
    float disp_z = ifft_disp_z.read(gid).r;
    
    // Apply choppy wave scaling to horizontal displacement
    disp_x *= params.choppy_factor;
    disp_z *= params.choppy_factor;
    
    // Clamp extreme displacements to avoid artifacts
    float max_disp = params.tile_size * 0.25f;  // Max 25% of tile size
    disp_x = clamp(disp_x, -max_disp, max_disp);
    disp_z = clamp(disp_z, -max_disp, max_disp);
    
    // Write combined displacement
    // Format: RGB32F where R=Dx, G=height, B=Dz
    displacement_map.write(float4(disp_x, height, disp_z, 1.0f), gid);
}
