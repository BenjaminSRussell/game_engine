import SwiftUI
import simd

// MARK: - Visual Scripting Editor
// Blueprint-style visual programming system

class VisualScriptingEditor: ObservableObject {
    @Published var nodes: [ScriptNode] = []
    @Published var connections: [ScriptConnection] = []
    @Published var selectedNodes: Set<UUID> = []
    @Published var canvasOffset: CGPoint = .zero
    @Published var canvasZoom: CGFloat = 1.0
    
    // Variables and functions
    @Published var variables: [ScriptVariable] = []
    @Published var functions: [ScriptFunction] = []
    
    // MARK: - Script Node
    struct ScriptNode: Identifiable {
        let id = UUID()
        var type: ScriptNodeType
        var position: CGPoint
        var inputs: [NodePin]
        var outputs: [NodePin]
        var parameters: [String: Any] = [:]
        var color: Color
        
        var displayName: String {
            type.rawValue
        }
    }
    
    // MARK: - Script Node Type
    enum ScriptNodeType: String, CaseIterable {
        // Flow control - All implemented with proper pin definitions
        case onStart = "On Start"
        case onUpdate = "On Update"
        case onCollision = "On Collision"
        case onTrigger = "On Trigger"
        case branch = "Branch"
        case switchNode = "Switch"
        case forLoop = "For Loop"
        case whileLoop = "While Loop"
        case sequence = "Sequence"
        case gate = "Gate"
        case flipFlop = "Flip Flop"
        
        // Variables - Implemented
        case getVariable = "Get Variable"
        case setVariable = "Set Variable"
        
        // Math - All implemented
        case add = "Add"
        case subtract = "Subtract"
        case multiply = "Multiply"
        case divide = "Divide"
        case stringConcat = "String Concat"
        case stringFormat = "String Format"
        case arrayGet = "Array Get"
        case arraySet = "Array Set"
        case arrayLength = "Array Length"
        
        // Entity operations - Implemented
        case getComponent = "Get Component"
        case setComponent = "Set Component"
        case spawnEntity = "Spawn Entity"
        case destroyEntity = "Destroy Entity"
        
        // Physics - Implemented
        case applyForce = "Apply Force"
        case applyImpulse = "Apply Impulse"
        
        // Raycasting - Implemented
        case lineTrace = "Line Trace"
        case sphereTrace = "Sphere Trace"
        
        // Input - Implemented
        case keyPressed = "Key Pressed"
        case mouseButton = "Mouse Button"
        case gamepadButton = "Gamepad Button"
        
        // Animation - Implemented
        case playAnimation = "Play Animation"
        case stopAnimation = "Stop Animation"
        
        // Audio - Implemented
        case playSound = "Play Sound"
        case stopSound = "Stop Sound"
        
        // Particles - Implemented
        case spawnParticles = "Spawn Particles"
        
        // Utility - Implemented
        case delay = "Delay"
        case timeline = "Timeline"
        case lerp = "Lerp"
        case ease = "Ease"
        case randomFloat = "Random Float"
        case randomInt = "Random Int"
        
        // Functions - Implemented
        case functionCall = "Function Call"
        case customEvent = "Custom Event"
        
        var category: NodeCategory {
            switch self {
            case .onStart, .onUpdate, .onCollision, .onTrigger:
                return .events
            case .branch, .switchNode, .forLoop, .whileLoop, .sequence, .gate, .flipFlop:
                return .flowControl
            case .getVariable, .setVariable:
                return .variables
            case .add, .subtract, .multiply, .divide:
                return .math
            case .stringConcat, .stringFormat:
                return .string
            case .arrayGet, .arraySet, .arrayLength:
                return .array
            case .getComponent, .setComponent, .spawnEntity, .destroyEntity:
                return .entity
            case .applyForce, .applyImpulse:
                return .physics
            case .lineTrace, .sphereTrace:
                return .raycasting
            case .keyPressed, .mouseButton, .gamepadButton:
                return .input
            case .playAnimation, .stopAnimation:
                return .animation
            case .playSound, .stopSound:
                return .audio
            case .spawnParticles:
                return .particles
            case .delay, .timeline, .lerp, .ease, .randomFloat, .randomInt:
                return .utility
            case .functionCall, .customEvent:
                return .functions
            }
        }
        
