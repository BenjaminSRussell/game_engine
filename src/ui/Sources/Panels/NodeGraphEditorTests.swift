import XCTest
import SwiftUI

// MARK: - NodeGraphEditor Integration Tests

class NodeGraphEditorIntegrationTests: XCTestCase {
    
    var nodeGraphManager: NodeGraphManager!
    var debugManager: DebugManager!
    var connectionManager: ConnectionManager!
    
    override func setUp() {
        super.setUp()
        nodeGraphManager = NodeGraphManager()
        debugManager = DebugManager.shared
        connectionManager = ConnectionManager()
    }
    
    override func tearDown() {
        nodeGraphManager = nil
        debugManager = nil
        connectionManager = nil
        super.tearDown()
    }
    
    // MARK: - Basic Node Management Tests
    
    func testNodeCreation() {
        // Test creating different node types
        let position = CGPoint(x: 100, y: 100)
        
        nodeGraphManager.addNode(type: .event, at: position)
        XCTAssertEqual(nodeGraphManager.nodes.count, 1)
        XCTAssertEqual(nodeGraphManager.nodes.first?.type, .event)
        
        nodeGraphManager.addNode(type: .mathAdd, at: CGPoint(x: 200, y: 100))
        XCTAssertEqual(nodeGraphManager.nodes.count, 2)
        XCTAssertEqual(nodeGraphManager.nodes[1].type, .mathAdd)
    }
    
    func testNodeDeletion() {
        // Create nodes
        nodeGraphManager.addNode(type: .variable, at: CGPoint(x: 100, y: 100))
        nodeGraphManager.addNode(type: .constant, at: CGPoint(x: 200, y: 100))
        
        let firstNodeId = nodeGraphManager.nodes[0].id
        nodeGraphManager.selectedNodes = [firstNodeId]
        
        // Delete selected nodes
        nodeGraphManager.deleteSelectedNodes()
        XCTAssertEqual(nodeGraphManager.nodes.count, 1)
        XCTAssertNotEqual(nodeGraphManager.nodes[0].id, firstNodeId)
    }
    
    // MARK: - Connection Management Tests
    
    func testConnectionCreation() {
        // Create nodes
        nodeGraphManager.addNode(type: .mathAdd, at: CGPoint(x: 100, y: 100))
        nodeGraphManager.addNode(type: .mathMultiply, at: CGPoint(x: 300, y: 100))
        
        let firstNode = nodeGraphManager.nodes[0]
        let secondNode = nodeGraphManager.nodes[1]
        
        // Create connection
        let connection = connectionManager.createConnection(
            fromNode: firstNode.id,
            fromPort: 0,
            toNode: secondNode.id,
            toPort: 0,
            portType: .float
        )
        
        XCTAssertNotNil(connection)
        XCTAssertEqual(connectionManager.connections.count, 1)
    }
    
    func testConnectionValidation() {
        // Create nodes
        nodeGraphManager.addNode(type: .mathAdd, at: CGPoint(x: 100, y: 100))
        nodeGraphManager.addNode(type: .mathMultiply, at: CGPoint(x: 300, y: 100))
        
        let firstNode = nodeGraphManager.nodes[0]
        let secondNode = nodeGraphManager.nodes[1]
        
        // Test self-connection validation
        let selfConnection = connectionManager.createConnection(
            fromNode: firstNode.id,
            fromPort: 0,
            toNode: firstNode.id,
            toPort: 0,
            portType: .float
        )
        
        XCTAssertNil(selfConnection)
        XCTAssertEqual(connectionManager.connections.count, 0)
        
        // Test valid connection
        let validConnection = connectionManager.createConnection(
            fromNode: firstNode.id,
            fromPort: 0,
            toNode: secondNode.id,
            toPort: 0,
            portType: .float
        )
        
        XCTAssertNotNil(validConnection)
        XCTAssertEqual(connectionManager.connections.count, 1)
    }
    
    // MARK: - Debugging System Tests
    
