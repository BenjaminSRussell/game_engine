import SwiftUI

// MARK: - Material Editor Panel

struct MaterialEditorPanel: View {
    @StateObject private var viewModel = MaterialEditorViewModel()
    @State private var selectedNode: MaterialNode?
    @State private var dragOffset: CGSize = .zero
    
    var body: some View {
        GeometryReader { geometry in
            HStack(spacing: 0) {
                // Left: Node Library
                NodeLibraryPanel(onNodeSelected: { nodeType in
                    viewModel.addNode(nodeType)
                })
                .frame(width: 250)
                
                EditorDivider()
                
                // Center: Node Graph
                ZStack {
                    // Grid background
                    MaterialGridBackground()
                    
                    // Connections
                    ForEach(viewModel.connections) { connection in
                        ConnectionLine(
                            from: viewModel.getNodeOutputPosition(connection.fromNode, connection.fromOutput),
                            to: viewModel.getNodeInputPosition(connection.toNode, connection.toInput)
                        )
                    }
                    
                    // Nodes
                    ForEach(viewModel.nodes) { node in
                        MaterialPreviewNodeView(
                            node: node,
                            isSelected: selectedNode?.id == node.id,
                            onSelect: { selectedNode = node },
                            onMove: { offset in
                                viewModel.moveNode(node.id, by: offset)
                            }
                        )
                        .position(node.position)
                    }
                }
                .background(DesignSystem.Colors.backgroundPrimary)
                .gesture(
                    DragGesture()
                        .onChanged { value in
                            // Pan canvas
                            dragOffset = value.translation
                        }
                )
                
                EditorDivider()
                
                // Right: Preview & Properties
                VStack(spacing: 0) {
                    // Material Preview
                    MaterialPreviewView(material: viewModel.currentMaterial)
                        .frame(height: 300)
                    
                    EditorDivider()
                    
                    // Node Properties
                    ScrollView {
                        if let selected = selectedNode {
                            NodePropertiesView(node: selected)
                        } else {
                            VStack(spacing: DesignSystem.Spacing.md) {
                                Image(systemName: "cube.transparent")
                                    .font(.system(size: 48))
                                    .foregroundColor(DesignSystem.Colors.textDisabled)
                                
                                Text("Select a node to edit properties")
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                            .frame(maxWidth: .infinity, maxHeight: .infinity)
                        }
                    }
                }
                .frame(width: 320)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Node Library Panel

private struct NodeLibraryPanel: View {
    let onNodeSelected: (PanelNodeType) -> Void
    @State private var searchText = ""
    @State private var selectedCategory: NodeCategory = .input
    
    enum NodeCategory: String, CaseIterable {
        case input = "Input"
        case math = "Math"
        case texture = "Texture"
        case color = "Color"
        case utility = "Utility"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Title
            HStack {
                Text("Node Library")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
            
            EditorDivider()
            
            // Search
            EditorSearchBar(text: $searchText, placeholder: "Search nodes...")
                .padding(DesignSystem.Spacing.sm)
            
            EditorDivider()
            
            // Categories
            ScrollView {
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
                    ForEach(NodeCategory.allCases, id: \.self) { category in
                        CategorySection(
                            category: category,
                            isExpanded: selectedCategory == category,
                            onSelect: { selectedCategory = category },
                            onNodeSelect: onNodeSelected
                        )
                    }
                }
                .padding(DesignSystem.Spacing.sm)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

private struct CategorySection: View {
    let category: NodeLibraryPanel.NodeCategory
    let isExpanded: Bool
    let onSelect: () -> Void
    let onNodeSelect: (PanelNodeType) -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
            Button(action: onSelect) {
                HStack {
                    Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                        .font(.system(size: 10))
                    Text(category.rawValue)
                        .font(DesignSystem.Typography.bodyBold)
                    Spacer()
                }
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.vertical, DesignSystem.Spacing.xs)
            }
            .buttonStyle(.plain)
            
            if isExpanded {
                ForEach(nodesForCategory(category), id: \.self) { nodeType in
                    NodeLibraryItem(nodeType: nodeType, onSelect: onNodeSelect)
                }
            }
        }
    }
    
    private func nodesForCategory(_ category: NodeLibraryPanel.NodeCategory) -> [PanelNodeType] {
        switch category {
        case .input:
            return [.albedo, .metallic, .roughness, .normal, .emission]
        case .math:
            return [.add, .multiply, .subtract, .divide, .lerp]
        case .texture:
            return [.textureSample, .textureCoordinate, .normalMap]
        case .color:
            return [.colorConstant, .colorMix, .hsvToRgb]
        case .utility:
            return [.fresnel, .dotProduct, .clamp]
        }
    }
}

private struct NodeLibraryItem: View {
    let nodeType: PanelNodeType
    let onSelect: (PanelNodeType) -> Void
    @State private var isHovering = false
    
    var body: some View {
        Button(action: { onSelect(nodeType) }) {
            HStack {
                Image(systemName: nodeType.icon)
                    .font(.system(size: 12))
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(width: 16)
                
                Text(nodeType.displayName)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
            }
            .padding(.vertical, DesignSystem.Spacing.xs)
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .background(isHovering ? DesignSystem.Colors.hover : Color.clear)
            .cornerRadius(4)
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            isHovering = hovering
        }
    }
}

// MARK: - Material Node View

private struct MaterialPreviewNodeView: View {
    @ObservedObject var node: MaterialNode
    let isSelected: Bool
    let onSelect: () -> Void
    let onMove: (CGSize) -> Void
    
    @State private var isDragging = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: node.type.icon)
                    .font(.system(size: 12))
                    .foregroundColor(.white)
                
                Text(node.type.displayName)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white)
                
                Spacer()
            }
            .padding(DesignSystem.Spacing.sm)
            .background(node.type.color)
            
            // Inputs and Outputs
            VStack(spacing: DesignSystem.Spacing.xs) {
                ForEach(node.inputs) { input in
                    NodeSocketView(socket: input, isInput: true)
                }
                
                ForEach(node.outputs) { output in
                    NodeSocketView(socket: output, isInput: false)
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundTertiary)
        }
        .frame(width: 180)
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: isSelected ? 2 : 1)
        )
        .shadow(
            color: DesignSystem.Shadows.medium.color,
            radius: DesignSystem.Shadows.medium.radius,
            x: DesignSystem.Shadows.medium.x,
            y: DesignSystem.Shadows.medium.y
        )
        .onTapGesture {
            onSelect()
        }
        .gesture(
            DragGesture()
                .onChanged { value in
                    isDragging = true
                    onMove(value.translation)
                }
                .onEnded { _ in
                    isDragging = false
                }
        )
    }
}

