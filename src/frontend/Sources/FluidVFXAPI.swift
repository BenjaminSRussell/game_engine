// FluidVFXAPI.swift
// Swift wrapper for Fluid Simulation VFX

import Foundation

public class FluidVFXAPI {
    public static let shared = FluidVFXAPI()
    private init() {}
    
    // MARK: - Global Control
    
    public var isEnabled: Bool {
        get { return fluid_is_enabled() }
        set { fluid_set_enabled(newValue) }
    }
    
    // MARK: - Emitter Management
    
    public func createEmitter(position: (x: Float, y: Float, z: Float), emitRate: Float) -> UInt64 {
        return fluid_create_emitter(position.x, position.y, position.z, emitRate)
    }
    
    public func destroyEmitter(_ emitterId: UInt64) {
        fluid_destroy_emitter(emitterId)
    }
    
    public func setViscosity(_ emitterId: UInt64, viscosity: Float) {
        fluid_set_viscosity(emitterId, viscosity)
    }
    
    public func getViscosity(_ emitterId: UInt64) -> Float {
        return fluid_get_viscosity(emitterId)
    }
    
    public func setParticleCount(_ emitterId: UInt64, count: UInt32) {
        fluid_set_particle_count(emitterId, count)
    }
    
    public func getParticleCount(_ emitterId: UInt64) -> UInt32 {
        return fluid_get_particle_count(emitterId)
    }
    
    public func enableSplashing(_ emitterId: UInt64, enabled: Bool) {
        fluid_enable_splashing(emitterId, enabled)
    }
    
    public func isSplashingEnabled(_ emitterId: UInt64) -> Bool {
        return fluid_is_splashing_enabled(emitterId)
    }
}

// MARK: - C Bridging

@_silgen_name("fluid_create_emitter")
private func fluid_create_emitter(_ x: Float, _ y: Float, _ z: Float, _ emitRate: Float) -> UInt64

@_silgen_name("fluid_destroy_emitter")
private func fluid_destroy_emitter(_ emitterId: UInt64)

@_silgen_name("fluid_set_viscosity")
private func fluid_set_viscosity(_ emitterId: UInt64, _ viscosity: Float)

@_silgen_name("fluid_get_viscosity")
private func fluid_get_viscosity(_ emitterId: UInt64) -> Float

@_silgen_name("fluid_set_particle_count")
private func fluid_set_particle_count(_ emitterId: UInt64, _ count: UInt32)

@_silgen_name("fluid_get_particle_count")
private func fluid_get_particle_count(_ emitterId: UInt64) -> UInt32

@_silgen_name("fluid_enable_splashing")
private func fluid_enable_splashing(_ emitterId: UInt64, _ enabled: Bool)

@_silgen_name("fluid_is_splashing_enabled")
private func fluid_is_splashing_enabled(_ emitterId: UInt64) -> Bool

@_silgen_name("fluid_set_enabled")
private func fluid_set_enabled(_ enabled: Bool)

@_silgen_name("fluid_is_enabled")
private func fluid_is_enabled() -> Bool
