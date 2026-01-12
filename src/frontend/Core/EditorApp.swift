import SwiftUI
import CEngineCore

/// Main entry point for VoxelForge Editor
// @main - Commented out to avoid conflict with App.swift
@available(macOS 14.0, *)
struct VoxelForgeEditorApp: App {
    @State private var deltaTime: Double = 0.016
    @State private var rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    var body: some Scene {
        WindowGroup {
            ContentView(deltaTime: $deltaTime, rendererBridge: rendererBridge)
                .onAppear {
                    setupEngineBridge()
                }
                .frame(minWidth: 1200, minHeight: 800)
        }
        .windowStyle(.hiddenTitleBar)
        .windowToolbarStyle(.unified)
    }
    
    private func setupEngineBridge() {
        // Initialize Metal renderer
        guard let metalRenderer = MetalRenderer() else {
            print("Failed to initialize Metal renderer")
            return
        }
        
        // Create renderer bridge
        let device = metalRenderer.metalDevice
        rendererBridge = renderer_bridge_create(Unmanaged.passUnretained(device as AnyObject).toOpaque())
        
        print("VoxelForge Editor initialized with Metal \(device.name)")
        print("Apple Silicon: \(MetalRenderer.isAppleSilicon)")
    }
}

// MARK: - Content View

struct ContentView: View {
    @Binding var deltaTime: Double
    @State private var systemStatus: StatusGlow.SystemStatus = .active
    @State private var selectedEntity: SwiftEntity?
    @State private var selectedTab: SidebarTab = .inspector
    
    let rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    enum SidebarTab: String, CaseIterable {
        case inspector = "Inspector"
        case assets = "Assets"
    }
    
    var body: some View {
        ZStack {
            // Vitality background
            VitalityMesh(
                colors: [.blue.opacity(0.3), .purple.opacity(0.3), .indigo.opacity(0.3)],
                speed: 12.0
            )
            
            // Main content
            GeometryReader { geometry in
                HStack(spacing: 0) {
                    // Metal viewport
                    ZStack {
                        MetalViewport(deltaTime: $deltaTime, rendererBridge: rendererBridge)
                        
                        // Status glow overlay
                        VStack {
                            HStack {
                                Spacer()
                                StatusGlow(status: systemStatus, intensity: 0.3)
                                    .frame(width: 100, height: 100)
                                    .padding()
                            }
                            Spacer()
                        }
                        
                        // Camera controls overlay
                        VStack {
                            Spacer()
                            HStack {
                                Text("WASD: Move | Space/Shift: Up/Down | Mouse: Look")
                                    .font(.caption)
                                    .padding(8)
                                    .background(.ultraThinMaterial)
                                    .cornerRadius(8)
                                    .padding()
                                Spacer()
                            }
                        }
                    }
                    .frame(width: geometry.size.width - 350)
                    
                    // Sidebar with tabs
                    VStack(spacing: 0) {
                        // Tab selector
                        Picker("", selection: $selectedTab) {
                            ForEach(SidebarTab.allCases, id: \.self) { tab in
                                Text(tab.rawValue).tag(tab)
                            }
                        }
                        .pickerStyle(.segmented)
                        .padding()
                        
                        Divider()
                        
                        // Tab content
                        GlassPlate(material: .thin, blurRadius: 30) {
                            Group {
                                switch selectedTab {
                                case .inspector:
                                    inspectorView
                                case .assets:
                                    AssetBrowserView { asset in
                                        print("Selected asset: \(asset.name)")
                                    }
                                }
                            }
                        }
                    }
                    .frame(width: 350)
                }
            }
        }
        .onAppear {
            updateSystemStatus()
        }
        .onChange(of: deltaTime) { _ in
            updateSystemStatus()
        }
    }
    
    private var inspectorView: some View {
        VStack(alignment: .leading, spacing: 16) {
            // Performance metrics
            VStack(alignment: .leading, spacing: 12) {
                Text("Performance")
                    .font(.headline)
                
                Group {
                    HStack {
                        Text("Frame Time:")
                        Spacer()
                        Text(String(format: "%.2f ms", deltaTime * 1000))
                            .foregroundColor(deltaTime < 0.016 ? .green : .orange)
                            .fontWeight(.semibold)
                    }
                    
                    HStack {
                        Text("FPS:")
                        Spacer()
                        Text(String(format: "%.0f", 1.0 / max(deltaTime, 0.001)))
                            .fontWeight(.semibold)
                    }
                    
                    HStack {
                        Text("Status:")
                        Spacer()
                        Circle()
                            .fill(systemStatus.color)
                            .frame(width: 12, height: 12)
                        Text(statusText)
                            .foregroundColor(.secondary)
                    }
                }
                .font(.system(.body, design: .monospaced))
            }
            
            Divider()
            
            // Property Inspector
            PropertyInspector(selectedEntity: $selectedEntity, world: nil)
            
            Spacer()
            
            // System info
            VStack(alignment: .leading, spacing: 8) {
                Text("System")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                if MetalRenderer.isAppleSilicon {
                    HStack {
                        Image(systemName: "cpu")
                        Text("Apple Silicon")
                    }
                    .font(.caption)
                }
            }
        }
        .padding()
    }
    
    private var statusText: String {
        switch systemStatus {
        case .idle: return "Idle"
        case .active: return "Rendering"
        case .warning: return "Slow Frame"
        case .error: return "Error"
        }
    }
    
    private func updateSystemStatus() {
        if deltaTime < 0.016 {
            systemStatus = .active
        } else if deltaTime < 0.033 {
            systemStatus = .warning
        } else {
            systemStatus = .error
        }
    }
}

// MARK: - Preview

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView(deltaTime: .constant(0.016), rendererBridge: nil)
    }
}
