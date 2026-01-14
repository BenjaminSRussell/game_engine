// PrefabAPI.swift
// Swift wrapper for prefab system

import Foundation

public class PrefabAPI {
    public static let shared = PrefabAPI()
    private init() {}
    
    // MARK: - Prefab Operations
    
    /// Create a new prefab from an existing entity
    public func createPrefab(from entityId: UInt64, at path: String) -> Bool {
        return prefab_create_from_entity(entityId, path)
    }
    
    /// Instantiate a prefab at a specific location
    public func instantiate(_ path: String, position: (x: Float, y: Float, z: Float)) -> UInt64 {
        return prefab_instantiate(path, position.x, position.y, position.z)
    }
    
    /// Save changes from an instance back to the prefab asset
    public func applyChanges(to path: String, from instanceId: UInt64) -> Bool {
        return prefab_apply_changes(path, instanceId)
    }
    
    /// Revert an instance to match the prefab source
    public func revertInstance(_ instanceId: UInt64) -> Bool {
        return prefab_revert_instance(instanceId)
    }
    
    /// Unpack a prefab instance (break link to prefab)
    public func unpackInstance(_ instanceId: UInt64) -> Bool {
        return prefab_unpack(instanceId)
    }
    
    // MARK: - Queries
    
    public func isInstance(_ entityId: UInt64) -> Bool {
        return prefab_is_instance(entityId)
    }
    
    public func getSourcePath(_ entityId: UInt64) -> String? {
        guard let ptr = prefab_get_source_path(entityId) else { return nil }
        return String(cString: ptr)
    }
}

// MARK: - C Bridging

@_silgen_name("prefab_create_from_entity")
private func prefab_create_from_entity(_ entityId: UInt64, _ path: String) -> Bool

@_silgen_name("prefab_instantiate")
private func prefab_instantiate(_ path: String, _ x: Float, _ y: Float, _ z: Float) -> UInt64

@_silgen_name("prefab_apply_changes")
private func prefab_apply_changes(_ path: String, _ instanceId: UInt64) -> Bool

@_silgen_name("prefab_revert_instance")
private func prefab_revert_instance(_ instanceId: UInt64) -> Bool

@_silgen_name("prefab_is_instance")
private func prefab_is_instance(_ entityId: UInt64) -> Bool

@_silgen_name("prefab_get_source_path")
private func prefab_get_source_path(_ entityId: UInt64) -> UnsafePointer<CChar>?

@_silgen_name("prefab_unpack")
private func prefab_unpack(_ entityId: UInt64) -> Bool
