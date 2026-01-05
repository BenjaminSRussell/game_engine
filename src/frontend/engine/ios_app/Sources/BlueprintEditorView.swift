import SwiftUI

struct BlueprintEditorView: View {
    @StateObject private var graph = BlueprintGraph()
    @State private var dragOffset: CGSize = .zero
    @State private var zoom: CGFloat = 1.0
    
    var body: some View {
        HStack(spacing: 0) {
            // Left: Node Library & Variables
            VStack(spacing: 0) {
                SectionHeader(title: "NODE LIBRARY")
                List {
                    ForEach(Array(Dictionary(grouping: BlueprintNodeType.allCases, by: { $0.category }).keys.sorted()), id: \.self) { category in
                        DisclosureGroup(category.uppercased()) {
                            ForEach(BlueprintNodeType.allCases.filter { $0.category == category }) { type in
                                Text(type.rawValue)
                                    .font(.caption2)
                                    .padding(.vertical, 4)
                                    .onDrag { NSItemProvider(object: type.rawValue as NSString) }
                            }
                        }
                        .font(.caption.bold())
                        .foregroundColor(.cyan)
                    }
                }
                .listStyle(PlainListStyle())
                
                Divider().background(Color.white.opacity(0.1))
                
                SectionHeader(title: "VARIABLES")
                List {
                    ForEach(graph.variables) { variable in
                        HStack {
                            Image(systemName: "v.square.fill")
                                .foregroundColor(pinColor(for: variable.type))
                            Text(variable.name)
                                .font(.caption2)
                            Spacer()
                        }
                        .padding(.vertical, 2)
                        .onDrag { NSItemProvider(object: "var:\(variable.id)" as NSString) }
                    }
                    
                    Button(action: { graph.addVariable() }) {
                        Text("+ Add Variable")
                            .font(.caption2.bold())
                            .foregroundColor(.green)
                    }
                    .padding(.vertical, 4)
                }
                .listStyle(PlainListStyle())
                .frame(height: 250)
            }
            .frame(width: 200)
            .background(Color(white: 0.12))
            
            // Center: Blueprint Canvas
            ZStack {
                CanvasBackgroundGrid()
                
                // Blueprint Links (Wires)
                ForEach(graph.links) { link in
                    BlueprintLinkView(link: link, graph: graph)
                }
                
                // Nodes
                ForEach(graph.nodes) { node in
                    BlueprintNodeView(node: node, graph: graph)
                        .position(x: node.position.x + dragOffset.width, y: node.position.y + dragOffset.height)
                        .onTapGesture {
                            graph.selectedNodeID = node.id
                        }
                        .gesture(
                            DragGesture()
                                .onChanged { value in
                                    graph.selectedNodeID = node.id
                                    node.position = CGPoint(x: node.position.x + value.translation.width, y: node.position.y + value.translation.height)
                                }
                        )
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Color.black)
            .clipped()
            .onTapGesture {
                graph.selectedNodeID = nil
            }
            .dropDestination(for: String.self) { items, location in
                if let typeStr = items.first, let type = BlueprintNodeType(rawValue: typeStr) {
                    graph.addNode(type: type, at: location)
                    return true
                }
                return false
            }
            
            // Right: Details Panel
            VStack(spacing: 0) {
                SectionHeader(title: "DETAILS")
                
                ScrollView {
                    VStack(alignment: .leading, spacing: 16) {
                        if let selectedID = graph.selectedNodeID,
                           let node = graph.nodes.first(where: { $0.id == selectedID }) {
                            // Node Details
                            PropertySection(title: "NODE: \(node.type.rawValue)") {
                                PropertyRow(label: "Node ID") {
                                    Text(node.id.uuidString.prefix(8))
                                        .font(.system(size: 10, design: .monospaced))
                                        .foregroundColor(.gray)
                                }
                                PropertyRow(label: "Position X") {
                                    Text("\(Int(node.position.x))")
                                        .foregroundColor(.cyan)
                                }
                                PropertyRow(label: "Position Y") {
                                    Text("\(Int(node.position.y))")
                                        .foregroundColor(.cyan)
                                }
                            }
                        } else {
                            // Graph Details
                            PropertySection(title: "GRAPH SETTINGS") {
                                Button(action: { graph.simulate() }) {
                                    Label("Simulate Logic", systemImage: "play.fill")
                                        .frame(maxWidth: .infinity)
                                }
                                .buttonStyle(BorderedButtonStyle())
                                .accentColor(.orange)
                                .padding(.bottom, 8)
                                
                                PropertyRow(label: "Target Entity") {
                                    TextField("Entity ID", text: $graph.targetEntityID)
                                        .textFieldStyle(PlainTextFieldStyle())
                                        .font(.caption)
                                        .foregroundColor(.cyan)
                                }
                                PropertyRow(label: "Logic Type") {
                                    Text("Component-Driven")
                                        .font(.caption)
                                        .foregroundColor(.gray)
                                }
                            }
                        }
                    }
                    .padding()
                }
            }
            .frame(width: 200)
            .background(Color(white: 0.12))
        }
    }
}

struct BlueprintVariable: Identifiable {
    let id = UUID()
    var name: String
    var type: BlueprintPinType
    var defaultValue: String
}

class BlueprintGraph: ObservableObject {
    @Published var nodes: [BlueprintNode] = []
    @Published var links: [BlueprintLink] = []
    @Published var variables: [BlueprintVariable] = []
    
