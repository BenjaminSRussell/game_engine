import SwiftUI
import Foundation

// MARK: - Terrain Prefab System

/// Terrain Prefab System provides comprehensive saving and loading of terrain configurations
@MainActor
class TerrainPrefabSystem: ObservableObject {
    static let shared = TerrainPrefabSystem()
    
    @Published var availablePrefabs: [TerrainPrefab] = []
    @Published var isLoadingPrefabs: Bool = false
    @Published var isSavingPrefab: Bool = false
    @Published var lastSaveError: PrefabError?
    @Published var lastLoadError: PrefabError?
    
    private let prefabDirectory = "TerrainPrefabs"
    private let fileManager = FileManager.default
    private let encoder = JSONEncoder()
    private let decoder = JSONDecoder()
    
    private init() {
        encoder.dateEncodingStrategy = .iso8601
        decoder.dateDecodingStrategy = .iso8601
        
        createPrefabDirectory()
        loadAvailablePrefabs()
    }
    
    // MARK: - Prefab Data Structure
    
    struct TerrainPrefab: Codable, Identifiable {
        let id: UUID
        let name: String
        let description: String
        let author: String
        let version: String
        let createdAt: Date
        let modifiedAt: Date
        let tags: [String]
        let previewImage: Data? // Base64 encoded preview image
        let metadata: PrefabMetadata
        let terrainData: CompressedTerrainData
        let modifications: [PrefabModification]
        
        struct PrefabMetadata: Codable {
            let terrainSize: SIMD2<Int>
            let heightRange: ClosedRange<Float>
            let totalVertices: Int
            let totalTriangles: Int
            let textureCount: Int
            let vegetationCount: Int
            let estimatedMemoryUsage: Int64
            let compatibilityVersion: String
        }
        
        struct CompressedTerrainData: Codable {
            let heightmapData: Data // Compressed heightmap
            let textureMapData: Data // Compressed texture map
            let vegetationData: Data // Compressed vegetation data
            let compressionFormat: CompressionFormat
            let originalSize: SIMD2<Int>
            
            enum CompressionFormat: String, Codable, CaseIterable {
                case gzip = "gzip"
                case lz4 = "lz4"
                case zlib = "zlib"
                case none = "none"
            }
        }
        
        struct PrefabModification: Codable {
            let id: UUID
            let type: ModificationType
            let position: SIMD2<Float>
            let parameters: [String: CodableValue]
            let timestamp: Date
            
            enum ModificationType: String, Codable, CaseIterable {
                case heightModification = "height_modification"
                case texturePaint = "texture_paint"
                case vegetationAdd = "vegetation_add"
                case erosion = "erosion"
                case custom = "custom"
            }
        }
    }
    
    // MARK: - Save Operations
    
    /// Save current terrain state as a prefab
    func savePrefab(
        name: String,
        description: String,
        author: String = "Unknown",
        tags: [String] = [],
        previewImage: NSImage? = nil,
        terrainData: TerrainData,
        modifications: [TerrainModification] = []
    ) async -> Result<URL, PrefabError> {
        
        isSavingPrefab = true
        lastSaveError = nil
        
        defer { isSavingPrefab = false }
        
        do {
            // Create prefab structure
            let prefab = try createPrefab(
                name: name,
                description: description,
                author: author,
                tags: tags,
                previewImage: previewImage,
                terrainData: terrainData,
                modifications: modifications
            )
            
            // Save to file
            let fileURL = try await savePrefabToFile(prefab)
            
            // Update available prefabs
            availablePrefabs.append(prefab)
            
            return .success(fileURL)
            
        } catch let error as PrefabError {
            lastSaveError = error
            return .failure(error)
        } catch {
            let prefabError = PrefabError.saveFailed(error.localizedDescription)
            lastSaveError = prefabError
            return .failure(prefabError)
        }
    }
    
