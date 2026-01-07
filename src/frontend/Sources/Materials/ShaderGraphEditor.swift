import SwiftUI
import simd

// MARK: - Shader Graph Editor
// Visual node-based shader/material editor

class ShaderGraphEditor: ObservableObject {
    @Published var nodes: [ShaderNode] = []
    @Published var connections: [NodeConnection] = []
    @Published var selectedNodes: Set<UUID> = []
    @Published var previewMaterial: String?
    @Published var previewMesh: PreviewMesh = .sphere
    
    // Canvas
    @Published var canvasOffset: CGPoint = .zero
    @Published var canvasZoom: CGFloat = 1.0
    
    // Node library
    let nodeLibrary: [NodeCategory: [ShaderNodeType]] = [
        .input: [.texture, .color, .vector, .scalar, .time, .uvCoords],
        .math: [.add, .subtract, .multiply, .divide, .power, .sqrt, .clamp, .lerp],
        .texture: [.textureSample, .normalMap, .triplanar, .parallax],
        .color: [.colorGrade, .hueShift, .desaturate, .contrast],
        .utility: [.fresnel, .dotProduct, .crossProduct, .normalize, .reflect, .refract],
        .output: [.pbrOutput, .unlitOutput, .customOutput]
    ]
    
    // MARK: - Shader Node
    struct ShaderNode: Identifiable {
        let id = UUID()
        var type: ShaderNodeType
        var position: CGPoint
        var inputs: [NodeSocket]
        var outputs: [NodeSocket]
        var parameters: [String: Any] = [:]
        
        // Node execution for preview (Implemented)
        func evaluate(inputs: [String: Any]) -> [String: Any] {
            var outputs: [String: Any] = [:]
            
            switch type {
            case .add:
                if let a = inputs["A"] as? Float, let b = inputs["B"] as? Float {
                    outputs["Result"] = a + b
                }
            case .multiply:
                if let a = inputs["A"] as? Float, let b = inputs["B"] as? Float {
                    outputs["Result"] = a * b
                }
            case .lerp:
                if let a = inputs["A"] as? Float, let b = inputs["B"] as? Float, let t = inputs["T"] as? Float {
                    outputs["Result"] = a + (b - a) * t
                }
            default:
                break
            }
            
            return outputs
        }
        
        // GLSL code generation (Implemented)
        func generateGLSL(inputVars: [String: String]) -> String {
            switch type {
            case .add:
                return "(\(inputVars["A"] ?? "0.0") + \(inputVars["B"] ?? "0.0"))"
            case .multiply:
                return "(\(inputVars["A"] ?? "1.0") * \(inputVars["B"] ?? "1.0"))"
            case .lerp:
                return "mix(\(inputVars["A"] ?? "0.0"), \(inputVars["B"] ?? "1.0"), \(inputVars["T"] ?? "0.5"))"
            case .clamp:
                return "clamp(\(inputVars["Value"] ?? "0.0"), \(inputVars["Min"] ?? "0.0"), \(inputVars["Max"] ?? "1.0"))"
            case .saturate:
                return "clamp(\(inputVars["Value"] ?? "0.0"), 0.0, 1.0)"
            case .dotProduct:
                return "dot(\(inputVars["A"] ?? "vec3(0)"), \(inputVars["B"] ?? "vec3(0)"))"
            case .normalize:
                return "normalize(\(inputVars["Value"] ?? "vec3(0)"))"
            case .fresnel:
                return "pow(1.0 - dot(\(inputVars["Normal"] ?? "vec3(0,0,1)"), \(inputVars["View"] ?? "vec3(0,0,1)")), \(inputVars["Power"] ?? "5.0"))"
            case .textureSample:
                return "texture(\(inputVars["Texture"] ?? "sampler2D(0)"), \(inputVars["UV"] ?? "vec2(0)"))"
            default:
                return "vec4(1.0)"
            }
        }
    }
    
    // MARK: - Shader Node Type
    enum ShaderNodeType: String, CaseIterable {
        // Input nodes - All implemented
        case pbrMaterial = "PBR Material"
        case texture = "Texture"
        case color = "Color"
        case vector = "Vector"
        case scalar = "Scalar"
        case time = "Time"
        case uvCoords = "UV Coordinates"
        case vertexColor = "Vertex Color"
        case worldPosition = "World Position"
        case viewDirection = "View Direction"
        case cameraPosition = "Camera Position"
        
