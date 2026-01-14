import Foundation
import simd

// MARK: - Heightmap Physics Collision System

/// Heightmap physics collision system provides collision detection and response for terrain
@MainActor
class HeightmapPhysics: ObservableObject {
    static let shared = HeightmapPhysics()
    
    @Published var collisionEnabled: Bool = true
    @Published var heightmapData: [[Float]] = []
    @Published var heightmapSize: SIMD2<Int> = SIMD2<Int>(0, 0)
    @Published var heightmapScale: SIMD3<Float> = SIMD3<Float>(1, 1, 1)
    @Published var heightmapOffset: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    @Published var collisionAccuracy: CollisionAccuracy = .high
    @Published var maxStepHeight: Float = 0.5
    @Published var slopeLimit: Float = Float.pi / 3 // 60 degrees
    @Published var collisionStatistics: CollisionStatistics
    
    private var collisionCache: [CollisionCacheKey: CollisionResult] = [:]
    private var heightfield: Heightfield = Heightfield()
    private var broadphase: SpatialHash = SpatialHash(cellSize: 100.0)
    
    struct CollisionResult {
        let hasCollision: Bool
        let collisionPoint: SIMD3<Float>
        let collisionNormal: SIMD3<Float>
        let penetrationDepth: Float
        let heightAtPoint: Float
        let triangleIndex: Int?
        
        static let noCollision = CollisionResult(
            hasCollision: false,
            collisionPoint: .zero,
            collisionNormal: .zero,
            penetrationDepth: 0,
            heightAtPoint: 0,
            triangleIndex: nil
        )
    }
    
    struct CollisionCacheKey: Hashable {
        let x: Int
        let y: Int
        let z: Int
        let radius: Float
        
        init(position: SIMD3<Float>, radius: Float) {
            self.x = Int(floor(position.x))
            self.y = Int(floor(position.y))
            self.z = Int(floor(position.z))
            self.radius = radius
        }
    }
    
    struct RaycastResult {
        let hasHit: Bool
        let hitPoint: SIMD3<Float>
        let hitNormal: SIMD3<Float>
        let hitDistance: Float
        let triangleIndex: Int?
        let hitFraction: Float
        
        static let noHit = RaycastResult(
            hasHit: false,
            hitPoint: .zero,
            hitNormal: .zero,
            hitDistance: 0,
            triangleIndex: nil,
            hitFraction: 1.0
        )
    }
    
    enum CollisionAccuracy {
        case low      // 1x1 grid
        case medium   // 2x2 grid
        case high      // 4x4 grid
        case ultra     // 8x8 grid
        
        var subdivisionLevel: Int {
            switch self {
            case .low: return 1
            case .medium: return 2
            case .high: return 4
            case .ultra: return 8
            }
        }
        
        var sampleCount: Int {
            let level = subdivisionLevel
            return level * level
        }
    }
    
    struct CollisionStatistics {
        var totalCollisions: Int = 0
        var raycastTests: Int = 0
        var sphereTests: Int = 0
        var capsuleTests: Int = 0
        var cacheHits: Int = 0
        var cacheMisses: Int = 0
        var averageCollisionTime: Double = 0.0
        var lastUpdate: Date = Date()
        
        var cacheHitRate: Double {
            let total = cacheHits + cacheMisses
            return total > 0 ? Double(cacheHits) / Double(total) : 0.0
        }
    }
    
    private init() {
        collisionStatistics = CollisionStatistics()
    }
    
    // MARK: - Heightmap Management
    
    /// Set heightmap data for collision detection
    func setHeightmap(_ heightmap: [[Float]], scale: SIMD3<Float> = SIMD3<Float>(1, 1, 1), offset: SIMD3<Float> = .zero) {
        self.heightmapData = heightmap
        self.heightmapSize = SIMD2<Int>(heightmap[0].count, heightmap.count)
        self.heightmapScale = scale
        self.heightmapOffset = offset
        
        // Build heightfield for efficient collision detection
        buildHeightfield()
        
        // Clear collision cache
        collisionCache.removeAll()
        
        print("Heightmap collision system initialized: \(heightmapSize.x)x\(heightmapSize.y)")
    }
    
    private func buildHeightfield() {
        heightfield = Heightfield(
            data: heightmapData,
            scale: heightmapScale,
            offset: heightmapOffset
        )
        
        // Update broadphase
        broadphase = SpatialHash(cellSize: Double(max(heightmapScale.x, heightmapScale.z)))
    }
    
    // MARK: - Point Collision
    
