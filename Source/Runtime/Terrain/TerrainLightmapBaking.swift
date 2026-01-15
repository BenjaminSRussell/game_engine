import Foundation
import simd

// MARK: - Terrain Lightmap Baking System

/// Terrain lightmap baking system provides lightmap generation and baking for terrain
@MainActor
class TerrainLightmapBaking: ObservableObject {
    static let shared = TerrainLightmapBaking()
    
    @Published var bakingEnabled: Bool = true
    @Published var bakingInProgress: Bool = false
    @Published var lightmapResolution: Int = 512
    @Published var lightmapQuality: LightmapQuality = .high
    @Published var bakingProgress: Float = 0.0
    @Published var currentBakeOperation: BakeOperation?
    @Published var bakingStatistics: BakingStatistics
    @Published var bakedLightmaps: [BakedLightmap] = []
    
    private var bakingQueue: DispatchQueue
    private var lightmapCache: [LightmapCacheKey: BakedLightmap] = [:]
    private var bakingTimer: Timer?
    
    struct BakeOperation: Identifiable, Codable {
        let id: UUID
        let terrainRegion: TerrainRegion
        let lightSettings: LightSettings
        let bakeType: BakeType
        let priority: Priority
        let status: Status
        let createdAt: Date
        let startedAt: Date?
        let completedAt: Date?
        let result: BakeResult?
        
        enum BakeType: String, Codable, CaseIterable {
            case direct = "direct"
            case indirect = "indirect"
            case ambient = "ambient"
            case radiosity = "radiosity"
            case globalIllumination = "global_illumination"
        }
        
        enum Priority: Int, Codable, CaseIterable {
            case critical = 0
            case high = 1
            case medium = 2
            case low = 3
        }
        
        enum Status: String, Codable, CaseIterable {
            case pending = "pending"
            case inProgress = "in_progress"
            case completed = "completed"
            case failed = "failed"
            case cancelled = "cancelled"
        }
    }
    
    struct LightSettings: Codable {
        let sunDirection: SIMD3<Float>
        let sunColor: SIMD3<Float>
        let sunIntensity: Float
        let ambientColor: SIMD3<Float>
        let ambientIntensity: Float
        let skyColor: SIMD3<Float>
        let indirectIntensity: Float
        let bounceCount: Int
        let sampleCount: Int
        let lightmapFormat: LightmapFormat
        
        enum LightmapFormat: String, Codable, CaseIterable {
            case rgb8 = "rgb8"
            case rgb16 = "rgb16"
            case rgbf = "rgbf"
            case rgbe = "rgbe"
            case hdr = "hdr"
        }
        
        static let defaultSettings = LightSettings(
            sunDirection: SIMD3<Float>(0.3, 0.7, 0.2),
            sunColor: SIMD3<Float>(1.0, 0.95, 0.8),
            sunIntensity: 1.2,
            ambientColor: SIMD3<Float>(0.4, 0.5, 0.6),
            ambientIntensity: 0.3,
            skyColor: SIMD3<Float>(0.6, 0.8, 1.0),
            indirectIntensity: 0.5,
            bounceCount: 3,
            sampleCount: 64,
            lightmapFormat: .rgbf
        )
    }
    
    struct TerrainRegion: Codable {
        let origin: SIMD3<Float>
        let size: SIMD3<Float>
        let heightmap: [[Float]]
        let normals: [[SIMD3<Float>]]
        let materials: [[Int]]
        
        var bounds: BoundingBox {
            return BoundingBox(min: origin, max: origin + size)
        }
    }
    
    struct BakedLightmap: Identifiable, Codable {
        let id: UUID
        let region: TerrainRegion
        let lightmapData: Data
        let width: Int
        let height: Int
        let format: LightSettings.LightmapFormat
        let bakeType: BakeOperation.BakeType
        let bakeSettings: LightSettings
        let createdAt: Date
        let fileSize: Int64
        
        var memoryUsage: Int64 {
            return Int64(width * height * 4) // 4 bytes per pixel for RGB
        }
    }
    
