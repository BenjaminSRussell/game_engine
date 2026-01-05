import XCTest
@testable import VoxelForgeStudio
import CVoxelForge

final class BridgeTests: XCTestCase {
    func testComponentCount() {
        // ID 0 should return mock count of 2
        let count = engine_get_component_count(0)
        XCTAssertEqual(count, 2, "Mock C bridge should return 2 components")
    }
    
    func testEntityActive() {
        // ID 0 should be active by default (mock)
        let active = engine_get_entity_active(0)
        XCTAssertTrue(active, "Entity 0 should be active")
        
        // available in mock
        engine_set_entity_active(0, false)
        XCTAssertFalse(engine_get_entity_active(0), "Entity 0 should be inactive after set")
    }
}
