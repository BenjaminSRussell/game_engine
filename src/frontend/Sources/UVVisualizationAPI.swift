// UVVisualizationAPI.swift
// Swift wrapper for UV visualization

import Foundation

public class UVVisualizationAPI {
    public static let shared = UVVisualizationAPI()
    private init() {}
    
    // MARK: - Entity Control
    
    public func setEnabled(_ enabled: Bool, forEntity entityId: UInt64, channel: UInt32 = 0) {
        uv_vis_set_enabled(entityId, enabled, channel)
    }
    
    public func isEnabled(forEntity entityId: UInt64) -> Bool {
        return uv_vis_is_enabled(entityId)
    }
    
    public func getChannel(forEntity entityId: UInt64) -> UInt32 {
        return uv_vis_get_channel(entityId)
    }
    
    // MARK: - Global Settings
    
    public var checkerboardScale: Float {
        get { return uv_vis_get_checkerboard_scale() }
        set { uv_vis_set_checkerboard_scale(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("uv_vis_set_enabled")
private func uv_vis_set_enabled(_ entityId: UInt64, _ enabled: Bool, _ channel: UInt32)

@_silgen_name("uv_vis_is_enabled")
private func uv_vis_is_enabled(_ entityId: UInt64) -> Bool

@_silgen_name("uv_vis_get_channel")
private func uv_vis_get_channel(_ entityId: UInt64) -> UInt32

@_silgen_name("uv_vis_set_checkerboard_scale")
private func uv_vis_set_checkerboard_scale(_ scale: Float)

@_silgen_name("uv_vis_get_checkerboard_scale")
private func uv_vis_get_checkerboard_scale() -> Float