    func testDebuggingLifecycle() {
        // Start debugging
        debugManager.startDebugging()
        XCTAssertTrue(debugManager.isDebugging)
        XCTAssertTrue(debugManager.isRunning)
        XCTAssertFalse(debugManager.isPaused)
        
        // Pause execution
        debugManager.pauseExecution()
        XCTAssertFalse(debugManager.isRunning)
        XCTAssertTrue(debugManager.isPaused)
        
        // Continue execution
        debugManager.continueExecution()
        XCTAssertTrue(debugManager.isRunning)
        XCTAssertFalse(debugManager.isPaused)
        
        // Stop debugging
        debugManager.stopDebugging()
        XCTAssertFalse(debugManager.isDebugging)
        XCTAssertFalse(debugManager.isRunning)
        XCTAssertFalse(debugManager.isPaused)
    }
    
    func testBreakpointManagement() {
        // Create a node
        nodeGraphManager.addNode(type: .event, at: CGPoint(x: 100, y: 100))
        let nodeId = nodeGraphManager.nodes[0].id
        
        // Test breakpoint toggle
        XCTAssertFalse(debugManager.breakpoints.contains(nodeId))
        debugManager.toggleBreakpoint(on: nodeId)
        XCTAssertTrue(debugManager.breakpoints.contains(nodeId))
        
        // Test conditional breakpoint
        debugManager.setConditionalBreakpoint(on: nodeId, condition: "x > 10")
        XCTAssertTrue(debugManager.conditionalBreakpoints.keys.contains(nodeId))
        XCTAssertEqual(debugManager.conditionalBreakpoints[nodeId], "x > 10")
        
        // Test hit count breakpoint
        debugManager.setHitCountBreakpoint(on: nodeId, targetCount: 5)
        XCTAssertTrue(debugManager.hitCountBreakpoints.keys.contains(nodeId))
        XCTAssertEqual(debugManager.hitCountBreakpoints[nodeId].target, 5)
        
        // Remove breakpoint
        debugManager.toggleBreakpoint(on: nodeId)
        XCTAssertFalse(debugManager.breakpoints.contains(nodeId))
        XCTAssertFalse(debugManager.conditionalBreakpoints.keys.contains(nodeId))
        XCTAssertFalse(debugManager.hitCountBreakpoints.keys.contains(nodeId))
    }
    
    func testVariableWatching() {
        // Test local variables
        let testVariables = ["x": 42, "y": 3.14, "name": "test"]
        debugManager.updateLocalVariables(testVariables)
        
        XCTAssertEqual(debugManager.localVariables.count, 3)
        XCTAssertEqual(debugManager.localVariables["x"]?.value as? Int, 42)
        XCTAssertEqual(debugManager.localVariables["y"]?.value as? Double, 3.14)
        XCTAssertEqual(debugManager.localVariables["name"]?.value as? String, "test")
        
        // Test expression evaluation
        let result = debugManager.evaluateExpression("x")
        XCTAssertNotNil(result)
        XCTAssertEqual(result?.name, "x")
        XCTAssertEqual(result?.value as? Int, 42)
        
        // Test instance variables
        let instanceVars = ["health": 100.0, "level": 5]
        debugManager.updateInstanceVariables(instanceVars)
        
        XCTAssertEqual(debugManager.instanceVariables.count, 2)
        XCTAssertEqual(debugManager.instanceVariables["health"]?.value as? Double, 100.0)
    }
    
    // MARK: - Performance Tests
    
    func testPerformanceTracking() {
        // Create nodes
        for i in 0..<100 {
            nodeGraphManager.addNode(type: .mathAdd, at: CGPoint(x: Double(i) * 50, y: 100))
        }
        
        let startTime = Date()
        
        // Simulate execution
        for node in nodeGraphManager.nodes {
            let executionTime = Double.random(in: 0.001...0.01)
            let perfData = NodePerformanceData(
                nodeId: node.id,
                executionTime: executionTime,
                memoryUsage: Int64.random(in: 100...1000),
                callCount: Int.random(in: 1...10)
            )
            debugManager.nodePerformance[node.id] = perfData
        }
        
        let endTime = Date()
        let totalTime = endTime.timeIntervalSince(startTime)
        
        // Verify performance data
        XCTAssertEqual(debugManager.nodePerformance.count, 100)
        XCTAssertLessThan(totalTime, 1.0) // Should complete in less than 1 second
        
        // Test performance metrics
        let totalExecutionTime = debugManager.nodePerformance.values.reduce(0) { $0 + $1.executionTime }
        XCTAssertGreaterThan(totalExecutionTime, 0)
    }
    
