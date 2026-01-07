// ProfilerAPI.swift
// Swift wrapper for performance profiling

import Foundation

/// Performance profiler API
public class ProfilerAPI {
    public static let shared = ProfilerAPI()
    private init() {}
    
    // MARK: - Profiler Snapshot
    
    public struct Snapshot {
        // Frame timing (ms)
        public var cpuFrameTime: Float
        public var gpuFrameTime: Float
        public var totalFrameTime: Float
        
        // System breakdown (ms)
        public var physicsTime: Float
        public var renderTime: Float
        public var scriptTime: Float
        public var audioTime: Float
        public var uiTime: Float
        
        // Rendering stats
        public var drawCalls: UInt32
        public var triangles: UInt32
        public var vertices: UInt32
        public var shaderSwitches: UInt32
        public var textureSwitches: UInt32
        
        // Memory (bytes)
        public var memoryUsed: UInt64
        public var memoryPeak: UInt64
        public var gpuMemoryUsed: UInt64
        public var gpuMemoryPeak: UInt64
        
        // Frame stats
        public var fps: Float
        public var avgFrameTime: Float
        public var minFrameTime: Float
        public var maxFrameTime: Float
    }
    
    // MARK: - Control
    
    public func setEnabled(_ enabled: Bool) {
        profiler_set_enabled(enabled)
    }
    
    public func isEnabled() -> Bool {
        return profiler_is_enabled()
    }
    
    // MARK: - Snapshot
    
    public func getSnapshot() -> Snapshot {
        let cSnapshot = UnsafeMutablePointer<ProfilerSnapshot_C>.allocate(capacity: 1)
        defer { cSnapshot.deallocate() }
        profiler_get_snapshot(cSnapshot)
        
        return Snapshot(
            cpuFrameTime: cSnapshot.pointee.cpu_frame_time,
            gpuFrameTime: cSnapshot.pointee.gpu_frame_time,
            totalFrameTime: cSnapshot.pointee.total_frame_time,
            physicsTime: cSnapshot.pointee.physics_time,
            renderTime: cSnapshot.pointee.render_time,
            scriptTime: cSnapshot.pointee.script_time,
            audioTime: cSnapshot.pointee.audio_time,
            uiTime: cSnapshot.pointee.ui_time,
            drawCalls: cSnapshot.pointee.draw_calls,
            triangles: cSnapshot.pointee.triangles,
            vertices: cSnapshot.pointee.vertices,
            shaderSwitches: cSnapshot.pointee.shader_switches,
            textureSwitches: cSnapshot.pointee.texture_switches,
            memoryUsed: cSnapshot.pointee.memory_used,
            memoryPeak: cSnapshot.pointee.memory_peak,
            gpuMemoryUsed: cSnapshot.pointee.gpu_memory_used,
            gpuMemoryPeak: cSnapshot.pointee.gpu_memory_peak,
            fps: cSnapshot.pointee.fps,
            avgFrameTime: cSnapshot.pointee.avg_frame_time,
            minFrameTime: cSnapshot.pointee.min_frame_time,
            maxFrameTime: cSnapshot.pointee.max_frame_time
        )
    }
    
    // MARK: - Samples
    
    public struct Sample {
        public var name: String
        public var timeMs: Float
        public var percentage: Float
        public var callCount: UInt32
        public var depth: UInt32
    }
    
    public func getSamples(maxCount: Int = 100) -> [Sample] {
        let cSamples = UnsafeMutablePointer<ProfilerSample_C>.allocate(capacity: maxCount)
        defer { cSamples.deallocate() }
        let count = profiler_get_samples(cSamples, UInt32(maxCount))
        
        return (0..<Int(count)).map { i in
            var nameTuple = cSamples[i].name
            let nameStr = withUnsafePointer(to: &nameTuple) { ptr in
                ptr.withMemoryRebound(to: CChar.self, capacity: 64) { cStr in
                    String(cString: cStr)
                }
            }
            
            return Sample(
                name: nameStr,
                timeMs: cSamples[i].time_ms,
                percentage: cSamples[i].percentage,
                callCount: cSamples[i].call_count,
                depth: cSamples[i].depth
            )
        }
    }
    
    // MARK: - Profiling
    
    public func beginSample(_ name: String) {
        profiler_begin_sample(name)
    }
    
    public func endSample() {
        profiler_end_sample()
    }
    
    public func measureSample<T>(_ name: String, _ block: () throws -> T) rethrows -> T {
        beginSample(name)
        defer { endSample() }
        return try block()
    }
    
    // MARK: - Settings
    
    public func resetStats() {
        profiler_reset_stats()
    }
    
    public func setUpdateFrequency(_ frequency: UInt32) {
        profiler_set_update_frequency(frequency)
    }
    
    public func getUpdateFrequency() -> UInt32 {
        return profiler_get_update_frequency()
    }
    
    public func setGPUProfilingEnabled(_ enabled: Bool) {
        profiler_set_gpu_profiling_enabled(enabled)
    }
    
    public func isGPUProfilingEnabled() -> Bool {
        return profiler_is_gpu_profiling_enabled()
    }
}

// MARK: - C Bridging

private typealias ProfilerSnapshot_C = (
    cpu_frame_time: Float,
    gpu_frame_time: Float,
    total_frame_time: Float,
    physics_time: Float,
    render_time: Float,
    script_time: Float,
    audio_time: Float,
    ui_time: Float,
    draw_calls: UInt32,
    triangles: UInt32,
    vertices: UInt32,
    shader_switches: UInt32,
    texture_switches: UInt32,
    memory_used: UInt64,
    memory_peak: UInt64,
    gpu_memory_used: UInt64,
    gpu_memory_peak: UInt64,
    fps: Float,
    avg_frame_time: Float,
    min_frame_time: Float,
    max_frame_time: Float
)

private typealias ProfilerSample_C = (
    name: (CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar, CChar),
    time_ms: Float,
    percentage: Float,
    call_count: UInt32,
    depth: UInt32
)

@_silgen_name("profiler_set_enabled")
private func profiler_set_enabled(_ enabled: Bool)

@_silgen_name("profiler_is_enabled")
private func profiler_is_enabled() -> Bool

@_silgen_name("profiler_get_snapshot")
private func profiler_get_snapshot(_ snapshot: UnsafeMutablePointer<ProfilerSnapshot_C>)

@_silgen_name("profiler_begin_sample")
private func profiler_begin_sample(_ name: String)

@_silgen_name("profiler_end_sample")
private func profiler_end_sample()

@_silgen_name("profiler_get_samples")
private func profiler_get_samples(_ samples: UnsafeMutablePointer<ProfilerSample_C>, _ maxCount: UInt32) -> UInt32

@_silgen_name("profiler_reset_stats")
private func profiler_reset_stats()

@_silgen_name("profiler_set_update_frequency")
private func profiler_set_update_frequency(_ frequency: UInt32)

@_silgen_name("profiler_get_update_frequency")
private func profiler_get_update_frequency() -> UInt32

@_silgen_name("profiler_set_gpu_profiling_enabled")
private func profiler_set_gpu_profiling_enabled(_ enabled: Bool)

@_silgen_name("profiler_is_gpu_profiling_enabled")
private func profiler_is_gpu_profiling_enabled() -> Bool
