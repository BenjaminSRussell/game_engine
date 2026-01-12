import SwiftUI

// MARK: - Terrain Runtime API

/// Terrain Runtime API provides comprehensive runtime modification capabilities for terrain systems
@MainActor
class TerrainRuntimeAPI: ObservableObject {
    static let shared = TerrainRuntimeAPI()
    
    @Published var isInitialized: Bool = false
    @Published var activeModifications: [TerrainModification] = []
    @Published var modificationHistory: [TerrainModification] = []
    @Published var apiStatistics: APIStatistics
    
    private var terrainData: TerrainData?
    private var modificationQueue: DispatchQueue
    private var maxHistorySize: Int = 1000
    
    struct APIStatistics {
        var totalModifications: Int = 0
        var successfulModifications: Int = 0
        var failedModifications: Int = 0
        var averageExecutionTime: Double = 0.0
        var memoryUsage: Int64 = 0
        var lastModificationTime: Date?
    }
    
    private init() {
        self.modificationQueue = DispatchQueue(label: "terrain.modification", qos: .userInteractive)
        self.apiStatistics = APIStatistics()
    }
    
    // MARK: - Initialization
    
    /// Initialize the terrain API with terrain data
    func initialize(with terrainData: TerrainData) throws {
        guard !isInitialized else {
            throw TerrainAPIError.alreadyInitialized
        }
        
        self.terrainData = terrainData
        self.isInitialized = true
        
        print("Terrain Runtime API initialized with terrain size: \(terrainData.size)")
    }
    
    /// Shutdown the terrain API and cleanup resources
    func shutdown() {
        guard isInitialized else { return }
        
        activeModifications.removeAll()
        terrainData = nil
        isInitialized = false
        
        print("Terrain Runtime API shutdown complete")
    }
    
    // MARK: - Height Modification
    
