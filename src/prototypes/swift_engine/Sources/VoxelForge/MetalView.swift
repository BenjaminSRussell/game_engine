import SwiftUI
import MetalKit

public struct MetalView: NSViewRepresentable {
    public init() {}

    public func makeNSView(context: Context) -> MTKView {
        guard let device = MTLCreateSystemDefaultDevice() else {
            fatalError("Metal is not supported")
        }
        
        let mtkView = MTKView(frame: .zero, device: device)
        mtkView.colorPixelFormat = .bgra8Unorm
        mtkView.depthStencilPixelFormat = .depth32Float
        
        // Initialize Renderer
        // Note: In a real app, you might want to manage the renderer lifecycle outside the view
        // to prevent recreation. For this editor, we'll attach it to the view.
        let renderer = Renderer(metalKitView: mtkView)
        context.coordinator.renderer = renderer
        
        return mtkView
    }
    
    public func updateNSView(_ nsView: MTKView, context: Context) {
        // Handle updates from SwiftUI state to Renderer
    }
    
    public func makeCoordinator() -> Coordinator {
        Coordinator()
    }
    
    public class Coordinator {
        var renderer: Renderer?
    }
}
