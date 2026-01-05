import Metal
import MetalKit
import SwiftUI
import simd

/// Represent a single voxel
struct Voxel {
    var color: SIMD4<Float>
    var isActive: Bool = false
}

/// 3D Voxel Model data structure
class VoxelModel: ObservableObject {
    @Published var voxels: [Voxel]
    let size: Int
    
    init(size: Int = 32) {
        self.size = size
        self.voxels = Array(repeating: Voxel(color: [1, 1, 1, 1], isActive: false), count: size * size * size)
    }
    
    func indexFor(x: Int, y: Int, z: Int) -> Int {
        return (z * size * size) + (y * size) + x
    }
    
    func setVoxel(x: Int, y: Int, z: Int, color: Color, active: Bool) {
        let idx = indexFor(x: x, y: y, z: z)
        guard idx < voxels.count else { return }
        
        let components = color.components
        voxels[idx] = Voxel(color: [Float(components.r), Float(components.g), Float(components.b), Float(components.a)], isActive: active)
        objectWillChange.send()
    }
    
    func getVoxel(x: Int, y: Int, z: Int) -> Voxel? {
        let idx = indexFor(x: x, y: y, z: z)
        guard idx < voxels.count else { return nil }
        return voxels[idx]
    }
    
    func clear() {
        voxels = Array(repeating: Voxel(color: [1, 1, 1, 1], isActive: false), count: size * size * size)
        objectWillChange.send()
    }
    
    // MARK: - Editing Operations
    
    func editVoxel(at ray: Ray, tool: ModelEditorView.Tool, color: Color, symmetry: (x: Bool, y: Bool, z: Bool)) {
        guard let result = intersectRay(ray) else { return }
        
        let pos = tool == .brush ? result.adjacentVoxel : result.hitVoxel
        let active = tool != .eraser
        
        applyEdit(at: pos, color: color, active: active)
        
        // Apply symmetry
        if symmetry.x {
            applyEdit(at: SIMD3<Int>(size - 1 - pos.x, pos.y, pos.z), color: color, active: active)
        }
        if symmetry.y {
            applyEdit(at: SIMD3<Int>(pos.x, size - 1 - pos.y, pos.z), color: color, active: active)
        }
        if symmetry.z {
            applyEdit(at: SIMD3<Int>(pos.x, pos.y, size - 1 - pos.z), color: color, active: active)
        }
        
        objectWillChange.send()
    }
    
    private func applyEdit(at pos: SIMD3<Int>, color: Color, active: Bool) {
        guard pos.x >= 0 && pos.x < size && pos.y >= 0 && pos.y < size && pos.z >= 0 && pos.z < size else { return }
        let idx = indexFor(x: pos.x, y: pos.y, z: pos.z)
        let components = color.components
        voxels[idx] = Voxel(color: [Float(components.r), Float(components.g), Float(components.b), Float(components.a)], isActive: active)
    }
    
    // MARK: - Raycasting
    
    struct Ray {
        var origin: SIMD3<Float>
        var direction: SIMD3<Float>
    }
    
    struct IntersectionResult {
        var hitVoxel: SIMD3<Int>
        var adjacentVoxel: SIMD3<Int>
        var distance: Float
    }
    
    /// DDA algorithm for voxel intersection
    func intersectRay(_ ray: Ray) -> IntersectionResult? {
        let offset = Float(size) / 2.0
        
        // Simplified DDA logic for voxel grid
        var t: Float = 0
        let maxDist: Float = 100.0
        let step: Float = 0.5
        
        while t < maxDist {
            let p = ray.origin + ray.direction * t
            let gridPos = SIMD3<Int>(Int(floor(p.x + offset)), Int(floor(p.y + offset)), Int(floor(p.z + offset)))
            
            if gridPos.x >= 0 && gridPos.x < size && gridPos.y >= 0 && gridPos.y < size && gridPos.z >= 0 && gridPos.z < size {
                if voxels[indexFor(x: gridPos.x, y: gridPos.y, z: gridPos.z)].isActive {
                    // Backtrack one step for adjacent voxel
                    let prevP = ray.origin + ray.direction * (t - step)
                    let adjPos = SIMD3<Int>(Int(floor(prevP.x + offset)), Int(floor(prevP.y + offset)), Int(floor(prevP.z + offset)))
                    return IntersectionResult(hitVoxel: gridPos, adjacentVoxel: adjPos, distance: t)
                }
            }
            
            // If we are outside but heading in, we can jump to the bounding box
            t += step
        }
        
        // Handle floor plane if no voxel hit
        if ray.direction.y < 0 {
            let tFloor = -(ray.origin.y + offset) / ray.direction.y
            if tFloor > 0 {
                let p = ray.origin + ray.direction * tFloor
                let gridPos = SIMD3<Int>(Int(floor(p.x + offset)), 0, Int(floor(p.z + offset)))
                if gridPos.x >= 0 && gridPos.x < size && gridPos.z >= 0 && gridPos.z < size {
                    return IntersectionResult(hitVoxel: gridPos, adjacentVoxel: gridPos, distance: tFloor)
                }
            }
        }
        
        return nil
    }
}

