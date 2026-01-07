// SelectionAPI.swift
// Swift wrapper for editor selection system

import Foundation

public class SelectionAPI {
    public static let shared = SelectionAPI()
    private init() {}
    
    // MARK: - Selection Management
    
    /// Select an entity
    public func select(_ entityId: UInt64, additive: Bool = false) {
        selection_select(entityId, additive)
    }
    
    /// Deselect an entity
    public func deselect(_ entityId: UInt64) {
        selection_deselect(entityId)
    }
    
    /// Clear selection
    public func clear() {
        selection_clear()
    }
    
    /// Check if selected
    public func isSelected(_ entityId: UInt64) -> Bool {
        return selection_is_selected(entityId)
    }
    
    /// Get selection count
    public func getCount() -> UInt32 {
        return selection_get_count()
    }
    
    /// Get all selected entities
    public func getSelection() -> [UInt64] {
        let count = Int(selection_get_count())
        guard count > 0 else { return [] }
        
        var buffer = [UInt64](repeating: 0, count: count)
        _ = selection_get_selected(&buffer, UInt32(count))
        
        return buffer
    }
    
    /// Get primary selection
    public func getPrimary() -> UInt64? {
        let id = selection_get_primary()
        return id != 0 ? id : nil
    }
    
    // MARK: - Raycasting
    
    public func raycast(origin: (x: Float, y: Float, z: Float), direction: (x: Float, y: Float, z: Float)) -> UInt64? {
        let id = selection_raycast(origin.x, origin.y, origin.z, direction.x, direction.y, direction.z)
        return id != 0 ? id : nil
    }
    
    // MARK: - Callbacks
    
    public typealias SelectionCallback = (UInt32) -> Void
    private var swiftCallback: SelectionCallback?
    
    public func setCallback(_ callback: @escaping SelectionCallback) {
        swiftCallback = callback
        // Note: Similar to other APIs, proper C callback handling required in full implementation
    }
}

// MARK: - C Bridging

@_silgen_name("selection_select")
private func selection_select(_ entityId: UInt64, _ additive: Bool)

@_silgen_name("selection_deselect")
private func selection_deselect(_ entityId: UInt64)

@_silgen_name("selection_clear")
private func selection_clear()

@_silgen_name("selection_is_selected")
private func selection_is_selected(_ entityId: UInt64) -> Bool

@_silgen_name("selection_get_count")
private func selection_get_count() -> UInt32

@_silgen_name("selection_get_selected")
private func selection_get_selected(_ entities: UnsafeMutablePointer<UInt64>, _ maxCount: UInt32) -> UInt32

@_silgen_name("selection_get_primary")
private func selection_get_primary() -> UInt64

@_silgen_name("selection_raycast")
private func selection_raycast(_ ox: Float, _ oy: Float, _ oz: Float, _ dx: Float, _ dy: Float, _ dz: Float) -> UInt64
