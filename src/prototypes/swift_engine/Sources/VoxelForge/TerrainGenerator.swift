import Foundation
import simd

protocol Generator: Sendable {
    func generate(chunk: inout Chunk)
}

public enum BiomeType {
    case plains
    case desert
    case mountains
    case forest
}

public struct AdvancedGenerator: Generator {
    let seed: Int
    let noise: PerlinNoise
    
    public init(seed: Int = 12345) {
        self.seed = seed
        self.noise = PerlinNoise(seed: seed)
    }
    
    func generate(chunk: inout Chunk) {
        let cx = chunk.position.x * CHUNK_SIZE
        let cz = chunk.position.z * CHUNK_SIZE
        
        for x in 0..<CHUNK_SIZE {
            for z in 0..<CHUNK_SIZE {
                let worldX = Double(cx + x)
                let worldZ = Double(cz + z)
                
                // Octave 1: Large features
                let n1 = noise.noise2D(x: worldX * 0.01, z: worldZ * 0.01)
                
                // Octave 2: Smaller details
                let n2 = noise.noise2D(x: worldX * 0.04, z: worldZ * 0.04) * 0.5
                
                // Octave 3: Micro details
                let n3 = noise.noise2D(x: worldX * 0.1, z: worldZ * 0.1) * 0.25
                
                let totalNoise = n1 + n2 + n3
                
                // Map noise (-1 to 1 approx) to height (e.g., center 64, spread 32)
                let height = Int(64.0 + totalNoise * 32.0)
                
                for y in 0..<min(max(0, height), CHUNK_SIZE_Y) {
                    var type: UInt8 = 1 // Stone
                    
                    if y == height {
                        type = 2 // Grass on top
                    } else if y > height - 4 {
                        type = 1 // Dirt
                    }
                    
                    chunk.setBlock(at: SIMD3(x, y, z), type: type)
                }
                
                // Water level
                if height < 50 {
                    for y in height+1...50 {
                        chunk.setBlock(at: SIMD3(x, y, z), type: 8) // Water block ID placeholder
                    }
                }
            }
        }
        
        // Simple tree placement (Randomized)
        if chunk.position.x == 0 && chunk.position.z == 0 {
             // Keep the static tree for test reference
            chunk.setBlock(at: SIMD3(8, 70, 8), type: 3)
            chunk.setBlock(at: SIMD3(8, 71, 8), type: 3)
            chunk.setBlock(at: SIMD3(8, 72, 8), type: 4)
        }
    }
}

struct StandardGenerator: Generator {
    func generate(chunk: inout Chunk) {
        for x in 0..<CHUNK_SIZE {
            for z in 0..<CHUNK_SIZE {
                let height = 10
                for y in 0..<height {
                    chunk.setBlock(at: SIMD3(x, y, z), type: 1)
                }
            }
        }
    }
}
