import XCTest
@testable import Engine

final class TerrainRuntimeAPITests: XCTestCase {
    
    var terrainAPI: TerrainRuntimeAPI!
    var testTerrainData: TerrainData!
    
    override func setUp() async throws {
        try await super.setUp()
        
        terrainAPI = TerrainRuntimeAPI.shared
        testTerrainData = TerrainData(size: SIMD2<Int>(100, 100))
        
        // Initialize with some test data
        for y in 0..<100 {
            for x in 0..<100 {
                testTerrainData.heightmap[y][x] = Float(sin(Double(x) * 0.1) * cos(Double(y) * 0.1)) * 10
                testTerrainData.textureMap[y][x] = (x + y) % 4
            }
        }
        
        try terrainAPI.initialize(with: testTerrainData)
    }
    
    override func tearDown() async throws {
        terrainAPI.shutdown()
        terrainAPI = nil
        testTerrainData = nil
        
        try await super.tearDown()
    }
    
    // MARK: - Initialization Tests
    
    func testInitialization() throws {
        XCTAssertTrue(terrainAPI.isInitialized)
        XCTAssertNotNil(terrainAPI.terrainData)
    }
    
    func testDoubleInitialization() throws {
        XCTAssertThrowsError(try terrainAPI.initialize(with: testTerrainData)) { error in
            XCTAssertEqual(error as? TerrainAPIError, .alreadyInitialized)
        }
    }
    
    // MARK: - Height Modification Tests
    
    func testModifyHeight() async throws {
        let position = SIMD2<Float>(50, 50)
        let initialHeight = terrainAPI.getHeight(at: position)
        
        let result = try await terrainAPI.modifyHeight(at: position, delta: 5.0)
        
        XCTAssertTrue(result.success)
        XCTAssertEqual(result.affectedArea.width, 2.0) // radius * 2
        XCTAssertEqual(result.affectedArea.height, 2.0)
        
        let newHeight = terrainAPI.getHeight(at: position)
        XCTAssertEqual(newHeight, initialHeight! + 5.0, accuracy: 0.1)
    }
    
    func testSetHeight() async throws {
        let position = SIMD2<Float>(25, 25)
        let targetHeight: Float = 100.0
        
        let result = try await terrainAPI.setHeight(at: position, height: targetHeight)
        
        XCTAssertTrue(result.success)
        let currentHeight = terrainAPI.getHeight(at: position)
        XCTAssertEqual(currentHeight, targetHeight, accuracy: 0.1)
    }
    
    func testModifyHeightRegion() async throws {
        let rect = CGRect(x: 10, y: 10, width: 20, height: 20)
        let delta: Float = 10.0
        
        let result = try await terrainAPI.modifyHeightRegion(rect: rect, delta: delta)
        
        XCTAssertTrue(result.success)
        XCTAssertEqual(result.affectedArea, rect)
    }
    
    // MARK: - Texture Modification Tests
    
    func testPaintTexture() async throws {
        let position = SIMD2<Float>(30, 30)
        let textureIndex = 2
        
        let result = try await terrainAPI.paintTexture(at: position, textureIndex: textureIndex)
        
        XCTAssertTrue(result.success)
        // Note: Actual texture modification would require terrain renderer integration
    }
    
    func testPaintTextureRegion() async throws {
        let rect = CGRect(x: 5, y: 5, width: 15, height: 15)
        let textureIndex = 3
        
        let result = try await terrainAPI.paintTextureRegion(rect: rect, textureIndex: textureIndex)
        
        XCTAssertTrue(result.success)
        XCTAssertEqual(result.affectedArea, rect)
    }
    
    // MARK: - Vegetation Modification Tests
    
    func testAddVegetation() async throws {
        let position = SIMD2<Float>(40, 40)
        let type: VegetationType = .oak
        
        let result = try await terrainAPI.addVegetation(at: position, type: type)
        
        XCTAssertTrue(result.success)
        XCTAssertEqual(terrainAPI.activeModifications.count, 0) // Should be cleared after execution
    }
    
    func testRemoveVegetation() async throws {
        let position = SIMD2<Float>(60, 60)
        let radius: Float = 5.0
        
        let result = try await terrainAPI.removeVegetation(at: position, radius: radius)
        
        XCTAssertTrue(result.success)
        XCTAssertEqual(result.affectedArea.width, radius * 2)
        XCTAssertEqual(result.affectedArea.height, radius * 2)
    }
    
    // MARK: - Erosion Tests
    
    func testHydraulicErosion() async throws {
        let position = SIMD2<Float>(70, 70)
        let intensity: Float = 0.5
        let iterations = 5
        
        let result = try await terrainAPI.applyHydraulicErosion(at: position, intensity: intensity, iterations: iterations)
        
        XCTAssertTrue(result.success)
        XCTAssertGreaterThan(result.executionTime, 0)
    }
    