        var color: Color {
            switch category {
            case .events: return .red
            case .flowControl: return .orange
            case .variables: return .green
            case .math: return .cyan
            case .string: return .pink
            case .array: return .purple
            case .entity: return .blue
            case .physics: return .yellow
            case .raycasting: return .indigo
            case .input: return .teal
            case .animation: return .mint
            case .audio: return .brown
            case .particles: return .orange
            case .utility: return .gray
            case .functions: return .blue
            }
        }
    }
    
    // MARK: - Node Category
    enum NodeCategory: String, CaseIterable {
        case events = "Events"
        case flowControl = "Flow Control"
        case variables = "Variables"
        case math = "Math"
        case string = "String"
        case array = "Array"
        case entity = "Entity"
        case physics = "Physics"
        case raycasting = "Raycasting"
        case input = "Input"
        case animation = "Animation"
        case audio = "Audio"
        case particles = "Particles"
        case utility = "Utility"
        case functions = "Functions"
    }
    
    // MARK: - Node Pin
    struct NodePin: Identifiable {
        let id = UUID()
        let name: String
        let type: PinType
        let isInput: Bool
        
        enum PinType: String {
            case exec = "Exec"
            case bool = "Bool"
            case int = "Int"
            case float = "Float"
            case string = "String"
            case vector3 = "Vector3"
            case entity = "Entity"
            case any = "Any"
        }
        
        var color: Color {
            switch type {
            case .exec: return .white
            case .bool: return .red
            case .int: return .cyan
            case .float: return .green
            case .string: return .pink
            case .vector3: return .yellow
            case .entity: return .blue
            case .any: return .gray
            }
        }
    }
    
    // MARK: - Script Connection
    struct ScriptConnection: Identifiable {
        let id = UUID()
        let fromNode: UUID
        let fromPin: UUID
        let toNode: UUID
        let toPin: UUID
    }
    
    // MARK: - Script Variable
    struct ScriptVariable: Identifiable {
        let id = UUID()
        var name: String
        var type: NodePin.PinType
        var defaultValue: Any
        var isPublic = false
    }
    
    // MARK: - Script Function
    struct ScriptFunction: Identifiable {
        let id = UUID()
        var name: String
        var inputs: [NodePin]
        var outputs: [NodePin]
    }
    
    // MARK: - Functions
    
    func addNode(type: ScriptNodeType, at position: CGPoint) {
        let inputs = getDefaultInputs(for: type)
        let outputs = getDefaultOutputs(for: type)
        
        let node = ScriptNode(
            type: type,
            position: position,
            inputs: inputs,
            outputs: outputs,
            color: type.color
        )
        nodes.append(node)
        print("[VisualScript] Added node: \(type.rawValue)")
    }
    
    private func getDefaultInputs(for type: ScriptNodeType) -> [NodePin] {
        switch type {
        // Event nodes (no exec input, they start execution)
        case .onStart, .onUpdate, .onCollision, .onTrigger:
            return []
            
        // Branch node
        case .branch:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Condition", type: .bool, isInput: true)
            ]
            
        // Switch node
        case .switchNode:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Selection", type: .int, isInput: true)
            ]
            
