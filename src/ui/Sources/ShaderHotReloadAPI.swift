// ShaderHotReloadAPI.swift
// Swift wrapper for Shader Hot Reload

import Foundation

public class ShaderHotReloadAPI {
    public static let shared = ShaderHotReloadAPI()
    private init() {}
    
    // MARK: - Hot Reload Control
    
    public var isEnabled: Bool {
        get { return shader_hotreload_is_enabled() }
        set { shader_hotreload_set_enabled(newValue) }
    }
    
    public func reloadAll() {
        shader_hotreload_trigger()
    }
    
    public func reloadShader(name: String) {
        shader_hotreload_shader(name)
    }
    
    public func getWatchCount() -> UInt32 {
        return shader_hotreload_get_watch_count()
    }
}

// MARK: - C Bridging

@_silgen_name("shader_hotreload_set_enabled")
private func shader_hotreload_set_enabled(_ enabled: Bool)

@_silgen_name("shader_hotreload_is_enabled")
private func shader_hotreload_is_enabled() -> Bool

@_silgen_name("shader_hotreload_trigger")
private func shader_hotreload_trigger()

@_silgen_name("shader_hotreload_shader")
private func shader_hotreload_shader(_ name: String)

@_silgen_name("shader_hotreload_get_watch_count")
private func shader_hotreload_get_watch_count() -> UInt32
