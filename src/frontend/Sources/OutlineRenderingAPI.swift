// OutlineRenderingAPI.swift
// Swift wrapper for outline rendering

import Foundation

public class OutlineRenderingAPI {
    public static let shared = OutlineRenderingAPI()
    private init() {}
    
    // MARK: - Outline Control
    
    public var isEnabled: Bool {
        get { return outline_is_enabled() }
        set { outline_set_enabled(newValue) }
    }
    
    public var width: Float {
        get { return outline_get_width() }
        set { outline_set_width(newValue) }
    }
    
    public func setColor(r: Float, g: Float, b: Float, a: Float) {
        outline_set_color(r, g, b, a)
    }
    
    public func setSelectionOnly(_ enabled: Bool) {
        outline_set_selection_only(enabled)
    }
    
    public func addEntity(_ entityId: UInt64) {
        outline_add_entity(entityId)
    }
    
    public func removeEntity(_ entityId: UInt64) {
        outline_remove_entity(entityId)
    }
    
    public func clearEntities() {
        outline_clear_entities()
    }
}

// MARK: - C Bridging

@_silgen_name("outline_set_enabled")
private func outline_set_enabled(_ enabled: Bool)

@_silgen_name("outline_is_enabled")
private func outline_is_enabled() -> Bool

@_silgen_name("outline_set_color")
private func outline_set_color(_ r: Float, _ g: Float, _ b: Float, _ a: Float)

@_silgen_name("outline_set_width")
private func outline_set_width(_ width: Float)

@_silgen_name("outline_get_width")
private func outline_get_width() -> Float

@_silgen_name("outline_set_selection_only")
private func outline_set_selection_only(_ enabled: Bool)

@_silgen_name("outline_add_entity")
private func outline_add_entity(_ entityId: UInt64)

@_silgen_name("outline_remove_entity")
private func outline_remove_entity(_ entityId: UInt64)

@_silgen_name("outline_clear_entities")
private func outline_clear_entities()
