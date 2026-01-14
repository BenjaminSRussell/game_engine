import SwiftUI

// MARK: - Particle Editor Panel

struct ParticleEditorPanel: View {
    @StateObject private var viewModel = ParticleEditorViewModel()
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                // Playback
                HStack(spacing: 2) {
                    EditorIconButton(icon: viewModel.isPlaying ? "pause.fill" : "play.fill", tooltip: "Play/Pause") {
                        viewModel.togglePlayback()
                    }
                    EditorIconButton(icon: "arrow.counterclockwise", tooltip: "Restart") {
                        viewModel.restart()
                    }
                }
                .padding(4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                
                Divider()
                    .frame(height: 20)
                
                // Settings Toggles
                HStack(spacing: 8) {
                    Toggle("GPU", isOn: $viewModel.useGPU)
                    Toggle("Bounds", isOn: $viewModel.showBounds)
                    Toggle("Gizmos", isOn: $viewModel.showGizmos)
                }
                .font(DesignSystem.Typography.small)
                
                Spacer()
                
                // Stats
                Text("\(viewModel.activeParticles) Particles")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()

            HStack(spacing: 0) {
            // Left: Particle Properties
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.md) {
                    // Emitter Settings
                    EditorCollapsibleSection("Emitter", isExpanded: true) {
                        VStack(spacing: DesignSystem.Spacing.md) {
                            EditorNumericField(label: "Rate", value: $viewModel.emissionRate, range: 0...1000, step: 1)
                            EditorNumericField(label: "Lifetime", value: $viewModel.particleLifetime, range: 0...10, step: 0.1)
                            EditorNumericField(label: "Start Speed", value: $viewModel.startSpeed, range: 0...100, step: 0.5)
                            
                            Toggle(isOn: $viewModel.looping) {
                                Text("Looping")
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                            }
                            .toggleStyle(.checkbox)
                            
                            EditorDivider()
                            
                            // Shape
                            Text("Emitter Shape")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Picker("", selection: $viewModel.emitterShape) {
                                ForEach(EmitterShape.allCases, id: \.self) { shape in
                                    Text(shape.rawValue).tag(shape)
                                }
                            }
                            .pickerStyle(.menu)
                            
                            if viewModel.emitterShape == .box {
                                EditorVector3Field("Box Size", x: $viewModel.shapeSize.x, y: $viewModel.shapeSize.y, z: $viewModel.shapeSize.z)
                            } else if viewModel.emitterShape == .sphere {
                                EditorNumericField(label: "Radius", value: $viewModel.shapeRadius, range: 0...10, step: 0.1)
                            } else if viewModel.emitterShape == .cone {
                                EditorNumericField(label: "Angle", value: $viewModel.shapeAngle, range: 0...180, step: 1)
                                EditorNumericField(label: "Radius", value: $viewModel.shapeRadius, range: 0...10, step: 0.1)
                            }
                        }
                    }
                    
                    // Particle Appearance
                    EditorCollapsibleSection("Appearance", isExpanded: true) {
                        VStack(spacing: DesignSystem.Spacing.md) {
                            EditorNumericField(label: "Start Size", value: $viewModel.startSize, range: 0...10, step: 0.1)
                            EditorNumericField(label: "End Size", value: $viewModel.endSize, range: 0...10, step: 0.1)
                            
                            // Color over lifetime
                            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                                Text("Color Over Lifetime")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                
                                ColorGradientEditor(gradient: $viewModel.colorGradient)
                            }
                            
                            // Texture
                            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                                Text("Texture")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                
                                HStack {
                                    Text(viewModel.textureName ?? "None")
                                        .font(DesignSystem.Typography.body)
                                        .foregroundColor(DesignSystem.Colors.textPrimary)
                                    
                                    Spacer()
                                    
                                    EditorIconButton(icon: "magnifyingglass", tooltip: "Select Texture") {
                                        // Open texture picker
                                        #if os(macOS)
                                        let panel = NSOpenPanel()
                                        panel.allowedContentTypes = [.image]
                                        panel.begin { response in
                                            if response == .OK, let url = panel.url {
                                                print("[Swift] Selected particle texture: \(url.lastPathComponent)")
                                                // Would set particle texture here
                                            }
                                        }
                                        #endif
                                    }
                                }
                                .padding(DesignSystem.Spacing.sm)
                                .background(DesignSystem.Colors.backgroundPrimary)
                                .cornerRadius(4)
                            }
                        }
                    }
                    