private struct NodeSocketView: View {
    let socket: NodeSocket
    let isInput: Bool
    
    var body: some View {
        HStack {
            if isInput {
                Circle()
                    .fill(socket.type.color)
                    .frame(width: 12, height: 12)
                    .overlay(
                        Circle()
                            .stroke(DesignSystem.Colors.border, lineWidth: 1)
                    )
                
                Text(socket.name)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
            } else {
                Spacer()
                
                Text(socket.name)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Circle()
                    .fill(socket.type.color)
                    .frame(width: 12, height: 12)
                    .overlay(
                        Circle()
                            .stroke(DesignSystem.Colors.border, lineWidth: 1)
                    )
            }
        }
    }
}

// MARK: - Connection Line

private struct ConnectionLine: View {
    let from: CGPoint
    let to: CGPoint
    
    var body: some View {
        Path { path in
            path.move(to: from)
            
            let controlPoint1 = CGPoint(x: from.x + 50, y: from.y)
            let controlPoint2 = CGPoint(x: to.x - 50, y: to.y)
            
            path.addCurve(to: to, control1: controlPoint1, control2: controlPoint2)
        }
        .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
    }
}

// MARK: - Grid Background

private struct MaterialGridBackground: View {
    var body: some View {
        GeometryReader { geometry in
            Path { path in
                let spacing: CGFloat = 20
                
                // Vertical lines
                for x in stride(from: 0, through: geometry.size.width, by: spacing) {
                    path.move(to: CGPoint(x: x, y: 0))
                    path.addLine(to: CGPoint(x: x, y: geometry.size.height))
                }
                
                // Horizontal lines
                for y in stride(from: 0, through: geometry.size.height, by: spacing) {
                    path.move(to: CGPoint(x: 0, y: y))
                    path.addLine(to: CGPoint(x: geometry.size.width, y: y))
                }
            }
            .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 0.5)
        }
    }
}