    /// Save prefab with custom metadata
    func savePrefabWithMetadata(
        name: String,
        description: String,
        metadata: TerrainPrefab.PrefabMetadata,
        terrainData: TerrainData,
        modifications: [TerrainModification] = []
    ) async -> Result<URL, PrefabError> {
        
        isSavingPrefab = true
        lastSaveError = nil
        
        defer { isSavingPrefab = false }
        
        do {
            let compressedData = try compressTerrainData(terrainData)
            let prefabModifications = try convertModifications(modifications)
            
            let prefab = TerrainPrefab(
                id: UUID(),
                name: name,
                description: description,
                author: "Unknown",
                version: "1.0",
                createdAt: Date(),
                modifiedAt: Date(),
                tags: [],
                previewImage: nil,
                metadata: metadata,
                terrainData: compressedData,
                modifications: prefabModifications
            )
            
            let fileURL = try await savePrefabToFile(prefab)
            availablePrefabs.append(prefab)
            
            return .success(fileURL)
            
        } catch let error as PrefabError {
            lastSaveError = error
            return .failure(error)
        } catch {
            let prefabError = PrefabError.saveFailed(error.localizedDescription)
            lastSaveError = prefabError
            return .failure(prefabError)
        }
    }
    
    // MARK: - Load Operations
    
    /// Load a prefab from file
    func loadPrefab(from url: URL) async -> Result<TerrainPrefab, PrefabError> {
        isLoadingPrefabs = true
        lastLoadError = nil
        
        defer { isLoadingPrefabs = false }
        
        do {
            let data = try Data(contentsOf: url)
            let prefab = try decoder.decode(TerrainPrefab.self, from: data)
            
            return .success(prefab)
            
        } catch let error as PrefabError {
            lastLoadError = error
            return .failure(error)
        } catch {
            let prefabError = PrefabError.loadFailed(error.localizedDescription)
            lastLoadError = prefabError
            return .failure(prefabError)
        }
    }
    
    /// Load terrain data from prefab
    func loadTerrainData(from prefab: TerrainPrefab) async -> Result<TerrainData, PrefabError> {
        do {
            let terrainData = try decompressTerrainData(prefab.terrainData)
            return .success(terrainData)
        } catch let error as PrefabError {
            return .failure(error)
        } catch {
            return .failure(.decompressionFailed(error.localizedDescription))
        }
    }
    
    /// Apply prefab modifications to terrain
    func applyModifications(
        from prefab: TerrainPrefab,
        to terrainAPI: TerrainRuntimeAPI,
        offset: SIMD2<Float> = .zero
    ) async -> Result<[TerrainModificationResult], PrefabError> {
        
        do {
            var results: [TerrainModificationResult] = []
            
            for prefabMod in prefab.modifications {
                let modification = try convertToRuntimeModification(prefabMod, offset: offset)
                let result = try await terrainAPI.executeModification(modification)
                results.append(result)
            }
            
            return .success(results)
            
        } catch let error as PrefabError {
            return .failure(error)
        } catch {
            return .failure(.applicationFailed(error.localizedDescription))
        }
    }
    
    // MARK: - Management Operations
    
    /// Delete a prefab
    func deletePrefab(_ prefab: TerrainPrefab) async -> Result<Void, PrefabError> {
        do {
            let fileURL = getPrefabURL(for: prefab)
            try fileManager.removeItem(at: fileURL)
            
            availablePrefabs.removeAll { $0.id == prefab.id }
            
            return .success(())
            
        } catch {
            return .failure(.deleteFailed(error.localizedDescription))
        }
    }
    
    /// Duplicate a prefab
    func duplicatePrefab(_ prefab: TerrainPrefab, newName: String) async -> Result<TerrainPrefab, PrefabError> {
        var duplicatedPrefab = prefab
        duplicatedPrefab.id = UUID()
        duplicatedPrefab.name = newName
        duplicatedPrefab.createdAt = Date()
        duplicatedPrefab.modifiedAt = Date()
        
        let result = await savePrefabToFile(duplicatedPrefab)
        
        switch result {
        case .success:
            availablePrefabs.append(duplicatedPrefab)
            return .success(duplicatedPrefab)
        case .failure(let error):
            return .failure(error)
        }
    }
    
    /// Refresh prefab list
    func refreshPrefabs() async {
        await loadAvailablePrefabs()
    }
    
