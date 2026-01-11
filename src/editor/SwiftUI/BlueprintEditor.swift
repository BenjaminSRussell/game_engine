// SwiftUI/BlueprintEditor.swift
// Visual scripting / Blueprint editor (node-based)
import SwiftUI

struct BlueprintEditorView: View {
    @State private var nodes: [BlueprintNode] = []
    @State private var connections: [NodeConnection] = []
    @State private var offset: CGSize = .zero
    @State private var zoom: CGFloat = 1.0
    
    var body: some View {
        ZStack {
            // Grid background
            BlueprintGrid(zoom: zoom)
            
            // Canvas
            GeometryReader { geometry in
                ZStack {
                    // Connections
                    ForEach(connections) { connection in
                        NodeConnectionView(connection: connection, nodes: nodes)
                    }
                    
                    // Nodes
                    ForEach($nodes) { $node in
                        NodeView(node: $node)
                            .offset(x: node.position.x * zoom + offset.width,
                                   y: node.position.y * zoom + offset.height)
                            .scaleEffect(zoom)
                    }
                }
            }
            .gesture(
                DragGesture()
                    .onChanged { value in
                        offset = value.translation
                    }
            )
            
            // Toolbar
            VStack {
                HStack {
                    Button(action: addNode) {
                        Label("Add Node", systemImage: "plus.app.fill")
                    }
                    Button(action: { zoom *= 1.2 }) {
                        Image(systemName: "plus.magnifyingglass")
                    }
                    Button(action: { zoom /= 1.2 }) {
                        Image(systemName: "minus.magnifyingglass")
                    }
                    Spacer()
                }
                .padding()
                .background(.ultraThinMaterial)
                Spacer()
            }
        }
    }
    
    func addNode() {
        nodes.append(BlueprintNode(
            id: UUID(),
            type: "Event",
            position: CGPoint(x: 100, y: 100)
        ))
    }
}

struct BlueprintGrid: View {
    let zoom: CGFloat
    
    var body: some View {
        GeometryReader { geometry in
            Path { path in
                let spacing: CGFloat = 20 * zoom
                let width = geometry.size.width
                let height = geometry.size.height
                
                // Vertical lines
                for x in stride(from: 0, through: width, by: spacing) {
                    path.move(to: CGPoint(x: x, y: 0))
                    path.addLine(to: CGPoint(x: x, y: height))
                }
                
                // Horizontal lines
                for y in stride(from: 0, through: height, by: spacing) {
                    path.move(to: CGPoint(x: 0, y: y))
                    path.addLine(to: CGPoint(x: width, y: y))
                }
            }
            .stroke(Color.gray.opacity(0.2), lineWidth: 0.5)
        }
        .background(Color(nsColor: .controlBackgroundColor))
    }
}

struct NodeView: View {
    @Binding var node: BlueprintNode
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(node.type)
                .font(.headline)
            
            // Input pins
            ForEach(0..<2) { i in
                HStack {
                    Circle()
                        .fill(Color.blue)
                        .frame(width: 12, height: 12)
                    Text("Input \\(i)")
                        .font(.caption)
                }
            }
            
            Divider()
            
            // Output pins
            ForEach(0..<2) { i in
                HStack {
                    Spacer()
                    Text("Output \\(i)")
                        .font(.caption)
                    Circle()
                        .fill(Color.green)
                        .frame(width: 12, height: 12)
                }
            }
        }
        .padding()
        .frame(width: 200)
        .background(Color(nsColor: .windowBackgroundColor))
        .cornerRadius(8)
        .shadow(radius: 4)
    }
}

struct NodeConnectionView: View {
    let connection: NodeConnection
    let nodes: [BlueprintNode]
    
    var body: some View {
        Path { path in
            // Draw Bezier curve between pins
            // Simplified for now
            path.move(to: CGPoint(x: 50, y: 50))
            path.addLine(to: CGPoint(x: 200, y: 200))
        }
        .stroke(Color.white, lineWidth: 2)
    }
}

struct BlueprintNode: Identifiable {
    let id: UUID
    var type: String
    var position: CGPoint
}

struct NodeConnection: Identifiable {
    let id = UUID()
    var from: UUID
    var to: UUID
}