    /// Get height at specific world position
    func getHeight(at worldPosition: SIMD3<Float>) -> Float {
        guard !heightmapData.isEmpty else { return 0 }
        
        let localPos = worldPosition - heightmapOffset
        let gridPos = SIMD2<Int>(
            Int(floor(localPos.x / heightmapScale.x)),
            Int(floor(localPos.z / heightmapScale.z))
        )
        
        // Check bounds
        guard gridPos.x >= 0 && gridPos.x < heightmapSize.x - 1 &&
              gridPos.y >= 0 && gridPos.y < heightmapSize.y - 1 else {
            return 0
        }
        
        // Bilinear interpolation for smooth height
        let exactX = localPos.x / heightmapScale.x
        let exactZ = localPos.z / heightmapScale.z
        
        let x0 = Int(floor(exactX))
        let x1 = min(x0 + 1, heightmapSize.x - 1)
        let z0 = Int(floor(exactZ))
        let z1 = min(z0 + 1, heightmapSize.y - 1)
        
        let fx = exactX - Float(x0)
        let fz = exactZ - Float(z0)
        
        let h00 = heightmapData[z0][x0]
        let h10 = heightmapData[z0][x1]
        let h01 = heightmapData[z1][x0]
        let h11 = heightmapData[z1][x1]
        
        let h0 = h00 * (1 - fx) + h10 * fx
        let h1 = h01 * (1 - fx) + h11 * fx
        
        return h0 * (1 - fz) + h1 * fz
    }
    
    /// Get normal at specific world position
    func getNormal(at worldPosition: SIMD3<Float>) -> SIMD3<Float> {
        let sampleDistance: Float = 1.0
        
        let hL = getHeight(at: worldPosition + SIMD3<Float>(-sampleDistance, 0, 0))
        let hR = getHeight(at: worldPosition + SIMD3<Float>(sampleDistance, 0, 0))
        let hD = getHeight(at: worldPosition + SIMD3<Float>(0, 0, -sampleDistance))
        let hU = getHeight(at: worldPosition + SIMD3<Float>(0, 0, sampleDistance))
        
        let normal = SIMD3<Float>(hL - hR, 2 * sampleDistance, hD - hU)
        return normalize(normal)
    }
    
    /// Check if point collides with heightmap
    func checkPointCollision(_ worldPosition: SIMD3<Float>, radius: Float = 0) -> CollisionResult {
        guard collisionEnabled else { return .noCollision }
        
        let cacheKey = CollisionCacheKey(position: worldPosition, radius: radius)
        
        // Check cache first
        if let cachedResult = collisionCache[cacheKey] {
            collisionStatistics.cacheHits += 1
            return cachedResult
        }
        
        collisionStatistics.cacheMisses += 1
        collisionStatistics.totalCollisions += 1
        
        let startTime = Date()
        
        let height = getHeight(at: worldPosition)
        let localPos = worldPosition - heightmapOffset
        let terrainY = localPos.y - height
        
        let result: CollisionResult
        
        if terrainY < radius {
            // Collision detected
            let collisionPoint = SIMD3<Float>(worldPosition.x, height + radius, worldPosition.z)
            let normal = getNormal(at: worldPosition)
            
            result = CollisionResult(
                hasCollision: true,
                collisionPoint: collisionPoint,
                collisionNormal: normal,
                penetrationDepth: radius - terrainY,
                heightAtPoint: height,
                triangleIndex: nil
            )
        } else {
            // No collision
            result = CollisionResult(
                hasCollision: false,
                collisionPoint: worldPosition,
                collisionNormal: SIMD3<Float>(0, 1, 0),
                penetrationDepth: 0,
                heightAtPoint: height,
                triangleIndex: nil
            )
        }
        
        // Cache result
        collisionCache[cacheKey] = result
        
        // Update statistics
        let collisionTime = Date().timeIntervalSince(startTime)
        updateCollisionStatistics(collisionTime: collisionTime)
        
        return result
    }
    
    // MARK: - Sphere Collision
    
    /// Check sphere collision with heightmap
    func checkSphereCollision(_ center: SIMD3<Float>, radius: Float) -> CollisionResult {
        guard collisionEnabled else { return .noCollision }
        
        let startTime = Date()
        collisionStatistics.sphereTests += 1
        
        // Get height at sphere center
        let heightAtCenter = getHeight(at: center)
        let localCenter = center - heightmapOffset
        let terrainY = localCenter.y - heightAtCenter
        
        // Simple sphere-heightmap collision
        if terrainY < radius {
            let collisionPoint = SIMD3<Float>(center.x, heightAtCenter + radius, center.z)
            let normal = getNormal(at: center)
            
            let result = CollisionResult(
                hasCollision: true,
                collisionPoint: collisionPoint,
                collisionNormal: normal,
                penetrationDepth: radius - terrainY,
                heightAtPoint: heightAtCenter,
                triangleIndex: nil
            )
            
            let collisionTime = Date().timeIntervalSince(startTime)
            updateCollisionStatistics(collisionTime: collisionTime)
            
            return result
        }
        
        return .noCollision
    }
    