    /// Search prefabs by criteria
    func searchPrefabs(
        query: String? = nil,
        tags: [String] = [],
        author: String? = nil,
        dateRange: ClosedRange<Date>? = nil
    ) -> [TerrainPrefab] {
        
        return availablePrefabs.filter { prefab in
            // Name/description search
            if let query = query, !query.isEmpty {
                let searchLower = query.lowercased()
                if !prefab.name.lowercased().contains(searchLower) &&
                   !prefab.description.lowercased().contains(searchLower) {
                    return false
                }
            }
            
            // Tag filter
            if !tags.isEmpty {
                let hasAllTags = tags.allSatisfy { tag in
                    prefab.tags.contains { $0.lowercased() == tag.lowercased() }
                }
                if !hasAllTags {
                    return false
                }
            }
            
            // Author filter
            if let author = author, !author.isEmpty {
                if !prefab.author.lowercased().contains(author.lowercased()) {
                    return false
                }
            }
            
            // Date range filter
            if let dateRange = dateRange {
                if !dateRange.contains(prefab.createdAt) {
                    return false
                }
            }
            
            return true
        }
    }
    
    // MARK: - Preview Generation
    
    /// Generate preview image for terrain
    func generatePreviewImage(
        for terrainData: TerrainData,
        size: CGSize = CGSize(width: 256, height: 256)
    ) -> NSImage? {
        
        // Create image representation
        let imageRep = NSBitmapImageRep(
            bitmapDataPlanes: nil,
            pixelsWide: Int(size.width),
            pixelsHigh: Int(size.height),
            bitsPerSample: 8,
            samplesPerPixel: 4,
            hasAlpha: true,
            isPlanar: false,
            colorSpaceName: .calibratedRGB,
            bytesPerRow: 0,
            bitsPerPixel: 0
        )
        
        guard let bitmapData = imageRep?.bitmapData else { return nil }
        
        // Generate height-based preview
        let scaleX = Float(size.width) / Float(terrainData.size.x)
        let scaleY = Float(size.height) / Float(terrainData.size.y)
        
        for y in 0..<Int(size.height) {
            for x in 0..<Int(size.width) {
                let terrainX = Int(Float(x) / scaleX)
                let terrainY = Int(Float(y) / scaleY)
                
                guard terrainX < terrainData.size.x && terrainY < terrainData.size.y else { continue }
                
                let height = terrainData.heightmap[terrainY][terrainX]
                let normalizedHeight = (height + 50) / 250 // Normalize to 0-1 range
                
                let pixelIndex = (y * Int(size.width) + x) * 4
                
                // Color based on height
                let color = heightToColor(normalizedHeight)
                bitmapData[pixelIndex] = UInt8(color.red * 255)
                bitmapData[pixelIndex + 1] = UInt8(color.green * 255)
                bitmapData[pixelIndex + 2] = UInt8(color.blue * 255)
                bitmapData[pixelIndex + 3] = 255 // Alpha
            }
        }
        
        guard let imageRep = imageRep else { return nil }
        return NSImage(size: size, drawingHandler: { _ in
            imageRep.draw(in: NSRect(origin: .zero, size: size))
            return true
        })
    }
    
    // MARK: - Private Methods
    
    private func createPrefabDirectory() {
        guard let documentsURL = fileManager.urls(for: .documentDirectory, in: .userDomainMask).first else {
            return
        }
        
        let prefabDir = documentsURL.appendingPathComponent(prefabDirectory)
        
        if !fileManager.fileExists(atPath: prefabDir.path) {
            do {
                try fileManager.createDirectory(at: prefabDir, withIntermediateDirectories: true)
            } catch {
                print("Failed to create prefab directory: \(error)")
            }
        }
    }
    
    private func loadAvailablePrefabs() async {
        isLoadingPrefabs = true
        defer { isLoadingPrefabs = false }
        
        guard let documentsURL = fileManager.urls(for: .documentDirectory, in: .userDomainMask).first else {
            return
        }
        
        let prefabDir = documentsURL.appendingPathComponent(prefabDirectory)
        
        do {
            let prefabFiles = try fileManager.contentsOfDirectory(
                at: prefabDir,
                includingPropertiesForKeys: [.creationDateKey, .contentModificationDateKey],
                options: [.skipsHiddenFiles]
            )
            
            var prefabs: [TerrainPrefab] = []
            
            for fileURL in prefabFiles where fileURL.pathExtension == "terrainprefab" {
                do {
                    let data = try Data(contentsOf: fileURL)
                    let prefab = try decoder.decode(TerrainPrefab.self, from: data)
                    prefabs.append(prefab)
                } catch {
                    print("Failed to load prefab from \(fileURL.lastPathComponent): \(error)")
                }
            }
            
            await MainActor.run {
                self.availablePrefabs = prefabs.sorted { $0.modifiedAt > $1.modifiedAt }
            }
            
        } catch {
            print("Failed to load prefabs directory: \(error)")
        }
    }
    