    init() {
        // Default variables
        variables.append(BlueprintVariable(name: "PlayerHealth", type: .float, defaultValue: "100.0"))
        variables.append(BlueprintVariable(name: "IsGameOver", type: .bool, defaultValue: "false"))
    }
    
    func addVariable() {
        variables.append(BlueprintVariable(name: "NewVariable", type: .int, defaultValue: "0"))
    }
    
    func addNode(type: BlueprintNodeType, at position: CGPoint) {
        // This part of the instruction seems to be a copy-paste error from BlueprintNodeView.
        // Assuming the user intended to add the variable management logic to BlueprintGraph
        // and not replace the addNode implementation with UI elements.
        // The original addNode implementation is not provided in the context,
        // so I'll add a placeholder or assume it exists elsewhere.
        // For now, I'll just add a basic node creation.
        let newNode = BlueprintNode(type: type, position: position)
        nodes.append(newNode)
    }
}

struct BlueprintNodeView: View {
    @ObservedObject var node: BlueprintNode
    @ObservedObject var graph: BlueprintGraph
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header (Unreal style)
            HStack {
                Image(systemName: headerIcon)
                Text(node.type.rawValue)
                    .font(.system(size: 11, weight: .bold))
            }
            .padding(8)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(headerColor)
            .foregroundColor(.white)
            
            HStack(alignment: .top, spacing: 10) {
                // Inputs
                VStack(alignment: .leading, spacing: 10) {
                    ForEach(node.inputs) { pin in
                        BlueprintPinView(pin: pin)
                    }
                }
                
                Spacer()
                
                // Outputs
                VStack(alignment: .trailing, spacing: 10) {
                    ForEach(node.outputs) { pin in
                        BlueprintPinView(pin: pin)
                    }
                }
            }
            .padding(10)
        }
        .frame(minWidth: 160)
        .background(Color(white: 0.18).opacity(0.95))
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(node.isExecuting ? Color.orange : (graph.selectedNodeID == node.id ? Color.cyan : Color.white.opacity(0.1)), lineWidth: (node.isExecuting || graph.selectedNodeID == node.id) ? 2 : 1)
        )
        .shadow(color: node.isExecuting ? .orange : (graph.selectedNodeID == node.id ? .cyan.opacity(0.3) : .black.opacity(0.5)), radius: node.isExecuting ? 15 : (graph.selectedNodeID == node.id ? 10 : 6), x: 0, y: 3)
    }
    
    private var headerIcon: String {
        switch node.type.category {
        case "Events": return "bolt.fill"
        case "Control Flow": return "arrow.branch"
        case "Variables": return "v.square"
        default: return "f.square"
        }
    }
    
    private var headerColor: Color {
        switch node.type.category {
        case "Events": return Color(red: 0.8, green: 0, blue: 0)
        case "Control Flow": return Color(red: 0.2, green: 0.4, blue: 0.8)
        case "Variables": return Color(red: 0.2, green: 0.6, blue: 0.2)
        default: return Color(white: 0.25)
        }
    }
}

