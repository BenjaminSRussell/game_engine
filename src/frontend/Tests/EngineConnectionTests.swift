import XCTest
@testable import VoxelForgeStudio
import CVoxelForge

class EngineConnectionTests: XCTestCase {
    
    // MARK: - Basic Connection Tests
    
    func testEngineBridgeConnection() {
        // Test that the engine bridge can be instantiated
        let bridge = EngineBridge.shared
        XCTAssertNotNil(bridge)
        
        // Test basic engine functions are callable
        bridge.newScene()
        XCTAssertTrue(bridge.entities.isEmpty)
    }
    
    func testCCallbacks() {
        // Test that C callbacks are properly connected
        let bridge = EngineBridge.shared
        
        // Create an entity to trigger callbacks
        let entityID = bridge.createEntity(name: "CallbackTest")
        
        // Should have logs from entity creation
        let creationLogs = bridge.logs.filter { log in
            log.message.contains("Created entity") || log.source.contains("Engine")
        }
        XCTAssertFalse(creationLogs.isEmpty)
        
        // Delete entity to trigger deletion callback
        bridge.deleteEntity(id: entityID)
        
        let deletionLogs = bridge.logs.filter { log in
            log.message.contains("Deleted entity") || log.source.contains("Engine")
        }
        XCTAssertFalse(deletionLogs.isEmpty)
    }
    
    // MARK: - Data Structure Tests
    
    func testEntityDataStructure() {
        let bridge = EngineBridge.shared
        let entityID = bridge.createEntity(name: "DataTest")
        
        // Test entity data structure
        let entity = bridge.entities[entityID]
        XCTAssertNotNil(entity)
        XCTAssertEqual(entity.name, "DataTest")
        XCTAssertNotNil(entity.id)
        XCTAssertTrue(entity.components.isEmpty)
    }
    
    func testComponentDataStructure() {
        let bridge = EngineBridge.shared
        let entityID = bridge.createEntity(name: "ComponentDataTest")
        
        // Add transform component
        let transform = TransformComponent(
            position: SIMD3<Float>(1, 2, 3),
            rotation: SIMD3<Float>(45, 90, 0),
            scale: SIMD3<Float>(2, 2, 2)
        )
        
        bridge.addComponent(id: entityID, component: transform)
        
        // Test component data structure
        let components = bridge.getComponents(id: entityID)
        XCTAssertEqual(components.count, 1)
        
        let transformComponent = components.first { $0.componentType == .transform }
        XCTAssertNotNil(transformComponent)
        
        if let transformData = transformComponent?.data as? [String: Any] {
            XCTAssertEqual(transformData["position"] as? SIMD3<Float>, SIMD3<Float>(1, 2, 3))
            XCTAssertEqual(transformData["rotation"] as? SIMD3<Float>, SIMD3<Float>(45, 90, 0))
            XCTAssertEqual(transformData["scale"] as? SIMD3<Float>, SIMD3<Float>(2, 2, 2))
        }
    }
    
    // MARK: - Type Conversion Tests
    
    func testSwiftToCTypeConversion() {
        let bridge = EngineBridge.shared
        let entityID = bridge.createEntity(name: "TypeTest")
        
        // Create Swift types
        let swiftTransform = Transform(
            position: SIMD3<Float>(1.5, 2.5, 3.5),
            rotation: SIMD3<Float>(30, 60, 90),
            scale: SIMD3<Float>(0.5, 1.5, 2.5)
        )
        
        // Set transform (converts to C internally)
        bridge.setTransform(id: entityID, transform: swiftTransform)
        
        // Get transform (converts from C internally)
        let retrievedTransform = bridge.getTransform(id: entityID)
        
        // Verify type conversion worked correctly
        XCTAssertEqual(retrievedTransform.position.x, swiftTransform.position.x, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.position.y, swiftTransform.position.y, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.position.z, swiftTransform.position.z, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.rotation.x, swiftTransform.rotation.x, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.rotation.y, swiftTransform.rotation.y, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.rotation.z, swiftTransform.rotation.z, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.scale.x, swiftTransform.scale.x, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.scale.y, swiftTransform.scale.y, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.scale.z, swiftTransform.scale.z, accuracy: 0.001)
    }
    
    // MARK: - Memory Safety Tests
    
