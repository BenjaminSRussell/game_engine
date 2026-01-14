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
    
    // CSV export data
    private var csvData: [String] = []
    private var isRecording = false
    private var recordingStartTime: Date?
    
    private init() {
        startUpdateTimer()
        initializeCSV()
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
    
    // MARK: - CSV Export
    
    /// Start recording performance data for CSV export
    func startRecording() {
        guard !isRecording else { return }
        
        isRecording = true
        recordingStartTime = Date()
        csvData.removeAll()
        
        // Add CSV header
        csvData.append("Timestamp,FPS,FrameTime,MemoryUsed,DrawCalls,Triangles")
        
        // Add initial data point
        addCSVDataPoint()
        
        LOG_INFO("Started profiler recording for CSV export")
    }
    
    /// Stop recording and export to CSV file
    func stopRecording() -> URL? {
        guard isRecording else { return nil }
        
        isRecording = false
        
        // Add final data point
        addCSVDataPoint()
        
        // Create CSV file
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        let timestamp = formatter.string(from: Date())
        let filename = "profiler_data_\(timestamp).csv"
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let fileURL = documentsPath.appendingPathComponent(filename)
        
        do {
            try csvData.joined(separator: "\n").write(to: fileURL, atomically: true, encoding: .utf8)
            
            LOG_INFO("Profiler data exported to: \(fileURL.path)")
            return fileURL
        } catch {
            LOG_ERROR("Failed to export profiler data: \(error)")
            return nil
        }
    }
    
    /// Get current recording state
    func getRecordingState() -> (isRecording: Bool, duration: TimeInterval?, dataPoints: Int) {
        let duration = recordingStartTime?.timeIntervalSinceNow
        return (isRecording: isRecording, duration: duration, dataPoints: csvData.count - 1)
    }
    
    /// Export current data to CSV string
    func exportToCSVString() -> String {
        return csvData.joined(separator: "\n")
    }
    
    /// Clear all recorded data
    func clearData() {
        csvData.removeAll()
        if !csvData.isEmpty {
            csvData.append("Timestamp,FPS,FrameTime,MemoryUsed,DrawCalls,Triangles")
        }
    }
    
    /// Get performance statistics as dictionary
    func getStatsAsDictionary() -> [String: Any] {
        return [
            "fps": stats.fps,
            "frameTime": stats.frameTime,
            "memoryUsed": stats.memoryUsed,
            "drawCalls": stats.drawCalls,
            "triangles": stats.triangles,
            "timestamp": Date().timeIntervalSinceReferenceDate
        ]
    }
    
    /// Export performance data to JSON
    func exportToJSON() -> String? {
        let data = getStatsAsDictionary()
        
        do {
            let jsonData = try JSONSerialization.data(withJSONObject: data, options: .prettyPrinted)
            return String(data: jsonData, encoding: .utf8)
        } catch {
            LOG_ERROR("Failed to export profiler data to JSON: \(error)")
            return nil
        }
    }
    
    // MARK: - Private Methods
    
    private func startUpdateTimer() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            
            // Record data point if recording
            if self.isRecording {
                self.addCSVDataPoint()
            }
            
            // Poll C engine for real statistics via EngineBridge (mocked for now until C function is ready)
            // let stats = EngineBridge.shared.getPerformanceStats()
            // For now, use simulated data
            self.simulateStats()
        }
    }
    
    private func initializeCSV() {
        csvData.append("Timestamp,FPS,FrameTime,MemoryUsed,DrawCalls,Triangles")
    }
    
    private func addCSVDataPoint() {
        let timestamp = Date().timeIntervalSinceReferenceDate
        let row = "\(timestamp),\(stats.fps),\(stats.frameTime),\(stats.memoryUsed),\(stats.drawCalls),\(stats.triangles)"
        csvData.append(row)
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
        if isRecording {
            stopRecording()
        }
    }
}

// MARK: - CSV Export Helper

extension Profiler {
    /// Export performance data to a custom file path
    func exportToCSV(at url: URL) -> Bool {
        let csvString = exportToCSVString()
        
        do {
            try csvString.write(to: url, atomically: true, encoding: .utf8)
            LOG_INFO("Profiler data exported to custom path: \(url.path)")
            return true
        } catch {
            LOG_ERROR("Failed to export profiler data to custom path: \(error)")
            return false
        }
    }
    
    /// Get formatted statistics string for display
    func getFormattedStats() -> String {
        return """
        Performance Statistics:
        FPS: \(String(format: "%.1f", stats.fps))
        Frame Time: \(String(format: "%.2f", stats.frameTime)) ms
        Memory Used: \(ByteCountFormatter.string(fromByteCount: Int64(stats.memoryUsed)))
        Draw Calls: \(stats.drawCalls)
        Triangles: \(stats.triangles)
        """
    }
    
    /// Check if performance is within acceptable thresholds
    func isPerformanceAcceptable() -> Bool {
        return stats.fps >= 30.0 && stats.frameTime <= 33.33 && stats.memoryUsed < 512_000_000
    }
    
    /// Get performance grade
    func getPerformanceGrade() -> String {
        if stats.fps >= 60 && stats.frameTime <= 16.67 {
            return "Excellent"
        } else if stats.fps >= 45 && stats.frameTime <= 22.22 {
            return "Good"
        } else if stats.fps >= 30 && stats.frameTime <= 33.33 {
            return "Fair"
        } else {
            return "Poor"
        }
    }
}

// MARK: - Performance Alerts

extension Profiler {
    /// Check for performance issues and return alerts
    func getPerformanceAlerts() -> [String] {
        var alerts: [String] = []
        
        if stats.fps < 30 {
            alerts.append("Low FPS detected: \(String(format: "%.1f", stats.fps))")
        }
        
        if stats.frameTime > 33.33 {
            alerts.append("High frame time: \(String(format: "%.2f", stats.frameTime)) ms")
        }
        
        if stats.memoryUsed > 512_000_000 {
            alerts.append("High memory usage: \(ByteCountFormatter.string(fromByteCount: Int64(stats.memoryUsed)))")
        }
        
        if stats.drawCalls > 1000 {
            alerts.append("High draw calls: \(stats.drawCalls)")
        }
        
        return alerts
    }
    
    /// Get performance recommendations
    func getPerformanceRecommendations() -> [String] {
        var recommendations: [String] = []
        
        if stats.fps < 30 {
            recommendations.append("Consider reducing visual quality or optimizing rendering")
        }
        
        if stats.memoryUsed > 512_000_000 {
            recommendations.append("Consider reducing texture quality or implementing memory pooling")
        }
        
        if stats.drawCalls > 1000 {
            recommendations.append("Consider batching draw calls or using instanced rendering")
        }
        
        return recommendations
    }
}
