import SwiftUI
import Charts

// MARK: - Performance Profiler
// Real-time performance monitoring and analysis

class PerformanceProfiler: ObservableObject {
    @Published var isRecording = false
    @Published var fpsHistory: [Float] = []
    @Published var frameTimeHistory: [Float] = []
    @Published var cpuUsageHistory: [Float] = []
    @Published var gpuUsageHistory: [Float] = []
    @Published var memoryUsageHistory: [Float] = []
    @Published var drawCallsHistory: [Int] = []
    
    @Published var currentFPS: Float = 60.0
    @Published var currentFrameTimeMs: Float = 16.6
    @Published var currentCPUPercent: Float = 25.0
    @Published var currentGPUPercent: Float = 40.0
    @Published var currentMemoryMB: Float = 512.0
    @Published var currentDrawCalls: Int = 1500
    @Published var currentTriangles: Int = 500000
    @Published var currentVertices: Int = 1500000
    
    // Performance warnings
    @Published var warnings: [PerformanceWarning] = []
    @Published var bottlenecks: [Bottleneck] = []
    
    // Profiler sessions
    @Published var sessions: [ProfilerSession] = []
    
    let maxHistorySize = 300 // 5 seconds at 60 FPS
    
    // MARK: - Performance Warning
    struct PerformanceWarning: Identifiable {
        let id = UUID()
        let timestamp: Date
        let severity: Severity
        let category: Category
        let message: String
        var isAcknowledged = false
        
        enum Severity: String {
            case info = "Info"
            case warning = "Warning"
            case critical = "Critical"
        }
        
        enum Category: String {
            case fps = "FPS"
            case memory = "Memory"
            case drawCalls = "Draw Calls"
            case cpu = "CPU"
            case gpu = "GPU"
            case texture = "Texture"
            case shader = "Shader"
        }
    }
    
    // MARK: - Bottleneck
    struct Bottleneck: Identifiable {
        let id = UUID()
        let name: String
        let timeMs: Float
        let percentage: Float
        let category: Category
        
        enum Category: String {
            case rendering = "Rendering"
            case physics = "Physics"
            case scripts = "Scripts"
            case audio = "Audio"
            case ai = "AI"
            case networking = "Networking"
        }
    }
    
    // MARK: - Profiler Session
    struct ProfilerSession: Identifiable {
        let id = UUID()
        let name: String
        let startTime: Date
        var endTime: Date?
        var averageFPS: Float = 0
        var averageFrameTime: Float = 0
        var peakMemoryMB: Float = 0
        var totalDrawCalls: Int = 0
        
        // Future: Session persistence and comparison features
    }
    
    // MARK: - Functions
    
    // Real-time FPS graph implementation
    func updateFPS(_ fps: Float) {
        currentFPS = fps
        fpsHistory.append(fps)
        if fpsHistory.count > maxHistorySize {
            fpsHistory.removeFirst()
        }
        
        // Check for FPS warning
        if fps < 30 {
            addWarning(severity: .critical, category: .fps, message: "FPS dropped below 30: \(Int(fps))")
        } else if fps < 50 {
            addWarning(severity: .warning, category: .fps, message: "FPS below 50: \(Int(fps))")
        }
    }
    
    // Frame time graph implementation
    func updateFrameTime(_ ms: Float) {
        currentFrameTimeMs = ms
        frameTimeHistory.append(ms)
        if frameTimeHistory.count > maxHistorySize {
            frameTimeHistory.removeFirst()
        }
    }
    
    // CPU usage graph implementation
    func updateCPUUsage(_ percent: Float) {
        currentCPUPercent = percent
        cpuUsageHistory.append(percent)
        if cpuUsageHistory.count > maxHistorySize {
            cpuUsageHistory.removeFirst()
        }
        
        if percent > 90 {
            addWarning(severity: .critical, category: .cpu, message: "CPU usage critical: \(Int(percent))%")
        }
    }
    
    // GPU usage graph implementation
    func updateGPUUsage(_ percent: Float) {
        currentGPUPercent = percent
        gpuUsageHistory.append(percent)
        if gpuUsageHistory.count > maxHistorySize {
            gpuUsageHistory.removeFirst()
        }
        
        if percent > 95 {
            addWarning(severity: .critical, category: .gpu, message: "GPU usage critical: \(Int(percent))%")
        }
    }
    
