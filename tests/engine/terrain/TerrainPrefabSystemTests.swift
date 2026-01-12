import XCTest
@testable import Engine

final class TerrainPrefabSystemTests: XCTestCase {
    
    var prefabSystem: TerrainPrefabSystem!
    var testTerrainData: TerrainData!
    var testModifications: [TerrainModification]!
    
    override func setUp() async throws {
        try await super.setUp()
        
        prefabSystem = TerrainPrefabSystem.shared
        
        // Create test terrain data
        testTerrainData = TerrainData(size: SIMD2<Int>(64, 64))
        for y in 0..<64 {
            for x in 0..<64 {
                testTerrainData.heightmap[y][x] = Float(sin(Double(x) * 0.2) * cos(Double(y) * 0.2)) * 10
                testTerrainData.textureMap[y][x] = (x + y) % 3
            }
        }
        
        // Create test modifications
        testModifications = [
            TerrainModification(
                id: UUID(),
                type: .heightModification,
                position: SIMD2<Float>(32, 32),
                parameters: ["delta": 5.0, "radius": 2.0],
                timestamp: Date()
            ),
            TerrainModification(
                id: UUID(),
                type: .texturePaint,
                position: SIMD2<Float>(16, 16),
                parameters: ["textureIndex": 2, "radius": 3.0],
                timestamp: Date()
            )
        ]
    }
    
    override func tearDown() async throws {
        prefabSystem = nil
        testTerrainData = nil
        testModifications = nil
        
        try await super.tearDown()
    }
    
    // MARK: - Save Tests
    
    func testSavePrefab() async throws {
        let result = await prefabSystem.savePrefab(
            name: "Test Prefab",
            description: "A test prefab for unit testing",
            author: "Unit Test",
            tags: ["test", "sample"],
            terrainData: testTerrainData,
            modifications: testModifications
        )
        
        switch result {
        case .success(let url):
            XCTAssertTrue(FileManager.default.fileExists(atPath: url.path))
            XCTAssertEqual(url.pathExtension, "terrainprefab")
            
        case .failure(let error):
            XCTFail("Failed to save prefab: \(error)")
        }
    }
    
    func testSavePrefabWithEmptyName() async throws {
        let result = await prefabSystem.savePrefab(
            name: "",
            description: "Test prefab",
            terrainData: testTerrainData
        )
        
        switch result {
        case .success:
            XCTFail("Should have failed with empty name")
        case .failure(let error):
            XCTAssertTrue(error.localizedDescription.contains("name") || error.localizedDescription.contains("empty"))
        }
    }
    
    func testSavePrefabWithLargeTerrain() async throws {
        // Create large terrain data
        let largeTerrainData = TerrainData(size: SIMD2<Int>(1024, 1024))
        
        let result = await prefabSystem.savePrefab(
            name: "Large Prefab",
            description: "Large terrain prefab",
            terrainData: largeTerrainData
        )
        
        switch result {
        case .success(let url):
            XCTAssertTrue(FileManager.default.fileExists(atPath: url.path))
            
            // Check file size is reasonable (should be compressed)
            let attributes = try FileManager.default.attributesOfItem(atPath: url.path)
            if let fileSize = attributes[.size] as? Int64 {
                XCTAssertLessThan(fileSize, 50 * 1024 * 1024) // Should be less than 50MB
            }
            
        case .failure(let error):
            XCTFail("Failed to save large prefab: \(error)")
        }
    }
    
    // MARK: - Load Tests
    
    func testLoadPrefab() async throws {
        // First save a prefab
        let saveResult = await prefabSystem.savePrefab(
            name: "Load Test Prefab",
            description: "Prefab for loading test",
            terrainData: testTerrainData
        )
        
        guard case .success(let url) = saveResult else {
            XCTFail("Failed to save test prefab")
            return
        }
        
        // Then load it
        let loadResult = await prefabSystem.loadPrefab(from: url)
        
        switch loadResult {
        case .success(let prefab):
            XCTAssertEqual(prefab.name, "Load Test Prefab")
            XCTAssertEqual(prefab.description, "Prefab for loading test")
            XCTAssertEqual(prefab.terrainData.originalSize, testTerrainData.size)
            
        case .failure(let error):
            XCTFail("Failed to load prefab: \(error)")
        }
    }
    
