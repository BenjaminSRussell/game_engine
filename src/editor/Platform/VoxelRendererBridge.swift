import Foundation
import Metal
import simd
import CEngineCore

/// Voxel Renderer Integration
/// Bridges C voxel_renderer.c with SwiftUI Metal viewport
@available(macOS 14.0, *)
public class VoxelRendererBridge {
    private let metalRenderer: MetalRenderer
    private var voxelBuffer: MTLBuffer?
    private var voxelCount: Int = 0
    
    // Rendering state
    public struct RenderConfig {
        var renderDistance: Float = 128.0
        var enableLighting: Bool = true
        var enableShadows: Bool = true
        var enableAO: Bool = true
    }
    
    public var config = RenderConfig()
    
    public init?(metalRenderer: MetalRenderer) {
        self.metalRenderer = metalRenderer
    }
    
    /// Initialize voxel data buffer
    public func initializeVoxelBuffer(maxVoxels: Int) -> Bool {
        let bufferSize = MetalRenderer.recommendedBufferSize(for: maxVoxels)
        
        guard let buffer = metalRenderer.createSharedBuffer(size: bufferSize, label: "VoxelData") else {
            print(" Failed to create voxel buffer")
            return false
        }
        
        self.voxelBuffer = buffer
        self.voxelCount = maxVoxels
        
        print(" Voxel buffer created: \(bufferSize) bytes for \(maxVoxels) voxels")
        return true
    }
    
    /// Get raw pointer for C voxel_renderer to write to
    public func getVoxelDataPointer() -> UnsafeMutableRawPointer? {
        guard let buffer = voxelBuffer else { return nil }
        return metalRenderer.getSharedBufferPointer(buffer)
    }
    
    /// Update voxel data from C renderer
    public func updateFromCRenderer() {
        // This will be called by the C renderer via bridge
        // The C code writes directly to the shared buffer
        // No copying needed due to unified memory
    }
    
    /// Render voxels in Metal viewport
    public func render(commandBuffer: MTLCommandBuffer, renderPassDescriptor: MTLRenderPassDescriptor) {
        // Device is non-optional and unused here currently
        guard let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else { return }
        
        // 1. Get Chunk Manager from World (TODO: Pass World* to bridge)
        // For now, we assume the C side has a singleton or we pass it in updateFromCRenderer
        
        // 2. Iterate chunks
        var chunkCount: UInt32 = 0
        guard let chunksPtr = chunk_bridge_get_loaded_chunks(nil, &chunkCount), chunkCount > 0 else {
            encoder.endEncoding()
            return
        }
        
        for i in 0..<Int(chunkCount) {
            guard let chunk = chunksPtr[i] else { continue }
            
            var vertices: UnsafeMutablePointer<VertexBridge>?
            var vertexCount: UInt32 = 0
            var indices: UnsafeMutablePointer<UInt32>?
            var indexCount: UInt32 = 0
            
            if chunk_bridge_get_mesh(chunk, &vertices, &vertexCount, &indices, &indexCount) {
                // Upload to Metal buffer (zero-copy if we mapped it, but here we likely need to copy 
                // because C engine owns the memory and it might change)
                // For optimal performance, C engine should write directly to MTLBuffer (Unified Memory)
                // But for Phase 4 step 1, we can create transient buffers or verify the pointers work
                
                // Demo: Just verifying we got data
                // print("Chunk \(i): \(vertexCount) vertices")
                
                // Actual rendering would go here:
                // encoder.setVertexBuffer(buffer, offset: 0, index: 0)
                // encoder.drawIndexedPrimitives(...)
            }
        }
        
        free(chunksPtr) // Free the array of pointers
        encoder.endEncoding()
    }
    
    /// Get current voxel count
    public var currentVoxelCount: Int {
        voxelCount
    }
}

// MARK: - Camera Controller

@available(macOS 14.0, *)
public class EditorCamera: ObservableObject {
    @Published public var position: SIMD3<Float> = [0, 5, 10]
    @Published public var rotation: SIMD2<Float> = [0, 0] // pitch, yaw
    @Published public var fov: Float = 60.0
    
    public var movementSpeed: Float = 10.0
    public var rotationSpeed: Float = 0.5
    
    public init() {}
    
    /// Update camera from keyboard input
    public func update(deltaTime: Float, forward: Bool, backward: Bool, left: Bool, right: Bool, up: Bool, down: Bool) {
        let moveAmount = movementSpeed * deltaTime
        
        // Calculate forward/right vectors from rotation
        let yawRad = rotation.y * .pi / 180.0
        let forwardDir = SIMD3<Float>(sin(yawRad), 0, cos(yawRad))
        let rightDir = SIMD3<Float>(cos(yawRad), 0, -sin(yawRad))
        
        if forward { position += forwardDir * moveAmount }
        if backward { position -= forwardDir * moveAmount }
        if right { position += rightDir * moveAmount }
        if left { position -= rightDir * moveAmount }
        if up { position.y += moveAmount }
        if down { position.y -= moveAmount }
    }
    
    /// Rotate camera from mouse delta
    public func rotate(deltaX: Float, deltaY: Float) {
        rotation.y += deltaX * rotationSpeed
        rotation.x += deltaY * rotationSpeed
        
        // Clamp pitch
        rotation.x = max(-89, min(89, rotation.x))
    }
    
    /// Get view matrix
    public func viewMatrix() -> simd_float4x4 {
        // TODO: Implement proper view matrix calculation
        // For now, return identity
        return matrix_identity_float4x4
    }
}

// Helper for identity matrix
private func matrix_identity_float4x4() -> simd_float4x4 {
    return simd_float4x4(
        SIMD4<Float>(1, 0, 0, 0),
        SIMD4<Float>(0, 1, 0, 0),
        SIMD4<Float>(0, 0, 1, 0),
        SIMD4<Float>(0, 0, 0, 1)
    )
}
