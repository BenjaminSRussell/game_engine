import Foundation
import simd

// MARK: - Terrain Neighbor Connectivity System

/// Terrain neighbor connectivity system manages connections between adjacent terrain chunks
@MainActor
class TerrainNeighborConnectivity: ObservableObject {
    static let shared = TerrainNeighborConnectivity()
    
    @Published var connections: [TerrainConnection] = []
    @Published var connectivityGraph: [UUID: Set<UUID>] = [:]
    @Published var connectionStrength: Float = 1.0
    @Published var autoConnectEnabled: Bool = true
    @Published var connectionDistance: Float = 100.0
    @Published var maxConnectionsPerChunk: Int = 8
    @Published var connectivityStatistics: ConnectivityStatistics
    
    private var terrainChunks: [UUID: TerrainChunk] = [:]
    private var connectionCache: [ConnectionCacheKey: [TerrainConnection]] = [:]
    private var updateTimer: Timer?
    
    struct TerrainConnection: Identifiable, Codable {
        let id: UUID
        let fromChunkID: UUID
        let toChunkID: UUID
        let connectionType: ConnectionType
        let strength: Float
        let seamData: SeamData?
        let createdAt: Date
        let lastValidated: Date
        
        enum ConnectionType: String, Codable, CaseIterable {
            case seamless = "seamless"
            case stitched = "stitched"
            case blended = "blended"
            case morphed = "morphed"
            case lodTransition = "lod_transition"
        }
        
        var isValid: Bool {
            return strength > 0.1
        }
        
        func reverse() -> TerrainConnection {
            return TerrainConnection(
                id: UUID(),
                fromChunkID: toChunkID,
                toChunkID: fromChunkID,
                connectionType: connectionType,
                strength: strength,
                seamData: seamData,
                createdAt: createdAt,
                lastValidated: lastValidated
            )
        }
    }
    
    struct SeamData: Codable {
        let vertices: [SIMD3<Float>]
        let indices: [Int]
        let normals: [SIMD3<Float>]
        let uvs: [[SIMD2<Float>]]
        let seamType: SeamType
        
        enum SeamType: String, Codable, CaseIterable {
            case height = "height"
            case texture = "texture"
            case normal = "normal"
            case lod = "lod"
            case complete = "complete"
        }
    }
    
    struct TerrainChunk: Identifiable, Codable {
        let id: UUID
        let position: SIMD3<Float>
        let size: SIMD3<Float>
        let lodLevel: Int
        let heightmap: [[Float]]
        let textures: [Int]
        let neighbors: Set<UUID>
        let isActive: Bool
        
        var bounds: BoundingBox {
            return BoundingBox(
                min: position,
                max: position + size
            )
        }
        
        var center: SIMD3<Float> {
            return position + size / 2.0
        }
        
        func distance(to other: TerrainChunk) -> Float {
            return length(center - other.center)
        }
    }
    
    struct BoundingBox: Codable {
        let min: SIMD3<Float>
        let max: SIMD3<Float>
        
        func contains(_ point: SIMD3<Float>) -> Bool {
            return point.x >= min.x && point.x <= max.x &&
                   point.y >= min.y && point.y <= max.y &&
                   point.z >= min.z && point.z <= max.z
        }
        
        func intersects(_ other: BoundingBox) -> Bool {
            return !(max.x < other.min.x || other.max.x < min.x ||
                    max.y < other.min.y || other.max.y < min.y ||
                    max.z < other.min.z || other.max.z < min.z)
        }
        
        var volume: Float {
            let size = max - min
            return size.x * size.y * size.z
        }
    }
    
    struct ConnectionCacheKey: Hashable {
        let chunk1: UUID
        let chunk2: UUID
        
        init(_ id1: UUID, _ id2: UUID) {
            self.chunk1 = min(id1, id2)
            self.chunk2 = max(id1, id2)
        }
    }
    
    struct ConnectivityStatistics {
        var totalConnections: Int = 0
        var seamlessConnections: Int = 0
        var stitchedConnections: Int = 0
        var blendedConnections: Int = 0
        var averageConnectionStrength: Float = 0.0
        var isolatedChunks: Int = 0
        var connectedComponents: Int = 0
        var lastUpdate: Date = Date()
        
