import Foundation
import simd

// MARK: - Large World Coordinates System

/// Large world coordinates system provides support for massive game worlds beyond floating-point precision limits
@MainActor
class LargeWorldCoordinates: ObservableObject {
    static let shared = LargeWorldCoordinates()
    
    @Published var worldOrigin: SIMD3<Double> = .zero
    @Published var currentChunk: WorldChunk = WorldChunk(x: 0, y: 0, z: 0)
    @Published var loadedChunks: Set<WorldChunk> = []
    @Published var coordinateSystem: CoordinateSystem = .rightHandedYUp
    @Published var worldScale: Double = 1.0
    @Published var maxWorldSize: SIMD3<Double> = SIMD3<Double>(1_000_000, 1_000_000, 1_000_000)
    
    private var chunkSize: SIMD3<Int> = SIMD3<Int>(1024, 1024, 1024)
    private var rebaseThreshold: Double = 100_000.0
    private var lastCameraPosition: SIMD3<Double> = .zero
    
    struct WorldChunk: Hashable, Codable {
        let x: Int
        let y: Int
        let z: Int
        
        var worldPosition: SIMD3<Double> {
            return SIMD3<Double>(Double(x), Double(y), Double(z))
        }
        
        func distance(to other: WorldChunk) -> Double {
            let dx = Double(x - other.x)
            let dy = Double(y - other.y)
            let dz = Double(z - other.z)
            return sqrt(dx*dx + dy*dy + dz*dz)
        }
    }
    
    enum CoordinateSystem {
        case rightHandedYUp
        case rightHandedZUp
        case leftHandedYUp
        case leftHandedZUp
    }
    
    private init() {}
    
    // MARK: - Coordinate Conversion
    
    /// Convert world coordinates to local coordinates relative to current origin
    func worldToLocal(_ worldPos: SIMD3<Double>) -> SIMD3<Double> {
        return worldPos - worldOrigin
    }
    
    /// Convert local coordinates to world coordinates
    func localToWorld(_ localPos: SIMD3<Double>) -> SIMD3<Double> {
        return localPos + worldOrigin
    }
    
    /// Convert world coordinates to chunk coordinates
    func worldToChunk(_ worldPos: SIMD3<Double>) -> WorldChunk {
        let chunkX = Int(floor(worldPos.x / Double(chunkSize.x)))
        let chunkY = Int(floor(worldPos.y / Double(chunkSize.y)))
        let chunkZ = Int(floor(worldPos.z / Double(chunkSize.z)))
        return WorldChunk(x: chunkX, y: chunkY, z: chunkZ)
    }
    
    /// Convert chunk coordinates to world coordinates (chunk center)
    func chunkToWorld(_ chunk: WorldChunk) -> SIMD3<Double> {
        return SIMD3<Double>(
            (Double(chunk.x) + 0.5) * Double(chunkSize.x),
            (Double(chunk.y) + 0.5) * Double(chunkSize.y),
            (Double(chunk.z) + 0.5) * Double(chunkSize.z)
        )
    }
    
    /// Convert world coordinates to floating-point precision (for rendering)
    func worldToFloat(_ worldPos: SIMD3<Double>) -> SIMD3<Float> {
        let localPos = worldToLocal(worldPos)
        return SIMD3<Float>(
            Float(localPos.x),
            Float(localPos.y),
            Float(localPos.z)
        )
    }
    
    // MARK: - Origin Management
    
    /// Update world origin based on camera position
    func updateOrigin(for cameraPosition: SIMD3<Double>) {
        let distanceFromOrigin = length(cameraPosition - worldOrigin)
        
        if distanceFromOrigin > rebaseThreshold {
            rebaseOrigin(to: cameraPosition)
        }
        
        lastCameraPosition = cameraPosition
    }
    
    /// Manually rebase world origin to new position
    func rebaseOrigin(to newOrigin: SIMD3<Double>) {
        let oldOrigin = worldOrigin
        worldOrigin = newOrigin
        
        // Update current chunk
        currentChunk = worldToChunk(newOrigin)
        
        // Notify systems that need coordinate rebasing
        NotificationCenter.default.post(
            name: .worldOriginRebased,
            object: nil,
            userInfo: [
                "oldOrigin": oldOrigin,
                "newOrigin": newOrigin,
                "delta": newOrigin - oldOrigin
            ]
        )
        
        print("World origin rebased from \(oldOrigin) to \(newOrigin)")
    }
    