    func testLoadNonExistentPrefab() async throws {
        let nonExistentURL = URL(fileURLWithPath: "/tmp/nonexistent.terrainprefab")
        
        let result = await prefabSystem.loadPrefab(from: nonExistentURL)
        
        switch result {
        case .success:
            XCTFail("Should have failed for non-existent file")
        case .failure(let error):
            XCTAssertTrue(error.localizedDescription.contains("not found") || error.localizedDescription.contains("exist"))
        }
    }
    
    func testLoadCorruptedPrefab() async throws {
        // Create corrupted file
        let corruptedURL = URL(fileURLWithPath: "/tmp/corrupted.terrainprefab")
        let corruptedData = Data([0xFF, 0xFE, 0xFD, 0xFC]) // Invalid JSON
        try corruptedData.write(to: corruptedURL)
        
        let result = await prefabSystem.loadPrefab(from: corruptedURL)
        
        switch result {
        case .success:
            XCTFail("Should have failed for corrupted file")
        case .failure(let error):
            XCTAssertTrue(error.localizedDescription.contains("format") || error.localizedDescription.contains("decode"))
        }
        
        // Cleanup
        try? FileManager.default.removeItem(at: corruptedURL)
    }
    
    // MARK: - Terrain Data Tests
    
    func testCompressAndDecompressTerrainData() async throws {
        let result = await prefabSystem.loadTerrainData(from: createTestPrefab())
        
        switch result {
        case .success(let loadedTerrainData):
            XCTAssertEqual(loadedTerrainData.size, testTerrainData.size)
            XCTAssertEqual(loadedTerrainData.heightmap.count, testTerrainData.heightmap.count)
            XCTAssertEqual(loadedTerrainData.textureMap.count, testTerrainData.textureMap.count)
            
            // Verify some data integrity
            for y in 0..<min(10, loadedTerrainData.size.y) {
                for x in 0..<min(10, loadedTerrainData.size.x) {
                    XCTAssertEqual(
                        loadedTerrainData.heightmap[y][x],
                        testTerrainData.heightmap[y][x],
                        accuracy: 0.001
                    )
                }
            }
            
        case .failure(let error):
            XCTFail("Failed to decompress terrain data: \(error)")
        }
    }
    
    // MARK: - Management Tests
    
    func testDeletePrefab() async throws {
        // Save a prefab first
        let saveResult = await prefabSystem.savePrefab(
            name: "Delete Test Prefab",
            description: "Prefab to delete",
            terrainData: testTerrainData
        )
        
        guard case .success(let url) = saveResult else {
            XCTFail("Failed to save test prefab")
            return
        }
        
        // Verify file exists
        XCTAssertTrue(FileManager.default.fileExists(atPath: url.path))
        
        // Delete it
        let deleteResult = await prefabSystem.deletePrefab(createTestPrefab())
        
        switch deleteResult {
        case .success:
            // File should be deleted
            XCTAssertFalse(FileManager.default.fileExists(atPath: url.path))
            
        case .failure(let error):
            XCTFail("Failed to delete prefab: \(error)")
        }
    }
    
    func testDuplicatePrefab() async throws {
        let originalPrefab = createTestPrefab()
        
        let result = await prefabSystem.duplicatePrefab(originalPrefab, newName: "Duplicated Prefab")
        
        switch result {
        case .success(let duplicatedPrefab):
            XCTAssertEqual(duplicatedPrefab.name, "Duplicated Prefab")
            XCTAssertEqual(duplicatedPrefab.description, originalPrefab.description)
            XCTAssertEqual(duplicatedPrefab.terrainData.originalSize, originalPrefab.terrainData.originalSize)
            XCTAssertNotEqual(duplicatedPrefab.id, originalPrefab.id)
            
        case .failure(let error):
            XCTFail("Failed to duplicate prefab: \(error)")
        }
    }
    
