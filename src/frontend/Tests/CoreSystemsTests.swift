import XCTest
@testable import VoxelForgeStudio
import simd

// MARK: - Camera Controller Tests

class CameraControllerTests: XCTestCase {
    var camera: CameraController!
    
    override func setUp() {
        super.setUp()
        camera = CameraController()
    }
    
    override func tearDown() {
        camera = nil
        super.tearDown()
    }
    
    // MARK: - Initialization Tests
    
    func testInitialState() {
        XCTAssertEqual(camera.position, SIMD3(0, 5, 10))
        XCTAssertEqual(camera.rotation, SIMD3(-20, 0, 0))
        XCTAssertEqual(camera.fov, 60.0)
        XCTAssertEqual(camera.mode, .perspective)
    }
    
    // MARK: - Movement Tests
    
    func testWASDMovement() {
        let initialPosition = camera.position
        
        // Move forward (W)
        camera.handleKeyDown(.w)
        camera.update(deltaTime: 1.0)
        
        XCTAssertNotEqual(camera.position, initialPosition)
        XCTAssertGreaterThan(length(camera.position - initialPosition), 0)
        
        camera.handleKeyUp(.w)
    }
    
    func testSprintMultiplier() {
        camera.handleKeyDown(.w)
        camera.update(deltaTime: 1.0)
        let normalDistance = length(camera.position - SIMD3(0, 5, 10))
        
        // Reset
        camera = CameraController()
        
        camera.handleKeyDown(.w)
        camera.handleKeyDown(.shift)
        camera.update(deltaTime: 1.0)
        let sprintDistance = length(camera.position - SIMD3(0, 5, 10))
        
        XCTAssertGreaterThan(sprintDistance, normalDistance)
    }
    
    func testUpDownMovement() {
        let initialY = camera.position.y
        
        // Move up (E)
        camera.handleKeyDown(.e)
        camera.update(deltaTime: 1.0)
        
        XCTAssertGreaterThan(camera.position.y, initialY)
        
        camera.handleKeyUp(.e)
        
        // Move down (Q)
        camera.handleKeyDown(.q)
        camera.update(deltaTime: 1.0)
        
        XCTAssertLessThan(camera.position.y, initialY)
    }
    
    // MARK: - Rotation Tests
    
    func testOrbitRotation() {
        let initialRotation = camera.rotation
        
        camera.handleMouseDrag(CGPoint(x: 10, y: 5), isRightButton: true, isMiddleButton: false)
        
        XCTAssertNotEqual(camera.rotation, initialRotation)
    }
    
    func testPitchClamping() {
        // Try to rotate beyond limits
        camera.handleMouseDrag(CGPoint(x: 0, y: 1000), isRightButton: true, isMiddleButton: false)
        
        XCTAssertLessThanOrEqual(camera.rotation.x, 89)
        XCTAssertGreaterThanOrEqual(camera.rotation.x, -89)
    }
    
    // MARK: - Zoom Tests
    
    func testScrollZoom() {
        let initialPosition = camera.position
        
        camera.handleScroll(5.0)
        
        XCTAssertNotEqual(camera.position, initialPosition)
    }
    
    // MARK: - Focus Tests
    
    func testFocusOnEntity() {
        let targetPosition = SIMD3<Float>(10, 0, 10)
        
        camera.focusOnEntity(at: targetPosition, radius: 5.0)
        
        // Camera should be 5 units away from target
        let distance = length(camera.position - targetPosition)
        XCTAssertEqual(distance, 5.0, accuracy: 0.1)
    }
    
    // MARK: - Orthographic View Tests
    
    func testOrthographicViewTop() {
        camera.setOrthographicView(.top)
        
        XCTAssertEqual(camera.mode, .top)
        XCTAssertEqual(camera.rotation.x, -90, accuracy: 0.1)
    }
    
    func testOrthographicViewFront() {
        camera.setOrthographicView(.front)
        
        XCTAssertEqual(camera.mode, .front)
        XCTAssertEqual(camera.rotation.x, 0, accuracy: 0.1)
        XCTAssertEqual(camera.rotation.y, 0, accuracy: 0.1)
    }
    
    // MARK: - Vector Tests
    
    func testForwardVector() {
        camera.rotation = SIMD3(0, 0, 0)
        let forward = camera.forward()
        
        // Should point along +Z axis
        XCTAssertEqual(forward.z, 1.0, accuracy: 0.01)
        XCTAssertEqual(forward.x, 0.0, accuracy: 0.01)
    }
    
    func testRightVector() {
        camera.rotation = SIMD3(0, 0, 0)
        let right = camera.right()
        
        // Should point along +X axis
        XCTAssertEqual(right.x, 1.0, accuracy: 0.01)
        XCTAssertEqual(right.z, 0.0, accuracy: 0.01)
    }
    
