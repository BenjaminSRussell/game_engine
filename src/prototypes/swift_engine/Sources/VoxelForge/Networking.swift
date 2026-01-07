import Foundation

public struct VoxelDelta: Codable, Sendable {
    public let position: SIMD3<Int32>
    public let type: UInt8
}

public struct ChunkDelta: Codable, Sendable {
    public let position: ChunkPosition
    public let deltas: [VoxelDelta]
}

@MainActor
public final class DeltaCompression {
    public static let shared = DeltaCompression()
    
    private init() {}
    
    /// Compresses chunk data by identifying only the changes (deltas).
    public func computeDeltas(original: Chunk, current: Chunk) -> ChunkDelta {
        var deltas: [VoxelDelta] = []
        
        for i in 0..<CHUNK_VOLUME {
            if original.blocks[i].type != current.blocks[i].type {
                let x = Int32(i % CHUNK_SIZE)
                let z = Int32((i / CHUNK_SIZE) % CHUNK_SIZE)
                let y = Int32(i / (CHUNK_SIZE * CHUNK_SIZE))
                deltas.append(VoxelDelta(position: SIMD3(x, y, z), type: current.blocks[i].type))
            }
        }
        
        return ChunkDelta(position: current.position, deltas: deltas)
    }
    
    /// Applies deltas to a chunk.
    public func applyDeltas(_ delta: ChunkDelta, to chunk: inout Chunk) {
        for d in delta.deltas {
            chunk.setBlock(at: SIMD3(Int(d.position.x), Int(d.position.y), Int(d.position.z)), type: d.type)
        }
    }
}