        var connectivityRatio: Float {
            guard !terrainChunks.isEmpty else { return 0.0 }
            return Float(totalConnections) / Float(terrainChunks.count * 4) // Max 4 connections per chunk
        }
    }
    
    private init() {
        connectivityStatistics = ConnectivityStatistics()
        setupUpdateTimer()
    }
    
    deinit {
        updateTimer?.invalidate()
    }
    
    // MARK: - Chunk Management
    
    /// Register a terrain chunk for connectivity
    func registerChunk(_ chunk: TerrainChunk) {
        terrainChunks[chunk.id] = chunk
        
        if autoConnectEnabled {
            findAndCreateConnections(for: chunk)
        }
        
        updateConnectivityStatistics()
        print("Registered terrain chunk: \(chunk.id)")
    }
    
    /// Unregister a terrain chunk
    func unregisterChunk(_ chunkID: UUID) {
        // Remove all connections to/from this chunk
        connections.removeAll { $0.fromChunkID == chunkID || $0.toChunkID == chunkID }
        
        // Remove from connectivity graph
        connectivityGraph.removeValue(forKey: chunkID)
        for (key, var neighbors) in connectivityGraph {
            neighbors.remove(chunkID)
            connectivityGraph[key] = neighbors
        }
        
        // Remove chunk
        terrainChunks.removeValue(forKey: chunkID)
        
        updateConnectivityStatistics()
        print("Unregistered terrain chunk: \(chunkID)")
    }
    
    /// Update chunk data (triggers connection re-evaluation)
    func updateChunk(_ chunk: TerrainChunk) {
        terrainChunks[chunk.id] = chunk
        
        if autoConnectEnabled {
            // Remove old connections
            removeConnections(for: chunk.id)
            
            // Create new connections
            findAndCreateConnections(for: chunk)
        }
        
        updateConnectivityStatistics()
    }
    
    // MARK: - Connection Management
    
    private func findAndCreateConnections(for chunk: TerrainChunk) {
        let nearbyChunks = findNearbyChunks(chunk)
        
        for nearbyChunk in nearbyChunks {
            if shouldConnect(chunk, to: nearbyChunk) {
                let connection = createConnection(from: chunk, to: nearbyChunk)
                addConnection(connection)
            }
        }
    }
    
    private func findNearbyChunks(_ chunk: TerrainChunk) -> [TerrainChunk] {
        return terrainChunks.values.filter { otherChunk in
            otherChunk.id != chunk.id &&
            chunk.distance(to: otherChunk) <= connectionDistance
        }.sorted { $0.distance(to: chunk) < $1.distance(to: chunk) }
    }
    
    private func shouldConnect(_ chunk1: TerrainChunk, to chunk2: TerrainChunk) -> Bool {
        // Check if chunks are already connected
        if connectivityGraph[chunk1.id]?.contains(chunk2.id) == true {
            return false
        }
        
        // Check LOD compatibility
        let lodDifference = abs(chunk1.lodLevel - chunk2.lodLevel)
        if lodDifference > 1 {
            return false // Don't connect chunks with large LOD differences
        }
        
        // Check connection limit
        let currentConnections = connectivityGraph[chunk1.id]?.count ?? 0
        if currentConnections >= maxConnectionsPerChunk {
            return false
        }
        
        // Check spatial proximity
        return chunk1.distance(to: chunk2) <= connectionDistance
    }
    
    private func createConnection(from chunk1: TerrainChunk, to chunk2: TerrainChunk) -> TerrainConnection {
        let connectionType = determineConnectionType(chunk1, chunk2)
        let strength = calculateConnectionStrength(chunk1, chunk2)
        let seamData = generateSeamData(chunk1, chunk2, type: connectionType)
        
        return TerrainConnection(
            id: UUID(),
            fromChunkID: chunk1.id,
            toChunkID: chunk2.id,
            connectionType: connectionType,
            strength: strength,
            seamData: seamData,
            createdAt: Date(),
            lastValidated: Date()
        )
    }
    
    private func determineConnectionType(_ chunk1: TerrainChunk, _ chunk2: TerrainChunk) -> TerrainConnection.ConnectionType {
        let lodDifference = abs(chunk1.lodLevel - chunk2.lodLevel)
        let distance = chunk1.distance(to: chunk2)
        
        if lodDifference == 0 && distance < 10.0 {
            return .seamless
        } else if lodDifference == 1 {
            return .lodTransition
        } else if distance < 50.0 {
            return .blended
        } else {
            return .stitched
        }
    }
    
