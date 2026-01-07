import Foundation
import SwiftUI

// MARK: - Socket Types

enum SocketType: String, Codable {
    case float = "Float"
    case color = "Color"
    case vector = "Vector"
    case shader = "Shader"
    case normal = "Normal"
    
    var color: Color {
        switch self {
        case .float: return .gray
        case .color: return .yellow
        case .vector: return .blue
        case .shader: return .green
        case .normal: return .purple
        }
    }
}

// MARK: - Graph Socket

class GraphSocket: Identifiable, ObservableObject, Codable {
    let id: UUID
    let name: String
    let dataType: SocketType
    @Published var value: SocketValue?
    let isInput: Bool
    
    init(id: UUID = UUID(), name: String, dataType: SocketType, value: SocketValue? = nil, isInput: Bool) {
        self.id = id
        self.name = name
        self.dataType = dataType
        self.value = value
        self.isInput = isInput
    }
    
    // MARK: - Codable
    enum CodingKeys: String, CodingKey {
        case id, name, dataType, value, isInput
    }
    
    required init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        id = try container.decode(UUID.self, forKey: .id)
        name = try container.decode(String.self, forKey: .name)
        dataType = try container.decode(SocketType.self, forKey: .dataType)
        value = try container.decodeIfPresent(SocketValue.self, forKey: .value)
        isInput = try container.decode(Bool.self, forKey: .isInput)
    }
    
    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(id, forKey: .id)
        try container.encode(name, forKey: .name)
        try container.encode(dataType, forKey: .dataType)
        try container.encodeIfPresent(value, forKey: .value)
        try container.encode(isInput, forKey: .isInput)
    }
}

// MARK: - Socket Value

enum SocketValue: Codable, Equatable {
    case float(Float)
    case color(red: Float, green: Float, blue: Float, alpha: Float)
    case vector(x: Float, y: Float, z: Float)
    
    var asFloat: Float? {
        if case .float(let v) = self { return v }
        return nil
    }
    
    var asColor: (Float, Float, Float, Float)? {
        if case .color(let r, let g, let b, let a) = self {
            return (r, g, b, a)
        }
        return nil
    }
    
    var asVector: (Float, Float, Float)? {
        if case .vector(let x, let y, let z) = self {
            return (x, y, z)
        }
        return nil
    }
}

// MARK: - Graph Node

class MaterialGraphNode: Identifiable, ObservableObject, Codable {
    let id: UUID
    @Published var position: CGPoint
    let nodeType: MaterialNodeType
    @Published var inputs: [GraphSocket]
    @Published var outputs: [GraphSocket]
    
    init(id: UUID = UUID(), position: CGPoint, nodeType: MaterialNodeType) {
        self.id = id
        self.position = position
        self.nodeType = nodeType
        self.inputs = []
        self.outputs = []
        
        // Initialize sockets based on node type
        setupSockets()
    }
    
    private func setupSockets() {
        switch nodeType {
        case .materialOutput:
            inputs = [
                GraphSocket(name: "Surface", dataType: .shader, isInput: true),
                GraphSocket(name: "Volume", dataType: .shader, isInput: true),
                GraphSocket(name: "Displacement", dataType: .vector, isInput: true)
            ]
            
        case .principledBSDF:
            inputs = [
                GraphSocket(name: "Base Color", dataType: .color, value: .color(red: 0.8, green: 0.8, blue: 0.8, alpha: 1.0), isInput: true),
                GraphSocket(name: "Metallic", dataType: .float, value: .float(0.0), isInput: true),
                GraphSocket(name: "Roughness", dataType: .float, value: .float(0.5), isInput: true),
                GraphSocket(name: "Normal", dataType: .normal, isInput: true),
                GraphSocket(name: "Alpha", dataType: .float, value: .float(1.0), isInput: true)
            ]
            outputs = [
                GraphSocket(name: "BSDF", dataType: .shader, isInput: false)
            ]
            
        case .imageTexture:
            outputs = [
                GraphSocket(name: "Color", dataType: .color, isInput: false),
                GraphSocket(name: "Alpha", dataType: .float, isInput: false)
            ]
            
        case .mixRGB:
            inputs = [
                GraphSocket(name: "Fac", dataType: .float, value: .float(0.5), isInput: true),
                GraphSocket(name: "Color1", dataType: .color, value: .color(red: 0, green: 0, blue: 0, alpha: 1), isInput: true),
                GraphSocket(name: "Color2", dataType: .color, value: .color(red: 1, green: 1, blue: 1, alpha: 1), isInput: true)
            ]
            outputs = [
                GraphSocket(name: "Color", dataType: .color, isInput: false)
            ]
            
        case .math(let operation):
            inputs = [
                GraphSocket(name: "Value1", dataType: .float, value: .float(0.5), isInput: true),
                GraphSocket(name: "Value2", dataType: .float, value: .float(0.5), isInput: true)
            ]
            outputs = [
                GraphSocket(name: "Value", dataType: .float, isInput: false)
            ]
            
        case .colorRamp:
            inputs = [
                GraphSocket(name: "Fac", dataType: .float, value: .float(0.5), isInput: true)
            ]
            outputs = [
                GraphSocket(name: "Color", dataType: .color, isInput: false),
                GraphSocket(name: "Alpha", dataType: .float, isInput: false)
            ]
        }
    }
    
