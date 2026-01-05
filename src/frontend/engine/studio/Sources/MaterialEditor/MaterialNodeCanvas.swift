import SwiftUI

struct MaterialNodeCanvas: View {
    @ObservedObject var viewModel: MaterialGraphViewModel
    @State private var isDraggingCanvas = false
    @State private var dragStart: CGPoint = .zero
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Grid background
                CanvasGridView(offset: viewModel.canvasOffset, zoom: viewModel.canvasZoom)
                    .background(DesignSystem.Colors.backgroundPrimary)
                
                // Links layer
                ForEach(viewModel.graph.links) { link in
                    LinkView(
                        viewModel: viewModel,
                        link: link
                    )
                }
                
                // Temporary drag link
                if let draggedSocket = viewModel.draggedSocket,
                   let dragPosition = viewModel.dragPosition {
                    let startPos = viewModel.worldToScreen(
                        getSocketPosition(node: draggedSocket.node, socket: draggedSocket.socket)
                    )
                    
                    LinkCurve(
                        from: startPos,
                        to: dragPosition,
                        color: draggedSocket.socket.dataType.color
                    )
                }
                
                // Nodes layer
                ForEach(viewModel.graph.nodes) { node in
                    MaterialNodeView(
                        viewModel: viewModel,
                        node: node,
                        isSelected: viewModel.selectedNodeID == node.id
                    )
                }
            }
            .gesture(
                DragGesture()
                    .onChanged { value in
                        if !isDraggingCanvas {
                            isDraggingCanvas = true
                            dragStart = value.location
                        }
                        let delta = CGSize(
                            width: value.location.x - dragStart.x,
                            height: value.location.y - dragStart.y
                        )
                        viewModel.panCanvas(by: delta)
                        dragStart = value.location
                    }
                    .onEnded { _ in
                        isDraggingCanvas = false
                    }
            )
            .gesture(
                MagnificationGesture()
                    .onChanged { scale in
                        viewModel.zoomCanvas(by: scale, at: CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2))
                    }
            )
            .onTapGesture {
                viewModel.selectedNodeID = nil
            }
        }
    }
    
    private func getSocketPosition(node: MaterialGraphNode, socket: GraphSocket) -> CGPoint {
        let nodePos = node.position
        let socketIndex: Int
        if socket.isInput {
            socketIndex = node.inputs.firstIndex(where: { $0.id == socket.id }) ?? 0
        } else {
            socketIndex = node.outputs.firstIndex(where: { $0.id == socket.id }) ?? 0
        }
        
        return CGPoint(
            x: nodePos.x + (socket.isInput ? 0 : 200),
            y: nodePos.y + 40 + CGFloat(socketIndex) * 24
        )
    }
}

// MARK: - Grid Background

struct CanvasGridView: View {
    let offset: CGSize
    let zoom: CGFloat
    
    var body: some View {
        Canvas { context, size in
            let gridSize: CGFloat = 20 * zoom
            let startX = -offset.width.truncatingRemainder(dividingBy: gridSize)
            let startY = -offset.height.truncatingRemainder(dividingBy: gridSize)
            
            var path = Path()
            
            // Vertical lines
            var x = startX
            while x < size.width {
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
                x += gridSize
            }
            
            // Horizontal lines
            var y = startY
            while y < size.height {
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
                y += gridSize
            }
            
            context.stroke(path, with: .color(Color.gray.opacity(0.2)), lineWidth: 1)
        }
    }
}

// MARK: - Node View

struct MaterialNodeView: View {
    @ObservedObject var viewModel: MaterialGraphViewModel
    @ObservedObject var node: MaterialGraphNode
    let isSelected: Bool
    
    @State private var isDragging = false
    @State private var dragOffset: CGSize = .zero
    
    var body: some View {
        let screenPos = viewModel.worldToScreen(node.position)
        
        VStack(alignment: .leading, spacing: 0) {
            // Header
            HStack {
                Text(node.title)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                Spacer()
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(node.headerColor)
            
            // Body
            VStack(alignment: .leading, spacing: 4) {
                // Inputs
                ForEach(node.inputs) { socket in
                    SocketRowView(
                        viewModel: viewModel,
                        node: node,
                        socket: socket
                    )
                }
                
                // Outputs
                ForEach(node.outputs) { socket in
                    SocketRowView(
                        viewModel: viewModel,
                        node: node,
                        socket: socket
                    )
                }
            }
            .padding(8)
        }
        .frame(width: 200 * viewModel.canvasZoom)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(6)
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear, lineWidth: 2)
        )
        .shadow(radius: 4)
        .scaleEffect(viewModel.canvasZoom)
        .position(x: screenPos.x + 100 * viewModel.canvasZoom, y: screenPos.y)
        .gesture(
            DragGesture()
                .onChanged { value in
                    if !isDragging {
                        isDragging = true
                        viewModel.selectedNodeID = node.id
                    }
                    let delta = CGSize(
                        width: value.translation.width - dragOffset.width,
                        height: value.translation.height - dragOffset.height
                    )
                    viewModel.moveNode(node.id, by: delta)
                    dragOffset = value.translation
                }
                .onEnded { _ in
                    isDragging = false
                    dragOffset = .zero
                }
        )
    }
}

