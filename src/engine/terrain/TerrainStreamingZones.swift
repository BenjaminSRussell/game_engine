import Foundation
import simd

// MARK: - Terrain Streaming Zones System

/// Terrain streaming zones system provides efficient terrain data streaming for large worlds
@MainActor
class TerrainStreamingZones: ObservableObject {
    static let shared = TerrainStreamingZones()
    
    @Published var activeZones: [StreamingZone] = []
    @Published var loadedZones: Set<UUID> = []
    @Published var streamingEnabled: Bool = true
    @Published var streamDistance: Double = 10_000.0
    @Published var unloadDistance: Double = 15_000.0
    @Published var maxLoadedZones: Int = 50
    @Published var streamingQuality: StreamingQuality = .high
    @Published var streamingStatistics: StreamingStatistics
    
    private var zoneGrid: [ZoneGridKey: StreamingZone] = [:]
    private var streamingQueue: DispatchQueue
    private var loadTimer: Timer?
    private var cameraPosition: SIMD3<Double> = .zero
    private var lastUpdatePosition: SIMD3<Double> = .zero
    
    struct StreamingZone: Identifiable, Codable {
        let id: UUID
        let position: SIMD3<Double>
        let size: SIMD3<Double>
        let levelOfDetail: Int
        let priority: Priority
        let dataPath: String
        let dependencies: [UUID]
        let createdAt: Date
        let lastAccessed: Date
        
        enum Priority: Int, Codable, CaseIterable {
            case critical = 0
            case high = 1
            case medium = 2
            case low = 3
            case background = 4
        }
        
        var center: SIMD3<Double> {
            return position + size / 2.0
        }
        
        var bounds: BoundingBox {
            return BoundingBox(
                min: position,
                max: position + size
            )
        }
        
        func distance(to point: SIMD3<Double>) -> Double {
            return length(center - point)
        }
        
        func intersects(_ other: StreamingZone) -> Bool {
            return bounds.intersects(other.bounds)
        }
    }
    
    struct ZoneGridKey: Hashable {
        let x: Int
        let y: Int
        let z: Int
        
        init(_ position: SIMD3<Double>, zoneSize: Double) {
            self.x = Int(floor(position.x / zoneSize))
            self.y = Int(floor(position.y / zoneSize))
            self.z = Int(floor(position.z / zoneSize))
        }
    }
    
    struct BoundingBox: Codable {
        let min: SIMD3<Double>
        let max: SIMD3<Double>
        
        func contains(_ point: SIMD3<Double>) -> Bool {
            return point.x >= min.x && point.x <= max.x &&
                   point.y >= min.y && point.y <= max.y &&
                   point.z >= min.z && point.z <= max.z
        }
        
        func intersects(_ other: BoundingBox) -> Bool {
            return !(max.x < other.min.x || other.max.x < min.x ||
                    max.y < other.min.y || other.max.y < min.y ||
                    max.z < other.min.z || other.max.z < min.z)
        }
        
        var volume: Double {
            let size = max - min
            return size.x * size.y * size.z
        }
    }
    
    enum StreamingQuality: Int, CaseIterable {
        case ultra = 0
        case high = 1
        case medium = 2
        case low = 3
        
        var resolutionMultiplier: Float {
            switch self {
            case .ultra: return 1.0
            case .high: return 0.75
            case .medium: return 0.5
            case .low: return 0.25
            }
        }
        
        var lodBias: Int {
            switch self {
            case .ultra: return 0
            case .high: return 1
            case .medium: return 2
            case .low: return 3
            }
        }
    }
    
    struct StreamingStatistics {
        var totalZones: Int = 0
        var loadedZones: Int = 0
        var loadingZones: Int = 0
        var unloadingZones: Int = 0
        var memoryUsage: Int64 = 0
        var averageLoadTime: Double = 0.0
        var averageUnloadTime: Double = 0.0
        var lastUpdate: Date = Date()
        
        var loadEfficiency: Double {
            guard totalZones > 0 else { return 0.0 }
            return Double(loadedZones) / Double(totalZones)
        }
        
        var memoryUsageMB: Double {
            return Double(memoryUsage) / (1024 * 1024)
        }
    }
    
    private init() {
        streamingQueue = DispatchQueue(label: "terrain.streaming", qos: .userInteractive)
        streamingStatistics = StreamingStatistics()
        setupLoadTimer()
    }
    
    deinit {
        loadTimer?.invalidate()
    }
    
    // MARK: - Zone Management
    