/// Metal structure for passing uniform data
struct Uniforms {
    var modelMatrix: matrix_float4x4
    var viewMatrix: matrix_float4x4
    var projectionMatrix: matrix_float4x4
}

/// Metal Vertex structure
struct VoxelVertex {
    var position: SIMD3<Float>
    var color: SIMD4<Float>
    var normal: SIMD3<Float>
}

/// Advanced Voxel Renderer using Metal
class AdvancedVoxelRenderer: NSObject, MTKViewDelegate {
    let device: MTLDevice
    let commandQueue: MTLCommandQueue
    var pipelineState: MTLRenderPipelineState?
    var depthStencilState: MTLDepthStencilState?
    
    var voxelModel: VoxelModel
    var vertexBuffer: MTLBuffer?
    var indexBuffer: MTLBuffer?
    var vertexCount: Int = 0
    
    // Camera state
    var rotation: SIMD3<Float> = [0, 0, 0]
    var zoom: Float = 45.0
    var position: SIMD3<Float> = [0, 0, 0]
    
    init(device: MTLDevice, model: VoxelModel) {
        self.device = device
        self.commandQueue = device.makeCommandQueue()!
        self.voxelModel = model
        super.init()
        
        setupPipeline()
        updateMesh()
    }
    
    private func setupPipeline() {
        let library = device.makeDefaultLibrary()
        let vertexFunction = library?.makeFunction(name: "voxel_vertex")
        let fragmentFunction = library?.makeFunction(name: "voxel_fragment")
        
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = vertexFunction
        pipelineDescriptor.fragmentFunction = fragmentFunction
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        pipelineDescriptor.depthAttachmentPixelFormat = .depth32Float
        
        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        } catch {
            print("Failed to create pipeline state: \(error)")
        }
        
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.depthCompareFunction = .less
        depthStencilDescriptor.isDepthWriteEnabled = true
        depthStencilState = device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }
    
    /// Generate a mesh from the voxel model
    /// Simplified version - in production would use greedy meshing
    func updateMesh() {
        var vertices: [VoxelVertex] = []
        var indices: [UInt32] = []
        
        let s = voxelModel.size
        let offset = Float(s) / 2.0
        
        for z in 0..<s {
            for y in 0..<s {
                for x in 0..<s {
                    guard let voxel = voxelModel.getVoxel(x: x, y: y, z: z), voxel.isActive else { continue }
                    
                    addCube(x: Float(x) - offset, y: Float(y) - offset, z: Float(z) - offset, color: voxel.color, vertices: &vertices, indices: &indices)
                }
            }
        }
        
        vertexCount = indices.count
        if !vertices.isEmpty {
            vertexBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<VoxelVertex>.stride, options: [])
            indexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt32>.stride, options: [])
        }
    }
    
    private func addCube(x: Float, y: Float, z: Float, color: SIMD4<Float>, vertices: inout [VoxelVertex], indices: inout [UInt32]) {
        let baseIndex = UInt32(vertices.count)
        
        // Front face (+Z)
        let normals: [SIMD3<Float>] = [
            [0,0,1], [0,0,-1], [0,1,0], [0,-1,0], [1,0,0], [-1,0,0]
        ]
        
        let p: [SIMD3<Float>] = [
            [x, y, z+1], [x+1, y, z+1], [x+1, y+1, z+1], [x, y+1, z+1], // Front
            [x, y, z], [x, y+1, z], [x+1, y+1, z], [x+1, y, z],       // Back
            [x, y+1, z], [x, y+1, z+1], [x+1, y+1, z+1], [x+1, y+1, z], // Top
            [x, y, z], [x+1, y, z], [x+1, y, z+1], [x, y, z+1],       // Bottom
            [x+1, y, z], [x+1, y+1, z], [x+1, y+1, z+1], [x+1, y, z+1], // Right
            [x, y, z], [x, y, z+1], [x, y+1, z+1], [x, y+1, z]        // Left
        ]
        
        for i in 0..<6 {
            let startIdx = UInt32(vertices.count)
            for j in 0..<4 {
                vertices.append(VoxelVertex(position: p[i*4 + j], color: color, normal: normals[i]))
            }
            indices.append(contentsOf: [startIdx, startIdx+1, startIdx+2, startIdx, startIdx+2, startIdx+3])
        }
    }
    
    // MARK: - MTKViewDelegate
    
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
        renderEncoder.setDepthStencilState(depthStencilState)
        renderEncoder.setFrontFacing(.counterClockwise)
        renderEncoder.setCullMode(.back)
        
        // Set Uniforms
        let projectionMatrix = matrix_perspective_right_hand(fovyRadians: radians_from_degrees(45), aspectRatio: Float(view.bounds.width / view.bounds.height), nearZ: 0.1, farZ: 1000)
        
        var viewMatrix = matrix_identity_float4x4
        viewMatrix = matrix_multiply(viewMatrix, translationMatrix(0, 0, -zoom))
        viewMatrix = matrix_multiply(viewMatrix, rotationMatrix(rotation.x, [1, 0, 0]))
        viewMatrix = matrix_multiply(viewMatrix, rotationMatrix(rotation.y, [0, 1, 0]))
        
        var uniforms = Uniforms(modelMatrix: matrix_identity_float4x4, viewMatrix: viewMatrix, projectionMatrix: projectionMatrix)
        lastUniforms = uniforms // Store for raycasting
        
        renderEncoder.setVertexBytes(&uniforms, length: MemoryLayout<Uniforms>.stride, index: 1)
        renderEncoder.setVertexBuffer(vertexBuffer, offset: 0, index: 0)
        
        renderEncoder.drawIndexedPrimitives(type: .triangle, indexCount: vertexCount, indexType: .uint32, indexBuffer: indexBuffer, indexBufferOffset: 0)
        
        renderEncoder.endEncoding()
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
    
    private var lastUniforms: Uniforms?
    
    // MARK: - Raycasting Support
    
    func getRay(from screenPoint: CGPoint, in viewSize: CGSize) -> VoxelModel.Ray {
        guard let uniforms = lastUniforms else {
            return VoxelModel.Ray(origin: [0, 0, 0], direction: [0, 0, -1])
        }
        
        // Normalized device coordinates
        let x = Float(screenPoint.x / viewSize.width * 2 - 1)
        let y = Float(screenPoint.y / viewSize.height * 2 - 1) * -1
        
        let invVP = matrix_invert(matrix_multiply(uniforms.projectionMatrix, uniforms.viewMatrix))
        
        let nearPos = matrix_multiply(invVP, [x, y, 0, 1])
        let farPos = matrix_multiply(invVP, [x, y, 1, 1])
        
        let worldNear = SIMD3<Float>(nearPos.x, nearPos.y, nearPos.z) / nearPos.w
        let worldFar = SIMD3<Float>(farPos.x, farPos.y, farPos.z) / farPos.w
        
        return VoxelModel.Ray(origin: worldNear, direction: normalize(worldFar - worldNear))
    }
}

