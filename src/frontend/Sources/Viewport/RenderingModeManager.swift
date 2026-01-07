import SwiftUI
import simd

// MARK: - Rendering Mode Manager
// Controls advanced visualization modes for debugging and content creation

class RenderingModeManager: ObservableObject {
    @Published var currentMode: RenderingMode = .lit
    @Published var overlays: Set<RenderingOverlay> = []
    @Published var debugVisualization: DebugVisualization?
    
    // MARK: - Rendering Modes
    enum RenderingMode: String, CaseIterable, Identifiable {
        case lit = "Lit"
        case unlit = "Unlit"
        case wireframe = "Wireframe"
        case shadedWireframe = "Shaded Wireframe"
        
        // TODO: Implement vertex normal visualization
        case vertexNormals = "Vertex Normals"
        // TODO: Implement face normal visualization
        case faceNormals = "Face Normals"
        // TODO: Implement tangent space visualization
        case tangentSpace = "Tangent Space"
        // TODO: Implement UV coordinate visualization
        case uvCoordinates = "UV Coordinates"
        // TODO: Implement vertex color display
        case vertexColors = "Vertex Colors"
        
        // PBR Component Views
        // TODO: Implement albedo-only view
        case albedoOnly = "Albedo Only"
        // TODO: Implement normal map view
        case normalMap = "Normal Map"
        // TODO: Implement roughness view
        case roughness = "Roughness"
        // TODO: Implement metallic view
        case metallic = "Metallic"
        // TODO: Implement ambient occlusion view
        case ambientOcclusion = "Ambient Occlusion"
        // TODO: Implement emissive view
        case emissive = "Emissive"
        // TODO: Implement specular view
        case specular = "Specular"
        
        // Technical Views
        // TODO: Implement depth buffer visualization
        case depthBuffer = "Depth Buffer"
        // TODO: Implement stencil buffer visualization
        case stencilBuffer = "Stencil Buffer"
        // TODO: Implement mipmap level visualization
        case mipmapLevels = "Mipmap Levels"
        // TODO: Implement texture coordinate heat map
        case textureCoordHeatMap = "Texture Coord Heat Map"
        // TODO: Implement polygon density heat map
        case polygonDensity = "Polygon Density"
        // TODO: Implement overdraw visualization
        case overdraw = "Overdraw"
        // TODO: Implement light complexity overlay
        case lightComplexity = "Light Complexity"
        
        var id: String { rawValue }
        
        var icon: String {
            switch self {
            case .lit: return "sun.max.fill"
            case .unlit: return "moon.fill"
            case .wireframe: return "square.grid.3x3"
            case .shadedWireframe: return "square.grid.3x3.fill"
            case .vertexNormals: return "arrow.up.circle"
            case .faceNormals: return "arrow.up.square"
            case .tangentSpace: return "arrow.triangle.turn.up.right.diamond"
            case .uvCoordinates: return "map"
            case .vertexColors: return "paintpalette"
            case .albedoOnly: return "photo"
            case .normalMap: return "cube.transparent"
            case .roughness: return "circle.dotted"
            case .metallic: return "sparkles"
            case .ambientOcclusion: return "shadow"
            case .emissive: return "light.max"
            case .specular: return "sparkle"
            case .depthBuffer: return "camera.metering.matrix"
            case .stencilBuffer: return "checkerboard.rectangle"
            case .mipmapLevels: return "square.stack.3d.down.right"
            case .textureCoordHeatMap: return "flame"
            case .polygonDensity: return "square.grid.2x2"
            case .overdraw: return "square.stack.3d.up"
            case .lightComplexity: return "lightbulb.led"
            }
        }
    }
    
    // MARK: - Overlays
    enum RenderingOverlay: String, CaseIterable, Identifiable {
        // TODO: Implement bounding box display
        case boundingBoxes = "Bounding Boxes"
        // TODO: Implement bounding sphere display
        case boundingSpheres = "Bounding Spheres"
        // TODO: Implement collision shape overlay
        case collisionShapes = "Collision Shapes"
        // TODO: Implement NavMesh visualization
        case navMesh = "NavMesh"
        // TODO: Implement AI path visualization
        case aiPaths = "AI Paths"
        // TODO: Implement reflection probe visualization
        case reflectionProbes = "Reflection Probes"
        // TODO: Implement light probe visualization
        case lightProbes = "Light Probes"
        // TODO: Implement volume visualization
        case volumes = "Volumes"
        // TODO: Implement particle system bounds
        case particleBounds = "Particle Bounds"
        // TODO: Implement audio source visual indicators
        case audioSources = "Audio Sources"
        // TODO: Implement decal projection visualization
        case decalProjections = "Decal Projections"
        // TODO: Implement LOD level indicators
        case lodLevels = "LOD Levels"
        // TODO: Implement shadow cascade visualization
        case shadowCascades = "Shadow Cascades"
        // TODO: Implement occlusion culling visualization
        case occlusionCulling = "Occlusion Culling"
        // TODO: Implement frustum culling debug view
        case frustumCulling = "Frustum Culling"
        
        var id: String { rawValue }
        
        var icon: String {
            switch self {
            case .boundingBoxes: return "cube"
            case .boundingSpheres: return "circle"
            case .collisionShapes: return "shield"
            case .navMesh: return "map.fill"
            case .aiPaths: return "arrow.triangle.branch"
            case .reflectionProbes: return "mirror"
            case .lightProbes: return "lightbulb"
            case .volumes: return "cube.transparent"
            case .particleBounds: return "sparkles"
            case .audioSources: return "speaker.wave.3"
            case .decalProjections: return "viewfinder"
            case .lodLevels: return "square.stack"
            case .shadowCascades: return "shadow"
            case .occlusionCulling: return "eye.slash"
            case .frustumCulling: return "viewfinder.trianglebadge.exclamationmark"
            }
        }
    }
    