    func testMemorySafety() {
        let bridge = EngineBridge.shared
        
        // Test that operations on deleted entities don't crash
        let entityID = bridge.createEntity(name: "MemoryTest")
        bridge.deleteEntity(id: entityID)
        
        // These operations should not crash
        let transform = bridge.getTransform(id: entityID)
        let components = bridge.getComponents(id: entityID)
        bridge.deleteEntity(id: entityID)
        bridge.removeComponent(id: entityID, componentType: .transform)
        
        // Should return default values
        XCTAssertEqual(transform.position, SIMD3<Float>(0, 0, 0))
        XCTAssertTrue(components.isEmpty)
    }
    
    // MARK: - Performance Tests
    
    func testBridgePerformance() {
        let bridge = EngineBridge.shared
        
        measure {
            // Test entity creation performance
            for i in 0..<100 {
                let entityID = bridge.createEntity(name: "PerfTest\(i)")
                
                // Add transform component
                let transform = TransformComponent(
                    position: SIMD3<Float>(Float(i), 0, 0),
                    rotation: SIMD3<Float>(0, 0, 0),
                    scale: SIMD3<Float>(1, 1, 1)
                )
                bridge.addComponent(id: entityID, component: transform)
                
                // Delete entity
                bridge.deleteEntity(id: entityID)
            }
        }
    }
    
    // MARK: - Error Handling Tests
    
    func testErrorHandling() {
        let bridge = EngineBridge.shared
        
        // Test operations with invalid data
        let invalidID = UUID()
        
        // These should not crash and should return reasonable defaults
        let transform = bridge.getTransform(id: invalidID)
        let components = bridge.getComponents(id: invalidID)
        
        XCTAssertEqual(transform.position, SIMD3<Float>(0, 0, 0))
        XCTAssertTrue(components.isEmpty)
        
        // Test component operations on non-existent entities
        bridge.removeComponent(id: invalidID, componentType: .transform)
        
        // Should not crash
        XCTAssertTrue(true)
    }
    
    // MARK: - Concurrency Tests
    
    func testConcurrentAccess() {
        let bridge = EngineBridge.shared
        let expectation = XCTestExpectation(description: "Concurrent access")
        
        let concurrentOperations = 20
        var completedOperations = 0
        
        for i in 0..<concurrentOperations {
            DispatchQueue.global(qos: .background).async {
                let entityID = bridge.createEntity(name: "Concurrent\(i)")
                
                // Perform operations concurrently
                let transform = TransformComponent(
                    position: SIMD3<Float>(Float(i), Float(i), Float(i)),
                    rotation: SIMD3<Float>(0, 0, 0),
                    scale: SIMD3<Float>(1, 1, 1)
                )
                bridge.addComponent(id: entityID, component: transform)
                
                let retrievedTransform = bridge.getTransform(id: entityID)
                XCTAssertEqual(retrievedTransform.position.x, Float(i), accuracy: 0.001)
                
                DispatchQueue.main.async {
                    completedOperations += 1
                    if completedOperations == concurrentOperations {
                        expectation.fulfill()
                    }
                }
            }
        }
        
        wait(for: [expectation], timeout: 5.0)
        
        // Verify all operations completed
        XCTAssertEqual(bridge.entities.count, concurrentOperations)
    }
    
    // MARK: - Integration Tests
    
    func testCompleteWorkflow() {
        let bridge = EngineBridge.shared
        
        // 1. Create new scene
        bridge.newScene()
        XCTAssertTrue(bridge.entities.isEmpty)
        
        // 2. Create entities with components
        let entity1 = bridge.createEntity(name: "TestEntity1")
        let entity2 = bridge.createEntity(name: "TestEntity2")
        
        // 3. Add components
        let transform1 = TransformComponent(
            position: SIMD3<Float>(0, 0, 0),
            rotation: SIMD3<Float>(0, 0, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        bridge.addComponent(id: entity1, component: transform1)
        
        let mesh1 = MeshRendererComponent(
            meshPath: "test.obj",
            materialPath: "test.mat",
            castShadows: true,
            receiveShadows: false
        )
        bridge.addComponent(id: entity1, component: mesh1)
        
        let transform2 = TransformComponent(
            position: SIMD3<Float>(5, 0, 0),
            rotation: SIMD3<Float>(0, 45, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        bridge.addComponent(id: entity2, component: transform2)
        
        // 4. Verify state
        XCTAssertEqual(bridge.entities.count, 2)
        XCTAssertEqual(bridge.getComponents(id: entity1).count, 2)
        XCTAssertEqual(bridge.getComponents(id: entity2).count, 1)
        
        // 5. Save scene
        bridge.saveScene(path: "/tmp/complete_test.voxel")
        
        // 6. Clean up
        bridge.deleteEntity(id: entity1)
        bridge.deleteEntity(id: entity2)
        
        XCTAssertEqual(bridge.entities.count, 0)
    }
}