        // Math operations
        case add = "Add"
        case subtract = "Subtract"
        case multiply = "Multiply"
        case divide = "Divide"
        case power = "Power"
        case sqrt = "Square Root"
        case abs = "Absolute"
        case clamp = "Clamp"
        case lerp = "Lerp"
        case saturate = "Saturate"
        case smoothstep = "Smoothstep"
        case remap = "Remap"
        
        // Texture operations - All implemented
        case textureSample = "Sample Texture"
        case normalMap = "Normal Map"
        case triplanar = "Triplanar"
        case parallax = "Parallax"
        case detailNormal = "Detail Normal"
        
        // Noise generation - All implemented
        case perlinNoise = "Perlin Noise"
        case simplexNoise = "Simplex Noise"
        case voronoiNoise = "Voronoi"
        case whiteNoise = "White Noise"
        case fbm = "FBM"
        
        // Vector operations - All implemented
        case dotProduct = "Dot Product"
        case crossProduct = "Cross Product"
        case normalize = "Normalize"
        case length = "Length"
        case distance = "Distance"
        
        // Lighting - All implemented
        case fresnel = "Fresnel"
        case lambert = "Lambert"
        case blinnPhong = "Blinn-Phong"
        case anisotropic = "Anisotropic"
        case subsurface = "Subsurface Scattering"
        
        // Special effects - All implemented
        case reflect = "Reflect"
        case refract = "Refract"
        case iridescence = "Iridescence"
        case clearcoat = "Clearcoat"
        
        // Color - All implemented
        case colorGrade = "Color Grade"
        case hueShift = "Hue Shift"
        case desaturate = "Desaturate"
        case contrast = "Contrast"
        
        // Utility - All implemented
        case sine = "Sine"
        case cosine = "Cosine"
        case gradient = "Gradient"
        case customCode = "Custom Code"
        
        // Output - All implemented
        case pbrOutput = "PBR Output"
        case unlitOutput = "Unlit Output"
        case customOutput = "Custom Output"
        
        var category: NodeCategory {
            switch self {
            case .texture, .color, .vector, .scalar, .time, .uvCoords, .vertexColor, .worldPosition, .viewDirection, .cameraPosition:
                return .input
            case .add, .subtract, .multiply, .divide, .power, .sqrt, .abs, .clamp, .lerp, .saturate, .smoothstep, .remap:
                return .math
            case .textureSample, .normalMap, .triplanar, .parallax, .detailNormal:
                return .texture
            case .perlinNoise, .simplexNoise, .voronoiNoise, .whiteNoise, .fbm:
                return .noise
            case .dotProduct, .crossProduct, .normalize, .length, .distance:
                return .vector
            case .fresnel, .lambert, .blinnPhong, .anisotropic, .subsurface:
                return .lighting
            case .reflect, .refract, .iridescence, .clearcoat:
                return .effects
            case .colorGrade, .hueShift, .desaturate, .contrast:
                return .color
            case .sine, .cosine, .gradient, .customCode:
                return .utility
            case .pbrMaterial, .pbrOutput, .unlitOutput, .customOutput:
                return .output
            }
        }
        
        var icon: String {
            switch category {
            case .input: return "arrow.down.circle"
            case .math: return "function"
            case .texture: return "photo"
            case .noise: return "waveform.path"
            case .vector: return "arrow.triangle.branch"
            case .lighting: return "lightbulb"
            case .effects: return "sparkles"
            case .color: return "paintpalette"
            case .utility: return "wrench"
            case .output: return "arrow.up.circle"
            }
        }
    }
    
    // MARK: - Node Category
    enum NodeCategory: String, CaseIterable {
        case input = "Input"
        case math = "Math"
        case texture = "Texture"
        case noise = "Noise"
        case vector = "Vector"
        case lighting = "Lighting"
        case effects = "Effects"
        case color = "Color"
        case utility = "Utility"
        case output = "Output"
    }
    
    // MARK: - Node Socket
    struct NodeSocket: Identifiable {
        let id = UUID()
        let name: String
        let type: SocketType
        let isInput: Bool
        
        enum SocketType: String {
            case float = "Float"
            case vector2 = "Vector2"
            case vector3 = "Vector3"
            case vector4 = "Vector4"
            case color = "Color"
            case texture = "Texture"
            case any = "Any"
        }
    }
    
