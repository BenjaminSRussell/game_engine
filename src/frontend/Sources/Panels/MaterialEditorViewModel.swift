import SwiftUI
import Combine

// MARK: - Material Editor View Model
class MaterialEditorViewModel: ObservableObject {
    @Published var nodes: [MaterialNode] = []
    @Published var connections: [MaterialConnection] = []
    @Published var selectedNode: MaterialNode?
    @Published var isCompiling = false
    @Published var compilationError: String?
    @Published var previewMaterial: Material?
    
    private var cancellables = Set<AnyCancellable>()
    
    init() {
        // Setup default material graph
        setupDefaultGraph()
    }
    
    // MARK: - Node Management
    
    func addNode(_ nodeType: MaterialNodeType) {
        let node = MaterialNode(type: nodeType, position: CGPoint(x: 100, y: 100))
        nodes.append(node)
        print("[MaterialEditor] Added node: \(nodeType.rawValue)")
    }
    
    func removeNode(_ nodeId: UUID) {
        nodes.removeAll { $0.id == nodeId }
        connections.removeAll { $0.fromNode == nodeId || $0.toNode == nodeId }
        print("[MaterialEditor] Removed node: \(nodeId)")
    }
    
    func moveNode(_ nodeId: UUID, by offset: CGSize) {
        guard let index = nodes.firstIndex(where: { $0.id == nodeId }) else { return }
        nodes[index].position.x += offset.width
        nodes[index].position.y += offset.height
    }
    
    // MARK: - Connection Management
    
    func addConnection(_ connection: MaterialConnection) {
        connections.append(connection)
        print("[MaterialEditor] Added connection")
    }
    
    func removeConnection(_ connectionId: UUID) {
        connections.removeAll { $0.id == connectionId }
        print("[MaterialEditor] Removed connection")
    }
    
    // MARK: - Compilation
    
    func compileMaterial() {
        isCompiling = true
        compilationError = nil
        
        // Simulate compilation process
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) { [weak self] in
            guard let self = self else { return }
            
            do {
                let material = try self.performCompilation()
                self.previewMaterial = material
                self.isCompiling = false
                print("[MaterialEditor] Material compiled successfully")
            } catch {
                self.compilationError = error.localizedDescription
                self.isCompiling = false
                print("[MaterialEditor] Compilation failed: \(error)")
            }
        }
    }
    
    private func performCompilation() throws -> Material {
        // Validate graph
        try validateGraph()
        
        // Generate shader code
        let shaderCode = generateShaderCode()
        
        // Create material
        let material = Material(
            name: "Generated Material",
            shaderCode: shaderCode,
            properties: extractMaterialProperties()
        )
        
        return material
    }
    
    private func validateGraph() throws {
        // Check for cycles
        var visited = Set<UUID>()
        var recursionStack = Set<UUID>()
        
        for node in nodes {
            try detectCycles(nodeId: node.id, visited: &visited, recursionStack: &recursionStack)
        }
        
        // Check for unconnected inputs
        for node in nodes {
            for input in node.inputs {
                if input.isRequired && !isInputConnected(nodeId: node.id, inputId: input.id) {
                    throw MaterialCompilationError.requiredInputNotConnected(input.name)
                }
            }
        }
    }
    
    private func detectCycles(nodeId: UUID, visited: inout Set<UUID>, recursionStack: inout Set<UUID>) throws {
        if recursionStack.contains(nodeId) {
            throw MaterialCompilationError.cycleDetected
        }
        
        if visited.contains(nodeId) {
            return
        }
        
        visited.insert(nodeId)
        recursionStack.insert(nodeId)
        
        // Check outgoing connections
        for connection in connections {
            if connection.fromNode == nodeId {
                try detectCycles(nodeId: connection.toNode, visited: &visited, recursionStack: &recursionStack)
            }
        }
        
        recursionStack.remove(nodeId)
    }
    
    private func isInputConnected(nodeId: UUID, inputId: UUID) -> Bool {
        return connections.contains { $0.toNode == nodeId && $0.toInput == inputId }
    }
    
    private func generateShaderCode() -> String {
        var shaderCode = """
        // Generated Material Shader
        #include <metal_stdlib>
        using namespace metal;
        
        struct VertexOut {
            float4 position [[position]];
            float3 normal;
            float2 uv;
        };
        
        fragment float4 fragment_main(VertexOut in [[stage_in]]) {
        """
        
        // Process nodes in order
        for node in nodes {
            shaderCode += generateNodeCode(node)
        }
        
        shaderCode += """
            return float4(1.0, 1.0, 1.0, 1.0);
        }
        """
        
        return shaderCode
    }
    
    private func generateNodeCode(_ node: MaterialNode) -> String {
        switch node.type {
        case .baseColor:
            return "    float3 baseColor = float3(1.0, 1.0, 1.0); // \(node.name)"
        case .metallic:
            return "    float metallic = 0.5; // \(node.name)"
        case .roughness:
            return "    float roughness = 0.5; // \(node.name)"
        case .normalMap:
            return "    float3 normal = normalize(in.normal); // \(node.name)"
        case .emission:
            return "    float3 emission = float3(0.0, 0.0, 0.0); // \(node.name)"
        case .add:
            return "    // Add node: \(node.name)"
        case .multiply:
            return "    // Multiply node: \(node.name)"
        case .mix:
            return "    // Mix node: \(node.name)"
        case .textureSample:
            return "    // Texture sample: \(node.name)"
        case .vector3:
            return "    // Vector3: \(node.name)"
        case .float:
            return "    // Float: \(node.name)"
        }
    }
    
    private func extractMaterialProperties() -> [String: Any] {
        var properties: [String: Any] = [:]
        
        for node in nodes {
            for parameter in node.parameters {
                properties[parameter.name] = parameter.value
            }
        }
        
        return properties
    }
    
    // MARK: - Helper Methods
    
    func getNodeOutputPosition(_ nodeId: UUID, _ outputId: UUID) -> CGPoint {
        guard let node = nodes.first(where: { $0.id == nodeId }) else { return .zero }
        return CGPoint(
            x: node.position.x + 150, // Node width
            y: node.position.y + CGFloat(node.outputs.firstIndex(where: { $0.id == outputId }) ?? 0) * 25 + 25
        )
    }
    
    func getNodeInputPosition(_ nodeId: UUID, _ inputId: UUID) -> CGPoint {
        guard let node = nodes.first(where: { $0.id == nodeId }) else { return .zero }
        return CGPoint(
            x: node.position.x,
            y: node.position.y + CGFloat(node.inputs.firstIndex(where: { $0.id == inputId }) ?? 0) * 25 + 25
        )
    }
    
    private func setupDefaultGraph() {
        // Add default PBR nodes
        addNode(.baseColor)
        addNode(.metallic)
        addNode(.roughness)
        
        print("[MaterialEditor] Default material graph setup complete")
    }
}