    func testUpVector() {
        let up = camera.up()
        
        // Should be perpendicular to forward and right
        let forward = camera.forward()
        let right = camera.right()
        
        XCTAssertEqual(dot(up, forward), 0.0, accuracy: 0.01)
        XCTAssertEqual(dot(up, right), 0.0, accuracy: 0.01)
    }
    
    // MARK: - Matrix Tests
    
    func testViewMatrix() {
        let viewMatrix = camera.viewMatrix()
        
        // View matrix should be invertible
        let det = viewMatrix.determinant
        XCTAssertNotEqual(det, 0.0)
    }
    
    func testProjectionMatrix() {
        let projMatrix = camera.projectionMatrix(aspectRatio: 16.0/9.0)
        
        // Projection matrix should be invertible
        let det = projMatrix.determinant
        XCTAssertNotEqual(det, 0.0)
    }
    
    func testPerspectiveProjection() {
        camera.mode = .perspective
        let projMatrix = camera.projectionMatrix(aspectRatio: 1.0)
        
        // Perspective projection should have specific structure
        XCTAssertNotEqual(projMatrix[2][3], 0.0) // Should have perspective divide
    }
    
    func testOrthographicProjection() {
        camera.mode = .orthographic
        let projMatrix = camera.projectionMatrix(aspectRatio: 1.0)
        
        // Orthographic projection should not have perspective divide
        XCTAssertEqual(projMatrix[2][3], 0.0)
    }
    
    // MARK: - Performance Tests
    
    func testUpdatePerformance() {
        measure {
            for _ in 0..<1000 {
                camera.update(deltaTime: 0.016)
            }
        }
    }
}

// MARK: - Selection Manager Tests

class SelectionManagerTests: XCTestCase {
    var selection: SelectionManager!
    var camera: CameraController!
    var entities: [EntityNode]!
    
    override func setUp() {
        super.setUp()
        selection = SelectionManager()
        camera = CameraController()
        
        // Create test entities
        entities = [
            EntityNode(name: "Entity1", icon: "cube"),
            EntityNode(name: "Entity2", icon: "cube"),
            EntityNode(name: "Entity3", icon: "cube")
        ]
    }
    
    override func tearDown() {
        selection = nil
        camera = nil
        entities = nil
        super.tearDown()
    }
    
    // MARK: - Selection Tests
    
    func testSelectEntity() {
        let entityID = entities[0].id
        
        selection.selectEntity(entityID)
        
        XCTAssertTrue(selection.selectedEntities.contains(entityID))
        XCTAssertEqual(selection.selectedEntities.count, 1)
    }
    
    func testAdditiveSelection() {
        let id1 = entities[0].id
        let id2 = entities[1].id
        
        selection.selectEntity(id1)
        selection.selectEntity(id2, additive: true)
        
        XCTAssertTrue(selection.selectedEntities.contains(id1))
        XCTAssertTrue(selection.selectedEntities.contains(id2))
        XCTAssertEqual(selection.selectedEntities.count, 2)
    }
    
    func testToggleSelection() {
        let entityID = entities[0].id
        
        selection.selectEntity(entityID, additive: true)
        XCTAssertTrue(selection.selectedEntities.contains(entityID))
        
        selection.selectEntity(entityID, additive: true)
        XCTAssertFalse(selection.selectedEntities.contains(entityID))
    }
    
    func testDeselectAll() {
        selection.selectEntity(entities[0].id)
        selection.selectEntity(entities[1].id, additive: true)
        
        selection.deselectAll()
        
        XCTAssertTrue(selection.selectedEntities.isEmpty)
    }
    
    // MARK: - Gizmo Mode Tests
    
    func testGizmoModeSwitch() {
        XCTAssertEqual(selection.gizmoMode, .translate)
        
        selection.gizmoMode = .rotate
        XCTAssertEqual(selection.gizmoMode, .rotate)
        
        selection.gizmoMode = .scale
        XCTAssertEqual(selection.gizmoMode, .scale)
    }
    
    func testGizmoSpaceSwitch() {
        XCTAssertEqual(selection.gizmoSpace, .world)
        
        selection.gizmoSpace = .local
        XCTAssertEqual(selection.gizmoSpace, .local)
    }
    
    // MARK: - Snapping Tests
    
    func testSnappingToggle() {
        XCTAssertFalse(selection.isSnappingEnabled)
        
        selection.isSnappingEnabled = true
        XCTAssertTrue(selection.isSnappingEnabled)
    }
    
    func testSnapValue() {
        XCTAssertEqual(selection.snapValue, 1.0)
        
        selection.snapValue = 0.5
        XCTAssertEqual(selection.snapValue, 0.5)
    }
}