    func testThermalErosion() async throws {
        let position = SIMD2<Float>(80, 80)
        let intensity: Float = 0.3
        
        let result = try await terrainAPI.applyThermalErosion(at: position, intensity: intensity)
        
        XCTAssertTrue(result.success)
    }
    
    // MARK: - Query Tests
    
    func testGetHeight() {
        let position = SIMD2<Float>(10, 10)
        let height = terrainAPI.getHeight(at: position)
        
        XCTAssertNotNil(height)
        XCTAssertIsFinite(height!)
    }
    
    func testGetHeightOutOfBounds() {
        let position = SIMD2<Float>(-10, -10)
        let height = terrainAPI.getHeight(at: position)
        
        XCTAssertNil(height)
    }
    
    func testGetTexture() {
        let position = SIMD2<Float>(15, 15)
        let texture = terrainAPI.getTexture(at: position)
        
        XCTAssertNotNil(texture)
        XCTAssertGreaterThanOrEqual(texture!, 0)
        XCTAssertLessThan(texture!, 4)
    }
    
    func testGetNormal() {
        let position = SIMD2<Float>(20, 20)
        let normal = terrainAPI.getNormal(at: position)
        
        XCTAssertNotNil(normal)
        XCTAssertEqual(length(normal!), 1.0, accuracy: 0.01) // Should be normalized
    }
    
    func testGetSlope() {
        let position = SIMD2<Float>(25, 25)
        let slope = terrainAPI.getSlope(at: position)
        
        XCTAssertNotNil(slope)
        XCTAssertGreaterThanOrEqual(slope!, 0)
        XCTAssertLessThanOrEqual(slope!, .pi) // Slope should be between 0 and π
    }
    
    // MARK: - Batch Operations Tests
    
    func testBatchExecution() async throws {
        let modifications = [
            TerrainModification(
                id: UUID(),
                type: .heightModification,
                position: SIMD2<Float>(10, 10),
                parameters: ["delta": 5.0, "radius": 1.0],
                timestamp: Date()
            ),
            TerrainModification(
                id: UUID(),
                type: .heightModification,
                position: SIMD2<Float>(20, 20),
                parameters: ["delta": -3.0, "radius": 1.0],
                timestamp: Date()
            )
        ]
        
        let results = try await terrainAPI.executeBatch(modifications)
        
        XCTAssertEqual(results.count, 2)
        XCTAssertTrue(results.allSatisfy { $0.success })
    }
    
    func testApplyHeightmap() async throws {
        let heightmap: [[Float]] = [
            [1.0, 2.0, 3.0],
            [4.0, 5.0, 6.0],
            [7.0, 8.0, 9.0]
        ]
        
        let result = try await terrainAPI.applyHeightmap(heightmap, offset: SIMD2<Float>(0, 0), scale: 1.0)
        
        XCTAssertTrue(result.success)
        XCTAssertEqual(result.affectedArea.width, 3.0)
        XCTAssertEqual(result.affectedArea.height, 3.0)
    }
    
    // MARK: - History Management Tests
    
    func testUndo() async throws {
        let position = SIMD2<Float>(35, 35)
        let initialHeight = terrainAPI.getHeight(at: position)
        
        // Perform a modification
        _ = try await terrainAPI.modifyHeight(at: position, delta: 10.0)
        let modifiedHeight = terrainAPI.getHeight(at: position)
        
        // Undo the modification
        let undoResult = try await terrainAPI.undo()
        
        XCTAssertNotNil(undoResult)
        XCTAssertTrue(undoResult!.success)
        
        let undoHeight = terrainAPI.getHeight(at: position)
        XCTAssertEqual(undoHeight, initialHeight, accuracy: 0.1)
        XCTAssertNotEqual(undoHeight, modifiedHeight)
    }
    
    func testUndoEmptyHistory() async throws {
        let undoResult = try await terrainAPI.undo()
        XCTAssertNil(undoResult)
    }
    
    func testClearHistory() async throws {
        // Perform some modifications
        _ = try await terrainAPI.modifyHeight(at: SIMD2<Float>(10, 10), delta: 5.0)
        _ = try await terrainAPI.modifyHeight(at: SIMD2<Float>(20, 20), delta: 3.0)
        
        XCTAssertGreaterThan(terrainAPI.modificationHistory.count, 0)
        
        terrainAPI.clearHistory()
        XCTAssertEqual(terrainAPI.modificationHistory.count, 0)
    }
    
    // MARK: - Statistics Tests
    
    func testAPIStatistics() async throws {
        let initialStats = terrainAPI.apiStatistics
        
        // Perform some modifications
        _ = try await terrainAPI.modifyHeight(at: SIMD2<Float>(10, 10), delta: 5.0)
        _ = try await terrainAPI.modifyHeight(at: SIMD2<Float>(20, 20), delta: 3.0)
        
        let updatedStats = terrainAPI.apiStatistics
        
        XCTAssertEqual(updatedStats.totalModifications, initialStats.totalModifications + 2)
        XCTAssertEqual(updatedStats.successfulModifications, initialStats.successfulModifications + 2)
        XCTAssertGreaterThan(updatedStats.averageExecutionTime, 0)
        XCTAssertNotNil(updatedStats.lastModificationTime)
    }
    
