// SwiftUI/Profiler.swift
// Live performance profiler view
import SwiftUI
import Charts

struct ProfilerView: View {
    @StateObject private var profilerData = ProfilerData()
    
    var body: some View {
        HSplitView {
            // Left: Performance charts
            VStack(spacing: 16) {
                // FPS Chart
                GroupBox("Frame Rate") {
                    Chart(profilerData.fpsHistory) { point in
                        LineMark(
                            x: .value("Frame", point.frame),
                            y: .value("FPS", point.fps)
                        )
                        .foregroundStyle(.green)
                    }
                    .chartYScale(domain: 0...144)
                    .frame(height: 120)
                }
                
                // Frame Time Chart
                GroupBox("Frame Time") {
                    Chart(profilerData.frameTimeHistory) { point in
                        AreaMark(
                            x: .value("Frame", point.frame),
                            y: .value("CPU", point.cpuTime)
                        )
                        .foregroundStyle(.blue.opacity(0.5))
                        
                        AreaMark(
                            x: .value("Frame", point.frame),
                            y: .value("GPU", point.gpuTime)
                        )
                        .foregroundStyle(.red.opacity(0.5))
                    }
                    .frame(height: 120)
                }
                
                // Memory Usage
                GroupBox("Memory") {
                    Chart(profilerData.memoryHistory) { point in
                        LineMark(
                            x: .value("Frame", point.frame),
                            y: .value("MB", point.memoryMB)
                        )
                        .foregroundStyle(.orange)
                    }
                    .chartYScale(domain: 0...8192)
                    .frame(height: 120)
                }
                
                Spacer()
            }
            .padding()
            
            // Right: Details
            VStack(alignment: .leading, spacing: 16) {
                GroupBox("Current Stats") {
                    Grid(alignment: .leading, horizontalSpacing: 20) {
                        GridRow {
                            Text("FPS:").bold()
                            Text("\\(profilerData.currentFPS, specifier: "%.1f")")
                            Color.clear
                        }
                        GridRow {
                            Text("CPU Time:").bold()
                            Text("\\(profilerData.currentCPUTime, specifier: "%.2f") ms")
                            progressBar(profilerData.currentCPUTime / 16.66)
                        }
                        GridRow {
                            Text("GPU Time:").bold()
                            Text("\\(profilerData.currentGPUTime, specifier: "%.2f") ms")
                            progressBar(profilerData.currentGPUTime / 16.66)
                        }
                        GridRow {
                            Text("Draw Calls:").bold()
                            Text("\\(profilerData.drawCalls)")
                            Color.clear
                        }
                        GridRow {
                            Text("Triangles:").bold()
                            Text("\\(profilerData.triangles / 1000)k")
                            Color.clear
                        }
                        GridRow {
                            Text("Memory:").bold()
                            Text("\\(profilerData.memoryMB) MB")
                            Color.clear
                        }
                    }
                }
                
                GroupBox("GPU Markers") {
                    List(profilerData.gpuMarkers) { marker in
                        HStack {
                            Text(marker.name)
                            Spacer()
                            Text("\\(marker.timeMs, specifier: "%.2f") ms")
                                .foregroundColor(.secondary)
                            progressBar(marker.timeMs / profilerData.currentGPUTime)
                                .frame(width: 100)
                        }
                    }
                }
                
                Spacer()
                
                HStack {
                    Button("Clear History") {
                        profilerData.clearHistory()
                    }
                    Button("Export CSV") {
                        profilerData.exportCSV()
                    }
                }
            }
            .padding()
            .frame(width: 400)
        }
    }
    
    func progressBar(_ value: Double) -> some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                RoundedRectangle(cornerRadius: 2)
                    .fill(Color.gray.opacity(0.3))
                
                RoundedRectangle(cornerRadius: 2)
                    .fill(value > 0.9 ? Color.red : (value > 0.7 ? Color.orange : Color.green))
                    .frame(width: geometry.size.width * min(value, 1.0))
            }
        }
        .frame(height: 8)
    }
}

class ProfilerData: ObservableObject {
    @Published var fpsHistory: [FPSPoint] = []
    @Published var frameTimeHistory: [FrameTimePoint] = []
    @Published var memoryHistory: [MemoryPoint] = []
    @Published var gpuMarkers: [GPUMarker] = []
    
    @Published var currentFPS: Double = 60
    @Published var currentCPUTime: Double = 8.5
    @Published var currentGPUTime: Double = 10.2
    @Published var drawCalls: Int = 1847
    @Published var triangles: Int = 2_500_000
    @Published var memoryMB: Int = 1024
    
    init() {
        // Simulate data
        for i in 0..<120 {
            fpsHistory.append(FPSPoint(frame: i, fps: 60 + Double.random(in: -5...5)))
            frameTimeHistory.append(FrameTimePoint(frame: i, cpuTime: 8 + Double.random(in: -2...2), gpuTime: 10 + Double.random(in: -2...2)))
            memoryHistory.append(MemoryPoint(frame: i, memoryMB: 1024 + Int.random(in: -50...50)))
        }
        
        gpuMarkers = [
            GPUMarker(name: "Geometry Pass", timeMs: 3.2),
            GPUMarker(name: "Lumen GI", timeMs: 2.5),
            GPUMarker(name: "Lighting", timeMs: 1.8),
            GPUMarker(name: "Post Process", timeMs: 1.5),
            GPUMarker(name: "MetalFX Upscale", timeMs: 0.8),
            GPUMarker(name: "UI", timeMs: 0.4)
        ]
    }
    
    func clearHistory() {
        fpsHistory.removeAll()
        frameTimeHistory.removeAll()
        memoryHistory.removeAll()
    }
    
    func exportCSV() {
        // TODO: Export to CSV
        print("Exporting profiler data...")
    }
}

struct FPSPoint: Identifiable {
    let id = UUID()
    var frame: Int
    var fps: Double
}

struct FrameTimePoint: Identifiable {
    let id = UUID()
    var frame: Int
    var cpuTime: Double
    var gpuTime: Double
}

struct MemoryPoint: Identifiable {
    let id = UUID()
    var frame: Int
    var memoryMB: Int
}

struct GPUMarker: Identifiable {
    let id = UUID()
    var name: String
    var timeMs: Double
}
