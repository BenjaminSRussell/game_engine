// VegetationAPI.swift
// Swift wrapper for Vegetation System

import Foundation

public class VegetationAPI {
    public static let shared = VegetationAPI()
    private init() {}
    
    // MARK: - Vegetation Control
    
    public var isEnabled: Bool {
        get { return vegetation_is_enabled() }
        set { vegetation_set_enabled(newValue) }
    }
    
    public var density: Float {
        get { return vegetation_get_density() }
        set { vegetation_set_density(newValue) }
    }
    
    public var windStrength: Float {
        get { return vegetation_get_wind_strength() }
        set { vegetation_set_wind_strength(newValue) }
    }
    
    public var lodBias: Float {
        get { return vegetation_get_lod_bias() }
        set { vegetation_set_lod_bias(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("vegetation_set_enabled")
private func vegetation_set_enabled(_ enabled: Bool)

@_silgen_name("vegetation_is_enabled")
private func vegetation_is_enabled() -> Bool

@_silgen_name("vegetation_set_density")
private func vegetation_set_density(_ density: Float)

@_silgen_name("vegetation_get_density")
private func vegetation_get_density() -> Float

@_silgen_name("vegetation_set_wind_strength")
private func vegetation_set_wind_strength(_ strength: Float)

@_silgen_name("vegetation_get_wind_strength")
private func vegetation_get_wind_strength() -> Float

@_silgen_name("vegetation_set_lod_bias")
private func vegetation_set_lod_bias(_ bias: Float)

@_silgen_name("vegetation_get_lod_bias")
private func vegetation_get_lod_bias() -> Float