    private func calculateConnectionStrength(_ chunk1: TerrainChunk, _ chunk2: TerrainChunk) -> Float {
        let distance = chunk1.distance(to: chunk2)
        let lodDifference = abs(chunk1.lodLevel - chunk2.lodLevel)
        
        // Base strength decreases with distance and LOD difference
        let distanceFactor = max(0, 1.0 - distance / connectionDistance)
        let lodFactor = max(0, 1.0 - Float(lodDifference) / 4.0)
        
        return distanceFactor * lodFactor * connectionStrength
    }
    
    private func generateSeamData(_ chunk1: TerrainChunk, _ chunk2: TerrainChunk, type: TerrainConnection.ConnectionType) -> SeamData? {
        switch type {
        case .seamless:
            return nil // No seam data needed for seamless connections
        case .stitched, .blended, .morphed:
            return generateStitchingSeam(chunk1, chunk2)
        case .lodTransition:
            return generateLODSeam(chunk1, chunk2)
        }
    }
    
    private func generateStitchingSeam(_ chunk1: TerrainChunk, _ chunk2: TerrainChunk) -> SeamData {
        // Generate seam vertices along the shared edge
        let edgeVertices = generateEdgeVertices(chunk1, chunk2)
        let seamNormals = generateSeamNormals(edgeVertices)
        let seamUVs = generateSeamUVs(edgeVertices, chunk1, chunk2)
        
        return SeamData(
            vertices: edgeVertices,
            indices: generateSeamIndices(edgeVertices),
            normals: seamNormals,
            uvs: seamUVs,
            seamType: .complete
        )
    }
    
    private func generateLODSeam(_ chunk1: TerrainChunk, _ chunk2: TerrainChunk) -> SeamData {
        // Generate LOD transition seam
        let transitionVertices = generateLODTransitionVertices(chunk1, chunk2)
        let transitionNormals = generateLODTransitionNormals(transitionVertices)
        
        return SeamData(
            vertices: transitionVertices,
            indices: generateLODTransitionIndices(transitionVertices),
            normals: transitionNormals,
            uvs: [],
            seamType: .lod
        )
    }
    
    // MARK: - Connection Operations
    
    private func addConnection(_ connection: TerrainConnection) {
        connections.append(connection)
        
        // Update connectivity graph
        if connectivityGraph[connection.fromChunkID] == nil {
            connectivityGraph[connection.fromChunkID] = []
        }
        connectivityGraph[connection.fromChunkID]?.insert(connection.toChunkID)
        
        // Add reverse connection for undirected graph
        if connectivityGraph[connection.toChunkID] == nil {
            connectivityGraph[connection.toChunkID] = []
        }
        connectivityGraph[connection.toChunkID]?.insert(connection.fromChunkID)
        
        // Update cache
        let cacheKey = ConnectionCacheKey(connection.fromChunkID, connection.toChunkID)
        connectionCache[cacheKey] = [connection]
    }
    
    private func removeConnections(for chunkID: UUID) {
        connections.removeAll { $0.fromChunkID == chunkID || $0.toChunkID == chunkID }
        
        // Update connectivity graph
        if let neighbors = connectivityGraph[chunkID] {
            for neighborID in neighbors {
                connectivityGraph[neighborID]?.remove(chunkID)
            }
        }
        connectivityGraph.removeValue(forKey: chunkID)
        
        // Clear cache
        let keysToRemove = connectionCache.keys.filter { $0.chunk1 == chunkID || $0.chunk2 == chunkID }
        for key in keysToRemove {
            connectionCache.removeValue(forKey: key)
        }
    }
    
    /// Manually create a connection between chunks
    func createConnection(from chunkID1: UUID, to chunkID2: UUID, type: TerrainConnection.ConnectionType) -> Bool {
        guard let chunk1 = terrainChunks[chunkID1],
              let chunk2 = terrainChunks[chunkID2] else {
            return false
        }
        
        let connection = TerrainConnection(
            id: UUID(),
            fromChunkID: chunkID1,
            toChunkID: chunkID2,
            connectionType: type,
            strength: calculateConnectionStrength(chunk1, chunk2),
            seamData: generateSeamData(chunk1, chunk2, type: type),
            createdAt: Date(),
            lastValidated: Date()
        )
        
        addConnection(connection)
        updateConnectivityStatistics()
        
        return true
    }
    
