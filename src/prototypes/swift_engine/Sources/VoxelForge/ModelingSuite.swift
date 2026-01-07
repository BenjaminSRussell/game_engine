import Foundation
import simd

public struct VoxelModel: Codable, Sendable {
    public var voxels: [SIMD3<Int32>: UInt8]
    public var size: SIMD3<Int32>
    
    public init(size: SIMD3<Int32>) {
        self.size = size
        self.voxels = [:]
    }
}

public protocol ModelingTool: Sendable {
    func apply(to model: inout VoxelModel, at position: SIMD3<Int32>)
}

public struct PencilTool: ModelingTool {
    public var type: UInt8
    public func apply(to model: inout VoxelModel, at position: SIMD3<Int32>) {
        model.voxels[position] = type
    }
}

@MainActor
public final class ModelingSystem {
    public static let shared = ModelingSystem()
    
    private init() {}
    
    public func saveModel(_ model: VoxelModel, to path: String) throws {
        let encoder = JSONEncoder()
        let data = try encoder.encode(model)
        try data.write(to: URL(fileURLWithPath: path))
    }
    
    public func loadModel(from path: String) throws -> VoxelModel {
        let data = try Data(contentsOf: URL(fileURLWithPath: path))
        let decoder = JSONDecoder()
        return try decoder.decode(VoxelModel.self, from: data)
    }
}

// SIMD3<Int32> needs to be Codable/Hashable

// Voxel modeling logic continues...
