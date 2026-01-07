// GizmoAPI.swift
// Swift wrapper for gizmo manipulation tools

import Foundation
import SwiftUI

/// Gizmo operation mode
public enum GizmoMode: Int32 {
    case translate = 0
    case rotate = 1
    case scale = 2
    case universal = 3
}

/// Gizmo coordinate space
public enum GizmoSpace: Int32 {
    case world = 0
    case local = 1
}

/// Gizmo manipulation API
public class GizmoAPI {
    public static let shared = GizmoAPI()
    private init() {}
    
    // MARK: - Mode & Space
    
    /// Set gizmo operation mode
    public func setMode(_ mode: GizmoMode) {
        gizmo_set_mode(mode.rawValue)
    }
    
    /// Get current gizmo mode
    public func getMode() -> GizmoMode {
        return GizmoMode(rawValue: gizmo_get_mode()) ?? .translate
    }
    
    /// Set coordinate space (world or local)
    public func setSpace(_ space: GizmoSpace) {
        gizmo_set_space(space.rawValue)
    }
    
    /// Get current coordinate space
    public func getSpace() -> GizmoSpace {
        return GizmoSpace(rawValue: gizmo_get_space()) ?? .world
    }
    
    // MARK: - Enable/Disable
    
    /// Enable or disable gizmo rendering
    public func setEnabled(_ enabled: Bool) {
        gizmo_set_enabled(enabled)
    }
    
    /// Check if gizmo is enabled
    public func isEnabled() -> Bool {
        return gizmo_is_enabled()
    }
    
    // MARK: - Snapping
    
    /// Enable or disable snapping
    public func setSnapEnabled(_ enabled: Bool) {
        gizmo_set_snap_enabled(enabled)
    }
    
    /// Check if snapping is enabled
    public func isSnapEnabled() -> Bool {
        return gizmo_is_snap_enabled()
    }
    
    /// Set translation snap value (world units)
    public func setTranslationSnap(_ value: Float) {
        gizmo_set_translation_snap(value)
    }
    
    /// Get translation snap value
    public func getTranslationSnap() -> Float {
        return gizmo_get_translation_snap()
    }
    
    /// Set rotation snap value (degrees)
    public func setRotationSnap(_ degrees: Float) {
        gizmo_set_rotation_snap(degrees)
    }
    
    /// Get rotation snap value
    public func getRotationSnap() -> Float {
        return gizmo_get_rotation_snap()
    }
    
    /// Set scale snap value (multiplier)
    public func setScaleSnap(_ value: Float) {
        gizmo_set_scale_snap(value)
    }
    
    /// Get scale snap value
    public func getScaleSnap() -> Float {
        return gizmo_get_scale_snap()
    }
    
    // MARK: - Appearance
    
    /// Set gizmo size multiplier
    public func setSize(_ size: Float) {
        gizmo_set_size(size)
    }
    
    /// Get gizmo size
    public func getSize() -> Float {
        return gizmo_get_size()
    }
    
    /// Set gizmo opacity (0-1)
    public func setOpacity(_ opacity: Float) {
        gizmo_set_opacity(opacity)
    }
    
    /// Get gizmo opacity
    public func getOpacity() -> Float {
        return gizmo_get_opacity()
    }
    
    // MARK: - Axis Customization
    
    /// Enable/disable specific axis (0=X, 1=Y, 2=Z)
    public func setAxisEnabled(_ axis: Int, enabled: Bool) {
        gizmo_set_axis_enabled(UInt32(axis), enabled)
    }
    
    /// Set color for specific axis (0=X, 1=Y, 2=Z)
    public func setAxisColor(_ axis: Int, color: Color) {
        let (r, g, b) = color.rgbComponents
        gizmo_set_axis_color(UInt32(axis), r, g, b)
    }
}

// MARK: - C Bridging

@_silgen_name("gizmo_set_mode")
private func gizmo_set_mode(_ mode: Int32)

@_silgen_name("gizmo_get_mode")
private func gizmo_get_mode() -> Int32

@_silgen_name("gizmo_set_space")
private func gizmo_set_space(_ space: Int32)

@_silgen_name("gizmo_get_space")
private func gizmo_get_space() -> Int32

@_silgen_name("gizmo_set_enabled")
private func gizmo_set_enabled(_ enabled: Bool)

@_silgen_name("gizmo_is_enabled")
private func gizmo_is_enabled() -> Bool

@_silgen_name("gizmo_set_snap_enabled")
private func gizmo_set_snap_enabled(_ enabled: Bool)

@_silgen_name("gizmo_is_snap_enabled")
private func gizmo_is_snap_enabled() -> Bool

@_silgen_name("gizmo_set_translation_snap")
private func gizmo_set_translation_snap(_ value: Float)

@_silgen_name("gizmo_get_translation_snap")
private func gizmo_get_translation_snap() -> Float

@_silgen_name("gizmo_set_rotation_snap")
private func gizmo_set_rotation_snap(_ degrees: Float)

@_silgen_name("gizmo_get_rotation_snap")
private func gizmo_get_rotation_snap() -> Float

@_silgen_name("gizmo_set_scale_snap")
private func gizmo_set_scale_snap(_ value: Float)

@_silgen_name("gizmo_get_scale_snap")
private func gizmo_get_scale_snap() -> Float

@_silgen_name("gizmo_set_size")
private func gizmo_set_size(_ size: Float)

@_silgen_name("gizmo_get_size")
private func gizmo_get_size() -> Float

@_silgen_name("gizmo_set_opacity")
private func gizmo_set_opacity(_ opacity: Float)

@_silgen_name("gizmo_get_opacity")
private func gizmo_get_opacity() -> Float

@_silgen_name("gizmo_set_axis_enabled")
private func gizmo_set_axis_enabled(_ axis: UInt32, _ enabled: Bool)

@_silgen_name("gizmo_set_axis_color")
private func gizmo_set_axis_color(_ axis: UInt32, _ r: Float, _ g: Float, _ b: Float)

// MARK: - Color Extension

private extension Color {
    var rgbComponents: (Float, Float, Float) {
        #if canImport(UIKit)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        UIColor(self).getRed(&r, green: &g, blue: &b, alpha: &a)
        return (Float(r), Float(g), Float(b))
        #elseif canImport(AppKit)
        let nsColor = NSColor(self)
        guard let rgb = nsColor.usingColorSpace(.deviceRGB) else {
            return (0, 0, 0)
        }
        return (Float(rgb.redComponent), Float(rgb.greenComponent), Float(rgb.blueComponent))
        #endif
    }
}