    // MARK: - Debug Visualization
    enum DebugVisualization: String, CaseIterable, Identifiable {
        // TODO: Implement physics velocity vectors
        case physicsVelocity = "Physics Velocity"
        // TODO: Implement angular velocity indicators
        case angularVelocity = "Angular Velocity"
        // TODO: Implement center of mass visualization
        case centerOfMass = "Center of Mass"
        // TODO: Implement contact points
        case contactPoints = "Contact Points"
        // TODO: Implement contact normals
        case contactNormals = "Contact Normals"
        // TODO: Implement joint constraints
        case jointConstraints = "Joint Constraints"
        // TODO: Implement spring forces
        case springForces = "Spring Forces"
        
        var id: String { rawValue }
    }
    
    // MARK: - Functions
    func setRenderingMode(_ mode: RenderingMode) {
        currentMode = mode
        // TODO: Send rendering mode to C engine via bridge
        print("[RenderingModeManager] Switched to mode: \(mode.rawValue)")
    }
    
    func toggleOverlay(_ overlay: RenderingOverlay) {
        if overlays.contains(overlay) {
            overlays.remove(overlay)
        } else {
            overlays.insert(overlay)
        }
        // TODO: Send overlay changes to C engine via bridge
        print("[RenderingModeManager] Toggled overlay: \(overlay.rawValue)")
    }
    
    func setDebugVisualization(_ debug: DebugVisualization?) {
        debugVisualization = debug
        // TODO: Send debug visualization mode to C engine
        if let debug = debug {
            print("[RenderingModeManager] Enabled debug: \(debug.rawValue)")
        } else {
            print("[RenderingModeManager] Disabled debug visualization")
        }
    }
    
    func clearAllOverlays() {
        overlays.removeAll()
        // TODO: Clear all overlays in C engine
    }
}

// MARK: - Rendering Mode Selector UI
struct RenderingModeSelectorPanel: View {
    @ObservedObject var manager: RenderingModeManager
    @State private var showingModeGrid = false
    @State private var showingOverlays = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Current mode display
            HStack {
                Text("Rendering Mode")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button(action: {
                    showingModeGrid.toggle()
                }) {
                    HStack(spacing: 4) {
                        Image(systemName: manager.currentMode.icon)
                        Text(manager.currentMode.rawValue)
                        Image(systemName: "chevron.down")
                            .font(.system(size: 10))
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
                .buttonStyle(.plain)
                .popover(isPresented: $showingModeGrid) {
                    RenderingModeGridView(manager: manager, isPresented: $showingModeGrid)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Overlays section
            VStack(alignment: .leading, spacing: 8) {
                Button(action: {
                    showingOverlays.toggle()
                }) {
                    HStack {
                        Text("Overlays (\(manager.overlays.count))")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        Image(systemName: showingOverlays ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                
                if showingOverlays {
                    ScrollView {
                        VStack(spacing: 4) {
                            ForEach(RenderingModeManager.RenderingOverlay.allCases) { overlay in
                                OverlayToggleRow(
                                    overlay: overlay,
                                    isEnabled: manager.overlays.contains(overlay),
                                    action: {
                                        manager.toggleOverlay(overlay)
                                    }
                                )
                            }
                        }
                        .padding(.horizontal, 8)
                    }
                    .frame(maxHeight: 200)
                }
            }
            .padding(.vertical, 8)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

// MARK: - Rendering Mode Grid
private struct RenderingModeGridView: View {
    @ObservedObject var manager: RenderingModeManager
    @Binding var isPresented: Bool
    
    let columns = [
        GridItem(.adaptive(minimum: 100, maximum: 150), spacing: 8)
    ]
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Select Rendering Mode")
                    .font(DesignSystem.Typography.h3)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
            }
            .padding()
            
            EditorDivider()
            
            ScrollView {
                LazyVGrid(columns: columns, spacing: 8) {
                    ForEach(RenderingModeManager.RenderingMode.allCases) { mode in
                        RenderingModeButton(
                            mode: mode,
                            isSelected: manager.currentMode == mode
                        ) {
                            manager.setRenderingMode(mode)
                            isPresented = false
                        }
                    }
                }
                .padding()
            }
        }
        .frame(width: 500, height: 600)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Rendering Mode Button
private struct RenderingModeButton: View {
    let mode: RenderingModeManager.RenderingMode
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            VStack(spacing: 8) {
                Image(systemName: mode.icon)
                    .font(.system(size: 32))
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                
                Text(mode.rawValue)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .multilineTextAlignment(.center)
                    .lineLimit(2)
            }
            .frame(maxWidth: .infinity)
            .padding()
            .background(isSelected ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(8)
            .overlay(
                RoundedRectangle(cornerRadius: 8)
                    .stroke(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear, lineWidth: 2)
            )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Overlay Toggle Row
private struct OverlayToggleRow: View {
    let overlay: RenderingModeManager.RenderingOverlay
    let isEnabled: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            HStack {
                Image(systemName: overlay.icon)
                    .font(.system(size: 14))
                    .foregroundColor(isEnabled ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    .frame(width: 20)
                
                Text(overlay.rawValue)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Image(systemName: isEnabled ? "checkmark.circle.fill" : "circle")
                    .foregroundColor(isEnabled ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textDisabled)
            }
            .padding(.vertical, 4)
            .padding(.horizontal, 8)
            .background(isEnabled ? DesignSystem.Colors.selection.opacity(0.3) : Color.clear)
            .cornerRadius(4)
        }
        .buttonStyle(.plain)
    }
}
