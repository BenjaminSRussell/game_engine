// GlobalIlluminationAPI.swift
// Swift wrapper for GI configuration

import Foundation

public class GlobalIlluminationAPI {
    public static let shared = GlobalIlluminationAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum Quality: UInt32 {
        case low = 0
        case medium = 1
        case high = 2
        case ultra = 3
    }
    
    // MARK: - Control
    
    public var isEnabled: Bool {
        get { return gi_is_enabled() }
        set { gi_set_enabled(newValue) }
    }
    
    public var quality: Quality {
        get { return Quality(rawValue: gi_get_quality()) ?? .medium }
        set { gi_set_quality(newValue.rawValue) }
    }
    
    public var intensity: Float {
        get { return gi_get_intensity() }
        set { gi_set_intensity(newValue) }
    }
    
    public var samples: UInt32 {
        get { return gi_get_samples() }
        set { gi_set_samples(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("gi_set_enabled")
private func gi_set_enabled(_ enabled: Bool)

@_silgen_name("gi_is_enabled")
private func gi_is_enabled() -> Bool

@_silgen_name("gi_set_quality")
private func gi_set_quality(_ quality: UInt32)

@_silgen_name("gi_get_quality")
private func gi_get_quality() -> UInt32

@_silgen_name("gi_set_intensity")
private func gi_set_intensity(_ intensity: Float)

@_silgen_name("gi_get_intensity")
private func gi_get_intensity() -> Float

@_silgen_name("gi_set_samples")
private func gi_set_samples(_ samples: UInt32)

@_silgen_name("gi_get_samples")
private func gi_get_samples() -> UInt32
