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
        
        // Send rendering mode to C engine via bridge
        switch mode {
        case .vertexNormals:
            enableVertexNormalVisualization()
        case .faceNormals:
            enableFaceNormalVisualization()
        case .tangentSpace:
            enableTangentSpaceVisualization()
        case .uvCoordinates:
            enableUVCoordinateVisualization()
        case .vertexColors:
            enableVertexColorVisualization()
        case .albedoOnly:
            enableAlbedoOnlyView()
        case .normalMap:
            enableNormalMapView()
        case .roughness:
            enableRoughnessView()
        case .metallic:
            enableMetallicView()
        case .ambientOcclusion:
            enableAmbientOcclusionView()
        case .emissive:
            enableEmissiveView()
        case .specular:
            enableSpecularView()
        case .depthBuffer:
            enableDepthBufferVisualization()
        case .stencilBuffer:
            enableStencilBufferVisualization()
        case .mipmapLevels:
            enableMipmapLevelVisualization()
        case .textureCoordHeatMap:
            enableTextureCoordHeatMap()
        case .polygonDensity:
            enablePolygonDensityHeatMap()
        case .overdraw:
            enableOverdrawVisualization()
        default:
            // Standard modes
            setStandardRenderingMode(mode)
        }
        
        print("[RenderingModeManager] Switched to mode: \(mode.rawValue)")
    }
    
    func toggleOverlay(_ overlay: RenderingOverlay) {
        if overlays.contains(overlay) {
            overlays.remove(overlay)
            // Disable overlay in C engine
            EngineBridge.shared.setOverlayEnabled(overlay, enabled: false)
        } else {
            overlays.insert(overlay)
            // Enable overlay in C engine
            EngineBridge.shared.setOverlayEnabled(overlay, enabled: true)
        }
        print("[RenderingModeManager] Toggled overlay: \(overlay.rawValue)")
    }
    
    func setDebugVisualization(_ debug: DebugVisualization?) {
        debugVisualization = debug
        // Send debug visualization mode to C engine
        if let debug = debug {
            EngineBridge.shared.setDebugVisualization(debug)
            print("[RenderingModeManager] Enabled debug: \(debug.rawValue)")
        } else {
            EngineBridge.shared.setDebugVisualization(nil)
            print("[RenderingModeManager] Disabled debug visualization")
        }
    }
    
    func clearAllOverlays() {
        overlays.removeAll()
        EngineBridge.shared.clearAllOverlays()
    }
    
    // MARK: - Rendering Mode Implementations
    
    private func setStandardRenderingMode(_ mode: RenderingMode) {
        let engineMode = convertToEngineRenderMode(mode)
        EngineBridge.shared.setRenderMode(engineMode)
    }
    
    // MARK: - Geometry Visualization Modes
    
    private func enableVertexNormalVisualization() {
        EngineBridge.shared.setRenderMode(.normals)
        EngineBridge.setNormalVisualizationMode(.vertex)
        EngineBridge.setNormalLength(1.0)
        EngineBridge.setNormalColor(SIMD4<Float>(1.0, 1.0, 0.0, 1.0)) // Yellow
    }
    
    private func enableFaceNormalVisualization() {
        EngineBridge.shared.setRenderMode(.normals)
        EngineBridge.setNormalVisualizationMode(.face)
        EngineBridge.setNormalLength(0.5)
        EngineBridge.setNormalColor(SIMD4<Float>(0.0, 1.0, 1.0, 1.0)) // Cyan
    }
    
    private func enableTangentSpaceVisualization() {
        EngineBridge.shared.setRenderMode(.normals)
        EngineBridge.setNormalVisualizationMode(.tangentSpace)
        EngineBridge.setTangentColor(SIMD4<Float>(1.0, 0.0, 0.0, 1.0)) // Red - Tangent
        EngineBridge.setBitangentColor(SIMD4<Float>(0.0, 1.0, 0.0, 1.0)) // Green - Bitangent
        EngineBridge.setNormalSpaceColor(SIMD4<Float>(0.0, 0.0, 1.0, 1.0)) // Blue - Normal
    }
    
    private func enableUVCoordinateVisualization() {
        EngineBridge.shared.setRenderMode(.albedo)
        EngineBridge.setUVVisualizationEnabled(true)
        EngineBridge.setUVScale(1.0)
        EngineBridge.setUVOffset(SIMD2<Float>(0.0, 0.0))
        EngineBridge.setUVChannel(0) // UV channel 0
    }
    
    private func enableVertexColorVisualization() {
        EngineBridge.shared.setRenderMode(.albedo)
        EngineBridge.setVertexColorEnabled(true)
        EngineBridge.setVertexColorAlpha(1.0)
    }
    
    // MARK: - PBR Component Views
    
    private func enableAlbedoOnlyView() {
        EngineBridge.shared.setRenderMode(.albedo)
        EngineBridge.setComponentExposure(1.0)
        EngineBridge.setComponentGamma(2.2)
    }
    
    private func enableNormalMapView() {
        EngineBridge.shared.setRenderMode(.normals)
        EngineBridge.setNormalMapScale(1.0)
        EngineBridge.setNormalMapBias(0.0)
    }
    
    private func enableRoughnessView() {
        EngineBridge.shared.setRenderMode(.smoothness)
        EngineBridge.setRoughnessRange(0.0, 1.0)
        EngineBridge.setRoughnessGamma(1.0)
    }
    
    private func enableMetallicView() {
        EngineBridge.shared.setRenderMode(.metallic)
        EngineBridge.setMetallicRange(0.0, 1.0)
        EngineBridge.setMetallicContrast(1.0)
    }
    
    private func enableAmbientOcclusionView() {
        EngineBridge.shared.setRenderMode(.ambientOcclusion)
        EngineBridge.setAOPower(1.0)
        EngineBridge.setAOBias(0.0)
    }
    
    private func enableEmissiveView() {
        EngineBridge.shared.setRenderMode(.shaded)
        EngineBridge.setEmissiveMode(true)
        EngineBridge.setEmissiveExposure(1.0)
        EngineBridge.setEmissiveTint(SIMD3<Float>(1.0, 1.0, 1.0))
    }
    
    private func enableSpecularView() {
        EngineBridge.shared.setRenderMode(.shaded)
        EngineBridge.setSpecularMode(true)
        EngineBridge.setSpecularPower(1.0)
        EngineBridge.setSpecularThreshold(0.01)
    }
    
    // MARK: - Technical Visualization Modes
    
    private func enableDepthBufferVisualization() {
        EngineBridge.shared.setRenderMode(.wireframe)
        EngineBridge.setDepthVisualizationEnabled(true)
        EngineBridge.setDepthRange(0.1, 1000.0)
        EngineBridge.setDepthColorMode(.grayscale) // 0 = Grayscale, 1 = Rainbow
    }
    
    private func enableStencilBufferVisualization() {
        EngineBridge.shared.setRenderMode(.wireframe)
        EngineBridge.setStencilVisualizationEnabled(true)
        EngineBridge.setStencilRange(0, 255)
        EngineBridge.setStencilColorMode(.grayscale) // 0 = Grayscale, 1 = Hue
    }
    
    private func enableMipmapLevelVisualization() {
        EngineBridge.shared.setRenderMode(.shaded)
        EngineBridge.setMipmapVisualizationEnabled(true)
        EngineBridge.setMipmapColorMode(.levelColors) // 0 = Level colors, 1 = Heat map
        EngineBridge.setMipmapShowLevels(true)
    }
    
    private func enableTextureCoordHeatMap() {
        EngineBridge.shared.setRenderMode(.albedo)
        EngineBridge.setTextureHeatmapEnabled(true)
        EngineBridge.setHeatmapMode(.uvDistortion) // 0 = UV distortion, 1 = Texture density
        EngineBridge.setHeatmapColorScale(1.0)
    }
    
    private func enablePolygonDensityHeatMap() {
        EngineBridge.shared.setRenderMode(.overdraw)
        EngineBridge.setPolygonDensityHeatmapEnabled(true)
        EngineBridge.setDensityRange(0, 100)
        EngineBridge.setDensityColorMode(.blueToRed) // 0 = Blue-Red, 1 = Green-Yellow
    }
    
    private func enableOverdrawVisualization() {
        EngineBridge.shared.setRenderMode(.overdraw)
        EngineBridge.setOverdrawMode(.additive) // 0 = Additive, 1 = Weighted
        EngineBridge.setOverdrawMaxCount(16)
    }
    
    // MARK: - Helper Functions
    
    private func convertToEngineRenderMode(_ mode: RenderingMode) -> EngineBridge.RenderMode {
        switch mode {
        case .lit: return .shaded
        case .unlit: return .unlit
        case .wireframe: return .wireframe
        case .shadedWireframe: return .shadedWireframe
        case .vertexNormals: return .normals
        case .faceNormals: return .normals
        case .tangentSpace: return .normals
        case .uvCoordinates: return .albedo // Use albedo as base for UV visualization
        case .vertexColors: return .albedo
        case .albedoOnly: return .albedo
        case .normalMap: return .normals
        case .roughness: return .smoothness
        case .metallic: return .metallic
        case .ambientOcclusion: return .ambientOcclusion
        case .emissive: return .shaded
        case .specular: return .shaded
        case .depthBuffer: return .wireframe
        case .stencilBuffer: return .wireframe
        case .mipmapLevels: return .shaded
        case .textureCoordHeatMap: return .albedo
        case .polygonDensity: return .overdraw
        case .overdraw: return .overdraw
        case .lightComplexity: return .overdraw
        }
    }
}