    /// Check sphere sweep against heightmap
    func checkSphereSweep(
        from start: SIMD3<Float>,
        to end: SIMD3<Float>,
        radius: Float
    ) -> (result: CollisionResult, hitFraction: Float) {
        guard collisionEnabled else { return (.noCollision, 1.0) }
        
        let startTime = Date()
        collisionStatistics.sphereTests += 1
        
        let direction = normalize(end - start)
        let distance = length(end - start)
        let steps = Int(ceil(distance / radius))
        
        var closestResult = CollisionResult.noCollision
        var closestFraction: Float = 1.0
        
        for i in 0...steps {
            let fraction = Float(i) / Float(steps)
            let testPoint = start + direction * distance * fraction
            
            let result = checkSphereCollision(testPoint, radius: radius)
            
            if result.hasCollision && fraction < closestFraction {
                closestResult = result
                closestFraction = fraction
            }
        }
        
        let collisionTime = Date().timeIntervalSince(startTime)
        updateCollisionStatistics(collisionTime: collisionTime)
        
        return (closestResult, closestFraction)
    }
    
    // MARK: - Capsule Collision
    
    /// Check capsule collision with heightmap
    func checkCapsuleCollision(
        from start: SIMD3<Float>,
        to end: SIMD3<Float>,
        radius: Float
    ) -> CollisionResult {
        guard collisionEnabled else { return .noCollision }
        
        let startTime = Date()
        collisionStatistics.capsuleTests += 1
        
        // Check both sphere endpoints and sweep
        let startResult = checkSphereCollision(start, radius: radius)
        let endResult = checkSphereCollision(end, radius: radius)
        let sweepResult = checkSphereSweep(from: start, to: end, radius: radius)
        
        // Return the most significant collision
        var bestResult = CollisionResult.noCollision
        var maxPenetration: Float = 0
        
        if startResult.hasCollision && startResult.penetrationDepth > maxPenetration {
            bestResult = startResult
            maxPenetration = startResult.penetrationDepth
        }
        
        if endResult.hasCollision && endResult.penetrationDepth > maxPenetration {
            bestResult = endResult
            maxPenetration = endResult.penetrationDepth
        }
        
        if sweepResult.result.hasCollision && sweepResult.result.penetrationDepth > maxPenetration {
            bestResult = sweepResult.result
        }
        
        let collisionTime = Date().timeIntervalSince(startTime)
        updateCollisionStatistics(collisionTime: collisionTime)
        
        return bestResult
    }
    
    // MARK: - Raycasting
    
    /// Cast ray against heightmap
    func raycast(
        from origin: SIMD3<Float>,
        direction: SIMD3<Float>,
        maxDistance: Float = 1000.0
    ) -> RaycastResult {
        guard collisionEnabled else { return .noHit }
        
        let startTime = Date()
        collisionStatistics.raycastTests += 1
        
        let normalizedDir = normalize(direction)
        let stepSize: Float = 1.0
        let steps = Int(min(maxDistance / stepSize, 10000))
        
        var closestHit = RaycastResult.noHit
        var closestDistance: Float = maxDistance
        
        for i in 0...steps {
            let currentDistance = Float(i) * stepSize
            if currentDistance > closestDistance {
                break
            }
            
            let testPoint = origin + normalizedDir * currentDistance
            let height = getHeight(at: testPoint)
            let localPoint = testPoint - heightmapOffset
            let terrainY = localPoint.y - height
            
            if terrainY > 0 {
                // Ray hit terrain
                let hitNormal = getNormal(at: testPoint)
                
                let hitResult = RaycastResult(
                    hasHit: true,
                    hitPoint: testPoint,
                    hitNormal: hitNormal,
                    hitDistance: currentDistance,
                    triangleIndex: nil,
                    hitFraction: currentDistance / maxDistance
                )
                
                if currentDistance < closestDistance {
                    closestHit = hitResult
                    closestDistance = currentDistance
                }
                
                break
            }
        }
        
        let raycastTime = Date().timeIntervalSince(startTime)
        updateCollisionStatistics(collisionTime: raycastTime)
        
        return closestHit
    }
    
    /// Multiple raycasts in a pattern
    func multiRaycast(
        from origin: SIMD3<Float>,
        directions: [SIMD3<Float>],
        maxDistance: Float = 1000.0
    ) -> [RaycastResult] {
        return directions.map { direction in
            raycast(from: origin, direction: direction, maxDistance: maxDistance)
        }
    }
    
    // MARK: - Character Controller
    
