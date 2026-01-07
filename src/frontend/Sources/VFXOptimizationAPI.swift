// VFXOptimizationAPI.swift
// Swift wrapper for VFX Optimization

import Foundation

public class VFXOptimizationAPI {
    public static let shared = VFXOptimizationAPI()
    private init() {}
    
    // MARK: - Culling Control
    
    public var cullingEnabled: Bool {
        get { return vfx_opt_is_culling_enabled() }
        set { vfx_opt_set_culling_enabled(newValue) }
    }
    
    public var cullingDistance: Float {
        get { return vfx_opt_get_culling_distance() }
        set { vfx_opt_set_culling_distance(newValue) }
    }
    
    // MARK: - LOD Control
    
    public var lodBias: Float {
        get { return vfx_opt_get_lod_bias() }
        set { vfx_opt_set_lod_bias(newValue) }
    }
    
    // MARK: - Instancing
    
    public var instancingEnabled: Bool {
        get { return vfx_opt_is_instancing_enabled() }
        set { vfx_opt_set_instancing_enabled(newValue) }
    }
    
    // MARK: - Particle Budget
    
    public var maxParticles: UInt32 {
        get { return vfx_opt_get_max_particles() }
        set { vfx_opt_set_max_particles(newValue) }
    }
    
    public var activeParticles: UInt32 {
        return vfx_opt_get_active_particles()
    }
}

// MARK: - C Bridging

@_silgen_name("vfx_opt_set_culling_enabled")
private func vfx_opt_set_culling_enabled(_ enabled: Bool)

@_silgen_name("vfx_opt_is_culling_enabled")
private func vfx_opt_is_culling_enabled() -> Bool

@_silgen_name("vfx_opt_set_culling_distance")
private func vfx_opt_set_culling_distance(_ distance: Float)

@_silgen_name("vfx_opt_get_culling_distance")
private func vfx_opt_get_culling_distance() -> Float

@_silgen_name("vfx_opt_set_lod_bias")
private func vfx_opt_set_lod_bias(_ bias: Float)

@_silgen_name("vfx_opt_get_lod_bias")
private func vfx_opt_get_lod_bias() -> Float

@_silgen_name("vfx_opt_set_instancing_enabled")
private func vfx_opt_set_instancing_enabled(_ enabled: Bool)

@_silgen_name("vfx_opt_is_instancing_enabled")
private func vfx_opt_is_instancing_enabled() -> Bool

@_silgen_name("vfx_opt_set_max_particles")
private func vfx_opt_set_max_particles(_ count: UInt32)

@_silgen_name("vfx_opt_get_max_particles")
private func vfx_opt_get_max_particles() -> UInt32

@_silgen_name("vfx_opt_get_active_particles")
private func vfx_opt_get_active_particles() -> UInt32