// MARK: - Rendering Mode Selector UI
struct RenderingModeSelectorPanel: View {
    @ObservedObject var manager: RenderingModeManager
    @State private var showingModeGrid = false
    @State private var showingOverlays = false
    @State private var showingDebug = false
    
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
            
            EditorDivider()
            
            // Debug visualization section
            VStack(alignment: .leading, spacing: 8) {
                Button(action: {
                    showingDebug.toggle()
                }) {
                    HStack {
                        Text("Debug Visualization")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        if let debug = manager.debugVisualization {
                            Text(debug.rawValue)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        } else {
                            Text("None")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                        
                        Image(systemName: showingDebug ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                
                if showingDebug {
                    ScrollView {
                        VStack(spacing: 4) {
                            // None option
                            DebugVisualizationRow(
                                debug: nil,
                                name: "None",
                                isSelected: manager.debugVisualization == nil,
                                action: {
                                    manager.setDebugVisualization(nil)
                                }
                            )
                            
                            ForEach(RenderingModeManager.DebugVisualization.allCases) { debug in
                                DebugVisualizationRow(
                                    debug: debug,
                                    name: debug.rawValue,
                                    isSelected: manager.debugVisualization == debug,
                                    action: {
                                        manager.setDebugVisualization(debug)
                                    }
                                )
                            }
                        }
                        .padding(.horizontal, 8)
                    }
                    .frame(maxHeight: 150)
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

// MARK: - Debug Visualization Row
private struct DebugVisualizationRow: View {
    let debug: RenderingModeManager.DebugVisualization?
    let name: String
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            HStack {
                Image(systemName: isSelected ? "checkmark.circle.fill" : "circle")
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textDisabled)
                    .font(.system(size: 14))
                
                Text(name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
            }
            .padding(.vertical, 4)
            .padding(.horizontal, 8)
            .background(isSelected ? DesignSystem.Colors.selection.opacity(0.3) : Color.clear)
            .cornerRadius(4)
        }
        .buttonStyle(.plain)
    }
}
