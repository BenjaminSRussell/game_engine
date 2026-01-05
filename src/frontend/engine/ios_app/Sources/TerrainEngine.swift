import SwiftUI
import Metal
import MetalKit

/// Terrain heightmap and texture data
struct FoliageInstance: Identifiable {
    let id = UUID()
    var position: SIMD3<Float>
    var type: String
    var rotation: Float
    var scale: Float
}

class TerrainData: ObservableObject {
    @Published var heightmap: [Float]
    @Published var textureLayers: [[Float]] // Opacity for each texture layer
    @Published var foliage: [FoliageInstance] = []
    let size: Int
    
    init(size: Int = 128) {
        self.size = size
        self.heightmap = Array(repeating: 0.0, count: size * size)
        self.textureLayers = [
            Array(repeating: 1.0, count: size * size), // Grass
            Array(repeating: 0.0, count: size * size), // Rock
            Array(repeating: 0.0, count: size * size)  // Sand
        ]
    }
    
    func setHeight(x: Int, y: Int, value: Float) {
        guard x >= 0 && x < size && y >= 0 && y < size else { return }
        heightmap[y * size + x] = value
        objectWillChange.send()
    }
    
    func getHeight(at x: Int, _ y: Int) -> Float {
        guard x >= 0 && x < size && y >= 0 && y < size else { return 0 }
        return heightmap[y * size + x]
    }
    
    // MARK: - Sculpting Operations
    
    func sculpt(at x: Int, y: Int, radius: Float, intensity: Float, tool: TerrainEditorView.TerrainTool) {
        let r = Int(radius)
        for dy in -r...r {
            for dx in -r...r {
                let px = x + dx
                let py = y + dy
                guard px >= 0 && px < size && py >= 0 && py < size else { continue }
                
                let dist = sqrt(Float(dx * dx + dy * dy))
                guard dist <= radius else { continue }
                
                let falloff = 1.0 - (dist / radius)
                let strength = intensity * falloff
                
                let idx = py * size + px
                switch tool {
                case .sculpt:
                    heightmap[idx] += strength
                case .smooth:
                    let avg = averageHeight(at: px, py)
                    heightmap[idx] = heightmap[idx] * 0.9 + avg * 0.1
                case .flatten:
                    let target: Float = 0
                    heightmap[idx] = heightmap[idx] * (1.0 - strength) + target * strength
                default: break
                }
            }
        }
        objectWillChange.send()
    }
    
    private func averageHeight(at x: Int, _ y: Int) -> Float {
        var sum: Float = 0
        var count: Int = 0
        for dy in -1...1 {
            for dx in -1...1 {
                let px = x + dx
                let py = y + dy
                if px >= 0 && px < size && py >= 0 && py < size {
                    sum += heightmap[py * size + px]
                    count += 1
                }
            }
        }
        return count > 0 ? sum / Float(count) : 0
    }
}

/// Renderer for terrain using a heightmap
class TerrainRenderer: NSObject, MTKViewDelegate {
    let device: MTLDevice
    let commandQueue: MTLCommandQueue
    var pipelineState: MTLRenderPipelineState?
    
    var terrainData: TerrainData
    var vertexBuffer: MTLBuffer?
    var indexBuffer: MTLBuffer?
    var indexCount: Int = 0
    
    // Camera & Transform
    var rotation: SIMD3<Float> = [0.5, 0.4, 0]
    var zoom: Float = 100.0
    
    init(device: MTLDevice, terrain: TerrainData) {
        self.device = device
        self.commandQueue = device.makeCommandQueue()!
        self.terrainData = terrain
        super.init()
        
        setupPipeline()
        updateMesh()
    }
    
    private func setupPipeline() {
        let library = device.makeDefaultLibrary()
        let vertexFunction = library?.makeFunction(name: "terrain_vertex")
        let fragmentFunction = library?.makeFunction(name: "terrain_fragment")
        
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        pipelineDescriptor.depthAttachmentPixelFormat = .depth32Float
        
        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        } catch {
            print("Failed to create terrain pipeline state: \(error)")
        }
    }
    
    func updateMesh() {
        var vertices: [VoxelVertex] = []
        let size = terrainData.size
        let offset = Float(size) / 2.0
        
        for z in 0..<size {
            for x in 0..<size {
                let h = terrainData.getHeight(at: x, z)
                let pos = SIMD3<Float>(Float(x) - offset, h, Float(z) - offset)
                
                // Color based on height (Biome placeholder)
                var color: SIMD4<Float> = [0.2, 0.5, 0.2, 1] // Green
                if h > 5 { color = [0.5, 0.5, 0.5, 1] } // Stone
                if h < 0.5 { color = [0.8, 0.7, 0.4, 1] } // Sand
                
                vertices.append(VoxelVertex(position: pos, color: color, normal: [0, 1, 0]))
            }
        }
        
        var indices: [UInt32] = []
        for z in 0..<size-1 {
            for x in 0..<size-1 {
                let i0 = UInt32(z * size + x)
                let i1 = UInt32(z * size + x + 1)
                let i2 = UInt32((z + 1) * size + x)
                let i3 = UInt32((z + 1) * size + x + 1)
                
                indices.append(contentsOf: [i0, i1, i2, i1, i3, i2])
            }
        }
        
        indexCount = indices.count
        vertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<VoxelVertex>.stride, options: [])
        indexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt32>.stride, options: [])
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
    
    func draw(in view: MTKView) {
        guard let drawable = view.currentDrawable,
              let renderPassDescriptor = view.currentRenderPassDescriptor,
              let pipelineState = pipelineState,
              let vertexBuffer = vertexBuffer,
              let indexBuffer = indexBuffer,
              let commandBuffer = commandQueue.makeCommandBuffer(),
              let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else { return }
        
        renderEncoder.setRenderPipelineState(pipelineState)
        renderEncoder.setFrontFacing(.counterClockwise)
        renderEncoder.setCullMode(.none) // Terrain needs both sides sometimes
        
        let projectionMatrix = matrix_perspective_right_hand(fovyRadians: radians_from_degrees(45), aspectRatio: Float(view.bounds.width / view.bounds.height), nearZ: 0.1, farZ: 2000)
        
        var viewMatrix = matrix_identity_float4x4
        viewMatrix = matrix_multiply(viewMatrix, translationMatrix(0, -20, -zoom))
        viewMatrix = matrix_multiply(viewMatrix, rotationMatrix(rotation.x, [1, 0, 0]))
        viewMatrix = matrix_multiply(viewMatrix, rotationMatrix(rotation.y, [0, 1, 0]))
        
        var uniforms = Uniforms(modelMatrix: matrix_identity_float4x4, viewMatrix: viewMatrix, projectionMatrix: projectionMatrix)
        renderEncoder.setVertexBytes(&uniforms, length: MemoryLayout<Uniforms>.stride, index: 1)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: indexCount, indexType: .uint32, indexBuffer: indexBuffer, indexBufferOffset: 0)
        
        // Render foliage (Billboarded points placeholder)
        // A full implementation would use instanced rendering
        
        renderEncoder.endEncoding()
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}