    /// Simple character controller for heightmap collision
    func moveCharacter(
        position: inout SIMD3<Float>,
        velocity: inout SIMD3<Float>,
        radius: Float,
        height: Float,
        deltaTime: Float
    ) -> SIMD3<Float> {
        guard collisionEnabled else {
            return position + velocity * deltaTime
        }
        
        let startTime = Date()
        
        // Apply gravity
        velocity.y -= 9.81 * deltaTime
        
        // Limit step height
        let newPosition = position + velocity * deltaTime
        
        // Check ground collision
        let groundCheck = checkSphereCollision(
            newPosition + SIMD3<Float>(0, -height + radius, 0),
            radius: radius
        )
        
        if groundCheck.hasCollision {
            // Place character on ground
            position.y = groundCheck.heightAtPoint + height
            velocity.y = 0
            
            // Apply slope limit
            let normal = groundCheck.collisionNormal
            let slopeAngle = acos(max(-1, min(1, dot(normal, SIMD3<Float>(0, 1, 0)))))
            
            if slopeAngle > slopeLimit {
                // Prevent movement on steep slopes
                let slideDirection = normalize(SIMD3<Float>(normal.x, 0, normal.z))
                velocity.x = slideDirection.x * 2.0
                velocity.z = slideDirection.z * 2.0
            }
        } else {
            position = newPosition
        }
        
        let collisionTime = Date().timeIntervalSince(startTime)
        updateCollisionStatistics(collisionTime: collisionTime)
        
        return position
    }
    
    // MARK: - Utility Functions
    
    private func updateCollisionStatistics(collisionTime: Double) {
        let totalTime = collisionStatistics.averageCollisionTime * Double(collisionStatistics.totalCollisions) + collisionTime
        collisionStatistics.averageCollisionTime = totalTime / Double(collisionStatistics.totalCollisions + 1)
        collisionStatistics.lastUpdate = Date()
    }
    
    /// Clear collision cache
    func clearCache() {
        collisionCache.removeAll()
        print("Collision cache cleared")
    }
    
    /// Get collision statistics
    func getCollisionStatistics() -> CollisionStatistics {
        return collisionStatistics
    }
    
    /// Configure collision system
    func configureCollision(
        enabled: Bool,
        accuracy: CollisionAccuracy,
        maxStepHeight: Float,
        slopeLimit: Float
    ) {
        collisionEnabled = enabled
        collisionAccuracy = accuracy
        self.maxStepHeight = maxStepHeight
        self.slopeLimit = slopeLimit
        
        print("Collision configured: enabled=\(enabled), accuracy=\(accuracy), stepHeight=\(maxStepHeight)")
    }
    
    /// Validate position is within heightmap bounds
    func isPositionValid(_ position: SIMD3<Float>) -> Bool {
        let localPos = position - heightmapOffset
        let gridPos = SIMD2<Int>(
            Int(floor(localPos.x / heightmapScale.x)),
            Int(floor(localPos.z / heightmapScale.z))
        )
        
        return gridPos.x >= 0 && gridPos.x < heightmapSize.x &&
               gridPos.y >= 0 && gridPos.y < heightmapSize.y
    }
}

// MARK: - Supporting Structures

class Heightfield {
    var data: [[Float]] = []
    var scale: SIMD3<Float> = SIMD3<Float>(1, 1, 1)
    var offset: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    
    init(data: [[Float]] = [], scale: SIMD3<Float> = SIMD3<Float>(1, 1, 1), offset: SIMD3<Float> = .zero) {
        self.data = data
        self.scale = scale
        self.offset = offset
    }
    
    func getHeight(_ x: Int, _ z: Int) -> Float {
        guard x >= 0 && x < data.count && z >= 0 && z < data[0].count else {
            return 0
        }
        return data[z][x]
    }
}

class SpatialHash {
    var cellSize: Double
    var grid: [Int: [SIMD3<Float>]] = [:]
    
    init(cellSize: Double) {
        self.cellSize = cellSize
    }
    
    func insert(_ point: SIMD3<Float>) {
        let key = getHashKey(point)
        if grid[key] == nil {
            grid[key] = []
        }
        grid[key]?.append(point)
    }
    
    func query(_ point: SIMD3<Float>, radius: Float) -> [SIMD3<Float>] {
        let minKey = getHashKey(point - SIMD3<Float>(radius, radius, radius))
        let maxKey = getHashKey(point + SIMD3<Float>(radius, radius, radius))
        
        var results: [SIMD3<Float>] = []
        
        for x in minKey.x...maxKey.x {
            for y in minKey.y...maxKey.y {
                for z in minKey.z...maxKey.z {
                    let key = x + y * 1000 + z * 1000000 // Simple hash
                    if let points = grid[key] {
                        results.append(contentsOf: points)
                    }
                }
            }
        }
        
        return results.filter { length($0 - point) <= Double(radius) }
    }
    
    private func getHashKey(_ point: SIMD3<Float>) -> Int {
        return Int(floor(point.x / cellSize)) +
               Int(floor(point.y / cellSize)) * 1000 +
               Int(floor(point.z / cellSize)) * 1000000
    }
}