    func testRefreshPrefabs() async throws {
        // Save a prefab
        _ = await prefabSystem.savePrefab(
            name: "Refresh Test Prefab",
            description: "Prefab for refresh test",
            terrainData: testTerrainData
        )
        
        // Refresh
        await prefabSystem.refreshPrefabs()
        
        // Check if prefab is in the list
        let found = prefabSystem.availablePrefabs.contains { prefab in
            prefab.name == "Refresh Test Prefab"
        }
        
        XCTAssertTrue(found, "Prefab should be available after refresh")
    }
    
    // MARK: - Search Tests
    
    func testSearchPrefabsByName() async throws {
        // Save test prefabs
        _ = await prefabSystem.savePrefab(
            name: "Mountain Terrain",
            description: "Mountain terrain prefab",
            terrainData: testTerrainData
        )
        
        _ = await prefabSystem.savePrefab(
            name: "Desert Terrain",
            description: "Desert terrain prefab",
            terrainData: testTerrainData
        )
        
        await prefabSystem.refreshPrefabs()
        
        // Search for "Mountain"
        let mountainPrefabs = prefabSystem.searchPrefabs(query: "Mountain")
        XCTAssertEqual(mountainPrefabs.count, 1)
        XCTAssertEqual(mountainPrefabs.first?.name, "Mountain Terrain")
        
        // Search for "Terrain"
        let terrainPrefabs = prefabSystem.searchPrefabs(query: "Terrain")
        XCTAssertEqual(terrainPrefabs.count, 2)
    }
    
    func testSearchPrefabsByTags() async throws {
        // Save tagged prefabs
        _ = await prefabSystem.savePrefab(
            name: "Forest Terrain",
            description: "Forest terrain",
            tags: ["forest", "trees"],
            terrainData: testTerrainData
        )
        
        _ = await prefabSystem.savePrefab(
            name: "Ocean Terrain",
            description: "Ocean terrain",
            tags: ["ocean", "water"],
            terrainData: testTerrainData
        )
        
        await prefabSystem.refreshPrefabs()
        
        // Search by forest tag
        let forestPrefabs = prefabSystem.searchPrefabs(tags: ["forest"])
        XCTAssertEqual(forestPrefabs.count, 1)
        XCTAssertEqual(forestPrefabs.first?.name, "Forest Terrain")
        
        // Search by multiple tags
        let multiTagPrefabs = prefabSystem.searchPrefabs(tags: ["forest", "ocean"])
        XCTAssertEqual(multiTagPrefabs.count, 0) // No prefab has both tags
    }
    
    func testSearchPrefabsByAuthor() async throws {
        // Save prefabs with different authors
        _ = await prefabSystem.savePrefab(
            name: "Prefab A",
            description: "By Alice",
            author: "Alice",
            terrainData: testTerrainData
        )
        
        _ = await prefabSystem.savePrefab(
            name: "Prefab B",
            description: "By Bob",
            author: "Bob",
            terrainData: testTerrainData
        )
        
        await prefabSystem.refreshPrefabs()
        
        // Search by Alice
        let alicePrefabs = prefabSystem.searchPrefabs(author: "Alice")
        XCTAssertEqual(alicePrefabs.count, 1)
        XCTAssertEqual(alicePrefabs.first?.author, "Alice")
    }
    
    // MARK: - Preview Tests
    
    func testGeneratePreviewImage() throws {
        let previewImage = prefabSystem.generatePreviewImage(
            for: testTerrainData,
            size: CGSize(width: 128, height: 128)
        )
        
        XCTAssertNotNil(previewImage)
        XCTAssertEqual(previewImage?.size.width, 128)
        XCTAssertEqual(previewImage?.size.height, 128)
        
        // Verify image data
        guard let tiffData = previewImage?.tiffRepresentation else {
            XCTFail("Could not get TIFF data from preview image")
            return
        }
        
        XCTAssertGreaterThan(tiffData.count, 0)
    }
    
    func testGeneratePreviewImageWithDifferentSizes() throws {
        let sizes = [CGSize(width: 64, height: 64), CGSize(width: 256, height: 256), CGSize(width: 512, height: 512)]
        
        for size in sizes {
            let previewImage = prefabSystem.generatePreviewImage(for: testTerrainData, size: size)
            
            XCTAssertNotNil(previewImage, "Failed to generate preview for size \(size)")
            XCTAssertEqual(previewImage?.size.width, size.width)
            XCTAssertEqual(previewImage?.size.height, size.height)
        }
    }
    