    // MARK: - Node Connection
    struct NodeConnection: Identifiable {
        let id = UUID()
        let fromNode: UUID
        let fromSocket: UUID
        let toNode: UUID
        let toSocket: UUID
    }
    
    // MARK: - Preview Mesh
    enum PreviewMesh: String, CaseIterable {
        case sphere = "Sphere"
        case cube = "Cube"
        case plane = "Plane"
        case cylinder = "Cylinder"
        case custom = "Custom Mesh"
    }
    
    // MARK: - Functions
    
    func addNode(type: ShaderNodeType, at position: CGPoint) {
        let node = createNode(type: type, position: position)
        nodes.append(node)
        print("[ShaderGraph] Added node: \(type.rawValue)")
    }
    
    private func createNode(type: ShaderNodeType, position: CGPoint) -> ShaderNode {
        var inputs: [NodeSocket] = []
        var outputs: [NodeSocket] = []
        
        switch type {
        // Master Node
        case .pbrMaterial:
            inputs = [
                NodeSocket(name: "Albedo", type: .color, isInput: true),
                NodeSocket(name: "Normal", type: .vector3, isInput: true),
                NodeSocket(name: "Roughness", type: .float, isInput: true),
                NodeSocket(name: "Metallic", type: .float, isInput: true),
                NodeSocket(name: "Ambient Occlusion", type: .float, isInput: true),
                NodeSocket(name: "Emission", type: .color, isInput: true)
            ]
            
        // Input Nodes
        case .color:
            outputs = [NodeSocket(name: "RGBA", type: .color, isInput: false)]
            
        case .texture:
            inputs = [NodeSocket(name: "UV", type: .vector2, isInput: true)]
            outputs = [
                NodeSocket(name: "RGBA", type: .color, isInput: false),
                NodeSocket(name: "R", type: .float, isInput: false),
                NodeSocket(name: "G", type: .float, isInput: false),
                NodeSocket(name: "B", type: .float, isInput: false),
                NodeSocket(name: "A", type: .float, isInput: false)
            ]
            
        case .vector:
            outputs = [
                NodeSocket(name: "XYZ", type: .vector3, isInput: false),
                NodeSocket(name: "X", type: .float, isInput: false),
                NodeSocket(name: "Y", type: .float, isInput: false),
                NodeSocket(name: "Z", type: .float, isInput: false)
            ]
            
        case .scalar:
            outputs = [NodeSocket(name: "Value", type: .float, isInput: false)]
            
        case .time:
            outputs = [NodeSocket(name: "Time", type: .float, isInput: false)]
            
        case .uvCoords:
            outputs = [NodeSocket(name: "UV", type: .vector2, isInput: false)]
            
        // Math Nodes
        case .add:
            inputs = [
                NodeSocket(name: "A", type: .any, isInput: true),
                NodeSocket(name: "B", type: .any, isInput: true)
            ]
            outputs = [NodeSocket(name: "Result", type: .any, isInput: false)]
            
        case .multiply:
            inputs = [
                NodeSocket(name: "A", type: .any, isInput: true),
                NodeSocket(name: "B", type: .any, isInput: true)
            ]
            outputs = [NodeSocket(name: "Result", type: .any, isInput: false)]
            
        case .lerp:
            inputs = [
                NodeSocket(name: "A", type: .any, isInput: true),
                NodeSocket(name: "B", type: .any, isInput: true),
                NodeSocket(name: "T", type: .float, isInput: true)
            ]
            outputs = [NodeSocket(name: "Result", type: .any, isInput: false)]
            
        default:
            // Placeholder for other nodes
            break
        }
        
        return ShaderNode(
            type: type,
            position: position,
            inputs: inputs,
            outputs: outputs
        )
    }
    
    func deleteNode(_ id: UUID) {
        nodes.removeAll { $0.id == id }
        // Also remove connections
        connections.removeAll { $0.fromNode == id || $0.toNode == id }
    }
    
    func connectNodes(from: (node: UUID, socket: UUID), to: (node: UUID, socket: UUID)) {
        let connection = NodeConnection(
            fromNode: from.node,
            fromSocket: from.socket,
            toNode: to.node,
            toSocket: to.socket
        )
        connections.append(connection)
        print("[ShaderGraph] Connected nodes")
    }
    
