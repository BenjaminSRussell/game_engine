//
// Wave Foam Generation
// Computes foam factor from displacement Jacobian determinant
//

#include <metal_stdlib>
using namespace metal;

kernel void wave_foam_generate(
    texture2d<float, access::read> displacement_map [[texture(0)]],   // (Dx, height, Dz)
    texture2d<float, access::read_write> normal_map [[texture(1)]],   // (Nx, Ny, Nz, foam) - update alpha
    constant float& tile_size [[buffer(0)]],
    constant float& foam_decay [[buffer(1)]],
    constant float& delta_time [[buffer(2)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 size = uint2(displacement_map.get_width(), displacement_map.get_height());
    
    if (gid.x >= size.x || gid.y >= size.y) {
        return;
    }
    
    int x = int(gid.x);
    int y = int(gid.y);
    int N = int(size.x);
    
    // Wrap coordinates
    int x_left = (x - 1 + N) % N;
    int x_right = (x + 1) % N;
    int y_down = (y - 1 + N) % N;
    int y_up = (y + 1) % N;
    
    // Read displacements
    float3 disp_center = displacement_map.read(uint2(x, y)).rgb;
    float3 disp_left = displacement_map.read(uint2(x_left, y)).rgb;
    float3 disp_right = displacement_map.read(uint2(x_right, y)).rgb;
    float3 disp_down = displacement_map.read(uint2(x, y_down)).rgb;
    float3 disp_up = displacement_map.read(uint2(x, y_up)).rgb;
    
    float Dx_center = disp_center.r;
    float Dz_center = disp_center.b;
    
    float Dx_left = disp_left.r;
    float Dx_right = disp_right.r;
    float Dz_down = disp_down.b;
    float Dz_up = disp_up.b;
    
    // Grid spacing
    float dx = tile_size / float(N);
    
    // Compute Jacobian matrix of displacement field
    // J = (x + Dx, z + Dz) / (x, z)
    //   = | 1 + Dx/x,    Dx/z |
    //     | Dz/x,    1 + Dz/z |
    
    float dDx_dx = (Dx_right - Dx_left) / (2.0f * dx);
    float dDz_dz = (Dz_up - Dz_down) / (2.0f * dx);
    
    // For simplicity, assume cross terms are small (Dx/z  0, Dz/x  0)
    // This is valid for deep water waves
    float J_00 = 1.0f + dDx_dx;
    float J_11 = 1.0f + dDz_dz;
    
    // Determinant: Det(J)  J_00 * J_11
    float det_J = J_00 * J_11;
    
    // If Det(J) < 0, wave is breaking (folding)  generate foam
    // Map negative determinant to foam intensity
    float foam_generation = 0.0f;
    
    if (det_J < 0.0f) {
        // Wave breaking detected
        foam_generation = -det_J;  // Higher negative = more foam
        foam_generation = clamp(foam_generation, 0.0f, 1.0f);
    }
    
    // Read existing foam value
    float4 normal_foam = normal_map.read(gid);
    float existing_foam = normal_foam.a;
    
    // Accumulate foam with decay
    // foam(t+dt) = max(foam(t) * exp(-decay*dt), foam_generation)
    float decayed_foam = existing_foam * exp(-foam_decay * delta_time);
    float new_foam = max(decayed_foam, foam_generation);
    
    // Clamp to [0, 1]
    new_foam = clamp(new_foam, 0.0f, 1.0f);
    
    // Write back (preserve normal, update foam in alpha)
    normal_foam.a = new_foam;
    normal_map.write(normal_foam, gid);
}
