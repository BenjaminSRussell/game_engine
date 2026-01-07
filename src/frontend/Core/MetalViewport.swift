import SwiftUI
import MetalKit
import CEngineCore

/// SwiftUI wrapper for Metal viewport rendering voxel engine output
public struct MetalViewport: NSViewRepresentable {
    @Binding var deltaTime: Double
    let rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    public init(deltaTime: Binding<Double>, rendererBridge: UnsafeMutablePointer<RendererBridge>?) {
        self._deltaTime = deltaTime
        self.rendererBridge = rendererBridge
    }
    
    public func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.delegate = context.coordinator
        mtkView.clearColor = MTLClearColor(red: 0.1, green: 0.1, blue: 0.15, alpha: 1.0)
        mtkView.colorPixelFormat = .bgra8Unorm
        mtkView.depthStencilPixelFormat = .depth32Float
        mtkView.framebufferOnly = false
        
        // Enable MetalFX upscaling for 5K rendering
        if #available(macOS 13.0, *) {
            mtkView.preferredFramesPerSecond = 120
        }
        
        return mtkView
    }
    
    public func updateNSView(_ nsView: MTKView, context: Context) {
        // Update coordinator with current delta time
        context.coordinator.deltaTime = deltaTime
    }
    
    public func makeCoordinator() -> Coordinator {
        Coordinator(self)
    }
    
    // MARK: - Coordinator (MTKViewDelegate)
    
    public class Coordinator: NSObject, MTKViewDelegate {
        var parent: MetalViewport
        var deltaTime: Double = 0
        var lastFrameTime: CFTimeInterval = CACurrentMediaTime()
        
        init(_ parent: MetalViewport) {
            self.parent = parent
        }
        
        public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
            // Notify C renderer of viewport resize
            if let bridge = parent.rendererBridge {
                renderer_bridge_resize_viewport(bridge, UInt32(size.width), UInt32(size.height))
            }
        }
        
        public func draw(in view: MTKView) {
            // Calculate delta time
            let currentTime = CACurrentMediaTime()
            deltaTime = currentTime - lastFrameTime
            lastFrameTime = currentTime
            
            // Render frame via C bridge
            if let bridge = parent.rendererBridge {
                let viewPointer = Unmanaged.passUnretained(view).toOpaque()
                renderer_bridge_render_frame(bridge, viewPointer, Float(deltaTime))
            }
            
            // Update parent binding
            DispatchQueue.main.async {
                self.parent.deltaTime = self.deltaTime
            }
        }
    }
}

// MARK: - Preview Provider

struct MetalViewport_Previews: PreviewProvider {
    static var previews: some View {
        MetalViewport(deltaTime: .constant(0.016), rendererBridge: nil)
            .frame(width: 800, height: 600)
    }
}
