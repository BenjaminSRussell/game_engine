// ChunkAPI.swift
// Swift wrapper for Chunk Management

import Foundation

public class ChunkAPI {
    public static let shared = ChunkAPI()
    private init() {}
    
    // MARK: - Distance Control
    
    public var loadDistance: Float {
        get { return chunk_get_load_distance() }
        set { chunk_set_load_distance(newValue) }
    }
    
    public var unloadDistance: Float {
        get { return chunk_get_unload_distance() }
        set { chunk_set_unload_distance(newValue) }
    }
    
    // MARK: - Statistics
    
    public var loadedCount: UInt32 {
        return chunk_get_loaded_count()
    }
    
    public var memoryUsage: UInt64 {
        return chunk_get_memory_usage()
    }
    
    // MARK: - Loading Control
    
    public var asyncLoadingEnabled: Bool {
        get { return chunk_is_async_loading() }
        set { chunk_set_async_loading(newValue) }
    }
    
    public func forceLoad(x: Int32, z: Int32) {
        chunk_force_load(x, z)
    }
    
    public func forceUnload(x: Int32, z: Int32) {
        chunk_force_unload(x, z)
    }
}

// MARK: - C Bridging

@_silgen_name("chunk_set_load_distance")
private func chunk_set_load_distance(_ distance: Float)

@_silgen_name("chunk_get_load_distance")
private func chunk_get_load_distance() -> Float

@_silgen_name("chunk_set_unload_distance")
private func chunk_set_unload_distance(_ distance: Float)

@_silgen_name("chunk_get_unload_distance")
private func chunk_get_unload_distance() -> Float

@_silgen_name("chunk_get_loaded_count")
private func chunk_get_loaded_count() -> UInt32

@_silgen_name("chunk_force_load")
private func chunk_force_load(_ x: Int32, _ z: Int32)

@_silgen_name("chunk_force_unload")
private func chunk_force_unload(_ x: Int32, _ z: Int32)

@_silgen_name("chunk_get_memory_usage")
private func chunk_get_memory_usage() -> UInt64

@_silgen_name("chunk_set_async_loading")
private func chunk_set_async_loading(_ enabled: Bool)

@_silgen_name("chunk_is_async_loading")
private func chunk_is_async_loading() -> Bool
