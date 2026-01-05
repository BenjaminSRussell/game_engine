import SwiftUI

// MARK: - Physics Editor Manager
class PhysicsEditorManager: ObservableObject {
    static let shared = PhysicsEditorManager()
    
    // World Settings
    @Published var gravity: SIMD3<Float> = SIMD3(0, -9.81, 0)
    @Published var simulationSpeed: Float = 1.0
    @Published var solverIterations: Int = 6
    @Published var fixedTimestep: Float = 0.02
    
    // Simulation State
    @Published var isPlaying: Bool = false
    @Published var isPaused: Bool = false
    
    // Debug Visualization
    @Published var showColliders: Bool = true
    @Published var showContacts: Bool = false
    @Published var showJoints: Bool = false
    @Published var showAABBs: Bool = false
    @Published var showBroadphase: Bool = false
    
    // Tools
    @Published var selectedLayer: Int = 0
    
    func stepSimulation() {
        print("[Physics] Stepping simulation one frame")
        // EngineBridge.shared.stepPhysics()
    }
    
    func toggleSimulation() {
        isPlaying.toggle()
        print("[Physics] Simulation \(isPlaying ? "Started" : "Stopped")")
    }
}

// MARK: - Physics Editor View
struct PhysicsEditorView: View {
    @ObservedObject var manager = PhysicsEditorManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Physics")
                    .font(DesignSystem.Typography.h3)
                
                Spacer()
                
                // Simulation Controls
                HStack(spacing: 2) {
                    EditorIconButton(icon: manager.isPlaying ? "pause.fill" : "play.fill", 
                                   tooltip: manager.isPlaying ? "Pause Simulation" : "Start Simulation",
                                   action: manager.toggleSimulation)
                    
                    EditorIconButton(icon: "forward.frame.fill", 
                                   tooltip: "Step Simulation",
                                   action: manager.stepSimulation)
                    
                    EditorIconButton(icon: "arrow.counterclockwise", 
                                   tooltip: "Reset Simulation (TODO)",
                                   action: {})
                }
                .padding(4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            ScrollView {
                VStack(alignment: .leading, spacing: 16) {
                    
                    // MARK: World Settings
                    Group {
                        Text("World Settings")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        VStack(spacing: 8) {
                            Vector3PropertyEditor(label: "Gravity", value: $manager.gravity)
                            
                            EditorSlider(label: "Time Scale", value: Binding(
                                get: { Double(manager.simulationSpeed) },
                                set: { manager.simulationSpeed = Float($0) }
                            ), range: 0...4)
                            
                            HStack {
                                Text("Solver Iterations")
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                Spacer()
                                TextField("", value: $manager.solverIterations, formatter: NumberFormatter())
                                    .textFieldStyle(RoundedBorderTextFieldStyle())
                                    .frame(width: 60)
                            }
                            
                            HStack {
                                Text("Fixed Timestep")
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                Spacer()
                                Text(String(format: "%.3f s", manager.fixedTimestep))
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                    
                    EditorDivider()
                    
                    // MARK: Debug Visualization
                    Group {
                        Text("Debug Visualization")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        VStack(spacing: 8) {
                            EditorToggle(label: "Show Colliders", isOn: $manager.showColliders)
                            EditorToggle(label: "Show Contacts", isOn: $manager.showContacts)
                            EditorToggle(label: "Show Joints", isOn: $manager.showJoints)
                            EditorToggle(label: "Show AABBs", isOn: $manager.showAABBs)
                            EditorToggle(label: "Show Broadphase", isOn: $manager.showBroadphase)
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                    
                    EditorDivider()
                    
                    // MARK: Layer Collision Matrix
                    Group {
                        Text("Collision Matrix (TODO)")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Text("Grid of layer interactions will go here.")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .padding()
                            .frame(maxWidth: .infinity)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                    }
                }
                .padding()
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}