    struct BakeResult: Codable {
        let success: Bool
        let lightmapData: Data?
        let error: String?
        let bakingTime: Double
        let samplesProcessed: Int
        let memoryUsage: Int64
        
        static let failure = BakeResult(
            success: false,
            lightmapData: nil,
            error: "Baking failed",
            bakingTime: 0,
            samplesProcessed: 0,
            memoryUsage: 0
        )
    }
    
    struct LightmapCacheKey: Hashable {
        let regionHash: Int
        let settingsHash: Int
        let bakeType: String
        
        init(region: TerrainRegion, settings: LightSettings, bakeType: BakeOperation.BakeType) {
            self.regionHash = region.hashValue
            self.settingsHash = settings.hashValue
            self.bakeType = bakeType.rawValue
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
    }
    
    enum LightmapQuality: Int, CaseIterable {
        case low = 0
        case medium = 1
        case high = 2
        case ultra = 3
        
        var sampleMultiplier: Int {
            switch self {
            case .low: return 16
            case .medium: return 32
            case .high: return 64
            case .ultra: return 128
            }
        }
        
        var bounceCount: Int {
            switch self {
            case .low: return 1
            case .medium: return 2
            case .high: return 3
            case .ultra: return 4
            }
        }
    }
    
    struct BakingStatistics {
        var totalBakes: Int = 0
        var completedBakes: Int = 0
        var failedBakes: Int = 0
        var averageBakingTime: Double = 0.0
        var totalSamplesProcessed: Int = 0
        var totalMemoryUsage: Int64 = 0
        var lastUpdate: Date = Date()
        
        var successRate: Float {
            guard totalBakes > 0 else { return 0.0 }
            return Float(completedBakes) / Float(totalBakes)
        }
        
        var memoryUsageMB: Double {
            return Double(totalMemoryUsage) / (1024 * 1024)
        }
    }
    
    private init() {
        bakingQueue = DispatchQueue(label: "terrain.lightmap.baking", qos: .userInteractive)
        bakingStatistics = BakingStatistics()
        setupUpdateTimer()
    }
    
    deinit {
        bakingTimer?.invalidate()
    }
    
    // MARK: - Lightmap Baking Operations
    
    /// Bake lightmap for a terrain region
    func bakeLightmap(
        for region: TerrainRegion,
        settings: LightSettings = .defaultSettings,
        bakeType: BakeOperation.BakeType = .direct
    ) -> UUID {
        let operation = BakeOperation(
            id: UUID(),
            terrainRegion: region,
            lightSettings: settings,
            bakeType: bakeType,
            priority: .medium,
            status: .pending,
            createdAt: Date(),
            startedAt: nil,
            completedAt: nil,
            result: nil
        )
        
        addBakeOperation(operation)
        return operation.id
    }
    
    /// Bake multiple lightmaps for different regions
    func bakeMultipleLightmaps(
        for regions: [TerrainRegion],
        settings: LightSettings = .defaultSettings,
        bakeType: BakeOperation.BakeType = .direct
    ) -> [UUID] {
        return regions.map { region in
            bakeLightmap(for: region, settings: settings, bakeType: bakeType)
        }
    }
    
    /// Bake global illumination for entire terrain
    func bakeGlobalIllumination(
        for regions: [TerrainRegion],
        settings: LightSettings = .defaultSettings
    ) -> [UUID] {
        return regions.map { region in
            let operation = BakeOperation(
                id: UUID(),
                terrainRegion: region,
                lightSettings: settings,
                bakeType: .globalIllumination,
                priority: .high,
                status: .pending,
                createdAt: Date(),
                startedAt: nil,
                completedAt: nil,
                result: nil
            )
            
            addBakeOperation(operation)
            return operation.id
        }
    }
    
