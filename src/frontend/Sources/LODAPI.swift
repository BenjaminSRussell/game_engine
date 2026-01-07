// LODAPI.swift
// Swift wrapper for LOD Control

import Foundation

public class LODAPI {
    public static let shared = LODAPI()
    private init() {}
    
    // MARK: - LOD Control
    
    public var isEnabled: Bool {
        get { return lod_is_enabled() }
        set { lod_set_enabled(newValue) }
    }
    
    public var bias: Float {
        get { return lod_get_bias() }
        set { lod_set_bias(newValue) }
    }
    
    public var distanceMultiplier: Float {
        get { return lod_get_distance_multiplier() }
        set { lod_set_distance_multiplier(newValue) }
    }
    
    public var forceLevel: Int32 {
        get { return lod_get_force_level() }
        set { lod_set_force_level(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("lod_set_enabled")
private func lod_set_enabled(_ enabled: Bool)

@_silgen_name("lod_is_enabled")
private func lod_is_enabled() -> Bool

@_silgen_name("lod_set_bias")
private func lod_set_bias(_ bias: Float)

@_silgen_name("lod_get_bias")
private func lod_get_bias() -> Float

@_silgen_name("lod_set_distance_multiplier")
private func lod_set_distance_multiplier(_ multiplier: Float)

@_silgen_name("lod_get_distance_multiplier")
private func lod_get_distance_multiplier() -> Float

@_silgen_name("lod_set_force_level")
private func lod_set_force_level(_ level: Int32)

@_silgen_name("lod_get_force_level")
private func lod_get_force_level() -> Int32