    /// Rebase origin to center of loaded chunks
    func rebaseToCenter() {
        guard !loadedChunks.isEmpty else { return }
        
        var sumX: Double = 0
        var sumY: Double = 0
        var sumZ: Double = 0
        
        for chunk in loadedChunks {
            let chunkWorldPos = chunkToWorld(chunk)
            sumX += chunkWorldPos.x
            sumY += chunkWorldPos.y
            sumZ += chunkWorldPos.z
        }
        
        let center = SIMD3<Double>(
            sumX / Double(loadedChunks.count),
            sumY / Double(loadedChunks.count),
            sumZ / Double(loadedChunks.count)
        )
        
        rebaseOrigin(to: center)
    }
    
    // MARK: - Chunk Management
    
    /// Get chunks within loading distance of position
    func getChunksInRange(of position: SIMD3<Double>, radius: Double) -> Set<WorldChunk> {
        let centerChunk = worldToChunk(position)
        let chunkRadius = Int(ceil(radius / Double(chunkSize.x)))
        
        var chunks: Set<WorldChunk> = []
        
        for dx in -chunkRadius...chunkRadius {
            for dy in -chunkRadius...chunkRadius {
                for dz in -chunkRadius...chunkRadius {
                    let chunk = WorldChunk(
                        x: centerChunk.x + dx,
                        y: centerChunk.y + dy,
                        z: centerChunk.z + dz
                    )
                    
                    let chunkWorldPos = chunkToWorld(chunk)
                    let distance = length(chunkWorldPos - position)
                    
                    if distance <= radius {
                        chunks.insert(chunk)
                    }
                }
            }
        }
        
        return chunks
    }
    
    /// Load chunks around position
    func loadChunks(around position: SIMD3<Double>, loadRadius: Double) {
        let chunksToLoad = getChunksInRange(of: position, radius: loadRadius)
        
        // Unload distant chunks
        let chunksToUnload = loadedChunks.subtracting(chunksToLoad)
        for chunk in chunksToUnload {
            unloadChunk(chunk)
        }
        
        // Load new chunks
        for chunk in chunksToLoad.subtracting(loadedChunks) {
            loadChunk(chunk)
        }
    }
    
    /// Load a single chunk
    private func loadChunk(_ chunk: WorldChunk) {
        loadedChunks.insert(chunk)
        
        NotificationCenter.default.post(
            name: .chunkLoaded,
            object: nil,
            userInfo: ["chunk": chunk]
        )
    }
    
    /// Unload a single chunk
    private func unloadChunk(_ chunk: WorldChunk) {
        loadedChunks.remove(chunk)
        
        NotificationCenter.default.post(
            name: .chunkUnloaded,
            object: nil,
            userInfo: ["chunk": chunk]
        )
    }
    
    // MARK: - Precision Management
    
    /// Check if position is within safe floating-point range
    func isWithinSafeRange(_ position: SIMD3<Double>) -> Bool {
        let maxSafeValue: Double = 100_000.0 // Safe range for single precision
        let localPos = worldToLocal(position)
        
        return abs(localPos.x) < maxSafeValue &&
               abs(localPos.y) < maxSafeValue &&
               abs(localPos.z) < maxSafeValue
    }
    
    /// Get recommended origin position for given camera position
    func getRecommendedOrigin(for cameraPosition: SIMD3<Double>) -> SIMD3<Double> {
        let chunkPos = worldToChunk(cameraPosition)
        return chunkToWorld(chunkPos)
    }
    
    // MARK: - Coordinate System Support
    
    /// Transform position based on coordinate system
    func transformPosition(_ position: SIMD3<Double>, from system: CoordinateSystem, to targetSystem: CoordinateSystem) -> SIMD3<Double> {
        if system == targetSystem {
            return position
        }
        
        // Handle coordinate system conversions
        switch (system, targetSystem) {
        case (.rightHandedYUp, .rightHandedZUp):
            return SIMD3<Double>(position.x, position.z, -position.y)
        case (.rightHandedZUp, .rightHandedYUp):
            return SIMD3<Double>(position.x, -position.z, position.y)
        case (.rightHandedYUp, .leftHandedYUp):
            return SIMD3<Double>(-position.x, position.y, position.z)
        case (.leftHandedYUp, .rightHandedYUp):
            return SIMD3<Double>(-position.x, position.y, position.z)
        default:
            // Add more conversions as needed
            return position
        }
    }
    
    // MARK: - Validation
    
    /// Validate world coordinates are within bounds
    func validateWorldBounds(_ position: SIMD3<Double>) -> Bool {
        return position.x >= 0 && position.x <= maxWorldSize.x &&
               position.y >= 0 && position.y <= maxWorldSize.y &&
               position.z >= 0 && position.z <= maxWorldSize.z
    }
    