                    // Velocity
                    EditorCollapsibleSection("Velocity", isExpanded: true) {
                        VStack(spacing: DesignSystem.Spacing.md) {
                            EditorVector3Field(
                                label: "Direction",
                                x: $viewModel.velocity.x,
                                y: $viewModel.velocity.y,
                                z: $viewModel.velocity.z
                            )
                            
                            EditorNumericField(label: "Speed Variation", value: $viewModel.speedVariation, range: 0...1, step: 0.01)
                        }
                    }
                    
                    // Forces
                    EditorCollapsibleSection("Forces", isExpanded: false) {
                        VStack(spacing: DesignSystem.Spacing.md) {
                            EditorVector3Field(
                                label: "Gravity",
                                x: $viewModel.gravity.x,
                                y: $viewModel.gravity.y,
                                z: $viewModel.gravity.z
                            )
                            
                            EditorNumericField(label: "Drag", value: $viewModel.drag, range: 0...1, step: 0.01)
                            
                            EditorDivider()
                            
                            Toggle(isOn: $viewModel.noiseEnabled) {
                                Text("Turbulence Noise")
                                    .font(DesignSystem.Typography.body)
                            }
                            .toggleStyle(.checkbox)
                            
                            if viewModel.noiseEnabled {
                                EditorNumericField(label: "Strength", value: $viewModel.noiseStrength, range: 0...10, step: 0.1)
                                EditorNumericField(label: "Frequency", value: $viewModel.noiseFrequency, range: 0.1...5, step: 0.1)
                            }
                            
                            EditorNumericField(label: "Drag", value: $viewModel.drag, range: 0...10, step: 0.1)
                        }
                    }
                    
                    // Presets
                    EditorCollapsibleSection("Presets", isExpanded: false) {
                        VStack(spacing: DesignSystem.Spacing.sm) {
                            ForEach(ParticlePreset.allCases, id: \.self) { preset in
                                EditorButton(preset.rawValue) {
                                    viewModel.applyPreset(preset)
                                }
                            }
                        }
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .frame(width: 320)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Center: Preview
            VStack(spacing: 0) {
                // Preview area
                ZStack {
                    Color.black
                    
                    // Particle preview (placeholder)
                    ParticlePreviewView(system: viewModel.particleSystem)
                    
                    // Controls overlay
                    VStack {
                        HStack {
                            Text("Preview")
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            
                            Spacer()
                            
                            HStack(spacing: DesignSystem.Spacing.xs) {
                                EditorIconButton(icon: viewModel.isPlaying ? "pause.fill" : "play.fill", tooltip: viewModel.isPlaying ? "Pause" : "Play") {
                                    viewModel.togglePlayback()
                                }
                                
                                EditorIconButton(icon: "arrow.clockwise", tooltip: "Restart") {
                                    viewModel.restart()
                                }
                            }
                        }
                        .padding(DesignSystem.Spacing.sm)
                        .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
                        .cornerRadius(8)
                        .padding(DesignSystem.Spacing.sm)
                        
                        Spacer()
                        
                        // Stats
                        HStack {
                            Spacer()
                            
                            VStack(alignment: .trailing, spacing: 4) {
                                StatRow(label: "Particles", value: "\(viewModel.activeParticles)")
                                StatRow(label: "Emission Rate", value: String(format: "%.0f/s", viewModel.emissionRate))
                            }
                            .padding(DesignSystem.Spacing.sm)
                            .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
                            .cornerRadius(8)
                            .padding(DesignSystem.Spacing.sm)
                        }
                    }
                }
                
                EditorDivider()
                
                // Timeline
                ParticleTimelineView(
                    lifetime: viewModel.particleLifetime,
                    currentTime: $viewModel.currentTime
                )
                .frame(height: 100)
            }
            
            EditorDivider()
            
            // Right: Curve Editors
            ScrollView {
                VStack(spacing: DesignSystem.Spacing.md) {
                    Text("Property Curves")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    EditorDivider()
                    
                    // Size over lifetime
                    CurveEditorSection(
                        title: "Size Over Lifetime",
                        curve: $viewModel.sizeOverLifetime
                    )
                    
                    // Speed over lifetime
                    CurveEditorSection(
                        title: "Speed Over Lifetime",
                        curve: $viewModel.speedOverLifetime
                    )
                    
                    // Alpha over lifetime
                    CurveEditorSection(
                        title: "Alpha Over Lifetime",
                        curve: $viewModel.alphaOverLifetime
                    )
                }
                .padding(DesignSystem.Spacing.md)
            }
            .frame(width: 280)
            .background(DesignSystem.Colors.backgroundSecondary)
        }
        }
    }
}