    private func addBakeOperation(_ operation: BakeOperation) {
        // Check cache first
        let cacheKey = LightmapCacheKey(
            region: operation.terrainRegion,
            settings: operation.lightSettings,
            bakeType: operation.bakeType
        )
        
        if let cachedLightmap = lightmapCache[cacheKey] {
            // Use cached result
            let completedOperation = BakeOperation(
                id: operation.id,
                terrainRegion: operation.terrainRegion,
                lightSettings: operation.lightSettings,
                bakeType: operation.bakeType,
                priority: operation.priority,
                status: .completed,
                createdAt: operation.createdAt,
                startedAt: Date(),
                completedAt: Date(),
                result: BakeResult(
                    success: true,
                    lightmapData: cachedLightmap.lightmapData,
                    error: nil,
                    bakingTime: 0.001,
                    samplesProcessed: 0,
                    memoryUsage: cachedLightmap.memoryUsage
                )
            )
            
            if let index = currentBakeOperation?.id == operation.id ? nil : bakingOperations.firstIndex(where: { $0.id == operation.id }) {
                bakingOperations[index ?? bakingOperations.count] = completedOperation
            }
            
            bakedLightmaps.append(cachedLightmap)
            updateBakingStatistics()
            return
        }
        
        bakingOperations.append(operation)
        
        if bakingEnabled {
            executeBakeOperation(operation)
        }
    }
    
    private func executeBakeOperation(_ operation: BakeOperation) {
        bakingQueue.async { [weak self] in
            let startTime = Date()
            
            // Update status
            DispatchQueue.main.async {
                self?.bakingInProgress = true
                self?.currentBakeOperation = operation
                
                var mutableOperation = operation
                mutableOperation.status = .inProgress
                mutableOperation.startedAt = Date()
                
                if let index = self?.bakingOperations.firstIndex(where: { $0.id == operation.id }) {
                    self?.bakingOperations[index] = mutableOperation
                }
            }
            
            do {
                let result = try self?.performBaking(operation) ?? BakeResult.failure
                
                DispatchQueue.main.async {
                    self?.completeBakeOperation(operation, result: result, bakingTime: Date().timeIntervalSince(startTime))
                }
                
            } catch {
                DispatchQueue.main.async {
                    self?.failBakeOperation(operation, error: error.localizedDescription)
                }
            }
        }
    }
    
    private func performBaking(_ operation: BakeOperation) throws -> BakeResult {
        switch operation.bakeType {
        case .direct:
            return try performDirectBaking(operation)
        case .indirect:
            return try performIndirectBaking(operation)
        case .ambient:
            return try performAmbientBaking(operation)
        case .radiosity:
            return try performRadiosityBaking(operation)
        case .globalIllumination:
            return try performGlobalIlluminationBaking(operation)
        }
    }
    
    private func performDirectBaking(_ operation: BakeOperation) throws -> BakeResult {
        let startTime = Date()
        let region = operation.terrainRegion
        let settings = operation.lightSettings
        
        // Generate direct lighting
        let lightmapData = try generateDirectLightmap(region, settings)
        
        let bakingTime = Date().timeIntervalSince(startTime)
        let samplesProcessed = region.heightmap.count * region.heightmap[0].count
        
        return BakeResult(
            success: true,
            lightmapData: lightmapData,
            error: nil,
            bakingTime: bakingTime,
            samplesProcessed: samplesProcessed,
            memoryUsage: Int64(lightmapData.count)
        )
    }
    
    private func performIndirectBaking(_ operation: BakeOperation) throws -> BakeResult {
        // Generate indirect lighting
        let startTime = Date()
        let region = operation.terrainRegion
        let settings = operation.lightSettings
        
        let lightmapData = try generateIndirectLightmap(region, settings)
        
        let bakingTime = Date().timeIntervalSince(startTime)
        let samplesProcessed = region.heightmap.count * region.heightmap[0].count
        
        return BakeResult(
            success: true,
            lightmapData: lightmapData,
            error: nil,
            bakingTime: bakingTime,
            samplesProcessed: samplesProcessed,
            memoryUsage: Int64(lightmapData.count)
        )
    }
    
    private func performAmbientBaking(_ operation: BakeOperation) throws -> BakeResult {
        // Generate ambient occlusion
        let startTime = Date()
        let region = operation.terrainRegion
        let settings = operation.lightSettings
        
        let lightmapData = try generateAmbientOcclusion(region, settings)
        
        let bakingTime = Date().timeIntervalSince(startTime)
        let samplesProcessed = region.heightmap.count * region.heightmap[0].count
        
        return BakeResult(
            success: true,
            lightmapData: lightmapData,
            error: nil,
            bakingTime: bakingTime,
            samplesProcessed: samplesProcessed,
            memoryUsage: Int64(lightmapData.count)
        )
    }
    