// MARK: - Material Compilation Error

enum MaterialCompilationError: LocalizedError {
    case cycleDetected
    case requiredInputNotConnected(String)
    case invalidNodeType(String)
    case shaderGenerationFailed(String)
    
    var errorDescription: String? {
        switch self {
        case .cycleDetected:
            return "Cycle detected in material graph"
        case .requiredInputNotConnected(let inputName):
            return "Required input '\(inputName)' is not connected"
        case .invalidNodeType(let nodeType):
            return "Invalid node type: \(nodeType)"
        case .shaderGenerationFailed(let details):
            return "Shader generation failed: \(details)"
        }
    }
}

// MARK: - Supporting Models

struct Material {
    let name: String
    let shaderCode: String
    let properties: [String: Any]
}

struct MaterialNode: Identifiable {
    let id = UUID()
    var type: MaterialNodeType
    var position: CGPoint
    var name: String
    var inputs: [MaterialPort] = []
    var outputs: [MaterialPort] = []
    var parameters: [MaterialParameter] = []
    
    init(type: MaterialNodeType, position: CGPoint) {
        self.type = type
        self.position = position
        self.name = type.rawValue
        self.setupPorts()
        self.setupParameters()
    }
    
    private func setupPorts() {
        switch type {
        case .baseColor:
            outputs = [MaterialPort(id: UUID(), name: "Color", type: .vector3)]
        case .metallic:
            inputs = [MaterialPort(id: UUID(), name: "Value", type: .float, isRequired: false)]
            outputs = [MaterialPort(id: UUID(), name: "Metallic", type: .float)]
        case .roughness:
            inputs = [MaterialPort(id: UUID(), name: "Value", type: .float, isRequired: false)]
            outputs = [MaterialPort(id: UUID(), name: "Roughness", type: .float)]
        default:
            break
        }
    }
    
    private func setupParameters() {
        switch type {
        case .baseColor:
            parameters = [
                MaterialParameter(name: "Color", value: SIMD3<Float>(1, 1, 1), type: .vector3)
            ]
        case .metallic:
            parameters = [
                MaterialParameter(name: "Value", value: 0.5, type: .float)
            ]
        case .roughness:
            parameters = [
                MaterialParameter(name: "Value", value: 0.5, type: .float)
            ]
        default:
            break
        }
    }
}

struct MaterialConnection: Identifiable {
    let id = UUID()
    let fromNode: UUID
    let fromOutput: UUID
    let toNode: UUID
    let toInput: UUID
}

struct MaterialPort: Identifiable {
    let id = UUID()
    let name: String
    let type: MaterialPortType
    let isRequired: Bool
}

struct MaterialParameter {
    let name: String
    var value: Any
    let type: MaterialPortType
}

enum MaterialNodeType: String, CaseIterable {
    case baseColor = "Base Color"
    case metallic = "Metallic"
    case roughness = "Roughness"
    case normalMap = "Normal Map"
    case emission = "Emission"
    case add = "Add"
    case multiply = "Multiply"
    case mix = "Mix"
    case textureSample = "Texture Sample"
    case vector3 = "Vector3"
    case float = "Float"
}

enum MaterialPortType {
    case float, vector3, color, texture2D
}
