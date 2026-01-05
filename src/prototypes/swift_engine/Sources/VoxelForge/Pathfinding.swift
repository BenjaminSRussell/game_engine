import Foundation
import simd

public struct PathNode: Hashable, Sendable {
    public let position: SIMD3<Int>
    public var gScore: Float = .infinity
    public var hScore: Float = 0
    public var fScore: Float { gScore + hScore }
}

public protocol PathfindingProvider: Actor {
    func findPath(from start: SIMD3<Int>, to end: SIMD3<Int>, in world: World) async -> [SIMD3<Int>]
}

public actor AStarPathfinder: PathfindingProvider {
    public init() {}
    
    public func findPath(from start: SIMD3<Int>, to end: SIMD3<Int>, in world: World) async -> [SIMD3<Int>] {
        var openSet: Set<SIMD3<Int>> = [start]
        var cameFrom: [SIMD3<Int>: SIMD3<Int>] = [:]
        var gScore: [SIMD3<Int>: Float] = [start: 0]
        var fScore: [SIMD3<Int>: Float] = [start: heuristic(start, end)]
        
        while !openSet.isEmpty {
            // Get node with lowest fScore
            let current = openSet.min { (fScore[$0] ?? .infinity) < (fScore[$1] ?? .infinity) }!
            
            if current == end {
                return reconstructPath(cameFrom: cameFrom, current: current)
            }
            
            openSet.remove(current)
            
            // Check 6 neighbors + diagonals could be added later
            let neighbors = [
                current &+ SIMD3(1, 0, 0), current &+ SIMD3(-1, 0, 0),
                current &+ SIMD3(0, 1, 0), current &+ SIMD3(0, -1, 0),
                current &+ SIMD3(0, 0, 1), current &+ SIMD3(0, 0, -1)
            ]
            
            for neighbor in neighbors {
                // 1. Is it passable? (Must be air)
                if await !isPassable(at: neighbor, world: world) { continue }
                
                // 2. Is there ground below? (Gravity check)
                // For flying entities, remove this. For walkers, keep it.
                // Simplified: We allow moving to a node if it is air. 
                // A better walker check would be isSolid(neighbor + down).
                // Let's enforce "Must support standing" unless it's a Jump (y+1) or Fall (y-1)
                // For this iteration, we simply check "IsPassable" to enable basic flying/ghost movement
                // which is safer for a generic "Pathfinder" without checking entity capabilities.
                
                let tentativeGScore = (gScore[current] ?? .infinity) + 1.0
                if tentativeGScore < (gScore[neighbor] ?? .infinity) {
                    cameFrom[neighbor] = current
                    gScore[neighbor] = tentativeGScore
                    fScore[neighbor] = tentativeGScore + heuristic(neighbor, end)
                    openSet.insert(neighbor)
                }
            }
        }
        
        return []
    }
    
    private func heuristic(_ a: SIMD3<Int>, _ b: SIMD3<Int>) -> Float {
        let dx = Float(abs(a.x - b.x))
        let dy = Float(abs(a.y - b.y))
        let dz = Float(abs(a.z - b.z))
        return dx + dy + dz
    }
    
    private func isPassable(at position: SIMD3<Int>, world: World) async -> Bool {
        // Convert to chunk coordinates
        let chunkSize = 16
        // Handle negative coordinates correctly for chunk calculation
        let cx = Int(floor(Float(position.x) / Float(chunkSize)))
        let cz = Int(floor(Float(position.z) / Float(chunkSize)))
        let chunkPos = ChunkPosition(x: cx, z: cz)
        
        guard let chunk = await world.getChunk(at: chunkPos) else { return false } // Unloaded chunks are impassable
        
        let lx = (position.x % chunkSize + chunkSize) % chunkSize
        let lz = (position.z % chunkSize + chunkSize) % chunkSize
        
        // Check bounds
        if position.y < 0 || position.y >= 256 { return false }
        
        let block = chunk.getBlock(at: SIMD3(lx, position.y, lz))
        return !block.active // Passable if block is NOT active (Air)
    }
    
    private func reconstructPath(cameFrom: [SIMD3<Int>: SIMD3<Int>], current: SIMD3<Int>) -> [SIMD3<Int>] {
        var path = [current]
        var curr = current
        while let prev = cameFrom[curr] {
            path.append(prev)
            curr = prev
        }
        return path.reversed()
    }
}