    /// Initialize streaming zones for a world
    func initializeZones(worldSize: SIMD3<Double>, zoneSize: SIMD3<Double>) {
        activeZones.removeAll()
        zoneGrid.removeAll()
        
        let zonesX = Int(ceil(worldSize.x / zoneSize.x))
        let zonesY = Int(ceil(worldSize.y / zoneSize.y))
        let zonesZ = Int(ceil(worldSize.z / zoneSize.z))
        
        for x in 0..<zonesX {
            for y in 0..<zonesY {
                for z in 0..<zonesZ {
                    let position = SIMD3<Double>(
                        Double(x) * zoneSize.x,
                        Double(y) * zoneSize.y,
                        Double(z) * zoneSize.z
                    )
                    
                    let zone = StreamingZone(
                        id: UUID(),
                        position: position,
                        size: zoneSize,
                        levelOfDetail: calculateLOD(for: position),
                        priority: calculatePriority(for: position),
                        dataPath: generateDataPath(for: SIMD3<Double>(x, y, z)),
                        dependencies: calculateDependencies(for: SIMD3<Double>(x, y, z)),
                        createdAt: Date(),
                        lastAccessed: Date()
                    )
                    
                    activeZones.append(zone)
                    zoneGrid[ZoneGridKey(position, zoneSize: zoneSize.x)] = zone
                }
            }
        }
        
        streamingStatistics.totalZones = activeZones.count
        print("Initialized \(activeZones.count) streaming zones")
    }
    
    /// Update streaming based on camera position
    func updateStreaming(for cameraPosition: SIMD3<Double>) {
        guard streamingEnabled else { return }
        
        self.cameraPosition = cameraPosition
        
        // Check if we need to update (camera moved significantly)
        let movement = length(cameraPosition - lastUpdatePosition)
        if movement < 1000.0 { return } // Only update if moved > 1km
        
        lastUpdatePosition = cameraPosition
        
        streamingQueue.async { [weak self] in
            self?.performStreamingUpdate(cameraPosition: cameraPosition)
        }
    }
    
    private func performStreamingUpdate(cameraPosition: SIMD3<Double>) {
        let startTime = Date()
        
        // Determine zones to load and unload
        let zonesToLoad = getZonesToLoad(cameraPosition: cameraPosition)
        let zonesToUnload = getZonesToUnload(cameraPosition: cameraPosition)
        
        // Update statistics
        streamingStatistics.loadingZones = zonesToLoad.count
        streamingStatistics.unloadingZones = zonesToUnload.count
        
        // Unload distant zones first to free memory
        for zone in zonesToUnload {
            unloadZone(zone)
        }
        
        // Load nearby zones
        for zone in zonesToLoad {
            loadZone(zone)
        }
        
        // Update memory usage
        updateMemoryUsage()
        
        let updateTime = Date().timeIntervalSince(startTime)
        streamingStatistics.lastUpdate = Date()
        
        print("Streaming update completed in \(updateTime)s: loaded=\(zonesToLoad.count), unloaded=\(zonesToUnload.count)")
    }
    
    private func getZonesToLoad(cameraPosition: SIMD3<Double>) -> [StreamingZone] {
        return activeZones.filter { zone in
            !loadedZones.contains(zone.id) &&
            zone.distance(to: cameraPosition) <= streamDistance &&
            shouldLoadZone(zone, cameraPosition: cameraPosition)
        }.sorted { $0.distance(to: cameraPosition) < $1.distance(to: cameraPosition) }
    }
    
    private func getZonesToUnload(cameraPosition: SIMD3<Double>) -> [StreamingZone] {
        return activeZones.filter { zone in
            loadedZones.contains(zone.id) &&
            zone.distance(to: cameraPosition) > unloadDistance &&
            shouldUnloadZone(zone, cameraPosition: cameraPosition)
        }.sorted { $0.distance(to: cameraPosition) > $1.distance(to: cameraPosition) }
    }
    
    private func shouldLoadZone(_ zone: StreamingZone, cameraPosition: SIMD3<Double>) -> Bool {
        // Check if we have room for more zones
        guard loadedZones.count < maxLoadedZones else {
            return zone.priority.rawValue < StreamingZone.Priority.high.rawValue
        }
        
        // Check dependencies
        let dependenciesLoaded = zone.dependencies.allSatisfy { loadedZones.contains($0) }
        
        return dependenciesLoaded
    }
    
    private func shouldUnloadZone(_ zone: StreamingZone, cameraPosition: SIMD3<Double>) -> Bool {
        // Don't unload critical zones unless very far
        if zone.priority == .critical {
            return zone.distance(to: cameraPosition) > unloadDistance * 2.0
        }
        
        // Check if any loaded zones depend on this zone
        let dependentZones = activeZones.filter { $0.dependencies.contains(zone.id) }
        let hasDependents = dependentZones.contains { loadedZones.contains($0.id) }
        
        return !hasDependents
    }
    