        // For loop
        case .forLoop:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Start", type: .int, isInput: true),
                NodePin(name: "End", type: .int, isInput: true)
            ]
            
        // While loop
        case .whileLoop:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Condition", type: .bool, isInput: true)
            ]
            
        // Sequence
        case .sequence:
            return [NodePin(name: "Exec", type: .exec, isInput: true)]
            
        // Gate
        case .gate:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Open", type: .bool, isInput: true)
            ]
            
        // Flip flop
        case .flipFlop:
            return [NodePin(name: "Exec", type: .exec, isInput: true)]
            
        // Get variable
        case .getVariable:
            return []
            
        // Set variable
        case .setVariable:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Value", type: .any, isInput: true)
            ]
            
        // Math operations
        case .add, .subtract, .multiply, .divide:
            return [
                NodePin(name: "A", type: .float, isInput: true),
                NodePin(name: "B", type: .float, isInput: true)
            ]
            
        // String concat
        case .stringConcat:
            return [
                NodePin(name: "A", type: .string, isInput: true),
                NodePin(name: "B", type: .string, isInput: true)
            ]
            
        // String format
        case .stringFormat:
            return [
                NodePin(name: "Format", type: .string, isInput: true),
                NodePin(name: "Args", type: .any, isInput: true)
            ]
            
        // Array get
        case .arrayGet:
            return [
                NodePin(name: "Array", type: .any, isInput: true),
                NodePin(name: "Index", type: .int, isInput: true)
            ]
            
        // Array set
        case .arraySet:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Array", type: .any, isInput: true),
                NodePin(name: "Index", type: .int, isInput: true),
                NodePin(name: "Value", type: .any, isInput: true)
            ]
            
        // Array length
        case .arrayLength:
            return [NodePin(name: "Array", type: .any, isInput: true)]
            
        // Get component
        case .getComponent:
            return [NodePin(name: "Entity", type: .entity, isInput: true)]
            
        // Set component
        case .setComponent:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Entity", type: .entity, isInput: true),
                NodePin(name: "Value", type: .any, isInput: true)
            ]
            
        // Spawn entity
        case .spawnEntity:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Position", type: .vector3, isInput: true),
                NodePin(name: "Template", type: .any, isInput: true)
            ]
            
        // Destroy entity
        case .destroyEntity:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Entity", type: .entity, isInput: true)
            ]
            
        // Apply force
        case .applyForce:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Entity", type: .entity, isInput: true),
                NodePin(name: "Force", type: .vector3, isInput: true)
            ]
            
        // Apply impulse
        case .applyImpulse:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Entity", type: .entity, isInput: true),
                NodePin(name: "Impulse", type: .vector3, isInput: true)
            ]
            
        // Line trace
        case .lineTrace:
            return [
                NodePin(name: "Start", type: .vector3, isInput: true),
                NodePin(name: "End", type: .vector3, isInput: true)
            ]
            
        // Sphere trace
        case .sphereTrace:
            return [
                NodePin(name: "Start", type: .vector3, isInput: true),
                NodePin(name: "End", type: .vector3, isInput: true),
                NodePin(name: "Radius", type: .float, isInput: true)
            ]
            
        // Input nodes
        case .keyPressed:
            return [NodePin(name: "Key", type: .string, isInput: true)]
        case .mouseButton:
            return [NodePin(name: "Button", type: .int, isInput: true)]
        case .gamepadButton:
            return [NodePin(name: "Button", type: .int, isInput: true)]
            
        // Animation
        case .playAnimation:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Entity", type: .entity, isInput: true),
                NodePin(name: "Animation", type: .string, isInput: true)
            ]
        case .stopAnimation:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Entity", type: .entity, isInput: true)
            ]
            
        // Audio
        case .playSound:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Sound", type: .string, isInput: true),
                NodePin(name: "Volume", type: .float, isInput: true)
            ]
        case .stopSound:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Sound", type: .string, isInput: true)
            ]
            
        // Particles
        case .spawnParticles:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Position", type: .vector3, isInput: true),
                NodePin(name: "Effect", type: .string, isInput: true)
            ]
            
        // Delay
        case .delay:
            return [
                NodePin(name: "Exec", type: .exec, isInput: true),
                NodePin(name: "Duration", type: .float, isInput: true)
            ]
            
        // Timeline
        case .timeline:
            return [NodePin(name: "Exec", type: .exec, isInput: true)]
            
        // Lerp
        case .lerp:
            return [
                NodePin(name: "A", type: .float, isInput: true),
                NodePin(name: "B", type: .float, isInput: true),
                NodePin(name: "Alpha", type: .float, isInput: true)
            ]
            
        // Ease
        case .ease:
            return [
                NodePin(name: "A", type: .float, isInput: true),
                NodePin(name: "B", type: .float, isInput: true),
                NodePin(name: "Alpha", type: .float, isInput: true),
                NodePin(name: "EaseType", type: .int, isInput: true)
            ]
            
        // Random
        case .randomFloat:
            return [
                NodePin(name: "Min", type: .float, isInput: true),
                NodePin(name: "Max", type: .float, isInput: true)
            ]
        case .randomInt:
            return [
                NodePin(name: "Min", type: .int, isInput: true),
                NodePin(name: "Max", type: .int, isInput: true)
            ]
            
        // Function call
        case .functionCall:
            return [NodePin(name: "Exec", type: .exec, isInput: true)]
            
        // Custom event
        case .customEvent:
            return []
        }
    }
    
    private func getDefaultOutputs(for type: ScriptNodeType) -> [NodePin] {
        switch type {
        // Event nodes
        case .onStart, .onUpdate:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
        case .onCollision:
            return [
                NodePin(name: "Exec", type: .exec, isInput: false),
                NodePin(name: "Other", type: .entity, isInput: false)
            ]
        case .onTrigger:
            return [
                NodePin(name: "Exec", type: .exec, isInput: false),
                NodePin(name: "Other", type: .entity, isInput: false)
            ]
            
        // Branch
        case .branch:
            return [
                NodePin(name: "True", type: .exec, isInput: false),
                NodePin(name: "False", type: .exec, isInput: false)
            ]
            
        // Switch
        case .switchNode:
            return [
                NodePin(name: "0", type: .exec, isInput: false),
                NodePin(name: "1", type: .exec, isInput: false),
                NodePin(name: "Default", type: .exec, isInput: false)
            ]
            
        // For loop
        case .forLoop:
            return [
                NodePin(name: "Loop", type: .exec, isInput: false),
                NodePin(name: "Index", type: .int, isInput: false),
                NodePin(name: "Completed", type: .exec, isInput: false)
            ]
            
        // While loop
        case .whileLoop:
            return [
                NodePin(name: "Loop", type: .exec, isInput: false),
                NodePin(name: "Completed", type: .exec, isInput: false)
            ]
            
        // Sequence
        case .sequence:
            return [
                NodePin(name: "Then 0", type: .exec, isInput: false),
                NodePin(name: "Then 1", type: .exec, isInput: false),
                NodePin(name: "Then 2", type: .exec, isInput: false)
            ]
            
        // Gate
        case .gate:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Flip flop
        case .flipFlop:
            return [
                NodePin(name: "A", type: .exec, isInput: false),
                NodePin(name: "B", type: .exec, isInput: false)
            ]
            
        // Get variable
        case .getVariable:
            return [NodePin(name: "Value", type: .any, isInput: false)]
            
        // Set variable
        case .setVariable:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Math operations
        case .add, .subtract, .multiply, .divide:
            return [NodePin(name: "Result", type: .float, isInput: false)]
            
        // String operations
        case .stringConcat, .stringFormat:
            return [NodePin(name: "Result", type: .string, isInput: false)]
            
        // Array get
        case .arrayGet:
            return [NodePin(name: "Value", type: .any, isInput: false)]
            
        // Array set
        case .arraySet:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Array length
        case .arrayLength:
            return [NodePin(name: "Length", type: .int, isInput: false)]
            
        // Get component
        case .getComponent:
            return [NodePin(name: "Component", type: .any, isInput: false)]
            
        // Set component
        case .setComponent:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Spawn entity
        case .spawnEntity:
            return [
                NodePin(name: "Exec", type: .exec, isInput: false),
                NodePin(name: "Entity", type: .entity, isInput: false)
            ]
            
        // Destroy entity
        case .destroyEntity:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Physics
        case .applyForce, .applyImpulse:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Tracing
        case .lineTrace, .sphereTrace:
            return [
                NodePin(name: "Hit", type: .bool, isInput: false),
                NodePin(name: "Entity", type: .entity, isInput: false),
                NodePin(name: "Location", type: .vector3, isInput: false)
            ]
            
        // Input
        case .keyPressed, .mouseButton, .gamepadButton:
            return [
                NodePin(name: "Pressed", type: .exec, isInput: false),
                NodePin(name: "Released", type: .exec, isInput: false)
            ]
            
        // Animation
        case .playAnimation:
            return [
                NodePin(name: "Exec", type: .exec, isInput: false),
                NodePin(name: "Completed", type: .exec, isInput: false)
            ]
        case .stopAnimation:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Audio
        case .playSound, .stopSound:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Particles
        case .spawnParticles:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Delay
        case .delay:
            return [NodePin(name: "Completed", type: .exec, isInput: false)]
            
        // Timeline
        case .timeline:
            return [
                NodePin(name: "Update", type: .exec, isInput: false),
                NodePin(name: "Alpha", type: .float, isInput: false),
                NodePin(name: "Completed", type: .exec, isInput: false)
            ]
            
        // Lerp/Ease
        case .lerp, .ease:
            return [NodePin(name: "Result", type: .float, isInput: false)]
            
        // Random
        case .randomFloat:
            return [NodePin(name: "Value", type: .float, isInput: false)]
        case .randomInt:
            return [NodePin(name: "Value", type: .int, isInput: false)]
            
        // Function call
        case .functionCall:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
            
        // Custom event
        case .customEvent:
            return [NodePin(name: "Exec", type: .exec, isInput: false)]
        }
    }
    
    func deleteNode(_ id: UUID) {
        nodes.removeAll { $0.id == id }
        connections.removeAll { $0.fromNode == id || $0.toNode == id }
    }
    
    func connectPins(from: (node: UUID, pin: UUID), to: (node: UUID, pin: UUID)) {
        let connection = ScriptConnection(
            fromNode: from.node,
            fromPin: from.pin,
            toNode: to.node,
            toPin: to.pin
        )
        connections.append(connection)
    }
    
    func deleteConnection(_ id: UUID) {
        connections.removeAll { $0.id == id }
    }
    
    // MARK: - Blueprint Debugging
    
    @Published var breakpoints: Set<UUID> = []
    @Published var watchVariables: Set<UUID> = []
    @Published var executionPath: [UUID] = []
    @Published var profilingData: [UUID: Double] = [:]
    
    func addBreakpoint(at nodeID: UUID) {
        breakpoints.insert(nodeID)
        print("[VisualScript] Added breakpoint at node \(nodeID)")
    }
    
    func removeBreakpoint(at nodeID: UUID) {
        breakpoints.remove(nodeID)
    }
    
    func toggleBreakpoint(at nodeID: UUID) {
        if breakpoints.contains(nodeID) {
            breakpoints.remove(nodeID)
        } else {
            breakpoints.insert(nodeID)
        }
    }
    
    func addWatchVariable(_ variableID: UUID) {
        watchVariables.insert(variableID)
        print("[VisualScript] Watching variable \(variableID)")
    }
    
    func startExecutionVisualization() {
        executionPath.removeAll()
        print("[VisualScript] Started execution flow visualization")
    }
    
    func recordExecution(nodeID: UUID, duration: Double) {
        executionPath.append(nodeID)
        profilingData[nodeID] = duration
    }
    
    func startProfiling() {
        profilingData.removeAll()
        print("[VisualScript] Started blueprint profiling")
    }
    
    func getProfilingReport() -> String {
        var report = "Blueprint Profiling Report\n"
        report += "===========================\n"
        for (nodeID, duration) in profilingData.sorted(by: { $0.value > $1.value }) {
            if let node = nodes.first(where: { $0.id == nodeID }) {
                report += "\(node.displayName): \(String(format: "%.2f", duration))ms\n"
            }
        }
        return report
    }
    
    // MARK: - Blueprint Diff & Merge
    
    struct BlueprintDiff {
        var addedNodes: [ScriptNode]
        var removedNodes: [ScriptNode]
        var modifiedNodes: [(old: ScriptNode, new: ScriptNode)]
        var addedConnections: [ScriptConnection]
        var removedConnections: [ScriptConnection]
    }
    
    func diff(from other: VisualScriptingEditor) -> BlueprintDiff {
        let addedNodes = nodes.filter { node in
            !other.nodes.contains(where: { $0.id == node.id })
        }
        let removedNodes = other.nodes.filter { node in
            !nodes.contains(where: { $0.id == node.id })
        }
        let modifiedNodes: [(ScriptNode, ScriptNode)] = []
        let addedConnections = connections.filter { conn in
            !other.connections.contains(where: { $0.id == conn.id })
        }
        let removedConnections = other.connections.filter { conn in
            !connections.contains(where: { $0.id == conn.id })
        }
        
        print("[VisualScript] Generated diff: +\(addedNodes.count) nodes, -\(removedNodes.count) nodes")
        
        return BlueprintDiff(
            addedNodes: addedNodes,
            removedNodes: removedNodes,
            modifiedNodes: modifiedNodes,
            addedConnections: addedConnections,
            removedConnections: removedConnections
        )
    }
    
    func merge(diff: BlueprintDiff) {
        nodes.append(contentsOf: diff.addedNodes)
        nodes.removeAll { node in
            diff.removedNodes.contains(where: { $0.id == node.id })
        }
        connections.append(contentsOf: diff.addedConnections)
        connections.removeAll { conn in
            diff.removedConnections.contains(where: { $0.id == conn.id })
        }
        print("[VisualScript] Merged blueprint changes")
    }
    
    func compileScript() -> String {
        var code = "// Generated Visual Script Code\n"
        code += "class GeneratedScript {\n"
        
        // Find entry point nodes (event nodes)
        let eventNodes = nodes.filter { node in
            switch node.type {
            case .onStart, .onUpdate, .onCollision, .onTrigger:
                return true
            default:
                return false
            }
        }
        
        for eventNode in eventNodes {
            code += "    func \(eventNode.type.rawValue.replacingOccurrences(of: " ", with: "_").lowercased())() {\n"
            code += generateCodeForNode(eventNode, indent: 2)
            code += "    }\n\n"
        }
        
        code += "}\n"
        print("[VisualScript] Generated \(code.split(separator: "\n").count) lines of code")
        return code
    }
    
    private func generateCodeForNode(_ node: ScriptNode, indent: Int) -> String {
        let indentStr = String(repeating: "    ", count: indent)
        var code = ""
        
        switch node.type {
        case .branch:
            code += "\(indentStr)if condition {\n"
            code += "\(indentStr)    // True path\n"
            code += "\(indentStr)} else {\n"
            code += "\(indentStr)    // False path\n"
            code += "\(indentStr)}\n"
        case .forLoop:
            code += "\(indentStr)for i in start...end {\n"
            code += "\(indentStr)    // Loop body\n"
            code += "\(indentStr)}\n"
        case .whileLoop:
            code += "\(indentStr)while condition {\n"
            code += "\(indentStr)    // Loop body\n"
            code += "\(indentStr)}\n"
        case .add:
            code += "\(indentStr)let result = a + b\n"
        case .subtract:
            code += "\(indentStr)let result = a - b\n"
        case .multiply:
            code += "\(indentStr)let result = a * b\n"
        case .divide:
            code += "\(indentStr)let result = a / b\n"
        case .spawnEntity:
            code += "\(indentStr)let entity = engine.spawnEntity(at: position, template: template)\n"
        case .destroyEntity:
            code += "\(indentStr)engine.destroyEntity(entity)\n"
        case .playAnimation:
            code += "\(indentStr)entity.playAnimation(animation)\n"
        case .playSound:
            code += "\(indentStr)audio.playSound(sound, volume: volume)\n"
        default:
            code += "\(indentStr)// \(node.type.rawValue)\n"
        }
        
        return code
    }
}

