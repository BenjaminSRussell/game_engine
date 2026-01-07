// OcclusionAPI.swift
// Swift wrapper for Occlusion Culling

import Foundation

public class OcclusionAPI {
    public static let shared = OcclusionAPI()
    private init() {}
    
    public enum Mode: Int32 {
        case none = 0
        case portal = 1
        case hardware = 2
    }
    
    public var isEnabled: Bool {
        get { return occlusion_is_enabled() }
        set { occlusion_set_enabled(newValue) }
    }
    
    public var mode: Mode {
        get { return Mode(rawValue: occlusion_get_mode()) ?? .hardware }
        set { occlusion_set_mode(newValue.rawValue) }
    }
    
    public var visibleCount: UInt32 {
        return occlusion_get_visible_count()
    }
    
    public var culledCount: UInt32 {
        return occlusion_get_culled_count()
    }
    
    public func addOccluder(_ entityId: UInt64) {
        occlusion_add_occluder(entityId)
    }
    
    public func removeOccluder(_ entityId: UInt64) {
        occlusion_remove_occluder(entityId)
    }
}

// MARK: - C Bridging

@_silgen_name("occlusion_set_enabled")
private func occlusion_set_enabled(_ enabled: Bool)

@_silgen_name("occlusion_is_enabled")
private func occlusion_is_enabled() -> Bool

@_silgen_name("occlusion_set_mode")
private func occlusion_set_mode(_ mode: Int32)

@_silgen_name("occlusion_get_mode")
private func occlusion_get_mode() -> Int32

@_silgen_name("occlusion_get_visible_count")
private func occlusion_get_visible_count() -> UInt32

@_silgen_name("occlusion_get_culled_count")
private func occlusion_get_culled_count() -> UInt32

@_silgen_name("occlusion_add_occluder")
private func occlusion_add_occluder(_ entityId: UInt64)

@_silgen_name("occlusion_remove_occluder")
private func occlusion_remove_occluder(_ entityId: UInt64)