// MARK: - Socket Row

struct SocketRowView: View {
    @ObservedObject var viewModel: MaterialGraphViewModel
    let node: MaterialGraphNode
    @ObservedObject var socket: GraphSocket
    
    var body: some View {
        HStack(spacing: 4) {
            if socket.isInput {
                SocketPortView(socket: socket, viewModel: viewModel, node: node)
                Text(socket.name)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
                
                // Value input if not connected
                if viewModel.graph.getConnectedSocket(to: socket.id) == nil {
                    SocketValueInput(socket: socket)
                }
            } else {
                Spacer()
                Text(socket.name)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                SocketPortView(socket: socket, viewModel: viewModel, node: node)
            }
        }
    }
}

// MARK: - Socket Port

struct SocketPortView: View {
    @ObservedObject var socket: GraphSocket
    let viewModel: MaterialGraphViewModel
    let node: MaterialGraphNode
    
    var body: some View {
        Circle()
            .fill(socket.dataType.color)
            .frame(width: 12, height: 12)
            .overlay(
                Circle()
                    .stroke(Color.white, lineWidth: 2)
            )
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { value in
                        viewModel.startDraggingSocket(node: node, socket: socket)
                        viewModel.updateDragPosition(value.location)
                    }
                    .onEnded { value in
                        // Find target socket at end position
                        viewModel.endDraggingSocket(on: nil, targetSocket: nil)
                    }
            )
    }
}

// MARK: - Socket Value Input

struct SocketValueInput: View {
    @ObservedObject var socket: GraphSocket
    
    var body: some View {
        Group {
            switch socket.dataType {
            case .float:
                if case .float(let value) = socket.value {
                    TextField("", value: Binding(
                        get: { value },
                        set: { socket.value = .float($0) }
                    ), format: .number)
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 50)
                }
                
            case .color:
                if case .color(let r, let g, let b, let a) = socket.value {
                    ColorPicker("", selection: Binding(
                        get: { Color(red: Double(r), green: Double(g), blue: Double(b), opacity: Double(a)) },
                        set: { color in
                            let components = color.cgColor?.components ?? [0, 0, 0, 1]
                            socket.value = .color(
                                red: Float(components[0]),
                                green: Float(components[1]),
                                blue: Float(components[2]),
                                alpha: Float(components[3])
                            )
                        }
                    ))
                    .labelsHidden()
                    .frame(width: 30, height: 20)
                }
                
            default:
                EmptyView()
            }
        }
    }
}

// MARK: - Link View

struct LinkView: View {
    let viewModel: MaterialGraphViewModel
    let link: GraphLink
    
    var body: some View {
        if let fromNode = viewModel.graph.findNode(link.fromNodeID),
           let toNode = viewModel.graph.findNode(link.toNodeID),
           let fromSocket = fromNode.outputs.first(where: { $0.id == link.fromSocketID }),
           let toSocket = toNode.inputs.first(where: { $0.id == link.toSocketID }) {
            
            let fromPos = viewModel.worldToScreen(getSocketPosition(node: fromNode, socket: fromSocket))
            let toPos = viewModel.worldToScreen(getSocketPosition(node: toNode, socket: toSocket))
            
            LinkCurve(from: fromPos, to: toPos, color: fromSocket.dataType.color)
                .onTapGesture {
                    viewModel.deleteLink(link.id)
                }
        }
    }
    
    private func getSocketPosition(node: MaterialGraphNode, socket: GraphSocket) -> CGPoint {
        let nodePos = node.position
        let socketIndex: Int
        if socket.isInput {
            socketIndex = node.inputs.firstIndex(where: { $0.id == socket.id }) ?? 0
        } else {
            socketIndex = node.outputs.firstIndex(where: { $0.id == socket.id }) ?? 0
        }
        
        return CGPoint(
            x: nodePos.x + (socket.isInput ? 0 : 200),
            y: nodePos.y + 40 + CGFloat(socketIndex) * 24
        )
    }
}

// MARK: - Link Curve

struct LinkCurve: View {
    let from: CGPoint
    let to: CGPoint
    let color: Color
    
    var body: some View {
        Path { path in
            path.move(to: from)
            
            let controlDistance = abs(to.x - from.x) * 0.5
            let control1 = CGPoint(x: from.x + controlDistance, y: from.y)
            let control2 = CGPoint(x: to.x - controlDistance, y: to.y)
            
            path.addCurve(to: to, control1: control1, control2: control2)
        }
        .stroke(color, lineWidth: 3)
    }
}
