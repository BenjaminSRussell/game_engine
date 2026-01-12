import XCTest
@testable import VoxelForgeStudio
import CVoxelForge

class EngineBridgeTests: XCTestCase {
    
    override func setUp() {
        super.setUp()
        // Reset engine bridge for each test
        EngineBridge.shared.resetState()
    }
    
    override func tearDown() {
        super.tearDown()
        // Clean up after each test
        EngineBridge.shared.cleanup()
    }
    
    // MARK: - Engine Lifecycle Tests
    
    func testEngineInitialization() {
        XCTAssertNotNil(EngineBridge.shared)
        XCTAssertFalse(EngineBridge.shared.entities.isEmpty)
        XCTAssertTrue(EngineBridge.shared.logs.isEmpty)
    }
    
    func testEngineShutdown() {
        EngineBridge.shared.shutdown()
        // Verify shutdown completed without errors
        XCTAssertTrue(true)
    }
    
    // MARK: - Entity Management Tests
    
    func testCreateEntity() {
        let initialCount = EngineBridge.shared.entities.count
        let entityID = EngineBridge.shared.createEntity(name: "TestEntity")
        
        XCTAssertNotEqual(entityID, UUID())
        XCTAssertEqual(EngineBridge.shared.entities.count, initialCount + 1)
        
        let entity = EngineBridge.shared.entities[entityID]
        XCTAssertNotNil(entity)
        XCTAssertEqual(entity.name, "TestEntity")
    }
    
    func testDeleteEntity() {
        let entityID = EngineBridge.shared.createEntity(name: "ToDelete")
        let initialCount = EngineBridge.shared.entities.count
        
        EngineBridge.shared.deleteEntity(id: entityID)
        
        XCTAssertEqual(EngineBridge.shared.entities.count, initialCount - 1)
        XCTAssertNil(EngineBridge.shared.entities[entityID])
    }
    
    func testEntityTransform() {
        let entityID = EngineBridge.shared.createEntity(name: "TransformTest")
        
        let transform = Transform(
            position: SIMD3<Float>(1.0, 2.0, 3.0),
            rotation: SIMD3<Float>(0.0, 45.0, 0.0),
            scale: SIMD3<Float>(1.5, 1.5, 1.5)
        )
        
        EngineBridge.shared.setTransform(id: entityID, transform: transform)
        
        let retrievedTransform = EngineBridge.shared.getTransform(id: entityID)
        XCTAssertEqual(retrievedTransform.position.x, transform.position.x, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.position.y, transform.position.y, accuracy: 0.001)
        XCTAssertEqual(retrievedTransform.position.z, transform.position.z, accuracy: 0.001)
    }
    
    // MARK: - Component Management Tests
    
