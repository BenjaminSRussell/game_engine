import SwiftUI
import Charts

// MARK: - Profiler Panel

struct ProfilerPanel: View {
    @StateObject private var viewModel = ProfilerViewModel()
    @State private var selectedCategory: ProfileCategory = .performance
    
    enum ProfileCategory: String, CaseIterable {
        case performance = "Performance"
        case memory = "Memory"
        case rendering = "Rendering"
        case systems = "Systems"
        
        var icon: String {
            switch self {
            case .performance: return "speedometer"
            case .memory: return "memorychip"
            case .rendering: return "paintbrush"
            case .systems: return "cpu"
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar with category tabs
            HStack(spacing: 0) {
                ForEach(ProfileCategory.allCases, id: \.self) { category in
                    CategoryTab(
                        category: category,
                        isSelected: selectedCategory == category,
                        onSelect: { selectedCategory = category }
                    )
                }
                
                Spacer()
                
                // Recording controls
                HStack(spacing: DesignSystem.Spacing.xs) {
                    Button(action: { viewModel.toggleRecording() }) {
                        HStack(spacing: 4) {
                            Image(systemName: viewModel.isRecording ? "stop.circle.fill" : "record.circle")
                                .foregroundColor(viewModel.isRecording ? DesignSystem.Colors.accentError : DesignSystem.Colors.accentPrimary)
                            Text(viewModel.isRecording ? "Stop" : "Record")
                                .font(DesignSystem.Typography.small)
                        }
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    }
                    .buttonStyle(.plain)
                    
                    EditorIconButton(icon: "trash", tooltip: "Clear Data") {
                        viewModel.clearData()
                    }
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
            }
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Content based on selected category
            ScrollView {
                switch selectedCategory {
                case .performance:
                    PerformanceView(viewModel: viewModel)
                case .memory:
                    MemoryView(viewModel: viewModel)
                case .rendering:
                    RenderingView(viewModel: viewModel)
                case .systems:
                    SystemsView(viewModel: viewModel)
                }
            }
            .background(DesignSystem.Colors.backgroundSecondary)
        }
        .task(id: viewModel.isRecording) {
            if viewModel.isRecording {
                while !Task.isCancelled {
                    viewModel.updateData()
                    try? await Task.sleep(nanoseconds: 100_000_000) // 0.1s
                }
            }
        }
    }
}

// MARK: - Category Tab

private struct CategoryTab: View {
    let category: ProfilerPanel.ProfileCategory
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            HStack(spacing: DesignSystem.Spacing.xs) {
                Image(systemName: category.icon)
                    .font(.system(size: 14))
                Text(category.rawValue)
                    .font(DesignSystem.Typography.body)
            }
            .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
            .padding(.horizontal, DesignSystem.Spacing.lg)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(isSelected ? DesignSystem.Colors.backgroundSecondary : Color.clear)
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Performance View

private struct PerformanceView: View {
    @ObservedObject var viewModel: ProfilerViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            // FPS Graph
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                HStack {
                    Text("Frame Rate")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    Text("\(String(format: "%.1f", viewModel.snapshot.fps)) FPS")
                        .font(DesignSystem.Typography.h3)
                        .foregroundColor(viewModel.snapshot.fps >= 60 ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentWarning)
                }
                
                Chart(viewModel.fpsHistory) { dataPoint in
                    LineMark(
                        x: .value("Time", dataPoint.timestamp),
                        y: .value("FPS", dataPoint.value)
                    )
                    .foregroundStyle(DesignSystem.Colors.accentPrimary)
                }
                .frame(height: 150)
                .chartYScale(domain: 0...120)
                .chartXAxis(.hidden)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
            
            // Frame Time Graph
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                HStack {
                    Text("Frame Time")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    Text(String(format: "%.2fms", viewModel.snapshot.totalFrameTime))
                        .font(DesignSystem.Typography.h3)
                        .foregroundColor(viewModel.snapshot.totalFrameTime <= 16.67 ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentWarning)
                }
                
