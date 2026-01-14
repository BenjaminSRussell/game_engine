// Swift Frontend Grid and Snapping Manager
// Phase 1-4 Implementation
// Roadmap: grid_snapping_roadmap.md

import SwiftUI
import simd

// MARK: - Grid Settings

struct GridSettings {
    // Grid visualization
    var gridEnabled: Bool = true
    var gridSpacing: Float = 1.0
    var gridMajorSpacing: Float = 10.0
    var gridOpacity: Float = 0.3
    var gridColor: Color = .gray
    var gridFadeDistance: Float = 100.0
    
    // Position snapping
    var snapPositionEnabled: Bool = false
    var snapPositionIncrement: Float = 1.0
    
    // Rotation snapping
    var snapRotationEnabled: Bool = false
    var snapRotationIncrement: Float = 15.0
    
    // Scale snapping
    var snapScaleEnabled: Bool = false
    var snapScaleIncrement: Float = 0.25
    
    // Available presets
    static let positionPresets: [Float] = [0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0]
    static let rotationPresets: [Float] = [1.0, 5.0, 15.0, 30.0, 45.0, 90.0]
    static let scalePresets: [Float] = [0.01, 0.1, 0.25, 0.5, 1.0]
}

// MARK: - Snap Manager

class SnapManager: ObservableObject {
    static let shared = SnapManager()
    
    @Published var settings = GridSettings()
    
    private init() {}
    
    // MARK: - Grid Controls
    
    func toggleGrid() {
        settings.gridEnabled.toggle()
        // Sync to C engine
        EngineBridge.shared.setGridEnabled(settings.gridEnabled)
    }
    
    func setGridSpacing(_ spacing: Float) {
        settings.gridSpacing = spacing
        EngineBridge.shared.setGridSpacing(spacing)
    }
    
    // MARK: - Snap Controls
    
    func togglePositionSnap() {
        settings.snapPositionEnabled.toggle()
        EngineBridge.shared.setPositionSnapEnabled(settings.snapPositionEnabled)
    }
    
    func toggleRotationSnap() {
        settings.snapRotationEnabled.toggle()
        EngineBridge.shared.setRotationSnapEnabled(settings.snapRotationEnabled)
    }
    
    func toggleScaleSnap() {
        settings.snapScaleEnabled.toggle()
        EngineBridge.shared.setScaleSnapEnabled(settings.snapScaleEnabled)
    }
    
    func cyclePositionIncrement() {
        guard let currentIndex = GridSettings.positionPresets.firstIndex(of: settings.snapPositionIncrement) else {
            settings.snapPositionIncrement = GridSettings.positionPresets[0]
            return
        }
        let nextIndex = (currentIndex + 1) % GridSettings.positionPresets.count
        settings.snapPositionIncrement = GridSettings.positionPresets[nextIndex]
        EngineBridge.shared.setPositionSnapIncrement(settings.snapPositionIncrement)
    }
    
    func cycleRotationIncrement() {
        guard let currentIndex = GridSettings.rotationPresets.firstIndex(of: settings.snapRotationIncrement) else {
            settings.snapRotationIncrement = GridSettings.rotationPresets[0]
            return
        }
        let nextIndex = (currentIndex + 1) % GridSettings.rotationPresets.count
        settings.snapRotationIncrement = GridSettings.rotationPresets[nextIndex]
        EngineBridge.shared.setRotationSnapIncrement(settings.snapRotationIncrement)
    }
    
    func cycleScaleIncrement() {
        guard let currentIndex = GridSettings.scalePresets.firstIndex(of: settings.snapScaleIncrement) else {
            settings.snapScaleIncrement = GridSettings.scalePresets[0]
            return
        }
        let nextIndex = (currentIndex + 1) % GridSettings.scalePresets.count
        settings.snapScaleIncrement = GridSettings.scalePresets[nextIndex]
        EngineBridge.shared.setScaleSnapIncrement(settings.snapScaleIncrement)
    }
    
    // MARK: - Snap Functions
    
    func snapPosition(_ position: SIMD3<Float>) -> SIMD3<Float> {
        guard settings.snapPositionEnabled else { return position }
        
        let increment = settings.snapPositionIncrement
        return SIMD3<Float>(
            round(position.x / increment) * increment,
            round(position.y / increment) * increment,
            round(position.z / increment) * increment
        )
    }
    
    func snapRotation(_ rotation: SIMD3<Float>) -> SIMD3<Float> {
        guard settings.snapRotationEnabled else { return rotation }
        
        let increment = settings.snapRotationIncrement
        return SIMD3<Float>(
            round(rotation.x / increment) * increment,
            round(rotation.y / increment) * increment,
            round(rotation.z / increment) * increment
        )
    }
    