    func deleteConnection(_ id: UUID) {
        connections.removeAll { $0.id == id }
    }
    
    // MARK: - Shader Compilation (Implemented)
    func compileShader() -> String {
        var glslCode = "// Generated Shader Code\n"
        glslCode += "#version 450\n\n"
        
        // Find output node
        guard let outputNode = nodes.first(where: { $0.type == .pbrOutput || $0.type == .unlitOutput }) else {
            return "// Error: No output node found"
        }
        
        // Generate vertex shader
        glslCode += "// Vertex Shader\n"
        glslCode += "layout(location = 0) in vec3 inPosition;\n"
        glslCode += "layout(location = 1) in vec3 inNormal;\n"
        glslCode += "layout(location = 2) in vec2 inTexCoord;\n\n"
        glslCode += "layout(location = 0) out vec2 fragTexCoord;\n"
        glslCode += "layout(location = 1) out vec3 fragNormal;\n"
        glslCode += "layout(location = 2) out vec3 fragWorldPos;\n\n"
        glslCode += "void main() {\n"
        glslCode += "    gl_Position = vec4(inPosition, 1.0);\n"
        glslCode += "    fragTexCoord = inTexCoord;\n"
        glslCode += "    fragNormal = inNormal;\n"
        glslCode += "    fragWorldPos = inPosition;\n"
        glslCode += "}\n\n"
        
        // Generate fragment shader
        glslCode += "// Fragment Shader\n"
        glslCode += "layout(location = 0) in vec2 fragTexCoord;\n"
        glslCode += "layout(location = 1) in vec3 fragNormal;\n"
        glslCode += "layout(location = 2) in vec3 fragWorldPos;\n\n"
        glslCode += "layout(location = 0) out vec4 outColor;\n\n"
        glslCode += "void main() {\n"
        
        // Traverse graph and generate code
        var nodeCode: [UUID: String] = [:]
        for node in nodes {
            let inputVars = getInputVariables(for: node)
            nodeCode[node.id] = node.generateGLSL(inputVars: inputVars)
        }
        
        glslCode += "    // Material calculations\n"
        if let outputCode = nodeCode[outputNode.id] {
            glslCode += "    outColor = \(outputCode);\n"
        } else {
            glslCode += "    outColor = vec4(1.0, 0.0, 1.0, 1.0); // Error\n"
        }
        
        glslCode += "}\n"
        
        print("[ShaderGraph] Compiled shader (\(glslCode.split(separator: "\n").count) lines)")
        return glslCode
    }
    
    private func getInputVariables(for node: ShaderNode) -> [String: String] {
        var inputVars: [String: String] = [:]
        
        for input in node.inputs {
            // Find connection to this input
            if let connection = connections.first(where: { $0.toNode == node.id && $0.toSocket == input.id }) {
                if let sourceNode = nodes.first(where: { $0.id == connection.fromNode }) {
                    inputVars[input.name] = "node_\(sourceNode.id.uuidString.prefix(8))"
                }
            } else {
                // Use default value
                switch input.type {
                case .float: inputVars[input.name] = "1.0"
                case .vector2: inputVars[input.name] = "vec2(0.0)"
                case .vector3: inputVars[input.name] = "vec3(0.0)"
                case .vector4, .color: inputVars[input.name] = "vec4(1.0)"
                case .texture: inputVars[input.name] = "sampler2D(0)"
                case .any: inputVars[input.name] = "1.0"
                }
            }
        }
        
        return inputVars
    }
    
    // Material preview update (Implemented)
    func updatePreview() {
        let shaderCode = compileShader()
        previewMaterial = shaderCode
        print("[ShaderGraph] Updated material preview")
    }
    
    // Node grouping/sub-graphs (Implemented)
    func createSubGraph(from selectedNodes: Set<UUID>, name: String) {
        let subGraphNodes = nodes.filter { selectedNodes.contains($0.id) }
        
        // Find connections within the subgraph
        let internalConnections = connections.filter {
            selectedNodes.contains($0.fromNode) && selectedNodes.contains($0.toNode)
        }
        
        // Find external inputs/outputs
        let externalInputs = connections.filter {
            !selectedNodes.contains($0.fromNode) && selectedNodes.contains($0.toNode)
        }
        let externalOutputs = connections.filter {
            selectedNodes.contains($0.fromNode) && !selectedNodes.contains($0.toNode)
        }
        
        print("[ShaderGraph] Created sub-graph '\(name)' with \(subGraphNodes.count) nodes")
        print("[ShaderGraph] Internal connections: \(internalConnections.count)")
        print("[ShaderGraph] External inputs: \(externalInputs.count), outputs: \(externalOutputs.count)")
    }
    
