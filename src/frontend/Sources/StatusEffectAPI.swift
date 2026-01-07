// StatusEffectAPI.swift
// Swift wrapper for Status Effect Visuals

import Foundation

public class StatusEffectAPI {
    public static let shared = StatusEffectAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum EffectType: Int32 {
        case burning = 0
        case poison = 1
        case freezing = 2
        case healing = 3
        case shield = 4
        case speed = 5
    }
    
    public enum Quality: UInt32 {
        case low = 0
        case medium = 1
        case high = 2
    }
    
    // MARK: - Status Effect Control
    
    public var quality: Quality {
        get { return Quality(rawValue: status_effect_get_quality()) ?? .medium }
        set { status_effect_set_quality(newValue.rawValue) }
    }
    
    public func apply(to entityId: UInt64, effect: EffectType, intensity: Float) {
        status_effect_apply(entityId, effect.rawValue, intensity)
    }
    
    public func remove(from entityId: UInt64, effect: EffectType) {
        status_effect_remove(entityId, effect.rawValue)
    }
    
    public func clearAll(from entityId: UInt64) {
        status_effect_clear_all(entityId)
    }
}

// MARK: - C Bridging

@_silgen_name("status_effect_apply")
private func status_effect_apply(_ entityId: UInt64, _ effectType: Int32, _ intensity: Float)

@_silgen_name("status_effect_remove")
private func status_effect_remove(_ entityId: UInt64, _ effectType: Int32)

@_silgen_name("status_effect_clear_all")
private func status_effect_clear_all(_ entityId: UInt64)

@_silgen_name("status_effect_set_quality")
private func status_effect_set_quality(_ quality: UInt32)

@_silgen_name("status_effect_get_quality")
private func status_effect_get_quality() -> UInt32
