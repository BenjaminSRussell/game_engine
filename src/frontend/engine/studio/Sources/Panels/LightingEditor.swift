import SwiftUI

// MARK: - Lighting Editor Manager
class LightingEditorManager: ObservableObject {
    static let shared = LightingEditorManager()
    
    @Published var lights: [LightObject] = []
    @Published var selectedLightID: UUID?
    
    // Environment Settings
    @Published var ambientColor: Color = .white
    @Published var ambientIntensity: Float = 0.3
    @Published var skyboxEnabled: Bool = true
    
    // Shadow Settings
    @Published var shadowQuality: ShadowQuality = .high
    @Published var shadowDistance: Float = 100.0
    @Published var contactShadows: Bool = true
    
    // Fog Settings
    @Published var fogEnabled: Bool = false
    @Published var fogColor: Color = .gray
    @Published var fogDensity: Float = 0.05
    @Published var fogStart: Float = 10.0
    
    enum ShadowQuality: String, CaseIterable {
        case low = "Low"
        case medium = "Medium"
        case high = "High"
        case ultra = "Ultra"
    }
    
    init() {
        // Mock data
        lights = [
            LightObject(name: "Sun", type: .directional, color: .yellow, intensity: 1.0),
            LightObject(name: "Fill Light", type: .point, color: .blue, intensity: 0.5),
            LightObject(name: "Rim Light", type: .spot, color: .white, intensity: 0.8)
        ]
    }
    
    func addLight(type: LightObject.LightType) {
        let newLight = LightObject(name: "New \(type.rawValue.capitalized)", type: type, color: .white, intensity: 1.0)
        lights.append(newLight)
        selectedLightID = newLight.id
    }
    
    func removeSelectedLight() {
        if let id = selectedLightID {
            lights.removeAll { $0.id == id }
            selectedLightID = nil
        }
    }
}

struct LightObject: Identifiable {
    let id = UUID()
    var name: String
    var type: LightType
    var color: Color
    var intensity: Float
    var range: Float = 10.0
    var spotAngle: Float = 45.0
    var castShadows: Bool = true
    
    enum LightType: String, CaseIterable {
        case directional, point, spot, area
    }
}

// MARK: - Lighting Editor View
struct LightingEditorView: View {
    @ObservedObject var manager = LightingEditorManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Lighting")
                    .font(DesignSystem.Typography.h3)
                
                Spacer()
                
                Menu {
                    Button("Directional") { manager.addLight(type: .directional) }
                    Button("Point") { manager.addLight(type: .point) }
                    Button("Spot") { manager.addLight(type: .spot) }
                    Button("Area") { manager.addLight(type: .area) }
                } label: {
                    Label("Add Light", systemImage: "plus")
                }
                .padding(4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                
                Button(action: { /* Bake lighting */ }) {
                    Image(systemName: "flame")
                        .help("Bake Lighting")
                }
                .padding(4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            HSplitView {
                // Light List
                VStack(alignment: .leading, spacing: 0) {
                    Text("Scene Lights")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(8)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .background(DesignSystem.Colors.backgroundSecondary)
                    
                    List(selection: $manager.selectedLightID) {
                        ForEach(manager.lights) { light in
                            HStack {
                                Image(systemName: iconForType(light.type))
                                    .foregroundColor(light.color)
                                Text(light.name)
                            }
                            .tag(light.id)
                        }
                        .onDelete { indexSet in
                            manager.lights.remove(atOffsets: indexSet)
                        }
                    }
                    .listStyle(.plain)
                }
                .frame(minWidth: 200)
                
                // Settings
                ScrollView {
                    VStack(alignment: .leading, spacing: 16) {
                        
                        // Environment
                        Group {
                            Text("Environment")
                                .font(DesignSystem.Typography.bodyBold)
                            
                            ColorPropertyEditor(label: "Ambient Color", value: $manager.ambientColor)
                            FloatPropertyEditor(label: "Intensity", value: $manager.ambientIntensity, range: 0...2, step: 0.1)
                            EditorToggle(label: "Skybox Enabled", isOn: $manager.skyboxEnabled)
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                        
                        // Shadows
                        Group {
                            Text("Shadows")
                                .font(DesignSystem.Typography.bodyBold)
                            
                            HStack {
                                Text("Quality")
                                    .font(DesignSystem.Typography.body)
                                Spacer()
                                Picker("", selection: $manager.shadowQuality) {
                                    ForEach(LightingEditorManager.ShadowQuality.allCases, id: \.self) { quality in
                                        Text(quality.rawValue).tag(quality)
                                    }
                                }
                                .pickerStyle(.menu)
                                .frame(width: 100)
                            }
                            
                            FloatPropertyEditor(label: "Distance", value: $manager.shadowDistance, range: 10...500, step: 10)
                            EditorToggle(label: "Contact Shadows", isOn: $manager.contactShadows)
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                        
                        // Fog
                        Group {
                            Text("Fog")
                                .font(DesignSystem.Typography.bodyBold)
                            
                            EditorToggle(label: "Enabled", isOn: $manager.fogEnabled)
                            
                            if manager.fogEnabled {
                                ColorPropertyEditor(label: "Color", value: $manager.fogColor)
                                FloatPropertyEditor(label: "Density", value: $manager.fogDensity, range: 0...0.5, step: 0.01)
                                FloatPropertyEditor(label: "Start Distance", value: $manager.fogStart, range: 0...100, step: 5)
                            }
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                    .padding()
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
    
    func iconForType(_ type: LightObject.LightType) -> String {
        switch type {
        case .directional: return "sun.max.fill"
        case .point: return "lightbulb.fill"
        case .spot: return "flashlight.on.fill"
        case .area: return "square.fill"
        }
    }
}
