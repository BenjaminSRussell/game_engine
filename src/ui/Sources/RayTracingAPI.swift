// RayTracingAPI.swift
// Swift wrapper for Ray Tracing configuration

import Foundation

public class RayTracingAPI {
    public static let shared = RayTracingAPI()
    private init() {}
    
    // MARK: - Ray Tracing Control
    
    public var isSupported: Bool {
        return raytracing_is_supported()
    }
    
    public var isEnabled: Bool {
        get { return raytracing_is_enabled() }
        set { raytracing_set_enabled(newValue) }
    }
    
    public var maxBounces: UInt32 {
        get { return raytracing_get_max_bounces() }
        set { raytracing_set_max_bounces(newValue) }
    }
    
    public var samplesPerPixel: UInt32 {
        get { return raytracing_get_samples_per_pixel() }
        set { raytracing_set_samples_per_pixel(newValue) }
    }
    
    public var isDenoisingEnabled: Bool {
        get { return raytracing_get_denoising_enabled() }
        set { raytracing_set_denoising_enabled(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("raytracing_set_enabled")
private func raytracing_set_enabled(_ enabled: Bool)

@_silgen_name("raytracing_is_enabled")
private func raytracing_is_enabled() -> Bool

@_silgen_name("raytracing_is_supported")
private func raytracing_is_supported() -> Bool

@_silgen_name("raytracing_set_max_bounces")
private func raytracing_set_max_bounces(_ bounces: UInt32)

@_silgen_name("raytracing_get_max_bounces")
private func raytracing_get_max_bounces() -> UInt32

@_silgen_name("raytracing_set_samples_per_pixel")
private func raytracing_set_samples_per_pixel(_ samples: UInt32)

@_silgen_name("raytracing_get_samples_per_pixel")
private func raytracing_get_samples_per_pixel() -> UInt32

@_silgen_name("raytracing_set_denoising_enabled")
private func raytracing_set_denoising_enabled(_ enabled: Bool)

@_silgen_name("raytracing_get_denoising_enabled")
private func raytracing_get_denoising_enabled() -> Bool