    // MARK: - Zone Loading/Unloading
    
    private func loadZone(_ zone: StreamingZone) {
        let startTime = Date()
        
        // Update last accessed time
        var mutableZone = zone
        mutableZone.lastAccessed = Date()
        
        // Load zone data (asynchronously)
        streamingQueue.async { [weak self] in
            do {
                let zoneData = try self?.loadZoneData(from: zone.dataPath)
                
                DispatchQueue.main.async {
                    self?.loadedZones.insert(zone.id)
                    self?.updateLoadStatistics(startTime: startTime, success: true)
                    print("Loaded zone: \(zone.id)")
                }
                
            } catch {
                DispatchQueue.main.async {
                    self?.updateLoadStatistics(startTime: startTime, success: false)
                    print("Failed to load zone \(zone.id): \(error)")
                }
            }
        }
    }
    
    private func unloadZone(_ zone: StreamingZone) {
        let startTime = Date()
        
        streamingQueue.async { [weak self] in
            do {
                try self?.unloadZoneData(from: zone.dataPath)
                
                DispatchQueue.main.async {
                    self?.loadedZones.remove(zone.id)
                    self?.updateUnloadStatistics(startTime: startTime, success: true)
                    print("Unloaded zone: \(zone.id)")
                }
                
            } catch {
                DispatchQueue.main.async {
                    self?.updateUnloadStatistics(startTime: startTime, success: false)
                    print("Failed to unload zone \(zone.id): \(error)")
                }
            }
        }
    }
    
    // MARK: - Data Management
    
    private func loadZoneData(from path: String) throws -> ZoneData {
        // Simulate loading zone data from disk
        // In real implementation, this would load heightmap, textures, etc.
        return ZoneData(
            id: UUID(),
            heightmap: [],
            textures: [],
            vegetation: [],
            metadata: ZoneMetadata(
                version: "1.0",
                createdAt: Date(),
                size: SIMD3<Double>(1024, 1024, 1024),
                resolution: 1.0
            )
        )
    }
    
    private func unloadZoneData(from path: String) throws {
        // Simulate unloading zone data
        // In real implementation, this would free GPU memory, etc.
    }
    
    // MARK: - LOD and Priority Calculation
    
    private func calculateLOD(for position: SIMD3<Double>) -> Int {
        let distanceFromOrigin = length(position)
        
        switch distanceFromOrigin {
        case 0..<5000:
            return 0 // Highest detail
        case 5000..<10000:
            return 1
        case 10000..<20000:
            return 2
        case 20000..<50000:
            return 3
        default:
            return 4 // Lowest detail
        }
    }
    
    private func calculatePriority(for position: SIMD3<Double>) -> StreamingZone.Priority {
        let distanceFromOrigin = length(position)
        
        switch distanceFromOrigin {
        case 0..<2000:
            return .critical
        case 2000..<5000:
            return .high
        case 5000..<10000:
            return .medium
        case 10000..<20000:
            return .low
        default:
            return .background
        }
    }
    
    private func generateDataPath(for gridPosition: SIMD3<Double>) -> String {
        return "terrain/zones/zone_\(Int(gridPosition.x))_\(Int(gridPosition.y))_\(Int(gridPosition.z)).terrain"
    }
    
    private func calculateDependencies(for gridPosition: SIMD3<Double>) -> [UUID] {
        // Calculate dependencies on neighboring zones
        var dependencies: [UUID] = []
        
        let neighborOffsets = [
            SIMD3<Double>(-1, 0, 0), SIMD3<Double>(1, 0, 0),
            SIMD3<Double>(0, -1, 0), SIMD3<Double>(0, 1, 0),
            SIMD3<Double>(0, 0, -1), SIMD3<Double>(0, 0, 1)
        ]
        
        for offset in neighborOffsets {
            let neighborPos = gridPosition + offset
            let key = ZoneGridKey(neighborPos, zoneSize: 1024.0)
            if let neighborZone = zoneGrid[key] {
                dependencies.append(neighborZone.id)
            }
        }
        
        return dependencies
    }
    
    // MARK: - Statistics and Monitoring
    
    private func updateLoadStatistics(startTime: Date, success: Bool) {
        let loadTime = Date().timeIntervalSince(startTime)
        
        if success {
            // Update average load time
            let totalTime = streamingStatistics.averageLoadTime * Double(streamingStatistics.totalZones) + loadTime
            streamingStatistics.averageLoadTime = totalTime / Double(streamingStatistics.totalZones + 1)
        }
        
        streamingStatistics.loadedZones = loadedZones.count
    }
    
