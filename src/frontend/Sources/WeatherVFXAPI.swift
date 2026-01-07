// WeatherVFXAPI.swift
// Swift wrapper for Weather VFX System

import Foundation

public class WeatherVFXAPI {
    public static let shared = WeatherVFXAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum WeatherType: Int32 {
        case clear = 0
        case rain = 1
        case snow = 2
        case storm = 3
        case fog = 4
    }
    
    // MARK: - Weather Control
    
    public var currentType: WeatherType {
        get { return WeatherType(rawValue: weather_get_type()) ?? .clear }
        set { weather_set_type(newValue.rawValue) }
    }
    
    public var rainIntensity: Float {
        get { return weather_get_rain_intensity() }
        set { weather_set_rain_intensity(newValue) }
    }
    
    public var snowIntensity: Float {
        get { return weather_get_snow_intensity() }
        set { weather_set_snow_intensity(newValue) }
    }
    
    public var windStrength: Float {
        get { return weather_get_wind_strength() }
        set { weather_set_wind_strength(newValue) }
    }
    
    public var fogDensity: Float {
        get { return weather_get_fog_density() }
        set { weather_set_fog_density(newValue) }
    }
    
    public func triggerLightning(at position: (x: Float, y: Float, z: Float)) {
        weather_trigger_lightning(position.x, position.y, position.z)
    }
}

// MARK: - C Bridging

@_silgen_name("weather_set_type")
private func weather_set_type(_ type: Int32)

@_silgen_name("weather_get_type")
private func weather_get_type() -> Int32

@_silgen_name("weather_set_rain_intensity")
private func weather_set_rain_intensity(_ intensity: Float)

@_silgen_name("weather_get_rain_intensity")
private func weather_get_rain_intensity() -> Float

@_silgen_name("weather_set_snow_intensity")
private func weather_set_snow_intensity(_ intensity: Float)

@_silgen_name("weather_get_snow_intensity")
private func weather_get_snow_intensity() -> Float

@_silgen_name("weather_trigger_lightning")
private func weather_trigger_lightning(_ x: Float, _ y: Float, _ z: Float)

@_silgen_name("weather_set_wind_strength")
private func weather_set_wind_strength(_ strength: Float)

@_silgen_name("weather_get_wind_strength")
private func weather_get_wind_strength() -> Float

@_silgen_name("weather_set_fog_density")
private func weather_set_fog_density(_ density: Float)

@_silgen_name("weather_get_fog_density")
private func weather_get_fog_density() -> Float