    // MARK: - Port Type Compatibility Tests
    
    func testPortTypeCompatibility() {
        // Test compatible types
        XCTAssertTrue(PortType.int.canConnect(to: .float))
        XCTAssertTrue(PortType.float.canConnect(to: .int))
        XCTAssertTrue(PortType.vector3.canConnect(to: .vector3))
        XCTAssertTrue(PortType.string.canConnect(to: .string))
        
        // Test incompatible types
        XCTAssertFalse(PortType.int.canConnect(to: .string))
        XCTAssertFalse(PortType.vector3.canConnect(to: .bool))
        
        // Test generic types
        XCTAssertTrue(PortType.any.canConnect(to: .int))
        XCTAssertTrue(PortType.any.canConnect(to: .string))
        XCTAssertTrue(PortType.any.canConnect(to: .vector3))
        
        // Test array compatibility
        XCTAssertTrue(PortType.array.canConnect(to: .array))
        XCTAssertFalse(PortType.array.canConnect(to: .int))
    }
    
    // MARK: - Fuzzy Search Tests
    
    func testFuzzySearch() {
        let nodeTypes = NodeType.allCases
        
        // Test exact match
        let exactScore = FuzzySearch.fuzzyMatch("mathAdd", "mathAdd")
        XCTAssertEqual(exactScore, 1.0)
        
        // Test prefix match
        let prefixScore = FuzzySearch.fuzzyMatch("math", "mathAdd")
        XCTAssertEqual(prefixScore, 0.9)
        
        // Test contains match
        let containsScore = FuzzySearch.fuzzyMatch("add", "mathAdd")
        XCTAssertEqual(containsScore, 0.7)
        
        // Test fuzzy match
        let fuzzyScore = FuzzySearch.fuzzyMatch("mtahdd", "mathAdd")
        XCTAssertGreaterThan(fuzzyScore, 0.1)
        XCTAssertLessThan(fuzzyScore, 1.0)
        
        // Test search functionality
        let results = FuzzySearch.search("math", in: nodeTypes) { "\($0)" }
        XCTAssertGreaterThan(results.count, 0)
        XCTAssertTrue(results.allSatisfy { $0.score >= 0.2 })
        XCTAssertEqual(results.first?.score, results.max { $0.score > $1.score }?.score)
    }
    
    // MARK: - Node Customization Tests
    
    func testNodeCustomization() {
        let customizationManager = NodeCustomizationManager.shared
        
        // Test custom colors
        customizationManager.setCustomColor(for: .event, color: .purple)
        XCTAssertEqual(customizationManager.getCustomColor(for: .event), .purple)
        
        // Test custom icons
        customizationManager.setCustomIcon(for: .variable, icon: "star.fill")
        XCTAssertEqual(customizationManager.getCustomIcon(for: .variable), "star.fill")
        
        // Test descriptions
        let eventDescription = customizationManager.getDescription(for: .event)
        XCTAssertFalse(eventDescription.isEmpty)
        
        let eventDocumentation = customizationManager.getDocumentation(for: .event)
        XCTAssertFalse(eventDocumentation.isEmpty)
    }
    
    // MARK: - Connection Rendering Tests
    
    func testConnectionStyles() {
        // Test all connection styles
        let styles = ConnectionStyle.allCases
        XCTAssertEqual(styles.count, 3)
        XCTAssertTrue(styles.contains(.bezier))
        XCTAssertTrue(styles.contains(.straight))
        XCTAssertTrue(styles.contains(.orthogonal))
        
        // Test thickness values
        let thicknesses = ConnectionThickness.allCases
        XCTAssertEqual(thicknesses.count, 3)
        XCTAssertEqual(ConnectionThickness.thin.value, 2.0)
        XCTAssertEqual(ConnectionThickness.medium.value, 3.0)
        XCTAssertEqual(ConnectionThickness.thick.value, 4.0)
    }
    