    /// Remove a specific connection
    func removeConnection(_ connectionID: UUID) {
        connections.removeAll { $0.id == connectionID }
        
        // Update connectivity graph and cache
        if let connection = connections.first(where: { $0.id == connectionID }) {
            connectivityGraph[connection.fromChunkID]?.remove(connection.toChunkID)
            connectivityGraph[connection.toChunkID]?.remove(connection.fromChunkID)
            
            let cacheKey = ConnectionCacheKey(connection.fromChunkID, connection.toChunkID)
            connectionCache.removeValue(forKey: cacheKey)
        }
        
        updateConnectivityStatistics()
    }
    
    // MARK: - Query Operations
    
    /// Get connections for a specific chunk
    func getConnections(for chunkID: UUID) -> [TerrainConnection] {
        return connections.filter { $0.fromChunkID == chunkID || $0.toChunkID == chunkID }
    }
    
    /// Get neighbors for a chunk
    func getNeighbors(for chunkID: UUID) -> Set<UUID> {
        return connectivityGraph[chunkID] ?? Set<UUID>()
    }
    
    /// Check if two chunks are connected
    func areConnected(_ chunkID1: UUID, _ chunkID2: UUID) -> Bool {
        return connectivityGraph[chunkID1]?.contains(chunkID2) == true
    }
    
    /// Find path between chunks using connectivity graph
    func findPath(from startChunkID: UUID, to endChunkID: UUID) -> [UUID]? {
        // Simple BFS pathfinding
        var visited: Set<UUID> = [startChunkID]
        var queue: [(UUID, [UUID])] = [(startChunkID, [startChunkID])]
        
        while !queue.isEmpty {
            let (currentID, path) = queue.removeFirst()
            
            if currentID == endChunkID {
                return path
            }
            
            if let neighbors = connectivityGraph[currentID] {
                for neighbor in neighbors {
                    if !visited.contains(neighbor) {
                        visited.insert(neighbor)
                        queue.append((neighbor, path + [neighbor]))
                    }
                }
            }
        }
        
        return nil // No path found
    }
    
    /// Get all connected components
    func getConnectedComponents() -> [Set<UUID>] {
        var visited: Set<UUID> = []
        var components: [Set<UUID>] = []
        
        for chunkID in terrainChunks.keys {
            if !visited.contains(chunkID) {
                let component = exploreComponent(chunkID, visited: &visited)
                components.append(component)
            }
        }
        
        return components
    }
    
    private func exploreComponent(startID: UUID, visited: inout Set<UUID>) -> Set<UUID> {
        var component: Set<UUID> = []
        var queue: [UUID] = [startID]
        
        while !queue.isEmpty {
            let currentID = queue.removeFirst()
            
            if !visited.contains(currentID) {
                visited.insert(currentID)
                component.insert(currentID)
                
                if let neighbors = connectivityGraph[currentID] {
                    queue.append(contentsOf: neighbors)
                }
            }
        }
        
        return component
    }
    
    // MARK: - Validation and Optimization
    
    /// Validate all connections
    func validateConnections() -> [TerrainConnection] {
        var invalidConnections: [TerrainConnection] = []
        
        for connection in connections {
            if !connection.isValid {
                invalidConnections.append(connection)
            }
            
            // Check if chunks still exist
            if terrainChunks[connection.fromChunkID] == nil ||
               terrainChunks[connection.toChunkID] == nil {
                invalidConnections.append(connection)
            }
        }
        
        return invalidConnections
    }
    
    /// Optimize connectivity by removing redundant connections
    func optimizeConnectivity() {
        // Remove connections that are redundant (can reach destination through other paths)
        var connectionsToRemove: [UUID] = []
        
        for connection in connections {
            if let neighbors = connectivityGraph[connection.fromChunkID] {
                if neighbors.count > 2 {
                    // Check if destination can be reached through other neighbors
                    let otherNeighbors = neighbors.filter { $0 != connection.toChunkID }
                    
                    for otherNeighbor in otherNeighbors {
                        if let otherNeighborConnections = connectivityGraph[otherNeighbor] {
                            if otherNeighborConnections.contains(connection.toChunkID) {
                                connectionsToRemove.append(connection.id)
                                break
                            }
                        }
                    }
                }
            }
        }
        
        // Remove redundant connections
        for connectionID in connectionsToRemove {
            removeConnection(connectionID)
        }
        
        if !connectionsToRemove.isEmpty {
            print("Removed \(connectionsToRemove.count) redundant connections")
        }
    }
    