    /// Modify terrain height at a specific position
    func modifyHeight(at position: SIMD2<Float>, delta: Float, radius: Float = 1.0, operation: HeightOperation = .add) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .heightModification,
            position: position,
            parameters: [
                "delta": delta,
                "radius": radius,
                "operation": operation.rawValue
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    /// Modify terrain height in a rectangular region
    func modifyHeightRegion(rect: CGRect, delta: Float, operation: HeightOperation = .add) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .heightRegionModification,
            position: SIMD2<Float>(Float(rect.midX), Float(rect.midY)),
            parameters: [
                "rect": NSStringFromCGRect(rect),
                "delta": delta,
                "operation": operation.rawValue
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    /// Set terrain height at a specific position
    func setHeight(at position: SIMD2<Float>, height: Float, radius: Float = 1.0) async throws -> TerrainModificationResult {
        return try await modifyHeight(at: position, delta: height, radius: radius, operation: .set)
    }
    
    // MARK: - Texture Modification
    
    /// Paint texture at a specific position
    func paintTexture(at position: SIMD2<Float>, textureIndex: Int, radius: Float = 1.0, strength: Float = 1.0) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .texturePaint,
            position: position,
            parameters: [
                "textureIndex": textureIndex,
                "radius": radius,
                "strength": strength
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    /// Paint texture in a rectangular region
    func paintTextureRegion(rect: CGRect, textureIndex: Int, strength: Float = 1.0) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .textureRegionPaint,
            position: SIMD2<Float>(Float(rect.midX), Float(rect.midY)),
            parameters: [
                "rect": NSStringFromCGRect(rect),
                "textureIndex": textureIndex,
                "strength": strength
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    // MARK: - Vegetation Modification
    
    /// Add vegetation at a specific position
    func addVegetation(at position: SIMD2<Float>, type: VegetationType, density: Float = 1.0) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .vegetationAdd,
            position: position,
            parameters: [
                "vegetationType": type.rawValue,
                "density": density
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    /// Remove vegetation in a circular region
    func removeVegetation(at position: SIMD2<Float>, radius: Float) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .vegetationRemove,
            position: position,
            parameters: [
                "radius": radius
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    // MARK: - Erosion Simulation
    
    /// Apply hydraulic erosion to a region
    func applyHydraulicErosion(at position: SIMD2<Float>, intensity: Float, iterations: Int = 10) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .erosionHydraulic,
            position: position,
            parameters: [
                "intensity": intensity,
                "iterations": iterations
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    /// Apply thermal erosion to a region
    func applyThermalErosion(at position: SIMD2<Float>, intensity: Float, iterations: Int = 10) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .erosionThermal,
            position: position,
            parameters: [
                "intensity": intensity,
                "iterations": iterations
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    // MARK: - Query Operations
    
    /// Get terrain height at a specific position
    func getHeight(at position: SIMD2<Float>) -> Float? {
        guard isInitialized, let terrainData = terrainData else {
            return nil
        }
        
        // Convert world coordinates to heightmap coordinates
        let x = Int(position.x)
        let y = Int(position.y)
        
        guard x >= 0 && x < terrainData.size.x && y >= 0 && y < terrainData.size.y else {
            return nil
        }
        
        return terrainData.heightmap[y][x]
    }
    
    /// Get texture at a specific position
    func getTexture(at position: SIMD2<Float>) -> Int? {
        guard isInitialized, let terrainData = terrainData else {
            return nil
        }
        
        let x = Int(position.x)
        let y = Int(position.y)
        
        guard x >= 0 && x < terrainData.size.x && y >= 0 && y < terrainData.size.y else {
            return nil
        }
        
        return terrainData.textureMap[y][x]
    }
    
    /// Get terrain normal at a specific position
    func getNormal(at position: SIMD2<Float>) -> SIMD3<Float>? {
        guard isInitialized else { return nil }
        
        // Calculate normal using height differences
        let heightL = getHeight(at: position + SIMD2<Float>(-1, 0)) ?? 0
        let heightR = getHeight(at: position + SIMD2<Float>(1, 0)) ?? 0
        let heightU = getHeight(at: position + SIMD2<Float>(0, -1)) ?? 0
        let heightD = getHeight(at: position + SIMD2<Float>(0, 1)) ?? 0
        
        let normal = SIMD3<Float>(
            heightL - heightR,
            2.0,
            heightU - heightD
        )
        
        return normalize(normal)
    }
    
    /// Get terrain slope at a specific position
    func getSlope(at position: SIMD2<Float>) -> Float? {
        guard let normal = getNormal(at: position) else { return nil }
        
        // Slope is the angle between normal and up vector
        let upVector = SIMD3<Float>(0, 1, 0)
        let dotProduct = dot(normal, upVector)
        let angle = acos(max(-1, min(1, dotProduct)))
        
        return angle
    }
    
    // MARK: - Batch Operations
    
    /// Execute multiple modifications as a single batch
    func executeBatch(_ modifications: [TerrainModification]) async throws -> [TerrainModificationResult] {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        var results: [TerrainModificationResult] = []
        
        for modification in modifications {
            let result = await executeModification(modification)
            results.append(result)
        }
        
        return results
    }
    
    /// Apply a heightmap to the terrain
    func applyHeightmap(_ heightmap: [[Float]], offset: SIMD2<Float> = .zero, scale: Float = 1.0) async throws -> TerrainModificationResult {
        guard isInitialized else {
            throw TerrainAPIError.notInitialized
        }
        
        let modification = TerrainModification(
            id: UUID(),
            type: .heightmapApply,
            position: offset,
            parameters: [
                "heightmapData": heightmap,
                "scale": scale
            ],
            timestamp: Date()
        )
        
        return await executeModification(modification)
    }
    
    // MARK: - History Management
    
    /// Undo the last modification
    func undo() async throws -> TerrainModificationResult? {
        guard let lastModification = modificationHistory.last else {
            return nil
        }
        
        let undoModification = TerrainModification(
            id: UUID(),
            type: .undo,
            position: lastModification.position,
            parameters: ["originalModification": lastModification],
            timestamp: Date()
        )
        
        _ = modificationHistory.popLast()
        return await executeModification(undoModification)
    }
    
    /// Redo the last undone modification
    func redo() async throws -> TerrainModificationResult? {
        // Implementation depends on how undo history is managed
        return nil
    }
    
    /// Clear modification history
    func clearHistory() {
        modificationHistory.removeAll()
    }
    
    // MARK: - Private Methods
    
    private func executeModification(_ modification: TerrainModification) async -> TerrainModificationResult {
        let startTime = CFAbsoluteTimeGetCurrent()
        
        activeModifications.append(modification)
        defer {
            activeModifications.removeAll { $0.id == modification.id }
        }
        
        do {
            // Simulate modification execution
            try await Task.sleep(nanoseconds: 10_000_000) // 10ms
            
            // Update statistics
            let executionTime = CFAbsoluteTimeGetCurrent() - startTime
            updateStatistics(success: true, executionTime: executionTime)
            
            // Add to history
            modificationHistory.append(modification)
            if modificationHistory.count > maxHistorySize {
                modificationHistory.removeFirst()
            }
            
            return TerrainModificationResult(
                modification: modification,
                success: true,
                executionTime: executionTime,
                affectedArea: calculateAffectedArea(for: modification),
                message: "Modification completed successfully"
            )
            
        } catch {
            updateStatistics(success: false, executionTime: CFAbsoluteTimeGetCurrent() - startTime)
            
            return TerrainModificationResult(
                modification: modification,
                success: false,
                executionTime: CFAbsoluteTimeGetCurrent() - startTime,
                affectedArea: .zero,
                message: "Modification failed: \(error.localizedDescription)"
            )
        }
    }
    
    private func updateStatistics(success: Bool, executionTime: Double) {
        apiStatistics.totalModifications += 1
        
        if success {
            apiStatistics.successfulModifications += 1
        } else {
            apiStatistics.failedModifications += 1
        }
        
        // Update average execution time
        let totalTime = apiStatistics.averageExecutionTime * Double(apiStatistics.totalModifications - 1) + executionTime
        apiStatistics.averageExecutionTime = totalTime / Double(apiStatistics.totalModifications)
        
        apiStatistics.lastModificationTime = Date()
    }
    
    private func calculateAffectedArea(for modification: TerrainModification) -> CGRect {
        // Calculate affected area based on modification type and parameters
        switch modification.type {
        case .heightModification, .texturePaint, .vegetationAdd, .vegetationRemove:
            let radius = modification.parameters["radius"] as? Float ?? 1.0
            return CGRect(
                x: modification.position.x - radius,
                y: modification.position.y - radius,
                width: radius * 2,
                height: radius * 2
            )
            
        case .heightRegionModification, .textureRegionPaint:
            if let rectString = modification.parameters["rect"] as? String {
                return CGRectFromString(rectString)
            }
            return .zero
            
        default:
            return .zero
        }
    }
}

// MARK: - Data Structures

struct TerrainModification: Identifiable, Codable {
    let id: UUID
    let type: ModificationType
    let position: SIMD2<Float>
    let parameters: [String: Any]
    let timestamp: Date
    
    enum ModificationType: String, Codable, CaseIterable {
        case heightModification = "height_modification"
        case heightRegionModification = "height_region_modification"
        case texturePaint = "texture_paint"
        case textureRegionPaint = "texture_region_paint"
        case vegetationAdd = "vegetation_add"
        case vegetationRemove = "vegetation_remove"
        case erosionHydraulic = "erosion_hydraulic"
        case erosionThermal = "erosion_thermal"
        case heightmapApply = "heightmap_apply"
        case undo = "undo"
    }
    
    // Custom coding for parameters dictionary
    enum CodingKeys: CodingKey {
        case id, type, position, parameters, timestamp
    }
    
    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        id = try container.decode(UUID.self, forKey: .id)
        type = try container.decode(ModificationType.self, forKey: .type)
        position = try container.decode(SIMD2<Float>.self, forKey: .position)
        timestamp = try container.decode(Date.self, forKey: .parameters)
        
        // For simplicity, we'll decode parameters as a JSON string
        let parametersString = try container.decode(String.self, forKey: .parameters)
        parameters = [:] // Simplified for this example
    }
    
    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(id, forKey: .id)
        try container.encode(type, forKey: .type)
        try container.encode(position, forKey: .position)
        try container.encode(timestamp, forKey: .timestamp)
        // Simplified encoding for parameters
        try container.encode("{}", forKey: .parameters)
    }
}

struct TerrainModificationResult {
    let modification: TerrainModification
    let success: Bool
    let executionTime: Double
    let affectedArea: CGRect
    let message: String
}

struct TerrainData {
    let size: SIMD2<Int>
    var heightmap: [[Float]]
    var textureMap: [[Int]]
    var vegetationMap: [[VegetationInstance]]
    
    init(size: SIMD2<Int>) {
        self.size = size
        self.heightmap = Array(repeating: Array(repeating: 0.0, count: size.x), count: size.y)
        self.textureMap = Array(repeating: Array(repeating: 0, count: size.x), count: size.y)
        self.vegetationMap = Array(repeating: Array(repeating: [], count: size.x), count: size.y)
    }
}

struct VegetationInstance: Codable {
    let id: UUID
    let type: VegetationType
    let position: SIMD3<Float>
    let rotation: simd_quatf
    let scale: Float
}

// MARK: - Enums and Types

enum HeightOperation: String, CaseIterable {
    case add = "add"
    case subtract = "subtract"
    case multiply = "multiply"
    case set = "set"
    case min = "min"
    case max = "max"
}

enum VegetationType: String, CaseIterable, Codable {
    case oak = "oak"
    case pine = "pine"
    case birch = "birch"
    case bush = "bush"
    case grass = "grass"
    case flower = "flower"
}

enum TerrainAPIError: LocalizedError {
    case notInitialized
    case alreadyInitialized
    case invalidPosition
    case invalidParameters
    case operationFailed(String)
    
    var errorDescription: String? {
        switch self {
        case .notInitialized:
            return "Terrain API is not initialized"
        case .alreadyInitialized:
            return "Terrain API is already initialized"
        case .invalidPosition:
            return "Invalid position specified"
        case .invalidParameters:
            return "Invalid parameters provided"
        case .operationFailed(let message):
            return "Operation failed: \(message)"
        }
    }
}

// MARK: - Helper Functions

private func NSStringFromCGRect(_ rect: CGRect) -> String {
    return "\(rect.origin.x),\(rect.origin.y),\(rect.size.width),\(rect.size.height)"
}

private func CGRectFromString(_ string: String) -> CGRect {
    let components = string.split(separator: ",").map { Float($0) ?? 0 }
    guard components.count == 4 else { return .zero }
    
    return CGRect(
        x: CGFloat(components[0]),
        y: CGFloat(components[1]),
        width: CGFloat(components[2]),
        height: CGFloat(components[3])
    )
}

// MARK: - SIMD2 Codable Extension

extension SIMD2<Float>: Codable {
    public init(from decoder: Decoder) throws {
        var container = try decoder.unkeyedContainer()
        let values = try container.decode([Float].self)
        guard values.count >= 2 else {
            throw DecodingError.dataCorrupted(DecodingError.Context(codingPath: decoder.codingPath, debugDescription: "Expected 2 values for SIMD2"))
        }
        self.init(values[0], values[1])
    }
    
    public func encode(to encoder: Encoder) throws {
        var container = encoder.unkeyedContainer()
        try container.encode([self.x, self.y])
    }
}
