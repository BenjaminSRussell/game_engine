import Foundation
import SwiftUI
import Combine

// MARK: - Profiler Data

struct ProfilerData: Identifiable {
    let id = UUID()
    let timestamp: Date
    let fps: Float
    let frameTime: Float
    let drawCalls: Int
    let triangles: Int
    let memoryUsed: Int64
}

// MARK: - Profiler Manager

@MainActor
class Profiler: ObservableObject {
    static let shared = Profiler()
    
    @Published var stats = RenderStats()
    @Published var history: [ProfilerData] = []
    @Published var isRecording = false
    
    private var updateTimer: Timer?
    private let maxHistorySize = 300 // 5 seconds at 60fps
    private let updateInterval = 1.0 / 60.0
    
    private init() {}
    
    // MARK: - Control
    
    func startRecording() {
        guard !isRecording else { return }
        
        isRecording = true
        history.removeAll()
        
        updateTimer = Timer.scheduledTimer(withTimeInterval: updateInterval, repeats: true) { [weak self] _ in
            self?.recordFrame()
        }
    }
    
    func stopRecording() {
        isRecording = false
        updateTimer?.invalidate()
        updateTimer = nil
    }
    
    // MARK: - Data Collection
    
    private func recordFrame() {
        let data = ProfilerData(
            timestamp: Date(),
            fps: stats.fps,
            frameTime: stats.frameTimeMs,
            drawCalls: stats.drawCalls,
            triangles: stats.triangles,
            memoryUsed: Int64(stats.memoryUsed)
        )
        
        history.append(data)
        
        // Limit history size
        if history.count > maxHistorySize {
            history.removeFirst()
        }
    }
    
    func updateStats(_ newStats: RenderStats) {
        stats = newStats
        
        if isRecording {
            recordFrame()
        }
    }
    
    // MARK: - Analytics
    
    func averageFPS() -> Float {
        guard !history.isEmpty else { return 0 }
        return history.map(\.fps).reduce(0, +) / Float(history.count)
    }
    
    func averageFrameTime() -> Float {
        guard !history.isEmpty else { return 0 }
        return history.map(\.frameTime).reduce(0, +) / Float(history.count)
    }
    
    func peakMemory() -> Int64 {
        guard !history.isEmpty else { return 0 }
        return history.map(\.memoryUsed).max() ?? 0
    }
    
    func totalDrawCalls() -> Int {
        return history.map(\.drawCalls).reduce(0, +)
    }
    
    func totalTriangles() -> Int {
        return history.map(\.triangles).reduce(0, +)
    }
}

// MARK: - Profiler Views

struct ProfilerPanel: View {
    @ObservedObject private var profiler = Profiler.shared
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Performance Profiler")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                HStack(spacing: DesignSystem.Spacing.xs) {
                    Button(action: {
                        if profiler.isRecording {
                            profiler.stopRecording()
                        } else {
                            profiler.startRecording()
                        }
                    }) {
                        Image(systemName: profiler.isRecording ? "stop.fill" : "play.fill")
                            .foregroundColor(profiler.isRecording ? DesignSystem.Colors.accentError : DesignSystem.Colors.accentSuccess)
                    }
                    .buttonStyle(.plain)
                    
                    Button(action: {
                        profiler.history.removeAll()
                    }) {
                        Image(systemName: "trash")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            Divider()
            
            // Tabs
            HStack(spacing: 0) {
                ForEach(["Live", "History", "Memory"], id: \.self) { tab in
                    Button(tab) {
                        selectedTab = Array(["Live", "History", "Memory"]).firstIndex(of: tab) ?? 0
                    }
                    .font(DesignSystem.Typography.callout)
                    .foregroundColor(selectedTab == Array(["Live", "History", "Memory"]).firstIndex(of: tab) ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    .padding(.horizontal, DesignSystem.Spacing.md)
                    .padding(.vertical, DesignSystem.Spacing.sm)
                    .background(
                        selectedTab == Array(["Live", "History", "Memory"]).firstIndex(of: tab) ?
                        DesignSystem.Colors.backgroundSecondary : Color.clear
                    )
                }
                
                Spacer()
            }
            
            Divider()
            
            // Content
            TabView(selection: $selectedTab) {
                LiveStatsView()
                    .tag(0)
                
                HistoryView()
                    .tag(1)
                
                MemoryView()
                    .tag(2)
            }
            .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
        }
        .frame(width: 350, height: 400)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(DesignSystem.CornerRadius.medium)
    }
}

