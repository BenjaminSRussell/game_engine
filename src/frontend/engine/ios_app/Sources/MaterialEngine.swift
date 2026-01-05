import SwiftUI
import Combine

/// Types of nodes available in the material editor
enum MaterialNodeType: String, CaseIterable, Identifiable {
    case color = "Color"
    case texture = "Texture Sample"
    case constant = "Constant"
    case add = "Add"
    case multiply = "Multiply"
    case lerp = "Lerp"
    case time = "Time"
    case fresnel = "Fresnel"
    case pbrOutput = "PBR Output"
    
    var id: String { self.rawValue }
}

/// Represents a single node in the material graph
class MaterialNode: ObservableObject, Identifiable {
    let id = UUID()
    let type: MaterialNodeType
    @Published var position: CGPoint
    @Published var properties: [String: Any] = [:]
    
    var inputs: [MaterialSocket] = []
    var outputs: [MaterialSocket] = []
    
    init(type: MaterialNodeType, position: CGPoint) {
        self.type = type
        self.position = position
        setupSockets()
    }
    
    private func setupSockets() {
        switch type {
        case .color:
            outputs.append(MaterialSocket(name: "RGB", type: .vector3))
            outputs.append(MaterialSocket(name: "A", type: .scalar))
            properties["color"] = Color.white
        case .texture:
            inputs.append(MaterialSocket(name: "UV", type: .vector2))
            outputs.append(MaterialSocket(name: "RGB", type: .vector3))
            properties["texture"] = "default_noise"
        case .add, .multiply:
            inputs.append(MaterialSocket(name: "A", type: .vector3))
            inputs.append(MaterialSocket(name: "B", type: .vector3))
            outputs.append(MaterialSocket(name: "Result", type: .vector3))
        case .pbrOutput:
            inputs.append(MaterialSocket(name: "Base Color", type: .vector3))
            inputs.append(MaterialSocket(name: "Metallic", type: .scalar))
            inputs.append(MaterialSocket(name: "Roughness", type: .scalar))
            inputs.append(MaterialSocket(name: "Normal", type: .vector3))
            inputs.append(MaterialSocket(name: "Opacity", type: .scalar))
        default: break
        }
    }
}

/// Represents a connection point on a node
struct MaterialSocket: Identifiable {
    let id = UUID()
    let name: String
    let type: SocketType
    
    enum SocketType {
        case scalar, vector2, vector3, vector4
    }
}

/// Represents a link between two sockets
struct MaterialLink: Identifiable {
    let id = UUID()
    let fromNode: UUID
    let fromSocket: UUID
    let toNode: UUID
    let toSocket: UUID
}

/// The entire material graph
class MaterialGraph: ObservableObject {
    @Published var nodes: [MaterialNode] = []
    @Published var links: [MaterialLink] = []
    
    init() {
        // Default output node
        let output = MaterialNode(type: .pbrOutput, position: CGPoint(x: 400, y: 100))
        nodes.append(output)
    }
    
    func addNode(type: MaterialNodeType, at position: CGPoint) {
        let node = MaterialNode(type: type, position: position)
        nodes.append(node)
    }
    
    func connect(from nodeA: MaterialNode, socketA: MaterialSocket, to nodeB: MaterialNode, socketB: MaterialSocket) {
        let link = MaterialLink(fromNode: nodeA.id, fromSocket: socketA.id, toNode: nodeB.id, toSocket: socketB.id)
        links.append(link)
    }
    
    func getBaseColor() -> Color {
        guard let outputNode = nodes.first(where: { $0.type == .pbrOutput }) else { return .white }
        // For now, just find the first color node connected to base color
        for link in links {
            if link.toNode == outputNode.id {
                if let fromNode = nodes.first(where: { $0.id == link.fromNode }), fromNode.type == .color {
                    return fromNode.properties["color"] as? Color ?? .white
                }
            }
        }
        return .gray
    }
}

import MetalKit

/// Renderer for the material preview sphere
class MaterialRenderer: NSObject, MTKViewDelegate {
    let device: MTLDevice
    let commandQueue: MTLCommandQueue
    var pipelineState: MTLRenderPipelineState?
    var meshBuffer: MTLBuffer?
    var indexBuffer: MTLBuffer?
    var indexCount: Int = 0
    