    // MARK: - Error Handling Tests
    
    func testModificationBeforeInitialization() async throws {
        let uninitializedAPI = TerrainRuntimeAPI()
        
        await XCTAssertThrowsError(try await uninitializedAPI.modifyHeight(at: SIMD2<Float>(0, 0), delta: 5.0)) { error in
            XCTAssertEqual(error as? TerrainAPIError, .notInitialized)
        }
    }
    
    func testInvalidPosition() async throws {
        let invalidPosition = SIMD2<Float>(-1000, -1000)
        
        // This should not crash but may return an error or handle gracefully
        let result = try await terrainAPI.modifyHeight(at: invalidPosition, delta: 5.0)
        
        // The API should handle invalid positions gracefully
        // Either succeed with no effect or fail with appropriate error
    }
    
    // MARK: - Performance Tests
    
    func testPerformanceBatchModifications() async throws {
        let modifications = (0..<100).map { i in
            TerrainModification(
                id: UUID(),
                type: .heightModification,
                position: SIMD2<Float>(Float(i), Float(i)),
                parameters: ["delta": 1.0, "radius": 1.0],
                timestamp: Date()
            )
        }
        
        let startTime = CFAbsoluteTimeGetCurrent()
        let results = try await terrainAPI.executeBatch(modifications)
        let executionTime = CFAbsoluteTimeGetCurrent() - startTime
        
        XCTAssertEqual(results.count, 100)
        XCTAssertTrue(results.allSatisfy { $0.success })
        XCTAssertLessThan(executionTime, 5.0) // Should complete within 5 seconds
    }
    
    func testMemoryUsage() async throws {
        let initialMemory = terrainAPI.apiStatistics.memoryUsage
        
        // Perform memory-intensive operations
        let largeHeightmap = Array(repeating: Array(repeating: Float.random(in: 0...100), count: 1000), count: 1000)
        _ = try await terrainAPI.applyHeightmap(largeHeightmap)
        
        let finalMemory = terrainAPI.apiStatistics.memoryUsage
        XCTAssertGreaterThan(finalMemory, initialMemory)
    }
}

// MARK: - Integration Tests

final class TerrainRuntimeAPIIntegrationTests: XCTestCase {
    
    func testEndToEndWorkflow() async throws {
        let terrainAPI = TerrainRuntimeAPI.shared
        let terrainData = TerrainData(size: SIMD2<Int>(50, 50))
        
        // Initialize
        try terrainAPI.initialize(with: terrainData)
        XCTAssertTrue(terrainAPI.isInitialized)
        
        // Perform series of modifications
        let position1 = SIMD2<Float>(10, 10)
        let position2 = SIMD2<Float>(20, 20)
        let position3 = SIMD2<Float>(30, 30)
        
        let result1 = try await terrainAPI.modifyHeight(at: position1, delta: 5.0)
        let result2 = try await terrainAPI.paintTexture(at: position2, textureIndex: 2)
        let result3 = try await terrainAPI.addVegetation(at: position3, type: .oak)
        
        XCTAssertTrue(result1.success)
        XCTAssertTrue(result2.success)
        XCTAssertTrue(result3.success)
        
        // Verify modifications
        let height1 = terrainAPI.getHeight(at: position1)
        XCTAssertNotNil(height1)
        
        let texture2 = terrainAPI.getTexture(at: position2)
        XCTAssertNotNil(texture2)
        
        let normal3 = terrainAPI.getNormal(at: position3)
        XCTAssertNotNil(normal3)
        
        // Check statistics
        XCTAssertGreaterThan(terrainAPI.apiStatistics.totalModifications, 2)
        XCTAssertEqual(terrainAPI.apiStatistics.successfulModifications, 3)
        
        // Cleanup
        terrainAPI.shutdown()
        XCTAssertFalse(terrainAPI.isInitialized)
    }
    
    func testConcurrentModifications() async throws {
        let terrainAPI = TerrainRuntimeAPI.shared
        let terrainData = TerrainData(size: SIMD2<Int>(100, 100))
        
        try terrainAPI.initialize(with: terrainData)
        
        // Create concurrent modifications
        let tasks = (0..<10).map { i in
            Task {
                try! await terrainAPI.modifyHeight(at: SIMD2<Float>(Float(i * 10), Float(i * 10)), delta: Float(i))
            }
        }
        
        // Wait for all tasks to complete
        for task in tasks {
            await task.value
        }
        
        // Verify all modifications completed successfully
        XCTAssertEqual(terrainAPI.apiStatistics.totalModifications, 10)
        XCTAssertEqual(terrainAPI.apiStatistics.successfulModifications, 10)
        
        terrainAPI.shutdown()
    }
}