    // MARK: - Memory Management Tests
    
    func testMemoryManagement() {
        // Create many nodes and connections
        for i in 0..<1000 {
            nodeGraphManager.addNode(type: .mathAdd, at: CGPoint(x: Double(i), y: 100))
        }
        
        // Create connections
        for i in 0..<500 {
            if i < nodeGraphManager.nodes.count - 1 {
                let connection = connectionManager.createConnection(
                    fromNode: nodeGraphManager.nodes[i].id,
                    fromPort: 0,
                    toNode: nodeGraphManager.nodes[i + 1].id,
                    toPort: 0,
                    portType: .float
                )
                XCTAssertNotNil(connection)
            }
        }
        
        // Verify counts
        XCTAssertEqual(nodeGraphManager.nodes.count, 1000)
        XCTAssertEqual(connectionManager.connections.count, 500)
        
        // Test cleanup
        nodeGraphManager.selectedNodes = Set(nodeGraphManager.nodes.map { $0.id })
        nodeGraphManager.deleteSelectedNodes()
        
        XCTAssertEqual(nodeGraphManager.nodes.count, 0)
        XCTAssertEqual(connectionManager.connections.count, 0)
    }
    
    // MARK: - Integration Test
    
    func testFullIntegration() {
        // 1. Create a simple graph
        let eventNode = GraphNode(type: .event, title: "Start", position: CGPoint(x: 100, y: 100))
        let mathNode = GraphNode(type: .mathAdd, title: "Add", position: CGPoint(x: 300, y: 100))
        let variableNode = GraphNode(type: .variable, title: "Result", position: CGPoint(x: 500, y: 100))
        
        nodeGraphManager.nodes = [eventNode, mathNode, variableNode]
        
        // 2. Create connections
        let connection1 = connectionManager.createConnection(
            fromNode: eventNode.id,
            fromPort: 0,
            toNode: mathNode.id,
            toPort: 0,
            portType: .flow
        )
        
        let connection2 = connectionManager.createConnection(
            fromNode: mathNode.id,
            fromPort: 0,
            toNode: variableNode.id,
            toPort: 0,
            portType: .float
        )
        
        XCTAssertNotNil(connection1)
        XCTAssertNotNil(connection2)
        
        // 3. Set up debugging
        debugManager.startDebugging()
        debugManager.toggleBreakpoint(on: mathNode.id)
        
        // 4. Simulate execution
        debugManager.currentExecutionNode = eventNode.id
        debugManager.updateLocalVariables(["a": 5, "b": 10])
        
        // 5. Verify state
        XCTAssertTrue(debugManager.isDebugging)
        XCTAssertTrue(debugManager.breakpoints.contains(mathNode.id))
        XCTAssertEqual(debugManager.currentExecutionNode, eventNode.id)
        XCTAssertEqual(debugManager.localVariables["a"]?.value as? Int, 5)
        
        // 6. Step through execution
        debugManager.stepThrough()
        
        // 7. Stop debugging
        debugManager.stopDebugging()
        
        // 8. Verify cleanup
        XCTAssertFalse(debugManager.isDebugging)
        XCTAssertNil(debugManager.currentExecutionNode)
    }
    
    // MARK: - Performance Benchmark
    
    func testPerformanceBenchmark() {
        measure {
            // Create 1000 nodes
            for i in 0..<1000 {
                nodeGraphManager.addNode(type: .mathAdd, at: CGPoint(x: Double(i), y: 100))
            }
            
            // Create connections
            for i in 0..<999 {
                let connection = connectionManager.createConnection(
                    fromNode: nodeGraphManager.nodes[i].id,
                    fromPort: 0,
                    toNode: nodeGraphManager.nodes[i + 1].id,
                    toPort: 0,
                    portType: .float
                )
                XCTAssertNotNil(connection)
            }
            
            // Simulate debugging operations
            debugManager.startDebugging()
            
            for node in nodeGraphManager.nodes.prefix(100) {
                debugManager.toggleBreakpoint(on: node.id)
            }
            
            debugManager.stopDebugging()
            
            // Cleanup
            nodeGraphManager.selectedNodes = Set(nodeGraphManager.nodes.map { $0.id })
            nodeGraphManager.deleteSelectedNodes()
        }
    }
}

