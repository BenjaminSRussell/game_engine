/**
 * =================================================================================================
 *                          TEST: NORMAL ENCODING ACCURACY
 * =================================================================================================
 */

#include <core/logger/logger.h>
#include <core/math/math.h> // Assuming math library for vectors
#include <math.h>
#include <stdlib.h>

// Re-implement the GLSL encoding logic in C for CPU-side verification

// --- Vector Types ---
typedef struct { float x, y, z; } vec3;
typedef struct { float x, y; } vec2;

// --- Helper Functions ---
static float sign(float x) { return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); }
static float abs_f(float x) { return (x >= 0.0f) ? x : -x; }

static vec3 vec3_normalize(vec3 v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < 0.0001f) return (vec3){0,0,0};
    return (vec3){v.x/len, v.y/len, v.z/len};
}

static float vec3_dot(vec3 a, vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

// --- Encoding Logic (Matches normal_encoding.glsl) ---

// Octahedral Encode
vec2 octEncode(vec3 n) {
    float l1 = abs_f(n.x) + abs_f(n.y) + abs_f(n.z);
    n.x /= l1; n.y /= l1; n.z /= l1;
    
    if (n.z < 0.0f) {
        float tempX = (1.0f - abs_f(n.y)) * sign(n.x);
        float tempY = (1.0f - abs_f(n.x)) * sign(n.y);
        n.x = tempX;
        n.y = tempY;
    }
    
    return (vec2){n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f};
}

// Octahedral Decode
vec3 octDecode(vec2 enc) {
    enc.x = enc.x * 2.0f - 1.0f;
    enc.y = enc.y * 2.0f - 1.0f;
    
    vec3 n;
    n.x = enc.x;
    n.y = enc.y;
    n.z = 1.0f - abs_f(enc.x) - abs_f(enc.y);
    
    if (n.z < 0.0f) {
        float tempX = (1.0f - abs_f(n.y)) * sign(n.x);
        float tempY = (1.0f - abs_f(n.x)) * sign(n.y);
        n.x = tempX;
        n.y = tempY;
    }
    
    return vec3_normalize(n);
}

// --- Test Suite ---

int main() {
    LOG_INFO("Starting Normal Encoding Test...");
    
    int num_samples = 1000;
    float total_error = 0.0f;
    float max_error = 0.0f;
    
    // Seed random (constant seed for reproducibility)
    srand(12345);
    
    for (int i = 0; i < num_samples; i++) {
        // Generate random test vector on sphere
        vec3 normal;
        normal.x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        normal.y = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        normal.z = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        normal = vec3_normalize(normal);
        
        // Encode
        vec2 encoded = octEncode(normal);
        
        // Simulating quantization to 16-bit float (semi-precision) would be ideal here
        // but for now we test algorithmic correctness
        
        // Decode
        vec3 decoded = octDecode(encoded);
        
        // Check accuracy (Dot product should be close to 1.0)
        float dot = vec3_dot(normal, decoded);
        // Clamp to prevent precision issues
        if (dot > 1.0f) dot = 1.0f; 
        if (dot < -1.0f) dot = -1.0f;
        
        // Angle error in radians
        float angle_error = acosf(dot);
        
        if (angle_error > max_error) max_error = angle_error;
        total_error += angle_error;
    }
    
    float avg_error = total_error / num_samples;
    LOG_INFO("Tested %d samples.", num_samples);
    LOG_INFO("Average Angular Error: %.6f radians", avg_error);
    LOG_INFO("Max Angular Error: %.6f radians", max_error);
    
    // Allow small epsilon for floating point error
    if (avg_error < 0.001f && max_error < 0.01f) {
        LOG_INFO("SUCCESS: Normal encoding/decoding is accurate.");
        return 0;
    } else {
        LOG_ERROR("FAILURE: Normal encoding error too high.");
        return 1;
    }
}