// MARK: - Material Preview

private struct MaterialPreviewView: View {
    let material: Material
    
    var body: some View {
        ZStack {
            Color.black
            
            // 3D preview sphere (placeholder)
            Circle()
                .fill(
                    LinearGradient(
                        colors: [Color.white, Color.gray],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    )
                )
                .frame(width: 150, height: 150)
            
            VStack {
                Spacer()
                
                HStack {
                    Text("Preview")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    EditorIconButton(icon: "arrow.clockwise", tooltip: "Refresh Preview") {
                        // Refresh material preview
                        // TODO: Material compilation needs to be wired up to view model
                        print("[Swift] Material preview refresh requested")
                    }
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.backgroundTertiary.opacity(0.9))
            }
        }
    }
}

// MARK: - Node Properties View

private struct NodePropertiesView: View {
    @ObservedObject var node: MaterialNode
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Node Properties")
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(DesignSystem.Spacing.sm)
            
            EditorDivider()
            
            // Node-specific properties
            switch node.type {
            case .colorConstant:
                ColorPicker("Color", selection: .constant(Color.white))
                    .padding(.horizontal, DesignSystem.Spacing.sm)
            
            case .textureSample:
                VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                    Text("Texture")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("None")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        EditorIconButton(icon: "magnifyingglass", tooltip: "Select Texture") {
                            // Open texture picker
                            #if os(macOS)
                            let panel = NSOpenPanel()
                            panel.allowedContentTypes = [.image]
                            panel.begin { response in
                                if response == .OK, let url = panel.url {
                                    print("[Swift] Selected texture: \(url.lastPathComponent)")
                                    // Would assign to node here
                                }
                            }
                            #endif
                        }
                    }
                    .padding(DesignSystem.Spacing.sm)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
            
            default:
                Text("No properties")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .padding(DesignSystem.Spacing.sm)
            }
            
            Spacer()
        }
    }
}

// MARK: - Data Models

enum PanelNodeType: Hashable {
    case albedo, metallic, roughness, normal, emission
    case add, multiply, subtract, divide, lerp
    case textureSample, textureCoordinate, normalMap
    case colorConstant, colorMix, hsvToRgb
    case fresnel, dotProduct, clamp
    
    var displayName: String {
        switch self {
        case .albedo: return "Albedo"
        case .metallic: return "Metallic"
        case .roughness: return "Roughness"
        case .normal: return "Normal"
        case .emission: return "Emission"
        case .add: return "Add"
        case .multiply: return "Multiply"
        case .subtract: return "Subtract"
        case .divide: return "Divide"
        case .lerp: return "Lerp"
        case .textureSample: return "Texture Sample"
        case .textureCoordinate: return "Texture Coordinate"
        case .normalMap: return "Normal Map"
        case .colorConstant: return "Color"
        case .colorMix: return "Color Mix"
        case .hsvToRgb: return "HSV to RGB"
        case .fresnel: return "Fresnel"
        case .dotProduct: return "Dot Product"
        case .clamp: return "Clamp"
        }
    }
    
    var icon: String {
        switch self {
        case .albedo, .metallic,.roughness, .normal, .emission:
            return "circle.fill"
        case .add, .subtract, .multiply, .divide, .lerp:
            return "function"
        case .textureSample, .textureCoordinate, .normalMap:
            return "photo"
        case .colorConstant, .colorMix, .hsvToRgb:
            return "paintpalette"
        case .fresnel, .dotProduct, .clamp:
            return "waveform"
        }
    }
    
    var color: Color {
        switch self {
        case .albedo, .metallic, .roughness, .normal, .emission:
            return DesignSystem.Colors.accentSuccess
        case .add, .subtract, .multiply, .divide, .lerp:
            return Color(hex: "#7C4DFF")
        case .textureSample, .textureCoordinate, .normalMap:
            return Color(hex: "#FF6F00")
        case .colorConstant, .colorMix, .hsvToRgb:
            return Color(hex: "#E91E63")
        case .fresnel, .dotProduct, .clamp:
            return Color(hex: "#00BCD4")
        }
    }
}