// MARK: - Math Utilities

func radians_from_degrees(_ degrees: Float) -> Float {
    return (degrees / 180) * .pi
}

func translationMatrix(_ x: Float, _ y: Float, _ z: Float) -> matrix_float4x4 {
    var matrix = matrix_identity_float4x4
    matrix.columns.3 = [x, y, z, 1]
    return matrix
}

func rotationMatrix(_ angle: Float, _ axis: SIMD3<Float>) -> matrix_float4x4 {
    let c = cos(angle)
    let s = sin(angle)
    let t = 1 - c
    
    let x = axis.x, y = axis.y, z = axis.z
    
    return matrix_float4x4(
        [t*x*x + c,   t*x*y + s*z, t*x*z - s*y, 0],
        [t*x*y - s*z, t*y*y + c,   t*y*z + s*x, 0],
        [t*x*z + s*y, t*y*z - s*x, t*z*z + c,   0],
        [0, 0, 0, 1]
    )
}

func matrix_perspective_right_hand(fovyRadians: Float, aspectRatio: Float, nearZ: Float, farZ: Float) -> matrix_float4x4 {
    let ys = 1 / tanf(fovyRadians * 0.5)
    let xs = ys / aspectRatio
    let zs = farZ / (nearZ - farZ)
    return matrix_float4x4(
        [xs, 0,  0, 0],
        [0, ys,  0, 0],
        [0, 0, zs, -1],
        [0, 0, nearZ * zs, 0]
    )
}

extension Color {
    var components: (r: Double, g: Double, b: Double, a: Double) {
        #if canImport(UIKit)
        let uiColor = UIColor(self)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        uiColor.getRed(&r, green: &g, blue: &b, alpha: &a)
        return (Double(r), Double(g), Double(b), Double(a))
        #else
        return (1, 1, 1, 1)
        #endif
    }
}
