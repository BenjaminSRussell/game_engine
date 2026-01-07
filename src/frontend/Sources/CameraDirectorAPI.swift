// CameraDirectorAPI.swift
// Swift wrapper for Camera Director (Cinematics)

import Foundation

public class CameraDirectorAPI {
    public static let shared = CameraDirectorAPI()
    private init() {}
    
    // MARK: - Path Management
    
    public func createPath(name: String) -> UInt64 {
        return camera_path_create(name)
    }
    
    public func destroyPath(_ pathId: UInt64) {
        camera_path_destroy(pathId)
    }
    
    public func addKeyframe(to pathId: UInt64, time: Float, 
                           position: (x: Float, y: Float, z: Float),
                           lookAt: (x: Float, y: Float, z: Float)) {
        camera_path_add_keyframe(pathId, time, 
                                position.x, position.y, position.z,
                                lookAt.x, lookAt.y, lookAt.z)
    }
    
    public func removeKeyframe(from pathId: UInt64, time: Float) {
        camera_path_remove_keyframe(pathId, time)
    }
    
    // MARK: - Playback Control
    
    public var isPlaying: Bool {
        return camera_path_is_playing()
    }
    
    public func play(_ pathId: UInt64, loop: Bool = false) {
        camera_path_play(pathId, loop)
    }
    
    public func stop() {
        camera_path_stop()
    }
    
    public func pause() {
        camera_path_pause()
    }
    
    public func resume() {
        camera_path_resume()
    }
    
    // MARK: - Camera Properties
    
    public var fov: Float {
        get { return camera_get_fov() }
        set { camera_set_fov(newValue) }
    }
    
    public var focusDistance: Float {
        get { return camera_get_focus_distance() }
        set { camera_set_focus_distance(newValue) }
    }
}

// MARK: - C Bridging

@_silgen_name("camera_path_create")
private func camera_path_create(_ name: String) -> UInt64

@_silgen_name("camera_path_destroy")
private func camera_path_destroy(_ pathId: UInt64)

@_silgen_name("camera_path_add_keyframe")
private func camera_path_add_keyframe(_ pathId: UInt64, _ time: Float,
                                     _ posX: Float, _ posY: Float, _ posZ: Float,
                                     _ lookX: Float, _ lookY: Float, _ lookZ: Float)

@_silgen_name("camera_path_remove_keyframe")
private func camera_path_remove_keyframe(_ pathId: UInt64, _ time: Float)

@_silgen_name("camera_path_play")
private func camera_path_play(_ pathId: UInt64, _ loop: Bool)

@_silgen_name("camera_path_stop")
private func camera_path_stop()

@_silgen_name("camera_path_pause")
private func camera_path_pause()

@_silgen_name("camera_path_resume")
private func camera_path_resume()

@_silgen_name("camera_path_is_playing")
private func camera_path_is_playing() -> Bool

@_silgen_name("camera_set_fov")
private func camera_set_fov(_ fov: Float)

@_silgen_name("camera_get_fov")
private func camera_get_fov() -> Float

@_silgen_name("camera_set_focus_distance")
private func camera_set_focus_distance(_ distance: Float)

@_silgen_name("camera_get_focus_distance")
private func camera_get_focus_distance() -> Float