    var title: String {
        switch nodeType {
        case .materialOutput: return "Material Output"
        case .principledBSDF: return "Principled BSDF"
        case .imageTexture: return "Image Texture"
        case .mixRGB: return "Mix RGB"
        case .math: return "Math"
        case .colorRamp: return "ColorRamp"
        }
    }
    
    var headerColor: Color {
        switch nodeType {
        case .materialOutput: return Color(red: 0.8, green: 0.2, blue: 0.2)
        case .principledBSDF: return Color(red: 0.2, green: 0.8, blue: 0.2)
        case .imageTexture: return Color(red: 0.8, green: 0.6, blue: 0.2)
        case .mixRGB: return Color(red: 0.6, green: 0.2, blue: 0.8)
        case .math: return Color(red: 0.2, green: 0.4, blue: 0.8)
        case .colorRamp: return Color(red: 0.8, green: 0.4, blue: 0.6)
        }
    }
    
    // MARK: - Codable
    enum CodingKeys: String, CodingKey {
        case id, position, nodeType, inputs, outputs
    }
    
    required init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        id = try container.decode(UUID.self, forKey: .id)
        let x = try container.decode(Double.self, forKey: .position)
        let y = try container.decode(Double.self, forKey: .position)
        position = CGPoint(x: x, y: y)
        nodeType = try container.decode(MaterialNodeType.self, forKey: .nodeType)
        inputs = try container.decode([GraphSocket].self, forKey: .inputs)
        outputs = try container.decode([GraphSocket].self, forKey: .outputs)
    }
    
    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(id, forKey: .id)
        try container.encode(position.x, forKey: .position)
        try container.encode(position.y, forKey: .position)
        try container.encode(nodeType, forKey: .nodeType)
        try container.encode(inputs, forKey: .inputs)
        try container.encode(outputs, forKey: .outputs)
    }
}

// MARK: - Node Types

enum MaterialNodeType: Codable, Equatable {
    case materialOutput
    case principledBSDF
    case imageTexture
    case mixRGB
    case math(operation: MathOperation)
    case colorRamp
    
    enum MathOperation: String, Codable {
        case add, subtract, multiply, divide, power, sqrt
    }
}

// MARK: - Graph Link

struct GraphLink: Identifiable, Codable {
    let id: UUID
    let fromNodeID: UUID
    let fromSocketID: UUID
    let toNodeID: UUID
    let toSocketID: UUID
    
    init(id: UUID = UUID(), fromNodeID: UUID, fromSocketID: UUID, toNodeID: UUID, toSocketID: UUID) {
        self.id = id
        self.fromNodeID = fromNodeID
        self.fromSocketID = fromSocketID
        self.toNodeID = toNodeID
        self.toSocketID = toSocketID
    }
}

// MARK: - Material Graph

class MaterialGraph: ObservableObject, Codable {
    @Published var nodes: [MaterialGraphNode]
    @Published var links: [GraphLink]
    
    init() {
        self.nodes = []
        self.links = []
        
        // Create default Material Output node
        let outputNode = MaterialGraphNode(position: CGPoint(x: 400, y: 200), nodeType: .materialOutput)
        nodes.append(outputNode)
    }
    
    func addNode(_ node: MaterialGraphNode) {
        nodes.append(node)
    }
    
    func removeNode(_ id: UUID) {
        nodes.removeAll { $0.id == id }
        // Remove associated links
        links.removeAll { $0.fromNodeID == id || $0.toNodeID == id }
    }
    
    func addLink(_ link: GraphLink) {
        // Remove existing link to the same input socket
        links.removeAll { $0.toSocketID == link.toSocketID }
        links.append(link)
    }
    
    func removeLink(_ id: UUID) {
        links.removeAll { $0.id == id }
    }
    
    func findNode(_ id: UUID) -> MaterialGraphNode? {
        return nodes.first { $0.id == id }
    }
    
    func getConnectedSocket(to inputSocket: UUID) -> (MaterialGraphNode, GraphSocket)? {
        guard let link = links.first(where: { $0.toSocketID == inputSocket }),
              let node = findNode(link.fromNodeID),
              let socket = node.outputs.first(where: { $0.id == link.fromSocketID }) else {
            return nil
        }
        return (node, socket)
    }
    
    // MARK: - Codable
    enum CodingKeys: String, CodingKey {
        case nodes, links
    }
    
    required init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        nodes = try container.decode([MaterialGraphNode].self, forKey: .nodes)
        links = try container.decode([GraphLink].self, forKey: .links)
    }
    
    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(nodes, forKey: .nodes)
        try container.encode(links, forKey: .links)
    }
}
