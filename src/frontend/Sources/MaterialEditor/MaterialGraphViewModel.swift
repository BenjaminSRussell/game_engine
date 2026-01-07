import Foundation
import SwiftUI
import Combine

class MaterialGraphViewModel: ObservableObject {
    @Published var graph = MaterialGraph()
    @Published var selectedNodeID: UUID?
    @Published var canvasOffset: CGSize = .zero
    @Published var canvasZoom: CGFloat = 1.0
    
    // Drag state for creating connections
    @Published var draggedSocket: (node: MaterialGraphNode, socket: GraphSocket)?
    @Published var dragPosition: CGPoint?
    
    // MARK: - Node Operations
    
    func addNode(type: MaterialNodeType, at position: CGPoint) {
        let worldPosition = screenToWorld(position)
        let node = MaterialGraphNode(position: worldPosition, nodeType: type)
        graph.addNode(node)
        selectedNodeID = node.id
    }
    
    func deleteSelectedNode() {
        guard let id = selectedNodeID else { return }
        graph.removeNode(id)
        selectedNodeID = nil
    }
    
    func moveNode(_ nodeID: UUID, by delta: CGSize) {
        guard var node = graph.findNode(nodeID) else { return }
        node.position.x += delta.width / canvasZoom
        node.position.y += delta.height / canvasZoom
        objectWillChange.send()
    }
    
    // MARK: - Link Operations
    
    func startDraggingSocket(node: MaterialGraphNode, socket: GraphSocket) {
        draggedSocket = (node, socket)
    }
    
    func updateDragPosition(_ position: CGPoint) {
        dragPosition = position
    }
    
    func endDraggingSocket(on targetNode: MaterialGraphNode?, targetSocket: GraphSocket?) {
        defer {
            draggedSocket = nil
            dragPosition = nil
        }
        
        guard let source = draggedSocket,
              let targetNode = targetNode,
              let targetSocket = targetSocket else {
            return
        }
        
        // Validate connection
        if !canConnect(from: source.socket, to: targetSocket) {
            return
        }
        
        // Create link (from output to input)
        let (fromNode, fromSocket, toNode, toSocket): (MaterialGraphNode, GraphSocket, MaterialGraphNode, GraphSocket)
        
        if source.socket.isInput {
            // Dragged from input, target must be output
            fromNode = targetNode
            fromSocket = targetSocket
            toNode = source.node
            toSocket = source.socket
        } else {
            // Dragged from output, target must be input
            fromNode = source.node
            fromSocket = source.socket
            toNode = targetNode
            toSocket = targetSocket
        }
        
        let link = GraphLink(
            fromNodeID: fromNode.id,
            fromSocketID: fromSocket.id,
            toNodeID: toNode.id,
            toSocketID: toSocket.id
        )
        
        graph.addLink(link)
    }
    
    func deleteLink(_ linkID: UUID) {
        graph.removeLink(linkID)
    }
    
    private func canConnect(from: GraphSocket, to: GraphSocket) -> Bool {
        // Can't connect two inputs or two outputs
        if from.isInput == to.isInput {
            return false
        }
        
        // Type validation (simple version - can be expanded)
        return from.dataType == to.dataType || 
               from.dataType == .shader || 
               to.dataType == .shader
    }
    
    // MARK: - Coordinate Conversion
    
    func screenToWorld(_ point: CGPoint) -> CGPoint {
        return CGPoint(
            x: (point.x - canvasOffset.width) / canvasZoom,
            y: (point.y - canvasOffset.height) / canvasZoom
        )
    }
    
    func worldToScreen(_ point: CGPoint) -> CGPoint {
        return CGPoint(
            x: point.x * canvasZoom + canvasOffset.width,
            y: point.y * canvasZoom + canvasOffset.height
        )
    }
    
    // MARK: - Canvas Controls
    
    func panCanvas(by delta: CGSize) {
        canvasOffset.width += delta.width
        canvasOffset.height += delta.height
    }
    
    func zoomCanvas(by factor: CGFloat, at point: CGPoint) {
        let oldZoom = canvasZoom
        canvasZoom = max(0.25, min(2.0, canvasZoom * factor))
        
        // Adjust offset to zoom towards cursor
        let zoomRatio = canvasZoom / oldZoom
        canvasOffset.width = point.x - (point.x - canvasOffset.width) * zoomRatio
        canvasOffset.height = point.y - (point.y - canvasOffset.height) * zoomRatio
    }
    
    func resetView() {
        canvasOffset = .zero
        canvasZoom = 1.0
    }
    
    // MARK: - Material Export
    
    func exportMaterial() -> PBRMaterialData? {
        // Find the Material Output node
        guard let outputNode = graph.nodes.first(where: { $0.nodeType == .materialOutput }),
              let surfaceInput = outputNode.inputs.first(where: { $0.name == "Surface" }),
              let (bsdfNode, _) = graph.getConnectedSocket(to: surfaceInput.id),
              case .principledBSDF = bsdfNode.nodeType else {
            return nil
        }
        
        // Extract values from Principled BSDF
        var material = PBRMaterialData()
        
        for input in bsdfNode.inputs {
            switch input.name {
            case "Base Color":
                if let (r, g, b, a) = input.value?.asColor {
                    material.baseColor = (r, g, b, a)
                }
            case "Metallic":
                material.metallic = input.value?.asFloat ?? 0.0
            case "Roughness":
                material.roughness = input.value?.asFloat ?? 0.5
            case "Alpha":
                material.alpha = input.value?.asFloat ?? 1.0
            default:
                break
            }
        }
        
        return material
    }
}

// MARK: - Material Data

struct PBRMaterialData {
    var baseColor: (Float, Float, Float, Float) = (0.8, 0.8, 0.8, 1.0)
    var metallic: Float = 0.0
    var roughness: Float = 0.5
    var alpha: Float = 1.0
}