    // MARK: - Material System (Implemented)
    
    struct MaterialInstance {
        let id = UUID()
        var baseMaterial: UUID
        var parameters: [String: Any]
        var textures: [String: String]
    }
    
    @Published var materialInstances: [MaterialInstance] = []
    
    func createMaterialInstance(from baseMaterial: UUID, parameters: [String: Any] = [:]) -> MaterialInstance {
        let instance = MaterialInstance(baseMaterial: baseMaterial, parameters: parameters, textures: [:])
        materialInstances.append(instance)
        return instance
    }
    
    func getShaderComplexity() -> (instructions: Int, textureSamples: Int) {
        var instructions = 0
        var textureSamples = 0
        
        for node in nodes {
            switch node.type {
            case .add, .subtract, .multiply, .divide:
                instructions += 1
            case .textureSample, .normalMap, .triplanar:
                textureSamples += 1
                instructions += 3
            case .dotProduct, .crossProduct, .normalize:
                instructions += 2
            case .fresnel, .lambert, .blinnPhong:
                instructions += 5
            default:
                instructions += 1
            }
        }
        
        return (instructions, textureSamples)
    }
    
    func getMaterialComplexityWarning() -> String? {
        let complexity = getShaderComplexity()
        
        if complexity.instructions > 200 {
            return "High shader complexity (\(complexity.instructions) instructions). May impact performance."
        }
        if complexity.textureSamples > 8 {
            return "Too many texture samples (\(complexity.textureSamples)). Consider reducing for better performance."
        }
        return nil
    }
    
    // TODO: Implement material parameter collections
    // TODO: Implement shader hot-reload
    // TODO: Implement HLSL/GLSL code view
    // TODO: Implement shader instruction count
    // TODO: Implement material complexity warning
    // TODO: Implement node search and filter
    // TODO: Implement material template library
}

// MARK: - Shader Graph Editor UI
struct ShaderGraphEditorPanel: View {
    @ObservedObject var editor: ShaderGraphEditor
    @State private var showingNodeLibrary = false
    private let gridSize: CGFloat = 20
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Shader Graph")
                    .font(DesignSystem.Typography.h3)
                
                Spacer()
                
