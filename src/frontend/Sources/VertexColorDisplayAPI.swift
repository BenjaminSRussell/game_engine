// VertexColorDisplayAPI.swift
// Swift wrapper for vertex color visualization

import Foundation

public class VertexColorDisplayAPI {
    public static let shared = VertexColorDisplayAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum ChannelMode: UInt32 {
        case rgb = 0
        case red = 1
        case green = 2
        case blue = 3
        case alpha = 4
    }
    
    // MARK: - Control
    
    public func setEnabled(_ enabled: Bool, forEntity entityId: UInt64) {
        vcol_display_set_enabled(entityId, enabled)
    }
    
    public func isEnabled(forEntity entityId: UInt64) -> Bool {
        return vcol_display_is_enabled(entityId)
    }
    
    public func setChannel(_ mode: ChannelMode, forEntity entityId: UInt64) {
        vcol_display_set_channel(entityId, mode.rawValue)
    }
    
    public func getChannel(forEntity entityId: UInt64) -> ChannelMode {
        return ChannelMode(rawValue: vcol_display_get_channel(entityId)) ?? .rgb
    }
}

// MARK: - C Bridging

@_silgen_name("vcol_display_set_enabled")
private func vcol_display_set_enabled(_ entityId: UInt64, _ enabled: Bool)

@_silgen_name("vcol_display_is_enabled")
private func vcol_display_is_enabled(_ entityId: UInt64) -> Bool

@_silgen_name("vcol_display_set_channel")
private func vcol_display_set_channel(_ entityId: UInt64, _ mode: UInt32)

@_silgen_name("vcol_display_get_channel")
private func vcol_display_get_channel(_ entityId: UInt64) -> UInt32