    // MARK: - Performance Tests
    
    func testSaveAndLoadPerformance() async throws {
        let iterations = 10
        var saveTimes: [TimeInterval] = []
        var loadTimes: [TimeInterval] = []
        
        for i in 0..<iterations {
            let saveStartTime = CFAbsoluteTimeGetCurrent()
            
            let saveResult = await prefabSystem.savePrefab(
                name: "Performance Test \(i)",
                description: "Performance test prefab",
                terrainData: testTerrainData
            )
            
            let saveEndTime = CFAbsoluteTimeGetCurrent()
            saveTimes.append(saveEndTime - saveStartTime)
            
            guard case .success(let url) = saveResult else {
                XCTFail("Failed to save prefab \(i)")
                continue
            }
            
            let loadStartTime = CFAbsoluteTimeGetCurrent()
            
            let loadResult = await prefabSystem.loadPrefab(from: url)
            
            let loadEndTime = CFAbsoluteTimeGetCurrent()
            loadTimes.append(loadEndTime - loadStartTime)
            
            switch loadResult {
            case .success:
                break // Expected
            case .failure(let error):
                XCTFail("Failed to load prefab \(i): \(error)")
            }
        }
        
        let averageSaveTime = saveTimes.reduce(0, +) / Double(saveTimes.count)
        let averageLoadTime = loadTimes.reduce(0, +) / Double(loadTimes.count)
        
        XCTAssertLessThan(averageSaveTime, 1.0, "Save should take less than 1 second on average")
        XCTAssertLessThan(averageLoadTime, 0.5, "Load should take less than 0.5 seconds on average")
        
        print("Average save time: \(averageSaveTime)s")
        print("Average load time: \(averageLoadTime)s")
    }
    
    func testMemoryUsage() async throws {
        let initialMemory = getMemoryUsage()
        
        // Save multiple large prefabs
        for i in 0..<5 {
            _ = await prefabSystem.savePrefab(
                name: "Memory Test \(i)",
                description: "Large terrain for memory test",
                terrainData: TerrainData(size: SIMD2<Int>(512, 512))
            )
        }
        
        let afterSaveMemory = getMemoryUsage()
        
        // Load all prefabs
        await prefabSystem.refreshPrefabs()
        
        let afterLoadMemory = getMemoryUsage()
        
        // Memory usage should increase but not excessively
        let saveIncrease = afterSaveMemory - initialMemory
        let loadIncrease = afterLoadMemory - afterSaveMemory
        
        XCTAssertLessThan(saveIncrease, 100 * 1024 * 1024) // Less than 100MB increase
        XCTAssertLessThan(loadIncrease, 50 * 1024 * 1024) // Less than 50MB increase
        
        print("Memory increase after save: \(saveIncrease / 1024 / 1024)MB")
        print("Memory increase after load: \(loadIncrease / 1024 / 1024)MB")
    }
    
    // MARK: - Helper Methods
    
    private func createTestPrefab() -> TerrainPrefabSystem.TerrainPrefab {
        let compressedData = try! TerrainPrefabSystem.CompressedTerrainData(
            heightmapData: Data([1, 2, 3, 4]),
            textureMapData: Data([1, 2, 3, 4]),
            vegetationData: Data(),
            compressionFormat: .none,
            originalSize: SIMD2<Int>(2, 2)
        )
        
        let metadata = TerrainPrefabSystem.TerrainPrefab.PrefabMetadata(
            terrainSize: SIMD2<Int>(2, 2),
            heightRange: 0.0...10.0,
            totalVertices: 4,
            totalTriangles: 2,
            textureCount: 2,
            vegetationCount: 0,
            estimatedMemoryUsage: 1024,
            compatibilityVersion: "1.0"
        )
        
        return TerrainPrefabSystem.TerrainPrefab(
            id: UUID(),
            name: "Test Prefab",
            description: "Test prefab for unit testing",
            author: "Unit Test",
            version: "1.0",
            createdAt: Date(),
            modifiedAt: Date(),
            tags: ["test"],
            previewImage: nil,
            metadata: metadata,
            terrainData: compressedData,
            modifications: []
        )
    }
    