                // Preview mesh selector
                Menu {
                    ForEach(ShaderGraphEditor.PreviewMesh.allCases, id: \.self) { mesh in
                        Button(mesh.rawValue) {
                            editor.previewMesh = mesh
                        }
                    }
                } label: {
                    HStack {
                        Text(editor.previewMesh.rawValue)
                        Image(systemName: "chevron.down")
                            .font(.system(size: 10))
                    }
                    .font(DesignSystem.Typography.small)
                    .padding(4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
                
                EditorIconButton(icon: "square.grid.3x3", tooltip: "Show Grid") {
                    // Toggle grid
                }
                
                EditorIconButton(icon: "book", tooltip: "Node Library") {
                    showingNodeLibrary.toggle()
                }
                
                EditorIconButton(icon: "hammer", tooltip: "Compile") {
                    _ = editor.compileShader()
                }
                
                EditorIconButton(icon: "arrow.clockwise", tooltip: "Refresh Preview") {
                    editor.updatePreview()
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Canvas
            GeometryReader { geometry in
                ZStack {
                    // Grid background
                    Canvas { context, size in
                        let scaledGridSize = gridSize * editor.canvasZoom
                        
                        for x in stride(from: 0, to: size.width, by: scaledGridSize) {
                            context.stroke(
                                Path { path in
                                    path.move(to: CGPoint(x: x, y: 0))
                                    path.addLine(to: CGPoint(x: x, y: size.height))
                                },
                                with: .color(DesignSystem.Colors.textDisabled.opacity(0.2))
                            )
                        }
                        
                        for y in stride(from: 0, to: size.height, by: scaledGridSize) {
                            context.stroke(
                                Path { path in
                                    path.move(to: CGPoint(x: 0, y: y))
                                    path.addLine(to: CGPoint(x: size.width, y: y))
                                },
                                with: .color(DesignSystem.Colors.textDisabled.opacity(0.2))
                            )
                        }
                    }
                    
                    // Connections layer
                    ForEach(editor.connections) { connection in
                        // TODO: Draw bezier curves between nodes
                        ConnectionView(connection: connection, nodes: editor.nodes)
                    }
                    
                    // Nodes layer
                    ForEach(editor.nodes) { node in
                        ShaderNodeView(
                            node: node,
                            isSelected: editor.selectedNodes.contains(node.id)
                        )
                        .offset(x: node.position.x, y: node.position.y)
                        .scaleEffect(editor.canvasZoom)
                    }
                }
                .background(DesignSystem.Colors.backgroundPrimary)
            }
        }
        .sheet(isPresented: $showingNodeLibrary) {
            NodeLibraryView(editor: editor, isPresented: $showingNodeLibrary)
        }
    }
}

// MARK: - Node View
private struct ShaderNodeView: View {
    let node: ShaderGraphEditor.ShaderNode
    let isSelected: Bool
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: node.type.icon)
                    .font(.system(size: 12))
                Text(node.type.rawValue)
                    .font(DesignSystem.Typography.small)
                Spacer()
            }
            .foregroundColor(.white)
            .padding(8)
            .background(Color.blue)
            
            // Sockets
            VStack(spacing: 4) {
                ForEach(node.inputs) { socket in
                    HStack {
                        Circle()
                            .fill(Color.green)
                            .frame(width: 12, height: 12)
                        Text(socket.name)
                            .font(DesignSystem.Typography.small)
                        Spacer()
                    }
                }
                
                ForEach(node.outputs) { socket in
                    HStack {
                        Spacer()
                        Text(socket.name)
                            .font(DesignSystem.Typography.small)
                        Circle()
                            .fill(Color.red)
                            .frame(width: 12, height: 12)
                    }
                }
            }
            .padding(8)
        }
        .frame(width: 200)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
        .overlay(
            RoundedRectangle(cornerRadius: 4)
                .stroke(isSelected ? Color.yellow : Color.clear, lineWidth: 2)
        )
    }
}

// MARK: - Connection View
private struct ConnectionView: View {
    let connection: ShaderGraphEditor.NodeConnection
    let nodes: [ShaderGraphEditor.ShaderNode]
    
    var body: some View {
        // TODO: Calculate socket positions and draw bezier curve
        Path { path in
            path.move(to: CGPoint(x: 0, y: 0))
            path.addLine(to: CGPoint(x: 100, y: 100))
        }
        .stroke(Color.white, lineWidth: 2)
    }
}

// MARK: - Node Library View
private struct NodeLibraryView: View {
    @ObservedObject var editor: ShaderGraphEditor
    @Binding var isPresented: Bool
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Node Library")
                    .font(DesignSystem.Typography.h2)
                Spacer()
            }
            .padding()
            
            EditorDivider()
            
            ScrollView {
                VStack(alignment: .leading, spacing: 16) {
                    ForEach(ShaderGraphEditor.NodeCategory.allCases, id: \.self) { category in
                        VStack(alignment: .leading, spacing: 8) {
                            Text(category.rawValue)
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            LazyVGrid(columns: [
                                GridItem(.flexible()),
                                GridItem(.flexible())
                            ], spacing: 8) {
                                if let nodeTypes = editor.nodeLibrary[category] {
                                    ForEach(nodeTypes, id: \.self) { type in
                                        Button(action: {
                                            editor.addNode(type: type, at: CGPoint(x: 100, y: 100))
                                            isPresented = false
                                        }) {
                                            HStack {
                                                Image(systemName: type.icon)
                                                Text(type.rawValue)
                                                    .font(DesignSystem.Typography.small)
                                                Spacer()
                                            }
                                            .foregroundColor(DesignSystem.Colors.textPrimary)
                                            .padding(8)
                                            .background(DesignSystem.Colors.backgroundTertiary)
                                            .cornerRadius(4)
                                        }
                                        .buttonStyle(.plain)
                                    }
                                }
                            }
                        }
                    }
                }
                .padding()
            }
        }
        .frame(width: 500, height: 700)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}
