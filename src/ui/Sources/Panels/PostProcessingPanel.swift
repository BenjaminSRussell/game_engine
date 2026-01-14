import SwiftUI

// MARK: - Post-Processing Panel

struct PostProcessingPanel: View {
    @StateObject private var viewModel = PostProcessingViewModel()
    @State private var showCompareMode = false
    @State private var dividerPosition: CGFloat = 0.5
    
    var body: some View {
        HStack(spacing: 0) {
            // Left: Controls
            VStack(spacing: 0) {
                // Toolbar
                HStack {
                    Text("Post-Process Stack")
                        .font(DesignSystem.Typography.title1)
                    Spacer()
                    
                    Toggle("Compare", isOn: $showCompareMode)
                        .toggleStyle(.switch)
                        .labelsHidden()
                    Text("Compare")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .padding(DesignSystem.Spacing.md)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                EditorDivider()
                
                // Effect Stack
                ScrollView {
                    VStack(spacing: DesignSystem.Spacing.lg) {
                        // Global Settings
                        EditorCollapsibleSection("Global", isExpanded: true) {
                            HStack {
                                Text("Quality Preset")
                                Spacer()
                                Picker("", selection: $viewModel.qualityPreset) {
                                    Text("Low").tag(0)
                                    Text("Medium").tag(1)
                                    Text("High").tag(2)
                                    Text("Ultra").tag(3)
                                }
                                .frame(width: 100)
                            }
                            Toggle("Tone Mapping", isOn: $viewModel.toneMappingEnabled)
                        }
                        
                        // Effects
                        ForEach($viewModel.effects) { $effect in
                            PostProcessEffectRow(effect: $effect)
                        }
                    }
                    .padding(DesignSystem.Spacing.md)
                }
                .background(DesignSystem.Colors.backgroundPrimary)
            }
            .frame(width: 320)
            
            EditorDivider()
            
            // Right: Preview (Mock for now, would be Metal View)
            ZStack {
                Color.black
                
                if showCompareMode {
                    GeometryReader { geometry in
                        ZStack(alignment: .leading) {
                            // "After" Image (Full width)
                            Image("preview_mock_after") // Asset placeholder
                                .resizable()
                                .aspectRatio(contentMode: .fill)
                                .frame(width: geometry.size.width, height: geometry.size.height)
                                .clipped()
                            
                            // "Before" Image (Masked)
                            Image("preview_mock_before") // Asset placeholder
                                .resizable()
                                .aspectRatio(contentMode: .fill)
                                .frame(width: geometry.size.width, height: geometry.size.height)
                                .clipped()
                                .mask(
                                    Rectangle()
                                        .frame(width: geometry.size.width * dividerPosition)
                                        .frame(maxWidth: .infinity, alignment: .leading)
                                )
                            
                            // Divider Handle
                            Rectangle()
                                .fill(Color.white)
                                .frame(width: 2)
                                .shadow(radius: 2)
                                .offset(x: geometry.size.width * dividerPosition)
                                .gesture(
                                    DragGesture()
                                        .onChanged { value in
                                            dividerPosition = min(max(value.location.x / geometry.size.width, 0), 1)
                                        }
                                )
                            
                            // Labels
                            Text("Original")
                                .font(DesignSystem.Typography.smallBold)
                                .padding(4)
                                .background(Color.black.opacity(0.6))
                                .cornerRadius(4)
                                .padding(8)
                                .position(x: 40, y: geometry.size.height - 20)
                            
                            Text("Processed")
                                .font(DesignSystem.Typography.smallBold)
                                .padding(4)
                                .background(Color.black.opacity(0.6))
                                .cornerRadius(4)
                                .padding(8)
                                .position(x: geometry.size.width - 50, y: geometry.size.height - 20)
                        }
                    }
                } else {
                    // Standard Preview
                    Image(systemName: "camera.viewfinder")
                        .font(.system(size: 48))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Text("Main Camera Preview")
                        .offset(y: 40)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
    }
}

// MARK: - Components

struct PostProcessEffectRow: View {
    @Binding var effect: PostProcessEffectWrapper
    
    var body: some View {
        EditorCollapsibleSection(effect.name, isExpanded: false) {
            Toggle("Enabled", isOn: $effect.isEnabled)
            
            if effect.isEnabled {
                switch effect.type {
                case .motionBlur:
                    MotionBlurControls(effect: $effect)
                case .chromaticAberration:
                    ChromaticAberrationControls(effect: $effect)
                case .filmGrain:
                    FilmGrainControls(effect: $effect)
                case .vignette:
                    VignetteControls(effect: $effect)
                case .bloom:
                    BloomControls(effect: $effect)
                }
            }
        }
    }
}

struct MotionBlurControls: View {
    @Binding var effect: PostProcessEffectWrapper
    
    var body: some View {
        SliderRow(label: "Strength", value: $effect.intensity, range: 0...1)
        Toggle("Camera Only", isOn: $effect.boolParam1)
    }
}

struct ChromaticAberrationControls: View {
    @Binding var effect: PostProcessEffectWrapper
    
    var body: some View {
        SliderRow(label: "Intensity", value: $effect.intensity, range: 0...1)
        ColorPicker("Tint", selection: $effect.colorParam)
    }
}

struct FilmGrainControls: View {
    @Binding var effect: PostProcessEffectWrapper
    
    var body: some View {
        SliderRow(label: "Intensity", value: $effect.intensity, range: 0...1)
        SliderRow(label: "Size", value: $effect.floatParam1, range: 0.5...2.0)
    }
}

struct VignetteControls: View {
    @Binding var effect: PostProcessEffectWrapper
    
    var body: some View {
        SliderRow(label: "Intensity", value: $effect.intensity, range: 0...1)
        SliderRow(label: "Smoothness", value: $effect.floatParam1, range: 0...1)
        ColorPicker("Color", selection: $effect.colorParam)
    }
}

struct BloomControls: View {
    @Binding var effect: PostProcessEffectWrapper
    
    var body: some View {
        SliderRow(label: "Threshold", value: $effect.intensity, range: 0...2)
        SliderRow(label: "Radius", value: $effect.floatParam1, range: 0...10)
    }
}

struct SliderRow: View {
    let label: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                Spacer()
                Text(String(format: "%.2f", value))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            Slider(value: $value, in: range)
        }
    }
}

// MARK: - View Model

enum PostFxType {
    case motionBlur, chromaticAberration, filmGrain, vignette, bloom
}

struct PostProcessEffectWrapper: Identifiable {
    let id = UUID()
    let type: PostFxType
    let name: String
    var isEnabled: Bool = false {
        didSet { updateEngine() }
    }
    var intensity: Float = 0.5 {
        didSet { updateEngine() }
    }
    var floatParam1: Float = 0.5 {
        didSet { updateEngine() }
    }
    var boolParam1: Bool = false {
        didSet { updateEngine() }
    }
    var colorParam: Color = .white {
        didSet { updateEngine() }
    }
    
    func updateEngine() {
        switch type {
        case .motionBlur:
            PostProcessingAPI.shared.motionBlurEnabled = isEnabled
            PostProcessingAPI.shared.motionBlurStrength = intensity
        case .chromaticAberration:
            PostProcessingAPI.shared.chromaticAberrationEnabled = isEnabled
            PostProcessingAPI.shared.chromaticAberrationIntensity = intensity
        case .filmGrain:
            PostProcessingAPI.shared.filmGrainEnabled = isEnabled
            PostProcessingAPI.shared.filmGrainIntensity = intensity
        case .vignette:
            PostProcessingAPI.shared.vignetteEnabled = isEnabled
            PostProcessingAPI.shared.vignetteIntensity = intensity
        case .bloom:
            // Placeholder for future bloom API
            break
        }
    }
}

class PostProcessingViewModel: ObservableObject {
    @Published var qualityPreset: Int = 2
    @Published var toneMappingEnabled: Bool = true
    
    @Published var effects: [PostProcessEffectWrapper] = [
        PostProcessEffectWrapper(type: .motionBlur, name: "Motion Blur"),
        PostProcessEffectWrapper(type: .chromaticAberration, name: "Chromatic Aberration"),
        PostProcessEffectWrapper(type: .filmGrain, name: "Film Grain"),
        PostProcessEffectWrapper(type: .vignette, name: "Vignette"),
        PostProcessEffectWrapper(type: .bloom, name: "Bloom (Beta)")
    ]
    
    init() {
        // Sync with engine state
        syncState()
    }
    
    func syncState() {
        // Update wrappers from API
        if let idx = effects.firstIndex(where: { $0.type == .motionBlur }) {
            effects[idx].isEnabled = PostProcessingAPI.shared.motionBlurEnabled
            effects[idx].intensity = PostProcessingAPI.shared.motionBlurStrength
        }
        
        if let idx = effects.firstIndex(where: { $0.type == .chromaticAberration }) {
            effects[idx].isEnabled = PostProcessingAPI.shared.chromaticAberrationEnabled
            effects[idx].intensity = PostProcessingAPI.shared.chromaticAberrationIntensity
        }
        
        // ... sync others
    }
}