    private func createPrefab(
        name: String,
        description: String,
        author: String,
        tags: [String],
        previewImage: NSImage?,
        terrainData: TerrainData,
        modifications: [TerrainModification]
    ) throws -> TerrainPrefab {
        
        // Compress terrain data
        let compressedData = try compressTerrainData(terrainData)
        
        // Convert modifications
        let prefabModifications = try convertModifications(modifications)
        
        // Generate metadata
        let metadata = generateMetadata(for: terrainData)
        
        // Encode preview image
        let previewData = previewImage != nil ? encodeImage(previewImage!) : nil
        
        return TerrainPrefab(
            id: UUID(),
            name: name,
            description: description,
            author: author,
            version: "1.0",
            createdAt: Date(),
            modifiedAt: Date(),
            tags: tags,
            previewImage: previewData,
            metadata: metadata,
            terrainData: compressedData,
            modifications: prefabModifications
        )
    }
    
    private func compressTerrainData(_ terrainData: TerrainData) throws -> TerrainPrefab.CompressedTerrainData {
        // Convert heightmap to data
        let heightmapData = try encodeHeightmap(terrainData.heightmap)
        let textureMapData = try encodeTextureMap(terrainData.textureMap)
        let vegetationData = try encodeVegetationMap(terrainData.vegetationMap)
        
        // Compress data (simplified - using gzip)
        let compressedHeightmap = try compressData(heightmapData, format: .gzip)
        let compressedTextureMap = try compressData(textureMapData, format: .gzip)
        let compressedVegetation = try compressData(vegetationData, format: .gzip)
        
        return TerrainPrefab.CompressedTerrainData(
            heightmapData: compressedHeightmap,
            textureMapData: compressedTextureMap,
            vegetationData: compressedVegetation,
            compressionFormat: .gzip,
            originalSize: terrainData.size
        )
    }
    
    private func decompressTerrainData(_ compressedData: TerrainPrefab.CompressedTerrainData) throws -> TerrainData {
        // Decompress data
        let heightmapData = try decompressData(compressedData.heightmapData, format: compressedData.compressionFormat)
        let textureMapData = try decompressData(compressedData.textureMapData, format: compressedData.compressionFormat)
        let vegetationData = try decompressData(compressedData.vegetationData, format: compressedData.compressionFormat)
        
        // Decode data
        let heightmap = try decodeHeightmap(heightmapData, size: compressedData.originalSize)
        let textureMap = try decodeTextureMap(textureMapData, size: compressedData.originalSize)
        let vegetationMap = try decodeVegetationMap(vegetationData, size: compressedData.originalSize)
        
        return TerrainData(
            size: compressedData.originalSize,
            heightmap: heightmap,
            textureMap: textureMap,
            vegetationMap: vegetationMap
        )
    }
    
    private func generateMetadata(for terrainData: TerrainData) -> TerrainPrefab.PrefabMetadata {
        let heightValues = terrainData.heightmap.flatMap { $0 }
        let heightRange = heightValues.min()!...heightValues.max()!
        
        let totalVertices = terrainData.size.x * terrainData.size.y
        let totalTriangles = (terrainData.size.x - 1) * (terrainData.size.y - 1) * 2
        
        let textureCount = Set(terrainData.textureMap.flatMap { $0 }).count
        let vegetationCount = terrainData.vegetationMap.flatMap { $0 }.count
        
        let estimatedMemoryUsage = Int64(totalVertices * 4 + totalTriangles * 6 + textureCount * 1024)
        
        return TerrainPrefab.PrefabMetadata(
            terrainSize: terrainData.size,
            heightRange: heightRange,
            totalVertices: totalVertices,
            totalTriangles: totalTriangles,
            textureCount: textureCount,
            vegetationCount: vegetationCount,
            estimatedMemoryUsage: estimatedMemoryUsage,
            compatibilityVersion: "1.0"
        )
    }
    
