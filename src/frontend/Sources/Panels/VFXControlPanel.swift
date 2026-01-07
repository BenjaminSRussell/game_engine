import SwiftUI

// MARK: - VFX Control Panel

typealias StatusEffectType = StatusEffectAPI.EffectType
typealias WeatherType = WeatherVFXAPI.WeatherType

struct VFXControlPanel: View {
    @StateObject private var viewModel = VFXControlViewModel()
    @State private var selectedTab: VFXTab = .destruction
    
    enum VFXTab: String, CaseIterable {
        case destruction = "Destruction"
        case weather = "Weather"
        case fluid = "Fluid"
        case status = "Status Effects"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab Bar
            HStack(spacing: 0) {
                ForEach(VFXTab.allCases, id: \.self) { tab in
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
                    case .destruction:
                        DestructionControlView(viewModel: viewModel)
                    case .weather:
                        WeatherControlView(viewModel: viewModel)
                    case .fluid:
                        FluidControlView(viewModel: viewModel)
                    case .status:
                        StatusEffectControlView(viewModel: viewModel)
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
            
            EditorDivider()
            
            // Footer: Performance Stats
            HStack {
                StatRow(label: "Active Particles", value: "\(viewModel.activeParticles)")
                Spacer()
                StatRow(label: "Memory", value: "128 MB") // Mock
                Spacer()
                StatRow(label: "GPU Time", value: "0.4ms") // Mock
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
        }
    }
}

// MARK: - Sub-Views

private struct DestructionControlView: View {
    @ObservedObject var viewModel: VFXControlViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            // System Status
            EditorCollapsibleSection("System Status", isExpanded: true) {
                Toggle("Destruction Enabled", isOn: $viewModel.destructionEnabled)
            }
            
            // Settings
            EditorCollapsibleSection("Settings", isExpanded: true) {
                EditorNumericField(label: "Fragment Count", value: $viewModel.fragmentCount, range: 1...100, step: 1)
                EditorNumericField(label: "Debris Lifetime", value: $viewModel.debrisLifetime, range: 1...60, step: 0.5)
            }
            
            // Triggers
            EditorCollapsibleSection("Manual Trigger", isExpanded: true) {
                EditorVector3Field(label: "Impact Point", x: $viewModel.impactPoint.x, y: $viewModel.impactPoint.y, z: $viewModel.impactPoint.z)
                EditorNumericField(label: "Impact Force", value: $viewModel.impactForce, range: 0...1000, step: 10)
                
                EditorButton("Break at Camera", icon: "burst.fill") {
                    viewModel.triggerBreakAtCamera()
                }
            }
        }
    }
}

private struct WeatherControlView: View {
    @ObservedObject var viewModel: VFXControlViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            // Weather Type
            EditorCollapsibleSection("Current Weather", isExpanded: true) {
                Picker("Type", selection: $viewModel.weatherType) {
                    Text("Clear").tag(WeatherType.clear)
                    Text("Rain").tag(WeatherType.rain)
                    Text("Snow").tag(WeatherType.snow)
                    Text("Storm").tag(WeatherType.storm)
                    Text("Fog").tag(WeatherType.fog)
                }
                .pickerStyle(.segmented)
                
                if viewModel.weatherType == .rain || viewModel.weatherType == .storm {
                    EditorNumericField(label: "Rain Intensity", value: $viewModel.rainIntensity, range: 0...1, step: 0.05)
                }
            }
            
            // Environment
            EditorCollapsibleSection("Environment", isExpanded: true) {
                EditorNumericField(label: "Wind Strength", value: $viewModel.windStrength, range: 0...10, step: 0.1)
                
                EditorButton("Trigger Lightning", icon: "bolt.fill") {
                    viewModel.triggerLightning()
                }
            }
        }
    }
}

private struct FluidControlView: View {
    @ObservedObject var viewModel: VFXControlViewModel
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Fluid Simulation", isExpanded: true) {
                EditorButton("Create Emitter", icon: "drop.fill") {
                    viewModel.createFluidEmitter()
                }
                
                if viewModel.activeFluidEmitters > 0 {
                    Text("\(viewModel.activeFluidEmitters) Active Emitters")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            
            EditorCollapsibleSection("Global Settings", isExpanded: true) {
                EditorNumericField(label: "Viscosity", value: $viewModel.fluidViscosity, range: 0...10, step: 0.1)
                Toggle("Enable Splashing", isOn: $viewModel.fluidSplashingEnabled)
            }
        }
    }
}

private struct StatusEffectControlView: View {
    @ObservedObject var viewModel: VFXControlViewModel
    