struct LiveStatsView: View {
    @ObservedObject private var profiler = Profiler.shared
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
                // FPS
                StatRow(
                    title: "FPS",
                    value: String(format: "%.1f", profiler.stats.fps),
                    color: profiler.stats.fps >= 50 ? DesignSystem.Colors.accentSuccess : 
                           profiler.stats.fps >= 30 ? DesignSystem.Colors.accentWarning : 
                           DesignSystem.Colors.accentError
                )
                
                // Frame Time
                StatRow(
                    title: "Frame Time",
                    value: String(format: "%.2f ms", profiler.stats.frameTimeMs),
                    color: profiler.stats.frameTimeMs <= 16.67 ? DesignSystem.Colors.accentSuccess : 
                           profiler.stats.frameTimeMs <= 33.33 ? DesignSystem.Colors.accentWarning : 
                           DesignSystem.Colors.accentError
                )
                
                // Draw Calls
                StatRow(
                    title: "Draw Calls",
                    value: "\(profiler.stats.drawCalls)",
                    color: DesignSystem.Colors.textPrimary
                )
                
                // Triangles
                StatRow(
                    title: "Triangles",
                    value: "\(profiler.stats.triangles)",
                    color: DesignSystem.Colors.textPrimary
                )
                
                // Memory
                StatRow(
                    title: "Memory Used",
                    value: formatMemory(profiler.stats.memoryUsed),
                    color: DesignSystem.Colors.textPrimary
                )
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.md)
        }
    }
    
    private func formatMemory(_ bytes: Int) -> String {
        let mb = Double(bytes) / (1024 * 1024)
        if mb >= 1024 {
            return String(format: "%.2f GB", mb / 1024)
        } else {
            return String(format: "%.2f MB", mb)
        }
    }
}

struct HistoryView: View {
    @ObservedObject private var profiler = Profiler.shared
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
                // Summary Stats
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Session Summary")
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    SummaryRow(title: "Average FPS", value: String(format: "%.1f", profiler.averageFPS()))
                    SummaryRow(title: "Average Frame Time", value: String(format: "%.2f ms", profiler.averageFrameTime()))
                    SummaryRow(title: "Peak Memory", value: formatMemory(profiler.peakMemory()))
                    SummaryRow(title: "Total Draw Calls", value: "\(profiler.totalDrawCalls())")
                    SummaryRow(title: "Total Triangles", value: "\(profiler.totalTriangles())")
                }
                
                Divider()
                
                // Recording Status
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Recording Status")
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    HStack {
                        Circle()
                            .fill(profiler.isRecording ? DesignSystem.Colors.accentError : DesignSystem.Colors.accentSuccess)
                            .frame(width: 8, height: 8)
                        
                        Text(profiler.isRecording ? "Recording" : "Stopped")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        Text("\(profiler.history.count) frames")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                }
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.md)
        }
    }
    
    private func formatMemory(_ bytes: Int64) -> String {
        let mb = Double(bytes) / (1024 * 1024)
        if mb >= 1024 {
            return String(format: "%.2f GB", mb / 1024)
        } else {
            return String(format: "%.2f MB", mb)
        }
    }
}

struct MemoryView: View {
    @ObservedObject private var profiler = Profiler.shared
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.lg) {
                Text("Memory Usage")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                // Memory breakdown would go here
                Text("Detailed memory breakdown coming soon...")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.md)
        }
    }
}

struct StatRow: View {
    let title: String
    let value: String
    let color: Color
    
    var body: some View {
        HStack {
            Text(title)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            Text(value)
                .font(DesignSystem.Typography.body)
                .fontWeight(.medium)
                .foregroundColor(color)
        }
    }
}

struct SummaryRow: View {
    let title: String
    let value: String
    
    var body: some View {
        HStack {
            Text(title)
                .font(DesignSystem.Typography.callout)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            Text(value)
                .font(DesignSystem.Typography.callout)
                .fontWeight(.medium)
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
    }
}
