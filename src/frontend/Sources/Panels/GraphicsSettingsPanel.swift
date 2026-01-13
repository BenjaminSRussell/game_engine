import SwiftUI

// MARK: - Graphics Settings Panel

struct GraphicsSettingsPanel: View {
    @StateObject private var viewModel = GraphicsSettingsViewModel()
    @State private var selectedTab: SettingsTab = .display
    
    enum SettingsTab: String, CaseIterable {
        case display = "Display"
        case quality = "Quality"
        case advanced = "Advanced"
        case performance = "Performance"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            HStack(spacing: 0) {
                ForEach(SettingsTab.allCases, id: \.self) { tab in
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
                    case .display:
                        DisplaySettingsView(viewModel: viewModel)
                    case .quality:
                        QualitySettingsView(viewModel: viewModel)
                    case .advanced:
                        AdvancedGraphicsView(viewModel: viewModel)
                    case .performance:
                        PerformanceSettingsView(viewModel: viewModel)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
            
            EditorDivider()
            
            // Action Buttons
            HStack {
                EditorButton("Reset to Default", icon: "arrow.counterclockwise") {
                    viewModel.resetToDefaults()
                }
                
                Spacer()
                
                EditorButton("Apply", icon: "checkmark") {
                    viewModel.applySettings()
                }
                .buttonStyle(.borderedProminent)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
        }
    }
}

// MARK: - Display Settings View

private struct DisplaySettingsView: View {
    @ObservedObject var viewModel: GraphicsSettingsViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Resolution & Display Mode", isExpanded: true) {
                Picker("Resolution", selection: $viewModel.resolution) {
                    ForEach(Resolution.allCases, id: \.self) { res in
                        Text(res.displayString).tag(res)
                    }
                }
                .pickerStyle(.menu)
                
                Picker("Display Mode", selection: $viewModel.displayMode) {
                    Text("Windowed").tag(DisplayMode.windowed)
                    Text("Fullscreen").tag(DisplayMode.fullscreen)
                    Text("Borderless Window").tag(DisplayMode.borderless)
                }
                .pickerStyle(.segmented)
                
                Toggle("VSync", isOn: $viewModel.vsyncEnabled)
                    .toggleStyle(EditorToggleStyle())
                
                EditorNumericField(
                    label: "Frame Rate Limit",
                    value: Binding(
                        get: { Float(viewModel.frameRateLimit) },
                        set: { viewModel.frameRateLimit = Int($0) }
                    ),
                    range: 30...300,
                    step: 10
                )
            }
            
            EditorCollapsibleSection("Monitor Settings", isExpanded: true) {
                Picker("Target Monitor", selection: $viewModel.targetMonitor) {
                    ForEach(0..<viewModel.availableMonitors.count, id: \.self) { index in
                        Text("Monitor \(index + 1)").tag(index)
                    }
                }
                
                EditorNumericField(
                    label: "Brightness",
                    value: $viewModel.brightness,
                    range: 0...2,
                    step: 0.1
                )
                
                EditorNumericField(
                    label: "Gamma",
                    value: $viewModel.gamma,
                    range: 0.5...3,
                    step: 0.1
                )
            }
        }
    }
}

// MARK: - Quality Settings View

private struct QualitySettingsView: View {
    @ObservedObject var viewModel: GraphicsSettingsViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Quality Preset", isExpanded: true) {
                Picker("Overall Quality", selection: $viewModel.qualityPreset) {
                    ForEach(QualityPreset.allCases, id: \.self) { preset in
                        Text(preset.rawValue).tag(preset)
                    }
                }
                .pickerStyle(.segmented)
                .onChange(of: viewModel.qualityPreset) { preset in
                    viewModel.applyQualityPreset(preset)
                }
                
                Text("Applies recommended settings for \(viewModel.qualityPreset.rawValue.lowercased()) performance")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            EditorCollapsibleSection("Texture Quality", isExpanded: true) {
                Picker("Texture Resolution", selection: $viewModel.textureQuality) {
                    Text("Low").tag(0)
                    Text("Medium").tag(1)
                    Text("High").tag(2)
                    Text("Ultra").tag(3)
                }
                
                Toggle("Anisotropic Filtering", isOn: $viewModel.anisotropicFiltering)
                    .toggleStyle(EditorToggleStyle())
                
                if viewModel.anisotropicFiltering {
                    Picker("AF Level", selection: $viewModel.anisotropicLevel) {
                        Text("2x").tag(2)
                        Text("4x").tag(4)
                        Text("8x").tag(8)
                        Text("16x").tag(16)
                    }
                    .pickerStyle(.segmented)
                }
            }
            
            EditorCollapsibleSection("Shadow Quality", isExpanded: true) {
                Picker("Shadow Quality", selection: $viewModel.shadowQuality) {
                    Text("Low").tag(0)
                    Text("Medium").tag(1)
                    Text("High").tag(2)
                    Text("Ultra").tag(3)
                }
                
                Toggle("Soft Shadows", isOn: $viewModel.softShadows)
                    .toggleStyle(EditorToggleStyle())
            }
            
