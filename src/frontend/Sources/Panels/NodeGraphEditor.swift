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
    
    init() {
        loadDemoGraph()
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
    
    private var nodeTemplates: [NodeType: GraphNode] = [:]
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
    var comment: String = ""
}

struct NodePort: Identifiable {
    let id = UUID()
    var name: String
    var type: PortType
    var defaultValue: Any? = nil
}

struct GraphConnection: Identifiable {
    let id = UUID()
    var fromNode: UUID
    var fromPort: Int
    var toNode: UUID
    var toPort: Int
}

enum NodeType: String, CaseIterable {
    case event, branch, sequence, forLoop, forEach
    case getter, setter, function, math, logic
    case variable, constant, comment
}

enum PortType: String {
    case flow, bool, int, float, vector3, string, object, any
    
    var color: Color {
        switch self {
        case .flow: return .white
        case .bool: return .red
        case .int: return .cyan
        case .float: return .green
        case .vector3: return .yellow
        case .string: return .pink
        case .object: return .blue
        case .any: return .gray
        }
    }
}

// MARK: - Node Graph View
@available(macOS 14.0, *)
struct NodeGraphView: View {
    @ObservedObject var manager = NodeGraphManager.shared
    @State private var showNodeMenu = false
    @State private var nodeMenuPosition: CGPoint = .zero
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            NodeGraphToolbar(manager: manager)
            
            EditorDivider()
            
            // Canvas
            ZStack {
                // Background grid
                NodeGraphGrid(offset: manager.panOffset, zoom: manager.zoom)
                
                // Connections
                ForEach(manager.connections) { connection in
                    ConnectionPath(connection: connection, nodes: manager.nodes, offset: manager.panOffset, zoom: manager.zoom)
                }
                
                // Nodes
                ForEach($manager.nodes) { $node in
                    NodeView(node: $node, isSelected: manager.selectedNodes.contains(node.id), offset: manager.panOffset, zoom: manager.zoom)
                        .onTapGesture {
                            manager.selectedNodes = [node.id]
                        }
                }
                
                // Node creation menu
                if showNodeMenu {
                    NodeCreationMenu(position: nodeMenuPosition) { type in
                        manager.addNode(type: type, at: nodeMenuPosition)
                        showNodeMenu = false
                    }
                }
            }
            .background(Color(red: 0.1, green: 0.1, blue: 0.12))
            .gesture(
                DragGesture()
                    .onChanged { value in
                        manager.panOffset = CGPoint(
                            x: manager.panOffset.x + value.translation.width,
                            y: manager.panOffset.y + value.translation.height
                        )
                    }
            )
            .gesture(
                MagnifyGesture()
                    .onChanged { value in
                        manager.zoom = min(max(value.magnification, 0.5), 2.0)
                    }
            )
            .contextMenu {
                Button("Add Node...") {
                    showNodeMenu = true
                }
                Divider()
                Button("Delete Selected", role: .destructive) {
                    manager.deleteSelectedNodes()
                }
            }
        }
    }
}