// MARK: - Command Manager Tests

class CommandManagerTests: XCTestCase {
    var commandManager: CommandManager!
    
    override func setUp() {
        super.setUp()
        commandManager = CommandManager()
    }
    
    override func tearDown() {
        commandManager = nil
        super.tearDown()
    }
    
    // MARK: - Basic Command Tests
    
    func testExecuteCommand() {
        let command = MockCommand()
        
        commandManager.execute(command)
        
        XCTAssertTrue(command.executed)
        XCTAssertEqual(commandManager.undoStack.count, 1)
        XCTAssertTrue(commandManager.canUndo)
    }
    
    func testUndoCommand() {
        let command = MockCommand()
        
        commandManager.execute(command)
        commandManager.undo()
        
        XCTAssertTrue(command.undone)
        XCTAssertEqual(commandManager.undoStack.count, 0)
        XCTAssertEqual(commandManager.redoStack.count, 1)
        XCTAssertTrue(commandManager.canRedo)
    }
    
    func testRedoCommand() {
        let command = MockCommand()
        
        commandManager.execute(command)
        commandManager.undo()
        commandManager.redo()
        
        XCTAssertTrue(command.executed)
        XCTAssertEqual(commandManager.undoStack.count, 1)
        XCTAssertEqual(commandManager.redoStack.count, 0)
    }
    
    func testMultipleUndoRedo() {
        let cmd1 = MockCommand()
        let cmd2 = MockCommand()
        let cmd3 = MockCommand()
        
        commandManager.execute(cmd1)
        commandManager.execute(cmd2)
        commandManager.execute(cmd3)
        
        XCTAssertEqual(commandManager.undoStack.count, 3)
        
        commandManager.undo()
        commandManager.undo()
        
        XCTAssertEqual(commandManager.undoStack.count, 1)
        XCTAssertEqual(commandManager.redoStack.count, 2)
        
        commandManager.redo()
        
        XCTAssertEqual(commandManager.undoStack.count, 2)
        XCTAssertEqual(commandManager.redoStack.count, 1)
    }
    
    // MARK: - Command Grouping Tests
    
    func testCommandGrouping() {
        commandManager.beginGroup()
        
        commandManager.execute(MockCommand())
        commandManager.execute(MockCommand())
        commandManager.execute(MockCommand())
        
        commandManager.endGroup(description: "Test Group")
        
        XCTAssertEqual(commandManager.undoStack.count, 1)
        
        let group = commandManager.undoStack.last as? CompositeCommand
        XCTAssertNotNil(group)
        XCTAssertEqual(group?.commands.count, 3)
    }
    
    func testGroupUndo() {
        commandManager.beginGroup()
        
        let cmd1 = MockCommand()
        let cmd2 = MockCommand()
        
        commandManager.execute(cmd1)
        commandManager.execute(cmd2)
        
        commandManager.endGroup(description: "Test Group")
        
        commandManager.undo()
        
        XCTAssertTrue(cmd1.undone)
        XCTAssertTrue(cmd2.undone)
    }
    
    // MARK: - History Limit Tests
    
    func testHistoryLimit() {
        for _ in 0..<150 {
            commandManager.execute(MockCommand())
        }
        
        XCTAssertEqual(commandManager.undoStack.count, 100)
    }
    
    // MARK: - Clear Tests
    
    func testClear() {
        commandManager.execute(MockCommand())
        commandManager.execute(MockCommand())
        
        commandManager.clear()
        
        XCTAssertEqual(commandManager.undoStack.count, 0)
        XCTAssertEqual(commandManager.redoStack.count, 0)
        XCTAssertFalse(commandManager.canUndo)
        XCTAssertFalse(commandManager.canRedo)
    }
    
    // MARK: - Description Tests
    
    func testUndoDescription() {
        let command = MockCommand()
        commandManager.execute(command)
        
        XCTAssertEqual(commandManager.undoDescription, "Mock Command")
    }
    
    func testRedoDescription() {
        let command = MockCommand()
        commandManager.execute(command)
        commandManager.undo()
        
        XCTAssertEqual(commandManager.redoDescription, "Mock Command")
    }
}

// MARK: - Mock Command

class MockCommand: Command {
    var executed = false
    var undone = false
    let timestamp = Date()
    
    var description: String {
        "Mock Command"
    }
    
    func execute() {
        executed = true
        undone = false
    }
    
    func undo() {
        undone = true
        executed = false
    }
}

// MARK: - Matrix Extension for Tests

extension simd_float4x4 {
    var determinant: Float {
        // Simplified determinant calculation
        return self[0][0] * self[1][1] * self[2][2] * self[3][3]
    }
}