            EditorCollapsibleSection("Anti-Aliasing", isExpanded: true) {
                Picker("AA Method", selection: $viewModel.antiAliasingMode) {
                    Text("Off").tag("Off")
                    Text("FXAA").tag("FXAA")
                    Text("TAA").tag("TAA")
                    Text("MSAA 2x").tag("MSAA2x")
                    Text("MSAA 4x").tag("MSAA4x")
                    Text("MSAA 8x").tag("MSAA8x")
                }
                
                if viewModel.antiAliasingMode == "TAA" {
                    EditorNumericField(
                        label: "TAA Sharpness",
                        value: $viewModel.taaSharpness,
                        range: 0...1,
                        step: 0.05
                    )
                }
            }
        }
    }
}

// MARK: - Advanced Graphics View

private struct AdvancedGraphicsView: View {
    @ObservedObject var viewModel: GraphicsSettingsViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Lighting & Shadows", isExpanded: true) {
                EditorNumericField(
                    label: "Shadow Distance",
                    value: $viewModel.shadowDistance,
                    range: 50...500,
                    step: 10
                )
                
                Picker("Shadow Cascades", selection: $viewModel.shadowCascades) {
                    Text("2").tag(2)
                    Text("3").tag(3)
                    Text("4").tag(4)
                }
                
                Toggle("Contact Shadows", isOn: $viewModel.contactShadows)
                    .toggleStyle(EditorToggleStyle())
            }
            
            EditorCollapsibleSection("Ray Tracing", isExpanded: true) {
                Toggle("Ray Traced Reflections", isOn: $viewModel.rayTracedReflections)
                    .toggleStyle(EditorToggleStyle())
                
                Toggle("Ray Traced Shadows", isOn: $viewModel.rayTracedShadows)
                    .toggleStyle(EditorToggleStyle())
                
                Toggle("Ray Traced Ambient Occlusion", isOn: $viewModel.rayTracedAO)
                    .toggleStyle(EditorToggleStyle())
                
                if viewModel.rayTracedReflections || viewModel.rayTracedShadows || viewModel.rayTracedAO {
                    EditorNumericField(
                        label: "Samples Per Pixel",
                        value: Binding(
                            get: { Float(viewModel.rayTracingSamples) },
                            set: { viewModel.rayTracingSamples = Int($0) }
                        ),
                        range: 1...16,
                        step: 1
                    )
                }
            }
            
            EditorCollapsibleSection("Post Processing", isExpanded: true) {
                Toggle("Bloom", isOn: $viewModel.bloomEnabled)
                    .toggleStyle(EditorToggleStyle())
                
                if viewModel.bloomEnabled {
                    EditorNumericField(
                        label: "Bloom Intensity",
                        value: $viewModel.bloomIntensity,
                        range: 0...2,
                        step: 0.1
                    )
                }
                
                Toggle("Motion Blur", isOn: $viewModel.motionBlur)
                    .toggleStyle(EditorToggleStyle())
                
                Toggle("Depth of Field", isOn: $viewModel.depthOfField)
                    .toggleStyle(EditorToggleStyle())
                
                Toggle("Chromatic Aberration", isOn: $viewModel.chromaticAberration)
                    .toggleStyle(EditorToggleStyle())
            }
            
            EditorCollapsibleSection("Ambient Occlusion", isExpanded: true) {
                Picker("AO Method", selection: $viewModel.aoMethod) {
                    Text("Off").tag("Off")
                    Text("SSAO").tag("SSAO")
                    Text("HBAO").tag("HBAO")
                }
                
                if viewModel.aoMethod != "Off" {
                    EditorNumericField(
                        label: "AO Intensity",
                        value: $viewModel.aoIntensity,
                        range: 0...2,
                        step: 0.1
                    )
                }
            }
        }
    }
}

// MARK: - Performance Settings View

private struct PerformanceSettingsView: View {
    @ObservedObject var viewModel: GraphicsSettingsViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Performance Metrics", isExpanded: true) {
                Toggle("Show FPS Counter", isOn: $viewModel.showFPS)
                    .toggleStyle(EditorToggleStyle())
                
                Toggle("Show Frame Time Graph", isOn: $viewModel.showFrameGraph)
                    .toggleStyle(EditorToggleStyle())
                
