//
// FFT Utilities for Ocean Simulation
// Shared functions and structures for FFT compute shaders
//

#include <metal_stdlib>
using namespace metal;

// =============================================================================
// Complex Number Operations
// =============================================================================

struct Complex {
    float real;
    float imag;
    
    Complex(float r = 0.0f, float i = 0.0f) : real(r), imag(i) {}
};

// Complex addition
inline Complex complex_add(Complex a, Complex b) {
    return Complex(a.real + b.real, a.imag + b.imag);
}

// Complex subtraction
inline Complex complex_sub(Complex a, Complex b) {
    return Complex(a.real - b.real, a.imag - b.imag);
}

// Complex multiplication
inline Complex complex_mul(Complex a, Complex b) {
    return Complex(
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real
    );
}

// Complex conjugate
inline Complex complex_conj(Complex a) {
    return Complex(a.real, -a.imag);
}

// Complex magnitude squared
inline float complex_mag_sq(Complex a) {
    return a.real * a.real + a.imag * a.imag;
}

// Complex magnitude
inline float complex_mag(Complex a) {
    return sqrt(complex_mag_sq(a));
}

// Complex from float2
inline Complex complex_from_float2(float2 v) {
    return Complex(v.x, v.y);
}

// Complex to float2
inline float2 complex_to_float2(Complex c) {
    return float2(c.real, c.imag);
}

// =============================================================================
// FFT Utilities
// =============================================================================

// Compute twiddle factor W_N^k = exp(-2ik/N)
inline Complex twiddle_factor(int k, int N) {
    float angle = -2.0f * M_PI_F * float(k) / float(N);
    return Complex(cos(angle), sin(angle));
}

// Bit reversal for FFT input reordering
inline uint bit_reverse(uint x, uint log2n) {
    uint result = 0;
    for (uint i = 0; i < log2n; ++i) {
        result <<= 1;
        result |= (x & 1);
        x >>= 1;
    }
    return result;
}

// Calculate log2 of a power-of-2 number
inline uint log2_uint(uint x) {
    uint log = 0;
    while (x >>= 1) ++log;
    return log;
}

// Butterfly operation for radix-2 FFT
// Given two complex numbers and a twiddle factor, performs:
// out_even = even + twiddle * odd
// out_odd = even - twiddle * odd
inline void butterfly_radix2(
    Complex even,
    Complex odd,
    Complex twiddle,
    thread Complex& out_even,
    thread Complex& out_odd
) {
    Complex twiddle_odd = complex_mul(twiddle, odd);
    out_even = complex_add(even, twiddle_odd);
    out_odd = complex_sub(even, twiddle_odd);
}

// =============================================================================
// FFT Parameters (uniform buffer structure)
// =============================================================================

struct FFTParams {
    uint resolution;          // FFT size (N)
    uint stage;              // Current FFT stage (0 to log2(N)-1)
    uint direction;          // 0 = forward, 1 = inverse
    float normalization;     // 1.0 for forward, 1/N for inverse
};

// =============================================================================
// Spectrum Parameters (uniform buffer structure)
// =============================================================================

struct SpectrumParams {
    // Wind
    float wind_speed;
    float wind_direction_x;
    float wind_direction_z;
    float _padding0;
    
    // Wave parameters
    float amplitude;
    float wave_suppression;
    float fetch;
    float choppy_factor;
    
    // Physics
    float gravity;
    float time;
    float tile_size;
    float _padding1;
};

// =============================================================================
// Helper Functions for Spectrum Update
// =============================================================================

// Compute h(k,t) = h0(k) * exp(it) + h0*(-k) * exp(-it)
inline Complex compute_ht(
    Complex h0_k,
    Complex h0_conj_k,
    float omega,
    float time
) {
    float omega_t = omega * time;
    
    // exp(it) = cos(t) + i*sin(t)
    Complex exp_plus = Complex(cos(omega_t), sin(omega_t));
    
    // exp(-it) = cos(t) - i*sin(t)
    Complex exp_minus = Complex(cos(omega_t), -sin(omega_t));
    
    // h(k,t) = h0(k) * exp(it) + h0*(-k) * exp(-it)
    Complex term1 = complex_mul(h0_k, exp_plus);
    Complex term2 = complex_mul(h0_conj_k, exp_minus);
    
    return complex_add(term1, term2);
}

// =============================================================================
// Constants
// =============================================================================

constant float EPSILON = 1e-8f;