    // Memory usage graph implementation
    func updateMemoryUsage(_ mb: Float) {
        currentMemoryMB = mb
        memoryUsageHistory.append(mb)
        if memoryUsageHistory.count > maxHistorySize {
            memoryUsageHistory.removeFirst()
        }
        
        if mb > 2048 {
            addWarning(severity: .warning, category: .memory, message: "Memory usage high: \(Int(mb)) MB")
        }
    }
    
    // Draw call counter implementation
    func updateDrawCalls(_ count: Int) {
        currentDrawCalls = count
        drawCallsHistory.append(count)
        if drawCallsHistory.count > maxHistorySize {
            drawCallsHistory.removeFirst()
        }
        
        if count > 5000 {
            addWarning(severity: .warning, category: .drawCalls, message: "High draw calls: \(count)")
        }
    }
    
    // Geometry tracking implementation
    func updateGeometry(triangles: Int, vertices: Int) {
        currentTriangles = triangles
        currentVertices = vertices
    }
    
    // Future profiling features:
    // - Texture memory tracking
    // - Shader compilation timing
    // - Physics/Audio/Script profiling
    // - GC event tracking
    // - Frame budget indicators
    
    func addWarning(severity: PerformanceWarning.Severity, category: PerformanceWarning.Category, message: String) {
        let warning = PerformanceWarning(timestamp: Date(), severity: severity, category: category, message: message)
        warnings.insert(warning, at: 0)
        
        // Keep only last 100 warnings
        if warnings.count > 100 {
            warnings.removeLast()
        }
    }
    
    // Bottleneck detection implementation
    func detectBottlenecks() {
        // Analyze current performance data and identify bottlenecks
        bottlenecks.removeAll()
        
        // Example bottleneck detection
        if currentGPUPercent > 80 {
            bottlenecks.append(Bottleneck(
                name: "GPU Rendering",
                timeMs: currentFrameTimeMs * (currentGPUPercent / 100),
                percentage: currentGPUPercent,
                category: .rendering
            ))
        }
    }
    
    // Future advanced profiling features:
    // - Timeline view & hierarchical profiler tree
    // - Flame graphs & CPU cache visualization
    // - Memory allocation/fragmentation tracking
    // - Asset loading & streaming analysis
    // - Network & disk I/O profiling
    
    func startRecording(name: String = "Session") {
        isRecording = true
        let session = ProfilerSession(name: name, startTime: Date())
        sessions.insert(session, at: 0)
        print("[Profiler] Started recording session: \(name)")
    }
    
    func stopRecording() {
        isRecording = false
        if !sessions.isEmpty {
            sessions[0].endTime = Date()
            // Calculate session statistics
            if !fpsHistory.isEmpty {
                sessions[0].averageFPS = fpsHistory.reduce(0, +) / Float(fpsHistory.count)
            }
            if !frameTimeHistory.isEmpty {
                sessions[0].averageFrameTime = frameTimeHistory.reduce(0, +) / Float(frameTimeHistory.count)
            }
            sessions[0].peakMemoryMB = memoryUsageHistory.max() ?? 0
        }
        print("[Profiler] Stopped recording")
    }
    
    func clearHistory() {
        fpsHistory.removeAll()
        frameTimeHistory.removeAll()
        cpuUsageHistory.removeAll()
        gpuUsageHistory.removeAll()
        memoryUsageHistory.removeAll()
        drawCallsHistory.removeAll()
        warnings.removeAll()
        bottlenecks.removeAll()
    }
}

// MARK: - Performance Profiler Panel
struct PerformanceProfilerPanel: View {
    @ObservedObject var profiler: PerformanceProfiler
    @State private var selectedTab: Tab = .overview
    
    enum Tab: String, CaseIterable {
        case overview = "Overview"
        case graphs = "Graphs"
        case warnings = "Warnings"
        case bottlenecks = "Bottlenecks"
        case sessions = "Sessions"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Performance Profiler")
                    .font(DesignSystem.Typography.h3)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                if profiler.isRecording {
                    HStack(spacing: 4) {
                        Circle()
                            .fill(Color.red)
                            .frame(width: 8, height: 8)
                        Text("Recording")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.red)
                    }
                }
                