    /// Clamp position to world bounds
    func clampToWorldBounds(_ position: SIMD3<Double>) -> SIMD3<Double> {
        return SIMD3<Double>(
            max(0, min(position.x, maxWorldSize.x)),
            max(0, min(position.y, maxWorldSize.y)),
            max(0, min(position.z, maxWorldSize.z))
        )
    }
    
    // MARK: - Statistics
    
    /// Get current world statistics
    func getWorldStatistics() -> WorldStatistics {
        return WorldStatistics(
            currentOrigin: worldOrigin,
            loadedChunkCount: loadedChunks.count,
            currentChunk: currentChunk,
            worldScale: worldScale,
            coordinateSystem: coordinateSystem,
            maxWorldSize: maxWorldSize,
            lastCameraPosition: lastCameraPosition
        )
    }
    
    struct WorldStatistics {
        let currentOrigin: SIMD3<Double>
        let loadedChunkCount: Int
        let currentChunk: WorldChunk
        let worldScale: Double
        let coordinateSystem: CoordinateSystem
        let maxWorldSize: SIMD3<Double>
        let lastCameraPosition: SIMD3<Double>
        
        var originDistanceFromZero: Double {
            return length(currentOrigin)
        }
        
        var loadedArea: Double {
            return Double(loadedChunkCount) * Double(1024 * 1024) // Assuming 1km x 1km chunks
        }
    }
}

// MARK: - Notification Names

extension Notification.Name {
    static let worldOriginRebased = Notification.Name("worldOriginRebased")
    static let chunkLoaded = Notification.Name("chunkLoaded")
    static let chunkUnloaded = Notification.Name("chunkUnloaded")
}

// MARK: - Large World Extensions

extension SIMD3<Double> {
    /// Convert to chunk coordinates
    func toChunk(chunkSize: SIMD3<Int>) -> LargeWorldCoordinates.WorldChunk {
        return LargeWorldCoordinates.WorldChunk(
            x: Int(floor(self.x / Double(chunkSize.x))),
            y: Int(floor(self.y / Double(chunkSize.y))),
            z: Int(floor(self.z / Double(chunkSize.z)))
        )
    }
    
    /// Get distance from origin in safe range
    func safeDistance(from origin: SIMD3<Double>) -> Double {
        let delta = self - origin
        let maxSafeValue: Double = 100_000.0
        
        // Clamp to safe range to avoid precision issues
        let clampedDelta = SIMD3<Double>(
            max(-maxSafeValue, min(delta.x, maxSafeValue)),
            max(-maxSafeValue, min(delta.y, maxSafeValue)),
            max(-maxSafeValue, min(delta.z, maxSafeValue))
        )
        
        return length(clampedDelta)
    }
}

// MARK: - Large World Renderer Integration

class LargeWorldRenderer {
    private let largeWorld = LargeWorldCoordinates.shared
    private var renderOrigin: SIMD3<Double> = .zero
    
    /// Update renderer for large world coordinates
    func updateRenderer(cameraPosition: SIMD3<Double>) {
        largeWorld.updateOrigin(for: cameraPosition)
        
        // Check if we need to update render origin
        let currentRenderOrigin = largeWorld.worldOrigin
        if length(currentRenderOrigin - renderOrigin) > 1000.0 {
            renderOrigin = currentRenderOrigin
            updateRenderMatrices()
        }
    }
    
    /// Get view matrix for large world
    func getViewMatrix(cameraPosition: SIMD3<Double>, cameraRotation: simd_quatd) -> simd_float4x4 {
        let localPosition = largeWorld.worldToLocal(cameraPosition)
        let floatPosition = SIMD3<Float>(Float(localPosition.x), Float(localPosition.y), Float(localPosition.z))
        let floatRotation = simd_quatf(ix: Float(cameraRotation.ix), iy: Float(cameraRotation.iy), iz: Float(cameraRotation.iz), r: Float(cameraRotation.r))
        
        return simd_float4x4(floatRotation) * simd_float4x4(translation: floatPosition)
    }
    
    private func updateRenderMatrices() {
        // Update any render matrices that depend on world origin
        NotificationCenter.default.post(
            name: .renderMatricesUpdated,
            object: nil,
            userInfo: ["origin": renderOrigin]
        )
    }
}

extension Notification.Name {
    static let renderMatricesUpdated = Notification.Name("renderMatricesUpdated")
}
