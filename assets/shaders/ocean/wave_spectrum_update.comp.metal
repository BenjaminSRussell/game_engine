//
// Wave Spectrum Update
// Computes time-varying spectrum h(k,t) from initial spectrum h0(k)
//

#include <metal_stdlib>
#include "fft_utils.metal"

using namespace metal;

kernel void wave_spectrum_update(
    texture2d<float, access::read> h0_texture [[texture(0)]],       // h0(k)
    texture2d<float, access::read> h0_conj_texture [[texture(1)]],  // h0*(-k)
    texture2d<float, access::read> omega_texture [[texture(2)]],    // (k)
    texture2d<float, access::write> ht_height [[texture(3)]],       // Output: height spectrum
    texture2d<float, access::write> ht_disp_x [[texture(4)]],       // Output: Dx spectrum
    texture2d<float, access::write> ht_disp_z [[texture(5)]],       // Output: Dz spectrum
    constant SpectrumParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 size = uint2(ht_height.get_width(), ht_height.get_height());
    
    if (gid.x >= size.x || gid.y >= size.y) {
        return;
    }
    
    // Read h0(k) and h0*(-k)
    float2 h0_k = h0_texture.read(gid).rg;
    float2 h0_conj_k = h0_conj_texture.read(gid).rg;
    
    Complex h0 = complex_from_float2(h0_k);
    Complex h0_conj = complex_from_float2(h0_conj_k);
    
    // Read dispersion (k)
    float omega = omega_texture.read(gid).r;
    
    // Compute h(k,t) = h0(k)*exp(it) + h0*(-k)*exp(-it)
    Complex ht = compute_ht(h0, h0_conj, omega, params.time);
    
    // Write height spectrum
    ht_height.write(float4(complex_to_float2(ht), 0.0f, 1.0f), gid);
    
    // Compute wave vector k from grid position
    int half_res = int(size.x) / 2;
    int n = int(gid.x) - half_res;
    int m = int(gid.y) - half_res;
    
    float kx = (2.0f * M_PI_F / params.tile_size) * float(n);
    float kz = (2.0f * M_PI_F / params.tile_size) * float(m);
    
    float k_mag = sqrt(kx * kx + kz * kz);
    
    // Compute displacement spectra
    // Dx(k) = -i * (kx/|k|) * h(k,t)
    // Dz(k) = -i * (kz/|k|) * h(k,t)
    
    Complex disp_x, disp_z;
    
    if (k_mag > EPSILON) {
        float kx_norm = kx / k_mag;
        float kz_norm = kz / k_mag;
        
        // Multiply by -i means: (a + bi) * (-i) = b - ai
        // So: -i * h(k,t) = imag(ht) - i * real(ht)
        Complex neg_i_ht = Complex(ht.imag, -ht.real);
        
        disp_x = Complex(kx_norm * neg_i_ht.real, kx_norm * neg_i_ht.imag);
        disp_z = Complex(kz_norm * neg_i_ht.real, kz_norm * neg_i_ht.imag);
    } else {
        disp_x = Complex(0.0f, 0.0f);
        disp_z = Complex(0.0f, 0.0f);
    }
    
    // Write displacement spectra
    ht_disp_x.write(float4(complex_to_float2(disp_x), 0.0f, 1.0f), gid);
    ht_disp_z.write(float4(complex_to_float2(disp_z), 0.0f, 1.0f), gid);
}