    private func performRadiosityBaking(_ operation: BakeOperation) throws -> BakeResult {
        // Generate radiosity solution
        let startTime = Date()
        let region = operation.terrainRegion
        let settings = operation.lightSettings
        
        let lightmapData = try generateRadiositySolution(region, settings)
        
        let bakingTime = Date().timeIntervalSince(startTime)
        let samplesProcessed = region.heightmap.count * region.heightmap[0].count
        
        return BakeResult(
            success: true,
            lightmapData: lightmapData,
            error: nil,
            bakingTime: bakingTime,
            samplesProcessed: samplesProcessed,
            memoryUsage: Int64(lightmapData.count)
        )
    }
    
    private func performGlobalIlluminationBaking(_ operation: BakeOperation) throws -> BakeResult {
        // Generate global illumination (combination of all lighting types)
        let startTime = Date()
        let region = operation.terrainRegion
        let settings = operation.lightSettings
        
        let lightmapData = try generateGlobalIllumination(region, settings)
        
        let bakingTime = Date().timeIntervalSince(startTime)
        let samplesProcessed = region.heightmap.count * region.heightmap[0].count
        
        return BakeResult(
            success: true,
            lightmapData: lightmapData,
            error: nil,
            bakingTime: bakingTime,
            samplesProcessed: samplesProcessed,
            memoryUsage: Int64(lightmapData.count)
        )
    }
    
    // MARK: - Lightmap Generation
    
    private func generateDirectLightmap(_ region: TerrainRegion, _ settings: LightSettings) throws -> Data {
        let width = lightmapResolution
        let height = lightmapResolution
        var lightmapData: [SIMD3<Float>] = []
        
        for y in 0..<height {
            for x in 0..<width {
                let worldPos = region.origin + SIMD3<Float>(
                    Float(x) * region.size.x / Float(width),
                    0,
                    Float(y) * region.size.z / Float(height)
                )
                
                let lighting = calculateDirectLighting(worldPos, region, settings)
                lightmapData.append(lighting)
            }
        }
        
        return encodeLightmapData(lightmapData, width: width, height: height, format: settings.lightmapFormat)
    }
    
    private func generateIndirectLightmap(_ region: TerrainRegion, _ settings: LightSettings) throws -> Data {
        // Simplified indirect lighting generation
        let width = lightmapResolution
        let height = lightmapResolution
        var lightmapData: [SIMD3<Float>] = []
        
        for y in 0..<height {
            for x in 0..<width {
                let worldPos = region.origin + SIMD3<Float>(
                    Float(x) * region.size.x / Float(width),
                    0,
                    Float(y) * region.size.z / Float(height)
                )
                
                let lighting = calculateIndirectLighting(worldPos, region, settings)
                lightmapData.append(lighting)
            }
        }
        
        return encodeLightmapData(lightmapData, width: width, height: height, format: settings.lightmapFormat)
    }
    
    private func generateAmbientOcclusion(_ region: TerrainRegion, _ settings: LightSettings) throws -> Data {
        // Generate ambient occlusion map
        let width = lightmapResolution
        let height = lightmapResolution
        var occlusionData: [Float] = []
        
        for y in 0..<height {
            for x in 0..<width {
                let worldPos = region.origin + SIMD3<Float>(
                    Float(x) * region.size.x / Float(width),
                    0,
                    Float(y) * region.size.z / Float(height)
                )
                
                let occlusion = calculateAmbientOcclusion(worldPos, region, settings)
                occlusionData.append(occlusion)
            }
        }
        
        // Convert to RGB format
        let rgbData = occlusionData.map { value in
            SIMD3<Float>(value, value, value)
        }
        
        return encodeLightmapData(rgbData, width: width, height: height, format: settings.lightmapFormat)
    }
    
