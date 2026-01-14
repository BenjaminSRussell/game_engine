/*
 * Post-Processing Shader Implementation Summary
 * 
 * This directory contains comprehensive shader implementations for all post-processing effects
 * required by the Jules Engine. Each shader is optimized for performance and visual quality.
 * 
 * Shader Files Created:
 * 
 * 1. post_process.vert
 *    - Full-screen quad vertex shader
 *    - Passes through texture coordinates and view rays
 *    - Supports depth reconstruction for effects like DoF and SSR
 * 
 * 2. tone_mapping.frag
 *    - Multiple tone mapping operators (Linear, Reinhard, ACES, Uncharted2, AGX)
 *    - Exposure, gamma, contrast, and brightness controls
 *    - HDR to LDR conversion with filmic curves
 * 
 * 3. bloom.frag
 *    - High-quality bloom with threshold and knee controls
 *    - Multiple iteration support for pyramid generation
 *    - Lens dirt integration for cinematic effects
 *    - Smooth bloom extraction with configurable intensity
 * 
 * 4. depth_of_field.frag
 *    - Physically-based depth of field with circle of confusion
 *    - Multiple bokeh shapes (circular, hexagonal, octagonal)
 *    - Auto-focus support with configurable focus distance
 *    - Smooth transitions with edge fade
 * 
 * 5. motion_blur.frag
 *    - Per-pixel motion blur using velocity vectors
 *    - Catmull-Rom interpolation for high-quality sampling
 *    - Configurable sample count and motion scale
 *    - Debug mode for velocity visualization
 * 
 * 6. screen_space_reflections.frag
 *    - High-quality SSR with ray marching
 *    - Temporal filtering for stable reflections
 *    - Edge fade and fresnel calculations
 *    - Roughness-based reflection intensity
 * 
 * 7. ssao.frag
 *    - Screen space ambient occlusion with hemisphere sampling
 *    - Temporal filtering for stable AO
 *    - Configurable radius, bias, and sample count
 *    - Optimized with noise texture rotation
 * 
 * 8. anti_aliasing.frag
 *    - Multiple AA methods (FXAA, TAA, SMAA)
 *    - FXAA with quality presets and edge detection
 *    - TAA with neighborhood clamping and variance filtering
 *    - SMAA with edge detection and blending
 * 
 * 9. color_grading.frag
 *    - LUT-based color grading with 64x64 LUT support
 *    - ASC-CDL color correction (lift, gamma, gain)
 *    - Saturation, hue shift, temperature, and tint controls
 *    - Film emulation with response curves and grain
 * 
 * 10. vignette_effects.frag
 *     - Cinematic vignette with configurable radius and strength
 *     - Chromatic aberration with distance-based intensity
 *     - Film grain with animated noise
 *     - Optional scanlines and color bleeding
 * 
 * 11. gaussian_blur.frag
 *     - Separable Gaussian blur for optimal performance
 *     - Configurable kernel size and sigma
 *     - Used for bloom and other blur effects
 * 
 * 12. downsample.frag
 *     - High-quality downsampling for bloom pyramid
 *     - 13-tap downsampling with proper weights
 *     - Kawase blur option for better bloom
 * 
 * 13. upsample.frag
 *     - High-quality upsampling for bloom reconstruction
 *     - Multiple upsampling methods (bilinear, 4-tap, Catmull-Rom)
 *     - Blending with lower resolution textures
 * 
 * 14. lens_flare.frag
 *     - Realistic lens flare with ghost images
 *     - Halo effects with configurable width
 *     - Star burst patterns with multiple spikes
 *     - Anamorphic lens effects with squeeze
 * 
 * 15. taa_resolve.frag
 *     - Temporal AA history resolve with clamping
 *     - Variance-based filtering for stability
 *     - Color box filtering for better quality
 *     - Disocclusion detection and handling
 * 
 * 16. velocity_buffer.frag
 *     - Motion vector generation for temporal effects
 *     - Depth-based velocity calculation
 *     - Jitter compensation for TAA
 * 
 * 17. post_process.comp
 *     - Compute shader for GPU-accelerated post-processing
 *     - 16x16 workgroup size for optimal performance
 *     - Combined effects for reduced passes
 * 
 * 18. uber_post_process.frag
 *     - Combined post-processing shader for optimal performance
 *     - All effects in a single pass with bitmask control
 *     - Simplified implementations for performance
 *     - Configurable effect combinations
 * 
 * Technical Features:
 * 
 * Performance Optimizations:
 * - Separable filters for Gaussian blur
 * - Temporal filtering for stable effects
 * - LOD-aware sampling for better quality
 * - Compute shader acceleration where applicable
 * - Efficient texture sampling with proper mipmapping
 * 
 * Quality Features:
 * - High-quality filtering (Catmull-Rom, bicubic)
 * - Physically-based calculations (DoF, SSR)
 * - Filmic tone mapping curves
 * - Advanced anti-aliasing methods
 * - Professional color grading tools
 * 
 * Integration:
 * - Compatible with existing post-processing pipeline
 * - Proper uniform naming conventions
 * - Texture binding consistency
 * - Effect enable/disable bitmask support
 * 
 * Usage:
 * 
 * Each shader can be used independently or combined in the uber shader.
 * The uber shader provides optimal performance by combining multiple effects
 * in a single pass, reducing texture bandwidth and improving frame times.
 * 
 * Performance Targets:
 * - 1440p @ 60FPS with all effects enabled
 * - Memory efficient with minimal texture allocations
 * - GPU-friendly with optimal sampling patterns
 * - Scalable quality settings for different hardware
 * 
 * All shaders are production-ready with comprehensive error handling,
 * proper bounds checking, and optimized for modern GPU architectures.
 */
