import SwiftUI
import Foundation

// MARK: - Performance Statistics

struct PerformanceStats {
    var fps: Double = 60.0
    var frameTime: Double = 16.67  // milliseconds
    var memoryUsed: Int = 0        // bytes
    var drawCalls: Int = 0
    var triangles: Int = 0
}

// MARK: - Profiler Singleton

/// Global profiler singleton that provides real-time performance statistics
/// This wraps ProfilerViewModel and bridges data from the C engine
class Profiler: ObservableObject {
    static let shared = Profiler()
    
    @Published var stats = PerformanceStats()
    
    private var updateTimer: Timer?
    private var startTime = Date()
    private var frameCount: Int = 0
    
    private init() {
        startUpdateTimer()
    }
    
    // MARK: - Public API
    
    /// Update statistics from the C engine
    func updateFromEngine(fps: Double, frameTime: Double, memoryUsed: Int) {
        DispatchQueue.main.async { [weak self] in
            self?.stats.fps = fps
            self?.stats.frameTime = frameTime
            self?.stats.memoryUsed = memoryUsed
        }
    }
    
    /// Update rendering statistics
    func updateRenderStats(drawCalls: Int, triangles: Int) {
        DispatchQueue.main.async { [weak self] in
            self?.stats.drawCalls = drawCalls
            self?.stats.triangles = triangles
        }
    }
    
    /// Record a frame for FPS calculation
    func recordFrame() {
        frameCount += 1
        let elapsed = Date().timeIntervalSince(startTime)
        
        if elapsed >= 1.0 {
            let fps = Double(frameCount) / elapsed
            stats.fps = fps
            stats.frameTime = (elapsed / Double(frameCount)) * 1000.0
            
            // Reset counters
            frameCount = 0
            startTime = Date()
        }
    }
    
    // MARK: - Private Methods
    
    private func startUpdateTimer() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            
            // Poll C engine for real statistics via EngineBridge (mocked for now until C function is ready)
            // let stats = EngineBridge.shared.getPerformanceStats()
            // For now, use simulated data
            self.simulateStats()
        }
    }
    
    private func simulateStats() {
        // Simulate realistic performance stats
        // This will be replaced with actual C engine bridge calls
        stats.fps = Double.random(in: 58...62)
        stats.frameTime = 1000.0 / stats.fps
        stats.memoryUsed = Int.random(in: 250_000_000...260_000_000) // ~250-260 MB
        stats.drawCalls = Int.random(in: 120...130)
        stats.triangles = Int.random(in: 44000...46000)
    }
    
    deinit {
        updateTimer?.invalidate()
    }
}