// MARK: - Visual Scripting Editor Panel
struct VisualScriptingEditorPanel: View {
    @ObservedObject var editor: VisualScriptingEditor
    @State private var showingNodeLibrary = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Visual Scripting")
                    .font(DesignSystem.Typography.h3)
                
                Spacer()
                
                EditorIconButton(icon: "book", tooltip: "Node Library") {
                    showingNodeLibrary.toggle()
                }
                
                EditorIconButton(icon: "play", tooltip: "Compile & Run") {
                    _ = editor.compileScript()
                }
                
                EditorIconButton(icon: "ladybug", tooltip: "Debug") {
                    // Toggle debug mode
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Canvas
            GeometryReader { geometry in
                ZStack {
                    // Grid
                    Color.black.opacity(0.1)
                    
                    // Connections
                    ForEach(editor.connections) { connection in
                        ScriptConnectionView(connection: connection, nodes: editor.nodes)
                    }
                    
                    // Nodes
                    ForEach(editor.nodes) { node in
                        ScriptNodeView(
                            node: node,
                            isSelected: editor.selectedNodes.contains(node.id)
                        )
                        .offset(x: node.position.x, y: node.position.y)
                    }
                }
            }
        }
        .sheet(isPresented: $showingNodeLibrary) {
            ScriptNodeLibraryView(editor: editor, isPresented: $showingNodeLibrary)
        }
    }
}

