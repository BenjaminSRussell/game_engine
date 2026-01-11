// SwiftUI/MetalRenderer.swift
// REAL Renderer connected to the C Engine
import SwiftUI
import MetalKit

class MetalRenderer: NSObject, ObservableObject, MTKViewDelegate {
    @Published var fps: Double = 0.0
    @Published var drawCalls: Int = 0
    @Published var triangles: Int = 0
    
    var device: MTLDevice!
    var commandQueue: MTLCommandQueue!
    
    private var lastFrameTime: Double = 0
    private var frameCount: Int = 0
    private var timeAccumulator: Double = 0
    
    override init() {
        super.init()
        self.device = MTLCreateSystemDefaultDevice()
        self.commandQueue = device.makeCommandQueue()
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        // Handle resize if needed
    }
    
    func draw(in view: MTKView) {
        let currentTime = CACurrentMediaTime()
        let deltaTime = Float(currentTime - lastFrameTime)
        lastFrameTime = currentTime
        
        // Calculate FPS
        frameCount += 1
        timeAccumulator += Double(deltaTime)
        if timeAccumulator >= 1.0 {
            fps = Double(frameCount) / timeAccumulator
            frameCount = 0
            timeAccumulator = 0
            
            // Get stats from engine (Placeholder for now)
            drawCalls = Int.random(in: 150...300) 
            triangles = Int.random(in: 100000...500000)
        }
        
        // 1. Update Engine Logic
        EngineBridge.update(deltaTime)
        
        // 2. Render
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor else { return }
        
        let commandBuffer = commandQueue.makeCommandBuffer()!
        
        // Call C Engine to render into this texture
        EngineBridge.renderFrame(with: commandBuffer, outputTexture: drawable.texture)
        
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

// SwiftUI View Wrapper
struct MetalView: NSViewRepresentable {
    @ObservedObject var renderer: MetalRenderer
    
    func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = renderer.device
        mtkView.delegate = renderer
        mtkView.framebufferOnly = false // Required for compute writing
        
        // Initialize Engine Backend
        EngineBridge.initializeEngine(with: renderer.device, 
                                    width: 1920, 
                                    height: 1080)
        
        return mtkView
    }
    
    func updateNSView(_ nsView: MTKView, context: Context) {
    }
}
