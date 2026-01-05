import Foundation

public protocol NodeValue: Sendable {}
extension Float: NodeValue {}
extension Int: NodeValue {}
extension Bool: NodeValue {}

public struct NodeHandle: Hashable, Codable, Sendable {
    let id: UUID
}

public struct VoxelNode: Codable, Sendable {
    public let handle: NodeHandle
    public let type: String
    public var inputs: [String: NodeHandle] = [:] // InputName -> SourceOutputHandle
    public var values: [String: Float] = [:] // Constant values
}

public struct VoxelGraph: Codable, Sendable {
    public var nodes: [NodeHandle: VoxelNode] = [:]
    
    public init() {}
    
    public mutating func addNode(type: String) -> NodeHandle {
        let handle = NodeHandle(id: UUID())
        let node = VoxelNode(handle: handle, type: type)
        nodes[handle] = node
        return handle
    }
    
    public mutating func connect(from source: NodeHandle, to destination: NodeHandle, inputName: String) {
        nodes[destination]?.inputs[inputName] = source
    }
}

@MainActor
public final class GraphEvaluator {
    public static let shared = GraphEvaluator()
    
    private init() {}
    
    public func evaluate(graph: VoxelGraph) -> [NodeHandle: Float] {
        let results: [NodeHandle: Float] = [:]
        _ = graph
        // This is a placeholder for the evaluation logic
        return results
    }
}