    private func savePrefabToFile(_ prefab: TerrainPrefab) async throws -> URL {
        let data = try encoder.encode(prefab)
        
        guard let documentsURL = fileManager.urls(for: .documentDirectory, in: .userDomainMask).first else {
            throw PrefabError.saveFailed("Could not access documents directory")
        }
        
        let prefabDir = documentsURL.appendingPathComponent(prefabDirectory)
        let fileName = "\(prefab.name.replacingOccurrences(of: " ", with: "_")).terrainprefab"
        let fileURL = prefabDir.appendingPathComponent(fileName)
        
        try data.write(to: fileURL)
        
        return fileURL
    }
    
    private func getPrefabURL(for prefab: TerrainPrefab) -> URL {
        guard let documentsURL = fileManager.urls(for: .documentDirectory, in: .userDomainMask).first else {
            fatalError("Could not access documents directory")
        }
        
        let prefabDir = documentsURL.appendingPathComponent(prefabDirectory)
        let fileName = "\(prefab.name.replacingOccurrences(of: " ", with: "_")).terrainprefab"
        return prefabDir.appendingPathComponent(fileName)
    }
    
    // MARK: - Helper Methods
    
    private func encodeHeightmap(_ heightmap: [[Float]]) throws -> Data {
        let flatArray = heightmap.flatMap { $0 }
        return try JSONEncoder().encode(flatArray)
    }
    
    private func decodeHeightmap(_ data: Data, size: SIMD2<Int>) throws -> [[Float]] {
        let flatArray = try JSONDecoder().decode([Float].self, from: data)
        guard flatArray.count == size.x * size.y else {
            throw PrefabError.decompressionFailed("Heightmap size mismatch")
        }
        
        var heightmap: [[Float]] = []
        for y in 0..<size.y {
            let startIndex = y * size.x
            let endIndex = startIndex + size.x
            heightmap.append(Array(flatArray[startIndex..<endIndex]))
        }
        
        return heightmap
    }
    
    private func encodeTextureMap(_ textureMap: [[Int]]) throws -> Data {
        let flatArray = textureMap.flatMap { $0 }
        return try JSONEncoder().encode(flatArray)
    }
    
    private func decodeTextureMap(_ data: Data, size: SIMD2<Int>) throws -> [[Int]] {
        let flatArray = try JSONDecoder().decode([Int].self, from: data)
        guard flatArray.count == size.x * size.y else {
            throw PrefabError.decompressionFailed("Texture map size mismatch")
        }
        
        var textureMap: [[Int]] = []
        for y in 0..<size.y {
            let startIndex = y * size.x
            let endIndex = startIndex + size.x
            textureMap.append(Array(flatArray[startIndex..<endIndex]))
        }
        
        return textureMap
    }
    
    private func encodeVegetationMap(_ vegetationMap: [[VegetationInstance]]) throws -> Data {
        let flatArray = vegetationMap.flatMap { $0 }
        return try JSONEncoder().encode(flatArray)
    }
    
    private func decodeVegetationMap(_ data: Data, size: SIMD2<Int>) throws -> [[VegetationInstance]] {
        let flatArray = try JSONDecoder().decode([VegetationInstance].self, from: data)
        
        var vegetationMap: [[VegetationInstance]] = Array(repeating: [], count: size.y)
        for instance in flatArray {
            // Distribute instances based on their position (simplified)
            let x = Int(instance.position.x) % size.x
            let y = Int(instance.position.z) % size.y
            vegetationMap[y].append(instance)
        }
        
        return vegetationMap
    }
    
    private func compressData(_ data: Data, format: TerrainPrefab.CompressedTerrainData.CompressionFormat) throws -> Data {
        switch format {
        case .gzip:
            return try (data as NSData).compressed(using: .lzfse) as Data
        case .lz4, .zlib:
            // Placeholder for other compression formats
            return data
        case .none:
            return data
        }
    }
    
    private func decompressData(_ data: Data, format: TerrainPrefab.CompressedTerrainData.CompressionFormat) throws -> Data {
        switch format {
        case .gzip:
            return try (data as NSData).decompressed(using: .lzfse) as Data
        case .lz4, .zlib:
            // Placeholder for other compression formats
            return data
        case .none:
            return data
        }
    }
    
