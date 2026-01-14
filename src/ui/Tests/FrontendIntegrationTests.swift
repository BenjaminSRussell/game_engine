import XCTest
@testable import VoxelForgeStudio
import SwiftUI

class FrontendIntegrationTests: XCTestCase {
    
    // MARK: - UI Integration Tests
    
    func testContentViewInitialization() {
        let contentView = ContentView()
        XCTAssertNotNil(contentView)
    }
    
    func testProjectExplorerView() {
        let projectExplorer = ProjectExplorerView()
        XCTAssertNotNil(projectExplorer)
    }
    
    func testDocumentationView() {
        let documentationView = DocumentationView()
        XCTAssertNotNil(documentationView)
    }
    
    // MARK: - Engine Bridge Integration Tests
    
    func testEngineBridgeUIIntegration() {
        let bridge = EngineBridge.shared
        
        // Test that UI can interact with engine bridge
        let entityID = bridge.createEntity(name: "UITestEntity")
        XCTAssertNotNil(entityID)
        
        // Test transform updates from UI
        let transform = Transform(
            position: SIMD3<Float>(1, 2, 3),
            rotation: SIMD3<Float>(0, 45, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        bridge.setTransform(id: entityID, transform: transform)
        
        let retrievedTransform = bridge.getTransform(id: entityID)
        XCTAssertEqual(retrievedTransform.position.x, 1.0, accuracy: 0.001)
    }
    
    // MARK: - Scene Management Integration Tests
    
    func testSceneManagementUI() {
        let bridge = EngineBridge.shared
        
        // Test new scene creation
        bridge.newScene()
        XCTAssertTrue(bridge.entities.isEmpty)
        
        // Add some entities
        let entity1 = bridge.createEntity(name: "Entity1")
        let entity2 = bridge.createEntity(name: "Entity2")
        
        XCTAssertEqual(bridge.entities.count, 2)
        
        // Test scene saving
        bridge.saveScene(path: "/tmp/test_scene.voxel")
        
        // Verify logs contain scene operations
        let sceneLogs = bridge.logs.filter { log in
            log.message.contains("scene") || log.source.contains("Scene")
        }
        XCTAssertFalse(sceneLogs.isEmpty)
    }
    
    // MARK: - Component System Integration Tests
    
    func testComponentSystemUI() {
        let bridge = EngineBridge.shared
        let entityID = bridge.createEntity(name: "ComponentTest")
        
        // Test adding components through UI-like operations
        let transformComponent = TransformComponent(
            position: SIMD3<Float>(0, 0, 0),
            rotation: SIMD3<Float>(0, 0, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        bridge.addComponent(id: entityID, component: transformComponent)
        
        let meshComponent = MeshRendererComponent(
            meshPath: "assets/models/test.obj",
            materialPath: "assets/materials/test.mat",
            castShadows: true,
            receiveShadows: true
        )
        bridge.addComponent(id: entityID, component: meshComponent)
        
        // Verify components can be retrieved for UI display
        let components = bridge.getComponents(id: entityID)
        XCTAssertEqual(components.count, 2)
        
        // Test component removal
        bridge.removeComponent(id: entityID, componentType: .transform)
        let remainingComponents = bridge.getComponents(id: entityID)
        XCTAssertEqual(remainingComponents.count, 1)
        XCTAssertEqual(remainingComponents[0].componentType, .meshRenderer)
    }
    
    // MARK: - Selection System Integration Tests
    
    func testSelectionSystemUI() {
        let bridge = EngineBridge.shared
        let selectionManager = SelectionManager.shared
        
        // Create test entities
        let entity1 = bridge.createEntity(name: "Selectable1")
        let entity2 = bridge.createEntity(name: "Selectable2")
        
        // Set up transforms for selection testing
        let transform1 = Transform(
            position: SIMD3<Float>(0, 0, 0),
            rotation: SIMD3<Float>(0, 0, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        let transform2 = Transform(
            position: SIMD3<Float>(5, 0, 0),
            rotation: SIMD3<Float>(0, 0, 0),
            scale: SIMD3<Float>(1, 1, 1)
        )
        
        bridge.setTransform(id: entity1, transform: transform1)
        bridge.setTransform(id: entity2, transform: transform2)
        
        // Test selection
        selectionManager.select(entity1)
        XCTAssertTrue(selectionManager.isSelected(entity1))
        XCTAssertFalse(selectionManager.isSelected(entity2))
        
        // Test multi-selection
        selectionManager.select(entity2, addToSelection: true)
        XCTAssertTrue(selectionManager.isSelected(entity1))
        XCTAssertTrue(selectionManager.isSelected(entity2))
        
        // Test selection clearing
        selectionManager.clearSelection()
        XCTAssertFalse(selectionManager.isSelected(entity1))
        XCTAssertFalse(selectionManager.isSelected(entity2))
    }
    
    // MARK: - Performance Integration Tests
    
    func testUIPerformanceWithLargeScene() {
        let bridge = EngineBridge.shared
        let selectionManager = SelectionManager.shared
        
        // Create a large number of entities
        let entityCount = 1000
        var entities: [UUID] = []
        
        measure {
            // Create entities
            for i in 0..<entityCount {
                let entityID = bridge.createEntity(name: "PerfEntity\(i)")
                entities.append(entityID)
                
                // Add transform component
                let transform = TransformComponent(
                    position: SIMD3<Float>(Float(i), 0, 0),
                    rotation: SIMD3<Float>(0, 0, 0),
                    scale: SIMD3<Float>(1, 1, 1)
                )
                bridge.addComponent(id: entityID, component: transform)
            }
            
            // Test selection performance
            for entityID in entities.prefix(100) {
                selectionManager.select(entityID, addToSelection: true)
            }
            
            // Clear up
            selectionManager.clearSelection()
            for entityID in entities {
                bridge.deleteEntity(id: entityID)
            }
        }
    }
    
    // MARK: - Error Handling Tests
    
    func testErrorHandlingInUI() {
        let bridge = EngineBridge.shared
        
        // Test operations on non-existent entities
        let nonExistentID = UUID()
        
        // These should not crash the UI
        bridge.deleteEntity(id: nonExistentID)
        let transform = bridge.getTransform(id: nonExistentID)
        XCTAssertEqual(transform.position, SIMD3<Float>(0, 0, 0)) // Default transform
        
        let components = bridge.getComponents(id: nonExistentID)
        XCTAssertTrue(components.isEmpty)
        
        // Test invalid component operations
        let validID = bridge.createEntity(name: "ValidEntity")
        bridge.removeComponent(id: validID, componentType: .transform) // Non-existent component
        
        // Should not crash
        XCTAssertTrue(true)
    }
    
    // MARK: - Memory Management Tests
    
    func testMemoryManagement() {
        let bridge = EngineBridge.shared
        
        // Test creating and deleting many entities
        let iterations = 100
        
        for _ in 0..<iterations {
            let entityID = bridge.createEntity(name: "TempEntity")
            
            // Add components
            let transform = TransformComponent(
                position: SIMD3<Float>(0, 0, 0),
                rotation: SIMD3<Float>(0, 0, 0),
                scale: SIMD3<Float>(1, 1, 1)
            )
            bridge.addComponent(id: entityID, component: transform)
            
            // Delete entity
            bridge.deleteEntity(id: entityID)
        }
        
        // Verify cleanup
        XCTAssertEqual(bridge.entities.count, 0)
    }
    
    // MARK: - Concurrency Tests
    
    func testConcurrentOperations() {
        let bridge = EngineBridge.shared
        let expectation = XCTestExpectation(description: "Concurrent operations")
        
        let concurrentOperations = 10
        var completedOperations = 0
        
        for i in 0..<concurrentOperations {
            DispatchQueue.global(qos: .background).async {
                let entityID = bridge.createEntity(name: "ConcurrentEntity\(i)")
                
                let transform = TransformComponent(
                    position: SIMD3<Float>(Float(i), 0, 0),
                    rotation: SIMD3<Float>(0, 0, 0),
                    scale: SIMD3<Float>(1, 1, 1)
                )
                bridge.addComponent(id: entityID, component: transform)
                
                DispatchQueue.main.async {
                    completedOperations += 1
                    if completedOperations == concurrentOperations {
                        expectation.fulfill()
                    }
                }
            }
        }
        
        wait(for: [expectation], timeout: 5.0)
        
        // Verify all entities were created
        XCTAssertEqual(bridge.entities.count, concurrentOperations)
    }
}