// MARK: - Node Graph Toolbar
struct NodeGraphToolbar: View {
    @ObservedObject var manager: NodeGraphManager
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.md) {
            // Graph selector
            Menu("Main Graph") {
                Button("Main Graph") { }
                Button("Function: Calculate Damage") { }
                Button("Function: Apply Buff") { }
                Divider()
                Button("Create New Function...") { }
            }
            
            Divider().frame(height: 20)
            
            // Compile
            Button(action: {}) {
                Label("Compile", systemImage: "hammer")
            }
            
            // Debug
            Button(action: {}) {
                Label("Debug", systemImage: "ladybug")
            }
            
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
    let gridSize: CGFloat = 20
    
    var body: some View {
        Canvas { context, size in
            let scaledGridSize = gridSize * zoom
            
            // Minor grid
            for x in stride(from: offset.x.truncatingRemainder(dividingBy: scaledGridSize), to: size.width, by: scaledGridSize) {
                var path = Path()
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
                context.stroke(path, with: .color(Color.white.opacity(0.05)), lineWidth: 1)
            }
            
            for y in stride(from: offset.y.truncatingRemainder(dividingBy: scaledGridSize), to: size.height, by: scaledGridSize) {
                var path = Path()
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
                context.stroke(path, with: .color(Color.white.opacity(0.05)), lineWidth: 1)
            }
            
            // Major grid (every 5)
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
    }
}

// MARK: - Node View
struct NodeView: View {
    @Binding var node: GraphNode
    let isSelected: Bool
    let offset: CGPoint
    let zoom: CGFloat
    
    var nodeColor: Color {
        switch node.type {
        case .event: return .red
        case .branch, .sequence, .forLoop, .forEach: return .gray
        case .getter: return .green
        case .setter: return .blue
        case .function: return .purple
        case .math, .logic: return .teal
        case .variable, .constant: return .orange
        case .comment: return .yellow.opacity(0.3)
        }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Image(systemName: iconForType(node.type))
                    .font(.system(size: 12))
                Text(node.title)
                    .font(DesignSystem.Typography.bodyBold)
                Spacer()
            }
            .foregroundColor(.white)
            .padding(.horizontal, 8)
            .padding(.vertical, 6)
            .background(nodeColor)
            
            // Body with ports
            HStack(alignment: .top, spacing: 0) {
                // Input ports
                VStack(alignment: .leading, spacing: 4) {
                    ForEach(node.inputs) { port in
                        PortView(port: port, isOutput: false)
                    }
                }
                .frame(minWidth: 60)
                
                Spacer()
                
                // Output ports
                VStack(alignment: .trailing, spacing: 4) {
                    ForEach(node.outputs) { port in
                        PortView(port: port, isOutput: true)
                    }
                }
                .frame(minWidth: 60)
            }
            .padding(8)
            .background(Color(red: 0.15, green: 0.15, blue: 0.18))
        }
        .frame(minWidth: 150)
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : Color.clear, lineWidth: 2)
        )
        .shadow(color: .black.opacity(0.3), radius: 4, x: 2, y: 2)
        .position(
            x: node.position.x * zoom + offset.x,
            y: node.position.y * zoom + offset.y
        )
        .gesture(
            DragGesture()
                .onChanged { value in
                    node.position = CGPoint(
                        x: (value.location.x - offset.x) / zoom,
                        y: (value.location.y - offset.y) / zoom
                    )
                }
        )
    }
    
    func iconForType(_ type: NodeType) -> String {
        switch type {
        case .event: return "bolt.fill"
        case .branch: return "arrow.branch"
        case .sequence: return "arrow.right"
        case .forLoop, .forEach: return "repeat"
        case .getter: return "arrow.down.doc"
        case .setter: return "arrow.up.doc"
        case .function: return "function"
        case .math: return "plus.forwardslash.minus"
        case .logic: return "questionmark.diamond"
        case .variable: return "v.square"
        case .constant: return "number"
        case .comment: return "text.bubble"
        }
    }
}

// MARK: - Port View
struct PortView: View {
    let port: NodePort
    let isOutput: Bool
    @State private var isHovering = false
    
    var body: some View {
        HStack(spacing: 4) {
            if !isOutput {
                PortConnector(type: port.type, isHovering: isHovering)
            }
            
            Text(port.name)
                .font(DesignSystem.Typography.small)
                .foregroundColor(.white.opacity(0.8))
            
            if isOutput {
                PortConnector(type: port.type, isHovering: isHovering)
            }
        }
        .onHover { isHovering = $0 }
    }
}

struct PortConnector: View {
    let type: PortType
    let isHovering: Bool
    
    var body: some View {
        Group {
            if type == .flow {
                // Flow ports are triangular
                Image(systemName: "arrowtriangle.right.fill")
                    .font(.system(size: 10))
            } else {
                // Data ports are circular
                Circle()
                    .frame(width: 10, height: 10)
            }
        }
        .foregroundColor(type.color)
        .scaleEffect(isHovering ? 1.3 : 1.0)
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