    private func updateUnloadStatistics(startTime: Date, success: Bool) {
        let unloadTime = Date().timeIntervalSince(startTime)
        
        if success {
            // Update average unload time
            let totalTime = streamingStatistics.averageUnloadTime * Double(streamingStatistics.totalZones) + unloadTime
            streamingStatistics.averageUnloadTime = totalTime / Double(streamingStatistics.totalZones + 1)
        }
        
        streamingStatistics.loadedZones = loadedZones.count
    }
    
    private func updateMemoryUsage() {
        // Calculate memory usage based on loaded zones
        var totalMemory: Int64 = 0
        
        for zoneID in loadedZones {
            if let zone = activeZones.first(where: { $0.id == zoneID }) {
                // Estimate memory usage based on LOD and quality
                let baseMemory: Int64 = 50 * 1024 * 1024 // 50MB base
                let lodMultiplier = Double(4 - zone.levelOfDetail) / 4.0
                let qualityMultiplier = Double(streamingQuality.resolutionMultiplier)
                
                totalMemory += Int64(Double(baseMemory) * lodMultiplier * qualityMultiplier)
            }
        }
        
        streamingStatistics.memoryUsage = totalMemory
    }
    
    // MARK: - Configuration
    
    func configureStreaming(
        enabled: Bool,
        streamDistance: Double,
        unloadDistance: Double,
        maxZones: Int,
        quality: StreamingQuality
    ) {
        streamingEnabled = enabled
        self.streamDistance = streamDistance
        self.unloadDistance = unloadDistance
        maxLoadedZones = maxZones
        streamingQuality = quality
        
        print("Streaming configured: enabled=\(enabled), streamDist=\(streamDistance), maxZones=\(maxZones)")
    }
    
    func forceUnloadAllZones() {
        for zoneID in loadedZones {
            if let zone = activeZones.first(where: { $0.id == zoneID }) {
                unloadZone(zone)
            }
        }
        
        loadedZones.removeAll()
        updateMemoryUsage()
        
        print("Force unloaded all zones")
    }
    
    func preloadZones(around position: SIMD3<Double>, radius: Double) {
        let zonesToPreload = activeZones.filter { zone in
            zone.distance(to: position) <= radius &&
            !loadedZones.contains(zone.id)
        }
        
        for zone in zonesToPreload.sorted(by: { $0.priority.rawValue < $1.priority.rawValue }) {
            loadZone(zone)
        }
        
        print("Preloading \(zonesToPreload.count) zones around position")
    }
    
    private func setupLoadTimer() {
        loadTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            self?.periodicUpdate()
        }
    }
    
    private func periodicUpdate() {
        guard streamingEnabled else { return }
        
        // Perform periodic streaming update
        performStreamingUpdate(cameraPosition: cameraPosition)
    }
    
    // MARK: - Debugging and Diagnostics
    
    func getStreamingDiagnostics() -> StreamingDiagnostics {
        let zonesInRange = activeZones.filter { $0.distance(to: cameraPosition) <= streamDistance }
        let zonesOutOfRange = activeZones.filter { $0.distance(to: cameraPosition) > unloadDistance }
        
        return StreamingDiagnostics(
            cameraPosition: cameraPosition,
            zonesInRange: zonesInRange.count,
            zonesOutOfRange: zonesOutOfRange.count,
            loadedZones: loadedZones.count,
            memoryUsageMB: streamingStatistics.memoryUsageMB,
            loadEfficiency: streamingStatistics.loadEfficiency,
            averageLoadTime: streamingStatistics.averageLoadTime,
            averageUnloadTime: streamingStatistics.averageUnloadTime,
            streamingQuality: streamingQuality,
            lastUpdate: streamingStatistics.lastUpdate
        )
    }
    
    struct StreamingDiagnostics {
        let cameraPosition: SIMD3<Double>
        let zonesInRange: Int
        let zonesOutOfRange: Int
        let loadedZones: Int
        let memoryUsageMB: Double
        let loadEfficiency: Double
        let averageLoadTime: Double
        let averageUnloadTime: Double
        let streamingQuality: StreamingQuality
        let lastUpdate: Date
    }
}

// MARK: - Zone Data Structures

struct ZoneData: Codable {
    let id: UUID
    let heightmap: [[Float]]
    let textures: [TextureData]
    let vegetation: [VegetationInstance]
    let metadata: ZoneMetadata
    
    struct TextureData: Codable {
        let id: UUID
        let type: String
        let data: Data
        let resolution: SIMD2<Int>
    }
    
    struct ZoneMetadata: Codable {
        let version: String
        let createdAt: Date
        let size: SIMD3<Double>
        let resolution: Double
    }
    
    struct VegetationInstance: Codable {
        let id: UUID
        let position: SIMD3<Float>
        let type: String
        let scale: Float
        let rotation: simd_quatf
    }
}
