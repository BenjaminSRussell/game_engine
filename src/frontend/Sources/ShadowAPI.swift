// ShadowAPI.swift
// Swift wrapper for Shadow configuration

import Foundation

public class ShadowAPI {
    public static let shared = ShadowAPI()
    private init() {}
    
    // MARK: - Shadow Control
    
    public var isEnabled: Bool {
        get { return shadow_is_enabled() }
        set { shadow_set_enabled(newValue) }
    }
    
    public var cascadeCount: UInt32 {
        get { return shadow_get_cascade_count() }
        set { shadow_set_cascade_count(newValue) }
    }
    
    public var resolution: UInt32 {
        get { return shadow_get_resolution() }
        set { shadow_set_resolution(newValue) }
    }
    
    public var splitLambda: Float {
        get { return shadow_get_split_lambda() }
        set { shadow_set_split_lambda(newValue) }
    }
    
    public func setSoftShadowsEnabled(_ enabled: Bool) {
        shadow_set_soft_shadows(enabled)
    }
}

// MARK: - C Bridging

@_silgen_name("shadow_set_enabled")
private func shadow_set_enabled(_ enabled: Bool)

@_silgen_name("shadow_is_enabled")
private func shadow_is_enabled() -> Bool

@_silgen_name("shadow_set_cascade_count")
private func shadow_set_cascade_count(_ count: UInt32)

@_silgen_name("shadow_get_cascade_count")
private func shadow_get_cascade_count() -> UInt32

@_silgen_name("shadow_set_resolution")
private func shadow_set_resolution(_ resolution: UInt32)

@_silgen_name("shadow_get_resolution")
private func shadow_get_resolution() -> UInt32

@_silgen_name("shadow_set_split_lambda")
private func shadow_set_split_lambda(_ lambda: Float)

@_silgen_name("shadow_get_split_lambda")
private func shadow_get_split_lambda() -> Float

@_silgen_name("shadow_set_soft_shadows")
private func shadow_set_soft_shadows(_ enabled: Bool)
