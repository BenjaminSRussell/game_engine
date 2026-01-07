// PhysicsDebugPanel.swift
// UI panel for physics debug visualization controls

import SwiftUI

struct PhysicsDebugPanel: View {
    @StateObject private var settings = PhysicsDebugSettings()
    
    var body: some View {
        EditorPanel("Physics Debug") {
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
                // Master Toggle
                EditorCard {
                    HStack {
                        Toggle("Enable Physics Debug", isOn: $settings.enabled)
                            .toggleStyle(.switch)
                            .onChange(of: settings.enabled) { newValue in
                                PhysicsDebugAPI.shared.setEnabled(newValue)
                            }
                        
                        Spacer()
                        
                        if settings.enabled {
                            Image(systemName: "eye.fill")
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                        }
                    }
                }
                
                if settings.enabled {
                    // Visualization Modes
                    EditorCard {
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Text("Visualization Modes")
                                .font(DesignSystem.Typography.sectionLabel)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            
                            Divider()
                            
                            DebugToggleRow(title: "Contact Points", icon: "circle.fill", isOn: $settings.showContacts) {
                                PhysicsDebugAPI.shared.showContacts($0)
                            }
                            
                            DebugToggleRow(title: "Constraints", icon: "link", isOn: $settings.showConstraints) {
                                PhysicsDebugAPI.shared.showConstraints($0)
                            }
                            
                            DebugToggleRow(title: "AABBs", icon: "cube", isOn: $settings.showAABBs) {
                                PhysicsDebugAPI.shared.showAABBs($0)
                            }
                            
                            DebugToggleRow(title: "OBBs", icon: "cube.fill", isOn: $settings.showOBBs) {
                                PhysicsDebugAPI.shared.showOBBs($0)
                            }
                            
                            DebugToggleRow(title: "Collision Shapes", icon: "square.dashed", isOn: $settings.showShapes) {
                                PhysicsDebugAPI.shared.showShapes($0)
                            }
                            
                            DebugToggleRow(title: "Velocity Vectors", icon: "arrow.forward", isOn: $settings.showVelocities) {
                                PhysicsDebugAPI.shared.showVelocities($0)
                            }
                            
                            DebugToggleRow(title: "Force Vectors", icon: "arrow.up.arrow.down", isOn: $settings.showForces) {
                                PhysicsDebugAPI.shared.showForces($0)
                            }
                            
                            DebugToggleRow(title: "Center of Mass", icon: "scope", isOn: $settings.showCenterOfMass) {
                                PhysicsDebugAPI.shared.showCenterOfMass($0)
                            }
                        }
                    }
                    
                    // Colors
                    EditorCard {
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Text("Colors")
                                .font(DesignSystem.Typography.sectionLabel)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            
                            Divider()
                            
                            ColorPickerRow(title: "Contacts", color: $settings.contactColor) {
                                PhysicsDebugAPI.shared.setContactColor($0)
                            }
                            
                            ColorPickerRow(title: "Constraints", color: $settings.constraintColor) {
                                PhysicsDebugAPI.shared.setConstraintColor($0)
                            }
                            
                            ColorPickerRow(title: "AABBs", color: $settings.aabbColor) {
                                PhysicsDebugAPI.shared.setAABBColor($0)
                            }
                            
                            ColorPickerRow(title: "Velocities", color: $settings.velocityColor) {
                                PhysicsDebugAPI.shared.setVelocityColor($0)
                            }
                        }
                    }
                    
                    // Scale Settings
                    EditorCard {
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Text("Scale Settings")
                                .font(DesignSystem.Typography.sectionLabel)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            
                            Divider()
                            
                            PhysicsSliderRow(title: "Contact Size", value: $settings.contactSize, range: 0.1...2.0) {
                                PhysicsDebugAPI.shared.setContactSize($0)
                            }
                            
                            PhysicsSliderRow(title: "Velocity Scale", value: $settings.velocityScale, range: 0.1...5.0) {
                                PhysicsDebugAPI.shared.setVelocityScale($0)
                            }
                            
                            PhysicsSliderRow(title: "Force Scale", value: $settings.forceScale, range: 0.1...5.0) {
                                PhysicsDebugAPI.shared.setForceScale($0)
                            }
                        }
                    }
                    