                Button(action: {
                    if profiler.isRecording {
                        profiler.stopRecording()
                    } else {
                        profiler.startRecording()
                    }
                }) {
                    Image(systemName: profiler.isRecording ? "stop.circle.fill" : "record.circle")
                        .foregroundColor(profiler.isRecording ? .red : DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
            }
            .padding(8)
            
            EditorDivider()
            
            // Tabs
            HStack(spacing: 0) {
                ForEach(Tab.allCases, id: \.self) { tab in
                    Button(action: {
                        selectedTab = tab
                    }) {
                        Text(tab.rawValue)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(selectedTab == tab ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .padding(.vertical, 8)
                            .padding(.horizontal, 12)
                            .background(selectedTab == tab ? DesignSystem.Colors.selection : Color.clear)
                    }
                    .buttonStyle(.plain)
                }
            }
            
            EditorDivider()
            
            // Content
            ScrollView {
                switch selectedTab {
                case .overview:
                    OverviewTab(profiler: profiler)
                case .graphs:
                    GraphsTab(profiler: profiler)
                case .warnings:
                    WarningsTab(profiler: profiler)
                case .bottlenecks:
                    BottlenecksTab(profiler: profiler)
                case .sessions:
                    SessionsTab(profiler: profiler)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

// MARK: - Overview Tab
private struct OverviewTab: View {
    @ObservedObject var profiler: PerformanceProfiler
    
    var body: some View {
        VStack(spacing: 8) {
            // Current stats
            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
                StatCard(title: "FPS", value: String(format: "%.1f", profiler.currentFPS), color: fpsColor)
                StatCard(title: "Frame Time", value: String(format: "%.1f ms", profiler.currentFrameTimeMs), color: .blue)
                StatCard(title: "CPU", value: "\(Int(profiler.currentCPUPercent))%", color: .orange)
                StatCard(title: "GPU", value: "\(Int(profiler.currentGPUPercent))%", color: .purple)
                StatCard(title: "Memory", value: "\(Int(profiler.currentMemoryMB)) MB", color: .green)
                StatCard(title: "Draw Calls", value: "\(profiler.currentDrawCalls)", color: .red)
                StatCard(title: "Triangles", value: "\(profiler.currentTriangles / 1000)K", color: .cyan)
                StatCard(title: "Vertices", value: "\(profiler.currentVertices / 1000)K", color: .indigo)
            }
            .padding(8)
        }
    }
    
    var fpsColor: Color {
        if profiler.currentFPS >= 60 { return .green }
        if profiler.currentFPS >= 30 { return .orange }
        return .red
    }
}

// MARK: - Graphs Tab
private struct GraphsTab: View {
    @ObservedObject var profiler: PerformanceProfiler
    
    var body: some View {
        VStack(spacing: 16) {
            // FPS Graph
            VStack(alignment: .leading, spacing: 4) {
                Text("FPS History")
                    .font(DesignSystem.Typography.bodyBold)
                MiniGraph(data: profiler.fpsHistory, color: .green, maxValue: 120)
            }
            
            // Frame Time Graph
            VStack(alignment: .leading, spacing: 4) {
                Text("Frame Time (ms)")
                    .font(DesignSystem.Typography.bodyBold)
                MiniGraph(data: profiler.frameTimeHistory, color: .blue, maxValue: 50)
            }
            
            // CPU Graph
            VStack(alignment: .leading, spacing: 4) {
                Text("CPU Usage (%)")
                    .font(DesignSystem.Typography.bodyBold)
                MiniGraph(data: profiler.cpuUsageHistory, color: .orange, maxValue: 100)
            }
            
            // GPU Graph
            VStack(alignment: .leading, spacing: 4) {
                Text("GPU Usage (%)")
                    .font(DesignSystem.Typography.bodyBold)
                MiniGraph(data: profiler.gpuUsageHistory, color: .purple, maxValue: 100)
            }
            
            // Memory Graph
            VStack(alignment: .leading, spacing: 4) {
                Text("Memory (MB)")
                    .font(DesignSystem.Typography.bodyBold)
                MiniGraph(data: profiler.memoryUsageHistory, color: .green, maxValue: 2048)
            }
        }
        .padding(8)
    }
}

// MARK: - Warnings Tab
private struct WarningsTab: View {
    @ObservedObject var profiler: PerformanceProfiler
    
    var body: some View {
        VStack(spacing: 4) {
            ForEach(profiler.warnings) { warning in
                WarningRow(warning: warning)
            }
        }
        .padding(8)
    }
}

// MARK: - Bottlenecks Tab
private struct BottlenecksTab: View {
    @ObservedObject var profiler: PerformanceProfiler
    
    var body: some View {
        VStack(spacing: 8) {
            Button("Analyze Bottlenecks") {
                profiler.detectBottlenecks()
            }
            .padding(8)
            
            ForEach(profiler.bottlenecks) { bottleneck in
                BottleneckRow(bottleneck: bottleneck)
            }
        }
        .padding(8)
    }
}

// MARK: - Sessions Tab
private struct SessionsTab: View {
    @ObservedObject var profiler: PerformanceProfiler
    
    var body: some View {
        VStack(spacing: 4) {
            ForEach(profiler.sessions) { session in
                SessionRow(session: session)
            }
        }
        .padding(8)
    }
}

// MARK: - Supporting Views
private struct StatCard: View {
    let title: String
    let value: String
    let color: Color
    
    var body: some View {
        VStack(spacing: 4) {
            Text(title)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            Text(value)
                .font(DesignSystem.Typography.h2)
                .foregroundColor(color)
        }
        .frame(maxWidth: .infinity)
        .padding(12)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(8)
    }
}

private struct MiniGraph: View {
    let data: [Float]
    let color: Color
    let maxValue: Float
    
    var body: some View {
        GeometryReader { geometry in
            Path { path in
                guard !data.isEmpty else { return }
                
                let width = geometry.size.width
                let height = geometry.size.height
                let stepX = width / CGFloat(max(data.count - 1, 1))
                
                path.move(to: CGPoint(x: 0, y: height - CGFloat(data[0] / maxValue) * height))
                
                for (index, value) in data.enumerated() {
                    let x = CGFloat(index) * stepX
                    let y = height - CGFloat(value / maxValue) * height
                    path.addLine(to: CGPoint(x: x, y: y))
                }
            }
            .stroke(color, lineWidth: 2)
        }
        .frame(height: 60)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
}

private struct WarningRow: View {
    let warning: PerformanceProfiler.PerformanceWarning
    
    var body: some View {
        HStack {
            Image(systemName: iconForSeverity)
                .foregroundColor(colorForSeverity)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(warning.message)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text("\(warning.category.rawValue)  \(warning.timestamp, style: .time)")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
        }
        .padding(8)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
    
    var iconForSeverity: String {
        switch warning.severity {
        case .info: return "info.circle"
        case .warning: return "exclamationmark.triangle"
        case .critical: return "exclamationmark.octagon"
        }
    }
    
    var colorForSeverity: Color {
        switch warning.severity {
        case .info: return .blue
        case .warning: return .orange
        case .critical: return .red
        }
    }
}

private struct BottleneckRow: View {
    let bottleneck: PerformanceProfiler.Bottleneck
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(bottleneck.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(bottleneck.category.rawValue)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            VStack(alignment: .trailing, spacing: 2) {
                Text(String(format: "%.1f ms", bottleneck.timeMs))
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.red)
                
                Text(String(format: "%.1f%%", bottleneck.percentage))
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
        .padding(8)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}

private struct SessionRow: View {
    let session: PerformanceProfiler.ProfilerSession
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(session.name)
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            HStack {
                Text("Avg FPS: \(String(format: "%.1f", session.averageFPS))")
                Text("")
                Text("Frame: \(String(format: "%.1f ms", session.averageFrameTime))")
                Text("")
                Text("Peak Mem: \(Int(session.peakMemoryMB)) MB")
            }
            .font(DesignSystem.Typography.small)
            .foregroundColor(DesignSystem.Colors.textSecondary)
        }
        .padding(8)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}