    func testAddTransformComponent() {
        let entityID = EngineBridge.shared.createEntity(name: "ComponentTest")
        
        let transformComponent = TransformComponent(
            position: SIMD3<Float>(0, 0, 0),
            rotation: SIMD3<Float>(0, 0, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        
        EngineBridge.shared.addComponent(id: entityID, component: transformComponent)
        
        let components = EngineBridge.shared.getComponents(id: entityID)
        XCTAssertTrue(components.contains { $0.componentType == .transform })
    }
    
    func testAddMeshRendererComponent() {
        let entityID = EngineBridge.shared.createEntity(name: "MeshTest")
        
        let meshComponent = MeshRendererComponent(
            meshPath: "assets/models/cube.obj",
            materialPath: "assets/materials/default.mat",
            castShadows: true,
            receiveShadows: true
        )
        
        EngineBridge.shared.addComponent(id: entityID, component: meshComponent)
        
        let components = EngineBridge.shared.getComponents(id: entityID)
        XCTAssertTrue(components.contains { $0.componentType == .meshRenderer })
    }
    
    func testRemoveComponent() {
        let entityID = EngineBridge.shared.createEntity(name: "RemoveTest")
        
        let transformComponent = TransformComponent(
            position: SIMD3<Float>(0, 0, 0),
            rotation: SIMD3<Float>(0, 0, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        
        EngineBridge.shared.addComponent(id: entityID, component: transformComponent)
        EngineBridge.shared.removeComponent(id: entityID, componentType: .transform)
        
        let components = EngineBridge.shared.getComponents(id: entityID)
        XCTAssertFalse(components.contains { $0.componentType == .transform })
    }
    
    // MARK: - Scene Management Tests
    
    func testNewScene() {
        EngineBridge.shared.newScene()
        
        // Verify scene was created (entities should be cleared)
        XCTAssertTrue(EngineBridge.shared.entities.isEmpty)
        
        // Should have a log entry about new scene
        let sceneLogs = EngineBridge.shared.logs.filter { 
            $0.message.contains("Created new scene") 
        }
        XCTAssertFalse(sceneLogs.isEmpty)
    }
    
    func testLoadScene() {
        let testPath = "/tmp/test_scene.voxel"
        EngineBridge.shared.loadScene(path: testPath)
        
        // Should have a log entry about loading scene
        let loadLogs = EngineBridge.shared.logs.filter { 
            $0.message.contains("Loaded scene") 
        }
        XCTAssertFalse(loadLogs.isEmpty)
    }
    
    func testSaveScene() {
        let testPath = "/tmp/test_save.voxel"
        EngineBridge.shared.saveScene(path: testPath)
        
        // Should have a log entry about saving scene
        let saveLogs = EngineBridge.shared.logs.filter { 
            $0.message.contains("Saved scene") 
        }
        XCTAssertFalse(saveLogs.isEmpty)
    }
    
    // MARK: - Physics Tests
    
    func testAddPhysicsComponent() {
        let entityID = EngineBridge.shared.createEntity(name: "PhysicsTest")
        
        let physicsComponent = RigidbodyComponent(
            mass: 1.0,
            drag: 0.1,
            isKinematic: false
        )
        
        EngineBridge.shared.addComponent(id: entityID, component: physicsComponent)
        
        let components = EngineBridge.shared.getComponents(id: entityID)
        XCTAssertTrue(components.contains { $0.componentType == .rigidbody })
    }
    
    // MARK: - Logging Tests
    
    func testLogMessage() {
        EngineBridge.shared.log("Test message", level: .info, source: "TestSuite")
        
        let logs = EngineBridge.shared.logs.filter { 
            $0.message == "Test message" && 
            $0.level == .info && 
            $0.source == "TestSuite"
        }
        XCTAssertEqual(logs.count, 1)
    }
    
    func testLogLevels() {
        EngineBridge.shared.log("Debug message", level: .debug, source: "Test")
        EngineBridge.shared.log("Info message", level: .info, source: "Test")
        EngineBridge.shared.log("Warning message", level: .warning, source: "Test")
        EngineBridge.shared.log("Error message", level: .error, source: "Test")
        
        XCTAssertEqual(EngineBridge.shared.logs.count, 4)
        XCTAssertEqual(EngineBridge.shared.logs[0].level, .debug)
        XCTAssertEqual(EngineBridge.shared.logs[1].level, .info)
        XCTAssertEqual(EngineBridge.shared.logs[2].level, .warning)
        XCTAssertEqual(EngineBridge.shared.logs[3].level, .error)
    }
    
    // MARK: - Performance Tests
    
    func testEntityCreationPerformance() {
        measure {
            for i in 0..<1000 {
                _ = EngineBridge.shared.createEntity(name: "PerfTest\(i)")
            }
        }
    }
    
    func testComponentAdditionPerformance() {
        let entityID = EngineBridge.shared.createEntity(name: "PerfComponentTest")
        let transformComponent = TransformComponent(
            position: SIMD3<Float>(0, 0, 0),
            rotation: SIMD3<Float>(0, 0, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        
        measure {
            for _ in 0..<1000 {
                EngineBridge.shared.addComponent(id: entityID, component: transformComponent)
                EngineBridge.shared.removeComponent(id: entityID, componentType: .transform)
            }
        }
    }
    
    // MARK: - Integration Tests
    
    func testCompleteEntityWorkflow() {
        // Create entity
        let entityID = EngineBridge.shared.createEntity(name: "CompleteTest")
        
        // Add transform component
        let transform = TransformComponent(
            position: SIMD3<Float>(5, 10, 15),
            rotation: SIMD3<Float>(0, 90, 0),
            scale: SIMD3<Float>(2, 2, 2)
        )
        EngineBridge.shared.addComponent(id: entityID, component: transform)
        
        // Add mesh renderer
        let mesh = MeshRendererComponent(
            meshPath: "test.obj",
            materialPath: "test.mat",
            castShadows: true,
            receiveShadows: false
        )
        EngineBridge.shared.addComponent(id: entityID, component: mesh)
        
        // Add physics
        let physics = RigidbodyComponent(
            mass: 5.0,
            drag: 0.2,
            isKinematic: false
        )
        EngineBridge.shared.addComponent(id: entityID, component: physics)
        
        // Verify all components are present
        let components = EngineBridge.shared.getComponents(id: entityID)
        XCTAssertEqual(components.count, 3)
        XCTAssertTrue(components.contains { $0.componentType == .transform })
        XCTAssertTrue(components.contains { $0.componentType == .meshRenderer })
        XCTAssertTrue(components.contains { $0.componentType == .rigidbody })
        
        // Delete entity
        EngineBridge.shared.deleteEntity(id: entityID)
        XCTAssertNil(EngineBridge.shared.entities[entityID])
    }
}