    // MARK: - Statistics and Monitoring
    
    private func updateConnectivityStatistics() {
        connectivityStatistics.totalConnections = connections.count
        
        var seamlessCount = 0
        var stitchedCount = 0
        var blendedCount = 0
        var totalStrength: Float = 0
        
        for connection in connections {
            switch connection.connectionType {
            case .seamless: seamlessCount += 1
            case .stitched: stitchedCount += 1
            case .blended: blendedCount += 1
            default: break
            }
            totalStrength += connection.strength
        }
        
        connectivityStatistics.seamlessConnections = seamlessCount
        connectivityStatistics.stitchedConnections = stitchedCount
        connectivityStatistics.blendedConnections = blendedCount
        connectivityStatistics.averageConnectionStrength = connections.isEmpty ? 0 : totalStrength / Float(connections.count)
        
        // Count isolated chunks
        connectivityStatistics.isolatedChunks = terrainChunks.values.filter { chunk in
            connectivityGraph[chunk.id]?.isEmpty ?? true
        }.count
        
        // Count connected components
        connectivityStatistics.connectedComponents = getConnectedComponents().count
        connectivityStatistics.lastUpdate = Date()
    }
    
    func getConnectivityStatistics() -> ConnectivityStatistics {
        return connectivityStatistics
    }
    
    // MARK: - Configuration
    
    func configureConnectivity(
        autoConnect: Bool,
        distance: Float,
        strength: Float,
        maxConnections: Int
    ) {
        autoConnectEnabled = autoConnect
        connectionDistance = distance
        connectionStrength = strength
        maxConnectionsPerChunk = maxConnections
        
        print("Connectivity configured: auto=\(autoConnect), distance=\(distance), maxConnections=\(maxConnections)")
    }
    
    private func setupUpdateTimer() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.periodicValidation()
        }
    }
    
    private func periodicValidation() {
        let invalidConnections = validateConnections()
        if !invalidConnections.isEmpty {
            print("Found \(invalidConnections.count) invalid connections")
        }
        
        // Periodic optimization
        if connections.count > 100 {
            optimizeConnectivity()
        }
    }
    
    // MARK: - Helper Methods (Simplified implementations)
    
    private func generateEdgeVertices(_ chunk1: TerrainChunk, _ chunk2: TerrainChunk) -> [SIMD3<Float>] {
        // Simplified edge vertex generation
        let edgeLength = min(chunk1.size.x, chunk2.size.x)
        let vertexCount = Int(edgeLength)
        
        return (0..<vertexCount).map { i in
            let t = Float(i) / Float(vertexCount - 1)
            return SIMD3<Float>(
                chunk1.position.x + t * edgeLength,
                chunk1.position.y,
                chunk1.position.z + chunk1.size.z * 0.5
            )
        }
    }
    
    private func generateSeamNormals(_ vertices: [SIMD3<Float>]) -> [SIMD3<Float>] {
        return vertices.map { _ in SIMD3<Float>(0, 1, 0) } // Simplified
    }
    
    private func generateSeamUVs(_ vertices: [SIMD3<Float>], _ chunk1: TerrainChunk, _ chunk2: TerrainChunk) -> [[SIMD2<Float>]] {
        return [[SIMD2<Float>]]() // Simplified
    }
    
    private func generateSeamIndices(_ vertices: [SIMD3<Float>]) -> [Int] {
        return Array(0..<vertices.count)
    }
    
    private func generateLODTransitionVertices(_ chunk1: TerrainChunk, _ chunk2: TerrainChunk) -> [SIMD3<Float>] {
        return generateEdgeVertices(chunk1, chunk2) // Simplified
    }
    
    private func generateLODTransitionNormals(_ vertices: [SIMD3<Float>]) -> [SIMD3<Float>] {
        return vertices.map { _ in SIMD3<Float>(0, 1, 0) } // Simplified
    }
    
    private func generateLODTransitionIndices(_ vertices: [SIMD3<Float>]) -> [Int] {
        return Array(0..<vertices.count)
    }
}