    var graph: MaterialGraph
    var rotation: Float = 0
    
    init(device: MTLDevice, graph: MaterialGraph) {
        self.device = device
        self.commandQueue = device.makeCommandQueue()!
        self.graph = graph
        super.init()
        setupPipeline()
        setupSphere()
    }
    
    private func setupPipeline() {
        let library = device.makeDefaultLibrary()
        let pipelineDescriptor = MTLRenderPipelineDescriptor()
        pipelineDescriptor.vertexFunction = library?.makeFunction(name: "voxel_vertex")
        pipelineDescriptor.fragmentFunction = library?.makeFunction(name: "voxel_fragment")
        pipelineDescriptor.colorAttachments[0].pixelFormat = .bgra8Unorm
        pipelineDescriptor.depthAttachmentPixelFormat = .depth32Float
        
        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
        } catch {
            print("Material pipeline failed: \(error)")
        }
    }
    
    private func setupSphere() {
        var vertices: [VoxelVertex] = []
        var indices: [UInt32] = []
        let sectors = 36
        let stacks = 18
        let radius: Float = 1.0
        
        for i in 0...stacks {
            let phi = .pi / 2 - Float(i) * (.pi / Float(stacks))
            for j in 0...sectors {
                let theta = Float(j) * (2 * .pi / Float(sectors))
                let x = radius * cos(phi) * cos(theta)
                let y = radius * sin(phi)
                let z = radius * cos(phi) * sin(theta)
                let pos = SIMD3<Float>(x, y, z)
                vertices.append(VoxelVertex(position: pos, color: [1, 1, 1, 1], normal: normalize(pos)))
            }
        }
        
        for i in 0..<stacks {
            var k1 = UInt32(i * (sectors + 1))
            var k2 = k1 + UInt32(sectors + 1)
            for _ in 0..<sectors {
                if i != 0 { indices.append(contentsOf: [k1, k2, k1 + 1]) }
                if i != (stacks - 1) { indices.append(contentsOf: [k1 + 1, k2, k2 + 1]) }
                k1 += 1
                k2 += 1
            }
        }
        
        indexCount = indices.count
        meshBuffer = device.makeBuffer(bytes: vertices, length: vertices.count * MemoryLayout<VoxelVertex>.stride, options: [])
        indexBuffer = device.makeBuffer(bytes: indices, length: indices.count * MemoryLayout<UInt32>.stride, options: [])
    }
    
    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}
    
    func draw(in view: MTKView) {
        rotation += 0.01
        guard let drawable = view.currentDrawable,
              let descriptor = view.currentRenderPassDescriptor,
              let pipeline = pipelineState,
              let commandBuffer = commandQueue.makeCommandBuffer(),
              let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: descriptor) else { return }
        
        encoder.setRenderPipelineState(pipeline)
        
        let projection = matrix_perspective_right_hand(fovyRadians: radians_from_degrees(45), aspectRatio: Float(view.bounds.width / view.bounds.height), nearZ: 0.1, farZ: 100)
        var viewMatrix = translationMatrix(0, 0, -3)
        viewMatrix = matrix_multiply(viewMatrix, rotationMatrix(rotation, [0, 1, 0]))
        
        let color = graph.getBaseColor().components
        let modelColor = SIMD4<Float>(Float(color.r), Float(color.g), Float(color.b), Float(color.a))
        
        // Hacky way to pass base color: override vertex colors in shader or pass as uniform
        // For simplicity, we'll just use uniforms
        var uniforms = Uniforms(modelMatrix: matrix_identity_float4x4, viewMatrix: viewMatrix, projectionMatrix: projection)
        
        encoder.setVertexBytes(&uniforms, length: MemoryLayout<Uniforms>.stride, index: 1)
        encoder.setVertexBuffer(meshBuffer, offset: 0, index: 0)
        encoder.drawIndexedPrimitives(type: .triangle, indexCount: indexCount, indexType: .uint32, indexBuffer: indexBuffer!, indexBufferOffset: 0)
        
        encoder.endEncoding()
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}
