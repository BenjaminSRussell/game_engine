import SwiftUI
import MetalKit
import CEngineCore

struct EngineViewport: NSViewRepresentable {
    var rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.colorPixelFormat = .bgra8Unorm
        mtkView.depthStencilPixelFormat = .depth32Float
        mtkView.framebufferOnly = true
        mtkView.preferredFramesPerSecond = 60
        mtkView.delegate = context.coordinator
        
        return mtkView
    }
    
    func updateNSView(_ nsView: MTKView, context: Context) {
        // Handle updates if needed
    }
    
    func makeCoordinator() -> Coordinator {
        Coordinator(self)
    }
    
    class Coordinator: NSObject, MTKViewDelegate {
        var parent: EngineViewport
        
        init(_ parent: EngineViewport) {
            self.parent = parent
        }
        
        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
            // Handle resize
            guard let bridge = parent.rendererBridge else { return }
            // bridge_resize(bridge, Float(size.width), Float(size.height)) // If such generic function existed
            // For now, might be handled by C side or we need to expose resize
        }
        
        func draw(in view: MTKView) {
            guard let bridge = parent.rendererBridge,
                  let drawable = view.currentDrawable,
                  let renderPassDescriptor = view.currentRenderPassDescriptor else {
                return
            }
            
            // Call C render function
            // renderer_bridge_render(bridge, drawable.texture, ...?)
            // We need to check CEngineCore exposed functions. 
            // For now, minimal implementation to satisfy build
        }
    }
}
