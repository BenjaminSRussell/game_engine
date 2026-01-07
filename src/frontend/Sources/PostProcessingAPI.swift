// PostProcessingAPI.swift
// Swift wrapper for Post-Processing Effects

import Foundation

public class PostProcessingAPI {
    public static let shared = PostProcessingAPI()
    private init() {}
    
    // MARK: - Motion Blur
    
    public var motionBlurEnabled: Bool {
        get { return postfx_is_motion_blur_enabled() }
        set { postfx_set_motion_blur_enabled(newValue) }
    }
    
    public var motionBlurStrength: Float {
        get { return postfx_get_motion_blur_strength() }
        set { postfx_set_motion_blur_strength(newValue) }
    }
    
    // MARK: - Chromatic Aberration
    
    public var chromaticAberrationEnabled: Bool {
        get { return postfx_is_chromatic_aberration_enabled() }
        set { postfx_set_chromatic_aberration_enabled(newValue) }
    }
    
    public var chromaticAberrationIntensity: Float {
        get { return postfx_get_chromatic_aberration_intensity() }
        set { postfx_set_chromatic_aberration_intensity(newValue) }
    }
    
    // MARK: - Film Grain
    
    public var filmGrainEnabled: Bool {
        get { return postfx_is_film_grain_enabled() }
        set { postfx_set_film_grain_enabled(newValue) }
    }
    
    public var filmGrainIntensity: Float {
        get { return postfx_get_film_grain_intensity() }
        set { postfx_set_film_grain_intensity(newValue) }
    }
    
    // MARK: - Vignette
    
    public var vignetteEnabled: Bool {
        get { return postfx_is_vignette_enabled() }
        set { postfx_set_vignette_enabled(newValue) }
    }
    
    public var vignetteIntensity: Float {
        get { return postfx_get_vignette_intensity() }
        set { postfx_set_vignette_intensity(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("postfx_set_motion_blur_enabled")
private func postfx_set_motion_blur_enabled(_ enabled: Bool)

@_silgen_name("postfx_is_motion_blur_enabled")
private func postfx_is_motion_blur_enabled() -> Bool

@_silgen_name("postfx_set_motion_blur_strength")
private func postfx_set_motion_blur_strength(_ strength: Float)

@_silgen_name("postfx_get_motion_blur_strength")
private func postfx_get_motion_blur_strength() -> Float

@_silgen_name("postfx_set_chromatic_aberration_enabled")
private func postfx_set_chromatic_aberration_enabled(_ enabled: Bool)

@_silgen_name("postfx_is_chromatic_aberration_enabled")
private func postfx_is_chromatic_aberration_enabled() -> Bool

@_silgen_name("postfx_set_chromatic_aberration_intensity")
private func postfx_set_chromatic_aberration_intensity(_ intensity: Float)

@_silgen_name("postfx_get_chromatic_aberration_intensity")
private func postfx_get_chromatic_aberration_intensity() -> Float

@_silgen_name("postfx_set_film_grain_enabled")
private func postfx_set_film_grain_enabled(_ enabled: Bool)

@_silgen_name("postfx_is_film_grain_enabled")
private func postfx_is_film_grain_enabled() -> Bool

@_silgen_name("postfx_set_film_grain_intensity")
private func postfx_set_film_grain_intensity(_ intensity: Float)

@_silgen_name("postfx_get_film_grain_intensity")
private func postfx_get_film_grain_intensity() -> Float

@_silgen_name("postfx_set_vignette_enabled")
private func postfx_set_vignette_enabled(_ enabled: Bool)

@_silgen_name("postfx_is_vignette_enabled")
private func postfx_is_vignette_enabled() -> Bool

@_silgen_name("postfx_set_vignette_intensity")
private func postfx_set_vignette_intensity(_ intensity: Float)

@_silgen_name("postfx_get_vignette_intensity")
private func postfx_get_vignette_intensity() -> Float
