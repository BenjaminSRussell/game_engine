// BiomeAPI.swift
// Swift wrapper for Biome System

import Foundation

public class BiomeAPI {
    public static let shared = BiomeAPI()
    private init() {}
    
    // MARK: - Types
    
    public enum BiomeType: Int32 {
        case desert = 0
        case forest = 1
        case plains = 2
        case tundra = 3
        case jungle = 4
        case ocean = 5
        case mountains = 6
    }
    
    public struct BiomeConfig {
        public var type: BiomeType
        public var temperature: Float    // -1.0 to 1.0
        public var humidity: Float        // 0.0 to 1.0
        public var vegetationDensity: Float // 0.0 to 1.0
        
        public init(type: BiomeType, temperature: Float, humidity: Float, vegetationDensity: Float) {
            self.type = type
            self.temperature = temperature
            self.humidity = humidity
            self.vegetationDensity = vegetationDensity
        }
    }
    
    // MARK: - Biome Control
    
    public var blendDistance: Float {
        get { return biome_get_blend_distance() }
        set { biome_set_blend_distance(newValue) }
    }
    
    public func createBiome(name: String, config: BiomeConfig) -> UInt64 {
        let cConfig = BiomeConfig_C(
            type: config.type.rawValue,
            temperature: config.temperature,
            humidity: config.humidity,
            vegetation_density: config.vegetationDensity
        )
        return biome_create(name, cConfig)
    }
    
    public func sampleBiomeAt(position: (x: Float, y: Float, z: Float)) -> BiomeType {
        let rawValue = biome_sample_at(position.x, position.y, position.z)
        return BiomeType(rawValue: rawValue) ?? .plains
    }
    
    public func getConfig(for type: BiomeType) -> BiomeConfig {
        let cConfig = biome_get_config(type.rawValue)
        return BiomeConfig(
            type: BiomeType(rawValue: cConfig.type) ?? .plains,
            temperature: cConfig.temperature,
            humidity: cConfig.humidity,
            vegetationDensity: cConfig.vegetation_density
        )
    }
}

// MARK: - C Types

private struct BiomeConfig_C {
    var type: Int32
    var temperature: Float
    var humidity: Float
    var vegetation_density: Float
}

// MARK: - C Bridging

@_silgen_name("biome_create")
private func biome_create(_ name: String, _ config: BiomeConfig_C) -> UInt64

@_silgen_name("biome_sample_at")
private func biome_sample_at(_ x: Float, _ y: Float, _ z: Float) -> Int32

@_silgen_name("biome_set_blend_distance")
private func biome_set_blend_distance(_ distance: Float)

@_silgen_name("biome_get_blend_distance")
private func biome_get_blend_distance() -> Float

@_silgen_name("biome_get_config")
private func biome_get_config(_ type: Int32) -> BiomeConfig_C
