import SwiftUI

// 
// MARK: - CATEGORY 6: VISUAL SCRIPTING SYSTEM (TODO-3101 to TODO-3700)
// 

// MARK: - Node Graph Canvas TODOs (TODO-3101 to TODO-3150)
// TODO-3101: Canvas infinite scrolling
// TODO-3102: Canvas smooth panning with mouse drag
// TODO-3103: Canvas zoom with mouse wheel
// TODO-3104: Canvas zoom to fit all nodes
// TODO-3105: Canvas zoom to selection
// TODO-3106: Canvas minimap/overview
// TODO-3107: Canvas grid snap toggle
// TODO-3108: Canvas grid size adjustment
// TODO-3109: Canvas background pattern options
// TODO-3110: Canvas rulers and guides
// TODO-3111: Canvas bookmarks/saved views
// TODO-3112: Canvas comment boxes (groups)
// TODO-3113: Canvas node alignment tools
// TODO-3114: Canvas node distribution tools
// TODO-3115: Canvas auto-layout algorithm
// TODO-3116: Canvas straighten connections
// TODO-3117: Canvas selection highlighting
// TODO-3118: Canvas multi-select with box
// TODO-3119: Canvas multi-select with Shift+click
// TODO-3120: Canvas select all connected
// TODO-3121: Canvas select downstream/upstream
// TODO-3122: Canvas focus on node (frame selected)
// TODO-3123: Canvas node search/filter
// TODO-3124: Canvas performance mode (simplified rendering)
// TODO-3125: Canvas debug overlays

// MARK: - Node System TODOs (TODO-3151 to TODO-3200)
// TODO-3151: Node collapse/expand
// TODO-3152: Node rename inline
// TODO-3153: Node color customization
// TODO-3154: Node icon customization
// TODO-3155: Node preview (shows current value)
// TODO-3156: Node tooltip with description
// TODO-3157: Node error indicators
// TODO-3158: Node warning indicators
// TODO-3159: Node execution indicator (debug)
// TODO-3160: Node breakpoint toggle
// TODO-3161: Node copy/paste
// TODO-3162: Node duplicate
// TODO-3163: Node delete with confirmation
// TODO-3164: Node disable/bypass
// TODO-3165: Node replace type
// TODO-3166: Node convert to function
// TODO-3167: Node extract to subgraph
// TODO-3168: Node inline subgraph
// TODO-3169: Node documentation popup
// TODO-3170: Node custom icons per type
// TODO-3171: Node size auto-adjustment
// TODO-3172: Node minimum size constraints
// TODO-3173: Node header collapse
// TODO-3174: Node input value editing
// TODO-3175: Node output value preview

// MARK: - Port System TODOs (TODO-3201 to TODO-3250)
// TODO-3201: Port type visualization (color, shape)
// TODO-3202: Port name display toggle
// TODO-3203: Port tooltip with type info
// TODO-3204: Port default value editing
// TODO-3205: Port connection validation
// TODO-3206: Port type conversion
// TODO-3207: Port array/list support
// TODO-3208: Port wildcard/any type
// TODO-3209: Port generics support
// TODO-3210: Port optional (nullable) type
// TODO-3211: Port execution flow (white)
// TODO-3212: Port boolean (red)
// TODO-3213: Port integer (cyan)
// TODO-3214: Port float (green)
// TODO-3215: Port vector2 (light yellow)
// TODO-3216: Port vector3 (yellow)
// TODO-3217: Port vector4/quaternion (orange)
// TODO-3218: Port string (magenta)
// TODO-3219: Port object reference (blue)
// TODO-3220: Port struct/class (purple)
// TODO-3221: Port enum (teal)
// TODO-3222: Port asset reference (gold)
// TODO-3223: Port delegate/event (pink)
// TODO-3224: Port array (dashed outline)
// TODO-3225: Port dictionary/map

// MARK: - Connection System TODOs (TODO-3251 to TODO-3300)
// TODO-3251: Connection bezier curves
// TODO-3252: Connection straight lines option
// TODO-3253: Connection orthogonal lines option
// TODO-3254: Connection color by type
// TODO-3255: Connection thickness option
// TODO-3256: Connection animation (flow direction)
// TODO-3257: Connection highlight on hover
// TODO-3258: Connection selection
// TODO-3259: Connection delete
// TODO-3260: Connection reroute points
// TODO-3261: Connection hide for clarity
// TODO-3262: Connection validation errors
// TODO-3263: Connection temporary during drag
// TODO-3264: Connection snap to valid ports
// TODO-3265: Connection auto-insert node
// TODO-3266: Connection split with node
// TODO-3267: Connection merge paths
// TODO-3268: Connection loop detection
// TODO-3269: Connection execution order numbering
// TODO-3270: Connection data flow visualization

// MARK: - Node Library TODOs (TODO-3301 to TODO-3350)
// TODO-3301: Node palette/library panel
// TODO-3302: Node search with fuzzy matching
// TODO-3303: Node categories/folders
// TODO-3304: Node favorites
// TODO-3305: Node recent used
// TODO-3306: Node drag from library
// TODO-3307: Node context menu creation
// TODO-3308: Node keyboard shortcut creation
// TODO-3309: Node quick-add (type name)
// TODO-3310: Event nodes (OnStart, OnUpdate, OnDestroy)
// TODO-3311: Input event nodes (keyboard, mouse, touch)
// TODO-3312: Collision event nodes
// TODO-3313: Trigger event nodes
// TODO-3314: Custom event nodes
// TODO-3315: Flow control nodes (Branch, Switch, Select)
// TODO-3316: Loop nodes (For, ForEach, While, DoWhile)
// TODO-3317: Sequence node
// TODO-3318: Gate/latch nodes
// TODO-3319: Delay/timer nodes
// TODO-3320: Async/await nodes
// TODO-3321: Math nodes (arithmetic, trigonometry)
// TODO-3322: Vector math nodes
// TODO-3323: Matrix math nodes
// TODO-3324: Logic nodes (AND, OR, NOT, XOR)
// TODO-3325: Comparison nodes
// TODO-3326: String manipulation nodes
// TODO-3327: Array manipulation nodes
// TODO-3328: Dictionary manipulation nodes
// TODO-3329: Object access nodes (get/set property)
// TODO-3330: Component access nodes
// TODO-3331: Transform manipulation nodes
// TODO-3332: Physics nodes (raycast, forces)
// TODO-3333: Audio nodes (play, stop, volume)
// TODO-3334: Animation nodes (play, blend, event)
// TODO-3335: UI nodes (show, hide, update)
// TODO-3336: Networking nodes
// TODO-3337: File I/O nodes
// TODO-3338: Debug nodes (print, log, assert)
// TODO-3339: Comment/sticky note nodes
// TODO-3340: Reroute nodes

// MARK: - Functions & Macros TODOs (TODO-3351 to TODO-3400)
// TODO-3351: Function graph creation
// TODO-3352: Function parameter definition
// TODO-3353: Function return value definition
// TODO-3354: Function local variables
// TODO-3355: Function call node generation
// TODO-3356: Function recursion support
// TODO-3357: Function overloading
// TODO-3358: Pure function marking
// TODO-3359: Latent function support
// TODO-3360: Function documentation
// TODO-3361: Macro definition
// TODO-3362: Macro expansion
// TODO-3363: Macro parameters
// TODO-3364: Collapsed graph (inline subgraph)
// TODO-3365: Graph library/asset
// TODO-3366: Graph instantiation
// TODO-3367: Graph input/output nodes
// TODO-3368: Graph local event dispatch
// TODO-3369: Graph parent reference
// TODO-3370: Graph reusable templates

// MARK: - Variables & Data TODOs (TODO-3401 to TODO-3450)
// TODO-3401: Variable creation panel
// TODO-3402: Variable type selection
// TODO-3403: Variable default value
// TODO-3404: Variable scope (local, instance, static)
// TODO-3405: Variable get node generation
// TODO-3406: Variable set node generation
// TODO-3407: Variable watch in debugger
// TODO-3408: Variable rename refactoring
// TODO-3409: Variable find all references
// TODO-3410: Constant value nodes
// TODO-3411: Literal input nodes
// TODO-3412: Enum value nodes
// TODO-3413: Struct construction nodes
// TODO-3414: Object instantiation nodes
// TODO-3415: Array literal nodes
// TODO-3416: Dictionary literal nodes
// TODO-3417: Type casting nodes
// TODO-3418: Type checking nodes
// TODO-3419: Null check nodes
// TODO-3420: Safe access nodes

// MARK: - Debugging TODOs (TODO-3451 to TODO-3500)
// TODO-3451: Breakpoint system
// TODO-3452: Step through execution
// TODO-3453: Step into function
// TODO-3454: Step over node
// TODO-3455: Step out of function
// TODO-3456: Continue execution
// TODO-3457: Pause execution
// TODO-3458: Stop execution
// TODO-3459: Call stack display
// TODO-3460: Local variables watch
// TODO-3461: Instance variables watch
// TODO-3462: Expression evaluation
// TODO-3463: Conditional breakpoints
// TODO-3464: Hit count breakpoints
// TODO-3465: Log points (tracepoints)
// TODO-3466: Execution path highlighting
// TODO-3467: Data flow visualization
// TODO-3468: Performance profiling per-node
// TODO-3469: Execution time display
// TODO-3470: Memory allocation tracking

// MARK: - Compilation & Optimization TODOs (TODO-3501 to TODO-3550)
// TODO-3501: Graph compilation to bytecode
// TODO-3502: Graph compilation to native code
// TODO-3503: Incremental compilation
// TODO-3504: Compilation error reporting
// TODO-3505: Compilation warning reporting
// TODO-3506: Error node highlighting
// TODO-3507: Error navigation
// TODO-3508: Syntax validation
// TODO-3509: Type checking
// TODO-3510: Dead code detection
// TODO-3511: Unreachable code warning
// TODO-3512: Infinite loop detection
// TODO-3513: Null reference checking
// TODO-3514: Optimization passes
// TODO-3515: Constant folding
// TODO-3516: Dead code elimination
// TODO-3517: Common subexpression elimination
// TODO-3518: Loop optimization
// TODO-3519: Inline expansion
// TODO-3520: Memory optimization

// MARK: - Integration TODOs (TODO-3551 to TODO-3600)
// TODO-3551: C# script interop
// TODO-3552: Swift script interop
// TODO-3553: C function calling
// TODO-3554: Component system integration
// TODO-3555: Entity system integration
// TODO-3556: Scene system integration
// TODO-3557: Asset system integration
// TODO-3558: Event system integration
// TODO-3559: Message bus integration
// TODO-3560: Save/load graph state
// TODO-3561: Hot reload support
// TODO-3562: Version control friendly format
// TODO-3563: Merge conflict resolution
// TODO-3564: Graph diffing
// TODO-3565: Import from other formats
// TODO-3566: Export to other formats

// MARK: - Macro System
struct GraphMacro: Identifiable, Codable {
    let id = UUID()
    var name: String
    var description: String
    var parameters: [MacroParameter]
    var nodes: [GraphNode]
    var connections: [GraphConnection]
    var isCollapsed: Bool = true
    
    func expand(with parameterValues: [String: Any]) -> (nodes: [GraphNode], connections: [GraphConnection]) {
        var expandedNodes: [GraphNode] = []
        var nodeMapping: [UUID: UUID] = [:]
        
        // Clone nodes with new IDs and substitute parameter values
        for node in nodes {
            var newNode = node
            newNode.id = UUID()
            nodeMapping[node.id] = newNode.id
            
            // Substitute parameter values in node properties
            if node.type == .graphInput, let paramName = node.title.split(separator: " ").last {
                if let value = parameterValues[String(paramName)] {
                    newNode.title = "\\(value)"
                }
            }
            
            expandedNodes.append(newNode)
        }
        
        // Clone connections with new node IDs
        var expandedConnections: [GraphConnection] = []
        for connection in connections {
            if let newFromNode = nodeMapping[connection.fromNode],
               let newToNode = nodeMapping[connection.toNode] {
                let newConnection = GraphConnection(
                    fromNode: newFromNode,
                    fromPort: connection.fromPort,
                    toNode: newToNode,
                    toPort: connection.toPort
                )
                expandedConnections.append(newConnection)
            }
        }
        
        return (expandedNodes, expandedConnections)
    }
    
    // MARK: - Function and Subgraph Operations
    
    /// Converts selected nodes to a reusable function
    func convertNodeToFunction(nodeIds: Set<UUID>, functionName: String) -> GraphFunction? {
        guard !nodeIds.isEmpty else { return nil }
        
        // Extract nodes and connections
        let selectedNodes = nodes.filter { nodeIds.contains($0.id) }
        let selectedConnections = connections.filter { conn in
            nodeIds.contains(conn.fromNode) && nodeIds.contains(conn.toNode)
        }
        
        // Identify external connections (inputs and outputs)
        var inputPorts: [FunctionPort] = []
        var outputPorts: [FunctionPort] = []
        
        for connection in connections {
            if nodeIds.contains(connection.toNode) && !nodeIds.contains(connection.fromNode) {
                // External input to this function
                if let fromNode = nodes.first(where: { $0.id == connection.fromNode }),
                   let fromPort = fromNode.outputs.indices.contains(connection.fromPort) ? fromNode.outputs[connection.fromPort] : nil,
                   let toNode = nodes.first(where: { $0.id == connection.toNode }),
                   let toPort = toNode.inputs.indices.contains(connection.toPort) ? toNode.inputs[connection.toPort] : nil {
                    
                    inputPorts.append(FunctionPort(
                        name: toPort.name,
                        type: toPort.type,
                        description: "Input from \(fromNode.title).\(fromPort.name)"
                    ))
                }
            }
            
            if nodeIds.contains(connection.fromNode) && !nodeIds.contains(connection.toNode) {
                // Output from this function to external node
                if let fromNode = nodes.first(where: { $0.id == connection.fromNode }),
                   let fromPort = fromNode.outputs.indices.contains(connection.fromPort) ? fromNode.outputs[connection.fromPort] : nil,
                   let toNode = nodes.first(where: { $0.id == connection.toNode }),
                   let toPort = toNode.inputs.indices.contains(connection.toPort) ? toNode.inputs[connection.toPort] : nil {
                    
                    outputPorts.append(FunctionPort(
                        name: fromPort.name,
                        type: fromPort.type,
                        description: "Output to \(toNode.title).\(toPort.name)"
                    ))
                }
            }
        }
        
        // Create the function
        let function = GraphFunction(
            name: functionName,
            description: "Converted from \(selectedNodes.count) nodes",
            category: .userDefined,
            nodes: selectedNodes,
            connections: selectedConnections,
            inputs: inputPorts,
            outputs: outputPorts
        )
        
        // Store the function
        functions[function.id] = function
        
        // Replace selected nodes with function call node
        let functionNode = GraphNode(
            type: .functionCall,
            title: functionName,
            position: CGPoint(x: 100, y: 100),
            inputs: inputPorts.map { NodePort(name: $0.name, type: $0.type) },
            outputs: outputPorts.map { NodePort(name: $0.name, type: $0.type) }
        )
        
        // Remove original nodes and add function node
        for nodeId in nodeIds {
            deleteNode(nodeId)
        }
        addNode(functionNode)
        
        return function
    }
    
    /// Extracts selected nodes to a reusable subgraph
    func extractToSubgraph(nodeIds: Set<UUID>, subgraphName: String) -> GraphTemplate? {
        guard !nodeIds.isEmpty else { return nil }
        
        // Extract nodes and connections
        let selectedNodes = nodes.filter { nodeIds.contains($0.id) }
        let selectedConnections = connections.filter { conn in
            nodeIds.contains(conn.fromNode) && nodeIds.contains(conn.toNode)
        }
        
        // Identify external connections
        var templateInputs: [TemplatePort] = []
        var templateOutputs: [TemplatePort] = []
        
        for connection in connections {
            if nodeIds.contains(connection.toNode) && !nodeIds.contains(connection.fromNode) {
                // External input
                if let toNode = nodes.first(where: { $0.id == connection.toNode }),
                   let toPort = toNode.inputs.indices.contains(connection.toPort) ? toNode.inputs[connection.toPort] : nil {
                    
                    templateInputs.append(TemplatePort(
                        name: toPort.name,
                        type: toPort.type,
                        description: "Input port \(toPort.name)"
                    ))
                }
            }
            
            if nodeIds.contains(connection.fromNode) && !nodeIds.contains(connection.toNode) {
                // External output
                if let fromNode = nodes.first(where: { $0.id == connection.fromNode }),
                   let fromPort = fromNode.outputs.indices.contains(connection.fromPort) ? fromNode.outputs[connection.fromPort] : nil {
                    
                    templateOutputs.append(TemplatePort(
                        name: fromPort.name,
                        type: fromPort.type,
                        description: "Output port \(fromPort.name)"
                    ))
                }
            }
        }
        
        // Create the template
        let template = GraphTemplate(
            name: subgraphName,
            description: "Extracted from \(selectedNodes.count) nodes",
            category: .userDefined,
            nodes: selectedNodes.map { node in
                GraphNodeData(
                    id: node.id,
                    type: node.type.rawValue,
                    title: node.title,
                    position: VCPosition(x: Int(node.position.x), y: Int(node.position.y)),
                    inputs: node.inputs.map { port in
                        NodePortData(
                            id: port.id,
                            name: port.name,
                            type: port.type.rawValue,
                            defaultValue: port.defaultValue?.description
                        )
                    },
                    outputs: node.outputs.map { port in
                        NodePortData(
                            id: port.id,
                            name: port.name,
                            type: port.type.rawValue,
                            defaultValue: port.defaultValue?.description
                        )
                    },
                    isCollapsed: node.isCollapsed,
                    comment: node.comment
                )
            },
            connections: selectedConnections.map { conn in
                GraphConnectionData(
                    id: conn.id,
                    fromNode: conn.fromNode,
                    fromPort: conn.fromPort,
                    toNode: conn.toNode,
                    toPort: conn.toPort
                )
            },
            inputs: templateInputs,
            outputs: templateOutputs
        )
        
        // Store the template
        templates[template.id] = template
        
        // Replace selected nodes with subgraph instance node
        let subgraphNode = GraphNode(
            type: .subgraphInstance,
            title: subgraphName,
            position: CGPoint(x: 100, y: 100),
            inputs: templateInputs.map { NodePort(name: $0.name, type: $0.type) },
            outputs: templateOutputs.map { NodePort(name: $0.name, type: $0.type) }
        )
        
        // Remove original nodes and add subgraph node
        for nodeId in nodeIds {
            deleteNode(nodeId)
        }
        addNode(subgraphNode)
        
        return template
    }
    
    /// Expands a subgraph instance into its constituent nodes
    func expandSubgraph(subgraphNodeId: UUID) -> (expandedNodes: [GraphNode], expandedConnections: [GraphConnection])? {
        guard let subgraphNode = nodes.first(where: { $0.id == subgraphNodeId }),
              subgraphNode.type == .subgraphInstance else { return nil }
        
        // Find the template for this subgraph
        guard let template = templates.values.first(where: { $0.name == subgraphNode.title }) else { return nil }
        
        var expandedNodes: [GraphNode] = []
        var expandedConnections: [GraphConnection] = []
        var nodeMapping: [UUID: UUID] = [:]
        
        // Convert template nodes back to GraphNode objects
        for nodeData in template.nodes {
            let newNode = GraphNode(
                type: NodeType(rawValue: nodeData.type) ?? .functionCall,
                title: nodeData.title,
                position: CGPoint(
                    x: nodeData.position.x + subgraphNode.position.x,
                    y: nodeData.position.y + subgraphNode.position.y
                ),
                inputs: nodeData.inputs.map { portData in
                    NodePort(
                        name: portData.name,
                        type: PortType(rawValue: portData.type) ?? .float,
                        defaultValue: portData.defaultValue
                    )
                },
                outputs: nodeData.outputs.map { portData in
                    NodePort(
                        name: portData.name,
                        type: PortType(rawValue: portData.type) ?? .float,
                        defaultValue: portData.defaultValue
                    )
                },
                isCollapsed: nodeData.isCollapsed,
                comment: nodeData.comment
            )
            
            nodeMapping[nodeData.id] = newNode.id
            expandedNodes.append(newNode)
        }
        
        // Convert template connections
        for connData in template.connections {
            if let newFromNode = nodeMapping[connData.fromNode],
               let newToNode = nodeMapping[connData.toNode] {
                let newConnection = GraphConnection(
                    fromNode: newFromNode,
                    fromPort: connData.fromPort,
                    toNode: newToNode,
                    toPort: connData.toPort
                )
                expandedConnections.append(newConnection)
            }
        }
        
        // Remove the subgraph instance node
        deleteNode(subgraphNodeId)
        
        // Add expanded nodes and connections
        for node in expandedNodes {
            addNode(node)
        }
        for connection in expandedConnections {
            connections.append(connection)
        }
        
        return (expandedNodes, expandedConnections)
    }
    
    /// Creates a new graph instance from a template
    func createGraphInstance(templateId: UUID, name: String, position: CGPoint) -> GraphInstance? {
        guard let template = templates[templateId] else { return nil }
        
        let instance = GraphInstance(
            id: UUID(),
            templateId: templateId,
            name: name,
            parameterValues: [:],
            isCollapsed: false
        )
        
        // Create instance node
        let instanceNode = GraphNode(
            type: .subgraphInstance,
            title: name,
            position: position,
            inputs: template.inputs.map { NodePort(name: $0.name, type: $0.type) },
            outputs: template.outputs.map { NodePort(name: $0.name, type: $0.type) }
        )
        
        addNode(instanceNode)
        instances[instance.id] = instance
        
        return instance
    }
}

struct MacroParameter: Identifiable, Codable {
    let id = UUID()
    var name: String
    var type: PortType
    var defaultValue: Any?
    var description: String
}

// MARK: - Variable System
struct GraphVariable: Identifiable, Codable {
    let id = UUID()
    var name: String
    var type: PortType
    var scope: VariableScope
    var defaultValue: Any?
    var description: String
    
    enum VariableScope: String, CaseIterable, Codable {
        case local = "Local"
        case instance = "Instance"
        case static = "Static"
    }
}

// MARK: - Graph Template System
struct GraphTemplate: Identifiable, Codable {
    let id = UUID()
    var name: String
    var description: String
    var category: String
    var nodes: [GraphNode]
    var connections: [GraphConnection]
    var inputs: [TemplatePort]
    var outputs: [TemplatePort]
    var preview: String?
}

struct TemplatePort: Identifiable, Codable {
    let id = UUID()
    var name: String
    var type: PortType
    var description: String
}

// MARK: - Graph Instance System
struct GraphInstance: Identifiable, Codable {
    let id = UUID()
    var templateId: UUID
    var name: String
    var position: CGPoint
    var parameterValues: [String: Any]
    var isCollapsed: Bool = false
    var parentGraphId: UUID?
}

// MARK: - Graph Serialization

struct GraphData: Codable {
    var nodes: [GraphNodeData]
    var connections: [GraphConnectionData]
    var metadata: GraphMetadata
}

struct GraphNodeData: Codable {
    var id: UUID
    var type: String
    var title: String
    var position: CGPoint
    var inputs: [NodePortData]
    var outputs: [NodePortData]
    var isCollapsed: Bool
    var comment: String
}

struct NodePortData: Codable {
    var id: UUID
    var name: String
    var type: String
    var defaultValue: String?
}

struct GraphConnectionData: Codable {
    var id: UUID
    var fromNode: UUID
    var fromPort: Int
    var toNode: UUID
    var toPort: Int
}

struct GraphMetadata: Codable {
    var version: String
    var created: Date
    var modified: Date
    var author: String
    var description: String
}

// MARK: - Version Control Friendly Format Structures

struct VCFormatData {
    let version: String
    let format: String
    let metadata: VCMetadata
    let nodes: [VCNodeData]
    let connections: [VCConnectionData]
}

struct VCMetadata {
    let name: String
    let created: String
    let description: String
}

struct VCNodeData {
    let id: String
    let type: String
    let title: String
    let position: VCPosition
    let inputs: [VCPortData]
    let outputs: [VCPortData]
    let metadata: VCNodeMetadata?
}

struct VCPosition {
    let x: Int
    let y: Int
}

struct VCPortData {
    let name: String
    let type: String
    let index: Int
}

struct VCNodeMetadata {
    let collapsed: Bool
    let comment: String?
}

struct VCConnectionData {
    let from: VCConnectionEndpoint
    let to: VCConnectionEndpoint
}

struct VCConnectionEndpoint {
    let node: String
    let port: String
}

// MARK: - Compilation & Optimization Systems

// MARK: - Compilation Error System
struct CompilationError: Identifiable {
    let id = UUID()
    let nodeId: UUID
    let message: String
    let severity: ErrorSeverity
    let category: ErrorCategory
    let line: Int?
    let column: Int?
    let suggestion: String?
}

enum ErrorSeverity: String, CaseIterable {
    case error = "Error"
    case warning = "Warning"
    case info = "Info"
    
    var color: Color {
        switch self {
        case .error: return .red
        case .warning: return .orange
        case .info: return .blue
        }
    }
    
    var icon: String {
        switch self {
        case .error: return "xmark.circle.fill"
        case .warning: return "exclamationmark.triangle.fill"
        case .info: return "info.circle.fill"
        }
    }
}

enum ErrorCategory: String, CaseIterable {
    case syntax = "Syntax"
    case type = "Type"
    case connection = "Connection"
    case logic = "Logic"
    case performance = "Performance"
    case runtime = "Runtime"
}

class CompilationManager: ObservableObject {
    static let shared = CompilationManager()
    
    @Published var errors: [CompilationError] = []
    @Published var warnings: [CompilationError] = []
    @Published var compilationInProgress = false
    @Published var lastCompilationTime: Date?
    
    func compileGraph(nodes: [GraphNode], connections: [GraphConnection]) {
        compilationInProgress = true
        errors.removeAll()
        warnings.removeAll()
        
        DispatchQueue.global(qos: .userInitiated).async {
            self.performCompilation(nodes: nodes, connections: connections)
            
            DispatchQueue.main.async {
                self.compilationInProgress = false
                self.lastCompilationTime = Date()
            }
        }
    }
    
    private func performCompilation(nodes: [GraphNode], connections: [GraphConnection]) {
        // Syntax validation
        validateSyntax(nodes: nodes, connections: connections)
        
        // Type checking
        validateTypes(nodes: nodes, connections: connections)
        
        // Connection validation
        validateConnections(nodes: nodes, connections: connections)
        
        // Logic validation
        validateLogic(nodes: nodes, connections: connections)
        
        // Performance analysis
        analyzePerformance(nodes: nodes, connections: connections)
    }
    
    private func validateSyntax(nodes: [GraphNode], connections: [GraphConnection]) {
        for node in nodes {
            // Check for required inputs
            for (index, input) in node.inputs.enumerated() {
                if input.defaultValue == nil {
                    let hasConnection = connections.contains { conn in
                        conn.toNode == node.id && conn.toPort == index
                    }
                    
                    if !hasConnection {
                        let error = CompilationError(
                            nodeId: node.id,
                            message: "Required input '\(input.name)' is not connected",
                            severity: .error,
                            category: .syntax,
                            line: nil,
                            column: nil,
                            suggestion: "Connect a node to this input or provide a default value"
                        )
                        DispatchQueue.main.async {
                            self.errors.append(error)
                        }
                    }
                }
            }
            
            // Validate node-specific syntax
            validateNodeSyntax(node)
        }
    }
    
    private func validateNodeSyntax(_ node: GraphNode) {
        switch node.type {
        case .math:
            // Math nodes should have at least one input
            if node.inputs.isEmpty {
                let error = CompilationError(
                    nodeId: node.id,
                    message: "Math node requires at least one input",
                    severity: .error,
                    category: .syntax,
                    line: nil,
                    column: nil,
                    suggestion: "Add input connections to the math node"
                )
                DispatchQueue.main.async {
                    self.errors.append(error)
                }
            }
            
        case .branch:
            // Branch nodes should have condition input
            if !node.inputs.contains(where: { $0.name.lowercased().contains("condition") }) {
                let warning = CompilationError(
                    nodeId: node.id,
                    message: "Branch node missing condition input",
                    severity: .warning,
                    category: .syntax,
                    line: nil,
                    column: nil,
                    suggestion: "Connect a boolean value to the condition input"
                )
                DispatchQueue.main.async {
                    self.warnings.append(warning)
                }
            }
            
        case .forLoop, .forEach:
            // Loop nodes should have iteration control
            if node.outputs.isEmpty {
                let error = CompilationError(
                    nodeId: node.id,
                    message: "Loop node must have execution outputs",
                    severity: .error,
                    category: .syntax,
                    line: nil,
                    column: nil,
                    suggestion: "Add execution flow outputs to the loop node"
                )
                DispatchQueue.main.async {
                    self.errors.append(error)
                }
            }
            
        default:
            break
        }
    }
    
    private func validateTypes(nodes: [GraphNode], connections: [GraphConnection]) {
        for connection in connections {
            guard let fromNode = nodes.first(where: { $0.id == connection.fromNode }),
                  let toNode = nodes.first(where: { $0.id == connection.toNode }),
                  connection.fromPort < fromNode.outputs.count,
                  connection.toPort < toNode.inputs.count else {
                continue
            }
            
            let fromPort = fromNode.outputs[connection.fromPort]
            let toPort = toNode.inputs[connection.toPort]
            
            // Check type compatibility
            if !areTypesCompatible(fromPort.type, toPort.type) {
                let error = CompilationError(
                    nodeId: toNode.id,
                    message: "Type mismatch: Cannot connect '\(fromPort.type.rawValue)' to '\(toPort.type.rawValue)'",
                    severity: .error,
                    category: .type,
                    line: nil,
                    column: nil,
                    suggestion: "Use a type conversion node or check connection compatibility"
                )
                DispatchQueue.main.async {
                    self.errors.append(error)
                }
            }
        }
    }
    
    private func areTypesCompatible(_ from: PortType, _ to: PortType) -> Bool {
        // Same types are compatible
        if from == to { return true }
        
        // 'any' type is compatible with everything
        if from == .any || to == .any { return true }
        
        // Numeric type conversions
        let numericTypes: [PortType] = [.int, .float, .vector3]
        if numericTypes.contains(from) && numericTypes.contains(to) { return true }
        
        // Flow type compatibility
        if from == .flow && to == .flow { return true }
        
        return false
    }
    
    private func validateConnections(nodes: [GraphNode], connections: [GraphConnection]) {
        // Check for multiple connections to input ports
        var inputConnections: [UUID: [Int]] = [:]
        
        for connection in connections {
            let key = connection.toNode
            if inputConnections[key] == nil {
                inputConnections[key] = []
            }
            inputConnections[key]?.append(connection.toPort)
        }
        
        for (nodeId, portIndices) in inputConnections {
            let connectedPorts = Dictionary(grouping: portIndices, by: { $0 })
            
            for (portIndex, connections) in connectedPorts {
                if connections.count > 1 {
                    let error = CompilationError(
                        nodeId: nodeId,
                        message: "Multiple connections to input port \(portIndex)",
                        severity: .error,
                        category: .connection,
                        line: nil,
                        column: nil,
                        suggestion: "Remove extra connections - input ports can only have one connection"
                    )
                    DispatchQueue.main.async {
                        self.errors.append(error)
                    }
                }
            }
        }
    }
    
    private func validateLogic(nodes: [GraphNode], connections: [GraphConnection]) {
        // Detect infinite loops
        detectInfiniteLoops(nodes: nodes, connections: connections)
        
        // Detect unreachable code
        detectUnreachableCode(nodes: nodes, connections: connections)
        
        // Detect null reference issues
        detectNullReferences(nodes: nodes, connections: connections)
    }
    
    private func detectInfiniteLoops(nodes: [GraphNode], connections: [GraphConnection]) {
        var visited: Set<UUID> = []
        var recursionStack: Set<UUID> = []
        
        func hasCycle(from nodeId: UUID) -> Bool {
            if recursionStack.contains(nodeId) {
                return true // Cycle detected
            }
            
            if visited.contains(nodeId) {
                return false
            }
            
            visited.insert(nodeId)
            recursionStack.insert(nodeId)
            
            // Check outgoing connections
            for connection in connections {
                if connection.fromNode == nodeId {
                    if hasCycle(from: connection.toNode) {
                        return true
                    }
                }
            }
            
            recursionStack.remove(nodeId)
            return false
        }
        
        for node in nodes {
            if hasCycle(from: node.id) {
                let error = CompilationError(
                    nodeId: node.id,
                    message: "Potential infinite loop detected",
                    severity: .warning,
                    category: .logic,
                    line: nil,
                    column: nil,
                    suggestion: "Add a break condition or check loop termination logic"
                )
                DispatchQueue.main.async {
                    self.warnings.append(error)
                }
                break
            }
        }
    }
    
    private func detectUnreachableCode(nodes: [GraphNode], connections: [GraphConnection]) {
        var reachableNodes: Set<UUID> = []
        
        // Find all event nodes (entry points)
        let eventNodes = nodes.filter { $0.type == .event }
        
        // BFS from event nodes
        var queue: [UUID] = eventNodes.map { $0.id }
        
        while !queue.isEmpty {
            let nodeId = queue.removeFirst()
            
            if !reachableNodes.contains(nodeId) {
                reachableNodes.insert(nodeId)
                
                // Find connected nodes
                for connection in connections {
                    if connection.fromNode == nodeId {
                        queue.append(connection.toNode)
                    }
                }
            }
        }
        
        // Check for unreachable nodes
        for node in nodes {
            if !reachableNodes.contains(node.id) && node.type != .comment {
                let warning = CompilationError(
                    nodeId: node.id,
                    message: "Node is unreachable from any event node",
                    severity: .warning,
                    category: .logic,
                    line: nil,
                    column: nil,
                    suggestion: "Connect this node to the execution flow or remove it"
                )
                DispatchQueue.main.async {
                    self.warnings.append(warning)
                }
            }
        }
    }
    
    private func detectNullReferences(nodes: [GraphNode], connections: [GraphConnection]) {
        for node in nodes {
            // Check for potential null dereferences
            if node.title.contains("Get") || node.title.contains("Access") {
                let hasNullCheck = connections.contains { conn in
                    conn.toNode == node.id && 
                    conn.toPort < node.inputs.count &&
                    node.inputs[conn.toPort].name.lowercased().contains("null")
                }
                
                if !hasNullCheck {
                    let warning = CompilationError(
                        nodeId: node.id,
                        message: "Potential null reference - consider adding null check",
                        severity: .warning,
                        category: .runtime,
                        line: nil,
                        column: nil,
                        suggestion: "Add a null check before accessing this value"
                    )
                    DispatchQueue.main.async {
                        self.warnings.append(warning)
                    }
                }
            }
        }
    }
    
    private func analyzePerformance(nodes: [GraphNode], connections: [GraphConnection]) {
        // Check for expensive operations in loops
        var loopNodes: Set<UUID> = []
        
        for node in nodes {
            if node.type == .forLoop || node.type == .forEach {
                loopNodes.insert(node.id)
            }
        }
        
        // Find nodes inside loops
        var nodesInLoops: Set<UUID> = []
        
        for loopNode in loopNodes {
            var visited: Set<UUID> = []
            var queue: [UUID] = [loopNode]
            
            while !queue.isEmpty {
                let nodeId = queue.removeFirst()
                
                if !visited.contains(nodeId) {
                    visited.insert(nodeId)
                    nodesInLoops.insert(nodeId)
                    
                    for connection in connections {
                        if connection.fromNode == nodeId {
                            queue.append(connection.toNode)
                        }
                    }
                }
            }
        }
        
        // Check for expensive operations in loops
        for nodeId in nodesInLoops {
            if let node = nodes.first(where: { $0.id == nodeId }) {
                if node.type == .math && node.title.contains("Complex") {
                    let warning = CompilationError(
                        nodeId: nodeId,
                        message: "Expensive operation inside loop",
                        severity: .warning,
                        category: .performance,
                        line: nil,
                        column: nil,
                        suggestion: "Consider moving this operation outside the loop or optimizing it"
                    )
                    DispatchQueue.main.async {
                        self.warnings.append(warning)
                    }
                }
            }
        }
    }
}

// MARK: - Error Reporting View
struct CompilationErrorPanel: View {
    @ObservedObject var compilationManager = CompilationManager.shared
    @State private var selectedError: CompilationError?
    @State private var showErrors = true
    @State private var showWarnings = true
    
    private var graphManager = NodeGraphManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Compilation Results")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                
                Spacer()
                
                if compilationManager.compilationInProgress {
                    ProgressView()
                        .scaleEffect(0.8)
                } else if let lastTime = compilationManager.lastCompilationTime {
                    Text("Last: \(lastTime, style: .time)")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.white.opacity(0.6))
                }
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color(red: 0.15, green: 0.15, blue: 0.18))
            
            // Filter controls
            HStack(spacing: 16) {
                Toggle("Errors", isOn: $showErrors)
                    .toggleStyle(CheckboxToggleStyle())
                
                Toggle("Warnings", isOn: $showWarnings)
                    .toggleStyle(CheckboxToggleStyle())
                
                Spacer()
                
                Button("Clear All") {
                    compilationManager.errors.removeAll()
                    compilationManager.warnings.removeAll()
                }
                .buttonStyle(.plain)
                .foregroundColor(.white.opacity(0.6))
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 6)
            .background(Color(red: 0.12, green: 0.12, blue: 0.14))
            
            // Error list
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    if showErrors {
                        ForEach(compilationManager.errors) { error in
                            CompilationErrorRow(error: error, isSelected: selectedError?.id == error.id)
                                .onTapGesture {
                                    selectedError = error
                                }
                        }
                    }
                    
                    if showWarnings {
                        ForEach(compilationManager.warnings) { warning in
                            CompilationErrorRow(error: warning, isSelected: selectedError?.id == warning.id)
                                .onTapGesture {
                                    selectedError = warning
                                }
                        }
                    }
                }
            }
            
            // Error details
            if let selectedError = selectedError {
                CompilationErrorDetails(error: selectedError) {
                    graphManager.navigateToError(selectedError)
                }
            }
        }
        .frame(width: 300)
        .background(Color(red: 0.1, green: 0.1, blue: 0.12))
    }
}

struct CompilationErrorRow: View {
    let error: CompilationError
    let isSelected: Bool
    
    var body: some View {
        HStack(alignment: .top, spacing: 8) {
            Image(systemName: error.severity.icon)
                .foregroundColor(error.severity.color)
                .font(.system(size: 12))
            
            VStack(alignment: .leading, spacing: 2) {
                Text(error.message)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(.white)
                    .lineLimit(2)
                
                HStack(spacing: 8) {
                    Text(error.category.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.white.opacity(0.6))
                    
                    Text(error.severity.rawValue)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(error.severity.color.opacity(0.8))
                }
            }
            
            Spacer()
        }
        .padding(8)
        .background(isSelected ? Color.white.opacity(0.1) : Color.clear)
        .contentShape(Rectangle())
    }
}

struct CompilationErrorDetails: View {
    let error: CompilationError
    let onNavigate: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Divider()
                .background(Color.white.opacity(0.2))
            
            HStack {
                Image(systemName: error.severity.icon)
                    .foregroundColor(error.severity.color)
                
                Text(error.severity.rawValue)
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(error.severity.color)
                
                Spacer()
                
                Button(action: onNavigate) {
                    Image(systemName: "arrow.right.circle")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .help("Navigate to node")
                
                Text(error.category.rawValue)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white.opacity(0.6))
            }
            
            Text(error.message)
                .font(DesignSystem.Typography.body)
                .foregroundColor(.white)
            
            if let suggestion = error.suggestion {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Suggestion:")
                        .font(DesignSystem.Typography.smallBold)
                        .foregroundColor(.white.opacity(0.8))
                    
                    Text(suggestion)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.white.opacity(0.7))
                }
            }
        }
        .padding(12)
        .background(Color(red: 0.08, green: 0.08, blue: 0.1))
    }
}

// MARK: - Graph Compilation System

// Bytecode instruction types
enum BytecodeInstruction: Codable {
    case nop
    case loadConstant(value: Any)
    case loadVariable(name: String)
    case storeVariable(name: String)
    case binaryOperation(op: String)
    case unaryOperation(op: String)
    case compare(op: String)
    case jump(address: Int)
    case jumpIfTrue(address: Int)
    case jumpIfFalse(address: Int)
    case call(function: String, argCount: Int)
    case return
    case push
    case pop
    case duplicate
}

// Compiled bytecode representation
struct CompiledBytecode: Codable {
    let version: String
    let instructions: [BytecodeInstruction]
    let constants: [Any]
    let variables: [String]
    let functions: [String]
    let metadata: GraphMetadata
    
    var size: Int {
        return instructions.count * 8 // Approximate size in bytes
    }
}

// Optimization pass protocol
protocol OptimizationPass {
    var name: String { get }
    var description: String { get }
    
    func optimize(nodes: inout [GraphNode], connections: inout [GraphConnection]) -> OptimizationResult
}

// Optimization result
struct OptimizationResult {
    let passName: String
    let success: Bool
    let changes: Int
    let message: String
    let duration: TimeInterval
}

// Bytecode compiler
class BytecodeCompiler: ObservableObject {
    static let shared = BytecodeCompiler()
    
    @Published var compilationInProgress = false
    @Published var lastBytecode: CompiledBytecode?
    @Published var compilationErrors: [String] = []
    
    private var instructionPointer = 0
    private var constantPool: [Any] = []
    private var variableTable: Set<String> = []
    private var functionTable: Set<String> = []
    
    func compileToBytecode(nodes: [GraphNode], connections: [GraphConnection]) -> CompiledBytecode? {
        compilationInProgress = true
        compilationErrors.removeAll()
        
        defer {
            compilationInProgress = false
        }
        
        // Reset compiler state
        instructionPointer = 0
        constantPool.removeAll()
        variableTable.removeAll()
        functionTable.removeAll()
        
        var instructions: [BytecodeInstruction] = []
        
        do {
            // Find entry points (event nodes)
            let eventNodes = nodes.filter { $0.type == .event }
            
            for eventNode in eventNodes {
                try compileNode(eventNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
            
            // Create bytecode object
            let bytecode = CompiledBytecode(
                version: "1.0",
                instructions: instructions,
                constants: constantPool,
                variables: Array(variableTable),
                functions: Array(functionTable),
                metadata: GraphMetadata(
                    version: "1.0",
                    created: Date(),
                    modified: Date(),
                    author: "NodeGraphEditor",
                    description: "Compiled from visual node graph"
                )
            )
            
            lastBytecode = bytecode
            return bytecode
            
        } catch {
            compilationErrors.append("Compilation failed: \(error.localizedDescription)")
            return nil
        }
    }
    
    private func compileNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        switch node.type {
        case .event:
            // Event nodes are entry points
            instructions.append(.nop) // Placeholder for event setup
            
            // Compile connected nodes
            let connectedNodes = getConnectedNodes(from: node.id, connections: connections)
            for connectedNode in connectedNodes {
                try compileNode(connectedNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
            
        case .math:
            try compileMathNode(node, nodes: nodes, connections: connections, instructions: &instructions)
            
        case .logic:
            try compileLogicNode(node, nodes: nodes, connections: connections, instructions: &instructions)
            
        case .branch:
            try compileBranchNode(node, nodes: nodes, connections: connections, instructions: &instructions)
            
        case .forLoop, .forEach:
            try compileLoopNode(node, nodes: nodes, connections: connections, instructions: &instructions)
            
        case .getter:
            try compileGetterNode(node, nodes: nodes, connections: connections, instructions: &instructions)
            
        case .setter:
            try compileSetterNode(node, nodes: nodes, connections: connections, instructions: &instructions)
            
        case .function:
            try compileFunctionNode(node, nodes: nodes, connections: connections, instructions: &instructions)
            
        case .variable:
            try compileVariableNode(node, instructions: &instructions)
            
        case .constant:
            try compileConstantNode(node, instructions: &instructions)
            
        default:
            // Skip comment nodes
            break
        }
    }
    
    private func compileMathNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        // Get input values
        let inputConnections = connections.filter { $0.toNode == node.id }
        
        for connection in inputConnections {
            if let fromNode = nodes.first(where: { $0.id == connection.fromNode }) {
                try compileNode(fromNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Perform math operation
        let operation = determineMathOperation(node.title)
        instructions.append(.binaryOperation(op: operation))
    }
    
    private func compileLogicNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        // Get input values
        let inputConnections = connections.filter { $0.toNode == node.id }
        
        for connection in inputConnections {
            if let fromNode = nodes.first(where: { $0.id == connection.fromNode }) {
                try compileNode(fromNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Perform logic operation
        let operation = determineLogicOperation(node.title)
        instructions.append(.binaryOperation(op: operation))
    }
    
    private func compileBranchNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        // Compile condition
        let conditionConnections = connections.filter { $0.toNode == node.id && $0.toPort < node.inputs.count }
        
        for connection in conditionConnections {
            if let fromNode = nodes.first(where: { $0.id == connection.fromNode }) {
                try compileNode(fromNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Placeholder for branch target addresses
        let jumpAddress = instructions.count + 2 // Will be updated later
        instructions.append(.jumpIfFalse(address: jumpAddress))
        
        // Compile true branch
        let trueBranchConnections = connections.filter { $0.fromNode == node.id }
        for connection in trueBranchConnections {
            if let toNode = nodes.first(where: { $0.id == connection.toNode }) {
                try compileNode(toNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Update jump address
        if let jumpIndex = instructions.lastIndex(where: {
            if case .jumpIfFalse = $0 { return true }
            return false
        }) {
            instructions[jumpIndex] = .jumpIfFalse(address: instructions.count)
        }
    }
    
    private func compileLoopNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        let loopStart = instructions.count
        
        // Compile loop condition
        let conditionConnections = connections.filter { $0.toNode == node.id }
        for connection in conditionConnections {
            if let fromNode = nodes.first(where: { $0.id == connection.fromNode }) {
                try compileNode(fromNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Check loop condition
        instructions.append(.duplicate) // Duplicate condition for check
        let jumpAddress = instructions.count + 2 // Will be updated later
        instructions.append(.jumpIfFalse(address: jumpAddress))
        
        // Compile loop body
        let bodyConnections = connections.filter { $0.fromNode == node.id }
        for connection in bodyConnections {
            if let toNode = nodes.first(where: { $0.id == connection.toNode }) {
                try compileNode(toNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Jump back to loop start
        instructions.append(.jump(address: loopStart))
        
        // Update jump address
        if let jumpIndex = instructions.lastIndex(where: {
            if case .jumpIfFalse = $0 { return true }
            return false
        }) {
            instructions[jumpIndex] = .jumpIfFalse(address: instructions.count)
        }
    }
    
    private func compileGetterNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        // Get variable name from node title
        let variableName = extractVariableName(from: node.title)
        variableTable.insert(variableName)
        instructions.append(.loadVariable(name: variableName))
    }
    
    private func compileSetterNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        // Compile value to set
        let valueConnections = connections.filter { $0.toNode == node.id }
        for connection in valueConnections {
            if let fromNode = nodes.first(where: { $0.id == connection.fromNode }) {
                try compileNode(fromNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Store variable
        let variableName = extractVariableName(from: node.title)
        variableTable.insert(variableName)
        instructions.append(.storeVariable(name: variableName))
    }
    
    private func compileFunctionNode(_ node: GraphNode, nodes: [GraphNode], connections: [GraphConnection], instructions: inout [BytecodeInstruction]) throws {
        // Compile arguments
        let argConnections = connections.filter { $0.toNode == node.id }
        for connection in argConnections {
            if let fromNode = nodes.first(where: { $0.id == connection.fromNode }) {
                try compileNode(fromNode, nodes: nodes, connections: connections, instructions: &instructions)
            }
        }
        
        // Call function
        let functionName = extractFunctionName(from: node.title)
        functionTable.insert(functionName)
        instructions.append(.call(function: functionName, argCount: argConnections.count))
    }
    
    private func compileVariableNode(_ node: GraphNode, instructions: inout [BytecodeInstruction]) throws {
        let variableName = extractVariableName(from: node.title)
        variableTable.insert(variableName)
        instructions.append(.loadVariable(name: variableName))
    }
    
    private func compileConstantNode(_ node: GraphNode, instructions: inout [BytecodeInstruction]) throws {
        let value = extractConstantValue(from: node.title)
        constantPool.append(value)
        instructions.append(.loadConstant(value: value))
    }
    
    // Helper methods
    private func getConnectedNodes(from nodeId: UUID, connections: [GraphConnection]) -> [GraphNode] {
        var connectedNodes: [GraphNode] = []
        var visited: Set<UUID> = []
        var queue: [UUID] = [nodeId]
        
        while !queue.isEmpty {
            let currentId = queue.removeFirst()
            
            if !visited.contains(currentId) {
                visited.insert(currentId)
                
                let outgoingConnections = connections.filter { $0.fromNode == currentId }
                for connection in outgoingConnections {
                    queue.append(connection.toNode)
                }
            }
        }
        
        return connectedNodes
    }
    
    private func determineMathOperation(_ title: String) -> String {
        if title.contains("Add") { return "+" }
        if title.contains("Subtract") { return "-" }
        if title.contains("Multiply") { return "*" }
        if title.contains("Divide") { return "/" }
        if title.contains("Modulo") { return "%" }
        return "+" // Default
    }
    
    private func determineLogicOperation(_ title: String) -> String {
        if title.contains("AND") { return "&&" }
        if title.contains("OR") { return "||" }
        if title.contains("NOT") { return "!" }
        if title.contains("XOR") { return "^" }
        return "&&" // Default
    }
    
    private func extractVariableName(from title: String) -> String {
        // Extract variable name from title like "Get Position" -> "position"
        return title.lowercased().replacingOccurrences(of: "get ", with: "")
            .replacingOccurrences(of: "set ", with: "")
            .replacingOccurrences(of: " ", with: "_")
    }
    
    private func extractFunctionName(from title: String) -> String {
        return title.lowercased().replacingOccurrences(of: " ", with: "_")
    }
    
    private func extractConstantValue(from title: String) -> Any {
        // Try to parse numeric values
        if let intValue = Int(title) {
            return intValue
        }
        if let floatValue = Float(title) {
            return floatValue
        }
        if title.lowercased() == "true" {
            return true
        }
        if title.lowercased() == "false" {
            return false
        }
        return title // Default to string
    }
}

// MARK: - Optimization Passes

// Constant folding optimization
struct ConstantFoldingPass: OptimizationPass {
    let name = "Constant Folding"
    let description = "Evaluates constant expressions at compile time"
    
    func optimize(nodes: inout [GraphNode], connections: inout [GraphConnection]) -> OptimizationResult {
        let startTime = Date()
        var changes = 0
        
        // Find math nodes with constant inputs
        for i in nodes.indices {
            if nodes[i].type == .math {
                if let constantValue = evaluateConstantExpression(nodes[i], connections: connections) {
                    // Replace math node with constant node
                    nodes[i] = GraphNode(
                        id: nodes[i].id,
                        type: .constant,
                        title: "\(constantValue)",
                        position: nodes[i].position,
                        inputs: [],
                        outputs: nodes[i].outputs
                    )
                    changes += 1
                }
            }
        }
        
        let duration = Date().timeIntervalSince(startTime)
        return OptimizationResult(
            passName: name,
            success: true,
            changes: changes,
            message: "Folded \(changes) constant expressions",
            duration: duration
        )
    }
    
    private func evaluateConstantExpression(_ node: GraphNode, connections: [GraphConnection]) -> Any? {
        // Check if all inputs are constants
        let inputConnections = connections.filter { $0.toNode == node.id }
        var inputValues: [Any] = []
        
        for connection in inputConnections {
            if let fromNode = connections.compactMap({ conn in
                nodes.first(where: { $0.id == conn.fromNode })
            }).first(where: { $0.id == connection.fromNode }) {
                if fromNode.type == .constant {
                    inputValues.append(extractConstantValue(from: fromNode.title))
                } else {
                    return nil // Not all inputs are constants
                }
            }
        }
        
        // Evaluate the expression
        if inputValues.count == 2 {
            if let value1 = inputValues[0] as? Int, let value2 = inputValues[1] as? Int {
                switch node.title {
                case "Add": return value1 + value2
                case "Subtract": return value1 - value2
                case "Multiply": return value1 * value2
                case "Divide": return value2 != 0 ? value1 / value2 : 0
                default: break
                }
            }
        }
        
        return nil
    }
}

// Dead code elimination
struct DeadCodeEliminationPass: OptimizationPass {
    let name = "Dead Code Elimination"
    let description = "Removes unreachable nodes and connections"
    
    func optimize(nodes: inout [GraphNode], connections: inout [GraphConnection]) -> OptimizationResult {
        let startTime = Date()
        var changes = 0
        
        // Find reachable nodes from event nodes
        let eventNodes = nodes.filter { $0.type == .event }
        var reachableNodes: Set<UUID> = []
        
        for eventNode in eventNodes {
            findReachableNodes(from: eventNode.id, nodes: nodes, connections: connections, reachable: &reachableNodes)
        }
        
        // Remove unreachable nodes
        let originalCount = nodes.count
        nodes.removeAll { !reachableNodes.contains($0.id) && $0.type != .comment }
        changes += originalCount - nodes.count
        
        // Remove connections to/from removed nodes
        let originalConnectionCount = connections.count
        connections.removeAll { conn in
            !reachableNodes.contains(conn.fromNode) || !reachableNodes.contains(conn.toNode)
        }
        changes += originalConnectionCount - connections.count
        
        let duration = Date().timeIntervalSince(startTime)
        return OptimizationResult(
            passName: name,
            success: true,
            changes: changes,
            message: "Removed \(changes) unreachable elements",
            duration: duration
        )
    }
    
    private func findReachableNodes(from nodeId: UUID, nodes: [GraphNode], connections: [GraphConnection], reachable: inout Set<UUID>) {
        if reachable.contains(nodeId) { return }
        reachable.insert(nodeId)
        
        let outgoingConnections = connections.filter { $0.fromNode == nodeId }
        for connection in outgoingConnections {
            findReachableNodes(from: connection.toNode, nodes: nodes, connections: connections, reachable: &reachable)
        }
    }
}

// Optimization manager
class OptimizationManager: ObservableObject {
    static let shared = OptimizationManager()
    
    @Published var optimizationInProgress = false
    @Published var optimizationResults: [OptimizationResult] = []
    @Published var enabledPasses: Set<String> = ["Constant Folding", "Dead Code Elimination"]
    
    private let availablePasses: [OptimizationPass] = [
        ConstantFoldingPass(),
        DeadCodeEliminationPass()
    ]
    
    func optimizeGraph(nodes: inout [GraphNode], connections: inout [GraphConnection]) {
        optimizationInProgress = true
        optimizationResults.removeAll()
        
        defer {
            optimizationInProgress = false
        }
        
        for pass in availablePasses {
            if enabledPasses.contains(pass.name) {
                let result = pass.optimize(nodes: &nodes, connections: &connections)
                optimizationResults.append(result)
            }
        }
    }
}

// MARK: - C# Script Interop System

// C# script representation
struct CSharpScript: Codable {
    let id: UUID
    let name: String
    let content: String
    let namespace: String
    let className: String
    let methods: [CSharpMethod]
    let properties: [CSharpProperty]
    let dependencies: [String]
    let metadata: ScriptMetadata
}

struct CSharpMethod: Codable {
    let name: String
    let returnType: String
    let parameters: [CSharpParameter]
    let isStatic: Bool
    let isPublic: Bool
    let body: String
}

struct CSharpParameter: Codable {
    let name: String
    let type: String
    let isOptional: Bool
    let defaultValue: String?
}

struct CSharpProperty: Codable {
    let name: String
    let type: String
    let isStatic: Bool
    let isPublic: Bool
    let getter: String?
    let setter: String?
}

struct ScriptMetadata: Codable {
    let version: String
    let author: String
    let description: String
    let created: Date
    let modified: Date
    let tags: [String]
}

// C# interop manager
class CSharpInteropManager: ObservableObject {
    static let shared = CSharpInteropManager()
    
    @Published var loadedScripts: [CSharpScript] = []
    @Published var compilationInProgress = false
    @Published var interopErrors: [String] = []
    @Published var availableMethods: [CSharpMethod] = []
    
    private var scriptAssembly: Any? // Placeholder for compiled assembly
    private var scriptContext: Any? // Placeholder for execution context
    
    func loadScript(_ script: CSharpScript) {
        // Validate script syntax
        guard validateScript(script) else {
            interopErrors.append("Invalid script syntax in \(script.name)")
            return
        }
        
        // Compile script
        compileScript(script)
        
        // Add to loaded scripts
        if !loadedScripts.contains(where: { $0.id == script.id }) {
            loadedScripts.append(script)
        }
        
        // Update available methods
        updateAvailableMethods()
    }
    
    func callMethod(_ methodName: String, parameters: [Any]) -> Any? {
        guard let method = availableMethods.first(where: { $0.name == methodName }) else {
            interopErrors.append("Method \(methodName) not found")
            return nil
        }
        
        // Validate parameter count
        if parameters.count != method.parameters.count {
            interopErrors.append("Parameter count mismatch for \(methodName)")
            return nil
        }
        
        // Execute method (placeholder implementation)
        return executeMethod(method, parameters: parameters)
    }
    
    func createScriptNode(for method: CSharpMethod, at position: CGPoint) -> GraphNode {
        let inputPorts = method.parameters.enumerated().map { index, param in
            NodePort(
                name: param.name,
                type: mapCSharpTypeToPortType(param.type),
                defaultValue: param.defaultValue
            )
        }
        
        let outputPorts = [
            NodePort(name: "Return", type: mapCSharpTypeToPortType(method.returnType))
        ]
        
        return GraphNode(
            type: .function,
            title: "C# \(method.name)",
            position: position,
            inputs: inputPorts,
            outputs: outputPorts
        )
    }
    
    private func validateScript(_ script: CSharpScript) -> Bool {
        // Basic syntax validation
        let content = script.content
        
        // Check for balanced braces
        let braceCount = content.reduce(0) { count, char in
            if char == "{" { return count + 1 }
            if char == "}" { return count - 1 }
            return count
        }
        
        guard braceCount == 0 else {
            interopErrors.append("Unbalanced braces in script")
            return false
        }
        
        // Check for required class definition
        guard content.contains("class \(script.className)") else {
            interopErrors.append("Missing class definition")
            return false
        }
        
        return true
    }
    
    private func compileScript(_ script: CSharpScript) {
        compilationInProgress = true
        interopErrors.removeAll()
        
        DispatchQueue.global(qos: .userInitiated).async {
            // Placeholder for actual C# compilation
            // In a real implementation, this would use Roslyn or similar
            
            DispatchQueue.main.async {
                self.compilationInProgress = false
                // Simulate successful compilation
                self.scriptAssembly = "compiled_\(script.id)"
                self.scriptContext = "context_\(script.id)"
            }
        }
    }
    
    private func updateAvailableMethods() {
        availableMethods.removeAll()
        
        for script in loadedScripts {
            availableMethods.append(contentsOf: script.methods.filter { $0.isPublic })
        }
    }
    
    private func executeMethod(_ method: CSharpMethod, parameters: [Any]) -> Any? {
        // Placeholder implementation
        // In a real implementation, this would use reflection to call the method
        
        switch method.returnType {
        case "void":
            return nil
        case "int":
            return 0
        case "float":
            return 0.0
        case "bool":
            return false
        case "string":
            return ""
        default:
            return nil
        }
    }
    
    private func mapCSharpTypeToPortType(_ csharpType: String) -> PortType {
        switch csharpType.lowercased() {
        case "void":
            return .flow
        case "bool", "boolean":
            return .bool
        case "int", "int32", "int64":
            return .int
        case "float", "double", "decimal":
            return .float
        case "string":
            return .string
        case "vector3", "vector2", "vector4":
            return .vector3
        default:
            return .object
        }
    }
    
    // Template scripts
    func createTemplateScript(type: ScriptTemplate) -> CSharpScript {
        switch type {
        case .utility:
            return createUtilityScriptTemplate()
        case .math:
            return createMathScriptTemplate()
        case .string:
            return createStringScriptTemplate()
        case .custom:
            return createCustomScriptTemplate()
        }
    }
    
    private func createUtilityScriptTemplate() -> CSharpScript {
        let content = """
        using System;
        using System.Collections.Generic;
        
        namespace NodeGraph.Scripts {
            public class UtilityScript {
                public static bool IsNullOrEmpty(string value) {
                    return string.IsNullOrEmpty(value);
                }
                
                public static int Clamp(int value, int min, int max) {
                    return Math.Max(min, Math.Min(max, value));
                }
                
                public static float Lerp(float a, float b, float t) {
                    return a + (b - a) * t;
                }
            }
        }
        """
        
        return CSharpScript(
            id: UUID(),
            name: "UtilityScript",
            content: content,
            namespace: "NodeGraph.Scripts",
            className: "UtilityScript",
            methods: [
                CSharpMethod(
                    name: "IsNullOrEmpty",
                    returnType: "bool",
                    parameters: [CSharpParameter(name: "value", type: "string", isOptional: false, defaultValue: nil)],
                    isStatic: true,
                    isPublic: true,
                    body: "return string.IsNullOrEmpty(value);"
                ),
                CSharpMethod(
                    name: "Clamp",
                    returnType: "int",
                    parameters: [
                        CSharpParameter(name: "value", type: "int", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "min", type: "int", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "max", type: "int", isOptional: false, defaultValue: nil)
                    ],
                    isStatic: true,
                    isPublic: true,
                    body: "return Math.Max(min, Math.Min(max, value));"
                ),
                CSharpMethod(
                    name: "Lerp",
                    returnType: "float",
                    parameters: [
                        CSharpParameter(name: "a", type: "float", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "b", type: "float", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "t", type: "float", isOptional: false, defaultValue: nil)
                    ],
                    isStatic: true,
                    isPublic: true,
                    body: "return a + (b - a) * t;"
                )
            ],
            properties: [],
            dependencies: ["System", "System.Collections.Generic"],
            metadata: ScriptMetadata(
                version: "1.0",
                author: "NodeGraphEditor",
                description: "Utility functions for common operations",
                created: Date(),
                modified: Date(),
                tags: ["utility", "math", "string"]
            )
        )
    }
    
    private func createMathScriptTemplate() -> CSharpScript {
        let content = """
        using System;
        
        namespace NodeGraph.Scripts {
            public class MathScript {
                public static float Distance(float x1, float y1, float x2, float y2) {
                    float dx = x2 - x1;
                    float dy = y2 - y1;
                    return (float)Math.Sqrt(dx * dx + dy * dy);
                }
                
                public static float NormalizeAngle(float angle) {
                    angle = angle % 360f;
                    if (angle < 0) angle += 360f;
                    return angle;
                }
                
                public static bool Approximately(float a, float b, float epsilon = 0.0001f) {
                    return Math.Abs(a - b) < epsilon;
                }
            }
        }
        """
        
        return CSharpScript(
            id: UUID(),
            name: "MathScript",
            content: content,
            namespace: "NodeGraph.Scripts",
            className: "MathScript",
            methods: [
                CSharpMethod(
                    name: "Distance",
                    returnType: "float",
                    parameters: [
                        CSharpParameter(name: "x1", type: "float", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "y1", type: "float", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "x2", type: "float", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "y2", type: "float", isOptional: false, defaultValue: nil)
                    ],
                    isStatic: true,
                    isPublic: true,
                    body: "return (float)Math.Sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));"
                )
            ],
            properties: [],
            dependencies: ["System"],
            metadata: ScriptMetadata(
                version: "1.0",
                author: "NodeGraphEditor",
                description: "Advanced mathematical functions",
                created: Date(),
                modified: Date(),
                tags: ["math", "geometry", "utility"]
            )
        )
    }
    
    private func createStringScriptTemplate() -> CSharpScript {
        let content = """
        using System;
        using System.Text.RegularExpressions;
        
        namespace NodeGraph.Scripts {
            public class StringScript {
                public static string Replace(string input, string oldValue, string newValue) {
                    return input.Replace(oldValue, newValue);
                }
                
                public static bool MatchesPattern(string input, string pattern) {
                    return Regex.IsMatch(input, pattern);
                }
                
                public static string Format(string format, params object[] args) {
                    return string.Format(format, args);
                }
            }
        }
        """
        
        return CSharpScript(
            id: UUID(),
            name: "StringScript",
            content: content,
            namespace: "NodeGraph.Scripts",
            className: "StringScript",
            methods: [
                CSharpMethod(
                    name: "Replace",
                    returnType: "string",
                    parameters: [
                        CSharpParameter(name: "input", type: "string", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "oldValue", type: "string", isOptional: false, defaultValue: nil),
                        CSharpParameter(name: "newValue", type: "string", isOptional: false, defaultValue: nil)
                    ],
                    isStatic: true,
                    isPublic: true,
                    body: "return input.Replace(oldValue, newValue);"
                )
            ],
            properties: [],
            dependencies: ["System", "System.Text.RegularExpressions"],
            metadata: ScriptMetadata(
                version: "1.0",
                author: "NodeGraphEditor",
                description: "String manipulation and formatting functions",
                created: Date(),
                modified: Date(),
                tags: ["string", "text", "formatting"]
            )
        )
    }
    
    private func createCustomScriptTemplate() -> CSharpScript {
        let content = """
        using System;
        
        namespace NodeGraph.Scripts {
            public class CustomScript {
                // Add your custom methods here
                public static void YourMethod() {
                    // Your implementation
                }
            }
        }
        """
        
        return CSharpScript(
            id: UUID(),
            name: "CustomScript",
            content: content,
            namespace: "NodeGraph.Scripts",
            className: "CustomScript",
            methods: [],
            properties: [],
            dependencies: ["System"],
            metadata: ScriptMetadata(
                version: "1.0",
                author: "User",
                description: "Custom script template",
                created: Date(),
                modified: Date(),
                tags: ["custom", "template"]
            )
        )
    }
}

enum ScriptTemplate {
    case utility
    case math
    case string
    case custom
}

// MARK: - C# Script Editor View
struct CSharpScriptEditor: View {
    @State private var selectedScript: CSharpScript?
    @State private var scriptContent: String = ""
    @State private var showingTemplatePicker = false
    
    private var interopManager = CSharpInteropManager.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("C# Scripts")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                
                Spacer()
                
                Button("New Script") {
                    showingTemplatePicker = true
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.small)
            }
            .padding(12)
            .background(Color(red: 0.15, green: 0.15, blue: 0.18))
            
            // Script list
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 4) {
                    ForEach(interopManager.loadedScripts, id: \.id) { script in
                        CSharpScriptRow(
                            script: script,
                            isSelected: selectedScript?.id == script.id
                        )
                        .onTapGesture {
                            selectedScript = script
                            scriptContent = script.content
                        }
                    }
                }
            }
            .frame(height: 200)
            
            // Script editor
            if let selectedScript = selectedScript {
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        Text(selectedScript.name)
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(.white)
                        
                        Spacer()
                        
                        Button("Compile") {
                            interopManager.loadScript(selectedScript)
                        }
                        .buttonStyle(.bordered)
                        .controlSize(.small)
                    }
                    
                    TextEditor(text: $scriptContent)
                        .font(.system(.body, design: .monospaced))
                        .foregroundColor(.white)
                        .background(Color(red: 0.08, green: 0.08, blue: 0.1))
                        .cornerRadius(4)
                }
                .padding(12)
            }
        }
        .frame(width: 400)
        .background(Color(red: 0.1, green: 0.1, blue: 0.12))
        .sheet(isPresented: $showingTemplatePicker) {
            CSharpTemplatePicker { template in
                let newScript = interopManager.createTemplateScript(type: template)
                        selectedScript = newScript
                        scriptContent = newScript.content
                        showingTemplatePicker = false
                    }
                }
            }
        }
    }
}

struct CSharpScriptRow: View {
    let script: CSharpScript
    let isSelected: Bool
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(script.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(.white)
                
                Text("\(script.methods.count) methods")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white.opacity(0.6))
            }
            
            Spacer()
            
            Text(script.className)
                .font(DesignSystem.Typography.small)
                .foregroundColor(.white.opacity(0.4))
        }
        .padding(8)
        .background(isSelected ? Color.white.opacity(0.1) : Color.clear)
        .contentShape(Rectangle())
    }
}

struct CSharpTemplatePicker: View {
    let onTemplateSelected: (ScriptTemplate) -> Void
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Choose Script Template")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            VStack(spacing: 8) {
                Button("Utility Script") {
                    onTemplateSelected(.utility)
                }
                .buttonStyle(.borderedProminent)
                
                Button("Math Script") {
                    onTemplateSelected(.math)
                }
                .buttonStyle(.borderedProminent)
                
                Button("String Script") {
                    onTemplateSelected(.string)
                }
                .buttonStyle(.borderedProminent)
                
                Button("Custom Script") {
                    onTemplateSelected(.custom)
                }
                .buttonStyle(.borderedProminent)
                
                Button("Cancel") {
                    // Dismiss
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(24)
        .frame(width: 300)
        .background(Color(red: 0.15, green: 0.15, blue: 0.18))
    }
}

// MARK: - Checkbox Toggle Style
struct CheckboxToggleStyle: ToggleStyle {
    func makeBody(configuration: Configuration) -> some View {
        HStack(spacing: 6) {
            Image(systemName: configuration.isOn ? "checkmark.square.fill" : "square")
                .foregroundColor(configuration.isOn ? DesignSystem.Colors.accentPrimary : .white.opacity(0.4))
                .font(.system(size: 12))
            
            configuration.label
                .font(DesignSystem.Typography.small)
                .foregroundColor(.white.opacity(0.8))
        }
        .onTapGesture {
            configuration.isOn.toggle()
        }
    }
}

// MARK: - Debugging System

class DebugManager: ObservableObject {
    static let shared = DebugManager()
    
    @Published var isDebugging: Bool = false
    @Published var isPaused: Bool = false
    @Published var isRunning: Bool = false
    @Published var currentExecutionNode: UUID?
    @Published var callStack: [DebugStackFrame] = []
    @Published var localVariables: [String: DebugVariable] = [:]
    @Published var instanceVariables: [String: DebugVariable] = [:]
    @Published var breakpoints: Set<UUID> = []
    @Published var conditionalBreakpoints: [UUID: String] = [:]
    @Published var hitCountBreakpoints: [UUID: (current: Int, target: Int)] = [:]
    @Published var logPoints: [UUID: String] = []
    @Published var executionPath: [UUID] = []
    @Published var nodePerformance: [UUID: NodePerformanceData] = [:]
    
    private var executionTimer: Timer?
    
    // Execution Controls
    func startDebugging() {
        isDebugging = true
        isRunning = true
        isPaused = false
        callStack.removeAll()
        localVariables.removeAll()
        instanceVariables.removeAll()
        executionPath.removeAll()
        nodePerformance.removeAll()
    }
    
    func stopDebugging() {
        isDebugging = false
        isRunning = false
        isPaused = false
        currentExecutionNode = nil
        executionTimer?.invalidate()
        executionTimer = nil
    }
    
    func pauseExecution() {
        isPaused = true
        isRunning = false
        executionTimer?.invalidate()
    }
    
    func continueExecution() {
        isPaused = false
        isRunning = true
        executeNextNode()
    }
    
    func stepThrough() {
        if isDebugging && !isRunning {
            executeNextNode()
        }
    }
    
    func stepIntoFunction() {
        // Implementation for stepping into function calls
        if let currentNode = currentExecutionNode {
            // Find function calls from current node and step into them
            executeNextNode(enterFunction: true)
        }
    }
    
    func stepOverNode() {
        // Execute current node and stop at the next node in the same scope
        if let currentNode = currentExecutionNode {
            executeNextNode(skipFunction: true)
        }
    }
    
    func stepOutOfFunction() {
        // Step out to the caller's scope
        if callStack.count > 1 {
            callStack.removeLast()
            if let previousFrame = callStack.last {
                currentExecutionNode = previousFrame.nodeId
            }
        }
    }
    
    // Breakpoint Management
    func toggleBreakpoint(on nodeId: UUID) {
        if breakpoints.contains(nodeId) {
            breakpoints.remove(nodeId)
            conditionalBreakpoints.removeValue(forKey: nodeId)
            hitCountBreakpoints.removeValue(forKey: nodeId)
        } else {
            breakpoints.insert(nodeId)
        }
    }
    
    func setConditionalBreakpoint(on nodeId: UUID, condition: String) {
        breakpoints.insert(nodeId)
        conditionalBreakpoints[nodeId] = condition
    }
    
    func setHitCountBreakpoint(on nodeId: UUID, targetCount: Int) {
        breakpoints.insert(nodeId)
        hitCountBreakpoints[nodeId] = (0, targetCount)
    }
    
    func addLogPoint(on nodeId: UUID, message: String) {
        logPoints[nodeId] = message
    }
    
    // Variable Management
    func updateLocalVariables(_ variables: [String: Any]) {
        localVariables.removeAll()
        for (name, value) in variables {
            localVariables[name] = DebugVariable(name: name, value: value, type: String(describing: type(of: value)))
        }
    }
    
    func updateInstanceVariables(_ variables: [String: Any]) {
        instanceVariables.removeAll()
        for (name, value) in variables {
            instanceVariables[name] = DebugVariable(name: name, value: value, type: String(describing: type(of: value)))
        }
    }
    
    func evaluateExpression(_ expression: String) -> DebugVariable? {
        // Simple expression evaluation (would need proper parser in production)
        if let variable = localVariables[expression] {
            return variable
        }
        if let variable = instanceVariables[expression] {
            return variable
        }
        return nil
    }
    
    // Private execution methods
    private func executeNextNode(enterFunction: Bool = false, skipFunction: Bool = false) {
        // This would integrate with the actual node execution system
        // For now, simulate execution
        
        if let currentNode = currentExecutionNode {
            // Record performance
            let startTime = Date()
            
            // Check for log points
            if let logMessage = logPoints[currentNode] {
                print("[LOG] \(logMessage)")
            }
            
            // Update performance data
            let executionTime = Date().timeIntervalSince(startTime)
            nodePerformance[currentNode] = NodePerformanceData(
                nodeId: currentNode,
                executionTime: executionTime,
                memoryUsage: 0, // Would track actual memory
                callCount: (nodePerformance[currentNode]?.callCount ?? 0) + 1
            )
        }
        
        // Simulate moving to next node
        // In real implementation, this would traverse the graph
        isRunning = false
        isPaused = true
    }
    
    private func checkBreakpoints(for nodeId: UUID) -> Bool {
        // Check regular breakpoint
        if breakpoints.contains(nodeId) {
            // Check conditional breakpoint
            if let condition = conditionalBreakpoints[nodeId] {
                // Evaluate condition (simplified)
                return evaluateCondition(condition)
            }
            
            // Check hit count breakpoint
            if let (current, target) = hitCountBreakpoints[nodeId] {
                hitCountBreakpoints[nodeId] = (current + 1, target)
                return (current + 1) >= target
            }
            
            return true
        }
        return false
    }
    
    private func evaluateCondition(_ condition: String) -> Bool {
        // Simplified condition evaluation
        // In production, would need proper expression parser
        return true
    }
}

// MARK: - Debug Data Structures

struct DebugStackFrame: Identifiable {
    let id = UUID()
    let nodeId: UUID
    let nodeName: String
    let functionName: String
    let lineNumber: Int
}

struct DebugVariable: Identifiable {
    let id = UUID()
    let name: String
    let value: Any
    let type: String
    
    var displayValue: String {
        if let stringValue = value as? String {
            return "\"\(stringValue)\""
        } else if let arrayValue = value as? [Any] {
            return "[\(arrayValue.count) items]"
        } else {
            return "\(value)"
        }
    }
}

struct NodePerformanceData {
    let nodeId: UUID
    let executionTime: TimeInterval
    let memoryUsage: Int64
    let callCount: Int
}

// MARK: - Visual Scripting Node Graph

class NodeGraphManager: ObservableObject {
    static let shared = NodeGraphManager()
    
    @Published var nodes: [GraphNode] = []
    @Published var connections: [GraphConnection] = []
    @Published var selectedNodes: Set<UUID> = []
    @Published var panOffset: CGPoint = .zero
    @Published var zoom: CGFloat = 1.0
    @Published var isConnecting: Bool = false
    @Published var connectionStart: (nodeID: UUID, portIndex: Int, isOutput: Bool)?
    @Published var showCompilationPanel: Bool = false
    @Published var showDebugPanels: Bool = false
    @Published var currentGraphName: String = "Main Graph"
    @Published var isDirty: Bool = false
    @Published var hotReloadEnabled: Bool = true
    @Published var parentGraph: UUID?
    
    // Function graph management
    @Published var functions: [GraphFunction] = []
    @Published var currentFunction: GraphFunction?
    @Published var isEditingFunction: Bool = false
    
    // Macro management
    @Published var macros: [GraphMacro] = []
    
    private let compilationManager = CompilationManager.shared
    private let bytecodeCompiler = BytecodeCompiler.shared
    private let optimizationManager = OptimizationManager.shared
    private var compilationTimer: Timer?
    private var lastSavedTime: Date?
    
    init() {
        loadDemoGraph()
        setupAutoCompilation()
        initializeTemplates()
    }
    
    private func setupAutoCompilation() {
        // Auto-compile after changes
        compilationTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
            self.autoCompileIfNeeded()
        }
    }
    
    private func autoCompileIfNeeded() {
        // Only auto-compile if there are no critical errors and the panel is visible
        if showCompilationPanel && compilationManager.errors.isEmpty {
            compileGraph()
        }
    }
    
    func compileGraph() {
        compilationManager.compileGraph(nodes: nodes, connections: connections)
        
        // Show panel if there are issues
        if !compilationManager.errors.isEmpty || !compilationManager.warnings.isEmpty {
            showCompilationPanel = true
        }
    }
    
    func compileToBytecode() {
        guard compilationManager.errors.isEmpty else {
            showCompilationPanel = true
            return
        }
        
        _ = bytecodeCompiler.compileToBytecode(nodes: nodes, connections: connections)
    }
    
    func optimizeGraph() {
        optimizationManager.optimizeGraph(nodes: &nodes, connections: &connections)
        
        // Re-compile after optimization
        compileGraph()
    }
    
    func navigateToError(_ error: CompilationError) {
        // Select the node with the error
        selectedNodes = [error.nodeId]
        
        // Center the view on the node
        if let node = nodes.first(where: { $0.id == error.nodeId }) {
            panOffset = CGPoint(
                x: -node.position.x * zoom + 200,
                y: -node.position.y * zoom + 200
            )
        }
    }
    
    func loadDemoGraph() {
        // Create demo nodes
        let startNode = GraphNode(
            type: .event,
            title: "On Start",
            position: CGPoint(x: 100, y: 200),
            outputs: [NodePort(name: "Exec", type: .flow)]
        )
        
        let getPositionNode = GraphNode(
            type: .getter,
            title: "Get Position",
            position: CGPoint(x: 350, y: 150),
            inputs: [NodePort(name: "Exec", type: .flow)],
            outputs: [
                NodePort(name: "Exec", type: .flow),
                NodePort(name: "Position", type: .vector3)
            ]
        )
        
        let addNode = GraphNode(
            type: .math,
            title: "Add",
            position: CGPoint(x: 600, y: 200),
            inputs: [
                NodePort(name: "A", type: .vector3),
                NodePort(name: "B", type: .vector3)
            ],
            outputs: [NodePort(name: "Result", type: .vector3)]
        )
        
        let setPositionNode = GraphNode(
            type: .setter,
            title: "Set Position",
            position: CGPoint(x: 850, y: 200),
            inputs: [
                NodePort(name: "Exec", type: .flow),
                NodePort(name: "Position", type: .vector3)
            ],
            outputs: [NodePort(name: "Exec", type: .flow)]
        )
        
        nodes = [startNode, getPositionNode, addNode, setPositionNode]
        
        // Create connections
        connections = [
            GraphConnection(fromNode: startNode.id, fromPort: 0, toNode: getPositionNode.id, toPort: 0),
            GraphConnection(fromNode: getPositionNode.id, fromPort: 0, toNode: setPositionNode.id, toPort: 0),
            GraphConnection(fromNode: getPositionNode.id, fromPort: 1, toNode: addNode.id, toPort: 0),
            GraphConnection(fromNode: addNode.id, fromPort: 0, toNode: setPositionNode.id, toPort: 1)
        ]
    }
    
    func addNode(type: NodeType, at position: CGPoint) {
        let template = nodeTemplates[type] ?? GraphNode(type: type, title: type.rawValue.capitalized, position: position)
        var newNode = template
        newNode.id = UUID()
        newNode.position = position
        nodes.append(newNode)
    }
    
    func addNode(_ node: GraphNode) {
        nodes.append(node)
    }
    
    func deleteNode(_ nodeId: UUID) {
        nodes.removeAll { $0.id == nodeId }
        connections.removeAll { $0.fromNode == nodeId || $0.toNode == nodeId }
        selectedNodes.remove(nodeId)
    }
    
    func pasteNodes(at position: CGPoint) {
        let clipboardManager = NodeClipboardManager.shared
        guard clipboardManager.canPaste() else { return }
        
        let (pastedNodes, pastedConnections) = clipboardManager.paste(at: position)
        
        // Add new nodes
        for node in pastedNodes {
            nodes.append(node)
        }
        
        // Add new connections
        for connection in pastedConnections {
            connections.append(connection)
        }
        
        // Select the newly pasted nodes
        selectedNodes = Set(pastedNodes.map { $0.id })
    }
    
    func deleteSelectedNodes() {
        nodes.removeAll { selectedNodes.contains($0.id) }
        connections.removeAll { conn in
            selectedNodes.contains(conn.fromNode) || selectedNodes.contains(conn.toNode)
        }
        selectedNodes.removeAll()
    }
    
    func connect(from: (UUID, Int), to: (UUID, Int)) {
        let connection = GraphConnection(fromNode: from.0, fromPort: from.1, toNode: to.0, toPort: to.1)
        connections.append(connection)
    }
    
    private var nodeTemplates: [NodeType: GraphNode] = [
        // Array manipulation nodes
        .arrayCreate: GraphNode(
            type: .arrayCreate,
            title: "Create Array",
            position: .zero,
            inputs: [NodePort(name: "Size", type: .int)],
            outputs: [NodePort(name: "Array", type: .array)]
        ),
        .arrayGet: GraphNode(
            type: .arrayGet,
            title: "Get Array Element",
            position: .zero,
            inputs: [
                NodePort(name: "Array", type: .array),
                NodePort(name: "Index", type: .int)
            ],
            outputs: [NodePort(name: "Value", type: .any)]
        ),
        .arraySet: GraphNode(
            type: .arraySet,
            title: "Set Array Element",
            position: .zero,
            inputs: [
                NodePort(name: "Array", type: .array),
                NodePort(name: "Index", type: .int),
                NodePort(name: "Value", type: .any)
            ],
            outputs: [NodePort(name: "Array", type: .array)]
        ),
        .arrayLength: GraphNode(
            type: .arrayLength,
            title: "Array Length",
            position: .zero,
            inputs: [NodePort(name: "Array", type: .array)],
            outputs: [NodePort(name: "Length", type: .int)]
        ),
        .arrayAdd: GraphNode(
            type: .arrayAdd,
            title: "Add to Array",
            position: .zero,
            inputs: [
                NodePort(name: "Array", type: .array),
                NodePort(name: "Value", type: .any)
            ],
            outputs: [NodePort(name: "Array", type: .array)]
        ),
        .arrayRemove: GraphNode(
            type: .arrayRemove,
            title: "Remove from Array",
            position: .zero,
            inputs: [
                NodePort(name: "Array", type: .array),
                NodePort(name: "Index", type: .int)
            ],
            outputs: [NodePort(name: "Array", type: .array)]
        ),
        
        // Dictionary manipulation nodes
        .dictCreate: GraphNode(
            type: .dictCreate,
            title: "Create Dictionary",
            position: .zero,
            outputs: [NodePort(name: "Dictionary", type: .dictionary)]
        ),
        .dictGet: GraphNode(
            type: .dictGet,
            title: "Get Dictionary Value",
            position: .zero,
            inputs: [
                NodePort(name: "Dictionary", type: .dictionary),
                NodePort(name: "Key", type: .string)
            ],
            outputs: [NodePort(name: "Value", type: .any)]
        ),
        .dictSet: GraphNode(
            type: .dictSet,
            title: "Set Dictionary Value",
            position: .zero,
            inputs: [
                NodePort(name: "Dictionary", type: .dictionary),
                NodePort(name: "Key", type: .string),
                NodePort(name: "Value", type: .any)
            ],
            outputs: [NodePort(name: "Dictionary", type: .dictionary)]
        ),
        
        // Transform manipulation nodes
        .transformGetPosition: GraphNode(
            type: .transformGetPosition,
            title: "Get Position",
            position: .zero,
            inputs: [NodePort(name: "Target", type: .object)],
            outputs: [NodePort(name: "Position", type: .vector3)]
        ),
        .transformSetPosition: GraphNode(
            type: .transformSetPosition,
            title: "Set Position",
            position: .zero,
            inputs: [
                NodePort(name: "Target", type: .object),
                NodePort(name: "Position", type: .vector3)
            ],
            outputs: []
        ),
        .transformTranslate: GraphNode(
            type: .transformTranslate,
            title: "Translate",
            position: .zero,
            inputs: [
                NodePort(name: "Target", type: .object),
                NodePort(name: "Offset", type: .vector3)
            ],
            outputs: []
        ),
        
        // Physics nodes
        .physicsRaycast: GraphNode(
            type: .physicsRaycast,
            title: "Raycast",
            position: .zero,
            inputs: [
                NodePort(name: "Origin", type: .vector3),
                NodePort(name: "Direction", type: .vector3),
                NodePort(name: "Distance", type: .float)
            ],
            outputs: [NodePort(name: "Hit Result", type: .raycast)]
        ),
        .physicsAddForce: GraphNode(
            type: .physicsAddForce,
            title: "Add Force",
            position: .zero,
            inputs: [
                NodePort(name: "Target", type: .object),
                NodePort(name: "Force", type: .vector3),
                NodePort(name: "Mode", type: .int)
            ],
            outputs: []
        ),
        
        // Audio nodes
        .audioPlay: GraphNode(
            type: .audioPlay,
            title: "Play Audio",
            position: .zero,
            inputs: [
                NodePort(name: "Audio Clip", type: .audioClip),
                NodePort(name: "Volume", type: .float, defaultValue: 1.0)
            ],
            outputs: []
        ),
        .audioStop: GraphNode(
            type: .audioStop,
            title: "Stop Audio",
            position: .zero,
            inputs: [NodePort(name: "Audio Clip", type: .audioClip)],
            outputs: []
        ),
        .audioSetVolume: GraphNode(
            type: .audioSetVolume,
            title: "Set Volume",
            position: .zero,
            inputs: [
                NodePort(name: "Audio Clip", type: .audioClip),
                NodePort(name: "Volume", type: .float)
            ],
            outputs: []
        ),
        
        // Animation nodes
        .animPlay: GraphNode(
            type: .animPlay,
            title: "Play Animation",
            position: .zero,
            inputs: [
                NodePort(name: "Animation", type: .animationClip),
                NodePort(name: "Speed", type: .float, defaultValue: 1.0)
            ],
            outputs: []
        ),
        .animStop: GraphNode(
            type: .animStop,
            title: "Stop Animation",
            position: .zero,
            inputs: [NodePort(name: "Animation", type: .animationClip)],
            outputs: []
        ),
        .animBlend: GraphNode(
            type: .animBlend,
            title: "Blend Animation",
            position: .zero,
            inputs: [
                NodePort(name: "From", type: .animationClip),
                NodePort(name: "To", type: .animationClip),
                NodePort(name: "Weight", type: .float)
            ],
            outputs: []
        ),
        
        // UI nodes
        .uiShow: GraphNode(
            type: .uiShow,
            title: "Show UI",
            position: .zero,
            inputs: [NodePort(name: "UI Element", type: .uiElement)],
            outputs: []
        ),
        .uiHide: GraphNode(
            type: .uiHide,
            title: "Hide UI",
            position: .zero,
            inputs: [NodePort(name: "UI Element", type: .uiElement)],
            outputs: []
        ),
        .uiSetText: GraphNode(
            type: .uiSetText,
            title: "Set Text",
            position: .zero,
            inputs: [
                NodePort(name: "UI Element", type: .uiElement),
                NodePort(name: "Text", type: .string)
            ],
            outputs: []
        ),
        
        // File I/O nodes
        .fileRead: GraphNode(
            type: .fileRead,
            title: "Read File",
            position: .zero,
            inputs: [NodePort(name: "File Path", type: .string)],
            outputs: [NodePort(name: "Content", type: .string)]
        ),
        .fileWrite: GraphNode(
            type: .fileWrite,
            title: "Write File",
            position: .zero,
            inputs: [
                NodePort(name: "File Path", type: .string),
                NodePort(name: "Content", type: .string)
            ],
            outputs: []
        ),
        .fileExists: GraphNode(
            type: .fileExists,
            title: "File Exists",
            position: .zero,
            inputs: [NodePort(name: "File Path", type: .string)],
            outputs: [NodePort(name: "Exists", type: .bool)]
        ),
        
        // Reroute nodes
        .reroute: GraphNode(
            type: .reroute,
            title: "Reroute",
            position: .zero,
            inputs: [NodePort(name: "In", type: .any)],
            outputs: [NodePort(name: "Out", type: .any)]
        ),
        
        // Enhanced comment/sticky note node
        .comment: GraphNode(
            type: .comment,
            title: "Comment",
            position: .zero,
            inputs: [],
            outputs: []
        )
    ]
    
    // MARK: - Function Graph Management
    
    func createFunction(name: String, description: String = "") {
        let newFunction = GraphFunction(name: name, description: description)
        functions.append(newFunction)
        currentFunction = newFunction
        isEditingFunction = true
        markDirty()
    }
    
    func deleteFunction(_ function: GraphFunction) {
        functions.removeAll { $0.id == function.id }
        if currentFunction?.id == function.id {
            currentFunction = nil
            isEditingFunction = false
        }
        markDirty()
    }
    
    func editFunction(_ function: GraphFunction) {
        currentFunction = function
        isEditingFunction = true
        
        // Load function nodes and connections into current graph
        nodes = function.nodes
        connections = function.connections
    }
    
    func saveCurrentFunction() {
        guard let function = currentFunction else { return }
        
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].nodes = nodes
            functions[index].connections = connections
            functions[index].modified = Date()
        }
        
        isEditingFunction = false
        markDirty()
    }
    
    func cancelFunctionEdit() {
        isEditingFunction = false
        currentFunction = nil
        // Reload main graph
        loadDemoGraph()
    }
    
    func addParameter(to function: GraphFunction, name: String, type: PortType, defaultValue: Any? = nil, isOptional: Bool = false) {
        let parameter = FunctionParameter(name: name, type: type, defaultValue: defaultValue, isOptional: isOptional)
        
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].parameters.append(parameter)
            
            // Add input node to function graph
            let inputNode = GraphNode(
                type: .graphInput,
                title: "Input: \(name)",
                position: CGPoint(x: 50, y: 100 + CGFloat(function.parameters.count) * 50),
                inputs: [],
                outputs: [NodePort(name: name, type: type)]
            )
            functions[index].nodes.append(inputNode)
        }
        
        markDirty()
    }
    
    func removeParameter(from function: GraphFunction, parameter: FunctionParameter) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].parameters.removeAll { $0.id == parameter.id }
            
            // Remove corresponding input node
            functions[index].nodes.removeAll { node in
                node.type == .graphInput && node.title.contains("Input: \(parameter.name)")
            }
        }
        
        markDirty()
    }
    
    func setReturnValue(for function: GraphFunction, type: PortType, description: String = "") {
        let returnValue = FunctionReturnValue(type: type, description: description)
        
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].returnValue = returnValue
            
            // Add output node to function graph
            let outputNode = GraphNode(
                type: .graphOutput,
                title: "Return",
                position: CGPoint(x: 500, y: 200),
                inputs: [NodePort(name: "Value", type: type)],
                outputs: []
            )
            functions[index].nodes.append(outputNode)
        }
        
        markDirty()
    }
    
    func addLocalVariable(to function: GraphFunction, name: String, type: PortType, defaultValue: Any? = nil, scope: VariableScope = .local) {
        let variable = FunctionVariable(name: name, type: type, defaultValue: defaultValue, scope: scope)
        
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].localVariables.append(variable)
        }
        
        markDirty()
    }
    
    func removeLocalVariable(from function: GraphFunction, variable: FunctionVariable) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].localVariables.removeAll { $0.id == variable.id }
        }
        
        markDirty()
    }
    
    func generateFunctionCallNode(for function: GraphFunction, at position: CGPoint) {
        let callNode = GraphNode(
            type: .function,
            title: function.name,
            position: position,
            inputs: function.parameters.map { NodePort(name: $0.name, type: $0.type) },
            outputs: function.returnValue != nil ? [NodePort(name: "Return", type: function.returnValue!.type)] : []
        )
        
        nodes.append(callNode)
        markDirty()
    }
    
    // MARK: - Recursion Management
    
    func enableRecursion(for function: GraphFunction, maxDepth: Int = 100, warningDepth: Int = 50) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].allowsRecursion = true
            functions[index].maxRecursionDepth = maxDepth
            functions[index].recursionWarningDepth = warningDepth
            markDirty()
        }
    }
    
    func disableRecursion(for function: GraphFunction) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].allowsRecursion = false
            markDirty()
        }
    }
    
    func setRecursionLimits(for function: GraphFunction, maxDepth: Int, warningDepth: Int) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].maxRecursionDepth = maxDepth
            functions[index].recursionWarningDepth = warningDepth
            markDirty()
        }
    }
    
    func validateRecursiveCalls() -> [RecursionWarning] {
        var warnings: [RecursionWarning] = []
        
        for function in functions {
            if function.hasRecursiveCalls() && !function.allowsRecursion {
                warnings.append(RecursionWarning(
                    functionName: function.name,
                    type: .unallowedRecursion,
                    message: "Function '\(function.name)' contains recursive calls but recursion is not enabled"
                ))
            }
            
            // Check for potential infinite recursion
            if function.hasRecursiveCalls() && function.allowsRecursion {
                // Simple heuristic: if there's no base case (no condition that could stop recursion)
                let hasBaseCase = function.nodes.contains { node in
                    // Look for branch nodes that could provide exit conditions
                    node.type == .branch || node.type == .logic
                }
                
                if !hasBaseCase {
                    warnings.append(RecursionWarning(
                        functionName: function.name,
                        type: .potentialInfiniteRecursion,
                        message: "Function '\(function.name)' may have infinite recursion - no base case detected"
                    ))
                }
            }
        }
        
        return warnings
    }
    
    func getRecursionDepth(for function: GraphFunction, callStack: [String]) -> Int {
        return function.getRecursionDepth(callStack: callStack)
    }
    
    func canExecuteFunction(_ function: GraphFunction, callStack: [String]) -> (canExecute: Bool, reason: String?) {
        if !function.allowsRecursion && function.hasRecursiveCalls() && callStack.contains(function.name) {
            return (false, "Recursion not allowed for function '\(function.name)'")
        }
        
        if function.wouldExceedRecursionLimit(callStack: callStack) {
            return (false, "Recursion depth limit (\(function.maxRecursionDepth)) exceeded for function '\(function.name)'")
        }
        
        return (true, nil)
    }
    
    // MARK: - Function Overloading Support
    
    func createOverload(baseFunction: GraphFunction, parameterSignature: [FunctionParameter], description: String = "") -> GraphFunction {
        let overloadName = "\(baseFunction.name)_\(baseFunction.parameters.count + 1)"
        let overload = GraphFunction(name: overloadName, description: description)
        
        // Copy properties from base function
        overload.parameters = parameterSignature
        overload.returnValue = baseFunction.returnValue
        overload.isPure = baseFunction.isPure
        overload.isLatent = baseFunction.isLatent
        overload.allowsRecursion = baseFunction.allowsRecursion
        overload.maxRecursionDepth = baseFunction.maxRecursionDepth
        overload.recursionWarningDepth = baseFunction.recursionWarningDepth
        overload.category = baseFunction.category
        
        functions.append(overload)
        markDirty()
        
        return overload
    }
    
    func getFunctionOverloads(for functionName: String) -> [GraphFunction] {
        return functions.filter { $0.name == functionName }
    }
    
    func findBestOverload(functionName: String, argumentTypes: [PortType]) -> GraphFunction? {
        let overloads = getFunctionOverloads(for: functionName)
        
        // Find exact match first
        for overload in overloads {
            if overload.parameters.count == argumentTypes.count {
                let typesMatch = zip(overload.parameters, argumentTypes).allSatisfy { param, argType in
                    param.type == argType || param.isOptional
                }
                if typesMatch {
                    return overload
                }
            }
        }
        
        // Find compatible match with type conversion
        for overload in overloads {
            if isCompatibleSignature(overload.parameters, argumentTypes: argumentTypes) {
                return overload
            }
        }
        
        return nil
    }
    
    private func isCompatibleSignature(_ parameters: [FunctionParameter], argumentTypes: [PortType]) -> Bool {
        guard parameters.count == argumentTypes.count else { return false }
        
        for (param, argType) in zip(parameters, argumentTypes) {
            if !areTypesCompatible(param.type, argType) && !param.isOptional {
                return false
            }
        }
        
        return true
    }
    
    private func areTypesCompatible(_ paramType: PortType, _ argType: PortType) -> Bool {
        // Same types are compatible
        if paramType == argType { return true }
        
        // 'any' type is compatible with everything
        if paramType == .any || argType == .any { return true }
        
        // Numeric type conversions
        let numericTypes: [PortType] = [.int, .float, .vector3]
        if numericTypes.contains(paramType) && numericTypes.contains(argType) { return true }
        
        return false
    }
    
    func validateOverloads() -> [OverloadConflict] {
        var conflicts: [OverloadConflict] = []
        let functionGroups = Dictionary(grouping: functions, by: { $0.name })
        
        for (functionName, overloads) in functionGroups {
            if overloads.count > 1 {
                // Check for ambiguous overloads
                for i in 0..<overloads.count {
                    for j in (i+1)..<overloads.count {
                        let overload1 = overloads[i]
                        let overload2 = overloads[j]
                        
                        if areOverloadsAmbiguous(overload1, overload2) {
                            conflicts.append(OverloadConflict(
                                functionName: functionName,
                                overload1: overload1,
                                overload2: overload2,
                                type: .ambiguousSignature,
                                message: "Overloads have ambiguous signatures that could cause conflicts"
                            ))
                        }
                        
                        if areOverloadsIdentical(overload1, overload2) {
                            conflicts.append(OverloadConflict(
                                functionName: functionName,
                                overload1: overload1,
                                overload2: overload2,
                                type: .identicalSignature,
                                message: "Overloads have identical signatures - one will never be called"
                            ))
                        }
                    }
                }
            }
        }
        
        return conflicts
    }
    
    private func areOverloadsAmbiguous(_ overload1: GraphFunction, _ overload2: GraphFunction) -> Bool {
        // Check if the signatures are too similar to be distinguished
        guard overload1.parameters.count == overload2.parameters.count else { return false }
        
        for (param1, param2) in zip(overload1.parameters, overload2.parameters) {
            // If types are identical and neither is optional, they could be ambiguous
            if param1.type == param2.type && !param1.isOptional && !param2.isOptional {
                return true
            }
        }
        
        return false
    }
    
    private func areOverloadsIdentical(_ overload1: GraphFunction, _ overload2: GraphFunction) -> Bool {
        guard overload1.parameters.count == overload2.parameters.count else { return false }
        
        return zip(overload1.parameters, overload2.parameters).allSatisfy { param1, param2 in
            param1.type == param2.type && param1.isOptional == param2.isOptional
        }
    }
    
    // MARK: - Pure Function Management
    
    func markAsPure(_ function: GraphFunction) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].isPure = true
            markDirty()
        }
    }
    
    func markAsImpure(_ function: GraphFunction) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].isPure = false
            markDirty()
        }
    }
    
    func validatePurity(_ function: GraphFunction) -> [PurityViolation] {
        var violations: [PurityViolation] = []
        
        // Check for side effects in function nodes
        for node in function.nodes {
            if nodeHasSideEffects(node) {
                violations.append(PurityViolation(
                    nodeType: node.type,
                    nodeName: node.title,
                    violationType: .sideEffect,
                    message: "Node '\(node.title)' has side effects that violate purity"
                ))
            }
        }
        
        // Check for external dependencies
        if functionHasExternalDependencies(function) {
            violations.append(PurityViolation(
                nodeType: .function,
                nodeName: function.name,
                violationType: .externalDependency,
                message: "Function has external dependencies that violate purity"
            ))
        }
        
        // Check for state modification
        if functionModifiesState(function) {
            violations.append(PurityViolation(
                nodeType: .function,
                nodeName: function.name,
                violationType: .stateModification,
                message: "Function modifies state which violates purity"
            ))
        }
        
        return violations
    }
    
    private func nodeHasSideEffects(_ node: GraphNode) -> Bool {
        switch node.type {
        case .setter, .variableSet:
            return true // Modifies variables
        case .fileWrite, .fileDelete, .fileCopy, .fileMove:
            return true // File system operations
        case .audioPlay, .audioStop, .audioPause, .audioResume:
            return true // Audio operations
        case .uiShow, .uiHide, .uiSetText, .uiSetPosition, .uiSetSize, .uiSetVisible, .uiSetEnabled:
            return true // UI operations
        case .physicsAddForce, .physicsAddImpulse, .physicsSetVelocity, .physicsSetMass:
            return true // Physics operations
        default:
            return false
        }
    }
    
    private func functionHasExternalDependencies(_ function: GraphFunction) -> Bool {
        // Check if function uses external systems
        return function.nodes.contains { node in
            node.type == .fileRead || 
            node.type == .audioGetVolume ||
            node.type == .uiGetText ||
            node.type == .physicsGetVelocity ||
            node.type == .transformGetPosition
        }
    }
    
    private func functionModifiesState(_ function: GraphFunction) -> Bool {
        // Check if function modifies any global state
        return function.nodes.contains { node in
            node.type == .setter ||
            node.type == .variableSet ||
            node.title.contains("Set") ||
            node.title.contains("Modify")
        }
    }
    
    func getPureFunctions() -> [GraphFunction] {
        return functions.filter { $0.isPure }
    }
    
    func getImpureFunctions() -> [GraphFunction] {
        return functions.filter { !$0.isPure }
    }
    
    func suggestPurityImprovements(_ function: GraphFunction) -> [PuritySuggestion] {
        var suggestions: [PuritySuggestion] = []
        
        let violations = validatePurity(function)
        
        for violation in violations {
            switch violation.violationType {
            case .sideEffect:
                suggestions.append(PuritySuggestion(
                    type: .removeSideEffects,
                    message: "Consider removing or isolating side effects in '\(violation.nodeName)'",
                    priority: .high
                ))
            case .externalDependency:
                suggestions.append(PuritySuggestion(
                    type: .passDependenciesAsParameters,
                    message: "Pass external dependencies as parameters instead of accessing them directly",
                    priority: .medium
                ))
            case .stateModification:
                suggestions.append(PuritySuggestion(
                    type: .returnNewState,
                    message: "Return new state instead of modifying existing state",
                    priority: .high
                ))
            }
        }
        
        return suggestions
    }
    
    // MARK: - Latent Function Management
    
    func markAsLatent(_ function: GraphFunction, duration: TimeInterval = 1.0) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].isLatent = true
            markDirty()
        }
    }
    
    func markAsImmediate(_ function: GraphFunction) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].isLatent = false
            markDirty()
        }
    }
    
    func getLatentFunctions() -> [GraphFunction] {
        return functions.filter { $0.isLatent }
    }
    
    func getImmediateFunctions() -> [GraphFunction] {
        return functions.filter { !$0.isLatent }
    }
    
    func validateLatentFunction(_ function: GraphFunction) -> [LatentFunctionIssue] {
        var issues: [LatentFunctionIssue] = []
        
        if !function.isLatent {
            return issues
        }
        
        // Check if function has proper async structure
        if !hasAsyncStructure(function) {
            issues.append(LatentFunctionIssue(
                type: .missingAsyncStructure,
                message: "Latent function should have proper async structure with start/complete events",
                severity: .warning
            ))
        }
        
        // Check if function has timeout handling
        if !hasTimeoutHandling(function) {
            issues.append(LatentFunctionIssue(
                type: .missingTimeout,
                message: "Latent function should have timeout handling to prevent infinite waiting",
                severity: .warning
            ))
        }
        
        // Check if function has cancellation support
        if !hasCancellationSupport(function) {
            issues.append(LatentFunctionIssue(
                type: .missingCancellation,
                message: "Consider adding cancellation support for better user experience",
                severity: .info
            ))
        }
        
        // Check for potential deadlocks
        if hasPotentialDeadlock(function) {
            issues.append(LatentFunctionIssue(
                type: .potentialDeadlock,
                message: "Function may cause deadlock - review async dependencies",
                severity: .error
            ))
        }
        
        return issues
    }
    
    private func hasAsyncStructure(_ function: GraphFunction) -> Bool {
        // Check for event nodes that indicate async flow
        let hasStartEvent = function.nodes.contains { node in
            node.type == .event && node.title.lowercased().contains("start")
        }
        
        let hasCompleteEvent = function.nodes.contains { node in
            node.type == .event && node.title.lowercased().contains("complete")
        }
        
        return hasStartEvent && hasCompleteEvent
    }
    
    private func hasTimeoutHandling(_ function: GraphFunction) -> Bool {
        // Look for timeout-related nodes
        return function.nodes.contains { node in
            node.type == .branch && node.title.lowercased().contains("timeout")
        }
    }
    
    private func hasCancellationSupport(_ function: GraphFunction) -> Bool {
        // Look for cancellation-related nodes
        return function.nodes.contains { node in
            node.type == .event && node.title.lowercased().contains("cancel")
        }
    }
    
    private func hasPotentialDeadlock(_ function: GraphFunction) -> Bool {
        // Simple heuristic: if function calls other latent functions
        // without proper async handling, it might deadlock
        let latentCalls = function.nodes.filter { node in
            node.type == .function && 
            functions.first(where: { $0.name == node.title })?.isLatent == true
        }
        
        return !latentCalls.isEmpty && !hasAsyncStructure(function)
    }
    
    func createLatentExecutionPlan(_ function: GraphFunction) -> LatentExecutionPlan {
        let plan = LatentExecutionPlan(function: function)
        
        // Identify async nodes
        plan.asyncNodes = function.nodes.filter { node in
            node.type == .event || 
            node.type == .delay ||
            node.type == .timer ||
            (node.type == .function && functions.first(where: { $0.name == node.title })?.isLatent == true)
        }
        
        // Create execution timeline
        plan.timeline = generateExecutionTimeline(function)
        
        // Identify potential blocking points
        plan.blockingPoints = identifyBlockingPoints(function)
        
        return plan
    }
    
    private func generateExecutionTimeline(_ function: GraphFunction) -> [TimelineEvent] {
        var timeline: [TimelineEvent] = []
        var currentTime: TimeInterval = 0
        
        for node in function.nodes.sorted(by: { $0.position.x < $1.position.x }) {
            let event = TimelineEvent(
                nodeId: node.id,
                nodeName: node.title,
                startTime: currentTime,
                duration: estimateNodeExecutionTime(node),
                type: getTimelineEventType(node)
            )
            
            timeline.append(event)
            currentTime += event.duration
        }
        
        return timeline
    }
    
    private func estimateNodeExecutionTime(_ node: GraphNode) -> TimeInterval {
        switch node.type {
        case .event: return 0.1
        case .delay, .timer: return 1.0
        case .function:
            if let function = functions.first(where: { $0.name == node.title }) {
                return function.isLatent ? 2.0 : 0.1
            }
            return 0.1
        case .fileRead, .fileWrite: return 0.5
        case .audioPlay, .audioStop: return 0.2
        case .animPlay, .animStop: return 0.3
        default: return 0.1
        }
    }
    
    private func getTimelineEventType(_ node: GraphNode) -> TimelineEventType {
        switch node.type {
        case .event: return .event
        case .delay, .timer: return .delay
        case .function: return .functionCall
        case .fileRead, .fileWrite: return .ioOperation
        case .audioPlay, .audioStop: return .audioOperation
        case .animPlay, .animStop: return .animationOperation
        default: return .computation
        }
    }
    
    private func identifyBlockingPoints(_ function: GraphFunction) -> [BlockingPoint] {
        var blockingPoints: [BlockingPoint] = []
        
        for node in function.nodes {
            if isBlockingNode(node) {
                blockingPoints.append(BlockingPoint(
                    nodeId: node.id,
                    nodeName: node.title,
                    blockingType: getBlockingType(node),
                    estimatedDuration: estimateNodeExecutionTime(node)
                ))
            }
        }
        
        return blockingPoints
    }
    
    private func isBlockingNode(_ node: GraphNode) -> Bool {
        switch node.type {
        case .delay, .timer: return true
        case .function:
            return functions.first(where: { $0.name == node.title })?.isLatent == true
        case .fileRead, .fileWrite: return true
        case .audioPlay: return true
        case .animPlay: return true
        default: return false
        }
    }
    
    private func getBlockingType(_ node: GraphNode) -> BlockingType {
        switch node.type {
        case .delay, .timer: return .timeDelay
        case .function: return .asyncFunction
        case .fileRead, .fileWrite: return .ioOperation
        case .audioPlay: return .audioOperation
        case .animPlay: return .animationOperation
        default: return .computation
        }
    }
    
    // MARK: - Function Documentation Management
    
    func setDocumentation(for function: GraphFunction, documentation: String) {
        if let index = functions.firstIndex(where: { $0.id == function.id }) {
            functions[index].documentation = documentation
            markDirty()
        }
    }
    
    func getDocumentation(for function: GraphFunction) -> String {
        return function.documentation
    }
    
    func generateDocumentation(_ function: GraphFunction) -> String {
        var doc = "# \(function.name)\n\n"
        
        if !function.description.isEmpty {
            doc += "## Description\n\(function.description)\n\n"
        }
        
        // Parameters section
        if !function.parameters.isEmpty {
            doc += "## Parameters\n\n"
            for param in function.parameters {
                doc += "- **\(param.name)** (\(param.type.rawValue.capitalized))"
                if !param.description.isEmpty {
                    doc += ": \(param.description)"
                }
                if param.isOptional {
                    doc += " *(Optional)*"
                }
                if let defaultValue = param.defaultValue {
                    doc += " - Default: `\(defaultValue)`"
                }
                doc += "\n"
            }
            doc += "\n"
        }
        
        // Return value section
        if let returnValue = function.returnValue {
            doc += "## Return Value\n\n"
            doc += "**Type**: \(returnValue.type.rawValue.capitalized)\n"
            if !returnValue.description.isEmpty {
                doc += "**Description**: \(returnValue.description)\n"
            }
            doc += "\n"
        }
        
        // Function properties
        doc += "## Properties\n\n"
        doc += "- **Category**: \(function.category)\n"
        doc += "- **Pure**: \(function.isPure ? "Yes" : "No")\n"
        doc += "- **Latent**: \(function.isLatent ? "Yes" : "No")\n"
        doc += "- **Allows Recursion**: \(function.allowsRecursion ? "Yes" : "No")\n"
        if function.allowsRecursion {
            doc += "- **Max Recursion Depth**: \(function.maxRecursionDepth)\n"
        }
        doc += "- **Created**: \(function.created, style: .date)\n"
        doc += "- **Modified**: \(function.modified, style: .date)\n"
        doc += "\n"
        
        // Usage examples
        let examples = generateUsageExamples(function)
        if !examples.isEmpty {
            doc += "## Usage Examples\n\n"
            doc += examples + "\n\n"
        }
        
        // Notes
        let notes = generateImplementationNotes(function)
        if !notes.isEmpty {
            doc += "## Implementation Notes\n\n"
            doc += notes + "\n\n"
        }
        
        // See also section
        let relatedFunctions = getRelatedFunctions(function)
        if !relatedFunctions.isEmpty {
            doc += "## See Also\n\n"
            for related in relatedFunctions {
                doc += "- [`\(related.name)`](#\(related.name.lowercased().replacing(" ", "-")))\n"
            }
        }
        
        return doc
    }
    
    private func generateUsageExamples(_ function: GraphFunction) -> String {
        var examples = ""
        
        // Generate example based on function signature
        let paramNames = function.parameters.map { $0.name }.joined(", ")
        let hasReturnValue = function.returnValue != nil
        
        if hasReturnValue {
            examples += "```swift\nlet result = \(function.name)(\(paramNames))\n```\n"
        } else {
            examples += "```swift\n\(function.name)(\(paramNames))\n```\n"
        }
        
        // Add conditional example if function has optional parameters
        let optionalParams = function.parameters.filter { $0.isOptional }
        if !optionalParams.isEmpty {
            let requiredParams = function.parameters.filter { !$0.isOptional }.map { $0.name }.joined(", ")
            examples += "```swift\n// With only required parameters\n"
            if hasReturnValue {
                examples += "let result = \(function.name)(\(requiredParams))\n"
            } else {
                examples += "\(function.name)(\(requiredParams))\n"
            }
            examples += "```\n"
        }
        
        return examples
    }
    
    private func generateImplementationNotes(_ function: GraphFunction) -> String {
        var notes = ""
        
        if function.isPure {
            notes += "- This is a pure function with no side effects\n"
        }
        
        if function.isLatent {
            notes += "- This is a latent function that executes asynchronously\n"
            if function.hasRecursiveCalls() {
                notes += "- Function contains recursive calls - ensure proper base case\n"
            }
        }
        
        if function.allowsRecursion && function.hasRecursiveCalls() {
            notes += "- Recursion is enabled with maximum depth of \(function.maxRecursionDepth)\n"
        }
        
        if !function.localVariables.isEmpty {
            notes += "- Uses \(function.localVariables.count) local variable(s)\n"
        }
        
        // Add notes about overloads
        let overloads = getFunctionOverloads(for: function.name)
        if overloads.count > 1 {
            notes += "- This function has \(overloads.count) overloads available\n"
        }
        
        return notes
    }
    
    private func getRelatedFunctions(_ function: GraphFunction) -> [GraphFunction] {
        // Find functions with similar names or in the same category
        return functions.filter { other in
            other.id != function.id && (
                other.name.lowercased().contains(function.name.lowercased()) ||
                function.name.lowercased().contains(other.name.lowercased()) ||
                other.category == function.category
            )
        }.prefix(5).map { $0 }
    }
    
    func validateDocumentation(_ function: GraphFunction) -> [DocumentationIssue] {
        var issues: [DocumentationIssue] = []
        
        // Check for missing description
        if function.description.isEmpty {
            issues.append(DocumentationIssue(
                type: .missingDescription,
                message: "Function should have a description explaining its purpose",
                severity: .warning
            ))
        }
        
        // Check for undocumented parameters
        let undocumentedParams = function.parameters.filter { $0.description.isEmpty }
        if !undocumentedParams.isEmpty {
            issues.append(DocumentationIssue(
                type: .undocumentedParameters,
                message: "Parameters '\(undocumentedParams.map { $0.name }.joined(", "))' lack documentation",
                severity: .info
            ))
        }
        
        // Check for undocumented return value
        if let returnValue = function.returnValue, returnValue.description.isEmpty {
            issues.append(DocumentationIssue(
                type: .undocumentedReturnValue,
                message: "Return value lacks documentation",
                severity: .info
            ))
        }
        
        // Check for missing examples
        if function.documentation.isEmpty || !function.documentation.contains("## Usage Examples") {
            issues.append(DocumentationIssue(
                type: .missingExamples,
                message: "Consider adding usage examples to improve documentation",
                severity: .info
            ))
        }
        
        return issues
    }
    
    func exportDocumentation(_ function: GraphFunction) -> String {
        let doc = generateDocumentation(function)
        
        // Save to file
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let docsFolder = documentsPath.appendingPathComponent("FunctionDocumentation")
        
        do {
            try FileManager.default.createDirectory(at: docsFolder, withIntermediateDirectories: true)
            let fileName = "\(function.name).replacingOccurrences(of: " ", with: "_").lowercased() + ".md"
            let fileURL = docsFolder.appendingPathComponent(fileName)
            try doc.write(to: fileURL, atomically: true, encoding: .utf8)
            print("[NodeGraph] Documentation exported to: \(fileURL.path)")
        } catch {
            print("[NodeGraph] Failed to export documentation: \(error)")
        }
        
        return doc
    }
    
    // MARK: - Macro Definition Management
    
    func createMacro(name: String, description: String = "", category: String = "Custom") -> GraphMacro {
        let macro = GraphMacro(
            name: name,
            description: description,
            category: category,
            parameters: [],
            nodes: [],
            connections: [],
            isCollapsed: true
        )
        
        macros.append(macro)
        markDirty()
        return macro
    }
    
    func deleteMacro(_ macro: GraphMacro) {
        macros.removeAll { $0.id == macro.id }
        markDirty()
    }
    
    func addParameter(to macro: GraphMacro, name: String, type: PortType, defaultValue: Any? = nil, description: String = "") {
        let parameter = MacroParameter(name: name, type: type, defaultValue: defaultValue, description: description)
        
        if let index = macros.firstIndex(where: { $0.id == macro.id }) {
            macros[index].parameters.append(parameter)
            markDirty()
        }
    }
    
    func removeParameter(from macro: GraphMacro, parameter: MacroParameter) {
        if let index = macros.firstIndex(where: { $0.id == macro.id }) {
            macros[index].parameters.removeAll { $0.id == parameter.id }
            markDirty()
        }
    }
    
    func expandMacro(_ macro: GraphMacro, at position: CGPoint) {
        let (expandedNodes, expandedConnections) = macro.expand(with: [:])
        
        // Position the expanded nodes relative to the macro position
        let offsetX = position.x - (macro.nodes.first?.position.x ?? 0)
        let offsetY = position.y - (macro.nodes.first?.position.y ?? 0)
        
        for (index, var node) in expandedNodes.enumerated() {
            node.position.x += offsetX
            node.position.y += offsetY
        }
        
        // Add expanded nodes and connections to current graph
        nodes.append(contentsOf: expandedNodes)
        connections.append(contentsOf: expandedConnections)
        
        markDirty()
    }
    
    func validateMacro(_ macro: GraphMacro) -> [MacroIssue] {
        var issues: [MacroIssue] = []
        
        // Check for empty macro
        if macro.nodes.isEmpty {
            issues.append(MacroIssue(
                type: .emptyMacro,
                message: "Macro contains no nodes to expand",
                severity: .warning
            ))
        }
        
        // Check for undefined parameters
        let nodeNames = Set(macro.nodes.map { $0.title })
        let paramNames = Set(macro.parameters.map { $0.name })
        let undefinedParams = paramNames.subtracting(nodeNames)
        
        if !undefinedParams.isEmpty {
            issues.append(MacroIssue(
                type: .undefinedParameters,
                message: "Parameters '\(undefinedParams.joined(", "))' are not used in macro",
                severity: .warning
            ))
        }
        
        // Check for circular dependencies
        let nodeDependencies = findMacroDependencies(macro)
        if hasCircularDependency(nodeDependencies) {
            issues.append(MacroIssue(
                type: .circularDependency,
                message: "Macro contains circular dependencies",
                severity: .error
            ))
        }
        
        return issues
    }
    
    private func findMacroDependencies(_ macro: GraphMacro) -> [String: Set<String>] {
        var dependencies: [String: Set<String>] = [:]
        
        for node in macro.nodes {
            var nodeDeps: Set<String> = []
            
            // Check connections to other nodes within the macro
            for connection in macro.connections {
                if let fromNode = macro.nodes.first(where: { $0.id == connection.fromNode }),
                   let toNode = macro.nodes.first(where: { $0.id == connection.toNode }) {
                    nodeDeps.insert(toNode.title)
                }
            }
            
            dependencies[node.title] = nodeDeps
        }
        
        return dependencies
    }
    
    private func hasCircularDependency(_ dependencies: [String: Set<String>]) -> Bool {
        var visited: Set<String> = []
        
        func hasCycle(from node: String, path: Set<String>) -> Bool {
            if visited.contains(node) {
                return path.contains(node)
            }
            
            visited.insert(node)
            
            for dependency in dependencies[node] ?? [] {
                if hasCycle(from: dependency, path: path.union([node])) {
                    return true
                }
            }
            
            visited.remove(node)
            return false
        }
        
        return dependencies.keys.contains { hasCycle(from: $0, path: Set([$0])) }
    }
    
    func getMacros(in category: String? = nil) -> [GraphMacro] {
        if let category = category {
            return macros.filter { $0.category == category }
        }
        return macros
    }
    
    func getMacroCategories() -> [String] {
        return Set(macros.map { $0.category }).sorted()
    }
    
    func searchMacros(query: String) -> [GraphMacro] {
        let query = query.lowercased()
        return macros.filter { 
            $0.name.lowercased().contains(query) ||
            $0.description.lowercased().contains(query) ||
            $0.category.lowercased().contains(query)
        }
    }
    
    func exportMacro(_ macro: GraphMacro) -> String {
        let encoder = JSONEncoder()
        encoder.outputFormatting = .prettyPrinted
        
        do {
            let data = try encoder.encode(macro)
            if let jsonString = String(data: data, encoding: .utf8) {
                return jsonString
            }
        } catch {
            print("[NodeGraph] Failed to export macro: \(error)")
        }
        
        return "{}"
    }
    
    func importMacro(from jsonString: String) -> GraphMacro? {
        let decoder = JSONDecoder()
        
        do {
            let data = jsonString.data(using: .utf8)
            let macro = try decoder.decode(GraphMacro.self, from: data)
            
            // Validate imported macro
            let issues = validateMacro(macro)
            if !issues.isEmpty {
                print("[NodeGraph] Imported macro has issues: \(issues.map { $0.message }.joined(", "))")
            }
            
            return macro
        } catch {
            print("[NodeGraph] Failed to import macro: \(error)")
            return nil
        }
    }
    
    private func initializeSampleFunctions() {
        // Create sample functions for demonstration
        let sampleFunction1 = GraphFunction(name: "CalculateDamage", description: "Calculate damage based on attack power and defense")
        sampleFunction1.parameters = [
            FunctionParameter(name: "attackPower", type: .float),
            FunctionParameter(name: "defense", type: .float)
        ]
        sampleFunction1.returnValue = FunctionReturnValue(type: .float, description: "Calculated damage")
        
        let sampleFunction2 = GraphFunction(name: "IsPlayerInRange", description: "Check if player is within range")
        sampleFunction2.parameters = [
            FunctionParameter(name: "playerPosition", type: .vector3),
            FunctionParameter(name: "targetPosition", type: .vector3),
            FunctionParameter(name: "range", type: .float)
        ]
        sampleFunction2.returnValue = FunctionReturnValue(type: .bool, description: "Whether player is in range")
        
        functions = [sampleFunction1, sampleFunction2]
    }
    
    func saveGraph() {
        let graphData = serializeGraph()
        
        guard let data = try? JSONEncoder().encode(graphData) else {
            print("[NodeGraph] Failed to encode graph data")
            return
        }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let graphPath = documentsPath.appendingPathComponent("NodeGraphs")
        let filePath = graphPath.appendingPathComponent("\(currentGraphName).json")
        
        do {
            try FileManager.default.createDirectory(at: graphPath, withIntermediateDirectories: true)
            try data.write(to: filePath)
            isDirty = false
            lastSavedTime = Date()
            print("[NodeGraph] Saved graph to: \(filePath.path)")
        } catch {
            print("[NodeGraph] Failed to save graph: \(error)")
        }
    }
    
    func loadGraph(name: String) {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let graphPath = documentsPath.appendingPathComponent("NodeGraphs")
        let filePath = graphPath.appendingPathComponent("\(name).json")
        
        guard let data = try? Data(contentsOf: filePath) else {
            print("[NodeGraph] Failed to load graph data from: \(filePath.path)")
            return
        }
        
        do {
            let graphData = try JSONDecoder().decode(GraphData.self, from: data)
            deserializeGraph(graphData)
            currentGraphName = name
            isDirty = false
            lastSavedTime = Date()
            print("[NodeGraph] Loaded graph: \(name)")
        } catch {
            print("[NodeGraph] Failed to decode graph data: \(error)")
        }
    }
    
    func getSavedGraphs() -> [String] {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let graphPath = documentsPath.appendingPathComponent("NodeGraphs")
        
        guard let enumerator = FileManager.default.enumerator(at: graphPath, includingPropertiesForKeys: nil) else {
            return []
        }
        
        var graphs: [String] = []
        for case let fileURL as URL in enumerator {
            if fileURL.pathExtension == "json" {
                graphs.append(fileURL.deletingPathExtension().lastPathComponent)
            }
        }
        
        return graphs.sorted()
    }
    
    private func serializeGraph() -> GraphData {
        let nodeData = nodes.map { node in
            GraphNodeData(
                id: node.id,
                type: node.type.rawValue,
                title: node.title,
                position: node.position,
                inputs: node.inputs.map { port in
                    NodePortData(
                        id: port.id,
                        name: port.name,
                        type: port.type.rawValue,
                        defaultValue: port.defaultValue?.description
                    )
                },
                outputs: node.outputs.map { port in
                    NodePortData(
                        id: port.id,
                        name: port.name,
                        type: port.type.rawValue,
                        defaultValue: port.defaultValue?.description
                    )
                },
                isCollapsed: node.isCollapsed,
                comment: node.comment
            )
        }
        
        let connectionData = connections.map { conn in
            GraphConnectionData(
                id: conn.id,
                fromNode: conn.fromNode,
                fromPort: conn.fromPort,
                toNode: conn.toNode,
                toPort: conn.toPort
            )
        }
        
        let metadata = GraphMetadata(
            version: "1.0",
            created: Date(),
            modified: Date(),
            author: "User",
            description: "Node graph created with Jules Engine"
        )
        
        return GraphData(nodes: nodeData, connections: connectionData, metadata: metadata)
    }
    
    private func deserializeGraph(_ graphData: GraphData) {
        nodes = graphData.nodes.map { nodeData in
            let inputs = nodeData.inputs.map { portData in
                NodePort(
                    name: portData.name,
                    type: PortType(rawValue: portData.type) ?? .any
                )
            }
            
            let outputs = nodeData.outputs.map { portData in
                NodePort(
                    name: portData.name,
                    type: PortType(rawValue: portData.type) ?? .any
                )
            }
            
            var node = GraphNode(
                type: NodeType(rawValue: nodeData.type) ?? .function,
                title: nodeData.title,
                position: nodeData.position,
                inputs: inputs,
                outputs: outputs
            )
            node.id = nodeData.id
            node.isCollapsed = nodeData.isCollapsed
            node.comment = nodeData.comment
            
            return node
        }
        
        connections = graphData.connections.map { connData in
            GraphConnection(
                fromNode: connData.fromNode,
                fromPort: connData.fromPort,
                toNode: connData.toNode,
                toPort: connData.toPort
            )
        }
    }
    
    // MARK: - Macro System Methods
    
    func createMacro(name: String, description: String, selectedNodeIds: Set<UUID>) {
        let macroNodes = nodes.filter { selectedNodeIds.contains($0.id) }
        let macroConnections = connections.filter { conn in
            selectedNodeIds.contains(conn.fromNode) && selectedNodeIds.contains(conn.toNode)
        }
        
        let macro = GraphMacro(
            name: name,
            description: description,
            parameters: [],
            nodes: macroNodes,
            connections: macroConnections
        )
        
        macros.append(macro)
        markDirty()
    }
    
    func expandMacro(_ macro: GraphMacro, at position: CGPoint, parameterValues: [String: Any] = [:]) {
        let (expandedNodes, expandedConnections) = macro.expand(with: parameterValues)
        
        // Offset nodes to the specified position
        let offset = position
        for i in 0..<expandedNodes.count {
            expandedNodes[i].position = CGPoint(
                x: expandedNodes[i].position.x + offset.x,
                y: expandedNodes[i].position.y + offset.y
            )
        }
        
        nodes.append(contentsOf: expandedNodes)
        connections.append(contentsOf: expandedConnections)
        markDirty()
    }
    
    // MARK: - Variable System Methods
    
    func createVariable(name: String, type: PortType, scope: GraphVariable.VariableScope, defaultValue: Any? = nil) {
        let variable = GraphVariable(
            name: name,
            type: type,
            scope: scope,
            defaultValue: defaultValue,
            description: ""
        )
        variables.append(variable)
        markDirty()
    }
    
    func generateVariableGetNode(_ variable: GraphVariable, at position: CGPoint) {
        let getNode = GraphNode(
            type: .variableGet,
            title: "Get \\(variable.name)",
            position: position,
            outputs: [NodePort(name: "Value", type: variable.type)]
        )
        nodes.append(getNode)
        markDirty()
    }
    
    func generateVariableSetNode(_ variable: GraphVariable, at position: CGPoint) {
        let setNode = GraphNode(
            type: .variableSet,
            title: "Set \\(variable.name)",
            position: position,
            inputs: [
                NodePort(name: "Exec", type: .flow),
                NodePort(name: "Value", type: variable.type)
            ],
            outputs: [NodePort(name: "Exec", type: .flow)]
        )
        nodes.append(setNode)
        markDirty()
    }
    
    func renameVariable(_ variable: GraphVariable, newName: String) {
        if let index = variables.firstIndex(where: { $0.id == variable.id }) {
            variables[index].name = newName
            
            // Update all related nodes
            for i in 0..<nodes.count {
                if nodes[i].type == .variableGet || nodes[i].type == .variableSet {
                    if nodes[i].title.contains("Get \\(variable.name)") {
                        nodes[i].title = "Get \\(newName)"
                    } else if nodes[i].title.contains("Set \\(variable.name)") {
                        nodes[i].title = "Set \\(newName)"
                    }
                }
            }
            markDirty()
        }
    }
    
    func findVariableReferences(_ variable: GraphVariable) -> [UUID] {
        return nodes.filter { node in
            (node.type == .variableGet || node.type == .variableSet) &&
            node.title.contains(variable.name)
        }.map { $0.id }
    }
    
    // MARK: - Template System Methods
    
    func initializeTemplates() {
        templates = [
            GraphTemplate(
                name: "For Each Loop",
                description: "Iterate over array elements",
                category: "Loops",
                nodes: [],
                connections: [],
                inputs: [
                    TemplatePort(name: "Exec", type: .flow),
                    TemplatePort(name: "Array", type: .array)
                ],
                outputs: [
                    TemplatePort(name: "Exec", type: .flow),
                    TemplatePort(name: "Element", type: .any),
                    TemplatePort(name: "Index", type: .int)
                ]
            ),
            GraphTemplate(
                name: "Branch Condition",
                description: "Conditional execution",
                category: "Flow Control",
                nodes: [],
                connections: [],
                inputs: [
                    TemplatePort(name: "Exec", type: .flow),
                    TemplatePort(name: "Condition", type: .bool)
                ],
                outputs: [
                    TemplatePort(name: "True", type: .flow),
                    TemplatePort(name: "False", type: .flow)
                ]
            )
        ]
    }
    
    func createGraphInstance(from template: GraphTemplate, at position: CGPoint) {
        let instance = GraphInstance(
            templateId: template.id,
            name: template.name,
            position: position,
            parameterValues: [:],
            parentGraphId: parentGraph
        )
        
        graphInstances.append(instance)
        
        // Create instance node
        let instanceNode = GraphNode(
            type: .graphInstance,
            title: template.name,
            position: position,
            inputs: template.inputs.map { NodePort(name: $0.name, type: $0.type) },
            outputs: template.outputs.map { NodePort(name: $0.name, type: $0.type) }
        )
        nodes.append(instanceNode)
        markDirty()
    }
    
    // MARK: - Data Node Creation Methods
    
    func createConstantNode(type: PortType, value: Any, at position: CGPoint) {
        let constantNode = GraphNode(
            type: .constantValue,
            title: "\\(value)",
            position: position,
            outputs: [NodePort(name: "Value", type: type)]
        )
        nodes.append(constantNode)
        markDirty()
    }
    
    func createLiteralNode(type: PortType, at position: CGPoint) {
        let literalNode = GraphNode(
            type: .literal,
            title: "Literal \\(type.rawValue)",
            position: position,
            outputs: [NodePort(name: "Value", type: type)]
        )
        nodes.append(literalNode)
        markDirty()
    }
    
    func createEnumNode(enumName: String, value: String, at position: CGPoint) {
        let enumNode = GraphNode(
            type: .enumValue,
            title: "\\(enumName).\\(value)",
            position: position,
            outputs: [NodePort(name: "Value", type: .enumType)]
        )
        nodes.append(enumNode)
        markDirty()
    }
    
    func createStructConstructorNode(structName: String, fields: [(String, PortType)], at position: CGPoint) {
        let constructorNode = GraphNode(
            type: .structConstructor,
            title: "Make \\(structName)",
            position: position,
            inputs: fields.map { NodePort(name: $0.0, type: $0.1) },
            outputs: [NodePort(name: "Struct", type: .structType)]
        )
        nodes.append(constructorNode)
        markDirty()
    }
    
    func createObjectInstanceNode(className: String, at position: CGPoint) {
        let instanceNode = GraphNode(
            type: .objectInstance,
            title: "New \\(className)",
            position: position,
            outputs: [NodePort(name: "Instance", type: .classType)]
        )
        nodes.append(instanceNode)
        markDirty()
    }
    
    func markDirty() {
        isDirty = true
    }
    
    // MARK: - Hot Reload Functions
    
    func enableHotReload() {
        guard hotReloadEnabled else { return }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let graphPath = documentsPath.appendingPathComponent("NodeGraphs")
        
        // Create directory if it doesn't exist
        try? FileManager.default.createDirectory(at: graphPath, withIntermediateDirectories: true)
        
        // Setup file watcher
        let fileDescriptor = open(graphPath.path, O_EVTONLY)
        guard fileDescriptor != -1 else {
            print("[NodeGraph] Failed to setup file watcher")
            return
        }
        
        fileWatcher = DispatchSource.makeFileSystemObjectSource(
            fileDescriptor: fileDescriptor,
            eventMask: .write,
            queue: DispatchQueue.global(qos: .utility)
        )
        
        fileWatcher?.setEventHandler { [weak self] in
            self?.handleFileModification()
        }
        
        fileWatcher?.setCancelHandler {
            close(fileDescriptor)
        }
        
        fileWatcher?.resume()
        print("[NodeGraph] Hot reload enabled for: \(graphPath.path)")
    }
    
    func disableHotReload() {
        fileWatcher?.cancel()
        fileWatcher = nil
        print("[NodeGraph] Hot reload disabled")
    }
    
    private func handleFileModification() {
        guard hotReloadEnabled else { return }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let graphPath = documentsPath.appendingPathComponent("NodeGraphs")
        let filePath = graphPath.appendingPathComponent("\(currentGraphName).json")
        
        // Check file modification date
        guard let attributes = try? FileManager.default.attributesOfItem(atPath: filePath.path),
              let modificationDate = attributes[.modificationDate] as? Date else {
            return
        }
        
        // Debounce rapid file changes
        if let lastMod = lastFileModification, modificationDate.timeIntervalSince(lastMod) < 0.5 {
            return
        }
        
        lastFileModification = modificationDate
        
        // Don't reload if we just saved
        if let lastSave = lastSavedTime, modificationDate.timeIntervalSince(lastSave) < 1.0 {
            return
        }
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [weak self] in
            self?.reloadCurrentGraph()
        }
    }
    
    private func reloadCurrentGraph() {
        guard !isDirty else {
            print("[NodeGraph] Skipping hot reload - graph has unsaved changes")
            return
        }
        
        print("[NodeGraph] Hot reloading graph: \(currentGraphName)")
        loadGraph(name: currentGraphName)
    }
    
    func toggleHotReload() {
        hotReloadEnabled.toggle()
        if hotReloadEnabled {
            enableHotReload()
        } else {
            disableHotReload()
        }
    }
    
    // MARK: - Version Control Friendly Format
    
    func exportToVCFormat() {
        let vcData = serializeToVCFormat()
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let graphPath = documentsPath.appendingPathComponent("NodeGraphs")
        let yamlPath = graphPath.appendingPathComponent("\(currentGraphName).yaml")
        
        do {
            try FileManager.default.createDirectory(at: graphPath, withIntermediateDirectories: true)
            let yamlString = generateYAML(from: vcData)
            try yamlString.write(to: yamlPath, atomically: true, encoding: .utf8)
            print("[NodeGraph] Exported VC format to: \(yamlPath.path)")
        } catch {
            print("[NodeGraph] Failed to export VC format: \(error)")
        }
    }
    
    private func serializeToVCFormat() -> VCFormatData {
        let nodeData = nodes.map { node in
            VCNodeData(
                id: node.id.uuidString.prefix(8).description, // Short ID for readability
                type: node.type.rawValue,
                title: node.title,
                position: VCPosition(x: Int(node.position.x), y: Int(node.position.y)),
                inputs: node.inputs.enumerated().map { index, port in
                    VCPortData(
                        name: port.name,
                        type: port.type.rawValue,
                        index: index
                    )
                },
                outputs: node.outputs.enumerated().map { index, port in
                    VCPortData(
                        name: port.name,
                        type: port.type.rawValue,
                        index: index
                    )
                },
                metadata: VCNodeMetadata(
                    collapsed: node.isCollapsed,
                    comment: node.comment.isEmpty ? nil : node.comment
                )
            )
        }
        
        let connectionData = connections.map { conn in
            let fromNode = nodes.first { $0.id == conn.fromNode }
            let toNode = nodes.first { $0.id == conn.toNode }
            
            return VCConnectionData(
                from: VCConnectionEndpoint(
                    node: fromNode?.id.uuidString.prefix(8).description ?? "unknown",
                    port: fromNode?.outputs[conn.fromPort].name ?? "unknown"
                ),
                to: VCConnectionEndpoint(
                    node: toNode?.id.uuidString.prefix(8).description ?? "unknown", 
                    port: toNode?.inputs[conn.toPort].name ?? "unknown"
                )
            )
        }
        
        return VCFormatData(
            version: "1.0",
            format: "jules-nodegraph",
            metadata: VCMetadata(
                name: currentGraphName,
                created: ISO8601DateFormatter().string(from: Date()),
                description: "Node graph created with Jules Engine"
            ),
            nodes: nodeData,
            connections: connectionData
        )
    }
    
    private func generateYAML(from data: VCFormatData) -> String {
        var yaml = "# Jules Engine Node Graph\n"
        yaml += "# Version: \(data.version)\n"
        yaml += "# Format: \(data.format)\n\n"
        
        // Metadata
        yaml += "metadata:\n"
        yaml += "  name: \(data.metadata.name)\n"
        yaml += "  created: \(data.metadata.created)\n"
        if !data.metadata.description.isEmpty {
            yaml += "  description: \(data.metadata.description)\n"
        }
        yaml += "\n"
        
        // Nodes
        yaml += "nodes:\n"
        for node in data.nodes {
            yaml += "  - id: \(node.id)\n"
            yaml += "    type: \(node.type)\n"
            yaml += "    title: \(node.title)\n"
            yaml += "    position:\n"
            yaml += "      x: \(node.position.x)\n"
            yaml += "      y: \(node.position.y)\n"
            
            if !node.inputs.isEmpty {
                yaml += "    inputs:\n"
                for input in node.inputs {
                    yaml += "      - name: \(input.name)\n"
                    yaml += "        type: \(input.type)\n"
                    yaml += "        index: \(input.index)\n"
                }
            }
            
            if !node.outputs.isEmpty {
                yaml += "    outputs:\n"
                for output in node.outputs {
                    yaml += "      - name: \(output.name)\n"
                    yaml += "        type: \(output.type)\n"
                    yaml += "        index: \(output.index)\n"
                }
            }
            
            if let metadata = node.metadata {
                yaml += "    metadata:\n"
                yaml += "      collapsed: \(metadata.collapsed)\n"
                if let comment = metadata.comment {
                    yaml += "      comment: \(comment)\n"
                }
            }
            yaml += "\n"
        }
        
        // Connections
        if !data.connections.isEmpty {
            yaml += "connections:\n"
            for conn in data.connections {
                yaml += "  - from:\n"
                yaml += "      node: \(conn.from.node)\n"
                yaml += "      port: \(conn.from.port)\n"
                yaml += "    to:\n"
                yaml += "      node: \(conn.to.node)\n"
                yaml += "      port: \(conn.to.port)\n"
                yaml += "\n"
            }
        }
        
        return yaml
    }
    
    func importFromVCFormat(yamlString: String) {
        // TODO: Implement YAML parsing for import
        print("[NodeGraph] VC format import not yet implemented")
    }
    
    // MARK: - Import from Other Formats
    
    func importFromJSON(jsonData: Data) {
        do {
            let graphData = try JSONDecoder().decode(GraphData.self, from: jsonData)
            deserializeGraph(graphData)
            print("[NodeGraph] Successfully imported from JSON")
        } catch {
            print("[NodeGraph] Failed to import from JSON: \(error)")
        }
    }
    
    func importFromYAML(yamlString: String) {
        // Basic YAML parsing for node graphs
        let lines = yamlString.components(separatedBy: .newlines)
        var currentSection = ""
        var nodes: [GraphNode] = []
        var connections: [GraphConnection] = []
        var currentNode: GraphNode?
        var nodeMap: [String: UUID] = [:]
        
        for line in lines {
            let trimmedLine = line.trimmingCharacters(in: .whitespaces)
            
            if trimmedLine.hasPrefix("nodes:") {
                currentSection = "nodes"
                continue
            } else if trimmedLine.hasPrefix("connections:") {
                currentSection = "connections"
                continue
            } else if trimmedLine.isEmpty || trimmedLine.hasPrefix("#") {
                continue
            }
            
            switch currentSection {
            case "nodes":
                parseNodeYAML(line: trimmedLine, node: &currentNode, nodes: &nodes, nodeMap: &nodeMap)
            case "connections":
                parseConnectionYAML(line: trimmedLine, connections: &connections, nodeMap: nodeMap)
            default:
                break
            }
        }
        
        // Add the last node if there's one pending
        if let node = currentNode {
            nodes.append(node)
        }
        
        self.nodes = nodes
        self.connections = connections
        markDirty()
        print("[NodeGraph] Successfully imported from YAML")
    }
    
    private func parseNodeYAML(line: String, node: inout GraphNode?, nodes: inout [GraphNode], nodeMap: inout [String: UUID]) {
        let trimmed = line.trimmingCharacters(in: .whitespaces)
        
        if trimmed.hasPrefix("- id:") {
            // Save previous node if exists
            if let prevNode = node {
                nodes.append(prevNode)
            }
            
            // Start new node
            let nodeId = trimmed.replacingOccurrences(of: "- id:", with: "").trimmingCharacters(in: .whitespaces)
            node = GraphNode(
                type: .function,
                title: "Imported Node",
                position: CGPoint(x: 100, y: 100),
                inputs: [],
                outputs: []
            )
            nodeMap[nodeId] = node!.id
        } else if let n = node {
            if trimmed.hasPrefix("type:") {
                let typeString = trimmed.replacingOccurrences(of: "type:", with: "").trimmingCharacters(in: .whitespaces)
                n.type = NodeType(rawValue: typeString) ?? .function
            } else if trimmed.hasPrefix("title:") {
                n.title = trimmed.replacingOccurrences(of: "title:", with: "").trimmingCharacters(in: .whitespaces)
            } else if trimmed.hasPrefix("position:") {
                // Position parsing would need to handle multi-line YAML
                // This is a simplified implementation
            }
        }
    }
    
    private func parseConnectionYAML(line: String, connections: inout [GraphConnection], nodeMap: [String: UUID]) {
        let trimmed = line.trimmingCharacters(in: .whitespaces)
        
        if trimmed.hasPrefix("- from:") {
            // This is a simplified connection parser
            // In a full implementation, you'd need to handle multi-line YAML properly
            // For now, this is a placeholder that shows the structure
        }
    }
    
    func importFromBlueprint(blueprintData: Data) {
        // Import from Unreal Engine Blueprint format (simplified)
        print("[NodeGraph] Blueprint import not yet implemented")
    }
    
    func importFromVisualScript(scriptData: Data) {
        // Import from Unity Visual Scripting format (simplified)
        print("[NodeGraph] Visual Script import not yet implemented")
    }
    
    func importFromGraphML(graphMLData: Data) {
        // Import from GraphML format (XML-based)
        print("[NodeGraph] GraphML import not yet implemented")
    }
    
    func importFromDOT(dotString: String) {
        // Import from DOT/GraphViz format
        let lines = dotString.components(separatedBy: .newlines)
        var nodes: [GraphNode] = []
        var connections: [GraphConnection] = []
        var nodeMap: [String: UUID] = [:]
        
        for line in lines {
            let trimmed = line.trimmingCharacters(in: .whitespaces)
            
            if trimmed.hasPrefix("\"") && trimmed.contains("[") {
                // Parse node definition: "Node1" [label="Start"];
                let components = trimmed.components(separatedBy: "\"")
                if components.count >= 3 {
                    let nodeId = components[1]
                    let labelPart = components[2]
                    
                    var label = nodeId
                    if labelPart.contains("label=") {
                        let labelStart = labelPart.range(of: "label=\"")?.upperBound
                        let labelEnd = labelPart.range(of: "\"")?.lowerBound
                        if let start = labelStart, let end = labelEnd, start < end {
                            label = String(labelPart[start..<end])
                        }
                    }
                    
                    let node = GraphNode(
                        type: .function,
                        title: label,
                        position: CGPoint(x: CGFloat.random(in: 50...500), y: CGFloat.random(in: 50...400)),
                        inputs: [],
                        outputs: []
                    )
                    nodes.append(node)
                    nodeMap[nodeId] = node.id
                }
            } else if trimmed.contains("->") {
                // Parse edge definition: Node1 -> Node2;
                let components = trimmed.components(separatedBy: "->")
                if components.count == 2 {
                    let fromId = components[0].trimmingCharacters(in: .whitespacesAndNewlines)
                    let toId = components[1].trimmingCharacters(in: .whitespacesAndNewlines)
                        .replacingOccurrences(of: ";", with: "")
                    
                    if let fromUUID = nodeMap[fromId], let toUUID = nodeMap[toId] {
                        // Create dummy ports for the connection
                        let fromNode = nodes.first { $0.id == fromUUID }
                        let toNode = nodes.first { $0.id == toUUID }
                        
                        fromNode?.outputs.append(NodePort(name: "Output", type: .flow))
                        toNode?.inputs.append(NodePort(name: "Input", type: .flow))
                        
                        let connection = GraphConnection(
                            fromNode: fromUUID,
                            fromPort: (fromNode?.outputs.count ?? 1) - 1,
                            toNode: toUUID,
                            toPort: (toNode?.inputs.count ?? 1) - 1
                        )
                        connections.append(connection)
                    }
                }
            }
        }
        
        self.nodes = nodes
        self.connections = connections
        markDirty()
        print("[NodeGraph] Successfully imported from DOT format")
    }
    
    // MARK: - Merge Conflict Resolution
    
    func detectMergeConflicts(localGraph: GraphData, remoteGraph: GraphData) -> [MergeConflict] {
        var conflicts: [MergeConflict] = []
        
        // Find conflicting nodes (same ID but different content)
        let localNodeMap = Dictionary(uniqueKeysWithValues: localGraph.nodes.map { ($0.id, $0) })
        let remoteNodeMap = Dictionary(uniqueKeysWithValues: remoteGraph.nodes.map { ($0.id, $0) })
        
        for (nodeId, localNode) in localNodeMap {
            if let remoteNode = remoteNodeMap[nodeId] {
                if localNode.title != remoteNode.title ||
                   localNode.type != remoteNode.type ||
                   !CGPointEqualToPoint(localNode.position, remoteNode.position) {
                    conflicts.append(MergeConflict(
                        type: .nodeConflict,
                        nodeId: nodeId,
                        localData: localNode,
                        remoteData: remoteNode,
                        description: "Node properties differ between versions"
                    ))
                }
            }
        }
        
        // Find conflicting connections
        let localConnMap = Set(localGraph.connections.map { "\($0.fromNode)-\($0.fromPort)-\($0.toNode)-\($0.toPort)" })
        let remoteConnMap = Set(remoteGraph.connections.map { "\($0.fromNode)-\($0.fromPort)-\($0.toNode)-\($0.toPort)" })
        
        // Connections in local but not in remote (local additions)
        for conn in localGraph.connections {
            let connKey = "\(conn.fromNode)-\(conn.fromPort)-\(conn.toNode)-\(conn.toPort)"
            if !remoteConnMap.contains(connKey) {
                conflicts.append(MergeConflict(
                    type: .connectionAdded,
                    nodeId: conn.fromNode,
                    localData: conn,
                    remoteData: nil,
                    description: "Connection added in local version"
                ))
            }
        }
        
        // Connections in remote but not in local (remote additions)
        for conn in remoteGraph.connections {
            let connKey = "\(conn.fromNode)-\(conn.fromPort)-\(conn.toNode)-\(conn.toPort)"
            if !localConnMap.contains(connKey) {
                conflicts.append(MergeConflict(
                    type: .connectionAdded,
                    nodeId: conn.fromNode,
                    localData: nil,
                    remoteData: conn,
                    description: "Connection added in remote version"
                ))
            }
        }
        
        return conflicts
    }
    
    func resolveMergeConflicts(_ conflicts: [MergeConflict], resolution: MergeResolution) -> GraphData {
        // Start with current local graph
        var resolvedGraph = serializeGraph()
        
        for conflict in conflicts {
            switch resolution {
            case .acceptLocal:
                // Keep local version - no changes needed
                break
            case .acceptRemote:
                // Apply remote changes
                applyRemoteChange(conflict, to: &resolvedGraph)
            case .merge:
                // Attempt to merge both versions
                mergeChanges(conflict, to: &resolvedGraph)
            }
        }
        
        return resolvedGraph
    }
    
    private func applyRemoteChange(_ conflict: MergeConflict, to graph: inout GraphData) {
        switch conflict.type {
        case .nodeConflict:
            if let remoteNode = conflict.remoteData as? GraphNodeData {
                // Replace local node with remote version
                if let index = graph.nodes.firstIndex(where: { $0.id == conflict.nodeId }) {
                    graph.nodes[index] = remoteNode
                }
            }
        case .connectionAdded:
            if let remoteConn = conflict.remoteData as? GraphConnectionData {
                // Add remote connection
                graph.connections.append(remoteConn)
            }
        case .nodeDeleted:
            if let remoteNodeId = conflict.remoteData as? UUID {
                // Remove node
                graph.nodes.removeAll { $0.id == remoteNodeId }
                // Remove related connections
                graph.connections.removeAll { $0.fromNode == remoteNodeId || $0.toNode == remoteNodeId }
            }
        }
    }
    
    private func mergeChanges(_ conflict: MergeConflict, to graph: inout GraphData) {
        switch conflict.type {
        case .nodeConflict:
            if let localNode = conflict.localData as? GraphNodeData,
               let remoteNode = conflict.remoteData as? GraphNodeData {
                // Merge positions (take average)
                let mergedPosition = CGPoint(
                    x: (localNode.position.x + remoteNode.position.x) / 2,
                    y: (localNode.position.y + remoteNode.position.y) / 2
                )
                
                // Update local node with merged position
                if let index = graph.nodes.firstIndex(where: { $0.id == conflict.nodeId }) {
                    var mergedNode = localNode
                    mergedNode.position = mergedPosition
                    graph.nodes[index] = mergedNode
                }
            }
        case .connectionAdded:
            // For connections, just add both if they don't conflict
            if let remoteConn = conflict.remoteData as? GraphConnectionData {
                if !graph.connections.contains(where: { 
                    $0.fromNode == remoteConn.fromNode && 
                    $0.fromPort == remoteConn.fromPort &&
                    $0.toNode == remoteConn.toNode && 
                    $0.toPort == remoteConn.toPort 
                }) {
                    graph.connections.append(remoteConn)
                }
            }
        default:
            break
        }
    }
    
    func generateMergeReport(_ conflicts: [MergeConflict]) -> String {
        var report = "# Merge Conflict Report\n\n"
        report += "Found \(conflicts.count) conflicts:\n\n"
        
        for (index, conflict) in conflicts.enumerated() {
            report += "## \(index + 1). \(conflict.type.rawValue)\n"
            report += "Node ID: \(conflict.nodeId.uuidString.prefix(8))\n"
            report += "Description: \(conflict.description)\n\n"
        }
        
        return report
    }
    
    // MARK: - Canvas Functionality
    
    func snapToGrid(_ point: CGPoint) -> CGPoint {
        guard gridSnapEnabled else { return point }
        
        let snappedX = round(point.x / gridSize) * gridSize
        let snappedY = round(point.y / gridSize) * gridSize
        
        return CGPoint(x: snappedX, y: snappedY)
    }
    
    func zoomToFitAllNodes() {
        guard !nodes.isEmpty else { return }
        
        let minX = nodes.map { $0.position.x }.min() ?? 0
        let maxX = nodes.map { $0.position.x + 200 }.max() ?? 800 // Approximate node width
        let minY = nodes.map { $0.position.y }.min() ?? 0
        let maxY = nodes.map { $0.position.y + 100 }.max() ?? 600 // Approximate node height
        
        let contentWidth = maxX - minX
        let contentHeight = maxY - minY
        let viewWidth = 800.0 // Canvas view width
        let viewHeight = 600.0 // Canvas view height
        
        let scaleX = viewWidth / contentWidth
        let scaleY = viewHeight / contentHeight
        let targetZoom = min(scaleX, scaleY) * 0.8 // Add some padding
        
        zoom = max(0.1, min(targetZoom, 3.0))
        
        // Center the content
        let contentCenterX = (minX + maxX) / 2
        let contentCenterY = (minY + maxY) / 2
        
        panOffset = CGPoint(
            x: viewWidth / 2 - contentCenterX * zoom,
            y: viewHeight / 2 - contentCenterY * zoom
        )
    }
    
    func zoomToSelection() {
        guard !selectedNodes.isEmpty else { 
            zoomToFitAllNodes()
            return 
        }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        guard !selectedNodeObjects.isEmpty else { return }
        
        let minX = selectedNodeObjects.map { $0.position.x }.min() ?? 0
        let maxX = selectedNodeObjects.map { $0.position.x + 200 }.max() ?? 800
        let minY = selectedNodeObjects.map { $0.position.y }.min() ?? 0
        let maxY = selectedNodeObjects.map { $0.position.y + 100 }.max() ?? 600
        
        let contentWidth = maxX - minX
        let contentHeight = maxY - minY
        let viewWidth = 800.0
        let viewHeight = 600.0
        
        let scaleX = viewWidth / contentWidth
        let scaleY = viewHeight / contentHeight
        let targetZoom = min(scaleX, scaleY) * 0.8
        
        zoom = max(0.1, min(targetZoom, 3.0))
        
        let contentCenterX = (minX + maxX) / 2
        let contentCenterY = (minY + maxY) / 2
        
        panOffset = CGPoint(
            x: viewWidth / 2 - contentCenterX * zoom,
            y: viewHeight / 2 - contentHeight / 2 * zoom
        )
    }
    
    func focusOnNode(_ nodeId: UUID) {
        guard let node = nodes.first(where: { $0.id == nodeId }) else { return }
        
        selectedNodes = [nodeId]
        
        let viewWidth = 800.0
        let viewHeight = 600.0
        let nodeWidth = 200.0
        let nodeHeight = 100.0
        
        zoom = 1.0
        
        panOffset = CGPoint(
            x: viewWidth / 2 - (node.position.x + nodeWidth / 2) * zoom,
            y: viewHeight / 2 - (node.position.y + nodeHeight / 2) * zoom
        )
    }
    
    func selectAllConnected() {
        guard !selectedNodes.isEmpty else { return }
        
        var connectedNodes: Set<UUID> = selectedNodes
        var toCheck = Array(selectedNodes)
        
        while !toCheck.isEmpty {
            let nodeId = toCheck.removeFirst()
            
            // Find all connections to/from this node
            let connectedFrom = connections.filter { $0.fromNode == nodeId }.map { $0.toNode }
            let connectedTo = connections.filter { $0.toNode == nodeId }.map { $0.fromNode }
            
            for connectedId in connectedFrom + connectedTo {
                if !connectedNodes.contains(connectedId) {
                    connectedNodes.insert(connectedId)
                    toCheck.append(connectedId)
                }
            }
        }
        
        selectedNodes = connectedNodes
    }
    
    func selectDownstream() {
        guard !selectedNodes.isEmpty else { return }
        
        var downstreamNodes: Set<UUID> = []
        var toCheck = Array(selectedNodes)
        
        while !toCheck.isEmpty {
            let nodeId = toCheck.removeFirst()
            
            // Find nodes that receive input from this node
            let connectedTo = connections.filter { $0.fromNode == nodeId }.map { $0.toNode }
            
            for connectedId in connectedTo {
                if !downstreamNodes.contains(connectedId) {
                    downstreamNodes.insert(connectedId)
                    toCheck.append(connectedId)
                }
            }
        }
        
        selectedNodes = selectedNodes.union(downstreamNodes)
    }
    
    func selectUpstream() {
        guard !selectedNodes.isEmpty else { return }
        
        var upstreamNodes: Set<UUID> = []
        var toCheck = Array(selectedNodes)
        
        while !toCheck.isEmpty {
            let nodeId = toCheck.removeFirst()
            
            // Find nodes that send output to this node
            let connectedFrom = connections.filter { $0.toNode == nodeId }.map { $0.fromNode }
            
            for connectedId in connectedFrom {
                if !upstreamNodes.contains(connectedId) {
                    upstreamNodes.insert(connectedId)
                    toCheck.append(connectedId)
                }
            }
        }
        
        selectedNodes = selectedNodes.union(upstreamNodes)
    }
    
    func addBookmark(name: String) {
        let bookmark = CanvasBookmark(
            name: name,
            position: panOffset,
            zoom: zoom,
            timestamp: Date()
        )
        bookmarks.append(bookmark)
    }
    
    func loadBookmark(_ bookmark: CanvasBookmark) {
        panOffset = bookmark.position
        zoom = bookmark.zoom
    }
    
    func deleteBookmark(_ bookmark: CanvasBookmark) {
        bookmarks.removeAll { $0.id == bookmark.id }
    }
    
    func addCommentBox(title: String, content: String, position: CGPoint, size: CGSize) {
        let commentBox = CommentBox(
            title: title,
            content: content,
            position: position,
            size: size
        )
        commentBoxes.append(commentBox)
    }
    
    func deleteCommentBox(_ commentBox: CommentBox) {
        commentBoxes.removeAll { $0.id == commentBox.id }
    }
    
    func updateSearchQuery(_ query: String) {
        searchQuery = query.lowercased()
        
        if searchQuery.isEmpty {
            filteredNodes.removeAll()
        } else {
            filteredNodes = nodes.filter { node in
                node.title.lowercased().contains(searchQuery) ||
                node.type.rawValue.lowercased().contains(searchQuery) ||
                node.comment.lowercased().contains(searchQuery)
            }.map { $0.id }
        }
    }
    
    func alignNodesLeft() {
        guard selectedNodes.count > 1 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        guard let leftmostNode = selectedNodeObjects.min(by: { $0.position.x < $1.position.x }) else { return }
        
        for node in selectedNodeObjects {
            if node.id != leftmostNode.id {
                node.position.x = leftmostNode.position.x
            }
        }
        markDirty()
    }
    
    func alignNodesCenter() {
        guard selectedNodes.count > 1 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        let centerX = selectedNodeObjects.reduce(0) { $0 + $1.position.x + 100 } / CGFloat(selectedNodeObjects.count)
        
        for node in selectedNodeObjects {
            node.position.x = centerX - 100
        }
        markDirty()
    }
    
    func alignNodesRight() {
        guard selectedNodes.count > 1 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        guard let rightmostNode = selectedNodeObjects.max(by: { $0.position.x < $1.position.x }) else { return }
        
        for node in selectedNodeObjects {
            if node.id != rightmostNode.id {
                node.position.x = rightmostNode.position.x
            }
        }
        markDirty()
    }
    
    func alignNodesTop() {
        guard selectedNodes.count > 1 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        guard let topmostNode = selectedNodeObjects.min(by: { $0.position.y < $1.position.y }) else { return }
        
        for node in selectedNodeObjects {
            if node.id != topmostNode.id {
                node.position.y = topmostNode.position.y
            }
        }
        markDirty()
    }
    
    func alignNodesMiddle() {
        guard selectedNodes.count > 1 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        let centerY = selectedNodeObjects.reduce(0) { $0 + $1.position.y + 50 } / CGFloat(selectedNodeObjects.count)
        
        for node in selectedNodeObjects {
            node.position.y = centerY - 50
        }
        markDirty()
    }
    
    func alignNodesBottom() {
        guard selectedNodes.count > 1 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        guard let bottommostNode = selectedNodeObjects.max(by: { $0.position.y < $1.position.y }) else { return }
        
        for node in selectedNodeObjects {
            if node.id != bottommostNode.id {
                node.position.y = bottommostNode.position.y
            }
        }
        markDirty()
    }
    
    func distributeNodesHorizontally() {
        guard selectedNodes.count > 2 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        let sortedNodes = selectedNodeObjects.sorted { $0.position.x < $1.position.x }
        
        let leftmostX = sortedNodes.first!.position.x
        let rightmostX = sortedNodes.last!.position.x
        let spacing = (rightmostX - leftmostX) / CGFloat(sortedNodes.count - 1)
        
        for (index, node) in sortedNodes.enumerated() {
            node.position.x = leftmostX + spacing * CGFloat(index)
        }
        markDirty()
    }
    
    func distributeNodesVertically() {
        guard selectedNodes.count > 2 else { return }
        
        let selectedNodeObjects = nodes.filter { selectedNodes.contains($0.id) }
        let sortedNodes = selectedNodeObjects.sorted { $0.position.y < $1.position.y }
        
        let topmostY = sortedNodes.first!.position.y
        let bottommostY = sortedNodes.last!.position.y
        let spacing = (bottommostY - topmostY) / CGFloat(sortedNodes.count - 1)
        
        for (index, node) in sortedNodes.enumerated() {
            node.position.y = topmostY + spacing * CGFloat(index)
        }
        markDirty()
    }
    
    func autoLayout() {
        // Simple force-directed layout algorithm
        var nodePositions: [UUID: CGPoint] = [:]
        
        // Initialize positions
        for node in nodes {
            nodePositions[node.id] = node.position
        }
        
        // Apply forces for several iterations
        for _ in 0..<50 {
            var forces: [UUID: CGPoint] = [:]
            
            // Initialize forces
            for node in nodes {
                forces[node.id] = .zero
            }
            
            // Repulsive forces between all nodes
            for node1 in nodes {
                for node2 in nodes {
                    if node1.id != node2.id {
                        let pos1 = nodePositions[node1.id]!
                        let pos2 = nodePositions[node2.id]!
                        let diff = CGPoint(x: pos1.x - pos2.x, y: pos1.y - pos2.y)
                        let distance = sqrt(diff.x * diff.x + diff.y * diff.y)
                        
                        if distance > 0 && distance < 200 {
                            let force = 1000 / (distance * distance)
                            let forceVector = CGPoint(x: diff.x / distance * force, y: diff.y / distance * force)
                            forces[node1.id] = CGPoint(
                                x: forces[node1.id]!.x + forceVector.x,
                                y: forces[node1.id]!.y + forceVector.y
                            )
                        }
                    }
                }
            }
            
            // Attractive forces along connections
            for connection in connections {
                if let pos1 = nodePositions[connection.fromNode],
                   let pos2 = nodePositions[connection.toNode] {
                    let diff = CGPoint(x: pos2.x - pos1.x, y: pos2.y - pos1.y)
                    let distance = sqrt(diff.x * diff.x + diff.y * diff.y)
                    
                    if distance > 150 {
                        let force = (distance - 150) * 0.01
                        let forceVector = CGPoint(x: diff.x / distance * force, y: diff.y / distance * force)
                        
                        forces[connection.fromNode] = CGPoint(
                            x: forces[connection.fromNode]!.x + forceVector.x,
                            y: forces[connection.fromNode]!.y + forceVector.y
                        )
                        forces[connection.toNode] = CGPoint(
                            x: forces[connection.toNode]!.x - forceVector.x,
                            y: forces[connection.toNode]!.y - forceVector.y
                        )
                    }
                }
            }
            
            // Apply forces
            for node in nodes {
                let currentPos = nodePositions[node.id]!
                let force = forces[node.id]!
                let newPos = CGPoint(
                    x: currentPos.x + force.x,
                    y: currentPos.y + force.y
                )
                nodePositions[node.id] = newPos
            }
        }
        
        // Update node positions
        for node in nodes {
            node.position = nodePositions[node.id]!
        }
        markDirty()
    }
    
    func straightenConnections() {
        // Simple connection straightening - remove reroute points
        for connection in connections {
            connection.reroutePoints.removeAll()
        }
        markDirty()
    }
    
    func startBoxSelection(at point: CGPoint) {
        isBoxSelecting = true
        boxSelectionStart = point
        boxSelectionEnd = point
    }
    
    func updateBoxSelection(at point: CGPoint) {
        boxSelectionEnd = point
    }
    
    func endBoxSelection() {
        guard isBoxSelecting,
              let start = boxSelectionStart,
              let end = boxSelectionEnd else {
            isBoxSelecting = false
            return
        }
        
        let selectionRect = CGRect(
            x: min(start.x, end.x),
            y: min(start.y, end.y),
            width: abs(end.x - start.x),
            height: abs(end.y - start.y)
        )
        
        let nodesInSelection = nodes.filter { node in
            let nodeRect = CGRect(
                x: node.position.x,
                y: node.position.y,
                width: 200,
                height: 100
            )
            return selectionRect.intersects(nodeRect)
        }
        
        let nodeIdsInSelection = Set(nodesInSelection.map { $0.id })
        
        // Toggle selection for nodes in the box
        for nodeId in nodeIdsInSelection {
            if selectedNodes.contains(nodeId) {
                selectedNodes.remove(nodeId)
            } else {
                selectedNodes.insert(nodeId)
            }
        }
        
        isBoxSelecting = false
        boxSelectionStart = nil
        boxSelectionEnd = nil
    }
    
    func toggleShiftClickSelection(_ nodeId: UUID) {
        if selectedNodes.contains(nodeId) {
            selectedNodes.remove(nodeId)
        } else {
            selectedNodes.insert(nodeId)
        }
    }
}

// MARK: - Merge Conflict Data Structures

struct MergeConflict {
    let type: ConflictType
    let nodeId: UUID
    let localData: Any?
    let remoteData: Any?
    let description: String
}

enum ConflictType: String {
    case nodeConflict = "Node Conflict"
    case connectionAdded = "Connection Added"
    case connectionRemoved = "Connection Removed"
    case nodeDeleted = "Node Deleted"
    case nodeAdded = "Node Added"
}

enum MergeResolution {
    case acceptLocal
    case acceptRemote
    case merge
}

// MARK: - Graph Diffing
    
func generateGraphDiff(baseGraph: GraphData, modifiedGraph: GraphData) -> GraphDiff {
    var addedNodes: [GraphNodeData] = []
    var removedNodes: [GraphNodeData] = []
    var modifiedNodes: [NodeModification] = []
    var addedConnections: [GraphConnectionData] = []
    var removedConnections: [GraphConnectionData] = []
    
    // Find added nodes
    let baseNodeIds = Set(baseGraph.nodes.map { $0.id })
    let modifiedNodeIds = Set(modifiedGraph.nodes.map { $0.id })
    
    addedNodes = modifiedGraph.nodes.filter { !baseNodeIds.contains($0.id) }
    removedNodes = baseGraph.nodes.filter { !modifiedNodeIds.contains($0.id) }
    
    // Find modified nodes
    for modifiedNode in modifiedGraph.nodes {
        if let baseNode = baseGraph.nodes.first(where: { $0.id == modifiedNode.id }) {
            let modifications = compareNodes(base: baseNode, modified: modifiedNode)
            if !modifications.isEmpty {
                modifiedNodes.append(NodeModification(
                    nodeId: modifiedNode.id,
                    modifications: modifications
                ))
            }
        }
    }
    
    // Find connection changes
    let baseConnKeys = Set(baseGraph.connections.map { connKey($0) })
    let modifiedConnKeys = Set(modifiedGraph.connections.map { connKey($0) })
    
    addedConnections = modifiedGraph.connections.filter { !baseConnKeys.contains(connKey($0)) }
    removedConnections = baseGraph.connections.filter { !modifiedConnKeys.contains(connKey($0)) }
    
    return GraphDiff(
        addedNodes: addedNodes,
        removedNodes: removedNodes,
        modifiedNodes: modifiedNodes,
        addedConnections: addedConnections,
        removedConnections: removedConnections
    )
}

private func connKey(_ conn: GraphConnectionData) -> String {
    return "\(conn.fromNode)-\(conn.fromPort)-\(conn.toNode)-\(conn.toPort)"
}

private func compareNodes(base: GraphNodeData, modified: GraphNodeData) -> [NodePropertyChange] {
    var changes: [NodePropertyChange] = []
    
    if base.title != modified.title {
        changes.append(NodePropertyChange(
            property: "title",
            oldValue: base.title,
            newValue: modified.title
        ))
    }
    
    if base.type != modified.type {
        changes.append(NodePropertyChange(
            property: "type",
            oldValue: base.type,
            newValue: modified.type
        ))
    }
    
    if !CGPointEqualToPoint(base.position, modified.position) {
        changes.append(NodePropertyChange(
            property: "position",
            oldValue: "\(base.position.x),\(base.position.y)",
            newValue: "\(modified.position.x),\(modified.position.y)"
        ))
    }
    
    if base.isCollapsed != modified.isCollapsed {
        changes.append(NodePropertyChange(
            property: "collapsed",
            oldValue: "\(base.isCollapsed)",
            newValue: "\(modified.isCollapsed)"
        ))
    }
    
    if base.comment != modified.comment {
        changes.append(NodePropertyChange(
            property: "comment",
            oldValue: base.comment,
            newValue: modified.comment
        ))
    }
    
    return changes
}

func applyGraphDiff(_ diff: GraphDiff, to graph: inout GraphData) {
    // Remove nodes
    graph.nodes.removeAll { removedNode in
        diff.removedNodes.contains { $0.id == removedNode.id }
    }
    
    // Add nodes
    graph.nodes.append(contentsOf: diff.addedNodes)
    
    // Apply node modifications
    for modification in diff.modifiedNodes {
        if let index = graph.nodes.firstIndex(where: { $0.id == modification.nodeId }) {
            var node = graph.nodes[index]
            
            for change in modification.modifications {
                switch change.property {
                case "title":
                    node.title = change.newValue
                case "type":
                    node.type = change.newValue
                case "position":
                    let coords = change.newValue.split(separator: ",").compactMap { Float($0) }
                    if coords.count == 2 {
                        node.position = CGPoint(x: CGFloat(coords[0]), y: CGFloat(coords[1]))
                    }
                case "collapsed":
                    node.isCollapsed = Bool(change.newValue) ?? false
                case "comment":
                    node.comment = change.newValue
                default:
                    break
                }
            }
            
            graph.nodes[index] = node
        }
    }
    
    // Remove connections
    graph.nodes.removeAll { removedConn in
        diff.removedConnections.contains { 
            connKey($0) == connKey(GraphConnectionData(
                id: removedConn.id,
                fromNode: removedConn.fromNode,
                fromPort: removedConn.fromPort,
                toNode: removedConn.toNode,
                toPort: removedConn.toPort
            ))
        }
    }
    
    // Add connections
    graph.connections.append(contentsOf: diff.addedConnections)
}

func generateDiffReport(_ diff: GraphDiff) -> String {
    var report = "# Graph Diff Report\n\n"
    
    if !diff.addedNodes.isEmpty {
        report += "## Added Nodes (\(diff.addedNodes.count))\n"
        for node in diff.addedNodes {
            report += "- \(node.title) (\(node.type))\n"
        }
        report += "\n"
    }
    
    if !diff.removedNodes.isEmpty {
        report += "## Removed Nodes (\(diff.removedNodes.count))\n"
        for node in diff.removedNodes {
            report += "- \(node.title) (\(node.type))\n"
        }
        report += "\n"
    }
    
    if !diff.modifiedNodes.isEmpty {
        report += "## Modified Nodes (\(diff.modifiedNodes.count))\n"
        for modification in diff.modifiedNodes {
            let node = diff.addedNodes.first { $0.id == modification.nodeId } ?? 
                      diff.removedNodes.first { $0.id == modification.nodeId } ??
                      GraphNodeData(id: modification.nodeId, type: "unknown", title: "Unknown", position: .zero, inputs: [], outputs: [], isCollapsed: false, comment: "")
            
            report += "- \(node.title):\n"
            for change in modification.modifications {
                report += "  - \(change.property): \(change.oldValue) → \(change.newValue)\n"
            }
        }
        report += "\n"
    }
    
    if !diff.addedConnections.isEmpty {
        report += "## Added Connections (\(diff.addedConnections.count))\n"
        for conn in diff.addedConnections {
            report += "- \(conn.fromNode.uuidString.prefix(8)) → \(conn.toNode.uuidString.prefix(8))\n"
        }
        report += "\n"
    }
    
    if !diff.removedConnections.isEmpty {
        report += "## Removed Connections (\(diff.removedConnections.count))\n"
        for conn in diff.removedConnections {
            report += "- \(conn.fromNode.uuidString.prefix(8)) → \(conn.toNode.uuidString.prefix(8))\n"
        }
        report += "\n"
    }
    
    return report
}

// MARK: - Graph Diff Data Structures

struct GraphDiff {
    let addedNodes: [GraphNodeData]
    let removedNodes: [GraphNodeData]
    let modifiedNodes: [NodeModification]
    let addedConnections: [GraphConnectionData]
    let removedConnections: [GraphConnectionData]
}

struct NodeModification {
    let nodeId: UUID
    let modifications: [NodePropertyChange]
}

struct NodePropertyChange {
    let property: String
    let oldValue: String
    let newValue: String
}
}

// MARK: - Node Models
struct GraphNode: Identifiable {
    var id = UUID()
    var type: NodeType
    var title: String
    var position: CGPoint
    var inputs: [NodePort] = []
    var outputs: [NodePort] = []
    var isCollapsed: Bool = false
    var isHeaderCollapsed: Bool = false
    var isDisabled: Bool = false
    var isBypassed: Bool = false
    var comment: String = ""
    
    // Custom appearance
    var customColor: Color? = nil
    var customIcon: String? = nil
    var customSize: CGSize? = nil
    
    // Preview and debugging
    var previewValue: Any? = nil
    var outputValues: [UUID: Any] = [:]
    var hasError: Bool = false
    var errorMessage: String? = nil
    var hasWarning: Bool = false
    var warningMessage: String? = nil
    
    // Documentation
    var description: String = ""
    var documentation: String = ""
    
    // Size constraints
    var minWidth: CGFloat = 150
    var minHeight: CGFloat = 80
    var autoSize: Bool = true
    
    // Inline editing
    var isRenaming: Bool = false
    var tempTitle: String = ""
}

struct NodePort: Identifiable {
    let id = UUID()
    var name: String
    var type: PortType
    var defaultValue: Any? = nil
    var currentValue: Any? = nil
    var isEditable: Bool = true
    var hasError: Bool = false
    var errorMessage: String? = nil
    var description: String = ""
}

// MARK: - Connection System

enum ConnectionStyle: String, CaseIterable {
    case bezier = "Bezier"
    case straight = "Straight"
    case orthogonal = "Orthogonal"
}

enum ConnectionThickness: String, CaseIterable {
    case thin = "Thin"
    case medium = "Medium"
    case thick = "Thick"
    
    var value: CGFloat {
        switch self {
        case .thin: return 2.0
        case .medium: return 3.0
        case .thick: return 4.0
        }
    }
}

struct ConnectionReroutePoint: Identifiable {
    let id = UUID()
    var position: CGPoint
}

struct GraphConnection: Identifiable {
    let id = UUID()
    var fromNode: UUID
    var fromPort: Int
    var toNode: UUID
    var toPort: Int
    var reroutePoints: [ConnectionReroutePoint] = []
    var isHidden: Bool = false
    var executionOrder: Int? = nil
    var isValid: Bool = true
    var validationError: String? = nil
}

// MARK: - Canvas Data Structures

enum GridPattern: String, CaseIterable {
    case dots = "dots"
    case lines = "lines"
    case cross = "cross"
    case none = "none"
}

struct CanvasBookmark: Identifiable, Codable {
    let id = UUID()
    var name: String
    var position: CGPoint
    var zoom: CGFloat
    var timestamp: Date
}

struct CommentBox: Identifiable, Codable {
    let id = UUID()
    var title: String
    var content: String
    var position: CGPoint
    var size: CGSize
    var color: Color
    var nodeIds: Set<UUID> = []
    
    init(title: String, content: String, position: CGPoint, size: CGSize, color: Color = .yellow.opacity(0.2)) {
        self.title = title
        self.content = content
        self.position = position
        self.size = size
        self.color = color
    }
}

// MARK: - Connection Rendering System

struct ConnectionRenderer: View {
    let connection: GraphConnection
    let startPoint: CGPoint
    let endPoint: CGPoint
    let style: ConnectionStyle
    let thickness: ConnectionThickness
    let portType: PortType
    let isHighlighted: Bool
    let isSelected: Bool
    let showExecutionOrder: Bool
    let animationProgress: Double
    
    var body: some View {
        ZStack {
            // Connection path
            Path { path in
                switch style {
                case .bezier:
                    drawBezierCurve(path: path, from: startPoint, to: endPoint)
                case .straight:
                    drawStraightLine(path: path, from: startPoint, to: endPoint)
                case .orthogonal:
                    drawOrthogonalLine(path: path, from: startPoint, to: endPoint)
                }
            }
            .stroke(
                connectionColor(for: portType, isValid: connection.isValid),
                style: StrokeStyle(
                    lineWidth: thickness.value,
                    lineCap: .round,
                    lineJoin: .round
                )
            )
            .opacity(isHighlighted ? 1.0 : (connection.isHidden ? 0.2 : 0.8))
            .overlay(
                // Selection highlight
                isSelected ? 
                    Path { path in
                        switch style {
                        case .bezier:
                            drawBezierCurve(path: path, from: startPoint, to: endPoint)
                        case .straight:
                            drawStraightLine(path: path, from: startPoint, to: endPoint)
                        case .orthogonal:
                            drawOrthogonalLine(path: path, from: startPoint, to: endPoint)
                        }
                    }
                    .stroke(Color.blue, style: StrokeStyle(lineWidth: thickness.value + 2))
                    .opacity(0.5) : nil
            )
            
            // Flow animation
            if animationProgress > 0 {
                Path { path in
                    switch style {
                    case .bezier:
                        drawBezierCurve(path: path, from: startPoint, to: endPoint)
                    case .straight:
                        drawStraightLine(path: path, from: startPoint, to: endPoint)
                    case .orthogonal:
                        drawOrthogonalLine(path: path, from: startPoint, to: endPoint)
                    }
                }
                .trim(from: 0, to: animationProgress)
                .stroke(
                    Color.white.opacity(0.8),
                    style: StrokeStyle(
                        lineWidth: thickness.value / 2,
                        lineCap: .round
                    )
                )
            }
            
            // Execution order number
            if showExecutionOrder, let order = connection.executionOrder {
                Text("\(order)")
                    .font(.caption2)
                    .foregroundColor(.white)
                    .padding(2)
                    .background(Color.blue.opacity(0.8))
                    .cornerRadius(4)
                    .position(midPoint(from: startPoint, to: endPoint))
            }
            
            // Validation error indicator
            if !connection.isValid, let error = connection.validationError {
                Image(systemName: "exclamationmark.triangle.fill")
                    .foregroundColor(.orange)
                    .font(.caption)
                    .position(midPoint(from: startPoint, to: endPoint))
                    .help(error)
            }
        }
    }
    
    private func drawBezierCurve(path: inout Path, from: CGPoint, to: CGPoint) {
        path.move(to: from)
        
        let controlOffset = min(abs(to.x - from.x) * 0.5, 100)
        let controlPoint1 = CGPoint(x: from.x + controlOffset, y: from.y)
        let controlPoint2 = CGPoint(x: to.x - controlOffset, y: to.y)
        
        path.addCurve(to: to, control1: controlPoint1, control2: controlPoint2)
    }
    
    private func drawStraightLine(path: inout Path, from: CGPoint, to: CGPoint) {
        path.move(to: from)
        path.addLine(to: to)
    }
    
    private func drawOrthogonalLine(path: inout Path, from: CGPoint, to: CGPoint) {
        path.move(to: from)
        
        let midX = (from.x + to.x) / 2
        let midY = (from.y + to.y) / 2
        
        // Choose the better orthogonal path
        if abs(to.x - from.x) > abs(to.y - from.y) {
            // Horizontal-first path
            path.addLine(to: CGPoint(x: midX, y: from.y))
            path.addLine(to: CGPoint(x: midX, y: to.y))
            path.addLine(to: to)
        } else {
            // Vertical-first path
            path.addLine(to: CGPoint(x: from.x, y: midY))
            path.addLine(to: CGPoint(x: to.x, y: midY))
            path.addLine(to: to)
        }
    }
    
    private func connectionColor(for type: PortType, isValid: Bool) -> Color {
        if !isValid {
            return .red.opacity(0.8)
        }
        
        switch type {
        case .flow: return .white
        case .bool: return .red
        case .int: return .cyan
        case .float: return .green
        case .vector3: return .yellow
        case .string: return .magenta
        case .object: return .blue
        case .any: return .gray
        case .array: return .orange
        case .dictionary: return .purple
        case .enumType: return .teal
        case .structType: return .indigo
        case .classType: return .purple
        case .transform: return .mint
        case .raycast: return .red
        case .audioClip: return .brown
        case .animationClip: return .cyan
        }
    }
    
    private func midPoint(from: CGPoint, to: CGPoint) -> CGPoint {
        return CGPoint(x: (from.x + to.x) / 2, y: (from.y + to.y) / 2)
    }
}
// MARK: - Connection Management System

class ConnectionManager: ObservableObject {
    @Published var connections: [GraphConnection] = []
    @Published var selectedConnections: Set<UUID> = []
    @Published var connectionStyle: ConnectionStyle = .bezier
    @Published var connectionThickness: ConnectionThickness = .medium
    @Published var showExecutionOrder: Bool = false
    @Published var animateFlow: Bool = false
    @Published var animationProgress: Double = 0.0
    
    private var animationTimer: Timer?
    
    init() {
        startFlowAnimation()
    }
    
    deinit {
        stopFlowAnimation()
    }
    
    // MARK: - Connection Creation and Validation
    
    func createConnection(fromNode: UUID, fromPort: Int, toNode: UUID, toPort: Int, portType: PortType) -> GraphConnection? {
        // Validate connection
        let validation = validateConnection(fromNode: fromNode, fromPort: fromPort, toNode: toNode, toPort: toPort)
        
        if !validation.isValid {
            print("Connection validation failed: \(validation.error ?? "Unknown error")")
            return nil
        }
        
        let connection = GraphConnection(
            fromNode: fromNode,
            fromPort: fromPort,
            toNode: toNode,
            toPort: toPort,
            isValid: validation.isValid,
            validationError: validation.error
        )
        
        connections.append(connection)
        updateExecutionOrder()
        
        return connection
    }
    
    func deleteConnection(_ connectionId: UUID) {
        connections.removeAll { $0.id == connectionId }
        selectedConnections.remove(connectionId)
        updateExecutionOrder()
    }
    
    func deleteSelectedConnections() {
        connections.removeAll { selectedConnections.contains($0.id) }
        selectedConnections.removeAll()
        updateExecutionOrder()
    }
    
    // MARK: - Connection Validation
    
    private func validateConnection(fromNode: UUID, fromPort: Int, toNode: UUID, toPort: Int) -> (isValid: Bool, error: String?) {
        // Check for self-connection
        if fromNode == toNode {
            return (false, "Cannot connect a node to itself")
        }
        
        // Check for existing connection to input port
        if let existingConnection = connections.first(where: { $0.toNode == toNode && $0.toPort == toPort }) {
            return (false, "Input port already has a connection")
        }
        
        // Check for loops
        if wouldCreateLoop(fromNode: fromNode, toNode: toNode) {
            return (false, "Connection would create a loop")
        }
        
        return (true, nil)
    }
    
    private func wouldCreateLoop(fromNode: UUID, toNode: UUID) -> Bool {
        var visited: Set<UUID> = []
        var stack: [UUID] = [toNode]
        
        while !stack.isEmpty {
            let current = stack.removeLast()
            
            if current == fromNode {
                return true // Loop detected
            }
            
            if visited.contains(current) {
                continue
            }
            
            visited.insert(current)
            
            // Add all nodes that current node connects to
            let outgoingConnections = connections.filter { $0.fromNode == current }
            stack.append(contentsOf: outgoingConnections.map { $0.toNode })
        }
        
        return false
    }
    
    // MARK: - Execution Order
    
    private func updateExecutionOrder() {
        var order = 1
        let orderedNodes = topologicalSort()
        
        for nodeId in orderedNodes {
            let nodeConnections = connections.filter { $0.fromNode == nodeId }
            for connection in nodeConnections {
                if let index = connections.firstIndex(where: { $0.id == connection.id }) {
                    connections[index].executionOrder = order
                    order += 1
                }
            }
        }
    }
    
    private func topologicalSort() -> [UUID] {
        var visited: Set<UUID> = []
        var result: [UUID] = []
        var inDegree: [UUID: Int] = [:]
        
        // Calculate in-degree for each node
        let allNodes = Set(connections.flatMap { [$0.fromNode, $0.toNode] })
        for node in allNodes {
            inDegree[node] = 0
        }
        
        for connection in connections {
            inDegree[connection.toNode, default: 0] += 1
        }
        
        // Queue of nodes with no incoming connections
        var queue: [UUID] = allNodes.filter { inDegree[$0] == 0 }
        
        while !queue.isEmpty {
            let current = queue.removeFirst()
            result.append(current)
            visited.insert(current)
            
            // Remove outgoing edges and update in-degree
            let outgoingConnections = connections.filter { $0.fromNode == current }
            for connection in outgoingConnections {
                inDegree[connection.toNode, default: 0] -= 1
                if inDegree[connection.toNode] == 0 {
                    queue.append(connection.toNode)
                }
            }
        }
        
        return result
    }
    
    // MARK: - Reroute Points
    
    func addReroutePoint(to connectionId: UUID, at position: CGPoint) {
        if let index = connections.firstIndex(where: { $0.id == connectionId }) {
            let reroutePoint = ConnectionReroutePoint(position: position)
            connections[index].reroutePoints.append(reroutePoint)
        }
    }
    
    func removeReroutePoint(from connectionId: UUID, reroutePointId: UUID) {
        if let index = connections.firstIndex(where: { $0.id == connectionId }) {
            connections[index].reroutePoints.removeAll { $0.id == reroutePointId }
        }
    }
    
    func updateReroutePointPosition(connectionId: UUID, reroutePointId: UUID, position: CGPoint) {
        if let connectionIndex = connections.firstIndex(where: { $0.id == connectionId }),
           let rerouteIndex = connections[connectionIndex].reroutePoints.firstIndex(where: { $0.id == reroutePointId }) {
            connections[connectionIndex].reroutePoints[rerouteIndex].position = position
        }
    }
    
    // MARK: - Connection Utilities
    
    func toggleConnectionVisibility(_ connectionId: UUID) {
        if let index = connections.firstIndex(where: { $0.id == connectionId }) {
            connections[index].isHidden.toggle()
        }
    }
    
    func selectConnection(_ connectionId: UUID) {
        selectedConnections.insert(connectionId)
    }
    
    func deselectConnection(_ connectionId: UUID) {
        selectedConnections.remove(connectionId)
    }
    
    func clearSelection() {
        selectedConnections.removeAll()
    }
    
    func selectAllConnections() {
        selectedConnections = Set(connections.map { $0.id })
    }
    
    // MARK: - Flow Animation
    
    private func startFlowAnimation() {
        guard animateFlow else { return }
        
        animationTimer = Timer.scheduledTimer(withTimeInterval: 0.016, repeats: true) { _ in
            withAnimation(.linear(duration: 0.016)) {
                self.animationProgress += 0.02
                if self.animationProgress > 1.0 {
                    self.animationProgress = 0.0
                }
            }
        }
    }
    
    private func stopFlowAnimation() {
        animationTimer?.invalidate()
        animationTimer = nil
    }
    
// MARK: - Temporary Connection System

struct TemporaryConnection {
    let fromNode: UUID
    let fromPort: Int
    let fromPosition: CGPoint
    var toPosition: CGPoint
    let portType: PortType
    var isValid: Bool = true
    var validationMessage: String? = nil
}

class ConnectionDragManager: ObservableObject {
    @Published var activeDrag: TemporaryConnection? = nil
    @Published var hoveredPort: (nodeId: UUID, portIndex: Int, isInput: Bool)? = nil
    @Published var snapEnabled: Bool = true
    @Published var snapDistance: CGFloat = 20.0
    
    private let connectionManager: ConnectionManager
    
    init(connectionManager: ConnectionManager) {
        self.connectionManager = connectionManager
    }
    
    // MARK: - Drag Operations
    
    func startDrag(fromNode: UUID, fromPort: Int, fromPosition: CGPoint, portType: PortType) {
        activeDrag = TemporaryConnection(
            fromNode: fromNode,
            fromPort: fromPort,
            fromPosition: fromPosition,
            toPosition: fromPosition,
            portType: portType
        )
    }
    
    func updateDrag(to position: CGPoint) {
        guard var drag = activeDrag else { return }
        
        // Check for port snapping
        if let snappedPort = findNearestPort(to: position) {
            drag.toPosition = snappedPort.position
            drag.isValid = validateDragConnection(drag, to: snappedPort)
            drag.validationMessage = drag.isValid ? nil : "Invalid connection"
        } else {
            drag.toPosition = position
            drag.isValid = true
            drag.validationMessage = nil
        }
        
        activeDrag = drag
    }
    
    func endDrag() -> GraphConnection? {
        guard let drag = activeDrag,
              let snappedPort = findNearestPort(to: drag.toPosition) else {
            activeDrag = nil
            return nil
        }
        
        let connection = connectionManager.createConnection(
            fromNode: drag.fromNode,
            fromPort: drag.fromPort,
            toNode: snappedPort.nodeId,
            toPort: snappedPort.portIndex,
            portType: drag.portType
        )
        
        activeDrag = nil
        return connection
    }
    
    func cancelDrag() {
        activeDrag = nil
    }
    
    // MARK: - Port Snapping
    
    private func findNearestPort(to position: CGPoint) -> (nodeId: UUID, portIndex: Int, position: CGPoint, isInput: Bool)? {
        guard snapEnabled else { return nil }
        
        var nearestPort: (nodeId: UUID, portIndex: Int, position: CGPoint, isInput: Bool)? = nil
        var minDistance: CGFloat = snapDistance
        
        // This would need access to the node graph to get actual port positions
        // For now, return nil - this would be implemented with the actual graph view
        return nearestPort
    }
    
    private func validateDragConnection(_ drag: TemporaryConnection, to port: (nodeId: UUID, portIndex: Int, position: CGPoint, isInput: Bool)) -> Bool {
        // Check for self-connection
        if drag.fromNode == port.nodeId {
            return false
        }
        
        // Check if it's an input port (can only connect to inputs)
        if !port.isInput {
            return false
        }
        
        // Check for existing connection to this input port
        if connectionManager.connections.contains(where: { $0.toNode == port.nodeId && $0.toPort == port.portIndex }) {
            return false
        }
        
        // Check for loops
        if wouldCreateLoop(fromNode: drag.fromNode, toNode: port.nodeId) {
            return false
        }
        
        return true
    }
    
    private func wouldCreateLoop(fromNode: UUID, toNode: UUID) -> Bool {
        var visited: Set<UUID> = []
        var stack: [UUID] = [toNode]
        
        while !stack.isEmpty {
            let current = stack.removeLast()
            
            if current == fromNode {
                return true // Loop detected
            }
            
            if visited.contains(current) {
                continue
            }
            
            visited.insert(current)
            
            // Add all nodes that current node connects to
            let outgoingConnections = connectionManager.connections.filter { $0.fromNode == current }
            stack.append(contentsOf: outgoingConnections.map { $0.toNode })
        }
        
        return false
    }
}

// MARK: - Connection Auto-Insert System

struct ConnectionInsertionPoint {
    let connectionId: UUID
    let position: CGPoint
    let t: CGFloat // Parameter along the connection (0.0 to 1.0)
}

class ConnectionAutoInsertManager: ObservableObject {
    private let connectionManager: ConnectionManager
    
    init(connectionManager: ConnectionManager) {
        self.connectionManager = connectionManager
    }
    
    func findInsertionPoint(at position: CGPoint, tolerance: CGFloat = 20.0) -> ConnectionInsertionPoint? {
        // Find the closest connection to the given position
        var closestConnection: ConnectionInsertionPoint? = nil
        var minDistance: CGFloat = tolerance
        
        for connection in connectionManager.connections {
            // This would need the actual connection path to calculate distance
            // For now, return nil - this would be implemented with the actual graph view
        }
        
        return closestConnection
    }
    
    func insertNode(at insertionPoint: ConnectionInsertionPoint, nodeType: NodeType) -> Bool {
        guard let connectionIndex = connectionManager.connections.firstIndex(where: { $0.id == insertionPoint.connectionId }) else {
            return false
        }
        
        let connection = connectionManager.connections[connectionIndex]
        
        // Remove the original connection
        connectionManager.deleteConnection(connection.id)
        
        // Create new node (this would be handled by the node graph manager)
        let newNodeId = UUID()
        
        // Create two new connections
        let connection1 = connectionManager.createConnection(
            fromNode: connection.fromNode,
            fromPort: connection.fromPort,
            toNode: newNodeId,
            toPort: 0, // Assuming first input port
            portType: .any // This would be determined by the node type
        )
        
        let connection2 = connectionManager.createConnection(
            fromNode: newNodeId,
            fromPort: 0, // Assuming first output port
            toNode: connection.toNode,
            toPort: connection.toPort,
            portType: .any // This would be determined by the node type
        )
        
        return connection1 != nil && connection2 != nil
    }
}

// MARK: - Connection Merge System

class ConnectionMergeManager: ObservableObject {
    private let connectionManager: ConnectionManager
    
    init(connectionManager: ConnectionManager) {
        self.connectionManager = connectionManager
    }
    
    func canMergeConnections(_ connections: [UUID]) -> Bool {
        guard connections.count >= 2 else { return false }
        
        let actualConnections = connections.compactMap { id in
            connectionManager.connections.first { $0.id == id }
        }
        
        // Check if all connections have the same target node and port
        guard let firstConnection = actualConnections.first else { return false }
        
        return actualConnections.allSatisfy { connection in
            connection.toNode == firstConnection.toNode &&
            connection.toPort == firstConnection.toPort &&
            connection.fromNode != firstConnection.fromNode
        }
    }
    
    func mergeConnections(_ connections: [UUID], using nodeType: NodeType) -> Bool {
        guard canMergeConnections(connections) else { return false }
        
        let actualConnections = connections.compactMap { id in
            connectionManager.connections.first { $0.id == id }
        }
        
        guard let firstConnection = actualConnections.first else { return false }
        
        // Delete original connections
        for connectionId in connections {
            connectionManager.deleteConnection(connectionId)
        }
        
        // Create merge node
        let mergeNodeId = UUID()
        
        // Create connections from all source nodes to merge node
        for (index, connection) in actualConnections.enumerated() {
            let _ = connectionManager.createConnection(
                fromNode: connection.fromNode,
                fromPort: connection.fromPort,
                toNode: mergeNodeId,
                toPort: index, // Each input gets its own port
                portType: .any
            )
        }
        
        // Create connection from merge node to original target
        let _ = connectionManager.createConnection(
            fromNode: mergeNodeId,
            fromPort: 0, // First output port
    let id = UUID()
    var name: String
    var description: String
    var parameters: [FunctionParameter]
    var returnValue: FunctionReturnValue?
    var localVariables: [FunctionVariable]
    var nodes: [GraphNode]
    var connections: [GraphConnection]
    var isPure: Bool = false
    var isLatent: Bool = false
    var documentation: String = ""
    var category: String = "Custom"
    var created: Date = Date()
    var modified: Date = Date()
    
    // Recursion support
    var allowsRecursion: Bool = false
    var maxRecursionDepth: Int = 100
    var recursionWarningDepth: Int = 50
    
    init(name: String, description: String = "") {
        self.name = name
        self.description = description
        self.parameters = []
        self.returnValue = nil
        self.localVariables = []
        self.nodes = []
        self.connections = []
    }
    
    // Check if function has recursive calls
    func hasRecursiveCalls() -> Bool {
        // Look for function call nodes that reference this function
        return nodes.contains { node in
            node.type == .function && node.title == self.name
        }
    }
    
    // Get recursion depth from current call stack
    func getRecursionDepth(callStack: [String]) -> Int {
        return callStack.filter { $0 == self.name }.count
    }
    
    // Check if recursion limit would be exceeded
    func wouldExceedRecursionLimit(callStack: [String]) -> Bool {
        let currentDepth = getRecursionDepth(callStack: callStack)
        return currentDepth >= maxRecursionDepth
    }
    
    // Check if recursion warning should be shown
    func shouldShowRecursionWarning(callStack: [String]) -> Bool {
        let currentDepth = getRecursionDepth(callStack: callStack)
        return currentDepth >= recursionWarningDepth
    }
}

struct FunctionParameter: Identifiable, Codable {
    let id = UUID()
    var name: String
    var type: PortType
    var defaultValue: Any?
    var isOptional: Bool = false
    var description: String = ""
}

struct FunctionReturnValue: Identifiable, Codable {
    let id = UUID()
    var type: PortType
    var description: String = ""
}

struct FunctionVariable: Identifiable, Codable {
    let id = UUID()
    var name: String
    var type: PortType
    var defaultValue: Any?
    var scope: VariableScope = .local
    var description: String = ""
}

enum VariableScope: String, CaseIterable, Codable {
    case local = "Local"
    case static = "Static"
    case instance = "Instance"
}

// MARK: - Recursion Support
struct RecursionWarning: Identifiable {
    let id = UUID()
    let functionName: String
    let type: RecursionWarningType
    let message: String
    let severity: WarningSeverity
}

enum RecursionWarningType {
    case unallowedRecursion
    case potentialInfiniteRecursion
    case depthLimitExceeded
    case performanceIssue
    
    var color: Color {
        switch self {
        case .unallowedRecursion: return .red
        case .potentialInfiniteRecursion: return .orange
        case .depthLimitExceeded: return .purple
        case .performanceIssue: return .yellow
        }
    }
    
    var icon: String {
        switch self {
        case .unallowedRecursion: return "xmark.circle.fill"
        case .potentialInfiniteRecursion: return "exclamationmark.triangle.fill"
        case .depthLimitExceeded: return "arrow.clockwise.circle.fill"
        case .performanceIssue: return "speedometer"
        }
    }
}

enum WarningSeverity {
    case error, warning, info
    
    var color: Color {
        switch self {
        case .error: return .red
        case .warning: return .orange
        case .info: return .blue
        }
    }
}

// MARK: - Function Overloading Support
struct OverloadConflict: Identifiable {
    let id = UUID()
    let functionName: String
    let overload1: GraphFunction
    let overload2: GraphFunction
    let type: OverloadConflictType
    let message: String
}

enum OverloadConflictType {
    case ambiguousSignature
    case identicalSignature
    case incompatibleReturn
    
    var color: Color {
        switch self {
        case .ambiguousSignature: return .orange
        case .identicalSignature: return .red
        case .incompatibleReturn: return .purple
        }
    }
    
    var icon: String {
        switch self {
        case .ambiguousSignature: return "questionmark.circle.fill"
        case .identicalSignature: return "xmark.circle.fill"
        case .incompatibleReturn: return "arrow.left.arrow.right"
        }
    }
}

// MARK: - Pure Function Support
struct PurityViolation: Identifiable {
    let id = UUID()
    let nodeType: NodeType
    let nodeName: String
    let violationType: PurityViolationType
    let message: String
}

enum PurityViolationType {
    case sideEffect
    case externalDependency
    case stateModification
    
    var color: Color {
        switch self {
        case .sideEffect: return .red
        case .externalDependency: return .orange
        case .stateModification: return .purple
        }
    }
    
    var icon: String {
        switch self {
        case .sideEffect: return "exclamationmark.triangle.fill"
        case .externalDependency: return "link.circle.fill"
        case .stateModification: return "arrow.up.doc.fill"
        }
    }
}

struct PuritySuggestion: Identifiable {
    let id = UUID()
    let type: PuritySuggestionType
    let message: String
    let priority: SuggestionPriority
}

enum PuritySuggestionType {
    case removeSideEffects
    case passDependenciesAsParameters
    case returnNewState
    case isolateSideEffects
    
    var icon: String {
        switch self {
        case .removeSideEffects: return "trash.circle"
        case .passDependenciesAsParameters: return "arrow.right.circle"
        case .returnNewState: return "arrow.up.circle"
        case .isolateSideEffects: return "cube.box"
        }
    }
}

enum SuggestionPriority {
    case high, medium, low
    
    var color: Color {
        switch self {
        case .high: return .red
        case .medium: return .orange
        case .low: return .blue
        }
    }
}

// MARK: - Latent Function Support
struct LatentFunctionIssue: Identifiable {
    let id = UUID()
    let type: LatentFunctionIssueType
    let message: String
    let severity: IssueSeverity
}

enum LatentFunctionIssueType {
    case missingAsyncStructure
    case missingTimeout
    case missingCancellation
    case potentialDeadlock
    case invalidAsyncFlow
    
    var color: Color {
        switch self {
        case .missingAsyncStructure: return .orange
        case .missingTimeout: return .yellow
        case .missingCancellation: return .blue
        case .potentialDeadlock: return .red
        case .invalidAsyncFlow: return .purple
        }
    }
    
    var icon: String {
        switch self {
        case .missingAsyncStructure: return "clock.arrow.circlepath"
        case .missingTimeout: return "hourglass"
        case .missingCancellation: return "xmark.circle"
        case .potentialDeadlock: return "link.badge.plus"
        case .invalidAsyncFlow: return "arrow.triangle.2.circlepath"
        }
    }
}

enum IssueSeverity {
    case error, warning, info
    
    var color: Color {
        switch self {
        case .error: return .red
        case .warning: return .orange
        case .info: return .blue
        }
    }
}

struct LatentExecutionPlan: Identifiable {
    let id = UUID()
    let function: GraphFunction
    var asyncNodes: [GraphNode] = []
    var timeline: [TimelineEvent] = []
    var blockingPoints: [BlockingPoint] = []
    var estimatedTotalDuration: TimeInterval = 0
    
    init(function: GraphFunction) {
        self.function = function
    }
}

struct TimelineEvent: Identifiable {
    let id = UUID()
    let nodeId: UUID
    let nodeName: String
    let startTime: TimeInterval
    let duration: TimeInterval
    let type: TimelineEventType
    
    var endTime: TimeInterval {
        return startTime + duration
    }
}

enum TimelineEventType {
    case event
    case delay
    case functionCall
    case ioOperation
    case audioOperation
    case animationOperation
    case computation
    
    var color: Color {
        switch self {
        case .event: return .blue
        case .delay: return .orange
        case .functionCall: return .purple
        case .ioOperation: return .green
        case .audioOperation: return .pink
        case .animationOperation: return .cyan
        case .computation: return .gray
        }
    }
    
    var icon: String {
        switch self {
        case .event: return "bolt.fill"
        case .delay: return "clock"
        case .functionCall: return "function"
        case .ioOperation: return "doc.text"
        case .audioOperation: return "speaker.2"
        case .animationOperation: return "play.rectangle"
        case .computation: return "cpu"
        }
    }
}

struct BlockingPoint: Identifiable {
    let id = UUID()
    let nodeId: UUID
    let nodeName: String
    let blockingType: BlockingType
    let estimatedDuration: TimeInterval
}

enum BlockingType {
    case timeDelay
    case asyncFunction
    case ioOperation
    case audioOperation
    case animationOperation
    case computation
    
    var color: Color {
        switch self {
        case .timeDelay: return .orange
        case .asyncFunction: return .purple
        case .ioOperation: return .green
        case .audioOperation: return .pink
        case .animationOperation: return .cyan
        case .computation: return .gray
        }
    }
    
    var icon: String {
        switch self {
        case .timeDelay: return "clock"
        case .asyncFunction: return "function"
        case .ioOperation: return "externaldrive"
        case .audioOperation: return "speaker.2"
        case .animationOperation: return "play.rectangle"
        case .computation: return "cpu"
        }
    }
}

// MARK: - Function Documentation Support
struct DocumentationIssue: Identifiable {
    let id = UUID()
    let type: DocumentationIssueType
    let message: String
    let severity: DocumentationSeverity
}

enum DocumentationIssueType {
    case missingDescription
    case undocumentedParameters
    case undocumentedReturnValue
    case missingExamples
    case outdatedDocumentation
    case inconsistentNaming
    
    var color: Color {
        switch self {
        case .missingDescription: return .red
        case .undocumentedParameters: return .orange
        case .undocumentedReturnValue: return .yellow
        case .missingExamples: return .blue
        case .outdatedDocumentation: return .purple
        case .inconsistentNaming: return .pink
        }
    }
    
    var icon: String {
        switch self {
        case .missingDescription: return "text.badge.minus"
        case .undocumentedParameters: return "list.bullet.rectangle"
        case .undocumentedReturnValue: return "arrow.left.circle"
        case .missingExamples: return "play.circle"
        case .outdatedDocumentation: return "clock.arrow.circlepath"
        case .inconsistentNaming: return "exclamationmark.triangle"
        }
    }
}

enum DocumentationSeverity {
    case error, warning, info
    
    var color: Color {
        switch self {
        case .error: return .red
        case .warning: return .orange
        case .info: return .blue
        }
    }
}

// MARK: - Macro Definition Support
struct MacroIssue: Identifiable {
    let id = UUID()
    let type: MacroIssueType
    let message: String
    let severity: MacroSeverity
}

enum MacroIssueType {
    case emptyMacro
    case undefinedParameters
    case circularDependency
    case invalidNodeTypes
    case missingConnections
    case incompatibleParameters
    
    var color: Color {
        switch self {
        case .emptyMacro: return .orange
        case .undefinedParameters: return .yellow
        case .circularDependency: return .red
        case .invalidNodeTypes: return .purple
        case .missingConnections: return .blue
        case .incompatibleParameters: return .pink
        }
    }
    
    var icon: String {
        switch self {
        case .emptyMacro: return "cube.box"
        case .undefinedParameters: return "questionmark.circle"
        case .circularDependency: return "arrow.triangle.2.circlepath"
        case .invalidNodeTypes: return "exclamationmark.triangle"
        case .missingConnections: return "link.badge.minus"
        case .incompatibleParameters: return "arrow.left.arrow.right"
        }
    }
}

enum MacroSeverity {
    case error, warning, info
    
    var color: Color {
        switch self {
        case .error: return .red
        case .warning: return .orange
        case .info: return .blue
        }
    }
}

enum NodeType: String, CaseIterable {
    case event, branch, sequence, forLoop, forEach
    case getter, setter, function, math, logic
    case variable, constant, comment
    case macro, graphInstance, graphInput, graphOutput
    case literal, enumValue, structConstructor, objectInstance
    case variableGet, variableSet, constantValue
    case arrayLiteral, dictionaryLiteral, typeCast, typeCheck, nullCheck, safeAccess
    
    // Array manipulation nodes
    case arrayCreate, arrayGet, arraySet, arrayLength
    case arrayAdd, arrayRemove, arrayInsert, arrayClear
    case arrayFind, arraySort, arrayReverse, arraySlice
    case arrayContains, arrayIndexOf, arrayJoin, arraySplit
    
    // Dictionary manipulation nodes
    case dictCreate, dictGet, dictSet, dictRemove
    case dictHasKey, dictGetKeys, dictGetValues, dictClear
    case dictMerge, dictFilter, dictMap, dictSize
    
    // Transform manipulation nodes
    case transformGetPosition, transformSetPosition, transformGetRotation
    case transformSetRotation, transformGetScale, transformSetScale
    case transformTranslate, transformRotate, transformScale
    case transformLookAt, transformGetForward, transformGetRight, transformGetUp
    
    // Physics nodes
    case physicsRaycast, physicsSphereCast, physicsBoxCast
    case physicsAddForce, physicsAddImpulse, physicsSetVelocity
    case physicsGetVelocity, physicsSetMass, physicsGetMass
    case physicsEnableGravity, physicsDisableGravity, physicsSetLinearDamping
    
    // Audio nodes
    case audioPlay, audioStop, audioPause, audioResume
    case audioSetVolume, audioGetVolume, audioSetPitch, audioGetPitch
    case audioPlayOneShot, audioPlayLooped, audioStopAll
    
    // Animation nodes
    case animPlay, animStop, animPause, animResume
    case animSetSpeed, animGetSpeed, animSetTime, animGetTime
    case animBlend, animCrossFade, animHasFinished, animGetDuration
    
    // UI nodes
    case uiShow, uiHide, uiSetText, uiGetText
    case uiSetPosition, uiGetPosition, uiSetSize, uiGetSize
    case uiSetVisible, uiIsVisible, uiSetEnabled, uiIsEnabled
    
    // File I/O nodes
    case fileRead, fileWrite, fileExists, fileDelete
    case fileCopy, fileMove, fileGetSize, fileGetModified
    
    // Event nodes
    case onStart, onUpdate, onDestroy
    case inputKeyboard, inputMouse, inputTouch
    case collisionEnter, collisionExit, collisionStay
    case triggerEnter, triggerExit, triggerStay
    case customEvent
    
    // Flow control nodes
    case flowBranch, flowSwitch, flowSelect
    case loopFor, loopForEach, loopWhile, loopDoWhile
    case flowSequence
    case gate, latch
    case delay, timer
    case asyncAwait
    
    // Math nodes
    case mathAdd, mathSubtract, mathMultiply, mathDivide, mathModulo
    case mathSin, mathCos, mathTan, mathAsin, mathAcos, mathAtan
    case mathPow, mathSqrt, mathAbs, mathMin, mathMax, mathClamp
    case mathLerp, mathInverseLerp, mathSmoothStep
    
    // Vector math nodes
    case vectorAdd, vectorSubtract, vectorMultiply, vectorDivide
    case vectorDot, vectorCross, vectorMagnitude, vectorNormalize
    case vectorDistance, vectorReflect, vectorProject, vectorAngle
    case vector3Make, vector2Make, vector4Make, quaternionMake
    
    // Matrix math nodes
    case matrixMake, matrixIdentity, matrixTranspose, matrixInverse
    case matrixMultiply, matrixDeterminant, matrixDecompose
    case matrixTranslation, matrixRotation, matrixScale, matrixTRS
    
    // Logic nodes
    case logicAnd, logicOr, logicNot, logicXor, logicNand, logicNor
    case logicImplies, logicEquivalent, logicBitwiseAnd, logicBitwiseOr
    case logicBitwiseXor, logicBitwiseNot, logicShiftLeft, logicShiftRight
    
    // Comparison nodes
    case compareEqual, compareNotEqual, compareLess, compareLessEqual
    case compareGreater, compareGreaterEqual, compareApproximately
    case compareIsNone, compareIsNotNull, compareIsTrue, compareIsFalse
    
    // String manipulation nodes
    case stringConcat, stringSubstring, stringLength, stringUpper
    case stringLower, stringReplace, stringSplit, stringJoin
    case stringTrim, stringPadLeft, stringPadRight, stringFormat
    
    // Reroute nodes
    case reroute
}

enum PortType: String, CaseIterable {
    // Basic types
    case flow, bool, int, float, string, object, any
    // Vector types
    case vector2, vector3, vector4, quaternion
    // Collection types
    case array, dictionary
    // Type system types
    case enumType, structType, classType
    // Optional and generic types
    case optional, generic, wildcard
    // Asset and reference types
    case asset, delegate, event
    // Engine-specific types
    case transform, raycast, audioClip, animationClip
    case uiElement, fileInfo
    
    // Dictionary key/value types for type safety
    case dictKeyString = "DictKey<String>"
    case dictKeyInt = "DictKey<Int>"
    case dictKeyFloat = "DictKey<Float>"
    case dictValueString = "DictValue<String>"
    case dictValueInt = "DictValue<Int>"
    case dictValueFloat = "DictValue<Float>"
    case dictValueObject = "DictValue<Object>"
    case dictValueVector3 = "DictValue<Vector3>"
    
    var isDictionaryKey: Bool {
        switch self {
        case .dictKeyString, .dictKeyInt, .dictKeyFloat:
            return true
        default:
            return false
        }
    }
    
    var isDictionaryValue: Bool {
        switch self {
        case .dictValueString, .dictValueInt, .dictValueFloat, .dictValueObject, .dictValueVector3:
            return true
        default:
            return false
        }
    }
    
    var baseDictionaryType: PortType? {
        switch self {
        case .dictKeyString, .dictValueString:
            return .string
        case .dictKeyInt, .dictValueInt:
            return .int
        case .dictKeyFloat, .dictValueFloat:
            return .float
        case .dictValueObject:
            return .object
        case .dictValueVector3:
            return .vector3
        default:
            return nil
        }
    }
    
    var color: Color {
        switch self {
        case .flow: return .white
        case .bool: return .red
        case .int: return .cyan
        case .float: return .green
        case .vector2: return Color(red: 1.0, green: 1.0, blue: 0.8) // Light yellow
        case .vector3: return .yellow
        case .vector4: return .orange
        case .quaternion: return .orange
        case .string: return .magenta
        case .object: return .blue
        case .enumType: return .teal
        case .structType: return .purple
        case .classType: return .purple
        case .asset: return Color(red: 1.0, green: 0.843, blue: 0.0) // Gold
        case .delegate: return .pink
        case .event: return .pink
        case .array: return .orange
        case .dictionary: return .purple
        case .optional: return .gray
        case .generic: return .gray
        case .wildcard: return .gray
        case .any: return .gray
        case .transform: return .secondary
        case .raycast: return .primary
        case .audioClip: return .brown
        case .animationClip: return .cyan
        case .uiElement: return .green
        case .fileInfo: return .orange
        
        // Dictionary key types (lighter shades)
        case .dictKeyString: return .magenta.opacity(0.7)
        case .dictKeyInt: return .cyan.opacity(0.7)
        case .dictKeyFloat: return .green.opacity(0.7)
        
        // Dictionary value types (darker shades)
        case .dictValueString: return .magenta.opacity(0.9)
        case .dictValueInt: return .cyan.opacity(0.9)
        case .dictValueFloat: return .green.opacity(0.9)
        case .dictValueObject: return .blue.opacity(0.9)
        case .dictValueVector3: return .yellow.opacity(0.9)
        }
    }
    
    var shape: PortShape {
        switch self {
        case .flow:
            return .triangle
        case .array:
            return .dashedCircle
        default:
            return .circle
        }
    }
    
    var isNullable: Bool {
        switch self {
        case .optional:
            return true
        default:
            return false
        }
    }
    
    var isGeneric: Bool {
        switch self {
        case .generic, .wildcard, .any:
            return true
        default:
            return false
        }
    }
    
    // MARK: - Dictionary Type Utilities
    
    static func dictionaryKeyType(for baseType: PortType) -> PortType? {
        switch baseType {
        case .string: return .dictKeyString
        case .int: return .dictKeyInt
        case .float: return .dictKeyFloat
        default: return nil
        }
    }
    
    static func dictionaryValueType(for baseType: PortType) -> PortType? {
        switch baseType {
        case .string: return .dictValueString
        case .int: return .dictValueInt
        case .float: return .dictValueFloat
        case .object: return .dictValueObject
        case .vector3: return .dictValueVector3
        default: return nil
        }
    }
    
    func asDictionaryKey() -> PortType? {
        return PortType.dictionaryKeyType(for: self)
    }
    
    func asDictionaryValue() -> PortType? {
        return PortType.dictionaryValueType(for: self)
    }
    
    func canConnectTo(_ other: PortType) -> Bool {
        // Dictionary keys can only connect to dictionary keys of the same base type
        if isDictionaryKey && other.isDictionaryKey {
            return baseDictionaryType == other.baseDictionaryType
        }
        
        // Dictionary values can only connect to dictionary values of compatible types
        if isDictionaryValue && other.isDictionaryValue {
            return areBaseTypesCompatible(baseDictionaryType, other.baseDictionaryType)
        }
        
        // Dictionary key can connect to its base type
        if isDictionaryKey && !other.isDictionaryKey {
            return baseDictionaryType == other
        }
        
        // Dictionary value can connect to its base type
        if isDictionaryValue && !other.isDictionaryValue {
            return baseDictionaryType == other
        }
        
        // Standard type compatibility
        return areBaseTypesCompatible(self, other)
    }
    
    private func areBaseTypesCompatible(_ type1: PortType?, _ type2: PortType?) -> Bool {
        guard let type1 = type1, let type2 = type2 else { return false }
        
        // Same types are compatible
        if type1 == type2 { return true }
        
        // 'any' type is compatible with everything
        if type1 == .any || type2 == .any { return true }
        
        // Numeric type conversions
        let numericTypes: [PortType] = [.int, .float, .vector2, .vector3, .vector4]
        if numericTypes.contains(type1) && numericTypes.contains(type2) { return true }
        
        // Flow type compatibility
        if type1 == .flow && type2 == .flow { return true }
        
        return false
    }
}

enum PortShape {
    case circle
    case triangle
    case square
    case diamond
    case dashedCircle
}

// MARK: - Node Customization Manager

class NodeCustomizationManager: ObservableObject {
    static let shared = NodeCustomizationManager()
    
    @Published var customIcons: [NodeType: String] = [:]
    @Published var customColors: [NodeType: Color] = [:]
    @Published var nodeDescriptions: [NodeType: String] = [:]
    @Published var nodeDocumentation: [NodeType: String] = [:]
    
    private init() {
        loadDefaultCustomizations()
    }
    
    private func loadDefaultCustomizations() {
        // Load default descriptions and documentation
        nodeDescriptions = [
            .event: "Triggers events in the graph",
            .branch: "Conditional branching logic",
            .sequence: "Execute actions in sequence",
            .getter: "Get property values",
            .setter: "Set property values",
            .function: "Call functions or methods",
            .math: "Mathematical operations",
            .logic: "Logical operations",
            .variable: "Variable storage and access",
            .constant: "Constant values",
            .comment: "Documentation and notes"
        ]
        
        nodeDocumentation = [
            .event: "Event nodes are entry points that trigger when specific events occur in the game or system.",
            .branch: "Branch nodes evaluate conditions and direct execution flow based on boolean results.",
            .sequence: "Sequence nodes execute multiple actions in order, waiting for each to complete.",
            .getter: "Getter nodes retrieve values from properties, components, or system variables.",
            .setter: "Setter nodes modify values of properties, components, or system variables.",
            .function: "Function nodes call predefined functions with input parameters and return values.",
            .math: "Math nodes perform mathematical calculations and transformations.",
            .logic: "Logic nodes evaluate boolean expressions and logical operations.",
            .variable: "Variable nodes store and retrieve data that can be shared across the graph.",
            .constant: "Constant nodes provide fixed values that don't change during execution.",
            .comment: "Comment nodes add documentation and notes to help explain graph logic."
        ]
    }
    
    func getIcon(for type: NodeType) -> String {
        return customIcons[type] ?? defaultIconForType(type)
    }
    
    func getColor(for type: NodeType) -> Color {
        return customColors[type] ?? defaultColorForType(type)
    }
    
    func getDescription(for type: NodeType) -> String {
        return nodeDescriptions[type] ?? ""
    }
    
    func getDocumentation(for type: NodeType) -> String {
        return nodeDocumentation[type] ?? ""
    }
    
    func setCustomIcon(for type: NodeType, icon: String) {
        customIcons[type] = icon
    }
    
    func setCustomColor(for type: NodeType, color: Color) {
        customColors[type] = color
    }
    
    private func defaultIconForType(_ type: NodeType) -> String {
        switch type {
        case .event: return "bolt.fill"
        case .branch: return "arrow.branch"
        case .sequence: return "arrow.right"
        case .getter: return "arrow.down.doc"
        case .setter: return "arrow.up.doc"
        case .function: return "function"
        case .math: return "plus.forwardslash.minus"
        case .logic: return "questionmark.diamond"
        case .variable: return "v.square"
        case .constant: return "number"
        case .comment: return "text.bubble"
        default: return "cube"
        }
    }
    
    private func defaultColorForType(_ type: NodeType) -> Color {
        switch type {
        case .event: return .red
        case .branch, .sequence: return .gray
        case .getter: return .green
        case .setter: return .blue
        case .function: return .purple
        case .math, .logic: return .teal
        case .variable: return .orange
        case .constant: return .orange
        case .comment: return .yellow.opacity(0.3)
        default: return .secondary
        }
    }
}

// MARK: - Node Clipboard Manager

class NodeClipboardManager: ObservableObject {
    static let shared = NodeClipboardManager()
    
    @Published var copiedNodes: [GraphNode] = []
    @Published var copiedConnections: [GraphConnection] = []
    
    private init() {}
    
    func copyNodes(_ nodes: [GraphNode], connections: [GraphConnection]) {
        copiedNodes = nodes.map { node in
            var copiedNode = node
            copiedNode.id = UUID() // Generate new ID for paste
            copiedNode.position = CGPoint(x: node.position.x + 50, y: node.position.y + 50)
            return copiedNode
        }
        
        // Copy connections with updated node IDs
        copiedConnections = connections.map { connection in
            var copiedConnection = connection
            copiedConnection.id = UUID()
            // Update node IDs to match copied nodes
            if let originalNodeIndex = nodes.firstIndex(where: { $0.id == connection.fromNode }) {
                copiedConnection.fromNode = copiedNodes[originalNodeIndex].id
            }
            if let originalNodeIndex = nodes.firstIndex(where: { $0.id == connection.toNode }) {
                copiedConnection.toNode = copiedNodes[originalNodeIndex].id
            }
            return copiedConnection
        }
    }
    
    func canPaste() -> Bool {
        return !copiedNodes.isEmpty
    }
    
    func paste(at position: CGPoint) -> ([GraphNode], [GraphConnection]) {
        let pastedNodes = copiedNodes.map { node in
            var pastedNode = node
            pastedNode.id = UUID() // Generate new ID
            pastedNode.position = CGPoint(
                x: position.x + (node.position.x - copiedNodes.first!.position.x),
                y: position.y + (node.position.y - copiedNodes.first!.position.y)
            )
            return pastedNode
        }
        
        let pastedConnections = copiedConnections.map { connection in
            var pastedConnection = connection
            pastedConnection.id = UUID()
            // Update node IDs to match pasted nodes
            if let originalNodeIndex = copiedNodes.firstIndex(where: { $0.id == connection.fromNode }) {
                pastedConnection.fromNode = pastedNodes[originalNodeIndex].id
            }
            if let originalNodeIndex = copiedNodes.firstIndex(where: { $0.id == connection.toNode }) {
                pastedConnection.toNode = pastedNodes[originalNodeIndex].id
            }
            return pastedConnection
        }
        
        return (pastedNodes, pastedConnections)
    }
}

// MARK: - Node Documentation System

struct NodeDocumentation: View {
    let nodeType: NodeType
    @State private var isVisible = false
    
    var body: some View {
        Button(action: { isVisible = true }) {
            Image(systemName: "info.circle")
                .font(.system(size: 12))
                .foregroundColor(.secondary)
        }
        .popover(isPresented: $isVisible) {
            DocumentationView(nodeType: nodeType)
        }
    }
}

struct DocumentationView: View {
    let nodeType: NodeType
    @Environment(\.dismiss) private var dismiss
    
    private var customizationManager = NodeCustomizationManager.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            // Header
            HStack {
                Image(systemName: customizationManager.getIcon(for: nodeType))
                    .font(.title2)
                    .foregroundColor(customizationManager.getColor(for: nodeType))
                
                VStack(alignment: .leading) {
                    Text(String(describing: nodeType))
                        .font(.title2)
                        .fontWeight(.bold)
                    Text(customizationManager.getDescription(for: nodeType))
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                
                Spacer()
                
                Button("Done") { dismiss() }
            }
            
            Divider()
            
            // Documentation content
            ScrollView {
                Text(customizationManager.getDocumentation(for: nodeType))
                    .font(.body)
                    .lineLimit(nil)
                    .multilineTextAlignment(.leading)
            }
            .frame(maxHeight: 300)
            
            // Additional info
            VStack(alignment: .leading, spacing: 8) {
                Text("Inputs")
                    .font(.headline)
                Text(getInputDescription())
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Text("Outputs")
                    .font(.headline)
                Text(getOutputDescription())
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
        .padding()
        .frame(width: 400, height: 500)
    }
    
    private func getInputDescription() -> String {
        switch nodeType {
        case .branch: return "Condition (Boolean)"
        case .math: return "A, B (Numeric)"
        case .logic: return "A, B (Boolean)"
        default: return "Varies by node type"
        }
    }
    
    private func getOutputDescription() -> String {
        switch nodeType {
        case .branch: return "True, False (Flow)"
        case .math: return "Result (Numeric)"
        case .logic: return "Result (Boolean)"
        default: return "Varies by node type"
        }
    }
}

// MARK: - Node Library Panel
struct NodeLibraryPanel: View {
    @ObservedObject var manager: NodeGraphManager
    @State private var searchText = ""
    @State private var selectedCategory = "All"
    @State private var draggedNode: NodeType?
    @State private var isDragging = false
    
    @ObservedObject private var favorites = NodeFavorites.shared
    @ObservedObject private var recentNodes = RecentNodes.shared
    
    private let nodeCategories: [String: [NodeType]] = [
        "Events": [
            .onStart, .onUpdate, .onDestroy,
            .inputKeyboard, .inputMouse, .inputTouch,
            .collisionEnter, .collisionExit, .collisionStay,
            .triggerEnter, .triggerExit, .triggerStay,
            .customEvent
        ],
        "Flow Control": [
            .flowBranch, .flowSwitch, .flowSelect,
            .loopFor, .loopForEach, .loopWhile, .loopDoWhile,
            .flowSequence, .gate, .latch, .delay, .timer, .asyncAwait
        ],
        "Math": [
            .mathAdd, .mathSubtract, .mathMultiply, .mathDivide, .mathModulo,
            .mathSin, .mathCos, .mathTan, .mathAsin, .mathAcos, .mathAtan,
            .mathPow, .mathSqrt, .mathAbs, .mathMin, .mathMax, .mathClamp,
            .mathLerp, .mathInverseLerp, .mathSmoothStep
        ],
        "Vector": [
            .vectorAdd, .vectorSubtract, .vectorMultiply, .vectorDivide,
            .vectorDot, .vectorCross, .vectorMagnitude, .vectorNormalize,
            .vectorDistance, .vectorReflect, .vectorProject, .vectorAngle,
            .vector3Make, .vector2Make, .vector4Make, .quaternionMake
        ],
        "Matrix": [
            .matrixMake, .matrixIdentity, .matrixTranspose, .matrixInverse,
            .matrixMultiply, .matrixDeterminant, .matrixDecompose,
            .matrixTranslation, .matrixRotation, .matrixScale, .matrixTRS
        ],
        "Logic": [
            .logicAnd, .logicOr, .logicNot, .logicXor, .logicNand, .logicNor,
            .logicImplies, .logicEquivalent, .logicBitwiseAnd, .logicBitwiseOr,
            .logicBitwiseXor, .logicBitwiseNot, .logicShiftLeft, .logicShiftRight
        ],
        "Comparison": [
            .compareEqual, .compareNotEqual, .compareLess, .compareLessEqual,
            .compareGreater, .compareGreaterEqual, .compareApproximately,
            .compareIsNone, .compareIsNotNull, .compareIsTrue, .compareIsFalse
        ],
        "String": [
            .stringConcat, .stringSubstring, .stringLength, .stringUpper,
            .stringLower, .stringReplace, .stringSplit, .stringJoin,
            .stringTrim, .stringPadLeft, .stringPadRight, .stringFormat
        ],
        "Variables": [.variable, .variableGet, .variableSet, .constant, .constantValue],
        "Data": [.literal, .enumValue, .structConstructor, .objectInstance],
        "Graph": [.macro, .graphInstance, .graphInput, .graphOutput],
        "Utility": [.comment, .reroute]
    ]
    
    var categories: [String] {
        ["All", "Favorites", "Recent"] + Array(nodeCategories.keys).sorted()
    }
    
    var filteredTypes: [NodeType] {
        let baseTypes: [NodeType]
        
        switch selectedCategory {
        case "All":
            baseTypes = NodeType.allCases
        case "Favorites":
            baseTypes = Array(favorites.favorites)
        case "Recent":
            baseTypes = recentNodes.recentNodes
        default:
            baseTypes = nodeCategories[selectedCategory] ?? []
        }
        
        if searchText.isEmpty {
            return baseTypes
        }
        
        // Use fuzzy search
        let scoredTypes = baseTypes.map { type in
            let score = FuzzySearch.fuzzyMatch(searchText, type.rawValue)
            return (type: type, score: score)
        }
        
        return scoredTypes
            .filter { $0.score >= 0.3 }
            .sorted { $0.score > $1.score }
            .map { $0.type }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: "square.grid.2x2")
                    .foregroundColor(.white.opacity(0.8))
                Text("Node Library")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                Spacer()
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(Color(red: 0.15, green: 0.15, blue: 0.18))
            
            // Search
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.white.opacity(0.6))
                TextField("Search nodes...", text: $searchText)
                    .textFieldStyle(.plain)
            }
            .padding(8)
            .background(Color.white.opacity(0.1))
            
            // Category Filter
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 6) {
                    ForEach(categories, id: \.self) { category in
                        Button(action: { selectedCategory = category }) {
                            Text(category)
                                .font(.caption)
                                .padding(.horizontal, 8)
                                .padding(.vertical, 4)
                                .background(selectedCategory == category ? Color.blue : Color.white.opacity(0.1))
                                .foregroundColor(.white)
                                .cornerRadius(4)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.horizontal, 8)
            }
            .padding(.vertical, 4)
            
            // Node List
            ScrollView {
                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 2), spacing: 8) {
                    ForEach(filteredTypes, id: \.self) { type in
                        NodeLibraryItem(
                            nodeType: type,
                            isDragging: draggedNode == type,
                            onDragStart: {
                                draggedNode = type
                                isDragging = true
                            },
                            onDragEnd: {
                                draggedNode = nil
                                isDragging = false
                            }
                        )
                    }
                }
                .padding(8)
            }
        }
        .frame(width: 300)
        .background(Color(red: 0.1, green: 0.1, blue: 0.12))
        .onDrop(of: [String.self], isTargeted: nil) { providers, location in
            // Handle drop from library to canvas
            if let provider = providers.first {
                if let data = provider.data(using: .utf8),
                   let nodeTypeString = String(data: data, encoding: .utf8),
                   let nodeType = NodeType(rawValue: nodeTypeString) {
                    // Convert drop location to canvas coordinates
                    let canvasPosition = convertDropLocationToCanvas(location)
                    manager.addNode(type: nodeType, at: canvasPosition)
                    return true
                }
            }
            return false
        }
    }
    
    private func convertDropLocationToCanvas(_ location: CGPoint) -> CGPoint {
        // This would need to be implemented based on the actual canvas layout
        // For now, return a default position
        return CGPoint(x: location.x, y: location.y)
    }
}

// MARK: - Node Library Item
struct NodeLibraryItem: View {
    let nodeType: NodeType
    let isDragging: Bool
    let onDragStart: () -> Void
    let onDragEnd: () -> Void
    
    @ObservedObject private var favorites = NodeFavorites.shared
    
    var body: some View {
        VStack(spacing: 4) {
            // Icon
            Image(systemName: iconForType(nodeType))
                .font(.system(size: 16))
                .foregroundColor(nodeColorForType(nodeType))
                .frame(width: 24, height: 24)
            
            // Name
            Text(nodeType.rawValue.capitalized)
                .font(.caption)
                .foregroundColor(.white)
                .lineLimit(2)
                .multilineTextAlignment(.center)
            
            // Favorite indicator
            HStack {
                Spacer()
                Button(action: { favorites.toggleFavorite(nodeType) }) {
                    Image(systemName: favorites.isFavorite(nodeType) ? "star.fill" : "star")
                        .foregroundColor(favorites.isFavorite(nodeType) ? .yellow : .white.opacity(0.4))
                        .font(.system(size: 10))
                }
                .buttonStyle(.plain)
            }
        }
        .padding(8)
        .frame(height: 60)
        .background(
            RoundedRectangle(cornerRadius: 6)
                .fill(isDragging ? Color.blue.opacity(0.3) : Color.white.opacity(0.05))
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(nodeColorForType(nodeType).opacity(0.5), lineWidth: 1)
                )
        )
        .scaleEffect(isDragging ? 0.95 : 1.0)
        .animation(.easeInOut(duration: 0.2), value: isDragging)
        .onDrag {
            onDragStart()
            return NSItemProvider(object: nodeType.rawValue as NSString)
        }
        .onDrop(of: [String.self], isTargeted: nil) { providers, location in
            // Handle drop for reordering or other interactions
            onDragEnd()
            return false
        }
    }
    
    private func iconForType(_ type: NodeType) -> String {
        // Reuse the icon mapping from EnhancedNodeCreationMenu
        switch type {
        case .event, .onStart, .onUpdate, .onDestroy: return "bolt.fill"
        case .flowBranch: return "arrow.branch"
        case .variable: return "v.square"
        case .constant: return "number"
        case .mathAdd: return "plus.forwardslash.minus"
        case .logicAnd: return "questionmark.diamond"
        case .compareEqual: return "equal"
        case .comment: return "text.bubble"
        case .stringConcat: return "textformat"
        default: return "square"
        }
    }
    
    private func nodeColorForType(_ type: NodeType) -> Color {
        switch type {
        case .event, .onStart, .onUpdate, .onDestroy: return .red
        case .flowBranch: return .gray
        case .variable: return .orange
        case .constant: return .orange
        case .mathAdd: return .teal
        case .logicAnd: return .teal
        case .compareEqual: return .orange
        case .comment: return .yellow.opacity(0.3)
        case .stringConcat: return .pink
        default: return .secondary
        }
    }
}
struct NodeGraphView: View {
    @ObservedObject var manager = NodeGraphManager.shared
    @State private var showNodeMenu = false
    @State private var nodeMenuPosition: CGPoint = .zero
    @State private var showVariablePanel = false
    @State private var showMacroPanel = false
    @State private var showTemplateBrowser = false
    @State private var quickAddText = ""
    @State private var showQuickAdd = false
    @State private var isPanning = false
    @State private var lastPanLocation: CGPoint = .zero
    @State private var panVelocity: CGPoint = .zero
    @State private var showSearchPanel = false
    @State private var showBookmarkPanel = false
    @State private var showAlignmentPanel = false
    
    @ObservedObject private var keyboardShortcuts = NodeKeyboardShortcuts.shared
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            NodeGraphToolbar(manager: manager)
            
            EditorDivider()
            
            // Canvas with enhanced features
            GeometryReader { geometry in
                ZStack {
                    // Background grid with pattern support
                    NodeGraphGrid(
                        offset: manager.panOffset, 
                        zoom: manager.zoom,
                        gridSize: manager.gridSize,
                        pattern: manager.gridPattern
                    )
                    
                    // Rulers (optional)
                    if manager.showRulers {
                        CanvasRulers(
                            offset: manager.panOffset,
                            zoom: manager.zoom,
                            size: geometry.size
                        )
                    }
                    
                    // Comment boxes (render behind nodes)
                    ForEach($manager.commentBoxes) { $commentBox in
                        CommentBoxView(
                            commentBox: $commentBox,
                            offset: manager.panOffset,
                            zoom: manager.zoom
                        )
                    }
                    
                    // Connections (with performance mode)
                    ForEach(manager.connections) { connection in
                        if !manager.performanceMode {
                            ConnectionPath(
                                connection: connection, 
                                nodes: manager.nodes, 
                                offset: manager.panOffset, 
                                zoom: manager.zoom
                            )
                        } else {
                            // Simplified rendering in performance mode
                            SimpleConnectionPath(
                                connection: connection,
                                nodes: manager.nodes,
                                offset: manager.panOffset,
                                zoom: manager.zoom
                            )
                        }
                    }
                    
                    // Nodes (with search filtering)
                    ForEach($manager.nodes) { $node in
                        let isFiltered = !manager.filteredNodes.isEmpty && !manager.filteredNodes.contains(node.id)
                        let isSearchMatch = manager.searchQuery.isEmpty || manager.filteredNodes.contains(node.id)
                        
                        NodeView(
                            node: $node, 
                            isSelected: manager.selectedNodes.contains(node.id),
                            offset: manager.panOffset, 
                            zoom: manager.zoom,
                            isFiltered: isFiltered,
                            isSearchMatch: isSearchMatch
                        )
                        .opacity(isFiltered ? 0.3 : 1.0)
                        .onTapGesture { event in
                            if event.modifiers.contains(.shift) {
                                manager.toggleShiftClickSelection(node.id)
                            } else {
                                manager.selectedNodes = [node.id]
                            }
                        }
                    }
                    
                    // Box selection overlay
                    if manager.isBoxSelecting, let start = manager.boxSelectionStart, let end = manager.boxSelectionEnd {
                        BoxSelectionOverlay(
                            start: start,
                            end: end,
                            offset: manager.panOffset,
                            zoom: manager.zoom
                        )
                    }
                    
                    // Enhanced node creation menu
                    if showNodeMenu {
                        EnhancedNodeCreationMenu(position: nodeMenuPosition) { type in
                            let snappedPosition = manager.snapToGrid(nodeMenuPosition)
                            manager.addNode(type: type, at: snappedPosition)
                            showNodeMenu = false
                        }
                    }
                    
                    // Quick-add overlay
                    if showQuickAdd {
                        QuickAddOverlay(
                            position: nodeMenuPosition,
                            text: $quickAddText,
                            onSubmit: { type in
                                let snappedPosition = manager.snapToGrid(nodeMenuPosition)
                                manager.addNode(type: type, at: snappedPosition)
                                showQuickAdd = false
                                quickAddText = ""
                            },
                            onCancel: {
                                showQuickAdd = false
                                quickAddText = ""
                            }
                        )
                    }
                    
                    // Minimap (optional)
                    if manager.showMinimap {
                        VStack {
                            Spacer()
                            HStack {
                                Spacer()
                                NodeGraphMinimap(
                                    manager: manager,
                                    canvasSize: geometry.size
                                )
                                .padding()
                            }
                        }
                    }
                }
                .clipped() // Enable infinite scrolling
                .background(Color(red: 0.1, green: 0.1, blue: 0.12))
            .gesture(
                    // Enhanced panning with momentum
                    DragGesture(minDistance: 0)
                        .onChanged { value in
                            if !isPanning {
                                isPanning = true
                                lastPanLocation = value.startLocation
                            }
                            
                            let delta = CGPoint(
                                x: value.location.x - lastPanLocation.x,
                                y: value.location.y - lastPanLocation.y
                            )
                            
                            manager.panOffset = CGPoint(
                                x: manager.panOffset.x + delta.x,
                                y: manager.panOffset.y + delta.y
                            )
                            
                            panVelocity = delta
                            lastPanLocation = value.location
                            manager.isPanning = true
                        }
                        .onEnded { value in
                            isPanning = false
                            manager.isPanning = false
                            manager.lastPanVelocity = panVelocity
                            
                            // Apply momentum
                            withAnimation(.easeOut(duration: 0.5)) {
                                let momentum = CGPoint(
                                    x: panVelocity.x * 10,
                                    y: panVelocity.y * 10
                                )
                                manager.panOffset = CGPoint(
                                    x: manager.panOffset.x + momentum.x,
                                    y: manager.panOffset.y + momentum.y
                                )
                            }
                            
                            panVelocity = .zero
                        }
                )
                .gesture(
                    // Mouse wheel zoom with focus point
                    DragGesture(minDistance: 0, coordinateSpace: .local)
                        .onChanged { value in
                            // This would be handled by scroll wheel in a real implementation
                        }
                )
                .onScroll { phase in
                    switch phase {
                    case .active(let scrollPhase):
                        if let delta = scrollPhase.delta {
                            let zoomDelta = 1.0 + (delta.y * -0.001)
                            let newZoom = max(0.1, min(manager.zoom * zoomDelta, 5.0))
                            
                            // Zoom towards mouse position
                            let mousePos = CGPoint(x: delta.x, y: delta.y)
                            let worldPos = convertToWorldCoordinates(mousePos, geometry: geometry)
                            
                            manager.zoom = newZoom
                            
                            let newWorldPos = convertToWorldCoordinates(mousePos, geometry: geometry)
                            let offsetDelta = CGPoint(
                                x: (newWorldPos.x - worldPos.x) * newZoom,
                                y: (newWorldPos.y - worldPos.y) * newZoom
                            )
                            
                            manager.panOffset = CGPoint(
                                x: manager.panOffset.x + offsetDelta.x,
                                y: manager.panOffset.y + offsetDelta.y
                            )
                        }
                    default:
                        break
                    }
                }
            .contextMenu {
                NodeContextMenu(position: nodeMenuPosition) { type in
                    let snappedPosition = manager.snapToGrid(nodeMenuPosition)
                    manager.addNode(type: type, at: snappedPosition)
                }
                Divider()
                Menu("View") {
                    Button("Zoom to Fit All") { manager.zoomToFitAllNodes() }
                    Button("Zoom to Selection") { manager.zoomToSelection() }
                    Button("Focus on Selected") {
                        if let selectedId = manager.selectedNodes.first {
                            manager.focusOnNode(selectedId)
                        }
                    }
                    Divider()
                    Toggle("Show Grid", isOn: Binding(
                        get: { manager.gridPattern != .none },
                        set: { enabled in
                            manager.gridPattern = enabled ? .dots : .none
                        }
                    ))
                    Toggle("Snap to Grid", isOn: $manager.gridSnapEnabled)
                    Toggle("Show Rulers", isOn: $manager.showRulers)
                    Toggle("Show Minimap", isOn: $manager.showMinimap)
                    Toggle("Performance Mode", isOn: $manager.performanceMode)
                }
                Menu("Selection") {
                    Button("Select All Connected") { manager.selectAllConnected() }
                    Button("Select Downstream") { manager.selectDownstream() }
                    Button("Select Upstream") { manager.selectUpstream() }
                    Divider()
                    Button("Clear Selection") { manager.selectedNodes.removeAll() }
                }
                Menu("Layout") {
                    Button("Auto Layout") { manager.autoLayout() }
                    Button("Straighten Connections") { manager.straightenConnections() }
                    Divider()
                    Menu("Align") {
                        Button("Align Left") { manager.alignNodesLeft() }
                        Button("Align Center") { manager.alignNodesCenter() }
                        Button("Align Right") { manager.alignNodesRight() }
                        Divider()
                        Button("Align Top") { manager.alignNodesTop() }
                        Button("Align Middle") { manager.alignNodesMiddle() }
                        Button("Align Bottom") { manager.alignNodesBottom() }
                    }
                    Menu("Distribute") {
                        Button("Distribute Horizontally") { manager.distributeNodesHorizontally() }
                        Button("Distribute Vertically") { manager.distributeNodesVertically() }
                    }
                }
                Menu("Bookmarks") {
                    Button("Add Bookmark") {
                        let bookmarkName = "Bookmark \(manager.bookmarks.count + 1)"
                        manager.addBookmark(name: bookmarkName)
                    }
                    if !manager.bookmarks.isEmpty {
                        Divider()
                        ForEach(manager.bookmarks) { bookmark in
                            Button(bookmark.name) {
                                manager.loadBookmark(bookmark)
                            }
                        }
                    }
                }
                Divider()
                Button("Create Variable...") {
                    showVariablePanel = true
                }
                Button("Add Comment Box...") {
                    let commentBox = CommentBox(
                        title: "Comment",
                        content: "",
                        position: nodeMenuPosition,
                        size: CGSize(width: 200, height: 100)
                    )
                    manager.commentBoxes.append(commentBox)
                }
                Button("Manage Macros...") {
                    showMacroPanel = true
                }
                Button("Browse Templates...") {
                    showTemplateBrowser = true
                }
                Divider()
                Button("Delete Selected", role: .destructive) {
                    manager.deleteSelectedNodes()
                }
            }
            .onKeyPress { keyPress in
                // Handle keyboard shortcuts for node creation
                if let nodeType = keyboardShortcuts.getNodeForKey(keyPress.key) {
                    // Add node at center of view
                    let centerPoint = CGPoint(
                        x: 400 - manager.panOffset.x,
                        y: 300 - manager.panOffset.y
                    )
                    let snappedPosition = manager.snapToGrid(centerPoint)
                    manager.addNode(type: nodeType, at: snappedPosition)
                    return .handled
                }
                
                // Enhanced keyboard shortcuts
                switch keyPress.key {
                case "f":
                    if keyPress.modifiers.contains(.command) {
                        manager.focusOnNode(manager.selectedNodes.first ?? UUID())
                        return .handled
                    }
                case "a":
                    if keyPress.modifiers.contains(.command) {
                        manager.selectedNodes = Set(manager.nodes.map { $0.id })
                        return .handled
                    }
                case "g":
                    if keyPress.modifiers.contains(.command) {
                        manager.gridSnapEnabled.toggle()
                        return .handled
                    }
                case "r":
                    if keyPress.modifiers.contains(.command) {
                        manager.showRulers.toggle()
                        return .handled
                    }
                case "m":
                    if keyPress.modifiers.contains(.command) {
                        manager.showMinimap.toggle()
                        return .handled
                    }
                case "p":
                    if keyPress.modifiers.contains(.command) {
                        manager.performanceMode.toggle()
                        return .handled
                    }
                case "0":
                    if keyPress.modifiers.contains(.command) {
                        manager.zoomToFitAllNodes()
                        return .handled
                    }
                case "=":
                    if keyPress.modifiers.contains(.command) {
                        manager.zoomToSelection()
                        return .handled
                    }
                case "l":
                    if keyPress.modifiers.contains(.command) {
                        manager.autoLayout()
                        return .handled
                    }
                case "s":
                    if keyPress.modifiers.contains(.command) {
                        manager.straightenConnections()
                        return .handled
                    }
                default:
                    break
                }
                
                // Quick-add with spacebar
                if keyPress.key == " " {
                    nodeMenuPosition = CGPoint(x: 400, y: 300)
                    showQuickAdd = true
                    return .handled
                }
                
                return .ignored
            }
            .sheet(isPresented: $showVariablePanel) {
                VariableCreationPanel(manager: manager)
            }
            .sheet(isPresented: $showMacroPanel) {
                MacroManagementPanel(manager: manager)
            }
            .sheet(isPresented: $showTemplateBrowser) {
                TemplateBrowser(manager: manager)
            }
        }
    }
    .searchable(text: $manager.searchQuery, prompt: "Search nodes...")
    .onChange(of: manager.searchQuery) { newValue in
        manager.updateSearchQuery(newValue)
    }
    
    // Helper functions for coordinate conversion
    private func convertToCanvasCoordinates(_ point: CGPoint, geometry: GeometryProxy) -> CGPoint {
        return CGPoint(
            x: (point.x - manager.panOffset.x) / manager.zoom,
            y: (point.y - manager.panOffset.y) / manager.zoom
        )
    }
    
    private func convertToWorldCoordinates(_ point: CGPoint, geometry: GeometryProxy) -> CGPoint {
        return CGPoint(
            x: point.x * manager.zoom + manager.panOffset.x,
            y: point.y * manager.zoom + manager.panOffset.y
        )
    }
}

// MARK: - Enhanced Canvas Components

struct CanvasRulers: View {
    let offset: CGPoint
    let zoom: CGFloat
    let size: CGSize
    
    var body: some View {
        ZStack {
            // Horizontal ruler
            VStack {
                HStack {
                    Rectangle()
                        .fill(Color.black.opacity(0.8))
                        .frame(height: 1)
                    
                    // Ruler marks
                    ForEach(0..<Int(size.width), id: \.self) { x in
                        let worldX = CGFloat(x) - offset.x
                        let canvasX = worldX / zoom
                        
                        if x % 50 == 0 { // Major tick every 50 pixels
                            Rectangle()
                                .fill(Color.white)
                                .frame(width: 1, height: 10)
                                .offset(x: CGFloat(x))
                        } else if x % 10 == 0 { // Minor tick every 10 pixels
                            Rectangle()
                                .fill(Color.white.opacity(0.6))
                                .frame(width: 1, height: 5)
                                .offset(x: CGFloat(x))
                        }
                    }
                }
                .frame(height: 20)
                
                Spacer()
            }
            
            // Vertical ruler
            HStack {
                VStack {
                    Rectangle()
                        .fill(Color.black.opacity(0.8))
                        .frame(width: 1)
                    
                    // Ruler marks
                    ForEach(0..<Int(size.height), id: \.self) { y in
                        let worldY = CGFloat(y) - offset.y
                        let canvasY = worldY / zoom
                        
                        if y % 50 == 0 { // Major tick every 50 pixels
                            Rectangle()
                                .fill(Color.white)
                                .frame(width: 10, height: 1)
                                .offset(y: CGFloat(y))
                        } else if y % 10 == 0 { // Minor tick every 10 pixels
                            Rectangle()
                                .fill(Color.white.opacity(0.6))
                                .frame(width: 5, height: 1)
                                .offset(y: CGFloat(y))
                        }
                    }
                }
                .frame(width: 20)
                
                Spacer()
            }
        }
    }
}

struct CommentBoxView: View {
    @Binding var commentBox: CommentBox
    let offset: CGPoint
    let zoom: CGFloat
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header
            HStack {
                Text(commentBox.title)
                    .font(.headline)
                    .foregroundColor(.black)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                
                Spacer()
                
                Button(action: {
                    // Delete comment box
                }) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundColor(.red)
                }
                .buttonStyle(.plain)
            }
            .background(commentBox.color)
            
            // Content
            Text(commentBox.content)
                .font(.body)
                .foregroundColor(.black)
                .padding(8)
                .frame(minHeight: 50)
                .background(commentBox.color.opacity(0.5))
        }
        .frame(width: commentBox.size.width, height: commentBox.size.height)
        .background(commentBox.color)
        .cornerRadius(8)
        .shadow(radius: 4)
        .position(
            x: commentBox.position.x * zoom + offset.x,
            y: commentBox.position.y * zoom + offset.y
        )
    }
}

struct BoxSelectionOverlay: View {
    let start: CGPoint
    let end: CGPoint
    let offset: CGPoint
    let zoom: CGFloat
    
    var body: some View {
        let rect = CGRect(
            x: min(start.x, end.x),
            y: min(start.y, end.y),
            width: abs(end.x - start.x),
            height: abs(end.y - start.y)
        )
        
        Rectangle()
            .strokeBorder(Color.blue, lineWidth: 2)
            .background(Color.blue.opacity(0.1))
            .frame(width: rect.width, height: rect.height)
            .position(
                x: rect.midX * zoom + offset.x,
                y: rect.midY * zoom + offset.y
            )
    }
}

struct SimpleConnectionPath: View {
    let connection: GraphConnection
    let nodes: [GraphNode]
    let offset: CGPoint
    let zoom: CGFloat
    
    var body: some View {
        // Simplified straight line for performance mode
        if let fromNode = nodes.first(where: { $0.id == connection.fromNode }),
           let toNode = nodes.first(where: { $0.id == connection.toNode }) {
            
            let startPoint = CGPoint(
                x: fromNode.position.x + 100, // Approximate node center
                y: fromNode.position.y + 50
            )
            
            let endPoint = CGPoint(
                x: toNode.position.x + 100,
                y: toNode.position.y + 50
            )
            
            Path { path in
                path.move(to: startPoint)
                path.addLine(to: endPoint)
            }
            .stroke(
                connectionColor(for: .flow, isValid: connection.isValid),
                style: StrokeStyle(lineWidth: 2, lineCap: .round)
            )
            .opacity(0.7)
            .position(
                x: offset.x,
                y: offset.y
            )
            .scaleEffect(zoom)
        }
    }
    
    private func connectionColor(for portType: PortType, isValid: Bool) -> Color {
        if !isValid {
            return .red
        }
        
        switch portType {
        case .flow: return .white
        case .bool: return .red
        case .int: return .cyan
        case .float: return .green
        case .string: return .magenta
        case .vector3: return .yellow
        case .object: return .blue
        default: return .gray
        }
    }
}

struct NodeGraphMinimap: View {
    @ObservedObject var manager: NodeGraphManager
    let canvasSize: CGSize
    
    private let minimapSize: CGFloat = 150
    
    var body: some View {
        VStack(spacing: 4) {
            Text("Minimap")
                .font(.caption)
                .foregroundColor(.white)
            
            ZStack {
                // Background
                Rectangle()
                    .fill(Color.black.opacity(0.8))
                    .frame(width: minimapSize, height: minimapSize)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(Color.gray, lineWidth: 1)
                    )
                
                // Nodes
                ForEach(manager.nodes) { node in
                    Circle()
                        .fill(nodeColor(for: node.type))
                        .frame(width: 4, height: 4)
                        .position(
                            x: (node.position.x / 10) + minimapSize/2, // Scale down for minimap
                            y: (node.position.y / 10) + minimapSize/2
                        )
                }
                
                // Viewport indicator
                Rectangle()
                    .stroke(Color.white, lineWidth: 1)
                    .frame(width: minimapSize / 10, height: minimapSize / 10)
                    .position(
                        x: (-manager.panOffset.x / 10) + minimapSize/2 - (minimapSize / 20),
                        y: (-manager.panOffset.y / 10) + minimapSize/2 - (minimapSize / 20)
                    )
            }
        }
        .frame(width: minimapSize, height: minimapSize + 20)
        .background(Color.black.opacity(0.9))
        .cornerRadius(8)
    }
    
    private func nodeColor(for type: NodeType) -> Color {
        switch type {
        case .event: return .green
        case .branch: return .orange
        case .math: return .blue
        case .logic: return .purple
        case .function: return .red
        default: return .gray
        }
    }
}

// MARK: - Quick Add Overlay
struct QuickAddOverlay: View {
    let position: CGPoint
    @Binding var text: String
    let onSubmit: (NodeType) -> Void
    let onCancel: () -> Void
    
    @State private var selectedIndex = 0
    @FocusState private var isFocused: Bool
    
    private var filteredTypes: [NodeType] {
        if text.isEmpty {
            return NodeType.allCases.prefix(10).map { $0 }
        }
        
        let scoredTypes = NodeType.allCases.map { type in
            let score = FuzzySearch.fuzzyMatch(text, type.rawValue)
            return (type: type, score: score)
        }
        
        return scoredTypes
            .filter { $0.score >= 0.2 }
            .sorted { $0.score > $1.score }
            .prefix(10)
            .map { $0.type }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Input field
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.white.opacity(0.6))
                TextField("Quick-add node...", text: $text)
                    .textFieldStyle(.plain)
                    .focused($isFocused)
                    .onSubmit {
                        if let firstType = filteredTypes.first {
                            onSubmit(firstType)
                        }
                    }
                Button("Cancel") {
                    onCancel()
                }
                .foregroundColor(.white.opacity(0.6))
            }
            .padding(12)
            .background(Color.black.opacity(0.8))
            
            // Suggestions list
            if !filteredTypes.isEmpty {
                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 0) {
                        ForEach(Array(filteredTypes.enumerated()), id: \.offset) { index, type in
                            Button(action: {
                                onSubmit(type)
                            }) {
                                HStack {
                                    Image(systemName: iconForType(type))
                                        .font(.system(size: 12))
                                        .foregroundColor(nodeColorForType(type))
                                    
                                    VStack(alignment: .leading, spacing: 2) {
                                        Text(type.rawValue.capitalized)
                                            .foregroundColor(.white)
                                            .font(.caption)
                                        Text(descriptionForType(type))
                                            .foregroundColor(.white.opacity(0.6))
                                            .font(.caption2)
                                    }
                                    
                                    Spacer()
                                    
                                    if index == selectedIndex {
                                        Image(systemName: "arrow.right")
                                            .foregroundColor(.blue)
                                            .font(.caption)
                                    }
                                }
                                .padding(.horizontal, 12)
                                .padding(.vertical, 8)
                                .frame(maxWidth: .infinity, alignment: .leading)
                                .background(index == selectedIndex ? Color.blue.opacity(0.3) : Color.clear)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }
                .frame(maxHeight: 200)
                .background(Color.black.opacity(0.9))
            }
        }
        .frame(width: 350)
        .cornerRadius(8)
        .shadow(radius: 12)
        .position(position)
        .onAppear {
            isFocused = true
        }
        .onReceive(NotificationCenter.default.publisher(for: UIResponder.keyboardWillHideNotification)) { _ in
            if text.isEmpty {
                onCancel()
            }
        }
    }
    
    private func iconForType(_ type: NodeType) -> String {
        // Reuse the icon mapping from EnhancedNodeCreationMenu
        switch type {
        case .event, .onStart, .onUpdate, .onDestroy: return "bolt.fill"
        case .flowBranch: return "arrow.branch"
        case .variable: return "v.square"
        case .constant: return "number"
        case .mathAdd: return "plus.forwardslash.minus"
        case .logicAnd: return "questionmark.diamond"
        case .compareEqual: return "equal"
        case .comment: return "text.bubble"
        case .stringConcat: return "textformat"
        default: return "square"
        }
    }
    
    private func nodeColorForType(_ type: NodeType) -> Color {
        switch type {
        case .event, .onStart, .onUpdate, .onDestroy: return .red
        case .flowBranch: return .gray
        case .variable: return .orange
        case .constant: return .orange
        case .mathAdd: return .teal
        case .logicAnd: return .teal
        case .compareEqual: return .orange
        case .comment: return .yellow.opacity(0.3)
        case .stringConcat: return .pink
        default: return .secondary
        }
    }
    
    private func descriptionForType(_ type: NodeType) -> String {
        switch type {
        case .event: return "Event trigger"
        case .onStart: return "On start event"
        case .onUpdate: return "On update event"
        case .onDestroy: return "On destroy event"
        case .flowBranch: return "Conditional branch"
        case .variable: return "Variable"
        case .constant: return "Constant value"
        case .mathAdd: return "Addition"
        case .logicAnd: return "Logical AND"
        case .compareEqual: return "Equal comparison"
        case .comment: return "Comment"
        case .stringConcat: return "String concatenate"
        default: return "Node"
        }
    }
}

// MARK: - Node Graph Toolbar
struct NodeGraphToolbar: View {
    @ObservedObject var manager: NodeGraphManager
    @State private var showLoadMenu = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Graph selector with save/load
            HStack(spacing: 4) {
                Menu(manager.currentGraphName) {
                    ForEach(manager.getSavedGraphs(), id: \.self) { graphName in
                        Button(action: {
                            if manager.isDirty {
                                // TODO: Show save confirmation dialog
                            }
                            manager.loadGraph(name: graphName)
                        }) {
                            Text(graphName)
                        }
                    }
                    Divider()
                    Button("New Graph...") {
                        // TODO: Show new graph dialog
                    }
                }
                
                Button(action: manager.saveGraph) {
                    Image(systemName: manager.isDirty ? "square.and.arrow.down.fill" : "square.and.arrow.down")
                        .foregroundColor(manager.isDirty ? .orange : .primary)
                }
                .disabled(!manager.isDirty)
                .help("Save Graph")
            }
            
            Divider().frame(height: 20)
            
            // Compile
            Button(action: { manager.compileGraph() }) {
                Label("Compile", systemImage: "hammer")
            }
            
            // Optimize
            Button(action: { manager.optimizeGraph() }) {
                Label("Optimize", systemImage: "gear")
            }
            
            // Compile to Bytecode
            Button(action: { manager.compileToBytecode() }) {
                Label("Bytecode", systemImage: "cpu")
            }
            
            // Show/Hide Compilation Panel
            Button(action: { manager.showCompilationPanel.toggle() }) {
                Label("Errors", systemImage: "exclamationmark.triangle")
            }
            .overlay(
                Circle()
                    .fill(Color.red)
                    .frame(width: 8, height: 8)
                    .offset(x: 8, y: -8),
                alignment: .topTrailing
            )
            .opacity(CompilationManager.shared.errors.isEmpty ? 0 : 1)
            
            // Debug Controls
            if DebugManager.shared.isDebugging {
                // Continue
                Button(action: { DebugManager.shared.continueExecution() }) {
                    Image(systemName: "play.fill")
                }
                .disabled(DebugManager.shared.isRunning)
                .help("Continue Execution")
                
                // Pause
                Button(action: { DebugManager.shared.pauseExecution() }) {
                    Image(systemName: "pause.fill")
                }
                .disabled(!DebugManager.shared.isRunning)
                .help("Pause Execution")
                
                // Stop
                Button(action: { DebugManager.shared.stopDebugging() }) {
                    Image(systemName: "stop.fill")
                }
                .help("Stop Debugging")
                
                // Step Controls
                Divider().frame(height: 20)
                
                Button(action: { DebugManager.shared.stepThrough() }) {
                    Image(systemName: "stepforward")
                }
                .disabled(DebugManager.shared.isRunning)
                .help("Step Through")
                
                Button(action: { DebugManager.shared.stepIntoFunction() }) {
                    Image(systemName: "arrow.right.square")
                }
                .disabled(DebugManager.shared.isRunning)
                .help("Step Into Function")
                
                Button(action: { DebugManager.shared.stepOverNode() }) {
                    Image(systemName: "arrow.right.circle")
                }
                .disabled(DebugManager.shared.isRunning)
                .help("Step Over Node")
                
                Button(action: { DebugManager.shared.stepOutOfFunction() }) {
                    Image(systemName: "arrow.up.left.square")
                }
                .disabled(DebugManager.shared.isRunning)
                .help("Step Out of Function")
                
                // Debug Panels
                Divider().frame(height: 20)
                
                Button(action: { manager.showDebugPanels.toggle() }) {
                    Label("Debug Panels", systemImage: "list.bullet.rectangle")
                }
            } else {
                // Start Debug
                Button(action: { DebugManager.shared.startDebugging() }) {
                    Label("Debug", systemImage: "ladybug")
                }
                .help("Start Debugging")
            }
            
            // Hot Reload
            Button(action: manager.toggleHotReload) {
                Image(systemName: manager.hotReloadEnabled ? "flame.fill" : "flame")
                    .foregroundColor(manager.hotReloadEnabled ? .orange : .secondary)
            }
            .help("Toggle Hot Reload")
            
            // Export
            Menu {
                Button("Export as JSON") {
                    manager.saveGraph()
                }
                Button("Export as YAML (VC Friendly)") {
                    manager.exportToVCFormat()
                }
            } label: {
                Image(systemName: "square.and.arrow.up")
            }
            .help("Export Graph")
            
            // Import
            Menu {
                Button("Import from JSON") {
                    // TODO: Show file picker for JSON import
                }
                Button("Import from YAML") {
                    // TODO: Show file picker for YAML import
                }
                Button("Import from DOT/GraphViz") {
                    // TODO: Show file picker for DOT import
                }
                Divider()
                Button("Import from Blueprint") {
                    // TODO: Show file picker for Blueprint import
                }
                Button("Import from Visual Script") {
                    // TODO: Show file picker for Visual Script import
                }
                Button("Import from GraphML") {
                    // TODO: Show file picker for GraphML import
                }
            } label: {
                Image(systemName: "square.and.arrow.down")
            }
            .help("Import Graph")
            
            Spacer()
            
            // Zoom controls
            HStack(spacing: 4) {
                Button(action: { manager.zoom = max(0.5, manager.zoom - 0.1) }) {
                    Image(systemName: "minus.magnifyingglass")
                }
                .buttonStyle(.plain)
                
                Text("\(Int(manager.zoom * 100))%")
                    .font(DesignSystem.Typography.small)
                    .frame(width: 40)
                
                Button(action: { manager.zoom = min(2.0, manager.zoom + 0.1) }) {
                    Image(systemName: "plus.magnifyingglass")
                }
                .buttonStyle(.plain)
                
                Button(action: { manager.zoom = 1.0; manager.panOffset = .zero }) {
                    Image(systemName: "arrow.up.left.and.arrow.down.right")
                }
                .buttonStyle(.plain)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
    }
}

// MARK: - Node Graph Grid
struct NodeGraphGrid: View {
    let offset: CGPoint
    let zoom: CGFloat
    let gridSize: CGFloat
    let pattern: GridPattern
    
    var body: some View {
        Canvas { context, size in
            let scaledGridSize = gridSize * zoom
            
            switch pattern {
            case .none:
                // No grid
                break
                
            case .dots:
                drawDotGrid(context: context, size: size, scaledGridSize: scaledGridSize)
                
            case .lines:
                drawLineGrid(context: context, size: size, scaledGridSize: scaledGridSize)
                
            case .cross:
                drawCrossGrid(context: context, size: size, scaledGridSize: scaledGridSize)
            }
        }
    }
    
    private func drawDotGrid(context: GraphicsContext, size: CGSize, scaledGridSize: CGFloat) {
        // Minor dots
        for x in stride(from: offset.x.truncatingRemainder(dividingBy: scaledGridSize), to: size.width, by: scaledGridSize) {
            for y in stride(from: offset.y.truncatingRemainder(dividingBy: scaledGridSize), to: size.height, by: scaledGridSize) {
                context.fill(
                    Path(ellipseIn: CGRect(x: x-1, y: y-1, width: 2, height: 2)),
                    with: .color(Color.white.opacity(0.1))
                )
            }
        }
        
        // Major dots (every 5)
        let majorGridSize = scaledGridSize * 5
        for x in stride(from: offset.x.truncatingRemainder(dividingBy: majorGridSize), to: size.width, by: majorGridSize) {
            for y in stride(from: offset.y.truncatingRemainder(dividingBy: majorGridSize), to: size.height, by: majorGridSize) {
                context.fill(
                    Path(ellipseIn: CGRect(x: x-1.5, y: y-1.5, width: 3, height: 3)),
                    with: .color(Color.white.opacity(0.2))
                )
            }
        }
    }
    
    private func drawLineGrid(context: GraphicsContext, size: CGSize, scaledGridSize: CGFloat) {
        // Minor lines
        for x in stride(from: offset.x.truncatingRemainder(dividingBy: scaledGridSize), to: size.width, by: scaledGridSize) {
            var path = Path()
            path.move(to: CGPoint(x: x, y: 0))
            path.addLine(to: CGPoint(x: x, y: size.height))
            context.stroke(path, with: .color(Color.white.opacity(0.05)), lineWidth: 0.5)
        }
        
        for y in stride(from: offset.y.truncatingRemainder(dividingBy: scaledGridSize), to: size.height, by: scaledGridSize) {
            var path = Path()
            path.move(to: CGPoint(x: 0, y: y))
            path.addLine(to: CGPoint(x: size.width, y: y))
            context.stroke(path, with: .color(Color.white.opacity(0.05)), lineWidth: 0.5)
        }
        
        // Major lines (every 5)
        let majorGridSize = scaledGridSize * 5
        for x in stride(from: offset.x.truncatingRemainder(dividingBy: majorGridSize), to: size.width, by: majorGridSize) {
            var path = Path()
            path.move(to: CGPoint(x: x, y: 0))
            path.addLine(to: CGPoint(x: x, y: size.height))
            context.stroke(path, with: .color(Color.white.opacity(0.1)), lineWidth: 1)
        }
        
        for y in stride(from: offset.y.truncatingRemainder(dividingBy: majorGridSize), to: size.height, by: majorGridSize) {
            var path = Path()
            path.move(to: CGPoint(x: 0, y: y))
            path.addLine(to: CGPoint(x: size.width, y: y))
            context.stroke(path, with: .color(Color.white.opacity(0.1)), lineWidth: 1)
        }
    }
    
    private func drawCrossGrid(context: GraphicsContext, size: CGSize, scaledGridSize: CGFloat) {
        // Draw cross pattern at grid intersections
        for x in stride(from: offset.x.truncatingRemainder(dividingBy: scaledGridSize), to: size.width, by: scaledGridSize) {
            for y in stride(from: offset.y.truncatingRemainder(dividingBy: scaledGridSize), to: size.height, by: scaledGridSize) {
                let centerX = x + scaledGridSize / 2
                let centerY = y + scaledGridSize / 2
                
                // Draw cross
                let crossSize: CGFloat = 4
                var path = Path()
                path.move(to: CGPoint(x: centerX - crossSize/2, y: centerY))
                path.addLine(to: CGPoint(x: centerX + crossSize/2, y: centerY))
                path.move(to: CGPoint(x: centerX, y: centerY - crossSize/2))
                path.addLine(to: CGPoint(x: centerX, y: centerY + crossSize/2))
                
                context.stroke(path, with: .color(Color.white.opacity(0.15)), lineWidth: 1)
            }
        }
    }
}

// MARK: - Node View
struct NodeView: View {
    @Binding var node: GraphNode
    let isSelected: Bool
    let offset: CGPoint
    let zoom: CGFloat
    let isFiltered: Bool
    let isSearchMatch: Bool
    
    @State private var hasError = false
    @State private var hasWarning = false
    @State private var showTooltip = false
    @State private var showDocumentation = false
    @State private var showDeleteConfirmation = false
    @State private var showTypeReplacement = false
    @State private var isDragging = false
    
    private var compilationManager = CompilationManager.shared
    private var customizationManager = NodeCustomizationManager.shared
    private var clipboardManager = NodeClipboardManager.shared
    
    var nodeColor: Color {
        if let customColor = node.customColor {
            return customColor
        }
        return customizationManager.getColor(for: node.type)
    }
    
    var nodeIcon: String {
        if let customIcon = node.customIcon {
            return customIcon
        }
        return customizationManager.getIcon(for: node.type)
    }
    
    var hasBreakpoint: Bool {
        DebugManager.shared.breakpoints.contains(node.id)
    }
    
    var isCurrentlyExecuting: Bool {
        DebugManager.shared.currentExecutionNode == node.id
    }
    
    var effectiveNodeSize: CGSize {
        if let customSize = node.customSize {
            return customSize
        }
        
        let width = max(node.minWidth, calculateContentWidth())
        let height = max(node.minHeight, calculateContentHeight())
        return CGSize(width: width, height: height)
    }
    
    var body: some View {
        if node.type == .comment {
            CommentNodeView(node: $node, isSelected: isSelected, offset: offset, zoom: zoom)
        } else {
            // Regular node
            VStack(spacing: 0) {
                // Header with collapse toggle and indicators
                headerView
                    .background(nodeColor.opacity(node.isDisabled ? 0.5 : 1.0))
                
                if !node.isHeaderCollapsed && !node.isCollapsed {
                    // Preview section
                    if node.previewValue != nil {
                        previewSection
                    }
                    
                    // Body with ports
                    bodySection
                }
            }
            .frame(width: effectiveNodeSize.width, height: effectiveNodeSize.height)
            .background(nodeBackgroundColor)
            .cornerRadius(8)
            .overlay(
                RoundedRectangle(cornerRadius: 8)
                    .stroke(borderColor, lineWidth: borderLineWidth)
            )
            .shadow(color: .black.opacity(0.3), radius: 4, x: 2, y: 2)
            .position(
                x: node.position.x * zoom + offset.x,
                y: node.position.y * zoom + offset.y
            )
            .opacity(node.isDisabled ? 0.6 : 1.0)
            .overlay(
                // Bypass indicator
                node.isBypassed ? 
                    VStack {
                        Spacer()
                        HStack {
                            Spacer()
                            Text("BYPASSED")
                                .font(.caption2)
                                .foregroundColor(.orange)
                                .padding(.horizontal, 4)
                                .padding(.vertical, 2)
                                .background(Color.orange.opacity(0.2))
                                .cornerRadius(4)
                        }
                    }
                    : nil
            )
            .onHover { hovering in
                showTooltip = hovering
            }
            .popover(isPresented: $showTooltip) {
                NodeTooltip(node: node)
            }
            .sheet(isPresented: $showDocumentation) {
                DocumentationView(nodeType: node.type)
            }
            .alert("Delete Node", isPresented: $showDeleteConfirmation) {
                Button("Cancel", role: .cancel) { }
                Button("Delete", role: .destructive) {
                    NodeGraphManager.shared.deleteNode(node.id)
                }
            } message: {
                Text("Are you sure you want to delete '\(node.title)'? This action cannot be undone.")
            }
            .sheet(isPresented: $showTypeReplacement) {
                NodeTypeReplacementView(node: $node)
            }
        }
    }
    .gesture(
        DragGesture()
            .onChanged { value in
                isDragging = true
                node.position = CGPoint(
                    x: (value.location.x - offset.x) / zoom,
                    y: (value.location.y - offset.y) / zoom
                )
            }
            .onEnded { _ in
                isDragging = false
            }
    )
    .onAppear {
        updateErrorStatus()
        if node.tempTitle.isEmpty {
            node.tempTitle = node.title
        }
    }
    .onChange(of: compilationManager.errors) { _ in
        updateErrorStatus()
    }
    .onChange(of: compilationManager.warnings) { _ in
        updateErrorStatus()
    }
    .contextMenu {
        contextMenuItems
    }
    .keyboardShortcut(.space, modifiers: []) {
        // Toggle breakpoint
        DebugManager.shared.toggleBreakpoint(on: node.id)
    }
    .keyboardShortcut(.return, modifiers: .command) {
        // Start inline rename
        node.isRenaming = true
        node.tempTitle = node.title
    }
    .keyboardShortcut(.d, modifiers: .command) {
        // Duplicate node
        duplicateNode()
    }
    .keyboardShortcut(.c, modifiers: .command) {
        // Copy node
        copyNode()
    }
    
    private var headerView: some View {
        HStack {
            // Collapse/expand toggle
            Button(action: {
                node.isHeaderCollapsed.toggle()
            }) {
                Image(systemName: node.isHeaderCollapsed ? "chevron.right" : "chevron.down")
                    .font(.system(size: 10))
                    .foregroundColor(.white)
            }
            .buttonStyle(PlainButtonStyle())
            
            // Icon and title
            HStack(spacing: 4) {
                Image(systemName: nodeIcon)
                    .font(.system(size: 12))
                
                if node.isRenaming {
                    TextField("Node name", text: $node.tempTitle)
                        .textFieldStyle(PlainTextFieldStyle())
                        .font(DesignSystem.Typography.bodyBold)
                        .onSubmit {
                            node.title = node.tempTitle
                            node.isRenaming = false
                        }
                        .onExitCommand {
                            node.tempTitle = node.title
                            node.isRenaming = false
                        }
                } else {
                    Text(node.title)
                        .font(DesignSystem.Typography.bodyBold)
                }
            }
            
            Spacer()
            
            // Indicators and controls
            HStack(spacing: 4) {
                // Documentation button
                Button(action: { showDocumentation = true }) {
                    Image(systemName: "info.circle")
                        .font(.system(size: 10))
                        .foregroundColor(.white.opacity(0.7))
                }
                .buttonStyle(PlainButtonStyle())
                
                // Error and warning indicators
                if node.hasError || hasError {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundColor(.red)
                        .font(.system(size: 10))
                } else if node.hasWarning || hasWarning {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(.orange)
                        .font(.system(size: 10))
                }
                
                // Breakpoint indicator
                if hasBreakpoint {
                    Image(systemName: "pause.circle.fill")
                        .foregroundColor(.red)
                        .font(.system(size: 10))
                }
                
                // Execution indicator
                if isCurrentlyExecuting {
                    Image(systemName: "play.circle.fill")
                        .foregroundColor(.green)
                        .font(.system(size: 10))
                }
                
                // Disable/bypass indicator
                if node.isDisabled {
                    Image(systemName: "power.slash")
                        .foregroundColor(.gray)
                        .font(.system(size: 10))
                }
            }
        }
        .foregroundColor(.white)
        .padding(.horizontal, 8)
        .padding(.vertical, 6)
    }
    
    private var previewSection: some View {
        VStack {
            Divider()
                .background(Color.white.opacity(0.2))
            
            HStack {
                Text("Preview:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(formatPreviewValue(node.previewValue))
                    .font(.caption)
                    .foregroundColor(.primary)
                    .padding(.horizontal, 4)
                    .padding(.vertical, 2)
                    .background(Color.white.opacity(0.1))
                    .cornerRadius(4)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
        }
    }
    
    private var bodySection: some View {
        HStack(alignment: .top, spacing: 0) {
            // Input ports
            VStack(alignment: .leading, spacing: 4) {
                ForEach(node.inputs) { port in
                    EnhancedPortView(port: port, isOutput: false, nodeId: node.id)
                }
            }
            .frame(minWidth: 60)
            
            Spacer()
            
            // Output ports
            VStack(alignment: .trailing, spacing: 4) {
                ForEach(node.outputs) { port in
                    EnhancedPortView(port: port, isOutput: true, nodeId: node.id)
                }
            }
            .frame(minWidth: 60)
        }
        .padding(8)
        .background(Color(red: 0.15, green: 0.15, blue: 0.18))
    }
    
    private var nodeBackgroundColor: Color {
        Color(red: 0.15, green: 0.15, blue: 0.18)
    }
    
    private var borderColor: Color {
        if node.hasError || hasError {
            return .red
        } else if node.hasWarning || hasWarning {
            return .orange
        } else if isSelected {
            return DesignSystem.Colors.accentPrimary
        } else {
            return .clear
        }
    }
    
    private var borderLineWidth: CGFloat {
        if node.hasError || hasError || node.hasWarning || hasWarning {
            return 3
        } else {
            return 2
        }
    }
    
    private var contextMenuItems: some View {
        Group {
            // Basic operations
            Button(action: {
                node.isRenaming = true
                node.tempTitle = node.title
            }) {
                Label("Rename", systemImage: "text.cursor")
            }
            
            Button(action: {
                duplicateNode()
            }) {
                Label("Duplicate", systemImage: "doc.on.doc")
            }
            
            Button(action: {
                copyNode()
            }) {
                Label("Copy", systemImage: "doc.on.clipboard")
            }
            
            Divider()
            
            // State management
            Button(action: {
                node.isCollapsed.toggle()
            }) {
                Label(node.isCollapsed ? "Expand" : "Collapse", systemImage: node.isCollapsed ? "expandall" : "collapseall")
            }
            
            Button(action: {
                node.isDisabled.toggle()
            }) {
                Label(node.isDisabled ? "Enable" : "Disable", systemImage: node.isDisabled ? "power" : "power.slash")
            }
            
            Button(action: {
                node.isBypassed.toggle()
            }) {
                Label(node.isBypassed ? "Unbypass" : "Bypass", systemImage: "arrow.trianglehead.2")
            }
            
            Divider()
            
            // Debug operations
            Button(action: {
                DebugManager.shared.toggleBreakpoint(on: node.id)
            }) {
                Label(hasBreakpoint ? "Remove Breakpoint" : "Set Breakpoint", 
                      systemImage: hasBreakpoint ? "pause.circle" : "pause.circle.fill")
            }
            
            if hasBreakpoint {
                Button("Conditional Breakpoint...") {
                    // Show conditional breakpoint dialog
                }
                
                Button("Hit Count Breakpoint...") {
                    // Show hit count breakpoint dialog
                }
                
                Button("Log Point...") {
                    DebugManager.shared.addLogPoint(on: node.id, message: "Node \(node.title) executed")
                }
            }
            
            Divider()
            
            // Advanced operations
            Button(action: {
                showTypeReplacement = true
            }) {
                Label("Replace Type...", systemImage: "arrow.triangle.2.circlepath")
            }
            
            Button(action: {
                // Extract to subgraph
            }) {
                Label("Extract to Subgraph", systemImage: "square.stack.3d.up")
            }
            
            Button(action: {
                // Convert to function
            }) {
                Label("Convert to Function", systemImage: "function")
            }
            
            Divider()
            
            // Destructive operations
            Button(role: .destructive, action: {
                showDeleteConfirmation = true
            }) {
                Label("Delete", systemImage: "trash")
            }
        }
    }
    
    private func calculateContentWidth() -> CGFloat {
        let titleWidth = (node.title as NSString).size(withAttributes: [.font: DesignSystem.Typography.bodyBold]).width
        let portsWidth = max(node.inputs.count, node.outputs.count) * 60.0
        return max(150.0, titleWidth + portsWidth + 40.0)
    }
    
    private func calculateContentHeight() -> CGFloat {
        let headerHeight: CGFloat = 30.0
        let portsHeight = max(node.inputs.count, node.outputs.count) * 25.0
        let previewHeight = node.previewValue != nil ? 30.0 : 0.0
        return headerHeight + portsHeight + previewHeight + 20.0
    }
    
    private func updateErrorStatus() {
        hasError = compilationManager.errors.contains { $0.nodeId == node.id }
        hasWarning = compilationManager.warnings.contains { $0.nodeId == node.id }
    }
    
    private func duplicateNode() {
        var duplicatedNode = node
        duplicatedNode.id = UUID()
        duplicatedNode.position = CGPoint(x: node.position.x + 50, y: node.position.y + 50)
        duplicatedNode.title = "\(node.title) Copy"
        NodeGraphManager.shared.addNode(duplicatedNode)
    }
    
    private func copyNode() {
        let connections = NodeGraphManager.shared.connections.filter { 
            $0.fromNode == node.id || $0.toNode == node.id 
        }
        clipboardManager.copyNodes([node], connections: connections)
    }
    
    private func formatPreviewValue(_ value: Any?) -> String {
        guard let value = value else { return "null" }
        
        if let stringValue = value as? String {
            return "\"\(stringValue)\""
        } else if let numericValue = value as? NSNumber {
            return numericValue.stringValue
        } else if let boolValue = value as? Bool {
            return boolValue.description
        } else if let vectorValue = value as? [Double] {
            return "(\(vectorValue.map { String(format: "%.2f", $0) }.joined(separator: ", ")))"
        } else {
            return String(describing: value)
        }
    }
}

// MARK: - Enhanced Port View
struct EnhancedPortView: View {
    @Binding var port: NodePort
    let isOutput: Bool
    let nodeId: UUID
    
    @State private var isHovering = false
    @State private var isEditing = false
    @State private var editValue = ""
    @State private var showTooltip = false
    
    private var customizationManager = NodeCustomizationManager.shared
    
    var body: some View {
        HStack(spacing: 4) {
            // Port circle
            portCircle
            
            // Port name and value
            VStack(alignment: isOutput ? .trailing : .leading, spacing: 2) {
                Text(port.name)
                    .font(.caption)
                    .foregroundColor(.white)
                
                // Value display/editing
                if isOutput {
                    // Output value preview
                    if let value = port.currentValue {
                        Text(formatValue(value))
                            .font(.caption2)
                            .foregroundColor(.secondary)
                            .padding(.horizontal, 4)
                            .padding(.vertical, 1)
                            .background(Color.white.opacity(0.1))
                            .cornerRadius(3)
                    }
                } else if !isOutput && port.isEditable {
                    // Input value editing
                    if isEditing {
                        HStack {
                            TextField("Value", text: $editValue)
                                .textFieldStyle(RoundedBorderTextFieldStyle())
                                .font(.caption2)
                                .frame(width: 80)
                            
                            Button("Done") {
                                updatePortValue()
                                isEditing = false
                            }
                            .font(.caption2)
                            
                            Button("Cancel") {
                                isEditing = false
                            }
                            .font(.caption2)
                        }
                    } else {
                        Button(action: { isEditing = true }) {
                            HStack {
                                Text(formatValue(port.currentValue ?? port.defaultValue))
                                    .font(.caption2)
                                    .foregroundColor(.secondary)
                                
                                Image(systemName: "pencil")
                                    .font(.caption2)
                                    .foregroundColor(.secondary)
                            }
                            .padding(.horizontal, 4)
                            .padding(.vertical, 1)
                            .background(Color.white.opacity(0.05))
                            .cornerRadius(3)
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
                
                // Error indicator
                if port.hasError {
                    Text(port.errorMessage ?? "Error")
                        .font(.caption2)
                        .foregroundColor(.red)
                }
            }
        }
        .onHover { hovering in
            isHovering = hovering
            showTooltip = hovering
        }
        .popover(isPresented: $showTooltip) {
            PortTooltip(port: port)
        }
    }
    
    private var portCircle: some View {
        Circle()
            .fill(portColor)
            .frame(width: 8, height: 8)
            .overlay(
                Circle()
                    .stroke(Color.white, lineWidth: isHovering ? 2 : 1)
            )
    }
    
    private var portColor: Color {
        switch port.type {
        case .flow: return .white
        case .boolean: return .red
        case .int: return .cyan
        case .float: return .green
        case .vector2: return .yellow.opacity(0.7)
        case .vector3: return .yellow
        case .vector4: return .orange
        case .string: return .magenta
        case .object: return .blue
        case .struct: return .purple
        case .enum: return .teal
        case .asset: return .gold
        case .delegate: return .pink
        case .array: return .orange
        case .dictionary: return .purple
        case .any: return .gray
        }
    }
    
    private func formatValue(_ value: Any?) -> String {
        guard let value = value else { return "null" }
        
        if let stringValue = value as? String {
            return "\"\(stringValue)\""
        } else if let numericValue = value as? NSNumber {
            return numericValue.stringValue
        } else if let boolValue = value as? Bool {
            return boolValue.description
        } else {
            return String(describing: value)
        }
    }
    
    private func updatePortValue() {
        // Convert string value based on port type
        switch port.type {
        case .string:
            port.currentValue = editValue
        case .int:
            port.currentValue = Int(editValue)
        case .float:
            port.currentValue = Double(editValue)
        case .boolean:
            port.currentValue = Bool(editValue.lowercased() == "true")
        default:
            port.currentValue = editValue
        }
    }
}

// MARK: - Port Tooltip
struct PortTooltip: View {
    let port: NodePort
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text(port.name)
                    .font(.headline)
                Spacer()
                Text(String(describing: port.type))
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            if !port.description.isEmpty {
                Text(port.description)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            if let defaultValue = port.defaultValue {
                HStack {
                    Text("Default:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text(formatValue(defaultValue))
                        .font(.caption)
                }
            }
            
            if let currentValue = port.currentValue {
                HStack {
                    Text("Current:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text(formatValue(currentValue))
                        .font(.caption)
                }
            }
        }
        .padding()
        .frame(width: 200)
        .background(Color(.controlBackgroundColor))
        .cornerRadius(8)
        .shadow(radius: 4)
    }
    
    private func formatValue(_ value: Any) -> String {
        if let stringValue = value as? String {
            return "\"\(stringValue)\""
        } else if let numericValue = value as? NSNumber {
            return numericValue.stringValue
        } else if let boolValue = value as? Bool {
            return boolValue.description
        } else {
            return String(describing: value)
        }
    }
}

// MARK: - Node Tooltip
struct NodeTooltip: View {
    let node: GraphNode
    
    private var customizationManager = NodeCustomizationManager.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: customizationManager.getIcon(for: node.type))
                    .foregroundColor(customizationManager.getColor(for: node.type))
                Text(node.title)
                    .font(.headline)
                Spacer()
                Text(String(describing: node.type))
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            if !node.description.isEmpty {
                Text(node.description)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            // Status indicators
            VStack(alignment: .leading, spacing: 2) {
                if node.hasError {
                    HStack {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(.red)
                        Text(node.errorMessage ?? "Error")
                            .font(.caption)
                            .foregroundColor(.red)
                    }
                }
                
                if node.hasWarning {
                    HStack {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(.orange)
                        Text(node.warningMessage ?? "Warning")
                            .font(.caption)
                            .foregroundColor(.orange)
                    }
                }
                
                if node.isDisabled {
                    HStack {
                        Image(systemName: "power.slash")
                            .foregroundColor(.gray)
                        Text("Disabled")
                            .font(.caption)
                            .foregroundColor(.gray)
                    }
                }
                
                if node.isBypassed {
                    HStack {
                        Image(systemName: "arrow.trianglehead.2")
                            .foregroundColor(.orange)
                        Text("Bypassed")
                            .font(.caption)
                            .foregroundColor(.orange)
                    }
                }
            }
            
            // Port summary
            if !node.inputs.isEmpty || !node.outputs.isEmpty {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Ports:")
                        .font(.caption)
                        .fontWeight(.bold)
                    
                    if !node.inputs.isEmpty {
                        Text("Inputs: \(node.inputs.count)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                    
                    if !node.outputs.isEmpty {
                        Text("Outputs: \(node.outputs.count)")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }
            }
        }
        .padding()
        .frame(width: 250)
        .background(Color(.controlBackgroundColor))
        .cornerRadius(8)
        .shadow(radius: 4)
    }
}

// MARK: - Node Type Replacement View
struct NodeTypeReplacementView: View {
    @Binding var node: GraphNode
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedType: NodeType = .event
    @State private var searchText = ""
    
    private let allNodeTypes: [NodeType] = [
        .event, .branch, .sequence, .forLoop, .forEach,
        .getter, .setter, .function, .math, .logic,
        .variable, .constant, .comment, .macro, .graphInstance
    ]
    
    var filteredTypes: [NodeType] {
        if searchText.isEmpty {
            return allNodeTypes
        } else {
            return allNodeTypes.filter { 
                String(describing: $0).localizedCaseInsensitiveContains(searchText)
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Replace Node Type")
                .font(.title2)
                .fontWeight(.bold)
            
            Text("Replace '\(node.title)' with a different node type")
                .font(.caption)
                .foregroundColor(.secondary)
            
            // Search
            TextField("Search node types...", text: $searchText)
                .textFieldStyle(RoundedBorderTextFieldStyle())
            
            // Type list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach(filteredTypes, id: \.self) { type in
                        Button(action: {
                            selectedType = type
                        }) {
                            HStack {
                                Image(systemName: NodeCustomizationManager.shared.getIcon(for: type))
                                    .foregroundColor(NodeCustomizationManager.shared.getColor(for: type))
                                Text(String(describing: type))
                                Spacer()
                                if selectedType == type {
                                    Image(systemName: "checkmark")
                                        .foregroundColor(.accentColor)
                                }
                            }
                            .padding()
                            .background(selectedType == type ? Color.accentColor.opacity(0.1) : Color.clear)
                            .cornerRadius(8)
                        }
                        .buttonStyle(PlainButtonStyle())
                    }
                }
            }
            .frame(maxHeight: 300)
            
            // Actions
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Replace") {
                    // Perform type replacement
                    node.type = selectedType
                    node.title = String(describing: selectedType)
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(selectedType == node.type)
            }
        }
        .padding()
        .frame(width: 400, height: 500)
    }
}

// MARK: - Comment Node View
struct CommentNodeView: View {
    @Binding var node: GraphNode
    let isSelected: Bool
    let offset: CGPoint
    let zoom: CGFloat
    @State private var isEditing = false
    @State private var editText = ""
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: "text.bubble")
                    .font(.system(size: 12))
                    .foregroundColor(.yellow)
                Text("Comment")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.yellow)
                Spacer()
                Button(action: { isEditing = true }) {
                    Image(systemName: "pencil")
                        .font(.system(size: 10))
                        .foregroundColor(.yellow.opacity(0.8))
                }
                .buttonStyle(.plain)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(Color.yellow.opacity(0.2))
            
            // Comment body
            VStack(alignment: .leading, spacing: 4) {
                if isEditing {
                    TextEditor(text: $editText)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.black)
                        .background(Color.yellow.opacity(0.9))
                        .frame(minHeight: 60, maxHeight: 200)
                        .onAppear {
                            editText = node.comment
                        }
                        .onSubmit {
                            node.comment = editText
                            isEditing = false
                        }
                } else {
                    Text(node.comment.isEmpty ? "Click edit to add comment..." : node.comment)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(node.comment.isEmpty ? .yellow.opacity(0.6) : .black)
                        .frame(minHeight: 40, alignment: .topLeading)
                        .padding(4)
                        .onTapGesture {
                            isEditing = true
                            editText = node.comment
                        }
                }
            }
            .padding(8)
            .background(Color.yellow.opacity(0.3))
        }
        .frame(width: 200)
        .background(Color.yellow.opacity(0.4))
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : Color.yellow.opacity(0.6), lineWidth: 2)
        )
        .shadow(color: .black.opacity(0.2), radius: 4, x: 2, y: 2)
        .position(
            x: node.position.x * zoom + offset.x,
            y: node.position.y * zoom + offset.y
        )
    }
}

// MARK: - Port View
struct PortView: View {
    let port: NodePort
    let isOutput: Bool
    @State private var isHovering = false
    @State private var showDefaultValueEditor = false
    @AppStorage("showPortNames") private var showPortNames = true
    
    var body: some View {
        HStack(spacing: 4) {
            if !isOutput {
                PortConnector(type: port.type, isHovering: isHovering)
                
                // Default value indicator and editor
                if !isOutput && port.defaultValue != nil {
                    Button(action: { showDefaultValueEditor = true }) {
                        Image(systemName: "slider.horizontal.3")
                            .font(.system(size: 8))
                            .foregroundColor(.white.opacity(0.6))
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: $showDefaultValueEditor) {
                        PortDefaultValueEditor(port: port)
                    }
                }
            }
            
            if showPortNames {
                Text(port.name)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white.opacity(0.8))
            }
            
            if isOutput {
                PortConnector(type: port.type, isHovering: isHovering)
            }
        }
        .onHover { isHovering = $0 }
        .contextMenu {
            Button(action: {
                UserDefaults.standard.set(!showPortNames, forKey: "showPortNames")
            }) {
                Label(showPortNames ? "Hide Port Names" : "Show Port Names", systemImage: showPortNames ? "eye.slash" : "eye")
            }
            
            if !isOutput {
                Button(action: { showDefaultValueEditor = true }) {
                    Label("Edit Default Value", systemImage: "slider.horizontal.3")
                }
            }
        }
    }
}

struct PortDefaultValueEditor: View {
    let port: NodePort
    @State private var defaultValue: String = ""
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Default Value for \(port.name)")
                .font(.headline)
            
            Text("Type: \(port.type.rawValue)")
                .font(.caption)
                .foregroundColor(.secondary)
            
            switch port.type {
            case .bool:
                Toggle("Default Value", isOn: Binding(
                    get: { defaultValue.lowercased() == "true" },
                    set: { defaultValue in $0 ? "true" : "false" }
                ))
                
            case .int, .float:
                TextField("Default value", text: $defaultValue)
                    .textFieldStyle(.roundedBorder)
                    .keyboardType(port.type == .int ? .numberPad : .decimalPad)
                
            case .string:
                TextField("Default value", text: $defaultValue)
                    .textFieldStyle(.roundedBorder)
                
            case .vector2, .vector3, .vector4:
                VStack(alignment: .leading, spacing: 4) {
                    ForEach(0..<port.type.componentCount, id: \.self) { index in
                        HStack {
                            Text(["X", "Y", "Z", "W"][index])
                                .frame(width: 20)
                            TextField("0", text: Binding(
                                get: { defaultValue.components(separatedBy: ",")[safe: index] ?? "0" },
                                set: { newValue in
                                    var components = defaultValue.components(separatedBy: ",")
                                    while components.count <= index { components.append("0") }
                                    components[index] = newValue
                                    defaultValue = components.joined(separator: ",")
                                }
                            ))
                            .textFieldStyle(.roundedBorder)
                            .keyboardType(.decimalPad)
                        }
                    }
                }
                
            default:
                TextField("Default value", text: $defaultValue)
                    .textFieldStyle(.roundedBorder)
            }
            
            HStack {
                Spacer()
                Button("Cancel") { dismiss() }
                Button("Apply") {
                    // Apply default value to port
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding()
        .frame(width: 250)
        .onAppear {
            defaultValue = port.defaultValue as? String ?? ""
        }
    }
}

extension Array {
    subscript(safe index: Index) -> Element? {
        return indices.contains(index) ? self[index] : nil
    }
}

extension PortType {
    var componentCount: Int {
        switch self {
        case .vector2:
            return 2
        case .vector3:
            return 3
        case .vector4, .quaternion:
            return 4
        default:
            return 1
        }
    }
    
    func canConnect(to other: PortType) -> Bool {
        // Same types are always compatible
        if self == other { return true }
        
        // Any/wildcard types are compatible with everything
        if self.isGeneric || other.isGeneric { return true }
        
        // Numeric type conversions
        let numericTypes: [PortType] = [.int, .float, .vector2, .vector3, .vector4]
        if numericTypes.contains(self) && numericTypes.contains(other) {
            return true
        }
        
        // Flow type compatibility
        if self == .flow && other == .flow { return true }
        
        // Array to array compatibility (same element type)
        if self == .array && other == .array { return true }
        
        // Object hierarchy compatibility
        let objectTypes: [PortType] = [.object, .structType, .classType]
        if objectTypes.contains(self) && objectTypes.contains(other) { return true }
        
        return false
    }
    
    func convertValue(from value: Any, fromType: PortType) -> Any? {
        // If types are the same, no conversion needed
        if self == fromType { return value }
        
        // Handle numeric conversions
        switch (fromType, self) {
        case (.int, .float):
            return (value as? Int).map { Float($0) }
        case (.float, .int):
            return (value as? Float).map { Int($0) }
        case (.vector2, .vector3):
            if let vec2 = value as? String {
                let components = vec2.components(separatedBy: ",")
                if components.count >= 2 {
                    return "\(components[0]),\(components[1]),0"
                }
            }
        case (.vector3, .vector2):
            if let vec3 = value as? String {
                let components = vec3.components(separatedBy: ",")
                if components.count >= 2 {
                    return "\(components[0]),\(components[1])"
                }
            }
        case (.vector3, .vector4):
            if let vec3 = value as? String {
                return "\(vec3),0"
            }
        case (.vector4, .vector3):
            if let vec4 = value as? String {
                let components = vec4.components(separatedBy: ",")
                if components.count >= 3 {
                    return "\(components[0]),\(components[1]),\(components[2])"
                }
            }
        default:
            break
        }
        
        // String conversion
        if self == .string {
            return String(describing: value)
        }
        
        // For generic types, pass through
        if self.isGeneric {
            return value
        }
        
        return nil
    }
}

// MARK: - Connection Validation Manager
class ConnectionValidationManager: ObservableObject {
    static let shared = ConnectionValidationManager()
    
    @Published var validationErrors: [ConnectionValidationError] = []
    
    struct ConnectionValidationError {
        let fromNode: UUID
        let fromPort: Int
        let toNode: UUID
        let toPort: Int
        let message: String
        let severity: ValidationSeverity
    }
    
    enum ValidationSeverity {
        case error, warning, info
    }
    
    func validateConnection(from fromPort: NodePort, to toPort: NodePort) -> ConnectionValidationError? {
        // Check type compatibility
        if !fromPort.type.canConnect(to: toPort.type) {
            return ConnectionValidationError(
                fromNode: UUID(), // Will be filled by caller
                fromPort: 0,
                toNode: UUID(),
                toPort: 0,
                message: "Cannot connect \(fromPort.type.rawValue) to \(toPort.type.rawValue)",
                severity: .error
            )
        }
        
        // Check for potential data loss in conversions
        if fromPort.type == .float && toPort.type == .int {
            return ConnectionValidationError(
                fromNode: UUID(),
                fromPort: 0,
                toNode: UUID(),
                toPort: 0,
                message: "Converting float to int may lose precision",
                severity: .warning
            )
        }
        
        return nil
    }
    
    func suggestConversion(from fromType: PortType, to toType: PortType) -> NodeType? {
        if fromType.canConnect(to: toType) {
            return nil // No conversion needed
        }
        
        // Suggest appropriate conversion nodes
        switch (fromType, toType) {
        case (.int, .float), (.float, .int):
            return .math
        case (.string, .int), (.string, .float):
            return .math
        case (.vector2, .vector3), (.vector3, .vector2):
            return .math
        case (.vector3, .vector4), (.vector4, .vector3):
            return .math
        default:
            return nil
        }
    }
}

struct PortConnector: View {
    let type: PortType
    let isHovering: Bool
    @State private var showTooltip = false
    
    var body: some View {
        Group {
            switch type.shape {
            case .triangle:
                // Flow ports are triangular
                Image(systemName: "arrowtriangle.right.fill")
                    .font(.system(size: 10))
            case .dashedCircle:
                // Array ports have dashed outline
                Circle()
                    .frame(width: 10, height: 10)
                    .overlay(
                        Circle()
                            .stroke(type.color, style: StrokeStyle(lineWidth: 1, dash: [2, 2]))
                    )
            case .square:
                // Some types use square shape
                Rectangle()
                    .frame(width: 10, height: 10)
            case .diamond:
                // Diamond shape for special types
                Diamond()
                    .frame(width: 10, height: 10)
            case .circle:
                // Default circular ports
                Circle()
                    .frame(width: 10, height: 10)
            }
        }
        .foregroundColor(type.shape == .dashedCircle ? .clear : type.color)
        .scaleEffect(isHovering ? 1.3 : 1.0)
        .onHover { isHovering in
            showTooltip = isHovering
        }
        .popover(isPresented: $showTooltip, arrowEdge: .trailing) {
            PortTooltip(portType: type)
        }
    }
}

struct Diamond: Shape {
    func path(in rect: CGRect) -> Path {
        var path = Path()
        let width = rect.width
        let height = rect.height
        
        path.move(to: CGPoint(x: width/2, y: 0))
        path.addLine(to: CGPoint(x: width, y: height/2))
        path.addLine(to: CGPoint(x: width/2, y: height))
        path.addLine(to: CGPoint(x: 0, y: height/2))
        path.closeSubpath()
        
        return path
    }
}

struct PortTooltip: View {
    let portType: PortType
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(portType.rawValue.capitalized)
                .font(.caption)
                .fontWeight(.bold)
            
            Text(portType.description)
                .font(.caption2)
                .foregroundColor(.secondary)
            
            if portType.isNullable {
                Text("Nullable")
                    .font(.caption2)
                    .foregroundColor(.blue)
            }
            
            if portType.isGeneric {
                Text("Generic")
                    .font(.caption2)
                    .foregroundColor(.purple)
            }
        }
        .padding(8)
        .background(Color(.controlBackgroundColor))
        .cornerRadius(6)
        .frame(maxWidth: 200)
    }
}

extension PortType {
    var description: String {
        switch self {
        case .flow:
            return "Execution flow control"
        case .bool:
            return "Boolean true/false value"
        case .int:
            return "Integer number"
        case .float:
            return "Floating point number"
        case .vector2:
            return "2D vector (x, y)"
        case .vector3:
            return "3D vector (x, y, z)"
        case .vector4:
            return "4D vector (x, y, z, w)"
        case .quaternion:
            return "Rotation quaternion"
        case .string:
            return "Text string"
        case .object:
            return "Object reference"
        case .enumType:
            return "Enumeration value"
        case .structType:
            return "Structure value"
        case .classType:
            return "Class instance"
        case .asset:
            return "Asset reference"
        case .delegate:
            return "Delegate function"
        case .event:
            return "Event handler"
        case .array:
            return "Array collection"
        case .dictionary:
            return "Key-value dictionary"
        case .optional:
            return "Optional nullable value"
        case .generic:
            return "Generic type parameter"
        case .wildcard:
            return "Wildcard any type"
        case .any:
            return "Universal type"
        case .transform:
            return "Transform matrix"
        case .raycast:
            return "Raycast result"
        case .audioClip:
            return "Audio asset"
        case .animationClip:
            return "Animation asset"
        case .uiElement:
            return "UI element reference"
        case .fileInfo:
            return "File information"
        }
    }
}

// MARK: - Connection Path
struct ConnectionPath: View {
    let connection: GraphConnection
    let nodes: [GraphNode]
    let offset: CGPoint
    let zoom: CGFloat
    
    var body: some View {
        if let fromNode = nodes.first(where: { $0.id == connection.fromNode }),
           let toNode = nodes.first(where: { $0.id == connection.toNode }) {
            
            let fromPos = CGPoint(
                x: fromNode.position.x * zoom + offset.x + 150,
                y: fromNode.position.y * zoom + offset.y + 40 + CGFloat(connection.fromPort * 20)
            )
            let toPos = CGPoint(
                x: toNode.position.x * zoom + offset.x,
                y: toNode.position.y * zoom + offset.y + 40 + CGFloat(connection.toPort * 20)
            )
            
            Path { path in
                path.move(to: fromPos)
                let controlOffset = abs(toPos.x - fromPos.x) * 0.5
                path.addCurve(
                    to: toPos,
                    control1: CGPoint(x: fromPos.x + controlOffset, y: fromPos.y),
                    control2: CGPoint(x: toPos.x - controlOffset, y: toPos.y)
                )
            }
            .stroke(connectionColor, lineWidth: 2)
        }
    }
    
    var connectionColor: Color {
        if let fromNode = nodes.first(where: { $0.id == connection.fromNode }),
           connection.fromPort < fromNode.outputs.count {
            return fromNode.outputs[connection.fromPort].type.color
        }
        return .white
    }
}

// MARK: - Node Creation Menu
struct NodeCreationMenu: View {
    let position: CGPoint
    let onSelect: (NodeType) -> Void
    @State private var searchText = ""
    
    var filteredTypes: [NodeType] {
        if searchText.isEmpty {
            return NodeType.allCases
        }
        return NodeType.allCases.filter { $0.rawValue.contains(searchText.lowercased()) }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            TextField("Search nodes...", text: $searchText)
                .textFieldStyle(.plain)
                .padding(8)
                .background(Color.white.opacity(0.1))
            
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    ForEach(filteredTypes, id: \.self) { type in
                        Button(action: { onSelect(type) }) {
                            Text(type.rawValue.capitalized)
                                .padding(.horizontal, 8)
                                .padding(.vertical, 6)
                                .frame(maxWidth: .infinity, alignment: .leading)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
            .frame(maxHeight: 200)
        }
        .frame(width: 200)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(8)
        .shadow(radius: 8)
        .position(position)
    }
}

// MARK: - Debug Panels

struct DebugPanels: View {
    @ObservedObject var debugManager = DebugManager.shared
    @State private var selectedTab: DebugTab = .callStack
    
    enum DebugTab: String, CaseIterable {
        case callStack = "Call Stack"
        case variables = "Variables"
        case breakpoints = "Breakpoints"
        case performance = "Performance"
        case console = "Console"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tab selector
            HStack(spacing: 0) {
                ForEach(DebugTab.allCases, id: \.self) { tab in
                    Button(action: { selectedTab = tab }) {
                        Text(tab.rawValue)
                            .font(DesignSystem.Typography.small)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 8)
                            .background(selectedTab == tab ? Color.blue : Color.clear)
                            .foregroundColor(selectedTab == tab ? .white : .gray)
                    }
                    .buttonStyle(.plain)
                }
            }
            .background(Color(red: 0.1, green: 0.1, blue: 0.12))
            
            Divider()
            
            // Tab content
            ScrollView {
                VStack(alignment: .leading, spacing: 8) {
                    switch selectedTab {
                    case .callStack:
                        CallStackView()
                    case .variables:
                        VariablesView()
                    case .breakpoints:
                        BreakpointsView()
                    case .performance:
                        PerformanceView()
                    case .console:
                        ConsoleView()
                    }
                }
                .padding(8)
            }
        }
    }
}

// MARK: - Call Stack View

struct CallStackView: View {
    @ObservedObject var debugManager = DebugManager.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Call Stack")
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(.white)
            
            if debugManager.callStack.isEmpty {
                Text("No active call stack")
                    .foregroundColor(.gray)
                    .font(DesignSystem.Typography.small)
            } else {
                ForEach(debugManager.callStack.reversed()) { frame in
                    HStack {
                        Image(systemName: "arrow.right.circle.fill")
                            .foregroundColor(.blue)
                            .font(.system(size: 10))
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text(frame.nodeName)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(.white)
                            
                            Text(frame.functionName)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(.gray)
                        }
                        
                        Spacer()
                        
                        Text("Line \(frame.lineNumber)")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.gray)
                    }
                    .padding(.vertical, 4)
                }
            }
        }
    }
}

// MARK: - Variables View

struct VariablesView: View {
    @ObservedObject var debugManager = DebugManager.shared
    @State private var showLocalVariables = true
    @State private var expressionInput = ""
    @State private var evaluationResult: DebugVariable?
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            // Variable type selector
            HStack {
                Button(action: { showLocalVariables = true }) {
                    Text("Local")
                        .foregroundColor(showLocalVariables ? .white : .gray)
                }
                .buttonStyle(.plain)
                
                Button(action: { showLocalVariables = false }) {
                    Text("Instance")
                        .foregroundColor(!showLocalVariables ? .white : .gray)
                }
                .buttonStyle(.plain)
                
                Spacer()
            }
            
            // Variables list
            if showLocalVariables {
                VariablesSection(title: "Local Variables", variables: debugManager.localVariables)
            } else {
                VariablesSection(title: "Instance Variables", variables: debugManager.instanceVariables)
            }
            
            // Expression evaluation
            Divider()
            
            VStack(alignment: .leading, spacing: 8) {
                Text("Expression Evaluation")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                
                HStack {
                    TextField("Enter expression...", text: $expressionInput)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .background(Color.white.opacity(0.1))
                    
                    Button("Evaluate") {
                        evaluationResult = debugManager.evaluateExpression(expressionInput)
                    }
                    .buttonStyle(.borderedProminent)
                    .controlSize(.small)
                }
                
                if let result = evaluationResult {
                    HStack {
                        Text("\(result.name):")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(.white)
                        
                        Text(result.displayValue)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(.green)
                        
                        Text("(\(result.type))")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.gray)
                    }
                    .padding(.vertical, 4)
                }
            }
        }
    }
}

struct VariablesSection: View {
    let title: String
    let variables: [String: DebugVariable]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(title)
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(.white)
            
            if variables.isEmpty {
                Text("No \(title.lowercased())")
                    .foregroundColor(.gray)
                    .font(DesignSystem.Typography.small)
            } else {
                ForEach(Array(variables.values), id: \.id) { variable in
                    HStack {
                        Text(variable.name)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(.white)
                        
                        Spacer()
                        
                        Text(variable.displayValue)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(.green)
                        
                        Text("(\(variable.type))")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.gray)
                    }
                    .padding(.vertical, 2)
                }
            }
        }
    }
}

// MARK: - Breakpoints View

struct BreakpointsView: View {
    @ObservedObject var debugManager = DebugManager.shared
    @ObservedObject var manager = NodeGraphManager.shared
    @State private var selectedBreakpoint: UUID?
    @State private var showConditionalDialog = false
    @State private var showHitCountDialog = false
    @State private var conditionalInput = ""
    @State private var hitCountInput = ""
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Breakpoints")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                
                Spacer()
                
                Button("Clear All") {
                    debugManager.breakpoints.removeAll()
                    debugManager.conditionalBreakpoints.removeAll()
                    debugManager.hitCountBreakpoints.removeAll()
                }
                .buttonStyle(.bordered)
                .controlSize(.small)
            }
            
            if debugManager.breakpoints.isEmpty {
                Text("No breakpoints set")
                    .foregroundColor(.gray)
                    .font(DesignSystem.Typography.small)
            } else {
                ForEach(debugManager.breakpoints, id: \.self) { breakpointId in
                    BreakpointRow(
                        breakpointId: breakpointId,
                        nodes: manager.nodes,
                        conditionalBreakpoint: debugManager.conditionalBreakpoints[breakpointId],
                        hitCountBreakpoint: debugManager.hitCountBreakpoints[breakpointId],
                        logPoint: debugManager.logPoints[breakpointId],
                        onRemove: {
                            debugManager.toggleBreakpoint(on: breakpointId)
                        }
                    )
                }
            }
            
            Divider()
            
            // Add breakpoint options
            VStack(alignment: .leading, spacing: 8) {
                Text("Add Special Breakpoint")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                
                if let selectedNodeId = manager.selectedNodes.first {
                    Button("Conditional Breakpoint") {
                        showConditionalDialog = true
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                    
                    Button("Hit Count Breakpoint") {
                        showHitCountDialog = true
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                    
                    Button("Log Point") {
                        debugManager.addLogPoint(on: selectedNodeId, message: "Node executed")
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                } else {
                    Text("Select a node to add special breakpoints")
                        .foregroundColor(.gray)
                        .font(DesignSystem.Typography.small)
                }
            }
        }
        .sheet(isPresented: $showConditionalDialog) {
            ConditionalBreakpointDialog(
                nodeId: manager.selectedNodes.first ?? UUID(),
                condition: $conditionalInput,
                onConfirm: { condition in
                    debugManager.setConditionalBreakpoint(on: manager.selectedNodes.first ?? UUID(), condition: condition)
                    showConditionalDialog = false
                }
            )
        }
        .sheet(isPresented: $showHitCountDialog) {
            HitCountBreakpointDialog(
                nodeId: manager.selectedNodes.first ?? UUID(),
                hitCount: $hitCountInput,
                onConfirm: { count in
                    if let count = Int(count) {
                        debugManager.setHitCountBreakpoint(on: manager.selectedNodes.first ?? UUID(), targetCount: count)
                    }
                    showHitCountDialog = false
                }
            )
        }
    }
}

struct BreakpointRow: View {
    let breakpointId: UUID
    let nodes: [GraphNode]
    let conditionalBreakpoint: String?
    let hitCountBreakpoint: (current: Int, target: Int)?
    let logPoint: String?
    let onRemove: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                if let node = nodes.first(where: { $0.id == breakpointId }) {
                    Text(node.title)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.white)
                } else {
                    Text("Unknown Node")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.gray)
                }
                
                Spacer()
                
                Button(action: onRemove) {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundColor(.red)
                }
                .buttonStyle(.plain)
            }
            
            if let condition = conditionalBreakpoint {
                HStack {
                    Image(systemName: "questionmark.circle.fill")
                        .foregroundColor(.orange)
                        .font(.system(size: 10))
                    
                    Text("Condition: \(condition)")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.orange)
                }
            }
            
            if let hitCount = hitCountBreakpoint {
                HStack {
                    Image(systemName: "number.circle.fill")
                        .foregroundColor(.blue)
                        .font(.system(size: 10))
                    
                    Text("Hit count: \(hitCount.current)/\(hitCount.target)")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.blue)
                }
            }
            
            if let logPoint = logPoint {
                HStack {
                    Image(systemName: "note.text")
                        .foregroundColor(.green)
                        .font(.system(size: 10))
                    
                    Text("Log: \(logPoint)")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.green)
                }
            }
        }
        .padding(.vertical, 4)
        .padding(.horizontal, 8)
        .background(Color.white.opacity(0.05))
        .cornerRadius(4)
    }
}

// MARK: - Performance View

struct PerformanceView: View {
    @ObservedObject var debugManager = DebugManager.shared
    @ObservedObject var manager = NodeGraphManager.shared
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Performance Profiling")
                .font(DesignSystem.Typography.bodyBold)
                .foregroundColor(.white)
            
            if debugManager.nodePerformance.isEmpty {
                Text("No performance data available")
                    .foregroundColor(.gray)
                    .font(DesignSystem.Typography.small)
            } else {
                ForEach(Array(debugManager.nodePerformance.values), id: \.nodeId) { perfData in
                    if let node = manager.nodes.first(where: { $0.id == perfData.nodeId }) {
                        VStack(alignment: .leading, spacing: 4) {
                            HStack {
                                Text(node.title)
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(.white)
                                
                                Spacer()
                                
                                Text("\(String(format: "%.2f", perfData.executionTime * 1000))ms")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(.green)
                            }
                            
                            HStack {
                                Text("Calls: \(perfData.callCount)")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(.gray)
                                
                                Spacer()
                                
                                Text("Memory: \(perfData.memoryUsage) bytes")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(.gray)
                            }
                        }
                        .padding(.vertical, 4)
                        .padding(.horizontal, 8)
                        .background(Color.white.opacity(0.05))
                        .cornerRadius(4)
                    }
                }
            }
        }
    }
}

// MARK: - Console View

struct ConsoleView: View {
    @ObservedObject var debugManager = DebugManager.shared
    @State private var consoleMessages: [ConsoleMessage] = []
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("Console Output")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                
                Spacer()
                
                Button("Clear") {
                    consoleMessages.removeAll()
                }
                .buttonStyle(.bordered)
                .controlSize(.small)
            }
            
            if consoleMessages.isEmpty {
                Text("No console output")
                    .foregroundColor(.gray)
                    .font(DesignSystem.Typography.small)
            } else {
                ForEach(consoleMessages, id: \.id) { message in
                    HStack {
                        Image(systemName: message.type.iconName)
                            .foregroundColor(message.type.color)
                            .font(.system(size: 10))
                        
                        Text(message.text)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(message.type.color)
                        
                        Spacer()
                        
                        Text(message.timestamp, style: .time)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.gray)
                    }
                }
            }
        }
        .onReceive(NotificationCenter.default.publisher(for: .debugConsoleMessage)) { notification in
            if let message = notification.object as? ConsoleMessage {
                consoleMessages.append(message)
            }
        }
    }
}

struct ConsoleMessage: Identifiable {
    let id = UUID()
    let text: String
    let type: ConsoleMessageType
    let timestamp = Date()
}

enum ConsoleMessageType {
    case log, warning, error, info
    
    var color: Color {
        switch self {
        case .log: return .white
        case .warning: return .orange
        case .error: return .red
        case .info: return .blue
        }
    }
    
    var iconName: String {
        switch self {
        case .log: return "text.bubble"
        case .warning: return "exclamationmark.triangle"
        case .error: return "xmark.circle"
        case .info: return "info.circle"
        }
    }
}

// MARK: - Breakpoint Dialogs

struct ConditionalBreakpointDialog: View {
    let nodeId: UUID
    @Binding var condition: String
    let onConfirm: (String) -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Conditional Breakpoint")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            VStack(alignment: .leading, spacing: 8) {
                Text("Condition:")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(.white)
                
                TextField("e.g., x > 10", text: $condition)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
            }
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Button("Set") {
                    onConfirm(condition)
                }
                .buttonStyle(.borderedProminent)
                .disabled(condition.isEmpty)
            }
        }
        .padding(20)
        .frame(width: 300)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
    }
}

struct HitCountBreakpointDialog: View {
    let nodeId: UUID
    @Binding var hitCount: String
    let onConfirm: (String) -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Hit Count Breakpoint")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            VStack(alignment: .leading, spacing: 8) {
                Text("Break after:")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(.white)
                
                HStack {
                    TextField("0", text: $hitCount)
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .keyboardType(.numberPad)
                    
                    Text("hits")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.white)
                }
            }
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Button("Set") {
                    onConfirm(hitCount)
                }
                .buttonStyle(.borderedProminent)
                .disabled(hitCount.isEmpty || Int(hitCount) == nil)
            }
        }
        .padding(20)
        .frame(width: 300)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
    }
}

// MARK: - Debug Notification Extension

extension Notification.Name {
    static let debugConsoleMessage = Notification.Name("debugConsoleMessage")
}

// MARK: - Function Parameter Editor
struct FunctionParameterEditor: View {
    @ObservedObject var manager: NodeGraphManager
    let function: GraphFunction
    @Environment(\.dismiss) private var dismiss
    
    @State private var parameterName = ""
    @State private var selectedType: PortType = .any
    @State private var defaultValue = ""
    @State private var isOptional = false
    @State private var description = ""
    @State private var selectedParameter: FunctionParameter?
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Function Parameters")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            // Function info
            VStack(alignment: .leading, spacing: 4) {
                Text("Function: \(function.name)")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                Text(function.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white.opacity(0.6))
            }
            
            Divider()
            
            // Add new parameter
            VStack(alignment: .leading, spacing: 12) {
                Text("Add Parameter")
                    .font(DesignSystem.Typography.subheadline)
                    .foregroundColor(.white)
                
                VStack(alignment: .leading, spacing: 8) {
                    // Name
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Parameter Name")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter parameter name...", text: $parameterName)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    // Type
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Type")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        Picker("Type", selection: $selectedType) {
                            ForEach(PortType.allCases, id: \.self) { type in
                                Text(type.rawValue.capitalized).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    // Default Value
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Default Value (Optional)")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter default value...", text: $defaultValue)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    // Optional toggle
                    Toggle("Optional Parameter", isOn: $isOptional)
                        .foregroundColor(.white)
                    
                    // Description
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Description")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter description...", text: $description)
                            .textFieldStyle(.roundedBorder)
                    }
                }
                
                Button("Add Parameter") {
                    manager.addParameter(
                        to: function,
                        name: parameterName,
                        type: selectedType,
                        defaultValue: defaultValue.isEmpty ? nil : defaultValue,
                        isOptional: isOptional
                    )
                    
                    // Clear form
                    parameterName = ""
                    selectedType = .any
                    defaultValue = ""
                    isOptional = false
                    description = ""
                }
                .buttonStyle(.borderedProminent)
                .disabled(parameterName.isEmpty)
            }
            
            Divider()
            
            // Existing parameters
            VStack(alignment: .leading, spacing: 8) {
                Text("Current Parameters")
                    .font(DesignSystem.Typography.subheadline)
                    .foregroundColor(.white)
                
                if function.parameters.isEmpty {
                    Text("No parameters defined")
                        .foregroundColor(.gray)
                        .font(DesignSystem.Typography.small)
                } else {
                    ForEach(function.parameters) { parameter in
                        HStack {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(parameter.name)
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(.white)
                                
                                HStack {
                                    Text(parameter.type.rawValue.capitalized)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(parameter.type.color)
                                    
                                    if parameter.isOptional {
                                        Text("Optional")
                                            .font(DesignSystem.Typography.small)
                                            .padding(.horizontal, 4)
                                            .padding(.vertical, 1)
                                            .background(Color.blue.opacity(0.3))
                                            .cornerRadius(3)
                                            .foregroundColor(.blue)
                                    }
                                }
                                
                                if !parameter.description.isEmpty {
                                    Text(parameter.description)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(.white.opacity(0.6))
                                }
                                
                                if let defaultValue = parameter.defaultValue {
                                    Text("Default: \(defaultValue)")
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(.white.opacity(0.6))
                                }
                            }
                            
                            Spacer()
                            
                            Button(action: {
                                manager.removeParameter(from: function, parameter: parameter)
                            }) {
                                Image(systemName: "trash")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                        }
                        .padding(.vertical, 4)
                        .padding(.horizontal, 8)
                        .background(Color.white.opacity(0.05))
                        .cornerRadius(6)
                    }
                }
            }
            
            Divider()
            
            HStack {
                Button("Close") {
                    dismiss()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(20)
        .frame(width: 500, height: 600)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
    }
}

// MARK: - Function Return Value Editor
struct FunctionReturnValueEditor: View {
    @ObservedObject var manager: NodeGraphManager
    let function: GraphFunction
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedType: PortType = .any
    @State private var description = ""
    @State private var hasReturnValue = false
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Function Return Value")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            // Function info
            VStack(alignment: .leading, spacing: 4) {
                Text("Function: \(function.name)")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                Text(function.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white.opacity(0.6))
            }
            
            Divider()
            
            // Return value configuration
            VStack(alignment: .leading, spacing: 12) {
                Toggle("Function has return value", isOn: $hasReturnValue)
                    .foregroundColor(.white)
                
                if hasReturnValue {
                    VStack(alignment: .leading, spacing: 8) {
                        // Type
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Return Type")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(.white.opacity(0.8))
                            Picker("Type", selection: $selectedType) {
                                ForEach(PortType.allCases, id: \.self) { type in
                                    Text(type.rawValue.capitalized).tag(type)
                                }
                            }
                            .pickerStyle(.menu)
                        }
                        
                        // Description
                        VStack(alignment: .leading, spacing: 4) {
                            Text("Description")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(.white.opacity(0.8))
                            TextField("Enter return value description...", text: $description)
                                .textFieldStyle(.roundedBorder)
                        }
                    }
                    
                    Button("Set Return Value") {
                        manager.setReturnValue(for: function, type: selectedType, description: description)
                        dismiss()
                    }
                    .buttonStyle(.borderedProminent)
                } else {
                    Button("Remove Return Value") {
                        // Remove return value from function
                        if let index = manager.functions.firstIndex(where: { $0.id == function.id }) {
                            manager.functions[index].returnValue = nil
                        }
                        dismiss()
                    }
                    .buttonStyle(.bordered)
                }
            }
            
            Spacer()
            
            HStack {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(20)
        .frame(width: 400, height: 300)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
    }
}

// MARK: - Function Local Variables Editor
struct FunctionLocalVariablesEditor: View {
    @ObservedObject var manager: NodeGraphManager
    let function: GraphFunction
    @Environment(\.dismiss) private var dismiss
    
    @State private var variableName = ""
    @State private var selectedType: PortType = .any
    @State private var defaultValue = ""
    @State private var selectedScope: VariableScope = .local
    @State private var description = ""
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Function Local Variables")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            // Function info
            VStack(alignment: .leading, spacing: 4) {
                Text("Function: \(function.name)")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(.white)
                Text(function.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(.white.opacity(0.6))
            }
            
            Divider()
            
            // Add new variable
            VStack(alignment: .leading, spacing: 12) {
                Text("Add Variable")
                    .font(DesignSystem.Typography.subheadline)
                    .foregroundColor(.white)
                
                VStack(alignment: .leading, spacing: 8) {
                    // Name
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Variable Name")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter variable name...", text: $variableName)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    // Type
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Type")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        Picker("Type", selection: $selectedType) {
                            ForEach(PortType.allCases, id: \.self) { type in
                                Text(type.rawValue.capitalized).tag(type)
                            }
                        }
                        .pickerStyle(.menu)
                    }
                    
                    // Scope
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Scope")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        Picker("Scope", selection: $selectedScope) {
                            ForEach(VariableScope.allCases, id: \.self) { scope in
                                Text(scope.rawValue).tag(scope)
                            }
                        }
                        .pickerStyle(.segmented)
                    }
                    
                    // Default Value
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Default Value (Optional)")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter default value...", text: $defaultValue)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    // Description
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Description")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter description...", text: $description)
                            .textFieldStyle(.roundedBorder)
                    }
                }
                
                Button("Add Variable") {
                    manager.addLocalVariable(
                        to: function,
                        name: variableName,
                        type: selectedType,
                        defaultValue: defaultValue.isEmpty ? nil : defaultValue,
                        scope: selectedScope
                    )
                    
                    // Clear form
                    variableName = ""
                    selectedType = .any
                    defaultValue = ""
                    selectedScope = .local
                    description = ""
                }
                .buttonStyle(.borderedProminent)
                .disabled(variableName.isEmpty)
            }
            
            Divider()
            
            // Existing variables
            VStack(alignment: .leading, spacing: 8) {
                Text("Current Variables")
                    .font(DesignSystem.Typography.subheadline)
                    .foregroundColor(.white)
                
                if function.localVariables.isEmpty {
                    Text("No variables defined")
                        .foregroundColor(.gray)
                        .font(DesignSystem.Typography.small)
                } else {
                    ForEach(function.localVariables) { variable in
                        HStack {
                            VStack(alignment: .leading, spacing: 2) {
                                Text(variable.name)
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(.white)
                                
                                HStack {
                                    Text(variable.type.rawValue.capitalized)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(variable.type.color)
                                    
                                    Text(variable.scope.rawValue)
                                        .font(DesignSystem.Typography.small)
                                        .padding(.horizontal, 4)
                                        .padding(.vertical, 1)
                                        .background(Color.gray.opacity(0.3))
                                        .cornerRadius(3)
                                        .foregroundColor(.gray)
                                }
                                
                                if !variable.description.isEmpty {
                                    Text(variable.description)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(.white.opacity(0.6))
                                }
                                
                                if let defaultValue = variable.defaultValue {
                                    Text("Default: \(defaultValue)")
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(.white.opacity(0.6))
                                }
                            }
                            
                            Spacer()
                            
                            Button(action: {
                                manager.removeLocalVariable(from: function, variable: variable)
                            }) {
                                Image(systemName: "trash")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                        }
                        .padding(.vertical, 4)
                        .padding(.horizontal, 8)
                        .background(Color.white.opacity(0.05))
                        .cornerRadius(6)
                    }
                }
            }
            
            Divider()
            
            HStack {
                Button("Close") {
                    dismiss()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(20)
        .frame(width: 500, height: 600)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
    }
}

// MARK: - Function Browser
struct FunctionBrowser: View {
    @ObservedObject var manager: NodeGraphManager
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedCategory = "All"
    @State private var searchText = ""
    @State private var selectedFunction: GraphFunction?
    
    var categories: [String] {
        let cats = Set(manager.functions.map { $0.category })
        return ["All"] + Array(cats).sorted()
    }
    
    var filteredFunctions: [GraphFunction] {
        var filtered = manager.functions
        
        // Filter by category
        if selectedCategory != "All" {
            filtered = filtered.filter { $0.category == selectedCategory }
        }
        
        // Filter by search text
        if !searchText.isEmpty {
            filtered = filtered.filter { 
                $0.name.localizedCaseInsensitiveContains(searchText) ||
                $0.description.localizedCaseInsensitiveContains(searchText)
            }
        }
        
        return filtered.sorted { $0.name < $1.name }
    }
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Function Browser")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            // Search and filter
            VStack(spacing: 8) {
                TextField("Search functions...", text: $searchText)
                    .textFieldStyle(.roundedBorder)
                
                Picker("Category", selection: $selectedCategory) {
                    ForEach(categories, id: \.self) { category in
                        Text(category).tag(category)
                    }
                }
                .pickerStyle(.segmented)
            }
            
            // Function list
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 8) {
                    ForEach(filteredFunctions) { function in
                        FunctionCard(
                            function: function,
                            isSelected: selectedFunction?.id == function.id,
                            onSelect: { selectedFunction = function },
                            onAddToGraph: {
                                let position = CGPoint(
                                    x: -manager.panOffset.x / manager.zoom + 200,
                                    y: -manager.panOffset.y / manager.zoom + 200
                                )
                                manager.generateFunctionCallNode(for: function, at: position)
                            }
                        )
                    }
                }
            }
            .frame(maxHeight: 400)
            
            // Selected function details
            if let function = selectedFunction {
                Divider()
                
                VStack(alignment: .leading, spacing: 12) {
                    Text("Function Details")
                        .font(DesignSystem.Typography.subheadline)
                        .foregroundColor(.white)
                    
                    VStack(alignment: .leading, spacing: 8) {
                        Text(function.name)
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(.white)
                        
                        Text(function.description)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.8))
                        
                        // Parameters
                        if !function.parameters.isEmpty {
                            Text("Parameters:")
                                .font(DesignSystem.Typography.smallBold)
                                .foregroundColor(.white.opacity(0.8))
                            
                            ForEach(function.parameters) { parameter in
                                HStack {
                                    Text(parameter.name)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(.white)
                                    
                                    Text(parameter.type.rawValue.capitalized)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(parameter.type.color)
                                    
                                    if parameter.isOptional {
                                        Text("(optional)")
                                            .font(DesignSystem.Typography.small)
                                            .foregroundColor(.gray)
                                    }
                                    
                                    Spacer()
                                }
                            }
                        }
                        
                        // Return value
                        if let returnValue = function.returnValue {
                            Text("Returns:")
                                .font(DesignSystem.Typography.smallBold)
                                .foregroundColor(.white.opacity(0.8))
                            
                            HStack {
                                Text(returnValue.type.rawValue.capitalized)
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(returnValue.type.color)
                                
                                if !returnValue.description.isEmpty {
                                    Text("- \(returnValue.description)")
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(.white.opacity(0.8))
                                }
                                
                                Spacer()
                            }
                        }
                        
                        // Metadata
                        HStack {
                            Text("Category: \(function.category)")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(.white.opacity(0.6))
                            
                            Spacer()
                            
                            Text("Modified: \(function.modified, style: .relative)")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(.white.opacity(0.6))
                        }
                    }
                    
                    HStack {
                        Button("Add to Graph") {
                            let position = CGPoint(
                                x: -manager.panOffset.x / manager.zoom + 200,
                                y: -manager.panOffset.y / manager.zoom + 200
                            )
                            manager.generateFunctionCallNode(for: function, at: position)
                        }
                        .buttonStyle(.borderedProminent)
                        
                        Button("Edit Function") {
                            manager.editFunction(function)
                            dismiss()
                        }
                        .buttonStyle(.bordered)
                    }
                }
            }
            
            Divider()
            
            HStack {
                Button("Close") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Create New Function") {
                    manager.createFunction(name: "New Function")
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(20)
        .frame(width: 600, height: 700)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
    }
}

struct FunctionCard: View {
    let function: GraphFunction
    let isSelected: Bool
    let onSelect: () -> Void
    let onAddToGraph: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text(function.name)
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(.white)
                    
                    Text(function.description)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.white.opacity(0.8))
                        .lineLimit(2)
                }
                
                Spacer()
                
                VStack(spacing: 4) {
                    Button(action: onAddToGraph) {
                        Image(systemName: "plus.circle.fill")
                            .foregroundColor(.green)
                    }
                    .buttonStyle(.plain)
                    .help("Add to Graph")
                    
                    Button(action: onSelect) {
                        Image(systemName: "info.circle")
                            .foregroundColor(isSelected ? .blue : .gray)
                    }
                    .buttonStyle(.plain)
                    .help("View Details")
                }
            }
            
            HStack {
                // Parameter count
                if !function.parameters.isEmpty {
                    HStack(spacing: 4) {
                        Image(systemName: "arrow.right.circle")
                            .font(.system(size: 10))
                            .foregroundColor(.white.opacity(0.6))
                        Text("\(function.parameters.count) params")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(.white.opacity(0.6))
                    }
                }
                
                // Return value indicator
                if function.returnValue != nil {
                    HStack(spacing: 4) {
                        Image(systemName: "arrow.left.circle")
                            .font(.system(size: 10))
                            .foregroundColor(.white.opacity(0.6))
                        Text("Returns \(function.returnValue!.type.rawValue)")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(function.returnValue!.type.color)
                    }
                }
                
                Spacer()
                
                // Category tag
                Text(function.category)
                    .font(DesignSystem.Typography.small)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(Color.blue.opacity(0.3))
                    .cornerRadius(4)
                    .foregroundColor(.blue)
            }
        }
        .padding(12)
        .background(isSelected ? Color.blue.opacity(0.2) : Color.white.opacity(0.05))
        .cornerRadius(8)
        .onTapGesture {
            onSelect()
        }
    }
}

// MARK: - Function Management Panel
struct FunctionManagementPanel: View {
    @ObservedObject var manager: NodeGraphManager
    @Environment(\.dismiss) private var dismiss
    
    @State private var showParameterEditor = false
    @State private var showReturnValueEditor = false
    @State private var showVariableEditor = false
    @State private var showFunctionBrowser = false
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Function Management")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            if let currentFunction = manager.currentFunction {
                // Current function info
                VStack(alignment: .leading, spacing: 8) {
                    Text("Editing: \(currentFunction.name)")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(.white)
                    
                    Text(currentFunction.description)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.white.opacity(0.8))
                }
                
                Divider()
                
                // Function editing options
                VStack(spacing: 8) {
                    Button("Edit Parameters") {
                        showParameterEditor = true
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Edit Return Value") {
                        showReturnValueEditor = true
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Edit Local Variables") {
                        showVariableEditor = true
                    }
                    .buttonStyle(.bordered)
                }
                
                Divider()
                
                // Save/Cancel
                HStack {
                    Button("Cancel") {
                        manager.cancelFunctionEdit()
                        dismiss()
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Save Function") {
                        manager.saveCurrentFunction()
                        dismiss()
                    }
                    .buttonStyle(.borderedProminent)
                }
            } else {
                // No function selected
                VStack(spacing: 16) {
                    Text("Select a function to edit or create a new one")
                        .foregroundColor(.white.opacity(0.6))
                    
                    Button("Browse Functions") {
                        showFunctionBrowser = true
                    }
                    .buttonStyle(.borderedProminent)
                    
                    Button("Create New Function") {
                        manager.createFunction(name: "New Function")
                    }
                    .buttonStyle(.bordered)
                }
            }
            
            Spacer()
        }
        .padding(20)
        .frame(width: 400, height: 300)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
        .sheet(isPresented: $showParameterEditor) {
            if let function = manager.currentFunction {
                FunctionParameterEditor(manager: manager, function: function)
            }
        }
        .sheet(isPresented: $showReturnValueEditor) {
            if let function = manager.currentFunction {
                FunctionReturnValueEditor(manager: manager, function: function)
            }
        }
        .sheet(isPresented: $showVariableEditor) {
            if let function = manager.currentFunction {
                FunctionLocalVariablesEditor(manager: manager, function: function)
            }
        }
        .sheet(isPresented: $showFunctionBrowser) {
            FunctionBrowser(manager: manager)
        }
    }
}

// MARK: - Variable Creation Panel
struct VariableCreationPanel: View {
    @ObservedObject var manager: NodeGraphManager
    @Environment(\.dismiss) private var dismiss
    
    @State private var variableName = ""
    @State private var selectedType: PortType = .any
    @State private var selectedScope: GraphVariable.VariableScope = .local
    @State private var defaultValue = ""
    @State private var description = ""
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Create Variable")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            VStack(alignment: .leading, spacing: 12) {
                // Name
                VStack(alignment: .leading, spacing: 4) {
                    Text("Variable Name")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.white.opacity(0.8))
                    TextField("Enter variable name...", text: $variableName)
                        .textFieldStyle(.roundedBorder)
                }
                
                // Type
                VStack(alignment: .leading, spacing: 4) {
                    Text("Type")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.white.opacity(0.8))
                    Picker("Type", selection: $selectedType) {
                        ForEach(PortType.allCases, id: \.self) { type in
                            Text(type.rawValue.capitalized).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                // Scope
                VStack(alignment: .leading, spacing: 4) {
                    Text("Scope")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.white.opacity(0.8))
                    Picker("Scope", selection: $selectedScope) {
                        ForEach(GraphVariable.VariableScope.allCases, id: \.self) { scope in
                            Text(scope.rawValue).tag(scope)
                        }
                    }
                    .pickerStyle(.segmented)
                }
                
                // Default Value
                VStack(alignment: .leading, spacing: 4) {
                    Text("Default Value (Optional)")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.white.opacity(0.8))
                    TextField("Enter default value...", text: $defaultValue)
                        .textFieldStyle(.roundedBorder)
                }
                
                // Description
                VStack(alignment: .leading, spacing: 4) {
                    Text("Description")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.white.opacity(0.8))
                    TextField("Enter description...", text: $description)
                        .textFieldStyle(.roundedBorder)
                }
            }
            
            HStack(spacing: 12) {
                Button("Cancel") {
                    dismiss()
                }
                .buttonStyle(.bordered)
                
                Button("Create Variable") {
                    manager.createVariable(
                        name: variableName,
                        type: selectedType,
                        scope: selectedScope,
                        defaultValue: defaultValue.isEmpty ? nil : defaultValue
                    )
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
                .disabled(variableName.isEmpty)
            }
        }
        .padding(20)
        .frame(width: 400)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
    }
}

// MARK: - Macro Management Panel
struct MacroManagementPanel: View {
    @ObservedObject var manager: NodeGraphManager
    @Environment(\.dismiss) private var dismiss
    
    @State private var macroName = ""
    @State private var macroDescription = ""
    @State private var selectedMacro: GraphMacro?
    @State private var showParameterEditor = false
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Macro Management")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            if manager.selectedNodes.isEmpty {
                Text("Select nodes to create a macro")
                    .foregroundColor(.white.opacity(0.6))
            } else {
                VStack(alignment: .leading, spacing: 12) {
                    Text("Selected \\(manager.selectedNodes.count) nodes")
                        .foregroundColor(.white.opacity(0.8))
                    
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Macro Name")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter macro name...", text: $macroName)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Description")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(.white.opacity(0.8))
                        TextField("Enter macro description...", text: $macroDescription)
                            .textFieldStyle(.roundedBorder)
                    }
                    
                    Button("Create Macro") {
                        manager.createMacro(
                            name: macroName,
                            description: macroDescription,
                            selectedNodeIds: manager.selectedNodes
                        )
                        dismiss()
                    }
                    .buttonStyle(.borderedProminent)
                    .disabled(macroName.isEmpty)
                }
            }
            
            Divider()
            
            // Existing Macros
            VStack(alignment: .leading, spacing: 8) {
                Text("Existing Macros")
                    .font(DesignSystem.Typography.subheadline)
                    .foregroundColor(.white.opacity(0.8))
                
                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 4) {
                        ForEach(manager.macros) { macro in
                            HStack {
                                VStack(alignment: .leading) {
                                    Text(macro.name)
                                        .foregroundColor(.white)
                                    Text(macro.description)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(.white.opacity(0.6))
                                }
                                
                                Spacer()
                                
                                Button("Expand") {
                                    // Expand macro at center of view
                                    let centerPosition = CGPoint(
                                        x: -manager.panOffset.x / manager.zoom + 200,
                                        y: -manager.panOffset.y / manager.zoom + 200
                                    )
                                    manager.expandMacro(macro, at: centerPosition)
                                }
                                .buttonStyle(.bordered)
                            }
                            .padding(8)
                            .background(Color.white.opacity(0.05))
                            .cornerRadius(6)
                        }
                    }
                }
                .frame(maxHeight: 200)
            }
            
            HStack {
                Button("Close") {
                    dismiss()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(20)
        .frame(width: 500, height: 600)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
    }
}

// MARK: - Template Browser
struct TemplateBrowser: View {
    @ObservedObject var manager: NodeGraphManager
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedCategory = "All"
    @State private var searchText = ""
    
    var categories: [String] {
        let cats = Set(manager.templates.map { $0.category })
        return ["All"] + Array(cats).sorted()
    }
    
    var filteredTemplates: [GraphTemplate] {
        var filtered = manager.templates
        
        if selectedCategory != "All" {
            filtered = filtered.filter { $0.category == selectedCategory }
        }
        
        if !searchText.isEmpty {
            filtered = filtered.filter { 
                $0.name.localizedCaseInsensitiveContains(searchText) ||
                $0.description.localizedCaseInsensitiveContains(searchText)
            }
        }
        
        return filtered
    }
    
    var body: some View {
        VStack(spacing: 16) {
            Text("Graph Templates")
                .font(DesignSystem.Typography.headline)
                .foregroundColor(.white)
            
            // Search and Filter
            VStack(spacing: 8) {
                TextField("Search templates...", text: $searchText)
                    .textFieldStyle(.roundedBorder)
                
                Picker("Category", selection: $selectedCategory) {
                    ForEach(categories, id: \.self) { category in
                        Text(category).tag(category)
                    }
                }
                .pickerStyle(.segmented)
            }
            
            // Template List
            ScrollView {
                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 2), spacing: 12) {
                    ForEach(filteredTemplates) { template in
                        VStack(alignment: .leading, spacing: 8) {
                            Text(template.name)
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(.white)
                            
                            Text(template.description)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(.white.opacity(0.8))
                                .lineLimit(2)
                            
                            Text(template.category)
                                .font(DesignSystem.Typography.small)
                                .padding(.horizontal, 8)
                                .padding(.vertical, 2)
                                .background(Color.blue.opacity(0.3))
                                .cornerRadius(4)
                                .foregroundColor(.white)
                            
                            Spacer()
                            
                            Button("Add to Graph") {
                                let position = CGPoint(
                                    x: -manager.panOffset.x / manager.zoom + 200,
                                    y: -manager.panOffset.y / manager.zoom + 200
                                )
                                manager.createGraphInstance(from: template, at: position)
                            }
                            .buttonStyle(.borderedProminent)
                        }
                        .padding(12)
                        .frame(height: 150)
                        .background(Color.white.opacity(0.05))
                        .cornerRadius(8)
                    }
                }
            }
            
            HStack {
                Button("Close") {
                    dismiss()
                }
                .buttonStyle(.bordered)
            }
        }
        .padding(20)
        .frame(width: 600, height: 500)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(12)
        .shadow(radius: 8)
    }
}

// MARK: - Fuzzy Search Algorithm
class FuzzySearch {
    static func levenshteinDistance(_ str1: String, _ str2: String) -> Int {
        let str1Array = Array(str1.lowercased())
        let str2Array = Array(str2.lowercased())
        
        var matrix = Array(repeating: Array(repeating: 0, count: str2Array.count + 1), count: str1Array.count + 1)
        
        for i in 0...str1Array.count {
            matrix[i][0] = i
        }
        
        for j in 0...str2Array.count {
            matrix[0][j] = j
        }
        
        for i in 1...str1Array.count {
            for j in 1...str2Array.count {
                let cost = str1Array[i-1] == str2Array[j-1] ? 0 : 1
                matrix[i][j] = min(
                    matrix[i-1][j] + 1,
                    matrix[i][j-1] + 1,
                    matrix[i-1][j-1] + cost
                )
            }
        }
        
        return matrix[str1Array.count][str2Array.count]
    }
    
    static func fuzzyMatch(_ query: String, _ target: String, threshold: Double = 0.6) -> Double {
        let query = query.lowercased()
        let target = target.lowercased()
        
        if query.isEmpty { return 1.0 }
        if target.isEmpty { return 0.0 }
        
        let distance = levenshteinDistance(query, target)
        let maxLength = max(query.count, target.count)
        let similarity = 1.0 - (Double(distance) / Double(maxLength))
        
        // Bonus points for exact matches and prefix matches
        var bonus = 0.0
        if target.hasPrefix(query) { bonus += 0.2 }
        if target.contains(query) { bonus += 0.1 }
        
        return min(1.0, similarity + bonus)
    }
}

// MARK: - Node Favorites System
class NodeFavorites: ObservableObject {
    static let shared = NodeFavorites()
    @Published var favorites: Set<NodeType> = []
    
    private let userDefaults = UserDefaults.standard
    private let favoritesKey = "NodeGraphFavorites"
    
    init() {
        loadFavorites()
    }
    
    func toggleFavorite(_ type: NodeType) {
        if favorites.contains(type) {
            favorites.remove(type)
        } else {
            favorites.insert(type)
        }
        saveFavorites()
    }
    
    func isFavorite(_ type: NodeType) -> Bool {
        return favorites.contains(type)
    }
    
    private func loadFavorites() {
        if let data = userDefaults.data(forKey: favoritesKey) {
            if let decoded = try? JSONDecoder().decode(Set<String>.self, from: data) {
                favorites = Set(decoded.compactMap { NodeType(rawValue: $0) })
            }
        }
    }
    
    private func saveFavorites() {
        let encoded = favorites.map { $0.rawValue }
        if let data = try? JSONEncoder().encode(encoded) {
            userDefaults.set(data, forKey: favoritesKey)
        }
    }
}

// MARK: - Recent Used Nodes System
class RecentNodes: ObservableObject {
    static let shared = RecentNodes()
    @Published var recentNodes: [NodeType] = []
    
    private let userDefaults = UserDefaults.standard
    private let recentKey = "NodeGraphRecent"
    private let maxRecentCount = 20
    
    init() {
        loadRecent()
    }
    
    func addRecent(_ type: NodeType) {
        // Remove if already exists
        recentNodes.removeAll { $0 == type }
        
        // Add to front
        recentNodes.insert(type, at: 0)
        
        // Limit to max count
        if recentNodes.count > maxRecentCount {
            recentNodes = Array(recentNodes.prefix(maxRecentCount))
        }
        
        saveRecent()
    }
    
    private func loadRecent() {
        if let data = userDefaults.data(forKey: recentKey) {
            if let decoded = try? JSONDecoder().decode([String].self, from: data) {
                recentNodes = decoded.compactMap { NodeType(rawValue: $0) }
            }
        }
    }
    
    private func saveRecent() {
        let encoded = recentNodes.map { $0.rawValue }
        if let data = try? JSONEncoder().encode(encoded) {
            userDefaults.set(data, forKey: recentKey)
        }
    }
}

// MARK: - Node Categories System
struct NodeCategory: Identifiable {
    let id = UUID()
    let name: String
    let icon: String
    let color: Color
    let nodes: [NodeType]
    let subcategories: [NodeCategory]?
}

let nodeCategories: [NodeCategory] = [
    NodeCategory(
        name: "Events",
        icon: "bolt.fill",
        color: .red,
        nodes: [.event],
        subcategories: [
            NodeCategory(name: "Lifecycle", icon: "play.circle", color: .orange, nodes: []),
            NodeCategory(name: "Input", icon: "keyboard", color: .blue, nodes: []),
            NodeCategory(name: "Collision", icon: "circle.hexagongrid", color: .purple, nodes: []),
            NodeCategory(name: "Trigger", icon: "hand.tap", color: .green, nodes: []),
            NodeCategory(name: "Custom", icon: "star.fill", color: .yellow, nodes: [])
        ]
    ),
    NodeCategory(
        name: "Flow Control",
        icon: "arrow.branch",
        color: .gray,
        nodes: [.branch, .sequence, .forLoop, .forEach],
        subcategories: [
            NodeCategory(name: "Control Flow", icon: "arrow.right", color: .gray, nodes: [.branch, .sequence]),
            NodeCategory(name: "Loops", icon: "repeat", color: .gray, nodes: [.forLoop, .forEach]),
            NodeCategory(name: "Timing", icon: "clock", color: .gray, nodes: []),
            NodeCategory(name: "Async", icon: "clock.arrow.circlepath", color: .gray, nodes: [])
        ]
    ),
    NodeCategory(
        name: "Math",
        icon: "plus.forwardslash.minus",
        color: .teal,
        nodes: [.math, .logic],
        subcategories: [
            NodeCategory(name: "Arithmetic", icon: "plusminus.circle", color: .green, nodes: [.math]),
            NodeCategory(name: "Trigonometry", icon: "triangle", color: .blue, nodes: [.math]),
            NodeCategory(name: "Vector", icon: "arrow.up.right", color: .yellow, nodes: [.math]),
            NodeCategory(name: "Matrix", icon: "square.3x3", color: .purple, nodes: [.math]),
            NodeCategory(name: "Logic", icon: "questionmark.diamond", color: .teal, nodes: [.logic]),
            NodeCategory(name: "Comparison", icon: "equal", color: .orange, nodes: [.logic])
        ]
    ),
    NodeCategory(
        name: "Data",
        icon: "externaldrive",
        color: .blue,
        nodes: [.variable, .variableGet, .variableSet, .constant, .constantValue],
        subcategories: [
            NodeCategory(name: "Variables", icon: "v.square", color: .orange, nodes: [.variable, .variableGet, .variableSet]),
            NodeCategory(name: "Constants", icon: "number", color: .orange, nodes: [.constant, .constantValue]),
            NodeCategory(name: "Arrays", icon: "square.grid.3x3", color: .orange, nodes: []),
            NodeCategory(name: "Dictionaries", icon: "book", color: .purple, nodes: []),
            NodeCategory(name: "Strings", icon: "textformat", color: .pink, nodes: []),
            NodeCategory(name: "Objects", icon: "cube", color: .blue, nodes: [.objectInstance, .structConstructor])
        ]
    ),
    NodeCategory(
        name: "Utilities",
        icon: "wrench.and.screwdriver",
        color: .secondary,
        nodes: [.comment, .reroute],
        subcategories: [
            NodeCategory(name: "Debug", icon: "ladybug", color: .red, nodes: [.comment]),
            NodeCategory(name: "Reroute", icon: "arrow.turn.right.down", color: .gray, nodes: [.reroute])
        ]
    )
]

// MARK: - Keyboard Shortcuts System
class NodeKeyboardShortcuts: ObservableObject {
    static let shared = NodeKeyboardShortcuts()
    
    private let nodeShortcuts: [String: NodeType] = [
        "E": .event,
        "B": .flowBranch,
        "S": .flowSequence,
        "F": .loopFor,
        "V": .variable,
        "C": .constant,
        "M": .mathAdd,
        "L": .logicAnd,
        "T": .compareEqual,
        "R": .comment,
        "A": .stringConcat
    ]
    
    func getNodeForKey(_ key: String) -> NodeType? {
        return nodeShortcuts[key.uppercased()]
    }
    
    func getAllShortcuts() -> [(String, NodeType)] {
        return nodeShortcuts.map { ($0.key, $0.value) }.sorted { $0.0 < $1.0 }
    }
}

// MARK: - Context Menu Node Creation
struct NodeContextMenu: View {
    let position: CGPoint
    let onSelect: (NodeType) -> Void
    
    var body: some View {
        Menu {
            // Events Section
            Menu("Events") {
                Button("On Start") { onSelect(.onStart) }
                Button("On Update") { onSelect(.onUpdate) }
                Button("On Destroy") { onSelect(.onDestroy) }
                Divider()
                Button("Keyboard Input") { onSelect(.inputKeyboard) }
                Button("Mouse Input") { onSelect(.inputMouse) }
                Button("Touch Input") { onSelect(.inputTouch) }
                Divider()
                Button("Collision Enter") { onSelect(.collisionEnter) }
                Button("Collision Exit") { onSelect(.collisionExit) }
                Button("Collision Stay") { onSelect(.collisionStay) }
                Divider()
                Button("Trigger Enter") { onSelect(.triggerEnter) }
                Button("Trigger Exit") { onSelect(.triggerExit) }
                Button("Trigger Stay") { onSelect(.triggerStay) }
                Divider()
                Button("Custom Event") { onSelect(.customEvent) }
            }
            
            // Flow Control Section
            Menu("Flow Control") {
                Button("Branch") { onSelect(.flowBranch) }
                Button("Switch") { onSelect(.flowSwitch) }
                Button("Select") { onSelect(.flowSelect) }
                Divider()
                Button("For Loop") { onSelect(.loopFor) }
                Button("For Each Loop") { onSelect(.loopForEach) }
                Button("While Loop") { onSelect(.loopWhile) }
                Button("Do While Loop") { onSelect(.loopDoWhile) }
                Divider()
                Button("Sequence") { onSelect(.flowSequence) }
                Button("Gate") { onSelect(.gate) }
                Button("Latch") { onSelect(.latch) }
                Divider()
                Button("Delay") { onSelect(.delay) }
                Button("Timer") { onSelect(.timer) }
                Button("Async/Await") { onSelect(.asyncAwait) }
            }
            
            // Math Section
            Menu("Math") {
                Menu("Arithmetic") {
                    Button("Add") { onSelect(.mathAdd) }
                    Button("Subtract") { onSelect(.mathSubtract) }
                    Button("Multiply") { onSelect(.mathMultiply) }
                    Button("Divide") { onSelect(.mathDivide) }
                    Button("Modulo") { onSelect(.mathModulo) }
                }
                Menu("Trigonometry") {
                    Button("Sin") { onSelect(.mathSin) }
                    Button("Cos") { onSelect(.mathCos) }
                    Button("Tan") { onSelect(.mathTan) }
                    Button("Arc Sin") { onSelect(.mathAsin) }
                    Button("Arc Cos") { onSelect(.mathAcos) }
                    Button("Arc Tan") { onSelect(.mathAtan) }
                }
                Menu("Utility") {
                    Button("Power") { onSelect(.mathPow) }
                    Button("Square Root") { onSelect(.mathSqrt) }
                    Button("Absolute") { onSelect(.mathAbs) }
                    Button("Minimum") { onSelect(.mathMin) }
                    Button("Maximum") { onSelect(.mathMax) }
                    Button("Clamp") { onSelect(.mathClamp) }
                    Divider()
                    Button("Lerp") { onSelect(.mathLerp) }
                    Button("Inverse Lerp") { onSelect(.mathInverseLerp) }
                    Button("Smooth Step") { onSelect(.mathSmoothStep) }
                }
            }
            
            // Vector Section
            Menu("Vector") {
                Menu("Basic Operations") {
                    Button("Add") { onSelect(.vectorAdd) }
                    Button("Subtract") { onSelect(.vectorSubtract) }
                    Button("Multiply") { onSelect(.vectorMultiply) }
                    Button("Divide") { onSelect(.vectorDivide) }
                }
                Menu("Advanced") {
                    Button("Dot Product") { onSelect(.vectorDot) }
                    Button("Cross Product") { onSelect(.vectorCross) }
                    Button("Magnitude") { onSelect(.vectorMagnitude) }
                    Button("Normalize") { onSelect(.vectorNormalize) }
                    Button("Distance") { onSelect(.vectorDistance) }
                    Button("Reflect") { onSelect(.vectorReflect) }
                    Button("Project") { onSelect(.vectorProject) }
                    Button("Angle") { onSelect(.vectorAngle) }
                }
                Menu("Constructors") {
                    Button("Make Vector2") { onSelect(.vector2Make) }
                    Button("Make Vector3") { onSelect(.vector3Make) }
                    Button("Make Vector4") { onSelect(.vector4Make) }
                    Button("Make Quaternion") { onSelect(.quaternionMake) }
                }
            }
            
            // Matrix Section
            Menu("Matrix") {
                Button("Make Matrix") { onSelect(.matrixMake) }
                Button("Identity") { onSelect(.matrixIdentity) }
                Button("Transpose") { onSelect(.matrixTranspose) }
                Button("Inverse") { onSelect(.matrixInverse) }
                Button("Multiply") { onSelect(.matrixMultiply) }
                Button("Determinant") { onSelect(.matrixDeterminant) }
                Button("Decompose") { onSelect(.matrixDecompose) }
                Divider()
                Button("Translation") { onSelect(.matrixTranslation) }
                Button("Rotation") { onSelect(.matrixRotation) }
                Button("Scale") { onSelect(.matrixScale) }
                Button("TRS") { onSelect(.matrixTRS) }
            }
            
            // Logic Section
            Menu("Logic") {
                Button("AND") { onSelect(.logicAnd) }
                Button("OR") { onSelect(.logicOr) }
                Button("NOT") { onSelect(.logicNot) }
                Button("XOR") { onSelect(.logicXor) }
                Divider()
                Button("NAND") { onSelect(.logicNand) }
                Button("NOR") { onSelect(.logicNor) }
                Button("Implies") { onSelect(.logicImplies) }
                Button("Equivalent") { onSelect(.logicEquivalent) }
                Divider()
                Button("Bitwise AND") { onSelect(.logicBitwiseAnd) }
                Button("Bitwise OR") { onSelect(.logicBitwiseOr) }
                Button("Bitwise XOR") { onSelect(.logicBitwiseXor) }
                Button("Bitwise NOT") { onSelect(.logicBitwiseNot) }
                Divider()
                Button("Shift Left") { onSelect(.logicShiftLeft) }
                Button("Shift Right") { onSelect(.logicShiftRight) }
            }
            
            // Comparison Section
            Menu("Comparison") {
                Button("Equal") { onSelect(.compareEqual) }
                Button("Not Equal") { onSelect(.compareNotEqual) }
                Button("Less") { onSelect(.compareLess) }
                Button("Less or Equal") { onSelect(.compareLessEqual) }
                Button("Greater") { onSelect(.compareGreater) }
                Button("Greater or Equal") { onSelect(.compareGreaterEqual) }
                Divider()
                Button("Approximately") { onSelect(.compareApproximately) }
                Button("Is Null") { onSelect(.compareIsNone) }
                Button("Is Not Null") { onSelect(.compareIsNotNull) }
                Button("Is True") { onSelect(.compareIsTrue) }
                Button("Is False") { onSelect(.compareIsFalse) }
            }
            
            // String Section
            Menu("String") {
                Button("Concatenate") { onSelect(.stringConcat) }
                Button("Substring") { onSelect(.stringSubstring) }
                Button("Length") { onSelect(.stringLength) }
                Button("To Upper") { onSelect(.stringUpper) }
                Button("To Lower") { onSelect(.stringLower) }
                Divider()
                Button("Replace") { onSelect(.stringReplace) }
                Button("Split") { onSelect(.stringSplit) }
                Button("Join") { onSelect(.stringJoin) }
                Divider()
                Button("Trim") { onSelect(.stringTrim) }
                Button("Pad Left") { onSelect(.stringPadLeft) }
                Button("Pad Right") { onSelect(.stringPadRight) }
                Button("Format") { onSelect(.stringFormat) }
            }
            
            Divider()
            
            // Variables Section
            Menu("Variables") {
                Button("Variable") { onSelect(.variable) }
                Button("Get Variable") { onSelect(.variableGet) }
                Button("Set Variable") { onSelect(.variableSet) }
                Divider()
                Button("Constant") { onSelect(.constant) }
                Button("Constant Value") { onSelect(.constantValue) }
            }
            
            // Data Section
            Menu("Data") {
                Button("Literal") { onSelect(.literal) }
                Button("Enum Value") { onSelect(.enumValue) }
                Button("Struct Constructor") { onSelect(.structConstructor) }
                Button("Object Instance") { onSelect(.objectInstance) }
            }
            
            // Graph Section
            Menu("Graph") {
                Button("Macro") { onSelect(.macro) }
                Button("Graph Instance") { onSelect(.graphInstance) }
                Button("Graph Input") { onSelect(.graphInput) }
                Button("Graph Output") { onSelect(.graphOutput) }
            }
            
            // Utility Section
            Menu("Utility") {
                Button("Comment") { onSelect(.comment) }
                Button("Reroute") { onSelect(.reroute) }
            }
        } label: {
            Text("Add Node")
        }
    }
}
struct EnhancedNodeCreationMenu: View {
    let position: CGPoint
    let onSelect: (NodeType) -> Void
    @State private var searchText = ""
    @State private var selectedCategory = "All"
    @State private var showFavorites = false
    @State private var showRecent = false
    
    @ObservedObject private var favorites = NodeFavorites.shared
    @ObservedObject private var recentNodes = RecentNodes.shared
    
    private let nodeCategories: [String: [NodeType]] = [
        "Events": [
            .onStart, .onUpdate, .onDestroy,
            .inputKeyboard, .inputMouse, .inputTouch,
            .collisionEnter, .collisionExit, .collisionStay,
            .triggerEnter, .triggerExit, .triggerStay,
            .customEvent
        ],
        "Flow Control": [
            .flowBranch, .flowSwitch, .flowSelect,
            .loopFor, .loopForEach, .loopWhile, .loopDoWhile,
            .flowSequence, .gate, .latch, .delay, .timer, .asyncAwait
        ],
        "Math": [
            .mathAdd, .mathSubtract, .mathMultiply, .mathDivide, .mathModulo,
            .mathSin, .mathCos, .mathTan, .mathAsin, .mathAcos, .mathAtan,
            .mathPow, .mathSqrt, .mathAbs, .mathMin, .mathMax, .mathClamp,
            .mathLerp, .mathInverseLerp, .mathSmoothStep
        ],
        "Vector": [
            .vectorAdd, .vectorSubtract, .vectorMultiply, .vectorDivide,
            .vectorDot, .vectorCross, .vectorMagnitude, .vectorNormalize,
            .vectorDistance, .vectorReflect, .vectorProject, .vectorAngle,
            .vector3Make, .vector2Make, .vector4Make, .quaternionMake
        ],
        "Matrix": [
            .matrixMake, .matrixIdentity, .matrixTranspose, .matrixInverse,
            .matrixMultiply, .matrixDeterminant, .matrixDecompose,
            .matrixTranslation, .matrixRotation, .matrixScale, .matrixTRS
        ],
        "Logic": [
            .logicAnd, .logicOr, .logicNot, .logicXor, .logicNand, .logicNor,
            .logicImplies, .logicEquivalent, .logicBitwiseAnd, .logicBitwiseOr,
            .logicBitwiseXor, .logicBitwiseNot, .logicShiftLeft, .logicShiftRight
        ],
        "Comparison": [
            .compareEqual, .compareNotEqual, .compareLess, .compareLessEqual,
            .compareGreater, .compareGreaterEqual, .compareApproximately,
            .compareIsNone, .compareIsNotNull, .compareIsTrue, .compareIsFalse
        ],
        "String": [
            .stringConcat, .stringSubstring, .stringLength, .stringUpper,
            .stringLower, .stringReplace, .stringSplit, .stringJoin,
            .stringTrim, .stringPadLeft, .stringPadRight, .stringFormat
        ],
        "Variables": [.variable, .variableGet, .variableSet, .constant, .constantValue],
        "Data": [.literal, .enumValue, .structConstructor, .objectInstance],
        "Graph": [.macro, .graphInstance, .graphInput, .graphOutput],
        "Utility": [.comment, .reroute]
    ]
    
    var categories: [String] {
        ["All", "Favorites", "Recent"] + Array(nodeCategories.keys).sorted()
    }
    
    var filteredTypes: [NodeType] {
        let baseTypes: [NodeType]
        
        switch selectedCategory {
        case "All":
            baseTypes = NodeType.allCases
        case "Favorites":
            baseTypes = Array(favorites.favorites)
        case "Recent":
            baseTypes = recentNodes.recentNodes
        default:
            baseTypes = nodeCategories[selectedCategory] ?? []
        }
        
        if searchText.isEmpty {
            return baseTypes
        }
        
        // Use fuzzy search
        let scoredTypes = baseTypes.map { type in
            let score = FuzzySearch.fuzzyMatch(searchText, type.rawValue)
            return (type: type, score: score)
        }
        
        return scoredTypes
            .filter { $0.score >= 0.3 }
            .sorted { $0.score > $1.score }
            .map { $0.type }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Search with quick-add hint
            HStack {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(.white.opacity(0.6))
                TextField("Quick-add (type name)...", text: $searchText)
                    .textFieldStyle(.plain)
                    .onSubmit {
                        if let firstMatch = filteredTypes.first {
                            onSelect(firstMatch)
                        }
                    }
            }
            .padding(8)
            .background(Color.white.opacity(0.1))
            
            // Category Filter with Favorites and Recent
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    ForEach(categories, id: \.self) { category in
                        Button(action: { selectedCategory = category }) {
                            HStack(spacing: 4) {
                                Image(systemName: iconForCategory(category))
                                    .font(.system(size: 10))
                                Text(category)
                                    .font(.caption)
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(selectedCategory == category ? Color.blue : Color.white.opacity(0.1))
                            .foregroundColor(.white)
                            .cornerRadius(6)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.horizontal, 8)
            }
            .padding(.vertical, 4)
            
            // Node List with favorites toggle
            ScrollView {
                LazyVStack(alignment: .leading, spacing: 0) {
                    ForEach(filteredTypes, id: \.self) { type in
                        Button(action: { 
                            onSelect(type)
                            recentNodes.addRecent(type)
                        }) {
                            HStack {
                                Image(systemName: iconForType(type))
                                    .font(.system(size: 12))
                                    .foregroundColor(nodeColorForType(type))
                                
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(type.rawValue.capitalized)
                                        .foregroundColor(.white)
                                        .font(.caption)
                                    
                                    Text(descriptionForType(type))
                                        .foregroundColor(.white.opacity(0.6))
                                        .font(.caption2)
                                }
                                
                                Spacer()
                                
                                // Favorite toggle
                                Button(action: { favorites.toggleFavorite(type) }) {
                                    Image(systemName: favorites.isFavorite(type) ? "star.fill" : "star")
                                        .foregroundColor(favorites.isFavorite(type) ? .yellow : .white.opacity(0.4))
                                        .font(.system(size: 10))
                                }
                                .buttonStyle(.plain)
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 6)
                            .frame(maxWidth: .infinity, alignment: .leading)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }
            .frame(maxHeight: 250)
        }
        .frame(width: 300)
        .background(Color(red: 0.2, green: 0.2, blue: 0.22))
        .cornerRadius(8)
        .shadow(radius: 8)
        .position(position)
    }
    
    func iconForCategory(_ category: String) -> String {
        switch category {
        case "All": return "square.grid.2x2"
        case "Favorites": return "star.fill"
        case "Recent": return "clock"
        case "Flow Control": return "arrow.branch"
        case "Variables": return "v.square"
        case "Data": return "externaldrive"
        case "Graph": return "cube.box"
        case "Logic": return "questionmark.diamond"
        case "Utility": return "wrench"
        default: return "folder"
        }
    }
    
    func descriptionForType(_ type: NodeType) -> String {
        switch type {
        // Event nodes
        case .event: return "Event trigger"
        case .onStart: return "On start event"
        case .onUpdate: return "On update event"
        case .onDestroy: return "On destroy event"
        case .inputKeyboard: return "Keyboard input event"
        case .inputMouse: return "Mouse input event"
        case .inputTouch: return "Touch input event"
        case .collisionEnter: return "Collision enter event"
        case .collisionExit: return "Collision exit event"
        case .collisionStay: return "Collision stay event"
        case .triggerEnter: return "Trigger enter event"
        case .triggerExit: return "Trigger exit event"
        case .triggerStay: return "Trigger stay event"
        case .customEvent: return "Custom event"
        
        // Flow control nodes
        case .branch, .flowBranch: return "Conditional branch"
        case .sequence, .flowSequence: return "Execute in sequence"
        case .forLoop, .loopFor: return "For loop"
        case .forEach, .loopForEach: return "For each loop"
        case .loopWhile: return "While loop"
        case .loopDoWhile: return "Do while loop"
        case .flowSwitch: return "Switch statement"
        case .flowSelect: return "Select value"
        case .gate: return "Gate control"
        case .latch: return "Latch control"
        case .delay: return "Delay execution"
        case .timer: return "Timer event"
        case .asyncAwait: return "Async/await"
        
        // Math nodes
        case .math: return "Math operation"
        case .mathAdd: return "Addition"
        case .mathSubtract: return "Subtraction"
        case .mathMultiply: return "Multiplication"
        case .mathDivide: return "Division"
        case .mathModulo: return "Modulo"
        case .mathSin: return "Sine"
        case .mathCos: return "Cosine"
        case .mathTan: return "Tangent"
        case .mathAsin: return "Arc sine"
        case .mathAcos: return "Arc cosine"
        case .mathAtan: return "Arc tangent"
        case .mathPow: return "Power"
        case .mathSqrt: return "Square root"
        case .mathAbs: return "Absolute value"
        case .mathMin: return "Minimum"
        case .mathMax: return "Maximum"
        case .mathClamp: return "Clamp value"
        case .mathLerp: return "Linear interpolation"
        case .mathInverseLerp: return "Inverse lerp"
        case .mathSmoothStep: return "Smooth step"
        
        // Vector math nodes
        case .vectorAdd: return "Vector addition"
        case .vectorSubtract: return "Vector subtraction"
        case .vectorMultiply: return "Vector multiplication"
        case .vectorDivide: return "Vector division"
        case .vectorDot: return "Vector dot product"
        case .vectorCross: return "Vector cross product"
        case .vectorMagnitude: return "Vector magnitude"
        case .vectorNormalize: return "Vector normalize"
        case .vectorDistance: return "Vector distance"
        case .vectorReflect: return "Vector reflect"
        case .vectorProject: return "Vector project"
        case .vectorAngle: return "Vector angle"
        case .vector3Make: return "Make Vector3"
        case .vector2Make: return "Make Vector2"
        case .vector4Make: return "Make Vector4"
        case .quaternionMake: return "Make Quaternion"
        
        // Matrix math nodes
        case .matrixMake: return "Make matrix"
        case .matrixIdentity: return "Identity matrix"
        case .matrixTranspose: return "Matrix transpose"
        case .matrixInverse: return "Matrix inverse"
        case .matrixMultiply: return "Matrix multiply"
        case .matrixDeterminant: return "Matrix determinant"
        case .matrixDecompose: return "Matrix decompose"
        case .matrixTranslation: return "Translation matrix"
        case .matrixRotation: return "Rotation matrix"
        case .matrixScale: return "Scale matrix"
        case .matrixTRS: return "TRS matrix"
        
        // Logic nodes
        case .logic: return "Logic operation"
        case .logicAnd: return "Logical AND"
        case .logicOr: return "Logical OR"
        case .logicNot: return "Logical NOT"
        case .logicXor: return "Logical XOR"
        case .logicNand: return "Logical NAND"
        case .logicNor: return "Logical NOR"
        case .logicImplies: return "Logical implies"
        case .logicEquivalent: return "Logical equivalent"
        case .logicBitwiseAnd: return "Bitwise AND"
        case .logicBitwiseOr: return "Bitwise OR"
        case .logicBitwiseXor: return "Bitwise XOR"
        case .logicBitwiseNot: return "Bitwise NOT"
        case .logicShiftLeft: return "Shift left"
        case .logicShiftRight: return "Shift right"
        
        // Comparison nodes
        case .compareEqual: return "Equal comparison"
        case .compareNotEqual: return "Not equal comparison"
        case .compareLess: return "Less than comparison"
        case .compareLessEqual: return "Less or equal comparison"
        case .compareGreater: return "Greater than comparison"
        case .compareGreaterEqual: return "Greater or equal comparison"
        case .compareApproximately: return "Approximately equal"
        case .compareIsNone: return "Is null check"
        case .compareIsNotNull: return "Is not null check"
        case .compareIsTrue: return "Is true check"
        case .compareIsFalse: return "Is false check"
        
        // String manipulation nodes
        case .stringConcat: return "String concatenate"
        case .stringSubstring: return "String substring"
        case .stringLength: return "String length"
        case .stringUpper: return "String to upper"
        case .stringLower: return "String to lower"
        case .stringReplace: return "String replace"
        case .stringSplit: return "String split"
        case .stringJoin: return "String join"
        case .stringTrim: return "String trim"
        case .stringPadLeft: return "String pad left"
        case .stringPadRight: return "String pad right"
        case .stringFormat: return "String format"
        
        // Original nodes
        case .getter: return "Get property"
        case .setter: return "Set property"
        case .function: return "Function call"
        case .variable: return "Variable"
        case .variableGet: return "Get variable"
        case .variableSet: return "Set variable"
        case .constant: return "Constant value"
        case .constantValue: return "Constant"
        case .comment: return "Comment"
        case .macro: return "Macro"
        case .graphInstance: return "Graph instance"
        case .graphInput: return "Graph input"
        case .graphOutput: return "Graph output"
        case .literal: return "Literal value"
        case .enumValue: return "Enum value"
        case .structConstructor: return "Struct constructor"
        case .objectInstance: return "Object instance"
        case .reroute: return "Reroute connection"
        default: return "Node"
        }
    }
    
    func iconForType(_ type: NodeType) -> String {
        switch type {
        // Event nodes
        case .event, .onStart, .onUpdate, .onDestroy: return "bolt.fill"
        case .inputKeyboard: return "keyboard"
        case .inputMouse: return "mouse"
        case .inputTouch: return "hand.tap"
        case .collisionEnter, .collisionExit, .collisionStay: return "circle.hexagongrid"
        case .triggerEnter, .triggerExit, .triggerStay: return "target"
        case .customEvent: return "star.fill"
        
        // Flow control nodes
        case .branch, .flowBranch: return "arrow.branch"
        case .sequence, .flowSequence: return "arrow.right"
        case .forLoop, .forEach, .loopFor, .loopForEach: return "repeat"
        case .loopWhile, .loopDoWhile: return "arrow.clockwise"
        case .flowSwitch: return "arrow.up.arrow.down.square"
        case .flowSelect: return "arrowtriangle.up.square"
        case .gate: return "gate"
        case .latch: return "lock"
        case .delay, .timer: return "clock"
        case .asyncAwait: return "clock.arrow.circlepath"
        
        // Math nodes
        case .math, .mathAdd, .mathSubtract, .mathMultiply, .mathDivide, .mathModulo: return "plus.forwardslash.minus"
        case .mathSin, .mathCos, .mathTan, .mathAsin, .mathAcos, .mathAtan: return "triangle"
        case .mathPow, .mathSqrt, .mathAbs, .mathMin, .mathMax, .mathClamp: return "function"
        case .mathLerp, .mathInverseLerp, .mathSmoothStep: return "arrow.up.and.down.text.horizontal"
        
        // Vector math nodes
        case .vectorAdd, .vectorSubtract, .vectorMultiply, .vectorDivide: return "arrow.up.right"
        case .vectorDot, .vectorCross, .vectorMagnitude, .vectorNormalize: return "arrow.trianglehead.2"
        case .vectorDistance, .vectorReflect, .vectorProject, .vectorAngle: return "ruler"
        case .vector3Make, .vector2Make, .vector4Make, .quaternionMake: return "cube"
        
        // Matrix math nodes
        case .matrixMake, .matrixIdentity, .matrixTranspose, .matrixInverse: return "square.3x3"
        case .matrixMultiply, .matrixDeterminant, .matrixDecompose: return "square.3x3.fill"
        case .matrixTranslation, .matrixRotation, .matrixScale, .matrixTRS: return "move.3d"
        
        // Logic nodes
        case .logic, .logicAnd, .logicOr, .logicNot, .logicXor: return "questionmark.diamond"
        case .logicNand, .logicNor, .logicImplies, .logicEquivalent: return "questionmark.diamond.fill"
        case .logicBitwiseAnd, .logicBitwiseOr, .logicBitwiseXor, .logicBitwiseNot: return "bit"
        case .logicShiftLeft, .logicShiftRight: return "arrow.left.arrow.right"
        
        // Comparison nodes
        case .compareEqual, .compareNotEqual, .compareLess, .compareLessEqual: return "equal"
        case .compareGreater, .compareGreaterEqual, .compareApproximately: return "greaterthan"
        case .compareIsNone, .compareIsNotNull, .compareIsTrue, .compareIsFalse: return "checkmark.circle"
        
        // String manipulation nodes
        case .stringConcat, .stringSubstring, .stringLength, .stringUpper: return "textformat"
        case .stringLower, .stringReplace, .stringSplit, .stringJoin: return "textformat.abc"
        case .stringTrim, .stringPadLeft, .stringPadRight, .stringFormat: return "textformat.alt"
        
        // Original nodes
        case .getter: return "arrow.down.doc"
        case .setter: return "arrow.up.doc"
        case .function: return "function"
        case .variable, .variableGet, .variableSet: return "v.square"
        case .constant, .constantValue: return "number"
        case .comment: return "text.bubble"
        case .macro: return "cube.box"
        case .graphInstance: return "cube.transparent"
        case .graphInput: return "arrow.right.circle"
        case .graphOutput: return "arrow.left.circle"
        case .literal: return "textformat"
        case .enumValue: return "list.bullet"
        case .structConstructor: return "square.stack.3d.up"
        case .objectInstance: return "circle.3d"
        case .reroute: return "arrow.turn.right.down"
        }
    }
    
    func nodeColorForType(_ type: NodeType) -> Color {
        switch type {
        // Event nodes
        case .event, .onStart, .onUpdate, .onDestroy: return .red
        case .inputKeyboard, .inputMouse, .inputTouch: return .orange
        case .collisionEnter, .collisionExit, .collisionStay: return .purple
        case .triggerEnter, .triggerExit, .triggerStay: return .green
        case .customEvent: return .yellow
        
        // Flow control nodes
        case .branch, .sequence, .forLoop, .forEach, .flowBranch, .flowSequence, .loopFor, .loopForEach: return .gray
        case .loopWhile, .loopDoWhile, .flowSwitch, .flowSelect: return .gray
        case .gate, .latch, .delay, .timer, .asyncAwait: return .gray
        
        // Math nodes
        case .math, .mathAdd, .mathSubtract, .mathMultiply, .mathDivide, .mathModulo: return .teal
        case .mathSin, .mathCos, .mathTan, .mathAsin, .mathAcos, .mathAtan: return .blue
        case .mathPow, .mathSqrt, .mathAbs, .mathMin, .mathMax, .mathClamp: return .green
        case .mathLerp, .mathInverseLerp, .mathSmoothStep: return .purple
        
        // Vector math nodes
        case .vectorAdd, .vectorSubtract, .vectorMultiply, .vectorDivide: return .yellow
        case .vectorDot, .vectorCross, .vectorMagnitude, .vectorNormalize: return .orange
        case .vectorDistance, .vectorReflect, .vectorProject, .vectorAngle: return .red
        case .vector3Make, .vector2Make, .vector4Make, .quaternionMake: return .cyan
        
        // Matrix math nodes
        case .matrixMake, .matrixIdentity, .matrixTranspose, .matrixInverse: return .purple
        case .matrixMultiply, .matrixDeterminant, .matrixDecompose: return .indigo
        case .matrixTranslation, .matrixRotation, .matrixScale, .matrixTRS: return .mint
        
        // Logic nodes
        case .logic, .logicAnd, .logicOr, .logicNot, .logicXor, .logicNand, .logicNor: return .teal
        case .logicImplies, .logicEquivalent, .logicBitwiseAnd, .logicBitwiseOr: return .teal
        case .logicBitwiseXor, .logicBitwiseNot, .logicShiftLeft, .logicShiftRight: return .teal
        
        // Comparison nodes
        case .compareEqual, .compareNotEqual, .compareLess, .compareLessEqual: return .orange
        case .compareGreater, .compareGreaterEqual, .compareApproximately: return .orange
        case .compareIsNone, .compareIsNotNull, .compareIsTrue, .compareIsFalse: return .orange
        
        // String manipulation nodes
        case .stringConcat, .stringSubstring, .stringLength, .stringUpper: return .pink
        case .stringLower, .stringReplace, .stringSplit, .stringJoin: return .pink
        case .stringTrim, .stringPadLeft, .stringPadRight, .stringFormat: return .pink
        
        // Original nodes
        case .getter: return .green
        case .setter: return .blue
        case .function: return .purple
        case .variable, .variableGet, .variableSet: return .orange
        case .constant, .constantValue: return .orange
        case .comment: return .yellow.opacity(0.3)
        case .macro: return .purple.opacity(0.8)
        case .graphInstance: return .cyan.opacity(0.8)
        case .uiShow, .uiHide, .uiSetText, .uiGetText, .uiSetPosition, .uiGetPosition, .uiSetSize,
             .uiGetSize, .uiSetVisible, .uiIsVisible, .uiSetEnabled, .uiIsEnabled: return .green
        
        // File I/O nodes
        case .fileRead, .fileWrite, .fileExists, .fileDelete, .fileCopy, .fileMove, .fileGetSize,
             .fileGetModified: return .orange
        
        case .reroute: return .gray
        
        // Default cases for any remaining types
        case .typeCast, .typeCheck, .nullCheck, .safeAccess: return .secondary.opacity(0.8)
        }
    }
}
