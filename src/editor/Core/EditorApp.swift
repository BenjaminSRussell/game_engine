import SwiftUI

/// Main entry point for VoxelForge Editor
@main
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
    let rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    var body: some View {
        HSplitView {
            // Main viewport
            MetalViewport(deltaTime: $deltaTime, rendererBridge: rendererBridge)
                .frame(minWidth: 800)
            
            // Inspector panel (placeholder)
            VStack(alignment: .leading, spacing: 12) {
                Text("Inspector")
                    .font(.headline)
                
                Divider()
                
                HStack {
                    Text("Frame Time:")
                    Spacer()
                    Text(String(format: "%.2f ms", deltaTime * 1000))
                        .foregroundColor(deltaTime < 0.016 ? .green : .orange)
                }
                
                HStack {
                    Text("FPS:")
                    Spacer()
                    Text(String(format: "%.0f", 1.0 / deltaTime))
                }
                
                Spacer()
            }
            .padding()
            .frame(width: 300)
        }
    }
}

// MARK: - Preview

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView(deltaTime: .constant(0.016), rendererBridge: nil)
    }
}
