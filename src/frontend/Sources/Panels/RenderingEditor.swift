import SwiftUI

// MARK: - Rendering Editor

class RenderSettingsManager: ObservableObject {
    static let shared = RenderSettingsManager()
    
    @Published var antiAliasing: AntiAliasingMode = .taa
    @Published var resolutionScale: Float = 1.0
    @Published var shadowQuality: QualityLevel = .high
    @Published var reflectionQuality: QualityLevel = .medium
    @Published var textureQuality: QualityLevel = .high
    @Published var vsyncEnabled: Bool = true
    @Published var frameRateLimit: Int = 60
    
    // Post Processing
    @Published var bloomEnabled: Bool = true
    @Published var bloomIntensity: Float = 1.0
    @Published var bloomThreshold: Float = 0.8
    @Published var vignetteEnabled: Bool = true
    @Published var vignetteIntensity: Float = 0.3
    @Published var motionBlurEnabled: Bool = false
    @Published var ambientOcclusionEnabled: Bool = true
    
    enum AntiAliasingMode: String, CaseIterable {
        case none = "None"
        case fxaa = "FXAA"
        case smaa = "SMAA"
        case taa = "TAA"
        case msaa2x = "MSAA 2x"
        case msaa4x = "MSAA 4x"
        case msaa8x = "MSAA 8x"
    }
    
    enum QualityLevel: String, CaseIterable {
        case low = "Low"
        case medium = "Medium"
        case high = "High"
        case ultra = "Ultra"
    }
}

struct RenderingEditorView: View {
    @ObservedObject var manager = RenderSettingsManager.shared
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Rendering")
                    .font(DesignSystem.Typography.h2)
                
                Spacer()
                
                Picker("", selection: $selectedTab) {
                    Text("General").tag(0)
                    Text("Post Process").tag(1)
                    Text("Shaders").tag(2)
                }
                .pickerStyle(.segmented)
                .frame(width: 300)
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.lg) {
                    if selectedTab == 0 {
                        GeneralRenderSettings(manager: manager)
                    } else if selectedTab == 1 {
                        PostProcessSettings(manager: manager)
                    } else {
                        ShaderList()
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct GeneralRenderSettings: View {
    @ObservedObject var manager: RenderSettingsManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Quality", isExpanded: true) {
                VStack(spacing: DesignSystem.Spacing.md) {
                    HStack {
                        Text("Anti-Aliasing")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        Spacer()
                        Picker("", selection: $manager.antiAliasing) {
                            ForEach(RenderSettingsManager.AntiAliasingMode.allCases, id: \.self) { mode in
                                Text(mode.rawValue).tag(mode)
                            }
                        }
                        .frame(width: 150)
                    }
                    
                    EditorNumericField(label: "Resolution Scale", value: $manager.resolutionScale, range: 0.5...2.0, step: 0.1)
                    
                    HStack {
                        Text("Shadow Quality")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        Spacer()
                        Picker("", selection: $manager.shadowQuality) {
                            ForEach(RenderSettingsManager.QualityLevel.allCases, id: \.self) { level in
                                Text(level.rawValue).tag(level)
                            }
                        }
                        .frame(width: 150)
                    }
                    
                    HStack {
                        Text("Reflection Quality")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        Spacer()
                        Picker("", selection: $manager.reflectionQuality) {
                            ForEach(RenderSettingsManager.QualityLevel.allCases, id: \.self) { level in
                                Text(level.rawValue).tag(level)
                            }
                        }
                        .frame(width: 150)
                    }
                }
            }
            
            EditorCollapsibleSection("Display", isExpanded: true) {
                VStack(spacing: DesignSystem.Spacing.md) {
                    Toggle("VSync", isOn: $manager.vsyncEnabled)
                        .toggleStyle(EditorToggleStyle())
                    
                    EditorNumericField(label: "Frame Rate Limit", value: Binding(
                        get: { Float(manager.frameRateLimit) },
                        set: { manager.frameRateLimit = Int($0) }
                    ), range: 30...240, step: 10)
                }
            }
        }
    }
}

struct PostProcessSettings: View {
    @ObservedObject var manager: RenderSettingsManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Bloom", isExpanded: true) {
                VStack(spacing: DesignSystem.Spacing.md) {
                    Toggle("Enabled", isOn: $manager.bloomEnabled)
                        .toggleStyle(EditorToggleStyle())
                    
                    if manager.bloomEnabled {
                        EditorNumericField(label: "Intensity", value: $manager.bloomIntensity, range: 0...5, step: 0.1)
                        EditorNumericField(label: "Threshold", value: $manager.bloomThreshold, range: 0...1, step: 0.05)
                    }
                }
            }
            
            EditorCollapsibleSection("Vignette", isExpanded: true) {
                VStack(spacing: DesignSystem.Spacing.md) {
                    Toggle("Enabled", isOn: $manager.vignetteEnabled)
                        .toggleStyle(EditorToggleStyle())
                    
                    if manager.vignetteEnabled {
                        EditorNumericField(label: "Intensity", value: $manager.vignetteIntensity, range: 0...1, step: 0.05)
                    }
                }
            }
            
            EditorCollapsibleSection("Ambient Occlusion", isExpanded: true) {
                Toggle("Enabled", isOn: $manager.ambientOcclusionEnabled)
                    .toggleStyle(EditorToggleStyle())
            }
            
            EditorCollapsibleSection("Motion Blur", isExpanded: true) {
                Toggle("Enabled", isOn: $manager.motionBlurEnabled)
                    .toggleStyle(EditorToggleStyle())
            }
        }
    }
}

struct ShaderList: View {
    // Placeholder for shader list
    var body: some View {
        VStack(alignment: .leading) {
            Text("Active Shaders")
                .font(DesignSystem.Typography.bodyBold)
            
            ForEach(0..<5) { i in
                HStack {
                    Image(systemName: "fx")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                    VStack(alignment: .leading) {
                        Text("Shader_Default_\(i)")
                            .font(DesignSystem.Typography.body)
                        Text("Compiled • 12ms")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    Spacer()
                    Button("Edit") { }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundPrimary)
                .cornerRadius(4)
            }
        }
    }
}

// Helper for EditorToggleStyle if not defined elsewhere, or use standard
struct EditorToggleStyle: ToggleStyle {
    func makeBody(configuration: Configuration) -> some View {
        HStack {
            configuration.label
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            Spacer()
            Toggle("", isOn: configuration.$isOn)
                .labelsHidden()
        }
    }
}