// MARK: - Color Gradient Editor

private struct ColorGradientEditor: View {
    @Binding var gradient: [ColorStop]
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.sm) {
            // Gradient preview
            LinearGradient(
                stops: gradient.map { Gradient.Stop(color: $0.color, location: CGFloat($0.position)) },
                startPoint: .leading,
                endPoint: .trailing
            )
            .frame(height: 40)
            .cornerRadius(4)
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
            )
            
            // Color stops
            ForEach(Array(gradient.enumerated()), id: \.offset) { index, stop in
                HStack {
                    ColorPicker("", selection: Binding(
                        get: { stop.color },
                        set: { gradient[index].color = $0 }
                    ))
                    .labelsHidden()
                    .frame(width: 40)
                    
                    Slider(value: Binding(
                        get: { Double(stop.position) },
                        set: { gradient[index].position = Float($0) }
                    ), in: 0...1)
                    
                    Text(String(format: "%.2f", stop.position))
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 50)
                    
                    if gradient.count > 2 {
                        Button(action: {
                            gradient.remove(at: index)
                        }) {
                            Image(systemName: "minus.circle")
                                .foregroundColor(DesignSystem.Colors.accentError)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
            
            EditorButton("Add Color Stop", icon: "plus") {
                gradient.append(ColorStop(color: .white, position: 0.5))
                gradient.sort { $0.position < $1.position }
            }
        }
    }
}

// MARK: - Particle Preview

private struct ParticlePreviewView: View {
    let system: ParticleSystem
    @State private var particles: [Particle] = []
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                ForEach(particles) { particle in
                    Circle()
                        .fill(particle.color)
                        .frame(width: CGFloat(particle.size), height: CGFloat(particle.size))
                        .position(
                            x: geometry.size.width / 2 + CGFloat(particle.position.x),
                            y: geometry.size.height / 2 + CGFloat(particle.position.y)
                        )
                        .opacity(Double(particle.alpha))
                }
            }
        }
        .onAppear {
            // Generate demo particles
            generateDemoParticles()
        }
    }
    
    private func generateDemoParticles() {
        particles = (0..<100).map { _ in
            Particle(
                position: SIMD2(Float.random(in: -100...100), Float.random(in: -100...100)),
                velocity: SIMD2(Float.random(in: -50...50), Float.random(in: -50...50)),
                size: Float.random(in: 2...8),
                color: Color(hue: Double.random(in: 0...1), saturation: 0.8, brightness: 1.0),
                alpha: Float.random(in: 0.3...1.0),
                lifetime: Float.random(in: 1...3)
            )
        }
    }
}

// MARK: - Particle Timeline

private struct ParticleTimelineView: View {
    let lifetime: Float
    @Binding var currentTime: Float
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Timeline")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
                Text(String(format: "%.2fs / %.2fs", currentTime, lifetime))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Timeline scrubber
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    // Background
                    Rectangle()
                        .fill(DesignSystem.Colors.backgroundPrimary)
                    
                    // Progress
                    Rectangle()
                        .fill(DesignSystem.Colors.accentPrimary.opacity(0.3))
                        .frame(width: geometry.size.width * CGFloat(currentTime / lifetime))
                    
                    // Playhead
                    Rectangle()
                        .fill(DesignSystem.Colors.accentPrimary)
                        .frame(width: 2)
                        .offset(x: geometry.size.width * CGFloat(currentTime / lifetime))
                }
                .gesture(
                    DragGesture(minimumDistance: 0)
                        .onChanged { value in
                            let newTime = Float(value.location.x / geometry.size.width) * lifetime
                            currentTime = max(0, min(newTime, lifetime))
                        }
                )
            }
            .frame(height: 40)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Curve Editor Section