class MaterialNode: Identifiable, ObservableObject {
    let id = UUID()
    let type: PanelNodeType
    @Published var position: CGPoint
    @Published var inputs: [NodeSocket]
    @Published var outputs: [NodeSocket]
    
    init(type: PanelNodeType, position: CGPoint = .zero) {
        self.type = type
        self.position = position
        self.inputs = []
        self.outputs = []
        
        // Configure inputs/outputs based on type
        configureSocketsForType(type)
    }
    
    private func configureSocketsForType(_ type: PanelNodeType) {
        switch type {
        case .add, .multiply, .subtract, .divide:
            inputs = [
                NodeSocket(name: "A", type: .float),
                NodeSocket(name: "B", type: .float)
            ]
            outputs = [NodeSocket(name: "Result", type: .float)]
            
        case .textureSample:
            inputs = [NodeSocket(name: "UV", type: .vector2)]
            outputs = [
                NodeSocket(name: "RGB", type: .vector3),
                NodeSocket(name: "Alpha", type: .float)
            ]
            
        case .colorConstant:
            outputs = [NodeSocket(name: "Color", type: .vector3)]
            
        default:
            outputs = [NodeSocket(name: "Output", type: .float)]
        }
    }
}

struct NodeSocket: Identifiable {
    let id = UUID()
    let name: String
    let type: SocketType
    
    enum SocketType {
        case float, vector2, vector3, vector4, color
        
        var color: Color {
            switch self {
            case .float: return Color.gray
            case .vector2: return Color.green
            case .vector3: return Color.blue
            case .vector4: return Color.purple
            case .color: return Color.red
            }
        }
    }
}

struct NodeConnection: Identifiable {
    let id = UUID()
    let fromNode: UUID
    let fromOutput: UUID
    let toNode: UUID
    let toInput: UUID
}

struct Material {
    var name: String = "New Material"
    var albedoColor: Color = .white
    var metallic: Float = 0.0
    var roughness: Float = 0.5
}

// MARK: - View Model

class MaterialEditorViewModel: ObservableObject {
    @Published var nodes: [MaterialNode] = []
    @Published var connections: [NodeConnection] = []
    @Published var currentMaterial = Material()
    
    init() {
        // Create demo nodes
        addDemoNodes()
    }
    
    func addDemoNodes() {
        let albedoNode = MaterialNode(type: .albedo, position: CGPoint(x: 200, y: 100))
        let textureNode = MaterialNode(type: .textureSample, position: CGPoint(x: 200, y: 250))
        let colorNode = MaterialNode(type: .colorConstant, position: CGPoint(x: 200, y: 400))
        
        nodes = [albedoNode, textureNode, colorNode]
    }
    
    func addNode(_ type: PanelNodeType) {
        let newNode = MaterialNode(type: type, position: CGPoint(x: 300, y: 300))
        nodes.append(newNode)
    }
    
    func moveNode(_ id: UUID, by offset: CGSize) {
        if let index = nodes.firstIndex(where: { $0.id == id }) {
            nodes[index].position.x += offset.width
            nodes[index].position.y += offset.height
        }
    }
    
    func getNodeOutputPosition(_ nodeId: UUID, _ socketId: UUID) -> CGPoint {
        // Calculate socket position based on node bounds
        let socketSpacing: CGFloat = 25
        guard let node = nodes.first(where: { $0.id == nodeId }),
              let index = node.outputs.firstIndex(where: { $0.id == socketId }) else {
            return .zero
        }
        
       let yOffset = socketSpacing * CGFloat(index) + 30 // Offset from top of node
        return CGPoint(x: node.position.x + 150, y: node.position.y + yOffset) // Assuming node width 150
    }
    
    func getNodeInputPosition(_ nodeId: UUID, _ socketId: UUID) -> CGPoint {
        guard let node = nodes.first(where: { $0.id == nodeId }),
              let index = node.inputs.firstIndex(where: { $0.id == socketId }) else {
            return .zero
        }
        
        let socketSpacing: CGFloat = 25
        let yOffset = socketSpacing * CGFloat(index) + 30 // Offset from top of node
        return CGPoint(x: node.position.x, y: node.position.y + yOffset)
    }
}

// MARK: - Preview

#Preview {
    MaterialEditorPanel()
        .frame(width: 1200, height: 800)
}
