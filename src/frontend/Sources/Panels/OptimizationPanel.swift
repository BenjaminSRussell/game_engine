import SwiftUI
import Charts

// typealias LodType = LodAPI.LodType // LodAPI not available yet
typealias OcclusionMode = OcclusionAPI.Mode

// MARK: - Optimization Panel (General)

struct OptimizationPanel: View {
    @StateObject private var viewModel = OptimizationViewModel()
    @State private var selectedTab: OptTab = .occlusion
    
    enum OptTab: String, CaseIterable {
        case occlusion = "Occlusion Culling"
        case memory = "Memory"
        case gpu = "GPU"
        case threads = "Threading"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            HStack(spacing: 0) {
                ForEach(OptTab.allCases, id: \.self) { tab in
                    Button(action: { selectedTab = tab }) {
                        VStack(spacing: 8) {
                            Text(tab.rawValue)
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(selectedTab == tab ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                            
                            Rectangle()
                                .fill(selectedTab == tab ? DesignSystem.Colors.accentPrimary : Color.clear)
                                .frame(height: 2)
                        }
                        .padding(.horizontal, DesignSystem.Spacing.md)
                        .padding(.top, DesignSystem.Spacing.sm)
                    }
                    .buttonStyle(.plain)
                    .frame(maxWidth: .infinity)
                }
            }
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Content
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.lg) {
                    switch selectedTab {
                    case .occlusion:
                        OcclusionCullingView(viewModel: viewModel)
                    case .memory, .gpu, .threads:
                        HStack {
                            Image(systemName: "wrench.and.screwdriver")
                            Text("Coming in Phase 7")
                        }
                        .font(DesignSystem.Typography.title2)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .frame(height: 300)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
}

// MARK: - Sub-Views

struct OcclusionCullingView: View {
    @ObservedObject var viewModel: OptimizationViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            // Mode Selection
            EditorCollapsibleSection("Culling Strategy", isExpanded: true) {
                Picker("Mode", selection: $viewModel.mode) {
                    Text("Disabled (Debug Only)").tag(OcclusionMode.none)
                    Text("Portal-Based (Indoor)").tag(OcclusionMode.portal)
                    Text("Hardware Queries (Outdoor)").tag(OcclusionMode.hardware)
                }
                .pickerStyle(.menu)
                
                Text(modeDescription)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .fixedSize(horizontal: false, vertical: true)
            }
            
            if viewModel.mode != .none {
                // Live Stats
                EditorCollapsibleSection("Visibility Statistics", isExpanded: true) {
                    HStack(spacing: 20) {
                        StatCircle(
                            label: "Visible",
                            value: viewModel.visibleObjects,
                            color: .green,
                            total: viewModel.totalObjects
                        )
                        
                        StatCircle(
                            label: "Culled",
                            value: viewModel.culledObjects,
                            color: .red,
                            total: viewModel.totalObjects
                        )
                    }
                    
                    Text("Cull Rate: \(String(format: "%.1f", viewModel.cullRate * 100))%")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(viewModel.cullRate > 0.5 ? .green : .orange)
                }
                
                // Visualization
                EditorCollapsibleSection("Debug Visualization", isExpanded: true) {
                    Toggle("Show Occlusion Buffer", isOn: $viewModel.debugVisEnabled)
                    Toggle("Freeze Culling", isOn: $viewModel.freezeCulling)
                }
            }
            
            // Manual Occluders
            EditorCollapsibleSection("Manual Occluders", isExpanded: true) {
                EditorButton("Add Selected as Occluder") {
                    viewModel.addSelectedAsOccluder()
                }
                
                List(viewModel.manualOccluders, id: \.self) { id in
                    HStack {
                        Text("Entity #\(id)")
                        Spacer()
                        EditorIconButton(icon: "trash", tooltip: "Remove Occluder") {
                            viewModel.removeOccluder(id)
                        }
                    }
                }
                .frame(height: 150)
            }
        }
    }
    
    var modeDescription: String {
        switch viewModel.mode {
        case .none:
            return "Renders everything in frustum. Use only for debugging or very simple scenes."
        case .portal:
            return "Uses cells and portals to determine visibility. Best for indoor environments and dungeons."
        case .hardware:
            return "Uses GPU occlusion queries. Best for complex outdoor environments with large blockers."
        }
    }
}

struct StatCircle: View {
    let label: String
    let value: Int
    let color: Color
    let total: Int
    
    var ratio: Float {
        return total > 0 ? Float(value) / Float(total) : 0
    }
    
    var body: some View {
        VStack {
            ZStack {
                Circle()
                    .stroke(DesignSystem.Colors.backgroundTertiary, lineWidth: 8)
                
                Circle()
                    .trim(from: 0, to: CGFloat(ratio))
                    .stroke(color, style: StrokeStyle(lineWidth: 8, lineCap: .round))
                    .rotationEffect(.degrees(-90))
                
                VStack {
                    Text("\(value)")
                        .font(DesignSystem.Typography.title2)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                }
            }
            .frame(width: 80, height: 80)
            
            Text(label)
                .font(DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textSecondary)
        }
    }
}

// MARK: - View Model

class OptimizationViewModel: ObservableObject {
    @Published var mode: OcclusionMode = .none {
        didSet { OcclusionAPI.shared.mode = mode }
    }
    
    @Published var visibleObjects: Int = 0
    @Published var culledObjects: Int = 0
    @Published var manualOccluders: [UInt64] = []
    
    @Published var debugVisEnabled: Bool = false
    @Published var freezeCulling: Bool = false
    
    private var timer: Timer?
    
    var totalObjects: Int { visibleObjects + culledObjects }
    var cullRate: Float { totalObjects > 0 ? Float(culledObjects) / Float(totalObjects) : 0 }
    
    init() {
        mode = OcclusionAPI.shared.mode
        startMonitoring()
    }
    
    func startMonitoring() {
        timer = Timer.scheduledTimer(withTimeInterval: 0.2, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            self.visibleObjects = Int(OcclusionAPI.shared.visibleCount)
            self.culledObjects = Int(OcclusionAPI.shared.culledCount)
        }
    }
    
    func addSelectedAsOccluder() {
        // Mock: Add currently selected entity ID
        let mockId = UInt64.random(in: 1...1000)
        OcclusionAPI.shared.addOccluder(mockId)
        manualOccluders.append(mockId)
    }
    
    func removeOccluder(_ id: UInt64) {
        OcclusionAPI.shared.removeOccluder(id)
        manualOccluders.removeAll { $0 == id }
    }
}
