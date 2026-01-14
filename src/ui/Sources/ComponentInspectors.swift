import SwiftUI

// MARK: - Light Component (TODO-1544)
struct LightData {
    var lightType: LightType = .point
    var color: Color = .white
    var intensity: Float = 1.0
    var range: Float = 10.0
    var spotAngle: Float = 30.0
    var spotInnerAngle: Float = 21.8
    var castShadows: Bool = true
    var shadowStrength: Float = 1.0
    var shadowBias: Float = 0.05
    var shadowNormalBias: Float = 0.4
    
    enum LightType: String, CaseIterable, CustomStringConvertible {
        case directional, point, spot, area
        var description: String { rawValue.capitalized }
    }
}

struct LightComponentInspector: View {
    @Binding var light: LightData
    
    var body: some View {
        EditorCollapsibleSection("Light", isExpanded: true) {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                // Light Type
                Picker("Type", selection: $light.lightType) {
                    ForEach(LightData.LightType.allCases, id: \.self) { type in
                        Text(type.description).tag(type)
                    }
                }
                .pickerStyle(.segmented)
                
                // Color
                ColorPropertyEditor(label: "Color", value: $light.color)
                
                // Intensity
                FloatPropertyEditor(label: "Intensity", value: $light.intensity, range: 0...10, step: 0.1)
                
                // Range (for point/spot)
                if light.lightType == .point || light.lightType == .spot {
                    FloatPropertyEditor(label: "Range", value: $light.range, range: 0...100, step: 0.5)
                }
                
                // Spot angle (for spot only)
                if light.lightType == .spot {
                    FloatPropertyEditor(label: "Spot Angle", value: $light.spotAngle, range: 1...179, step: 1)
                    FloatPropertyEditor(label: "Inner Angle", value: $light.spotInnerAngle, range: 1...179, step: 1)
                }
                
                EditorDivider()
                
                // Shadows
                BoolPropertyEditor(label: "Cast Shadows", value: $light.castShadows)
                
                if light.castShadows {
                    FloatPropertyEditor(label: "Shadow Strength", value: $light.shadowStrength, range: 0...1, step: 0.05)
                    FloatPropertyEditor(label: "Shadow Bias", value: $light.shadowBias, range: 0...1, step: 0.01)
                }
            }
        }
    }
}

// MARK: - Camera Component (TODO-1545)
struct CameraData {
    var projection: ProjectionType = .perspective
    var fieldOfView: Float = 60.0
    var orthographicSize: Float = 5.0
    var nearClip: Float = 0.3
    var farClip: Float = 1000.0
    var clearFlags: ClearFlags = .skybox
    var backgroundColor: Color = .black
    var depth: Int = -1
    var cullingMask: Int = -1
    var isMainCamera: Bool = false
    
    enum ProjectionType: String, CaseIterable, CustomStringConvertible {
        case perspective, orthographic
        var description: String { rawValue.capitalized }
    }
    
    enum ClearFlags: String, CaseIterable, CustomStringConvertible {
        case skybox, solidColor, depthOnly, nothing
        var description: String { rawValue.capitalized }
    }
}

struct CameraComponentInspector: View {
    @Binding var camera: CameraData
    @State private var showPreview = false
    
    var body: some View {
        EditorCollapsibleSection("Camera", isExpanded: true) {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                // Preview toggle
                HStack {
                    Text("Camera Preview")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Spacer()
                    Toggle("", isOn: $showPreview)
                        .labelsHidden()
                }
                
                if showPreview {
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color.black)
                        .frame(height: 120)
                        .overlay(
                            Text("Camera Preview")
                                .foregroundColor(.gray)
                        )
                }
                
                EditorDivider()
                
                // Projection
                Picker("Projection", selection: $camera.projection) {
                    ForEach(CameraData.ProjectionType.allCases, id: \.self) { type in
                        Text(type.description).tag(type)
                    }
                }
                .pickerStyle(.segmented)
                
                // FOV / Ortho Size
                if camera.projection == .perspective {
                    FloatPropertyEditor(label: "Field of View", value: $camera.fieldOfView, range: 1...179, step: 1)
                } else {
                    FloatPropertyEditor(label: "Size", value: $camera.orthographicSize, range: 0.1...100, step: 0.1)
                }
                
                // Clipping planes
                FloatPropertyEditor(label: "Near Clip", value: $camera.nearClip, range: 0.01...10, step: 0.01)
                FloatPropertyEditor(label: "Far Clip", value: $camera.farClip, range: 10...10000, step: 10)
                
                EditorDivider()
                
                // Clear flags
                Picker("Clear Flags", selection: $camera.clearFlags) {
                    ForEach(CameraData.ClearFlags.allCases, id: \.self) { flag in
                        Text(flag.description).tag(flag)
                    }
                }
                
                if camera.clearFlags == .solidColor {
                    ColorPropertyEditor(label: "Background", value: $camera.backgroundColor)
                }
                
                IntPropertyEditor(label: "Depth", value: $camera.depth, range: -100...100)
                
                EditorDivider()
                
                // Additional properties
                IntPropertyEditor(label: "Culling Mask", value: $camera.cullingMask, range: -1...32)
                BoolPropertyEditor(label: "Is Main Camera", value: $camera.isMainCamera)
            }
        }
    }
}

// MARK: - Audio Source Component (TODO-1546)
struct AudioSourceData {
    var audioClip: String = ""
    var volume: Float = 1.0
    var pitch: Float = 1.0
    var spatialBlend: Float = 1.0
    var dopplerLevel: Float = 1.0
    var minDistance: Float = 1.0
    var maxDistance: Float = 500.0
    var loop: Bool = false
    var playOnAwake: Bool = true
    var mute: Bool = false
    var priority: Int = 128
}

