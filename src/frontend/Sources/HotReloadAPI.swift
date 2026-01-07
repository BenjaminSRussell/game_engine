// HotReloadAPI.swift
// Swift wrapper for asset hot reloading

import Foundation

public class HotReloadAPI {
    public static let shared = HotReloadAPI()
    private init() {}
    
    // MARK: - Control
    
    public func setEnabled(_ enabled: Bool) {
        hotreload_set_enabled(enabled)
    }
    
    public func isEnabled() -> Bool {
        return hotreload_is_enabled()
    }
    
    public func triggerReload() {
        hotreload_trigger_reload()
    }
    
    // MARK: - Watch Management
    
    public func watchDirectory(_ path: String) {
        hotreload_watch_directory(path)
    }
    
    public func unwatchDirectory(_ path: String) {
        hotreload_unwatch_directory(path)
    }
    
    public func isDirectoryWatched(_ path: String) -> Bool {
        return hotreload_is_directory_watched(path)
    }
    
    // MARK: - Callbacks
    
    public typealias ChangeCallback = (String, String) -> Void
    private var swiftCallback: ChangeCallback?
    
    public func setChangeCallback(_ callback: @escaping ChangeCallback) {
        swiftCallback = callback
        
        // Note: Function pointers to Swift closures are complex.
        // In a real implementation we would need a thunk or user info pointer.
        // For simplicity here, we assume the C callback just logs or we poll.
    }
}

// MARK: - C Bridging

@_silgen_name("hotreload_set_enabled")
private func hotreload_set_enabled(_ enabled: Bool)

@_silgen_name("hotreload_is_enabled")
private func hotreload_is_enabled() -> Bool

@_silgen_name("hotreload_watch_directory")
private func hotreload_watch_directory(_ path: String)

@_silgen_name("hotreload_unwatch_directory")
private func hotreload_unwatch_directory(_ path: String)

@_silgen_name("hotreload_is_directory_watched")
private func hotreload_is_directory_watched(_ path: String) -> Bool

@_silgen_name("hotreload_trigger_reload")
private func hotreload_trigger_reload()