    private func generateRadiositySolution(_ region: TerrainRegion, _ settings: LightSettings) throws -> Data {
        // Generate radiosity solution
        let width = lightmapResolution
        let height = lightmapResolution
        var radiosityData: [SIMD3<Float>] = []
        
        for y in 0..<height {
            for x in 0..<width {
                let worldPos = region.origin + SIMD3<Float>(
                    Float(x) * region.size.x / Float(width),
                    0,
                    Float(y) * region.size.z / Float(height)
                )
                
                let radiosity = calculateRadiosity(worldPos, region, settings)
                radiosityData.append(radiosity)
            }
        }
        
        return encodeLightmapData(radiosityData, width: width, height: height, format: settings.lightmapFormat)
    }
    
    private func generateGlobalIllumination(_ region: TerrainRegion, _ settings: LightSettings) throws -> Data {
        // Generate global illumination (combination of all lighting)
        let width = lightmapResolution
        let height = lightmapResolution
        var giData: [SIMD3<Float>] = []
        
        for y in 0..<height {
            for x in 0..<width {
                let worldPos = region.origin + SIMD3<Float>(
                    Float(x) * region.size.x / Float(width),
                    0,
                    Float(y) * region.size.z / Float(height)
                )
                
                let direct = calculateDirectLighting(worldPos, region, settings)
                let indirect = calculateIndirectLighting(worldPos, region, settings)
                let ambient = calculateAmbientLighting(worldPos, region, settings)
                
                let gi = direct + indirect * settings.indirectIntensity + ambient * settings.ambientIntensity
                giData.append(gi)
            }
        }
        
        return encodeLightmapData(giData, width: width, height: height, format: settings.lightmapFormat)
    }
    
    // MARK: - Lighting Calculations
    
    private func calculateDirectLighting(_ position: SIMD3<Float>, _ region: TerrainRegion, _ settings: LightSettings) -> SIMD3<Float> {
        // Sample height and normal at position
        let height = sampleHeight(region, at: SIMD2<Float>(position.x, position.z))
        let normal = sampleNormal(region, at: SIMD2<Float>(position.x, position.z))
        
        // Calculate direct lighting
        let ndotl = max(0, dot(normal, settings.sunDirection))
        let directLight = settings.sunColor * settings.sunIntensity * ndotl
        
        // Add ambient
        let ambientLight = settings.ambientColor * settings.ambientIntensity
        
        return directLight + ambientLight
    }
    
    private func calculateIndirectLighting(_ position: SIMD3<Float>, _ region: TerrainRegion, _ settings: LightSettings) -> SIMD3<Float> {
        // Simplified indirect lighting calculation
        let normal = sampleNormal(region, at: SIMD2<Float>(position.x, position.z))
        
        // Sample surrounding points for indirect lighting
        let indirectLight = settings.skyColor * settings.indirectIntensity * (normal.y * 0.5 + 0.5)
        
        return indirectLight
    }
    
    private func calculateAmbientLighting(_ position: SIMD3<Float>, _ region: TerrainRegion, _ settings: LightSettings) -> SIMD3<Float> {
        // Calculate ambient lighting
        return settings.ambientColor * settings.ambientIntensity
    }
    
    private func calculateAmbientOcclusion(_ position: SIMD3<Float>, _ region: TerrainRegion, _ settings: LightSettings) -> Float {
        // Simplified ambient occlusion
        let sampleRadius: Float = 5.0
        var occlusion: Float = 0
        var sampleCount: Int = 0
        
        // Sample surrounding points
        for dx in -2...2 {
            for dz in -2...2 {
                if dx == 0 && dz == 0 { continue }
                
                let samplePos = position + SIMD3<Float>(Float(dx) * sampleRadius, 0, Float(dz) * sampleRadius)
                let sampleHeight = sampleHeight(region, at: SIMD2<Float>(samplePos.x, samplePos.z))
                let currentHeight = sampleHeight(region, at: SIMD2<Float>(position.x, position.z))
                
                if sampleHeight > currentHeight + 1.0 {
                    occlusion += 1.0
                }
                sampleCount += 1
            }
        }
        
        return sampleCount > 0 ? 1.0 - (occlusion / Float(sampleCount)) : 1.0
    }
    