    let effects: [StatusEffectType] = [.burning, .poison, .freezing, .healing, .shield, .speed]
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            EditorCollapsibleSection("Apply Effect", isExpanded: true) {
                LazyVGrid(columns: [GridItem(.adaptive(minimum: 100))], spacing: 8) {
                    ForEach(effects, id: \.self) { effect in
                        Button(action: { viewModel.applyEffect(effect) }) {
                            VStack {
                                Image(systemName: effectIcon(for: effect))
                                    .font(.system(size: 24))
                                    .frame(height: 40)
                                Text(effectName(for: effect))
                                    .font(DesignSystem.Typography.small)
                            }
                            .frame(maxWidth: .infinity)
                            .padding(.vertical, 8)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(8)
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
                            )
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
            
            EditorCollapsibleSection("Active Effects", isExpanded: true) {
                if viewModel.activeEffects.isEmpty {
                    Text("No active effects on selected entity")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .padding()
                } else {
                    ForEach(viewModel.activeEffects, id: \.self) { effect in
                        HStack {
                            Image(systemName: effectIcon(for: effect))
                            Text(effectName(for: effect))
                            Spacer()
                            Button(action: { viewModel.removeEffect(effect) }) {
                                Image(systemName: "xmark.circle.fill")
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                            .buttonStyle(.plain)
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                }
            }
        }
    }
    
    private func effectIcon(for type: StatusEffectType) -> String {
        switch type {
        case .burning: return "flame.fill"
        case .poison: return "flask.fill"
        case .freezing: return "snowflake"
        case .healing: return "heart.fill"
        case .shield: return "shield.fill"
        case .speed: return "hare.fill"
        }
    }
    
    private func effectName(for type: StatusEffectType) -> String {
        switch type {
        case .burning: return "Burning"
        case .poison: return "Poison"
        case .freezing: return "Freezing"
        case .healing: return "Healing"
        case .shield: return "Shield"
        case .speed: return "Speed"
        }
    }
}

// MARK: - View Model

class VFXControlViewModel: ObservableObject {
    // Destruction
    @Published var destructionEnabled: Bool = true {
        didSet { DestructionAPI.shared.isEnabled = destructionEnabled }
    }
    @Published var fragmentCount: Float = 10
    @Published var debrisLifetime: Float = 10.0 {
        didSet { DestructionAPI.shared.debrisLifetime = debrisLifetime }
    }
    @Published var impactPoint = SIMD3<Float>(0, 0, 0)
    @Published var impactForce: Float = 500
    
    // Weather
    @Published var weatherType: WeatherType = .clear {
        didSet { WeatherVFXAPI.shared.currentType = weatherType }
    }
    @Published var rainIntensity: Float = 0.0 {
        didSet { WeatherVFXAPI.shared.rainIntensity = rainIntensity }
    }
    @Published var windStrength: Float = 0.0 {
        didSet { WeatherVFXAPI.shared.windStrength = windStrength }
    }
    
    // Fluid
    @Published var activeFluidEmitters: Int = 0
    @Published var fluidViscosity: Float = 1.0 {
        didSet { 
            // Update all active emitters (mock)
        }
    }
    @Published var fluidSplashingEnabled: Bool = true
    
    // Status Effects
    @Published var activeEffects: [StatusEffectType] = []
    
    // Optimization Stats
    @Published var activeParticles: Int = 0
    private var timer: Timer?
    
    init() {
        // Sync initial state
        destructionEnabled = DestructionAPI.shared.isEnabled
        debrisLifetime = DestructionAPI.shared.debrisLifetime
        weatherType = WeatherVFXAPI.shared.currentType
        
        startMonitoring()
    }
    
    deinit {
        timer?.invalidate()
    }
    
    func startMonitoring() {
        timer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            self.activeParticles = Int(VFXOptimizationAPI.shared.activeParticles)
        }
    }
    
    // Actions
    
    func triggerBreakAtCamera() {
        // Mock: Trigger at origin for now
        DestructionAPI.shared.triggerBreak(
            entityId: 0, // Mock entity
            impactPoint: (x: impactPoint.x, y: impactPoint.y, z: impactPoint.z),
            force: impactForce
        )
    }
    
    func triggerLightning() {
        // Trigger at random position nearby
        let x = Float.random(in: -50...50)
        let z = Float.random(in: -50...50)
        WeatherVFXAPI.shared.triggerLightning(at: (x: x, y: 100, z: z))
    }
    
    func createFluidEmitter() {
        _ = FluidVFXAPI.shared.createEmitter(position: (0, 5, 0), emitRate: 50)
        activeFluidEmitters += 1
    }
    
    func applyEffect(_ effect: StatusEffectType) {
        // Apply to "selected" entity (mock ID 1)
        StatusEffectAPI.shared.apply(
            to: 1, 
            effect: effect, 
            intensity: 1.0
        )
        if !activeEffects.contains(effect) {
            activeEffects.append(effect)
        }
    }
    
    func removeEffect(_ effect: StatusEffectType) {
        StatusEffectAPI.shared.remove(from: 1, effect: effect)
        activeEffects.removeAll { $0 == effect }
    }
}