private struct CurveEditorSection: View {
    let title: String
    @Binding var curve: ParticleCurve
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text(title)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            ParticleCurveEditorView(curve: $curve)
                .frame(height: 120)
                .background(DesignSystem.Colors.backgroundPrimary)
                .cornerRadius(4)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
        }
    }
}

private struct ParticleCurveEditorView: View {
    @Binding var curve: ParticleCurve
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Grid
                Path { path in
                    for i in 0...4 {
                        let y = geometry.size.height * CGFloat(i) / 4
                        path.move(to: CGPoint(x: 0, y: y))
                        path.addLine(to: CGPoint(x: geometry.size.width, y: y))
                    }
                    for i in 0...4 {
                        let x = geometry.size.width * CGFloat(i) / 4
                        path.move(to: CGPoint(x: x, y: 0))
                        path.addLine(to: CGPoint(x: x, y: geometry.size.height))
                    }
                }
                .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 0.5)
                
                // Curve
                Path { path in
                    let points = curve.points.sorted { $0.time < $1.time }
                    guard !points.isEmpty else { return }
                    
                    let first = points[0]
                    path.move(to: CGPoint(
                        x: geometry.size.width * CGFloat(first.time),
                        y: geometry.size.height * (1 - CGFloat(first.value))
                    ))
                    
                    for point in points.dropFirst() {
                        path.addLine(to: CGPoint(
                            x: geometry.size.width * CGFloat(point.time),
                            y: geometry.size.height * (1 - CGFloat(point.value))
                        ))
                    }
                }
                .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                
                // Control points
                ForEach(Array(curve.points.enumerated()), id: \.offset) { index, point in
                    Circle()
                        .fill(DesignSystem.Colors.accentPrimary)
                        .frame(width: 8, height: 8)
                        .position(
                            x: geometry.size.width * CGFloat(point.time),
                            y: geometry.size.height * (1 - CGFloat(point.value))
                        )
                        .gesture(
                            DragGesture()
                                .onChanged { value in
                                    let newTime = Float(value.location.x / geometry.size.width)
                                    let newValue = 1 - Float(value.location.y / geometry.size.height)
                                    curve.points[index] = CurvePoint(
                                        time: max(0, min(newTime, 1)),
                                        value: max(0, min(newValue, 1))
                                    )
                                }
                        )
                }
            }
        }
    }
}

// MARK: - Data Models

struct ColorStop {
    var color: Color
    var position: Float
}

struct ParticleSystem {
    var emissionRate: Float = 50
    var particleLifetime: Float = 2.0
    var startSpeed: Float = 5.0
    var looping: Bool = true
}

struct Particle: Identifiable {
    let id = UUID()
    var position: SIMD2<Float>
    var velocity: SIMD2<Float>
    var size: Float
    var color: Color
    var alpha: Float
    var lifetime: Float
}

struct ParticleCurve {
    var points: [CurvePoint] = [
        CurvePoint(time: 0, value: 0),
        CurvePoint(time: 1, value: 1)
    ]
}

struct CurvePoint {
    var time: Float
    var value: Float
}

enum EmitterShape: String, CaseIterable {
    case point = "Point"
    case sphere = "Sphere"
    case box = "Box"
    case cone = "Cone"
    case circle = "Circle"
}

enum ParticlePreset: String, CaseIterable {
    case fire = "Fire"
    case smoke = "Smoke"
    case sparks = "Sparks"
    case magic = "Magic"
    case explosion = "Explosion"
    case rain = "Rain"
}

// MARK: - View Model

class ParticleEditorViewModel: ObservableObject {
    @Published var particleSystem = ParticleSystem()
    @Published var emissionRate: Float = 50
    @Published var particleLifetime: Float = 2.0
    @Published var startSpeed: Float = 5.0
    @Published var looping: Bool = true
    
    @Published var startSize: Float = 1.0
    @Published var endSize: Float = 0.5
    @Published var colorGradient: [ColorStop] = [
        ColorStop(color: .orange, position: 0.0),
        ColorStop(color: .red, position: 0.5),
        ColorStop(color: Color.black.opacity(0), position: 1.0)
    ]
    @Published var textureName: String? = nil
    
    @Published var velocity = SIMD3<Float>(0, 1, 0)
    @Published var speedVariation: Float = 0.2
    
    @Published var gravity = SIMD3<Float>(0, -9.8, 0)
    @Published var drag: Float = 0.1
    