    private func calculateRadiosity(_ position: SIMD3<Float>, _ region: TerrainRegion, _ settings: LightSettings) -> SIMD3<Float> {
        // Simplified radiosity calculation
        let normal = sampleNormal(region, at: SIMD2<Float>(position.x, position.z))
        
        // Calculate form factor
        let formFactor = max(0.1, normal.y)
        
        return settings.sunColor * settings.sunIntensity * formFactor
    }
    
    // MARK: - Utility Methods
    
    private func sampleHeight(_ region: TerrainRegion, at position: SIMD2<Float>) -> Float {
        let x = Int(position.x)
        let z = Int(position.y)
        
        guard x >= 0 && x < region.heightmap[0].count &&
              z >= 0 && z < region.heightmap.count else {
            return 0
        }
        
        return region.heightmap[z][x]
    }
    
    private func sampleNormal(_ region: TerrainRegion, at position: SIMD2<Float>) -> SIMD3<Float> {
        // Calculate normal using height differences
        let heightL = sampleHeight(region, at: position - SIMD2<Float>(1, 0))
        let heightR = sampleHeight(region, at: position + SIMD2<Float>(1, 0))
        let heightD = sampleHeight(region, at: position - SIMD2<Float>(0, 1))
        let heightU = sampleHeight(region, at: position + SIMD2<Float>(0, 1))
        
        let normal = normalize(SIMD3<Float>(heightL - heightR, 2.0, heightD - heightU))
        return normal
    }
    
    private func encodeLightmapData(_ data: [SIMD3<Float>], width: Int, height: Int, format: LightSettings.LightmapFormat) throws -> Data {
        switch format {
        case .rgb8:
            let rgbData = data.flatMap { vec in
                [UInt8(min(255, max(0, vec.x * 255))),
                 UInt8(min(255, max(0, vec.y * 255))),
                 UInt8(min(255, max(0, vec.z * 255)))]
            }
            return Data(rgbData)
            
        case .rgb16:
            let rgbData = data.flatMap { vec in
                [UInt16(min(65535, max(0, vec.x * 65535))),
                 UInt16(min(65535, max(0, vec.y * 65535))),
                 UInt16(min(65535, max(0, vec.z * 65535)))]
            }
            return Data(rgbData)
            
        case .rgbf:
            let floatData = data.flatMap { vec in
                [vec.x, vec.y, vec.z].map { Float($0) }
            }
            return Data(bytes: floatData, count: floatData.count * 4)
            
        case .rgbe:
            // RGBE encoding (simplified)
            let rgbeData = data.flatMap { vec in
                encodeRGBE(vec)
            }
            return Data(rgbeData)
            
        case .hdr:
            // HDR format (simplified)
            let hdrData = data.flatMap { vec in
                [Float($0.x), Float($0.y), Float($0.z)].map { Float($0) }
            }
            return Data(bytes: hdrData, count: hdrData.count * 4)
        }
    }
    
    private func encodeRGBE(_ color: SIMD3<Float>) -> [UInt8] {
        // Simplified RGBE encoding
        let sharedExp = UInt8(128) // Fixed exponent for simplicity
        let r = UInt8(min(255, max(0, color.x * 255)))
        let g = UInt8(min(255, max(0, color.y * 255)))
        let b = UInt8(min(255, max(0, color.z * 255)))
        
        return [r, g, b, sharedExp]
    }
    
    // MARK: - Operation Management
    
