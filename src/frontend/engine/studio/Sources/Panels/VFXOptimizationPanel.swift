import SwiftUI
import Charts

// MARK: - VFX Optimization Panel

struct VFXOptimizationPanel: View {
    @StateObject private var viewModel = VFXOptimizationViewModel()
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("VFX Performance Monitor")
                    .font(DesignSystem.Typography.title1)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
                LiveIndicator()
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.lg) {
                    
                    // 1. Performance Controls
                    VStack(spacing: DesignSystem.Spacing.md) {
                        Text("Optimization Strategy")
                            .font(DesignSystem.Typography.sectionLabel)
                            .frame(maxWidth: .infinity, alignment: .leading)
                        
                        // Distance Culling
                        EditorCollapsibleSection("Distance Culling", isExpanded: true) {
                            Toggle("Enabled", isOn: $viewModel.cullingEnabled)
                            
                            if viewModel.cullingEnabled {
                                EditorNumericField(
                                    label: "Max Distance", 
                                    value: $viewModel.cullingDistance, 
                                    range: 10...1000, 
                                    step: 10
                                )
                                Text("Effects beyond \(Int(viewModel.cullingDistance))m are culled")
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                        }
                        
                        // LOD Bias
                        EditorCollapsibleSection("Level of Detail (LOD)", isExpanded: true) {
                            VStack(alignment: .leading) {
                                Text(viewModel.lodBiasLabel)
                                    .font(DesignSystem.Typography.bodyBold)
                                    .foregroundColor(viewModel.lodBiasColor)
                                
                                Slider(value: $viewModel.lodBias, in: -2...2, step: 0.1)
                                    .accentColor(viewModel.lodBiasColor)
                                
                                HStack {
                                    Text("Performance")
                                    Spacer()
                                    Text("Quality")
                                }
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                        }
                        
                        // Particle Budget
                        EditorCollapsibleSection("Particle Budget", isExpanded: true) {
                            EditorNumericField(
                                label: "Max Particles", 
                                value: $viewModel.maxParticles, 
                                range: 1000...500000, 
                                step: 1000
                            )
                            
                            VStack(alignment: .leading, spacing: 4) {
                                HStack {
                                    Text("Usage")
                                    Spacer()
                                    Text("\(viewModel.activeParticles) / \(Int(viewModel.maxParticles))")
                                }
                                .font(DesignSystem.Typography.caption)
                                
                                GeometryReader { geometry in
                                    ZStack(alignment: .leading) {
                                        Rectangle()
                                            .fill(DesignSystem.Colors.backgroundTertiary)
                                            .frame(height: 8)
                                            .cornerRadius(4)
                                        
                                        Rectangle()
                                            .fill(viewModel.budgetColor)
                                            .frame(width: geometry.size.width * CGFloat(viewModel.budgetUsage), height: 8)
                                            .cornerRadius(4)
                                    }
                                }
                                .frame(height: 8)
                            }
                        }
                        
                        // GPU Instancing
                        EditorCollapsibleSection("Hardware Acceleration", isExpanded: true) {
                            Toggle("GPU Instancing", isOn: $viewModel.instancingEnabled)
                            Text("Batches particles to reduce draw calls")
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                    
                    EditorDivider()
                    
                    // 2. Real-Time Statistics
                    VStack(spacing: DesignSystem.Spacing.md) {
                        Text("Live Telemetry")
                            .font(DesignSystem.Typography.sectionLabel)
                            .frame(maxWidth: .infinity, alignment: .leading)
                        
                        // Charts
                        if #available(macOS 14.0, *) {
                           VStack(spacing: 16) {
                               ChartHeader(title: "Active Particles")
                               Chart(viewModel.particleHistory) { point in
                                   LineMark(
                                       x: .value("Time", point.time),
                                       y: .value("Count", point.count)
                                   )
                                   .interpolationMethod(.catmullRom)
                                   .foregroundStyle(DesignSystem.Colors.accentPrimary)
                                   
                                   AreaMark(
                                       x: .value("Time", point.time),
                                       y: .value("Count", point.count)
                                   )
                                   .interpolationMethod(.catmullRom)
                                   .foregroundStyle(
                                       LinearGradient(
                                           colors: [DesignSystem.Colors.accentPrimary.opacity(0.3), .clear],
                                           startPoint: .top,
                                           endPoint: .bottom
                                       )
                                    )
                               }
                               .chartXAxis(.hidden)
                               .frame(height: 150)
                               .background(DesignSystem.Colors.backgroundTertiary)
                               .cornerRadius(8)
                               .padding(8)
                               
                               ChartHeader(title: "Frame Time Impact (ms)")
                               Chart(viewModel.frameTimeHistory) { point in
                                   BarMark(
                                       x: .value("Time", point.time),
                                       y: .value("MS", point.value)
                                   )
                                   .foregroundStyle(point.value > 16.6 ? Color.red : Color.green)
                               }
                               .chartXAxis(.hidden)
                               .frame(height: 100)
                               .background(DesignSystem.Colors.backgroundTertiary)
                               .cornerRadius(8)
                               .padding(8)
                           }
                        } else {
                            Text("Charts require macOS 14.0+")
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        
                        // Breakdown
                        Grid(horizontalSpacing: 20, verticalSpacing: 10) {
                            GridRow {
                                StatBox(label: "Draw Calls", value: "\(viewModel.drawCalls)", icon: "square.stack.3d.up")
                                StatBox(label: "Culled", value: "\(viewModel.culledCount)", icon: "eye.slash")
                            }
                            GridRow {
                                StatBox(label: "GPU Memory", value: viewModel.memoryString, icon: "memorychip")
                                StatBox(label: "FPS", value: "\(Int(viewModel.fps))", icon: "gauge.medium")
                            }
                        }
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .onAppear { viewModel.startMonitoring() }
        .onDisappear { viewModel.stopMonitoring() }
    }
}

// MARK: - Subcomponents

private struct LiveIndicator: View {
    @State private var isBlinking = false
    
    var body: some View {
        HStack(spacing: 6) {
            Circle()
                .fill(Color.green)
                .frame(width: 8, height: 8)
                .opacity(isBlinking ? 1 : 0.4)
                .animation(Animation.easeInOut(duration: 0.8).repeatForever(), value: isBlinking)
            
            Text("LIVE")
                .font(DesignSystem.Typography.smallBold)
                .foregroundColor(Color.green)
        }
        .onAppear { isBlinking = true }
    }
}

private struct ChartHeader: View {
    let title: String
    
    var body: some View {
        HStack {
            Text(title)
                .font(DesignSystem.Typography.smallBold)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            Spacer()
        }
        .padding(.horizontal, 8)
    }
}

private struct StatBox: View {
    let label: String
    let value: String
    let icon: String
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: icon)
                    .foregroundColor(DesignSystem.Colors.accentSecondary)
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Text(value)
                .font(.system(size: 24, weight: .bold, design: .monospaced))
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding()
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - View Model

struct DataPoint: Identifiable {
    let id = UUID()
    let time: Date
    let count: Int
}

struct FloatDataPoint: Identifiable {
    let id = UUID()
    let time: Date
    let value: Float
}

class VFXOptimizationViewModel: ObservableObject {
    // Controls
    @Published var cullingEnabled: Bool = true {
        didSet { VFXOptimizationAPI.shared.cullingEnabled = cullingEnabled }
    }
    @Published var cullingDistance: Float = 100.0 {
        didSet { VFXOptimizationAPI.shared.cullingDistance = cullingDistance }
    }
    @Published var lodBias: Float = 0.0 {
        didSet { VFXOptimizationAPI.shared.lodBias = lodBias }
    }
    @Published var maxParticles: Float = 50000 {
        didSet { VFXOptimizationAPI.shared.maxParticles = UInt32(maxParticles) }
    }
    @Published var instancingEnabled: Bool = true {
        didSet { VFXOptimizationAPI.shared.instancingEnabled = instancingEnabled }
    }
    
    // Live Stats
    @Published var activeParticles: Int = 0
    @Published var culledCount: Int = 0
    @Published var drawCalls: Int = 0
    @Published var fps: Float = 60.0
    @Published var particleHistory: [DataPoint] = []
    @Published var frameTimeHistory: [FloatDataPoint] = []
    
    private var timer: Timer?
    let historyLimit = 50
    
    // Computed UI Helpers
    
    var lodBiasLabel: String {
        if lodBias < -0.5 { return "Performance Priority" }
        if lodBias > 0.5 { return "Quality Priority" }
        return "Balanced"
    }
    
    var lodBiasColor: Color {
        if lodBias < -0.5 { return .green }
        if lodBias > 0.5 { return .orange }
        return DesignSystem.Colors.accentPrimary
    }
    
    var budgetUsage: Float {
        return Float(activeParticles) / maxParticles
    }
    
    var budgetColor: Color {
        if budgetUsage > 0.9 { return .red }
        if budgetUsage > 0.7 { return .orange }
        return .green
    }
    
    var memoryString: String {
        // Estimate: 64 bytes per particle
        let bytes = activeParticles * 64
        let mb = Float(bytes) / 1024 / 1024
        return String(format: "%.1f MB", mb)
    }
    
    init() {
        // Sync initial state
        cullingEnabled = VFXOptimizationAPI.shared.cullingEnabled
        cullingDistance = VFXOptimizationAPI.shared.cullingDistance
        lodBias = VFXOptimizationAPI.shared.lodBias
        maxParticles = Float(VFXOptimizationAPI.shared.maxParticles)
        instancingEnabled = VFXOptimizationAPI.shared.instancingEnabled
    }
    
    func startMonitoring() {
        timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            self?.updateStats()
        }
    }
    
    func stopMonitoring() {
        timer?.invalidate()
    }
    
    private func updateStats() {
        let currentParticles = Int(VFXOptimizationAPI.shared.activeParticles)
        let currentCulled = 0 // Int(VFXOptimizationAPI.shared.culledEffectCount) // API not available
        let now = Date()
        
        DispatchQueue.main.async {
            self.activeParticles = currentParticles
            self.culledCount = currentCulled
            self.drawCalls = self.calculateDrawCalls(particles: currentParticles) // Mock estimation
            self.fps = Float.random(in: 58...60) // Mock
            
            // Update Charts
            self.particleHistory.append(DataPoint(time: now, count: currentParticles))
            if self.particleHistory.count > self.historyLimit {
                self.particleHistory.removeFirst()
            }
            
            let frameTime = Float.random(in: 14...17) // Mock ms
            self.frameTimeHistory.append(FloatDataPoint(time: now, value: frameTime))
            if self.frameTimeHistory.count > self.historyLimit {
                self.frameTimeHistory.removeFirst()
            }
        }
    }
    
    private func calculateDrawCalls(particles: Int) -> Int {
        if instancingEnabled {
            // One call per emitter type approx
            return 10
        } else {
            // Worst case without instancing
            return particles / 100
        }
    }
}
