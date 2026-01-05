import XCTest
@testable import VoxelForgeEditor

/// Swift/C++ Interoperability Test Suite
/// Validates direct C function calls, type bridging, and thread safety
final class SwiftCInteropTests: XCTestCase {
    
    var testWorld: UnsafeMutablePointer<World>!
    
    override func setUp() {
        super.setUp()
        // Initialize minimal test world
        testWorld = UnsafeMutablePointer<World>.allocate(capacity: 1)
        testWorld.initialize(to: World())
        ecs_world_init(testWorld, 1000, 50, 10)
    }
    
    override func tearDown() {
        ecs_world_free(testWorld)
        testWorld.deinitialize(count: 1)
        testWorld.deallocate()
        super.tearDown()
    }
    
    // MARK: - Basic C Function Calls
    
    func testEntityCreation() {
        // Test direct C function call from Swift
        let entity = ecs_create_entity(testWorld)
        
        XCTAssertNotEqual(entity.id, 0, "Entity ID should be non-zero")
        XCTAssertTrue(ecs_is_valid(testWorld, entity), "Created entity should be valid")
    }
    
    func testMultipleEntityCreation() {
        var entities: [Entity] = []
        
        for _ in 0..<100 {
            let entity = ecs_create_entity(testWorld)
            entities.append(entity)
        }
        
        XCTAssertEqual(entities.count, 100)
        
        // Verify all entities are unique and valid
        let uniqueIds = Set(entities.map { $0.id })
        XCTAssertEqual(uniqueIds.count, 100, "All entity IDs should be unique")
        
        for entity in entities {
            XCTAssertTrue(ecs_is_valid(testWorld, entity))
        }
    }
    
    // MARK: - Swift Type Wrappers
    
    func testSwiftEntityWrapper() {
        let cEntity = ecs_create_entity(testWorld)
        let swiftEntity = SwiftEntity(from: cEntity)
        
        XCTAssertEqual(swiftEntity.id, cEntity.id, "Swift wrapper should preserve ID")
        XCTAssertEqual(swiftEntity.generation, cEntity.generation, "Swift wrapper should preserve generation")
        XCTAssertTrue(swiftEntity.isValid, "Swift entity should report as valid")
    }
    
    func testSwiftEntityRoundTrip() {
        let cEntity = ecs_create_entity(testWorld)
        let swiftEntity = SwiftEntity(from: cEntity)
        let roundTrip = swiftEntity.toCEntity()
        
        XCTAssertEqual(roundTrip.id, cEntity.id, "Round-trip conversion should preserve ID")
        XCTAssertEqual(roundTrip.generation, cEntity.generation, "Round-trip conversion should preserve generation")
    }
    
    func testSwiftEntityInvalid() {
        let invalid = SwiftEntity.invalid
        
        XCTAssertFalse(invalid.isValid, "Invalid entity should report as invalid")
        XCTAssertEqual(invalid.id, 0, "Invalid entity should have ID 0")
    }
    
    func testSwiftEntityHashable() {
        let entity1 = SwiftEntity(from: ecs_create_entity(testWorld))
        let entity2 = SwiftEntity(from: ecs_create_entity(testWorld))
        
        var entitySet = Set<SwiftEntity>()
        entitySet.insert(entity1)
        entitySet.insert(entity2)
        entitySet.insert(entity1) // Duplicate
        
        XCTAssertEqual(entitySet.count, 2, "Set should contain 2 unique entities")
    }
    
    // MARK: - Collection Protocol Conformance
    
    func testEntityCollectionIteration() {
        // Create test entities
        let entityCount = 50
        for _ in 0..<entityCount {
            _ = ecs_create_entity(testWorld)
        }
        
        // Test Collection conformance
        let collection = EntityCollection(world: testWorld)
        XCTAssertEqual(collection.count, entityCount, "Collection should contain all entities")
        
        var iteratedCount = 0
        for entity in collection {
            XCTAssertTrue(entity.isValid, "Each entity in collection should be valid")
            iteratedCount += 1
        }
        
        XCTAssertEqual(iteratedCount, entityCount, "Should iterate through all entities")
    }
    
    func testEntityCollectionRandomAccess() {
        for _ in 0..<10 {
            _ = ecs_create_entity(testWorld)
        }
        
        let collection = EntityCollection(world: testWorld)
        
        // Test subscript access
        if collection.count > 5 {
            let entity = collection[5]
            XCTAssertTrue(entity.isValid)
        }
        
        // Test first/last
        XCTAssertNotNil(collection.first)
        XCTAssertNotNil(collection.last)
    }
    
    // MARK: - Actor Thread Safety
    
    func testWorldManagerActorAccess() async {
        let manager = WorldManager(world: testWorld)
        
        // Test async entity access
        let entities = await manager.getAllEntities()
        XCTAssertNotNil(entities, "WorldManager should return entities")
    }
    
    func testWorldManagerThreadSafe() async {
        let manager = WorldManager(world: testWorld)
        
        // Create entities from multiple tasks (validates actor isolation)
        await withTaskGroup(of: Void.self) { group in
            for _ in 0..<10 {
                group.addTask {
                    let entities = await manager.getAllEntities()
                    XCTAssertNotNil(entities)
                }
            }
        }
        
        // If this completes without data race warnings, actor isolation works
    }
    
    func testWorldManagerComponentAccess() async {
        let manager = WorldManager(world: testWorld)
        let entities = await manager.getAllEntities()
        
        if let firstEntity = entities.first {
            // Test component query (even if no components exist)
            let hasComponent = await manager.hasComponent(firstEntity, type: 1)
            XCTAssertFalse(hasComponent, "Entity without components should return false")
        }
    }
    
    // MARK: - Bridge API Functions
    
    func testRendererBridgeCreation() {
        guard let metalRenderer = MetalRenderer() else {
            XCTFail("Failed to create Metal renderer")
            return
        }
        
        let device = metalRenderer.metalDevice
        let bridge = renderer_bridge_create(Unmanaged.passUnretained(device as AnyObject).toOpaque())
        
        XCTAssertNotNil(bridge, "Renderer bridge should be created successfully")
        
        if let bridge = bridge {
            renderer_bridge_destroy(bridge)
        }
    }
    
    func testMemoryBridgeFunctions() {
        // Test memory profiler function calls
        let totalUsed = memory_bridge_get_total_used()
        XCTAssertGreaterThanOrEqual(totalUsed, 0, "Total memory usage should be non-negative")
        
        // These should not crash
        memory_bridge_start_profiling()
        memory_bridge_stop_profiling()
    }
    
    // MARK: - Performance
    
    func testEntityCreationPerformance() {
        measure {
            for _ in 0..<1000 {
                _ = ecs_create_entity(testWorld)
            }
        }
    }
    
    func testSwiftEntityConversionPerformance() {
        var entities: [Entity] = []
        for _ in 0..<1000 {
            entities.append(ecs_create_entity(testWorld))
        }
        
        measure {
            for cEntity in entities {
                let _ = SwiftEntity(from: cEntity)
            }
        }
    }
    
    func testCollectionIterationPerformance() {
        for _ in 0..<1000 {
            _ = ecs_create_entity(testWorld)
        }
        
        let collection = EntityCollection(world: testWorld)
        
        measure {
            var count = 0
            for _ in collection {
                count += 1
            }
        }
    }
}
