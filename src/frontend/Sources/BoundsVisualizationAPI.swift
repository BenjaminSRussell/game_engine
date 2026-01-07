// BoundsVisualizationAPI.swift
// Swift wrapper for bounds visualization

import Foundation

public class BoundsVisualizationAPI {
    public static let shared = BoundsVisualizationAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum BoundsType {
        case aabb
        case obb
        case sphere
    }
    
    // MARK: - Control
    
    public func setEnabled(_ enabled: Bool, forEntity entityId: UInt64, type: BoundsType) {
        switch type {
        case .aabb: bounds_vis_set_aabb_enabled(entityId, enabled)
        case .obb: bounds_vis_set_obb_enabled(entityId, enabled)
        case .sphere: bounds_vis_set_sphere_enabled(entityId, enabled)
        }
    }
    
    public func isEnabled(forEntity entityId: UInt64, type: BoundsType) -> Bool {
        switch type {
        case .aabb: return bounds_vis_get_aabb_enabled(entityId)
        case .obb: return bounds_vis_get_obb_enabled(entityId)
        case .sphere: return bounds_vis_get_sphere_enabled(entityId)
        }
    }
    
    public func setGlobalColor(r: Float, g: Float, b: Float, a: Float) {
        bounds_vis_set_color(r, g, b, a)
    }
    
    public func setGlobalWidth(_ width: Float) {
        bounds_vis_set_width(width)
    }
}

// MARK: - C Bridging

@_silgen_name("bounds_vis_set_aabb_enabled")
private func bounds_vis_set_aabb_enabled(_ entityId: UInt64, _ enabled: Bool)

@_silgen_name("bounds_vis_set_obb_enabled")
private func bounds_vis_set_obb_enabled(_ entityId: UInt64, _ enabled: Bool)

@_silgen_name("bounds_vis_set_sphere_enabled")
private func bounds_vis_set_sphere_enabled(_ entityId: UInt64, _ enabled: Bool)

@_silgen_name("bounds_vis_get_aabb_enabled")
private func bounds_vis_get_aabb_enabled(_ entityId: UInt64) -> Bool

@_silgen_name("bounds_vis_get_obb_enabled")
private func bounds_vis_get_obb_enabled(_ entityId: UInt64) -> Bool

@_silgen_name("bounds_vis_get_sphere_enabled")
private func bounds_vis_get_sphere_enabled(_ entityId: UInt64) -> Bool

@_silgen_name("bounds_vis_set_color")
private func bounds_vis_set_color(_ r: Float, _ g: Float, _ b: Float, _ a: Float)

@_silgen_name("bounds_vis_set_width")
private func bounds_vis_set_width(_ width: Float)