struct AudioSourceComponentInspector: View {
    @Binding var audio: AudioSourceData
    @State private var isPlaying = false
    
    var body: some View {
        EditorCollapsibleSection("Audio Source", isExpanded: true) {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                // Audio clip
                AssetReferenceEditor(label: "Audio Clip", assetPath: $audio.audioClip, assetType: .audio)
                
                // Playback controls
                HStack {
                    Button(action: { isPlaying.toggle() }) {
                        Image(systemName: isPlaying ? "stop.fill" : "play.fill")
                    }
                    .buttonStyle(.plain)
                    
                    // Waveform placeholder
                    RoundedRectangle(cornerRadius: 2)
                        .fill(DesignSystem.Colors.backgroundPrimary)
                        .frame(height: 30)
                        .overlay(
                            Text("Waveform")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textDisabled)
                        )
                }
                
                EditorDivider()
                
                // Volume/Pitch
                FloatPropertyEditor(label: "Volume", value: $audio.volume, range: 0...1, step: 0.05)
                FloatPropertyEditor(label: "Pitch", value: $audio.pitch, range: 0.1...3, step: 0.1)
                
                EditorDivider()
                
                // 3D Sound Settings
                Text("3D Sound Settings")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                FloatPropertyEditor(label: "Spatial Blend", value: $audio.spatialBlend, range: 0...1, step: 0.1)
                FloatPropertyEditor(label: "Doppler Level", value: $audio.dopplerLevel, range: 0...5, step: 0.1)
                FloatPropertyEditor(label: "Min Distance", value: $audio.minDistance, range: 0...100, step: 0.5)
                FloatPropertyEditor(label: "Max Distance", value: $audio.maxDistance, range: 1...1000, step: 10)
                
                EditorDivider()
                
                // Toggles
                BoolPropertyEditor(label: "Loop", value: $audio.loop)
                BoolPropertyEditor(label: "Play On Awake", value: $audio.playOnAwake)
                BoolPropertyEditor(label: "Mute", value: $audio.mute)
                IntPropertyEditor(label: "Priority", value: $audio.priority, range: 0...256)
            }
        }
    }
}

// MARK: - Collider Component (TODO-1547)
struct ColliderData {
    var colliderType: ColliderType = .box
    var isTrigger: Bool = false
    var center: SIMD3<Float> = .zero
    var size: SIMD3<Float> = SIMD3<Float>(1, 1, 1)
    var radius: Float = 0.5
    var height: Float = 2.0
    var physicsMaterial: String = ""
    
    enum ColliderType: String, CaseIterable, CustomStringConvertible {
        case box, sphere, capsule, mesh
        var description: String { rawValue.capitalized }
    }
}

struct ColliderComponentInspector: View {
    @Binding var collider: ColliderData
    
    var body: some View {
        EditorCollapsibleSection("Collider", isExpanded: true) {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                // Collider type
                Picker("Type", selection: $collider.colliderType) {
                    ForEach(ColliderData.ColliderType.allCases, id: \.self) { type in
                        Text(type.description).tag(type)
                    }
                }
                .pickerStyle(.segmented)
                
                BoolPropertyEditor(label: "Is Trigger", value: $collider.isTrigger)
                
                Vector3PropertyEditor(label: "Center", value: $collider.center)
                
                // Type-specific properties
                switch collider.colliderType {
                case .box:
                    Vector3PropertyEditor(label: "Size", value: $collider.size)
                case .sphere:
                    FloatPropertyEditor(label: "Radius", value: $collider.radius, range: 0.01...100, step: 0.1)
                case .capsule:
                    FloatPropertyEditor(label: "Radius", value: $collider.radius, range: 0.01...100, step: 0.1)
                    FloatPropertyEditor(label: "Height", value: $collider.height, range: 0.01...100, step: 0.1)
                case .mesh:
                    Text("Mesh collider uses entity mesh")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                AssetReferenceEditor(label: "Material", assetPath: $collider.physicsMaterial, assetType: .material)
            }
        }
    }
}

// MARK: - Animator Component (TODO-1549)
struct AnimatorData {
    var controller: String = ""
    var avatar: String = ""
    var applyRootMotion: Bool = false
    var updateMode: UpdateMode = .normal
    var cullingMode: CullingMode = .alwaysAnimate
    var speed: Float = 1.0
    
    enum UpdateMode: String, CaseIterable, CustomStringConvertible {
        case normal, animatePhysics, unscaledTime
        var description: String { rawValue.capitalized }
    }
    
    enum CullingMode: String, CaseIterable, CustomStringConvertible {
        case alwaysAnimate, cullUpdateTransforms, cullCompletely
        var description: String { rawValue.capitalized }
    }
}

struct AnimatorComponentInspector: View {
    @Binding var animator: AnimatorData
    
    var body: some View {
        EditorCollapsibleSection("Animator", isExpanded: true) {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                AssetReferenceEditor(label: "Controller", assetPath: $animator.controller, assetType: .animation)
                AssetReferenceEditor(label: "Avatar", assetPath: $animator.avatar, assetType: .mesh)
                
                BoolPropertyEditor(label: "Apply Root Motion", value: $animator.applyRootMotion)
                
                Picker("Update Mode", selection: $animator.updateMode) {
                    ForEach(AnimatorData.UpdateMode.allCases, id: \.self) { mode in
                        Text(mode.description).tag(mode)
                    }
                }
                
                Picker("Culling Mode", selection: $animator.cullingMode) {
                    ForEach(AnimatorData.CullingMode.allCases, id: \.self) { mode in
                        Text(mode.description).tag(mode)
                    }
                }
                
                FloatPropertyEditor(label: "Speed", value: $animator.speed, range: 0...5, step: 0.1)
            }
        }
    }
}
