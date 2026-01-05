// GizmoToolbar.swift
// Toolbar controls for gizmo manipulation

import SwiftUI
import simd

struct GizmoToolbar: View {
    @StateObject private var settings = GizmoSettings()
    @State private var showSnapSettings = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Mode buttons
            GizmoModeButton(mode: .translate, icon: "arrow.up.and.down.and.arrow.left.and.right", hotkey: "Q", currentMode: $settings.mode)
            GizmoModeButton(mode: .rotate, icon: "rotate.3d", hotkey: "W", currentMode: $settings.mode)
            GizmoModeButton(mode: .scale, icon: "arrow.up.left.and.arrow.down.right", hotkey: "E", currentMode: $settings.mode)
            
            Divider()
                .frame(height: 24)
            
            // Space toggle
            Button(action: {
                settings.toggleSpace()
            }) {
                HStack(spacing: DesignSystem.Spacing.xs) {
                    Image(systemName: settings.space == .world ? "globe" : "cube")
                    Text(settings.space == .world ? "World" : "Local")
                        .font(DesignSystem.Typography.caption)
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, DesignSystem.Spacing.xs)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(DesignSystem.CornerRadius.small)
            }
            .buttonStyle(.plain)
            .help("Toggle World/Local Space (⌥Space)")
            
            // Snap toggle
            Button(action: {
                settings.snapEnabled.toggle()
                GizmoAPI.shared.setSnapEnabled(settings.snapEnabled)
            }) {
                Image(systemName: settings.snapEnabled ? "square.grid.3x3.fill" : "square.grid.3x3")
                    .foregroundColor(settings.snapEnabled ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            .help("Toggle Snapping (⌘⇧S)")
            .popover(isPresented: $showSnapSettings) {
                SnapSettingsPopover(settings: settings)
            }
            .onTapGesture(count: 2) {
                showSnapSettings.toggle()
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(DesignSystem.CornerRadius.regular)
        .shadow(color: DesignSystem.Shadows.small.color, radius: DesignSystem.Shadows.small.radius, y: DesignSystem.Shadows.small.y)
    }
}

// MARK: - Helper Views & Models

struct GizmoModeButton: View {
    let mode: GizmoMode
    let icon: String
    let hotkey: String
    @Binding var currentMode: GizmoMode
    
    var isSelected: Bool {
        currentMode == mode
    }
    
    var body: some View {
        Button(action: {
            currentMode = mode
            GizmoAPI.shared.setMode(mode)
        }) {
            VStack(spacing: 2) {
                Image(systemName: icon)
                    .font(.system(size: 16))
                Text(hotkey)
                    .font(DesignSystem.Typography.small)
            }
            .frame(width: 40, height: 40)
            .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
            .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
            .cornerRadius(4)
        }
        .buttonStyle(.plain)
        .help("\(String(describing: mode).capitalized) Mode (\(hotkey))")
    }
}

struct SnapSettingsPopover: View {
    @ObservedObject var settings: GizmoSettings
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text("Snapping Options")
                .font(DesignSystem.Typography.bodyBold)
            
            Toggle("Enable Snapping", isOn: $settings.snapEnabled)
                .onChange(of: settings.snapEnabled) { val in
                    GizmoAPI.shared.setSnapEnabled(val)
                }
            
            Divider()
            
            HStack {
                Text("Translation Step")
                Spacer()
                TextField("Units", value: $settings.translationSnap, format: .number)
                    .frame(width: 60)
            }
            
            HStack {
                Text("Rotation Step")
                Spacer()
                TextField("Degrees", value: $settings.rotationSnap, format: .number)
                    .frame(width: 60)
            }
            
            HStack {
                Text("Scale Step")
                Spacer()
                TextField("Factor", value: $settings.scaleSnap, format: .number)
                    .frame(width: 60)
            }
        }
        .padding(DesignSystem.Spacing.md)
        .frame(width: 200)
    }
}

class GizmoSettings: ObservableObject {
    @Published var mode: GizmoMode = .translate
    @Published var space: GizmoSpace = .world
    @Published var snapEnabled: Bool = false
    @Published var translationSnap: Float = 1.0
    @Published var rotationSnap: Float = 15.0
    @Published var scaleSnap: Float = 0.1
    
    init() {
        self.mode = GizmoAPI.shared.getMode()
        self.space = GizmoAPI.shared.getSpace()
        self.snapEnabled = GizmoAPI.shared.isSnapEnabled()
    }
    
    func toggleSpace() {
        space = (space == .world) ? .local : .world
        GizmoAPI.shared.setSpace(space)
    }
}