                Chart(viewModel.frameTimeHistory) { dataPoint in
                    AreaMark(
                        x: .value("Time", dataPoint.timestamp),
                        y: .value("ms", dataPoint.value)
                    )
                    .foregroundStyle(
                        LinearGradient(
                            colors: [DesignSystem.Colors.accentSuccess.opacity(0.5), DesignSystem.Colors.accentSuccess.opacity(0.1)],
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
                }
                .frame(height: 150)
                .chartYScale(domain: 0...50)
                .chartXAxis(.hidden)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
            
            // Performance Stats
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("Statistics")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorDivider()
                
                StatRow(label: "Average FPS", value: String(format: "%.1f", viewModel.snapshot.fps))
                StatRow(label: "Min Frame Time", value: String(format: "%.2fms", viewModel.snapshot.minFrameTime))
                StatRow(label: "Max Frame Time", value: String(format: "%.2fms", viewModel.snapshot.maxFrameTime))
                StatRow(label: "CPU Time", value: String(format: "%.2fms", viewModel.snapshot.cpuFrameTime))
                StatRow(label: "GPU Time", value: String(format: "%.2fms", viewModel.snapshot.gpuFrameTime))
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
        }
        .padding(DesignSystem.Spacing.md)
    }
}

// MARK: - Memory View

private struct MemoryView: View {
    @ObservedObject var viewModel: ProfilerViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            // Memory Usage Graph
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                HStack {
                    Text("Memory Usage")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    Text("\(Double(viewModel.snapshot.memoryUsed) / 1024 / 1024, specifier: "%.1f") MB")
                        .font(DesignSystem.Typography.h3)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                
                Chart(viewModel.memoryHistory) { dataPoint in
                    AreaMark(
                        x: .value("Time", dataPoint.timestamp),
                        y: .value("MB", dataPoint.value)
                    )
                    .foregroundStyle(
                        LinearGradient(
                            colors: [DesignSystem.Colors.accentWarning.opacity(0.5), DesignSystem.Colors.accentWarning.opacity(0.1)],
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
                }
                .frame(height: 150)
                .chartXAxis(.hidden)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
            
            // Memory Breakdown
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("Memory Stats")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorDivider()
                
                StatRow(label: "System Memory Used", value: formatBytes(viewModel.snapshot.memoryUsed))
                StatRow(label: "System Memory Peak", value: formatBytes(viewModel.snapshot.memoryPeak))
                StatRow(label: "GPU Memory Used", value: formatBytes(viewModel.snapshot.gpuMemoryUsed))
                StatRow(label: "GPU Memory Peak", value: formatBytes(viewModel.snapshot.gpuMemoryPeak))
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
        }
        .padding(DesignSystem.Spacing.md)
    }
    
    func formatBytes(_ bytes: UInt64) -> String {
        let mb = Double(bytes) / 1024 / 1024
        return String(format: "%.1f MB", mb)
    }
}

// MARK: - Rendering View

private struct RenderingView: View {
    @ObservedObject var viewModel: ProfilerViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            // Draw Calls
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("Draw Calls")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorDivider()
                
                StatRow(label: "Total Draw Calls", value: "\(viewModel.snapshot.drawCalls)")
                StatRow(label: "Triangles", value: "\(viewModel.snapshot.triangles)")
                StatRow(label: "Vertices", value: "\(viewModel.snapshot.vertices)")
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
            
            // Shader & Texture Stats
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("Resources")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorDivider()
                
                StatRow(label: "Shader Switches", value: "\(viewModel.snapshot.shaderSwitches)")
                StatRow(label: "Texture Switches", value: "\(viewModel.snapshot.textureSwitches)")
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
        }
        .padding(DesignSystem.Spacing.md)
    }
}

// MARK: - Systems View

private struct SystemsView: View {
    @ObservedObject var viewModel: ProfilerViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.lg) {
            // Detailed Samples
            ForEach(viewModel.samples, id: \.name) { sample in
                SystemRow(sample: sample)
            }
            
            // Breakdown Chart
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("System Breakdown")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                EditorDivider()
                
                SystemBarRow(label: "Physics", timeMs: viewModel.snapshot.physicsTime, totalMs: viewModel.snapshot.totalFrameTime, color: .orange)
                SystemBarRow(label: "Render", timeMs: viewModel.snapshot.renderTime, totalMs: viewModel.snapshot.totalFrameTime, color: .blue)
                SystemBarRow(label: "Script", timeMs: viewModel.snapshot.scriptTime, totalMs: viewModel.snapshot.totalFrameTime, color: .purple)
                SystemBarRow(label: "Audio", timeMs: viewModel.snapshot.audioTime, totalMs: viewModel.snapshot.totalFrameTime, color: .green)
                SystemBarRow(label: "UI", timeMs: viewModel.snapshot.uiTime, totalMs: viewModel.snapshot.totalFrameTime, color: .pink)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
        }
        .padding(DesignSystem.Spacing.md)
    }
}