    private func completeBakeOperation(_ operation: BakeOperation, result: BakeResult, bakingTime: Double) {
        var mutableOperation = operation
        mutableOperation.status = result.success ? .completed : .failed
        mutableOperation.completedAt = Date()
        mutableOperation.result = result
        
        if let index = bakingOperations.firstIndex(where: { $0.id == operation.id }) {
            bakingOperations[index] = mutableOperation
        }
        
        // Update progress
        let completedCount = bakingOperations.filter { $0.status == .completed }.count
        bakingProgress = Float(completedCount) / Float(bakingOperations.count)
        
        if result.success {
            // Create baked lightmap
            let bakedLightmap = BakedLightmap(
                id: UUID(),
                region: operation.terrainRegion,
                lightmapData: result.lightmapData ?? Data(),
                width: lightmapResolution,
                height: lightmapResolution,
                format: operation.lightSettings.lightmapFormat,
                bakeType: operation.bakeType,
                bakeSettings: operation.lightSettings,
                createdAt: Date(),
                fileSize: Int64(result.lightmapData?.count ?? 0)
            )
            
            bakedLightmaps.append(bakedLightmap)
            
            // Cache result
            let cacheKey = LightmapCacheKey(
                region: operation.terrainRegion,
                settings: operation.lightSettings,
                bakeType: operation.bakeType
            )
            lightmapCache[cacheKey] = bakedLightmap
        }
        
        updateBakingStatistics()
        bakingInProgress = false
        currentBakeOperation = nil
        
        if result.success {
            print("Completed bake operation \(operation.id) in \(bakingTime)s")
        } else {
            print("Failed bake operation \(operation.id): \(result.error ?? "Unknown error")")
        }
    }
    
    private func failBakeOperation(_ operation: BakeOperation, error: String) {
        var mutableOperation = operation
        mutableOperation.status = .failed
        mutableOperation.completedAt = Date()
        mutableOperation.result = BakeResult(
            success: false,
            lightmapData: nil,
            error: error,
            bakingTime: 0,
            samplesProcessed: 0,
            memoryUsage: 0
        )
        
        if let index = bakingOperations.firstIndex(where: { $0.id == operation.id }) {
            bakingOperations[index] = mutableOperation
        }
        
        updateBakingStatistics()
        bakingInProgress = false
        currentBakeOperation = nil
        
        print("Failed bake operation \(operation.id): \(error)")
    }
    
    // MARK: - Statistics and Configuration
    
    private func updateBakingStatistics() {
        bakingStatistics.totalBakes = bakingOperations.count
        
        let completed = bakingOperations.filter { $0.status == .completed }
        let failed = bakingOperations.filter { $0.status == .failed }
        
        bakingStatistics.completedBakes = completed.count
        bakingStatistics.failedBakes = failed.count
        
        let totalTime = completed.reduce(0) { $0 + ($0.completedAt?.timeIntervalSince($0.createdAt) ?? 0) }
        bakingStatistics.averageBakingTime = completed.isEmpty ? 0 : totalTime / Double(completed.count)
        
        bakingStatistics.totalSamplesProcessed = completed.reduce(0) { $0 + ($0.result?.samplesProcessed ?? 0) }
        bakingStatistics.totalMemoryUsage = bakedLightmaps.reduce(0) { $0 + $1.memoryUsage }
        
        bakingStatistics.lastUpdate = Date()
    }
    
    func getBakingStatistics() -> BakingStatistics {
        return bakingStatistics
    }
    
    func configureBaking(
        enabled: Bool,
        resolution: Int,
        quality: LightmapQuality
    ) {
        bakingEnabled = enabled
        lightmapResolution = resolution
        lightmapQuality = quality
        
        print("Lightmap baking configured: enabled=\(enabled), resolution=\(resolution), quality=\(quality)")
    }
    
    private func setupUpdateTimer() {
        bakingTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.periodicUpdate()
        }
    }
    
    private func periodicUpdate() {
        // Process pending bake operations
        let pendingOperations = bakingOperations.filter { $0.status == .pending }
        
        for operation in pendingOperations.sorted(by: { $0.priority.rawValue < $1.priority.rawValue }) {
            if !bakingInProgress {
                executeBakeOperation(operation)
                break // Only process one at a time
            }
        }
    }
}

// MARK: - Extensions

extension TerrainRegion {
    var hashValue: Int {
        return origin.hashValue ^ size.hashValue ^ heightmap.count
    }
}

extension LightSettings {
    var hashValue: Int {
        return sunDirection.hashValue ^ sunColor.hashValue ^ sunIntensity.hashValue ^
               ambientColor.hashValue ^ ambientIntensity.hashValue ^
               skyColor.hashValue ^ indirectIntensity.hashValue ^
               bounceCount ^ sampleCount ^ lightmapFormat.hashValue
    }
}
