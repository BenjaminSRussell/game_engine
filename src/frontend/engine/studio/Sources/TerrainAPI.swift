// TerrainAPI.swift
// Swift wrapper for Terrain configuration

import Foundation

public class TerrainAPI {
    public static let shared = TerrainAPI()
    private init() {}
    
    // MARK: - Terrain Control
    
    public var isEnabled: Bool {
        get { return terrain_is_enabled() }
        set { terrain_set_enabled(newValue) }
    }
    
    public var lodCount: UInt32 {
        get { return terrain_get_lod_count() }
        set { terrain_set_lod_count(newValue) }
    }
    
    public var tileSize: Float {
        get { return terrain_get_tile_size() }
        set { terrain_set_tile_size(newValue) }
    }
    
    public var heightScale: Float {
        get { return terrain_get_height_scale() }
        set { terrain_set_height_scale(newValue) }
    }
    
    public func setCenter(x: Float, y: Float, z: Float) {
        terrain_set_center(x, y, z)
    }
}

// MARK: - C Bridging

@_silgen_name("terrain_set_enabled")
private func terrain_set_enabled(_ enabled: Bool)

@_silgen_name("terrain_is_enabled")
private func terrain_is_enabled() -> Bool

@_silgen_name("terrain_set_lod_count")
private func terrain_set_lod_count(_ count: UInt32)

@_silgen_name("terrain_get_lod_count")
private func terrain_get_lod_count() -> UInt32

@_silgen_name("terrain_set_center")
private func terrain_set_center(_ x: Float, _ y: Float, _ z: Float)

@_silgen_name("terrain_set_tile_size")
private func terrain_set_tile_size(_ size: Float)

@_silgen_name("terrain_get_tile_size")
private func terrain_get_tile_size() -> Float

@_silgen_name("terrain_set_height_scale")
private func terrain_set_height_scale(_ scale: Float)

@_silgen_name("terrain_get_height_scale")
private func terrain_get_height_scale() -> Float