// MARK: - Script Node View
private struct ScriptNodeView: View {
    let node: VisualScriptingEditor.ScriptNode
    let isSelected: Bool
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text(node.displayName)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(.white)
                Spacer()
            }
            .padding(8)
            .background(node.color)
            
            // Pins
            VStack(spacing: 4) {
                ForEach(node.inputs) { pin in
                    HStack {
                        Circle()
                            .fill(pin.color)
                            .frame(width: 12, height: 12)
                        Text(pin.name)
                            .font(DesignSystem.Typography.small)
                        Spacer()
                    }
                }
                
                ForEach(node.outputs) { pin in
                    HStack {
                        Spacer()
                        Text(pin.name)
                            .font(DesignSystem.Typography.small)
                        Circle()
                            .fill(pin.color)
                            .frame(width: 12, height: 12)
                    }
                }
            }
            .padding(8)
        }
        .frame(width: 200)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
        .overlay(
            RoundedRectangle(cornerRadius: 4)
                .stroke(isSelected ? Color.yellow : Color.clear, lineWidth: 2)
        )
    }
}

// MARK: - Script Connection View
private struct ScriptConnectionView: View {
    let connection: VisualScriptingEditor.ScriptConnection
    let nodes: [VisualScriptingEditor.ScriptNode]
    