    private func encodeImage(_ image: NSImage) -> Data {
        guard let tiffData = image.tiffRepresentation,
              let bitmapImage = NSBitmapImageRep(data: tiffData),
              let pngData = bitmapImage.representation(using: .png, properties: [:]) else {
            return Data()
        }
        return pngData
    }
    
    private func convertModifications(_ modifications: [TerrainModification]) throws -> [TerrainPrefab.PrefabModification] {
        return try modifications.map { modification in
            let parameters = try modification.parameters.mapValues { value in
                CodableValue(value: value)
            }
            
            return TerrainPrefab.PrefabModification(
                id: modification.id,
                type: TerrainPrefab.PrefabModification.ModificationType(rawValue: modification.type.rawValue) ?? .custom,
                position: modification.position,
                parameters: parameters,
                timestamp: modification.timestamp
            )
        }
    }
    
    private func convertToRuntimeModification(
        _ prefabMod: TerrainPrefab.PrefabModification,
        offset: SIMD2<Float>
    ) throws -> TerrainModification {
        let parameters = prefabMod.parameters.mapValues { value in
            value.value
        }
        
        return TerrainModification(
            id: prefabMod.id,
            type: TerrainModification.ModificationType(rawValue: prefabMod.type.rawValue) ?? .heightModification,
            position: prefabMod.position + offset,
            parameters: parameters,
            timestamp: prefabMod.timestamp
        )
    }
    
    private func convertToRuntimeModification(_ modification: TerrainPrefab.PrefabModification) async throws -> TerrainModificationResult {
        // This would need to be implemented based on the actual TerrainRuntimeAPI
        fatalError("Not implemented")
    }
    
    private func heightToColor(_ height: Float) -> (red: Float, green: Float, blue: Float) {
        // Color gradient based on height
        switch height {
        case 0.0..<0.2:
            return (0.2, 0.4, 0.8) // Deep water - blue
        case 0.2..<0.3:
            return (0.4, 0.6, 0.9) // Shallow water - light blue
        case 0.3..<0.4:
            return (0.8, 0.7, 0.4) // Beach - sand
        case 0.4..<0.6:
            return (0.3, 0.7, 0.2) // Grass - green
        case 0.6..<0.8:
            return (0.5, 0.4, 0.3) // Mountain - brown
        default:
            return (0.9, 0.9, 0.9) // Snow - white
        }
    }
}

// MARK: - Supporting Types

enum PrefabError: LocalizedError {
    case saveFailed(String)
    case loadFailed(String)
    case deleteFailed(String)
    case decompressionFailed(String)
    case compressionFailed(String)
    case invalidFormat(String)
    case applicationFailed(String)
    case fileNotFound
    case permissionDenied
    
    var errorDescription: String? {
        switch self {
        case .saveFailed(let message):
            return "Failed to save prefab: \(message)"
        case .loadFailed(let message):
            return "Failed to load prefab: \(message)"
        case .deleteFailed(let message):
            return "Failed to delete prefab: \(message)"
        case .decompressionFailed(let message):
            return "Failed to decompress prefab data: \(message)"
        case .compressionFailed(let message):
            return "Failed to compress prefab data: \(message)"
        case .invalidFormat(let message):
            return "Invalid prefab format: \(message)"
        case .applicationFailed(let message):
            return "Failed to apply prefab: \(message)"
        case .fileNotFound:
            return "Prefab file not found"
        case .permissionDenied:
            return "Permission denied accessing prefab files"
        }
    }
}

struct CodableValue: Codable {
    let value: Any
    
    init(value: Any) {
        self.value = value
    }
    
    init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        
        if let bool = try? container.decode(Bool.self) {
            value = bool
        } else if let int = try? container.decode(Int.self) {
            value = int
        } else if let double = try? container.decode(Double.self) {
            value = double
        } else if let string = try? container.decode(String.self) {
            value = string
        } else {
            value = NSNull()
        }
    }
    
    func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        
        if let bool = value as? Bool {
            try container.encode(bool)
        } else if let int = value as? Int {
            try container.encode(int)
        } else if let double = value as? Double {
            try container.encode(double)
        } else if let string = value as? String {
            try container.encode(string)
        } else {
            try container.encodeNil()
        }
    }
}
