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
            ContentView() // Will update to accept rendererBridge
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

// MARK: - Preview

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        // Preview placeholder
        Text("Content View Preview")
    }
}