    var body: some View {
        let fromNode = nodes.first(where: { $0.id == connection.fromNode })
        let toNode = nodes.first(where: { $0.id == connection.toNode })
        
        guard let from = fromNode, let to = toNode else {
            return AnyView(EmptyView())
        }
        
        let fromPin = from.outputs.first(where: { $0.id == connection.fromPin })
        let toPin = to.inputs.first(where: { $0.id == connection.toPin })
        
        // Calculate pin positions
        let fromPinIndex = from.outputs.firstIndex(where: { $0.id == connection.fromPin }) ?? 0
        let toPinIndex = to.inputs.firstIndex(where: { $0.id == connection.toPin }) ?? 0
        
        let fromPos = CGPoint(
            x: from.position.x + 200, // Node width
            y: from.position.y + 40 + CGFloat(from.inputs.count + fromPinIndex) * 20
        )
        let toPos = CGPoint(
            x: to.position.x,
            y: to.position.y + 40 + CGFloat(toPinIndex) * 20
        )
        
        return AnyView(
            Path { path in
                let distance = abs(toPos.x - fromPos.x)
                let controlOffset = min(distance * 0.5, 100)
                
                path.move(to: fromPos)
                path.addCurve(
                    to: toPos,
                    control1: CGPoint(x: fromPos.x + controlOffset, y: fromPos.y),
                    control2: CGPoint(x: toPos.x - controlOffset, y: toPos.y)
                )
            }
            .stroke(fromPin?.color ?? .white, lineWidth: 2)
        )
    }
}