private struct SystemRow: View {
    let sample: ProfilerAPI.Sample
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            HStack {
                Text(sample.name)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Text(String(format: "%.2fms", sample.timeMs))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(sample.timeMs > 5 ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.accentSuccess)
            }
            
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    Rectangle()
                        .fill(DesignSystem.Colors.backgroundPrimary)
                        .frame(height: 6)
                        .cornerRadius(3)
                    
                    Rectangle()
                        .fill(sample.timeMs > 5 ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.accentSuccess)
                        .frame(width: max(2, geometry.size.width * CGFloat(sample.percentage / 100.0)), height: 6)
                        .cornerRadius(3)
                }
            }
            .frame(height: 6)
            
            HStack(spacing: DesignSystem.Spacing.lg) {
                Text("Count: \(sample.callCount)")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Text("\(String(format: "%.1f%%", sample.percentage))")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
        .padding(DesignSystem.Spacing.md)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(8)
        .padding(.leading, CGFloat(sample.depth) * 16)
    }
}

private struct SystemBarRow: View {
    let label: String
    let timeMs: Float
    let totalMs: Float
    let color: Color
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
                Text("\(String(format: "%.2fms", timeMs))")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    Rectangle()
                        .fill(DesignSystem.Colors.backgroundPrimary)
                        .frame(height: 8)
                        .cornerRadius(4)
                    
                    Rectangle()
                        .fill(color)
                        .frame(width: geometry.size.width * CGFloat(timeMs / totalMs), height: 8)
                        .cornerRadius(4)
                }
            }
            .frame(height: 8)
        }
    }
}

// MARK: - Data Models & Helper

struct ProfilerDataPoint: Identifiable {
    let id = UUID()
    let timestamp: Date
    let value: Double
}



// MARK: - View Model

@MainActor
class ProfilerViewModel: ObservableObject {
    @Published var isRecording = true
    @Published var snapshot = ProfilerAPI.Snapshot(
        cpuFrameTime: 0, gpuFrameTime: 0, totalFrameTime: 0,
        physicsTime: 0, renderTime: 0, scriptTime: 0, audioTime: 0, uiTime: 0,
        drawCalls: 0, triangles: 0, vertices: 0, shaderSwitches: 0, textureSwitches: 0,
        memoryUsed: 0, memoryPeak: 0, gpuMemoryUsed: 0, gpuMemoryPeak: 0,
        fps: 0, avgFrameTime: 0, minFrameTime: 0, maxFrameTime: 0
    )
    
    @Published var samples: [ProfilerAPI.Sample] = []
    
    // History buffers
    @Published var fpsHistory: [ProfilerDataPoint] = []
    @Published var frameTimeHistory: [ProfilerDataPoint] = []
    @Published var memoryHistory: [ProfilerDataPoint] = []
    
    
    init() {
        // Initialize with default update frequency
        ProfilerAPI.shared.setUpdateFrequency(30) // 30Hz for UI updates
    }
    
    
    func toggleRecording() {
        isRecording.toggle()
    }
    
    func updateData() {
        guard isRecording else { return }
        
        // Get latest API data
        snapshot = ProfilerAPI.shared.getSnapshot()
        samples = ProfilerAPI.shared.getSamples()
        
        let now = Date()
        
        // Update history buffers
        fpsHistory.append(ProfilerDataPoint(timestamp: now, value: Double(snapshot.fps)))
        if fpsHistory.count > 100 { fpsHistory.removeFirst() }
        
        frameTimeHistory.append(ProfilerDataPoint(timestamp: now, value: Double(snapshot.totalFrameTime)))
        if frameTimeHistory.count > 100 { frameTimeHistory.removeFirst() }
        
        memoryHistory.append(ProfilerDataPoint(timestamp: now, value: Double(snapshot.memoryUsed) / 1024 / 1024))
        if memoryHistory.count > 100 { memoryHistory.removeFirst() }
    }
    
    func clearData() {
        fpsHistory.removeAll()
        frameTimeHistory.removeAll()
        memoryHistory.removeAll()
        ProfilerAPI.shared.resetStats()
        
        // Force an immediate update
        updateData()
    }
}

// MARK: - Preview

#Preview {
    ProfilerPanel()
        .frame(width: 800, height: 600)
}
