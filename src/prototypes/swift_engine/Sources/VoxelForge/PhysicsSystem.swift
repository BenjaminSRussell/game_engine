import Foundation
import simd

public struct AABB: Component {
    public var min: SIMD3<Float>
    public var max: SIMD3<Float>
    
    public init(min: SIMD3<Float>, max: SIMD3<Float>) {
        self.min = min
        self.max = max
    }
    
    public func intersects(_ other: AABB) -> Bool {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z)
    }
}

public struct VelocityComponent: Component {
    public var value: SIMD3<Float>
    public init(value: SIMD3<Float> = .zero) {
        self.value = value
    }
}

@MainActor
public final class PhysicsSystem {
    public static let shared = PhysicsSystem()
    
    private init() {}
    
    /// Updates all entities with Transform and Velocity, resolving AABB collisions.
    public func update(deltaTime: Float, ecs: ECSManager, world: World) async {
        // Query for entities that have Transform, Velocity, and optionally AABB
        let querySig: Set<String> = ["TransformComponent", "VelocityComponent"]
        let entities = await ecs.query(signature: querySig)
        
        for (_, components) in entities {
            guard var transform = components["TransformComponent"] as? TransformComponent,
                  var velocity = components["VelocityComponent"] as? VelocityComponent else { continue }
            
            // Apply Gravity (Simple)
            velocity.value.y -= 9.8 * deltaTime
            
            // 1. Move X
            let nextPosX = transform.position + SIMD3(velocity.value.x * deltaTime, 0, 0)
            if await !checkCollision(at: nextPosX, world: world) {
                transform.position.x = nextPosX.x
            } else {
                velocity.value.x = 0
            }

            // 2. Move Z
            let nextPosZ = transform.position + SIMD3(0, 0, velocity.value.z * deltaTime)
            if await !checkCollision(at: nextPosZ, world: world) {
                transform.position.z = nextPosZ.z
            } else {
                velocity.value.z = 0
            }
            
            // 3. Move Y
            let nextPosY = transform.position + SIMD3(0, velocity.value.y * deltaTime, 0)
            if await !checkCollision(at: nextPosY, world: world) {
                transform.position.y = nextPosY.y
            } else { // Hit ground or ceiling
                velocity.value.y = 0
            }
        }
    }
    
    private func checkCollision(at position: SIMD3<Float>, world: World) async -> Bool {
        // Simple point collision for now (center of entity)
        // In a real implementation, we would check the AABB corners
        let bx = Int(floor(position.x))
        let by = Int(floor(position.y))
        let bz = Int(floor(position.z))
        
        let chunkPos = ChunkPosition(x: Int(floor(Float(bx) / 16.0)), z: Int(floor(Float(bz) / 16.0)))
        
        if let chunk = await world.getChunk(at: chunkPos) {
            let lx = (bx % 16 + 16) % 16
            let lz = (bz % 16 + 16) % 16
            return chunk.getBlock(at: SIMD3(lx, by, lz)).active
        }
        
        return false
    }
}