    func snapScale(_ scale: SIMD3<Float>) -> SIMD3<Float> {
        guard settings.snapScaleEnabled else { return scale }
        
        let increment = settings.snapScaleIncrement
        return SIMD3<Float>(
            round(scale.x / increment) * increment,
            round(scale.y / increment) * increment,
            round(scale.z / increment) * increment
        )
    }
}

// MARK: - Grid Settings Panel

struct GridSettingsPanel: View {
    @ObservedObject var snapManager = SnapManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Title bar
            HStack {
                Text("Grid & Snapping")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            ScrollView {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                    // Grid Visualization
                    SettingsSection(title: "Grid Visualization") {
                        Toggle("Show Grid", isOn: $snapManager.settings.gridEnabled)
                            .onChange(of: snapManager.settings.gridEnabled) { _ in
                                snapManager.toggleGrid()
                            }
                        
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                            Text("Grid Spacing")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            HStack {
                                ForEach([0.25, 0.5, 1.0, 5.0, 10.0], id: \.self) { spacing in
                                    Button(String(format: "%.2f", spacing)) {
                                        snapManager.setGridSpacing(Float(spacing))
                                    }
                                    .buttonStyle(PresetButtonStyle(isSelected: snapManager.settings.gridSpacing == Float(spacing)))
                                }
                            }
                        }
                        
                        HStack {
                            Text("Opacity")
                                .font(DesignSystem.Typography.small)
                            Slider(value: $snapManager.settings.gridOpacity, in: 0...1)
                        }
                    }
                    
                    EditorDivider()
                    
                    // Position Snapping
                    SettingsSection(title: "Position Snapping") {
                        Toggle("Enable Position Snap", isOn: $snapManager.settings.snapPositionEnabled)
                            .onChange(of: snapManager.settings.snapPositionEnabled) { _ in
                                snapManager.togglePositionSnap()
                            }
                        
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                            Text("Snap Increment")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            HStack {
                                ForEach(GridSettings.positionPresets, id: \.self) { preset in
                                    Button(String(format: "%.2f", preset)) {
                                        snapManager.settings.snapPositionIncrement = preset
                                        EngineBridge.shared.setPositionSnapIncrement(preset)
                                    }
                                    .buttonStyle(PresetButtonStyle(isSelected: snapManager.settings.snapPositionIncrement == preset))
                                }
                            }
                        }
                    }
                    
                    EditorDivider()
                    
                    // Rotation Snapping
                    SettingsSection(title: "Rotation Snapping") {
                        Toggle("Enable Rotation Snap", isOn: $snapManager.settings.snapRotationEnabled)
                            .onChange(of: snapManager.settings.snapRotationEnabled) { _ in
                                snapManager.toggleRotationSnap()
                            }
                        
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                            Text("Snap Increment (degrees)")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            HStack {
                                ForEach(GridSettings.rotationPresets, id: \.self) { preset in
                                    Button(String(format: "%.0f", preset)) {
                                        snapManager.settings.snapRotationIncrement = preset
                                        EngineBridge.shared.setRotationSnapIncrement(preset)
                                    }
                                    .buttonStyle(PresetButtonStyle(isSelected: snapManager.settings.snapRotationIncrement == preset))
                                }
                            }
                        }
                    }
                    
                    EditorDivider()
                    
                    // Scale Snapping
                    SettingsSection(title: "Scale Snapping") {
                        Toggle("Enable Scale Snap", isOn: $snapManager.settings.snapScaleEnabled)
                            .onChange(of: snapManager.settings.snapScaleEnabled) { _ in
                                snapManager.toggleScaleSnap()
                            }
                        
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                            Text("Snap Increment")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            HStack {
                                ForEach(GridSettings.scalePresets, id: \.self) { preset in
                                    Button(String(format: "%.2f", preset)) {
                                        snapManager.settings.snapScaleIncrement = preset
                                        EngineBridge.shared.setScaleSnapIncrement(preset)
                                    }
                                    .buttonStyle(PresetButtonStyle(isSelected: snapManager.settings.snapScaleIncrement == preset))
                                }
                            }
                        }
                    }
                }
                .padding(DesignSystem.Spacing.md)
            }
            .background(DesignSystem.Colors.backgroundSecondary)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Helper Views

private struct SettingsSection<Content: View>: View {
    let title: String
    let content: Content
    
    init(title: String, @ViewBuilder content: () -> Content) {
        self.title = title
        self.content = content()
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text(title)
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            content
        }
    }
}

private struct PresetButtonStyle: ButtonStyle {
    let isSelected: Bool
    
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(DesignSystem.Typography.small)
            .foregroundColor(isSelected ? .white : DesignSystem.Colors.textPrimary)
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
            .opacity(configuration.isPressed ? 0.7 : 1.0)
    }
}

// MARK: - Preview

#Preview {
    GridSettingsPanel()
        .frame(width: 320, height: 600)
}