    // New VFX Properties
    @Published var emitterShape: EmitterShape = .cone
    @Published var shapeSize: SIMD3<Float> = SIMD3(1, 1, 1)
    @Published var shapeRadius: Float = 1.0
    @Published var shapeAngle: Float = 45.0
    
    @Published var noiseEnabled: Bool = false
    @Published var noiseStrength: Float = 1.0
    @Published var noiseFrequency: Float = 1.0
    
    @Published var useGPU: Bool = true
    @Published var showBounds: Bool = true
    @Published var showGizmos: Bool = true
    
    @Published var sizeOverLifetime = ParticleCurve(points: [
        CurvePoint(time: 0, value: 1),
        CurvePoint(time: 1, value: 0)
    ])
    @Published var speedOverLifetime = ParticleCurve(points: [
        CurvePoint(time: 0, value: 1),
        CurvePoint(time: 1, value: 0.5)
    ])
    @Published var alphaOverLifetime = ParticleCurve(points: [
        CurvePoint(time: 0, value: 1),
        CurvePoint(time: 0.8, value: 0.8),
        CurvePoint(time: 1, value: 0)
    ])
    
    @Published var isPlaying: Bool = true
    @Published var currentTime: Float = 0
    @Published var activeParticles: Int = 0
    
    func togglePlayback() {
        isPlaying.toggle()
    }
    
    func restart() {
        currentTime = 0
    }
    
    func applyPreset(_ preset: ParticlePreset) {
        switch preset {
        case .fire:
            emissionRate = 100
            particleLifetime = 1.5
            startSpeed = 3.0
            startSize = 1.5
            endSize = 0.5
            colorGradient = [
                ColorStop(color: .yellow, position: 0.0),
                ColorStop(color: .orange, position: 0.3),
                ColorStop(color: .red, position: 0.7),
                ColorStop(color: Color.black.opacity(0), position: 1.0)
            ]
            velocity = SIMD3(0, 5, 0)
            
        case .smoke:
            emissionRate = 30
            particleLifetime = 3.0
            startSpeed = 2.0
            startSize = 2.0
            endSize = 4.0
            colorGradient = [
                ColorStop(color: Color.gray.opacity(0.8), position: 0.0),
                ColorStop(color: Color.gray.opacity(0.3), position: 0.5),
                ColorStop(color: Color.gray.opacity(0), position: 1.0)
            ]
            velocity = SIMD3(0, 3, 0)
            
        case .sparks:
            emissionRate = 200
            particleLifetime = 0.5
            startSpeed = 10.0
            startSize = 0.3
            endSize = 0.1
            colorGradient = [
                ColorStop(color: .yellow, position: 0.0),
                ColorStop(color: .orange, position: 0.5),
                ColorStop(color: Color.black.opacity(0), position: 1.0)
            ]
            velocity = SIMD3(0, 1, 0)
            
        case .magic:
            emissionRate = 50
            particleLifetime = 2.0
            startSpeed = 4.0
            startSize = 0.8
            endSize = 0.2
            colorGradient = [
                ColorStop(color: .purple, position: 0.0),
                ColorStop(color: .blue, position: 0.5),
                ColorStop(color: Color.cyan.opacity(0), position: 1.0)
            ]
            velocity = SIMD3(0, 2, 0)
            
        case .explosion:
            emissionRate = 500
            particleLifetime = 1.0
            startSpeed = 15.0
            startSize = 1.0
            endSize = 0.3
            colorGradient = [
                ColorStop(color: .white, position: 0.0),
                ColorStop(color: .yellow, position: 0.2),
                ColorStop(color: .orange, position: 0.5),
                ColorStop(color: Color.black.opacity(0), position: 1.0)
            ]
            velocity = SIMD3(0, 0, 0)
            
        case .rain:
            emissionRate = 300
            particleLifetime = 2.0
            startSpeed = 20.0
            startSize = 0.2
            endSize = 0.2
            colorGradient = [
                ColorStop(color: Color.blue.opacity(0.6), position: 0.0),
                ColorStop(color: Color.blue.opacity(0.4), position: 1.0)
            ]
            velocity = SIMD3(0, -20, 0)
        }
    }
}

// MARK: - Preview

#Preview {
    ParticleEditorPanel()
        .frame(width: 1400, height: 900)
}
