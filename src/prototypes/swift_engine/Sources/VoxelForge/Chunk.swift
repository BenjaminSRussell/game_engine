import Foundation
import simd

// Chunk Constants
let CHUNK_SIZE: Int = 16
let CHUNK_SIZE_Y: Int = 256 // Vertical height limit like MC
let CHUNK_VOLUME: Int = CHUNK_SIZE * CHUNK_SIZE_Y * CHUNK_SIZE

public struct Block: Codable, Sendable {
    public var type: UInt8 // 0 = air, 1 = dirt, etc.
    public var active: Bool { return type != 0 }
}

public struct ChunkPosition: Hashable, Codable, Sendable {
    public let x: Int
    public let z: Int
    
    public init(x: Int, z: Int) {
        self.x = x
        self.z = z
    }
}

public struct Chunk: Sendable, Codable {
    public let position: ChunkPosition
    public var blocks: [Block]
    public var isDirty: Bool = true
    
    init(position: ChunkPosition) {
        self.position = position
        self.blocks = Array(repeating: Block(type: 0), count: CHUNK_VOLUME)
    }
    
    func getBlock(at localPosition: SIMD3<Int>) -> Block {
        if localPosition.x < 0 || localPosition.x >= CHUNK_SIZE ||
           localPosition.y < 0 || localPosition.y >= CHUNK_SIZE_Y ||
           localPosition.z < 0 || localPosition.z >= CHUNK_SIZE {
            return Block(type: 0)
        }
        let index = localPosition.x + (localPosition.z * CHUNK_SIZE) + (localPosition.y * CHUNK_SIZE * CHUNK_SIZE)
        return blocks[index]
    }
    
    mutating func setBlock(at localPosition: SIMD3<Int>, type: UInt8) {
        if localPosition.x < 0 || localPosition.x >= CHUNK_SIZE ||
           localPosition.y < 0 || localPosition.y >= CHUNK_SIZE_Y ||
           localPosition.z < 0 || localPosition.z >= CHUNK_SIZE {
            return
        }
        let index = localPosition.x + (localPosition.z * CHUNK_SIZE) + (localPosition.y * CHUNK_SIZE * CHUNK_SIZE)
        blocks[index].type = type
        isDirty = true
    }
}
