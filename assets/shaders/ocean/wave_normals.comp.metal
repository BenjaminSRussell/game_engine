//
// Wave Normal Map Generation
// Computes normals from height displacement field using finite differences
//

#include <metal_stdlib>
using namespace metal;

kernel void wave_normals_generate(
    texture2d<float, access::read> displacement_map [[texture(0)]],  // (Dx, height, Dz)
    texture2d<float, access::write> normal_map [[texture(1)]],       // Output: (Nx, Ny, Nz, _)
    constant float& tile_size [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 size = uint2(displacement_map.get_width(), displacement_map.get_height());
    
    if (gid.x >= size.x || gid.y >= size.y) {
        return;
    }
    
    // Sample neighboring heights for finite differences
    // h(x-1, z), h(x+1, z), h(x, z-1), h(x, z+1)
    
    int x = int(gid.x);
    int y = int(gid.y);
    int N = int(size.x);
    
    // Wrap coordinates (periodic boundary)
    int x_left = (x - 1 + N) % N;
    int x_right = (x + 1) % N;
    int y_down = (y - 1 + N) % N;
    int y_up = (y + 1) % N;
    
    // Read heights (G channel contains height displacement)
    float h_center = displacement_map.read(uint2(x, y)).g;
    float h_left = displacement_map.read(uint2(x_left, y)).g;
    float h_right = displacement_map.read(uint2(x_right, y)).g;
    float h_down = displacement_map.read(uint2(x, y_down)).g;
    float h_up = displacement_map.read(uint2(x, y_up)).g;
    
    // Compute grid spacing
    float dx = tile_size / float(N);
    
    // Compute gradients using central differences
    // dh/dx  (h(x+1) - h(x-1)) / (2*dx)
    // dh/dz  (h(z+1) - h(z-1)) / (2*dz)
    
    float dh_dx = (h_right - h_left) / (2.0f * dx);
    float dh_dz = (h_up - h_down) / (2.0f * dx);
    
    // Normal is cross product of tangent vectors:
    // tangent_x = (1, dh/dx, 0)
    // tangent_z = (0, dh/dz, 1)
    // normal = tangent_x  tangent_z = (-dh/dx, 1, -dh/dz)
    
    float3 normal = float3(-dh_dx, 1.0f, -dh_dz);
    
    // Normalize
    normal = normalize(normal);
    
    // Pack normal into texture (map from [-1,1] to [0,1] or keep as is)
    // Most renderers expect normals in [-1,1] range in world space
    // We'll store them directly
    
    normal_map.write(float4(normal, 1.0f), gid);
}
