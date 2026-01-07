import Foundation
import simd

@MainActor
public final class SkySystem {
    public static let shared = SkySystem()
    
    public var timeOfDay: Float = 0.0 // 0.0 to 1.0 (noon to noon)
    public var dayDuration: Float = 600.0 // Seconds
    
    public var sunPosition: SIMD3<Float> = .zero
    public var skyColor: SIMD3<Float> = .zero
    
    private init() {}
    
    public func update(deltaTime: Float) {
        timeOfDay += deltaTime / dayDuration
        if timeOfDay >= 1.0 { timeOfDay -= 1.0 }
        
        // Calculate sun position (orbit around X axis)
        let angle = timeOfDay * 2.0 * .pi
        sunPosition = SIMD3<Float>(0, sin(angle), cos(angle)) * 100.0
        
        // Calculate sky color based on sun height
        let sunHeight = sunPosition.y
        if sunHeight > 0 {
            // Day time (blue to orange)
            let t = sunHeight / 100.0
            skyColor = mix(SIMD3<Float>(0.9, 0.5, 0.2), SIMD3<Float>(0.4, 0.6, 0.9), t)
        } else {
            // Night time (dark blue to black)
            let t = abs(sunHeight) / 100.0
            skyColor = mix(SIMD3<Float>(0.2, 0.1, 0.3), SIMD3<Float>(0.0, 0.0, 0.05), t)
        }
    }
    
    private func mix(_ a: SIMD3<Float>, _ b: SIMD3<Float>, _ t: Float) -> SIMD3<Float> {
        return a * (1 - t) + b * t
    }
}

public enum WeatherType {
    case clear
    case rain
    case snow
    case storm
}

@MainActor
public final class WeatherSystem {
    public static let shared = WeatherSystem()
    
    public var currentWeather: WeatherType = .clear
    public var cloudDensity: Float = 0.2
    
    private init() {}
    
    public func setWeather(_ type: WeatherType) {
        currentWeather = type
        switch type {
        case .clear: cloudDensity = 0.2
        case .rain: cloudDensity = 0.8
        case .snow: cloudDensity = 0.9
        case .storm: cloudDensity = 1.0
        }
        
        Logger.info("Weather changed to \(type)")
    }
}