    private func getMemoryUsage() -> Int64 {
        var info = mach_task_basic_info()
        var count = mach_msg_type_number_t(MemoryLayout<mach_task_basic_info>.size)/4
        
        let kerr: kern_return_t = withUnsafeMutablePointer(to: &info) {
            $0.withMemoryRebound(to: integer_t.self, capacity: 1) {
                task_info(mach_task_self_,
                         task_flavor_t(MACH_TASK_BASIC_INFO),
                         $0,
                         &count)
            }
        }
        
        if kerr == KERN_SUCCESS {
            return Int64(info.resident_size)
        } else {
            return 0
        }
    }
}

// MARK: - Integration Tests

final class TerrainPrefabSystemIntegrationTests: XCTestCase {
    
    func testEndToEndWorkflow() async throws {
        let prefabSystem = TerrainPrefabSystem.shared
        let terrainAPI = TerrainRuntimeAPI.shared
        
        // Create and initialize terrain
        let terrainData = TerrainData(size: SIMD2<Int>(128, 128))
        try terrainAPI.initialize(with: terrainData)
        
        // Make some modifications
        let modificationResult = try await terrainAPI.modifyHeight(at: SIMD2<Float>(64, 64), delta: 10.0)
        XCTAssertTrue(modificationResult.success)
        
        // Save as prefab
        let saveResult = await prefabSystem.savePrefab(
            name: "Integration Test Prefab",
            description: "End-to-end test prefab",
            author: "Integration Test",
            tags: ["test", "integration"],
            terrainData: terrainData
        )
        
        guard case .success(let url) = saveResult else {
            XCTFail("Failed to save prefab")
            return
        }
        
        // Load prefab
        let loadResult = await prefabSystem.loadPrefab(from: url)
        
        guard case .success(let prefab) = loadResult else {
            XCTFail("Failed to load prefab")
            return
        }
        
        // Load terrain data from prefab
        let terrainResult = await prefabSystem.loadTerrainData(from: prefab)
        
        guard case .success(let loadedTerrainData) = terrainResult else {
            XCTFail("Failed to load terrain data from prefab")
            return
        }
        
        // Reinitialize with loaded terrain
        try terrainAPI.initialize(with: loadedTerrainData)
        
        // Verify terrain is loaded
        XCTAssertTrue(terrainAPI.isInitialized)
        XCTAssertEqual(terrainAPI.getHeight(at: SIMD2<Float>(64, 64)), 10.0, accuracy: 0.1)
        
        // Cleanup
        terrainAPI.shutdown()
    }
    
    func testConcurrentOperations() async throws {
        let prefabSystem = TerrainPrefabSystem.shared
        let terrainData = TerrainData(size: SIMD2<Int>(64, 64))
        
        // Create concurrent save operations
        let saveTasks = (0..<5).map { i in
            Task {
                await prefabSystem.savePrefab(
                    name: "Concurrent Prefab \(i)",
                    description: "Concurrent test prefab",
                    terrainData: terrainData
                )
            }
        }
        
        // Wait for all saves to complete
        let saveResults = await withTaskGroup(of: Result<URL, PrefabError>.self) { group in
            var results: [Result<URL, PrefabError>] = []
            
            for task in saveTasks {
                group.addTask {
                    await task.value
                }
            }
            
            for await result in group {
                results.append(result)
            }
            
            return results
        }
        
        // Verify all saves succeeded
        XCTAssertEqual(saveResults.count, 5)
        let successCount = saveResults.compactMap { try? $0.get() }.count
        XCTAssertEqual(successCount, 5)
        
        // Refresh and verify all prefabs are available
        await prefabSystem.refreshPrefabs()
        
        let concurrentPrefabs = prefabSystem.availablePrefabs.filter { prefab in
            prefab.name.contains("Concurrent Prefab")
        }
        
        XCTAssertEqual(concurrentPrefabs.count, 5)
    }
}
