// GridRenderingAPI.swift
// Swift wrapper for editor grid

import Foundation

public class GridRenderingAPI {
    public static let shared = GridRenderingAPI()
    private init() {}
    
    // MARK: - Grid Control
    
    public var isEnabled: Bool {
        get { return grid_is_enabled() }
        set { grid_set_enabled(newValue) }
    }
    
    public var size: Float {
        get { return grid_get_size() }
        set { grid_set_size(newValue) }
    }
    
    public var spacing: Float {
        get { return grid_get_spacing() }
        set { grid_set_spacing(newValue) }
    }
    
    public func setPrimaryColor(r: Float, g: Float, b: Float, a: Float) {
        grid_set_primary_color(r, g, b, a)
    }
    
    public func setSecondaryColor(r: Float, g: Float, b: Float, a: Float) {
        grid_set_secondary_color(r, g, b, a)
    }
}

// MARK: - C Bridging

@_silgen_name("grid_set_enabled")
private func grid_set_enabled(_ enabled: Bool)

@_silgen_name("grid_is_enabled")
private func grid_is_enabled() -> Bool

@_silgen_name("grid_set_size")
private func grid_set_size(_ size: Float)

@_silgen_name("grid_get_size")
private func grid_get_size() -> Float

@_silgen_name("grid_set_spacing")
private func grid_set_spacing(_ spacing: Float)

@_silgen_name("grid_get_spacing")
private func grid_get_spacing() -> Float

@_silgen_name("grid_set_primary_color")
private func grid_set_primary_color(_ r: Float, _ g: Float, _ b: Float, _ a: Float)

@_silgen_name("grid_set_secondary_color")
private func grid_set_secondary_color(_ r: Float, _ g: Float, _ b: Float, _ a: Float)