                Toggle("Show GPU Stats", isOn: $viewModel.showGPUStats)
                    .toggleStyle(EditorToggleStyle())
            }
            
            EditorCollapsibleSection("LOD Settings", isExpanded: true) {
                EditorNumericField(
                    label: "LOD Bias",
                    value: $viewModel.lodBias,
                    range: -2...2,
                    step: 0.1
                )
                
                EditorNumericField(
                    label: "View Distance",
                    value: $viewModel.viewDistance,
                    range: 100...1000,
                    step: 50
                )
            }
            
            EditorCollapsibleSection("Culling", isExpanded: true) {
                Toggle("Frustum Culling", isOn: $viewModel.frustumCulling)
                    .toggleStyle(EditorToggleStyle())
                
                Toggle("Occlusion Culling", isOn: $viewModel.occlusionCulling)
                    .toggleStyle(EditorToggleStyle())
            }
            
            EditorCollapsibleSection("Threading", isExpanded: true) {
                Picker("Worker Threads", selection: $viewModel.workerThreads) {
                    Text("Auto").tag(0)
                    ForEach(1...16, id: \.self) { count in
                        Text("\(count)").tag(count)
                    }
                }
                
                Toggle("Multi-threaded Rendering", isOn: $viewModel.multiThreadedRendering)
                    .toggleStyle(EditorToggleStyle())
            }
        }
    }
}

// MARK: - Data Models

enum DisplayMode: String {
    case windowed = "Windowed"
    case fullscreen = "Fullscreen"
    case borderless = "Borderless"
}

enum Resolution: CaseIterable {
    case r1280x720
    case r1920x1080
    case r2560x1440
    case r3840x2160
    
    var displayString: String {
        switch self {
        case .r1280x720: return "1280 × 720"
        case .r1920x1080: return "1920 × 1080"
        case .r2560x1440: return "2560 × 1440"
        case .r3840x2160: return "3840 × 2160"
        }
    }
}

enum QualityPreset: String, CaseIterable {
    case low = "Low"
    case medium = "Medium"
    case high = "High"
    case ultra = "Ultra"
    case custom = "Custom"
}

// MARK: - View Model

class GraphicsSettingsViewModel: ObservableObject {
    // Display
    @Published var resolution: Resolution = .r1920x1080
    @Published var displayMode: DisplayMode = .windowed
    @Published var vsyncEnabled: Bool = true
    @Published var frameRateLimit: Int = 144
    @Published var targetMonitor: Int = 0
    @Published var brightness: Float = 1.0
    @Published var gamma: Float = 2.2
    
    let availableMonitors = [0, 1] // Mock data
    
    // Quality
    @Published var qualityPreset: QualityPreset = .high
    @Published var textureQuality: Int = 2
    @Published var anisotropicFiltering: Bool = true
    @Published var anisotropicLevel: Int = 8
    @Published var shadowQuality: Int = 2
    @Published var softShadows: Bool = true
    @Published var antiAliasingMode: String = "TAA"
    @Published var taaSharpness: Float = 0.5
    
    // Advanced
    @Published var shadowDistance: Float = 200
    @Published var shadowCascades: Int = 4
    @Published var contactShadows: Bool = true
    @Published var rayTracedReflections: Bool = false
    @Published var rayTracedShadows: Bool = false
    @Published var rayTracedAO: Bool = false
    @Published var rayTracingSamples: Int = 1
    @Published var bloomEnabled: Bool = true
    @Published var bloomIntensity: Float = 0.5
    @Published var motionBlur: Bool = false
    @Published var depthOfField: Bool = false
    @Published var chromaticAberration: Bool = false
    @Published var aoMethod: String = "HBAO"
    @Published var aoIntensity: Float = 1.0
    
    // Performance
    @Published var showFPS: Bool = true
    @Published var showFrameGraph: Bool = false
    @Published var showGPUStats: Bool = false
    @Published var lodBias: Float = 0.0
    @Published var viewDistance: Float = 500
    @Published var frustumCulling: Bool = true
    @Published var occlusionCulling: Bool = true
    @Published var workerThreads: Int = 0
    @Published var multiThreadedRendering: Bool = true
    
    func applyQualityPreset(_ preset: QualityPreset) {
        switch preset {
        case .low:
            textureQuality = 0
            shadowQuality = 0
            softShadows = false
            antiAliasingMode = "Off"
            shadowDistance = 50
            bloomEnabled = false
            aoMethod = "Off"
        case .medium:
            textureQuality = 1
            shadowQuality = 1
            softShadows = false
            antiAliasingMode = "FXAA"
            shadowDistance = 100
            bloomEnabled = true
            aoMethod = "SSAO"
        case .high:
            textureQuality = 2
            shadowQuality = 2
            softShadows = true
            antiAliasingMode = "TAA"
            shadowDistance = 200
            bloomEnabled = true
            aoMethod = "HBAO"
        case .ultra:
            textureQuality = 3
            shadowQuality = 3
            softShadows = true
            antiAliasingMode = "TAA"
            shadowDistance = 300
            bloomEnabled = true
            aoMethod = "HBAO"
            rayTracedReflections = true
        case .custom:
            break
        }
    }
    
    func applySettings() {
        print("[Swift] Applying graphics settings...")
        // Would call engine bridge here
    }
    
    func resetToDefaults() {
        qualityPreset = .high
        applyQualityPreset(.high)
    }
}