                    // Statistics
                    EditorCard {
                        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                            Text("Physics Statistics")
                                .font(DesignSystem.Typography.sectionLabel)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            
                            Divider()
                            
                            StatRow(label: "Active Bodies", value: "\(settings.stats.activeBodies)")
                            StatRow(label: "Sleeping Bodies", value: "\(settings.stats.sleepingBodies)")
                            StatRow(label: "Contacts", value: "\(settings.stats.contactCount)")
                            StatRow(label: "Constraints", value: "\(settings.stats.constraintCount)")
                            StatRow(label: "Islands", value: "\(settings.stats.islandCount)")
                        }
                    }
                    .onAppear {
                        settings.startStatsTimer()
                    }
                    .onDisappear {
                        settings.stopStatsTimer()
                    }
                }
            }
        }
    }
}

// MARK: - Helper Components

private struct DebugToggleRow: View {
    let title: String
    let icon: String
    @Binding var isOn: Bool
    let onChange: (Bool) -> Void
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Image(systemName: icon)
                .foregroundColor(isOn ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                .frame(width: 20)
            
            Text(title)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
            
            Toggle("", isOn: $isOn)
                .toggleStyle(.switch)
                .labelsHidden()
                .onChange(of: isOn) { newValue in
                    onChange(newValue)
                }
        }
        .padding(.vertical, DesignSystem.Spacing.xs)
    }
}

private struct ColorPickerRow: View {
    let title: String
    @Binding var color: Color
    let onChange: (Color) -> Void
    
    var body: some View {
        HStack {
            Text(title)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            Spacer()
            
            ColorPicker("", selection: $color)
                .labelsHidden()
                .onChange(of: color) { newValue in
                    onChange(newValue)
                }
        }
        .padding(.vertical, DesignSystem.Spacing.xs)
    }
}

private struct PhysicsSliderRow: View {
    let title: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let onChange: (Float) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
            HStack {
                Text(title)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Text(String(format: "%.2f", value))
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Slider(value: Binding(
                get: { Double(value) },
                set: { value = Float($0) }
            ), in: Double(range.lowerBound)...Double(range.upperBound))
            .onChange(of: value) { newValue in
                onChange(newValue)
            }
        }
        .padding(.vertical, DesignSystem.Spacing.xs)
    }
}

// MARK: - Settings Model

@MainActor
class PhysicsDebugSettings: ObservableObject {
    @Published var enabled = false
    @Published var showContacts = false
    @Published var showConstraints = false
    @Published var showAABBs = false
    @Published var showOBBs = false
    @Published var showShapes = false
    @Published var showVelocities = false
    @Published var showForces = false
    @Published var showCenterOfMass = false
    
    @Published var contactColor = Color.red
    @Published var constraintColor = Color.blue
    @Published var aabbColor = Color.green
    @Published var velocityColor = Color.yellow
    
    @Published var contactSize: Float = 0.5
    @Published var velocityScale: Float = 1.0
    @Published var forceScale: Float = 1.0
    
    @Published var stats = PhysicsDebugAPI.Stats(
        activeBodies: 0,
        sleepingBodies: 0,
        contactCount: 0,
        constraintCount: 0,
        islandCount: 0
    )
    
    private var statsTimer: Timer?
    
    func startStatsTimer() {
        statsTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] _ in
            Task { @MainActor in
                self?.updateStats()
            }
        }
    }
    
    func stopStatsTimer() {
        statsTimer?.invalidate()
        statsTimer = nil
    }
    
    private func updateStats() {
        stats = PhysicsDebugAPI.shared.getStats()
    }
}

// MARK: - Preview

#Preview {
    PhysicsDebugPanel()
        .frame(width: 300, height: 800)
}
