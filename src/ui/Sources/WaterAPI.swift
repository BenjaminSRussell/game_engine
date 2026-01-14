// WaterAPI.swift
// Swift wrapper for Water System configuration

import Foundation

public class WaterAPI {
    public static let shared = WaterAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum Quality: UInt32 {
        case low = 0
        case medium = 1
        case high = 2
    }
    
    // MARK: - Water Control
    
    public var isEnabled: Bool {
        get { return water_is_enabled() }
        set { water_set_enabled(newValue) }
    }
    
    public var quality: Quality {
        get { return Quality(rawValue: water_get_quality()) ?? .medium }
        set { water_set_quality(newValue.rawValue) }
    }
    
    public var waveAmplitude: Float {
        get { return water_get_wave_amplitude() }
        set { water_set_wave_amplitude(newValue) }
    }
    
    public var waveFrequency: Float {
        get { return water_get_wave_frequency() }
        set { water_set_wave_frequency(newValue) }
    }
    
    public var reflectionsEnabled: Bool {
        get { return water_get_reflections_enabled() }
        set { water_set_reflections_enabled(newValue) }
    }
    
    public var refractionsEnabled: Bool {
        get { return water_get_refractions_enabled() }
        set { water_set_refractions_enabled(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("water_set_enabled")
private func water_set_enabled(_ enabled: Bool)

@_silgen_name("water_is_enabled")
private func water_is_enabled() -> Bool

@_silgen_name("water_set_quality")
private func water_set_quality(_ quality: UInt32)

@_silgen_name("water_get_quality")
private func water_get_quality() -> UInt32

@_silgen_name("water_set_wave_amplitude")
private func water_set_wave_amplitude(_ amplitude: Float)

@_silgen_name("water_get_wave_amplitude")
private func water_get_wave_amplitude() -> Float

@_silgen_name("water_set_wave_frequency")
private func water_set_wave_frequency(_ frequency: Float)

@_silgen_name("water_get_wave_frequency")
private func water_get_wave_frequency() -> Float

@_silgen_name("water_set_reflections_enabled")
private func water_set_reflections_enabled(_ enabled: Bool)

@_silgen_name("water_get_reflections_enabled")
private func water_get_reflections_enabled() -> Bool

@_silgen_name("water_set_refractions_enabled")
private func water_set_refractions_enabled(_ enabled: Bool)

@_silgen_name("water_get_refractions_enabled")
private func water_get_refractions_enabled() -> Bool