struct BlueprintPinView: View {
    let pin: BlueprintPin
    
    var body: some View {
        HStack(spacing: 6) {
            if !pin.isOutput {
                PinShape(type: pin.type)
            }
            
            Text(pin.name)
                .font(.system(size: 10))
                .foregroundColor(.white.opacity(0.8))
            
            if pin.isOutput {
                PinShape(type: pin.type)
            }
        }
    }
}

struct PinShape: View {
    let type: BlueprintPinType
    
    var body: some View {
        if type == .exec {
            Polygon(sides: 5)
                .fill(Color.white)
                .frame(width: 10, height: 10)
        } else {
            Circle()
                .fill(pinColor)
                .frame(width: 10, height: 10)
                .overlay(Circle().stroke(Color.white.opacity(0.2), lineWidth: 1))
        }
    }
    
    private var pinColor: Color {
        switch type {
        case .bool: return .red
        case .float: return .cyan
        case .int: return .green
        case .string: return .pink
        case .vector: return .yellow
        case .entity: return .blue
        default: return .white
        }
    }
}

struct BlueprintLinkView: View {
    let link: BlueprintLink
    @ObservedObject var graph: BlueprintGraph
    
    var body: some View {
        if let fromNode = graph.nodes.first(where: { $0.id == link.fromNodeID }),
           let toNode = graph.nodes.first(where: { $0.id == link.toNodeID }) {
            
            // Simplified socket position calculation for demo
            let start = fromNode.position.offset(x: 160, y: 50)
            let end = toNode.position.offset(x: 0, y: 50)
            
            Path { path in
                path.move(to: start)
                let control1 = CGPoint(x: start.x + abs(end.x - start.x) * 0.5, y: start.y)
                let control2 = CGPoint(x: end.x - abs(end.x - start.x) * 0.5, y: end.y)
                path.addCurve(to: end, control1: control1, control2: control2)
            }
            .stroke(link.type == .exec ? Color.white : pinColor(for: link.type), lineWidth: link.type == .exec ? 4 : 2)
            .shadow(color: .black.opacity(0.3), radius: 2)
        }
    }
    
    private func pinColor(for type: BlueprintPinType) -> Color {
        switch type {
        case .bool: return .red
        case .float: return .cyan
        case .int: return .green
        case .string: return .pink
        case .vector: return .yellow
        case .entity: return .blue
        default: return .white
        }
    }
}

func pinColor(for type: BlueprintPinType) -> Color {
    switch type {
    case .bool: return .red
    case .float: return .cyan
    case .int: return .green
    case .string: return .pink
    case .vector: return .yellow
    case .entity: return .blue
    default: return .white
    }
}

struct Polygon: Shape {
    let sides: Int
    func path(in rect: CGRect) -> Path {
        // Pentagonal shape for exec pins
        var path = Path()
        let center = CGPoint(x: rect.midX, y: rect.midY)
        let radius = min(rect.width, rect.height) / 2
        let angle = (2 * .pi) / CGFloat(sides)
        for i in 0..<sides {
            let x = center.x + radius * cos(CGFloat(i) * angle - .pi/2)
            let y = center.y + radius * sin(CGFloat(i) * angle - .pi/2)
            if i == 0 { path.move(to: CGPoint(x: x, y: y)) }
            else { path.addLine(to: CGPoint(x: x, y: y)) }
        }
        path.closeSubpath()
        return path
    }
}