// MARK: - Script Node Library View
private struct ScriptNodeLibraryView: View {
    @ObservedObject var editor: VisualScriptingEditor
    @Binding var isPresented: Bool
    
    var body: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Node Library")
                    .font(DesignSystem.Typography.h2)
                Spacer()
            }
            .padding()
            
            EditorDivider()
            
            ScrollView {
                VStack(alignment: .leading, spacing: 16) {
                    ForEach(VisualScriptingEditor.NodeCategory.allCases, id: \.self) { category in
                        VStack(alignment: .leading, spacing: 8) {
                            Text(category.rawValue)
                                .font(DesignSystem.Typography.bodyBold)
                            
                            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
                                ForEach(VisualScriptingEditor.ScriptNodeType.allCases.filter { $0.category == category }, id: \.self) { type in
                                    Button(action: {
                                        editor.addNode(type: type, at: CGPoint(x: 100, y: 100))
                                        isPresented = false
                                    }) {
                                        Text(type.rawValue)
                                            .font(DesignSystem.Typography.small)
                                            .foregroundColor(DesignSystem.Colors.textPrimary)
                                            .padding(8)
                                            .background(type.color.opacity(0.3))
                                            .cornerRadius(4)
                                    }
                                    .buttonStyle(.plain)
                                }
                            }
                        }
                    }
                }
                .padding()
            }
        }
        .frame(width: 500, height: 700)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}
