// SwiftUI/MaterialEditor.swift
// Node-based material editor (PBR shader graph)
import SwiftUI

struct MaterialEditorView: View {
    @State private var nodes: [MaterialNode] = []
    @State private var selectedNode: UUID?
    @State private var previewMesh: PreviewMesh = .sphere
    
    enum PreviewMesh {
        case sphere, cube, plane, custom
    }
    
    var body: some View {
        HSplitView {
            // Node graph canvas
            MaterialGraphCanvas(nodes: $nodes, selected: $selectedNode)
            
            // Material preview
            VStack {
                Text("Preview")
                    .font(.headline)
                
                // 3D preview viewport
                MaterialPreviewView(nodes: nodes)
                    .frame(height: 300)
                
                Picker("Mesh", selection: $previewMesh) {
                    Text("Sphere").tag(PreviewMesh.sphere)
                    Text("Cube").tag(PreviewMesh.cube)
                    Text("Plane").tag(PreviewMesh.plane)
                }
                .pickerStyle(.segmented)
                
                // Material properties
                Form {
                    Section("Base") {
                        ColorPicker("Base Color", selection: .constant(.white))
                        Slider(value: .constant(0.5)) {
                            Text("Metallic")
                        }
                        Slider(value: .constant(0.5)) {
                            Text("Roughness")
                        }
                    }
                    
                    Section("Advanced") {
                        Toggle("Two Sided", isOn: .constant(false))
                        Toggle("Alpha Clip", isOn: .constant(false))
                    }
                }
                .formStyle(.grouped)
            }
            .frame(width: 300)
        }
        .toolbar {
            ToolbarItemGroup {
                Button(action: { addNode(.texture) }) {
                    Label("Texture", systemImage: "photo.fill")
                }
                Button(action: { addNode(.math) }) {
                    Label("Math", systemImage: "function")
                }
                Button(action: { addNode(.constant) }) {
                    Label("Constant", systemImage: "number")
                }
                Divider()
                Button("Compile") {
                    compileMaterial()
                }
            }
        }
    }
    
    func addNode(_ type: MaterialNodeType) {
        nodes.append(MaterialNode(type: type, position: CGPoint(x: 200, y: 200)))
    }
    
    func compileMaterial() {
        // Generate Metal shader code
        print("Compiling material...")
    }
}

struct MaterialGraphCanvas: View {
    @Binding var nodes: [MaterialNode]
    @Binding var selected: UUID?
    @State private var offset: CGSize = .zero
    
    var body: some View {
        ZStack {
            Color(nsColor: .controlBackgroundColor)
            
            // Grid
            Canvas { context, size in
                let spacing: CGFloat = 50
                context.stroke(
                    Path { path in
                        for x in stride(from: 0, through: size.width, by: spacing) {
                            path.move(to: CGPoint(x: x, y: 0))
                            path.addLine(to: CGPoint(x: x, y: size.height))
                        }
                        for y in stride(from: 0, through: size.height, by: spacing) {
                            path.move(to: CGPoint(x: 0, y: y))
                            path.addLine(to: CGPoint(x: size.width, y: y))
                        }
                    },
                    with: .color(.gray.opacity(0.3)),
                    lineWidth: 0.5
                )
            }
            
            // Nodes
            ForEach($nodes) { $node in
                MaterialNodeView(node: $node, isSelected: selected == node.id)
                    .offset(x: node.position.x + offset.width,
                           y: node.position.y + offset.height)
                    .onTapGesture {
                        selected = node.id
                    }
            }
        }
        .gesture(
            DragGesture()
                .onChanged { offset = $0.translation }
        )
    }
}

struct MaterialNodeView: View {
    @Binding var node: MaterialNode
    let isSelected: Bool
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(node.type.rawValue)
                .font(.caption).bold()
            
            // Input pins
            ForEach(node.inputs, id: \.self) { input in
                HStack(spacing: 4) {
                    Circle()
                        .fill(Color.white)
                        .frame(width: 8, height: 8)
                    Text(input)
                        .font(.caption2)
                }
            }
            
            Divider()
            
            // Output pins
            ForEach(node.outputs, id: \.self) { output in
                HStack(spacing: 4) {
                    Spacer()
                    Text(output)
                        .font(.caption2)
                    Circle()
                        .fill(Color.green)
                        .frame(width: 8, height: 8)
                }
            }
        }
        .padding(8)
        .frame(width: 150)
        .background(isSelected ? Color.blue.opacity(0.3) : Color(nsColor: .windowBackgroundColor))
        .cornerRadius(6)
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(isSelected ? Color.blue : Color.clear, lineWidth: 2)
        )
    }
}

struct MaterialPreviewView: NSViewRepresentable {
    let nodes: [MaterialNode]
    
    func makeNSView(context: Context) -> MTKView {
        let view = MTKView()
        view.device = MTLCreateSystemDefaultDevice()
        view.clearColor = MTLClearColor(red: 0.2, green: 0.2, blue: 0.2, alpha: 1.0)
        return view
    }
    
    func updateNSView(_ nsView: MTKView, context: Context) {
        // Update material preview
    }
}

enum MaterialNodeType: String {
    case texture = "Texture2D"
    case color = "Color"
    case constant = "Constant"
    case math = "Math"
    case lerp = "Lerp"
    case multiply = "Multiply"
    case add = "Add"
}

struct MaterialNode: Identifiable {
    let id = UUID()
    var type: MaterialNodeType
    var position: CGPoint
    
    var inputs: [String] {
        switch type {
        case .texture: return ["UV"]
        case .lerp: return ["A", "B", "Alpha"]
        case .multiply, .add: return ["A", "B"]
        default: return []
        }
    }
    
    var outputs: [String] {
        switch type {
        case .texture: return ["RGB", "A"]
        case .color: return ["RGB"]
        default: return ["Result"]
        }
    }
}
