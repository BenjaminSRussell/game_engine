// DecalAPI.swift
// Swift wrapper for Decal System

import Foundation

public class DecalAPI {
    public static let shared = DecalAPI()
    private init() {}
    
    // MARK: - Decal Management
    
    public var isEnabled: Bool {
        get { return decal_is_enabled() }
        set { decal_set_enabled(newValue) }
    }
    
    public func createDecal(texturePath: String, position: (x: Float, y: Float, z: Float), size: Float) -> UInt64 {
        return decal_create(texturePath, position.x, position.y, position.z, size)
    }
    
    public func destroyDecal(_ decalId: UInt64) {
        decal_destroy(decalId)
    }
    
    public func setOpacity(_ decalId: UInt64, opacity: Float) {
        decal_set_opacity(decalId, opacity)
    }
    
    public func setRotation(_ decalId: UInt64, angle: Float) {
        decal_set_rotation(decalId, angle)
    }
}

// MARK: - C Bridging

@_silgen_name("decal_create")
private func decal_create(_ texturePath: String, _ x: Float, _ y: Float, _ z: Float, _ size: Float) -> UInt64

@_silgen_name("decal_destroy")
private func decal_destroy(_ decalId: UInt64)

@_silgen_name("decal_set_opacity")
private func decal_set_opacity(_ decalId: UInt64, _ opacity: Float)

@_silgen_name("decal_set_rotation")
private func decal_set_rotation(_ decalId: UInt64, _ angle: Float)

@_silgen_name("decal_set_enabled")
private func decal_set_enabled(_ enabled: Bool)

@_silgen_name("decal_is_enabled")
private func decal_is_enabled() -> Bool
