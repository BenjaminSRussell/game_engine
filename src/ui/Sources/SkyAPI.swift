// SkyAPI.swift
// Swift wrapper for Sky & Atmosphere configuration

import Foundation

public class SkyAPI {
    public static let shared = SkyAPI()
    private init() {}
    
    // MARK: - Sky Control
    
    public var timeOfDay: Float {
        get { return sky_get_time_of_day() }
        set { sky_set_time_of_day(newValue) }
    }
    
    public var scatteringIntensity: Float {
        get { return sky_get_scattering_intensity() }
        set { sky_set_scattering_intensity(newValue) }
    }
    
    public var cloudDensity: Float {
        get { return sky_get_cloud_density() }
        set { sky_set_cloud_density(newValue) }
    }
    
    public var cloudsEnabled: Bool {
        get { return sky_get_clouds_enabled() }
        set { sky_set_clouds_enabled(newValue) }
    }
    
    public func setSunDirection(x: Float, y: Float, z: Float) {
        sky_set_sun_direction(x, y, z)
    }
}

// MARK: - C Bridging

@_silgen_name("sky_set_sun_direction")
private func sky_set_sun_direction(_ x: Float, _ y: Float, _ z: Float)

@_silgen_name("sky_set_time_of_day")
private func sky_set_time_of_day(_ hours: Float)

@_silgen_name("sky_get_time_of_day")
private func sky_get_time_of_day() -> Float

@_silgen_name("sky_set_scattering_intensity")
private func sky_set_scattering_intensity(_ intensity: Float)

@_silgen_name("sky_get_scattering_intensity")
private func sky_get_scattering_intensity() -> Float

@_silgen_name("sky_set_cloud_density")
private func sky_set_cloud_density(_ density: Float)

@_silgen_name("sky_get_cloud_density")
private func sky_get_cloud_density() -> Float

@_silgen_name("sky_set_clouds_enabled")
private func sky_set_clouds_enabled(_ enabled: Bool)

@_silgen_name("sky_get_clouds_enabled")
private func sky_get_clouds_enabled() -> Bool
