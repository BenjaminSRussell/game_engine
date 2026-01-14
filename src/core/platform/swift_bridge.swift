import SwiftUI
import MetalKit

// Bridging Header should include "swift_bridge.h"

class EngineHost: ObservableObject {
    @Published var isInitialized = false
    
    init() {
        // Initialize engine on startup? Or wait for view?
        // Defer until boot() is called usually.
    }
    
    func boot() {
        if !isInitialized {
            engine_swift_init()
            isInitialized = true
        }
    }
    
    func shutdown() {
        if isInitialized {
            engine_swift_shutdown()
            isInitialized = false
        }
    }
    
    func update() {
        if isInitialized {
            engine_swift_update()
        }
    }
}

struct EngineView: NSViewRepresentable {
    let host: EngineHost
    
    func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        
        // Select device (should match what engine selects, or engine uses this device)
        if let device = MTLCreateSystemDefaultDevice() {
            mtkView.device = device
        }
        
        mtkView.delegate = context.coordinator
        mtkView.enableSetNeedsDisplay = false // Driven by game loop or timer?
        // Actually, for a game loop, we usually use a CVDisplayLink or let MTKView drive it.
        // If engine_swift_render is driven by MTKView delegate:
        mtkView.isPaused = false
        mtkView.preferredFramesPerSecond = 60
        
        // Tell C engine about this layer?
        // engine_set_layer((__bridge void*)mtkView.layer) // We might need this API
        
        return mtkView
    }
    
    func updateNSView(_ nsView: MTKView, context: Context) {
        // Handle resize or updates
    }
    
    func makeCoordinator() -> Coordinator {
        Coordinator(self)
    }
    
    class Coordinator: NSObject, MTKViewDelegate {
        var parent: EngineView
        
        init(_ parent: EngineView) {
            self.parent = parent
        }
        
        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
            engine_swift_resize(u32(size.width), u32(size.height))
        }
        
        func draw(in view: MTKView) {
            // Update logic (Physics, Game Logic)
            parent.host.update()
            
            // Render logic
            engine_swift_render()
        }
    }
}
