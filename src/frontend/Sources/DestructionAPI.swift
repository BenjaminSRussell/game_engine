// DestructionAPI.swift
// Swift wrapper for Destruction System

import Foundation

public class DestructionAPI {
    public static let shared = DestructionAPI()
    private init() {}
    
    // MARK: - Destruction Control
    
    public var isEnabled: Bool {
        get { return destruction_is_enabled() }
        set { destruction_set_enabled(newValue) }
    }
    
    public var debrisLifetime: Float {
        get { return destruction_get_debris_lifetime() }
        set { destruction_set_debris_lifetime(newValue) }
    }
    
    public func createFracturedMesh(sourceMeshId: UInt64, fragmentCount: UInt32) -> UInt64 {
        return destruction_create_fractured_mesh(sourceMeshId, fragmentCount)
    }
    
    public func triggerBreak(entityId: UInt64, impactPoint: (x: Float, y: Float, z: Float), force: Float) {
        destruction_trigger_break(entityId, impactPoint.x, impactPoint.y, impactPoint.z, force)
    }
}

// MARK: - C Bridging

@_silgen_name("destruction_create_fractured_mesh")
private func destruction_create_fractured_mesh(_ sourceMeshId: UInt64, _ fragmentCount: UInt32) -> UInt64

@_silgen_name("destruction_trigger_break")
private func destruction_trigger_break(_ entityId: UInt64, _ x: Float, _ y: Float, _ z: Float, _ force: Float)

@_silgen_name("destruction_set_debris_lifetime")
private func destruction_set_debris_lifetime(_ seconds: Float)

@_silgen_name("destruction_get_debris_lifetime")
private func destruction_get_debris_lifetime() -> Float

@_silgen_name("destruction_set_enabled")
private func destruction_set_enabled(_ enabled: Bool)

@_silgen_name("destruction_is_enabled")
private func destruction_is_enabled() -> Bool