// MARK: - UI Tests

class NodeGraphEditorUITests: XCTestCase {
    
    func testNodeViewRendering() {
        // This would require SwiftUI testing framework
        // For now, we'll test the data structures
        
        let node = GraphNode(
            type: .mathAdd,
            title: "Add Node",
            position: CGPoint(x: 100, y: 100),
            inputs: [
                NodePort(name: "A", type: .float),
                NodePort(name: "B", type: .float)
            ],
            outputs: [
                NodePort(name: "Result", type: .float)
            ]
        )
        
        XCTAssertEqual(node.title, "Add Node")
        XCTAssertEqual(node.type, .mathAdd)
        XCTAssertEqual(node.inputs.count, 2)
        XCTAssertEqual(node.outputs.count, 1)
        XCTAssertEqual(node.inputs[0].type, .float)
        XCTAssertEqual(node.outputs[0].type, .float)
    }
    
    func testPortTypeColors() {
        XCTAssertEqual(PortType.flow.color, .white)
        XCTAssertEqual(PortType.bool.color, .red)
        XCTAssertEqual(PortType.int.color, .cyan)
        XCTAssertEqual(PortType.float.color, .green)
        XCTAssertEqual(PortType.vector3.color, .yellow)
        XCTAssertEqual(PortType.string.color, .magenta)
        XCTAssertEqual(PortType.object.color, .blue)
        XCTAssertEqual(PortType.any.color, .gray)
        XCTAssertEqual(PortType.array.color, .orange)
        XCTAssertEqual(PortType.dictionary.color, .purple)
    }
    
    func testNodeTypeCategories() {
        let mathNodes = NodeType.allCases.filter { $0.category == .math }
        XCTAssertGreaterThan(mathNodes.count, 0)
        XCTAssertTrue(mathNodes.contains(.mathAdd))
        XCTAssertTrue(mathNodes.contains(.mathMultiply))
        XCTAssertTrue(mathNodes.contains(.mathSqrt))
        
        let eventNodes = NodeType.allCases.filter { $0.category == .events }
        XCTAssertGreaterThan(eventNodes.count, 0)
        XCTAssertTrue(eventNodes.contains(.event))
        XCTAssertTrue(eventNodes.contains(.onStart))
        XCTAssertTrue(eventNodes.contains(.onUpdate))
    }
}

// MARK: - Mock Data for Testing

extension NodeGraphManager {
    func createTestGraph() {
        // Event node
        let startNode = GraphNode(
            type: .onStart,
            title: "On Start",
            position: CGPoint(x: 100, y: 200),
            outputs: [NodePort(name: "Exec", type: .flow)]
        )
        
        // Math nodes
        let addNode = GraphNode(
            type: .mathAdd,
            title: "Add",
            position: CGPoint(x: 350, y: 150),
            inputs: [
                NodePort(name: "A", type: .float),
                NodePort(name: "B", type: .float)
            ],
            outputs: [NodePort(name: "Result", type: .float)]
        )
        
        let multiplyNode = GraphNode(
            type: .mathMultiply,
            title: "Multiply",
            position: CGPoint(x: 350, y: 250),
            inputs: [
                NodePort(name: "A", type: .float),
                NodePort(name: "B", type: .float)
            ],
            outputs: [NodePort(name: "Result", type: .float)]
        )
        
        // Variable node
        let resultNode = GraphNode(
            type: .variable,
            title: "Result",
            position: CGPoint(x: 600, y: 200),
            inputs: [NodePort(name: "Value", type: .float)]
        )
        
        nodes = [startNode, addNode, multiplyNode, resultNode]
        
        // Connections
        connections = [
            GraphConnection(fromNode: startNode.id, fromPort: 0, toNode: addNode.id, toPort: 0),
            GraphConnection(fromNode: addNode.id, fromPort: 0, toNode: multiplyNode.id, toPort: 0),
            GraphConnection(fromNode: multiplyNode.id, fromPort: 0, toNode: resultNode.id, toPort: 0)
        ]
    }
}
