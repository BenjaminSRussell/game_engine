import SwiftUI

struct MaterialEditorView: View {
    @StateObject private var graph = MaterialGraph()
    @State private var dragOffset: CGSize = .zero
    @State private var zoom: CGFloat = 1.0
    
    var body: some View {
        HStack(spacing: 0) {
            // Left: Node Library
            VStack {
                Text("NODE LIBRARY")
                    .font(.caption.bold())
                    .padding()
                
                List(MaterialNodeType.allCases) { type in
                    Text(type.rawValue)
                        .font(.subheadline)
                        .onDrag { NSItemProvider(object: type.rawValue as NSString) }
                }
                .listStyle(PlainListStyle())
            }
            .frame(width: 180)
            .background(Color(white: 0.12))
            
            // Center: Node Graph Canvas
            ZStack {
                // Background Grid
                CanvasBackgroundGrid()
                
                // Connection Links
                ForEach(graph.links) { link in
                    LinkView(link: link, graph: graph)
                }
                
                // Nodes
                ForEach(graph.nodes) { node in
                    NodeView(node: node)
                        .position(x: node.position.x + dragOffset.width, y: node.position.y + dragOffset.height)
                        .gesture(
                            DragGesture()
                                .onChanged { value in
                                    node.position = CGPoint(x: node.position.x + value.translation.width, y: node.position.y + value.translation.height)
                                }
                        )
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Color.black)
            .clipped()
            .dropDestination(for: String.self) { items, location in
                if let typeStr = items.first, let type = MaterialNodeType(rawValue: typeStr) {
                    graph.addNode(type: type, at: location)
                    return true
                }
                return false
            }
            
            // Right: Material Preview & Properties
            VStack {
                SectionHeader(title: "PREVIEW")
                MaterialPreviewView(graph: graph)
                    .frame(height: 200)
                    .background(Color.black)
                    .cornerRadius(8)
                    .padding()
                
                SectionHeader(title: "PROPERTIES")
                // Properties Inspector placeholder
                Spacer()
            }
            .frame(width: 250)
            .background(Color(white: 0.12))
        }
    }
}

struct NodeView: View {
    @ObservedObject var node: MaterialNode
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header
            Text(node.type.rawValue.uppercased())
                .font(.caption2.bold())
                .padding(8)
                .frame(maxWidth: .infinity, alignment: .leading)
                .background(Color.cyan.opacity(0.3))
            
            HStack(alignment: .top) {
                // Inputs
                VStack(alignment: .leading, spacing: 8) {
                    ForEach(node.inputs) { socket in
                        SocketView(socket: socket, isInput: true)
                    }
                }
                
                Spacer()
                
                // Outputs
                VStack(alignment: .trailing, spacing: 8) {
                    ForEach(node.outputs) { socket in
                        SocketView(socket: socket, isInput: false)
                    }
                }
            }
            .padding(8)
        }
        .frame(width: 140)
        .background(Color(white: 0.18))
        .cornerRadius(6)
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(Color.white.opacity(0.1), lineWidth: 1)
        )
    }
}

struct SocketView: View {
    let socket: MaterialSocket
    let isInput: Bool
    
    var body: some View {
        HStack(spacing: 4) {
            if isInput {
                Circle()
                    .fill(Color.gray)
                    .frame(width: 8, height: 8)
            }
            Text(socket.name)
                .font(.system(size: 10))
                .foregroundColor(.white.opacity(0.8))
            if !isInput {
                Circle()
                    .fill(Color.gray)
                    .frame(width: 8, height: 8)
            }
        }
    }
}

struct LinkView: View {
    let link: MaterialLink
    @ObservedObject var graph: MaterialGraph
    
    var body: some View {
        if let fromNode = graph.nodes.first(where: { $0.id == link.fromNode }),
           let toNode = graph.nodes.first(where: { $0.id == link.toNode }),
           let fromSocket = fromNode.outputs.first(where: { $0.id == link.fromSocket }),
           let toSocket = toNode.inputs.first(where: { $0.id == link.toSocket }) {
            
            let start = fromNode.position.offset(x: 140, y: 40) // Placeholder offset
            let end = toNode.position.offset(x: 0, y: 40)
            
            Path { path in
                path.move(to: start)
                let control1 = CGPoint(x: start.x + abs(end.x - start.x) * 0.5, y: start.y)
                let control2 = CGPoint(x: end.x - abs(end.x - start.x) * 0.5, y: end.y)
                path.addCurve(to: end, control1: control1, control2: control2)
            }
            .stroke(Color.cyan.opacity(0.6), lineWidth: 3)
        }
    }
}

extension CGPoint {
    func offset(x: CGFloat, y: CGFloat) -> CGPoint {
        CGPoint(x: self.x + x, y: self.y + y)
    }
}

struct CanvasBackgroundGrid: View {
    var body: some View {
        GeometryReader { geo in
            Path { path in
                for x in stride(from: 0, to: geo.size.width, by: 50) {
                    path.move(to: CGPoint(x: x, y: 0))
                    path.addLine(to: CGPoint(x: x, y: geo.size.height))
                }
                for y in stride(from: 0, to: geo.size.height, by: 50) {
                    path.move(to: CGPoint(x: 0, y: y))
                    path.addLine(to: CGPoint(x: geo.size.width, y: y))
                }
            }
            .stroke(Color.white.opacity(0.05), lineWidth: 1)
        }
    }
}

struct SectionHeader: View {
    let title: String
    var body: some View {
        Text(title)
            .font(.caption2.bold())
            .foregroundColor(.gray)
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(.horizontal)
            .padding(.top, 10)
    }
}

struct MaterialPreviewView: UIViewRepresentable {
    @ObservedObject var graph: MaterialGraph
    
    func makeUIView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 1)
        mtkView.depthStencilPixelFormat = .depth32Float
        
        let renderer = MaterialRenderer(device: mtkView.device!, graph: graph)
        mtkView.delegate = renderer
        
        return mtkView
    }
    
    func updateUIView(_ uiView: MTKView, context: Context) {}
}
