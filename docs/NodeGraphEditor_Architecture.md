# NodeGraphEditor Architecture Documentation

## Overview

The NodeGraphEditor is a comprehensive visual scripting system built with SwiftUI that provides professional-grade node-based programming capabilities. It features advanced debugging, extensive node type support, connection management, and a sophisticated user interface.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    NodeGraphEditor                           │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │   UI Layer      │  │  Debug Layer    │  │  Data Layer      │ │
│  │                 │  │                 │  │                 │ │
│  │ • NodeView      │  │ • DebugManager   │  │ • GraphNode      │ │
│  │ • PortView      │  │ • Breakpoints    │  │ • NodePort       │ │
│  │ • Connection    │  │ • Performance    │  │ • GraphConnection│ │
│  │ • Toolbar       │  │ • Variables      │  │ • GraphMetadata  │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐ │
│  │  Manager Layer  │  │  Utility Layer  │  │  Extension Layer│ │
│  │                 │  │                 │  │                 │ │
│  │ • NodeGraphMgr  │  │ • FuzzySearch    │  │ • NodeTypeExt    │ │
│  │ • ConnectionMgr │  │ • SearchHistory  │  │ • PortTypeExt    │ │
│  │ • ClipboardMgr  │  │ • PerformanceMgr │  │ • NodeCustomMgr  │ │
│  │ • KeyboardShort │  │ • ValidationMgr  │  │ • DebugUtils     │ │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Data Layer

#### GraphNode
```swift
struct GraphNode: Identifiable, Codable {
    let id: UUID
    var type: NodeType
    var title: String
    var position: CGPoint
    var inputs: [NodePort]
    var outputs: [NodePort]
    
    // Enhanced features
    var isCollapsed: Bool
    var isHeaderCollapsed: Bool
    var isDisabled: Bool
    var isBypassed: Bool
    var comment: String
    
    // Customization
    var customColor: Color?
    var customIcon: String?
    var customSize: CGSize?
    
    // Preview and debugging
    var previewValue: Any?
    var outputValues: [UUID: Any]
    var hasError: Bool
    var errorMessage: String?
    
    // Documentation
    var description: String
    var documentation: String
    
    // Size constraints
    var minWidth: CGFloat
    var minHeight: CGFloat
    var autoSize: Bool
    
    // Inline editing
    var isRenaming: Bool
    var tempTitle: String
}
```

#### NodePort
```swift
struct NodePort: Identifiable, Codable {
    let id = UUID()
    var name: String
    var type: PortType
    var defaultValue: Any?
    
    // Enhanced features
    var currentValue: Any?
    var isEditable: Bool
    var hasError: Bool
    var errorMessage: String?
    var description: String
}
```

#### GraphConnection
```swift
struct GraphConnection: Identifiable, Codable {
    let id = UUID()
    var fromNode: UUID
    var fromPort: Int
    var toNode: UUID
    var toPort: Int
    
    // Enhanced features
    var reroutePoints: [ConnectionReroutePoint]
    var isHidden: Bool
    var executionOrder: Int?
    var isValid: Bool
    var validationError: String?
}
```

### 2. Manager Layer

#### NodeGraphManager
```swift
class NodeGraphManager: ObservableObject {
    static let shared = NodeGraphManager()
    
    @Published var nodes: [GraphNode] = []
    @Published var connections: [GraphConnection] = []
    @Published var selectedNodes: Set<UUID> = []
    @Published var panOffset: CGPoint = .zero
    @Published var zoom: CGFloat = 1.0
    @Published var isConnecting: Bool = false
    @Published var showCompilationPanel: Bool = false
    @Published var showDebugPanels: Bool = false
    @Published var currentGraphName: String = "Main Graph"
    @Published var isDirty: Bool = false
    
    // Core operations
    func addNode(type: NodeType, at position: CGPoint)
    func deleteNode(_ nodeId: UUID)
    func deleteSelectedNodes()
    func duplicateSelectedNodes()
    func createConnection(from: UUID, fromPort: Int, to: UUID, toPort: Int)
    func deleteConnection(_ connectionId: UUID)
    
    // Graph operations
    func saveGraph() -> GraphData?
    func loadGraph(_ data: GraphData)
    func clearGraph()
    func validateGraph() -> [GraphError]
}
```

#### ConnectionManager
```swift
class ConnectionManager: ObservableObject {
    @Published var connections: [GraphConnection] = []
    @Published var selectedConnections: Set<UUID> = []
    @Published var connectionStyle: ConnectionStyle = .bezier
    @Published var connectionThickness: ConnectionThickness = .medium
    @Published var showExecutionOrder: Bool = false
    @Published var animateFlow: Bool = false
    @Published var animationProgress: Double = 0.0
    
    // Connection operations
    func createConnection(fromNode: UUID, fromPort: Int, toNode: UUID, toPort: Int, portType: PortType) -> GraphConnection?
    func deleteConnection(_ connectionId: UUID)
    func deleteSelectedConnections()
    
    // Validation
    private func validateConnection(fromNode: UUID, fromPort: Int, toNode: UUID, toPort: Int) -> (isValid: Bool, error: String?)
    private func wouldCreateLoop(fromNode: UUID, toNode: UUID) -> Bool
    
    // Execution order
    private func updateExecutionOrder()
    private func topologicalSort() -> [UUID]
    
    // Reroute points
    func addReroutePoint(to connectionId: UUID, at position: CGPoint)
    func removeReroutePoint(from connectionId: UUID, reroutePointId: UUID)
    func updateReroutePointPosition(connectionId: UUID, reroutePointId: UUID, position: CGPoint)
}
```

#### DebugManager
```swift
class DebugManager: ObservableObject {
    static let shared = DebugManager()
    
    // Debug state
    @Published var isDebugging: Bool = false
    @Published var isPaused: Bool = false
    @Published var isRunning: Bool = false
    @Published var currentExecutionNode: UUID?
    
    // Debug data
    @Published var callStack: [DebugStackFrame] = []
    @Published var localVariables: [String: DebugVariable] = [:]
    @Published var instanceVariables: [String: DebugVariable] = [:]
    @Published var breakpoints: Set<UUID> = []
    @Published var conditionalBreakpoints: [UUID: String] = [:]
    @Published var hitCountBreakpoints: [UUID: (current: Int, target: Int)] = [:]
    @Published var logPoints: [UUID: String] = []
    @Published var executionPath: [UUID] = []
    @Published var nodePerformance: [UUID: NodePerformanceData] = [:]
    
    // Execution controls
    func startDebugging()
    func stopDebugging()
    func pauseExecution()
    func continueExecution()
    func stepThrough()
    func stepInto()
    func stepOver()
    func stepOut()
    
    // Breakpoint management
    func toggleBreakpoint(on nodeId: UUID)
    func setConditionalBreakpoint(on nodeId: UUID, condition: String)
    func setHitCountBreakpoint(on nodeId: UUID, targetCount: Int)
    func addLogPoint(on nodeId: UUID, message: String)
    
    // Variable management
    func updateLocalVariables(_ variables: [String: Any])
    func updateInstanceVariables(_ variables: [String: Any])
    func evaluateExpression(_ expression: String) -> DebugVariable?
    
    // Performance tracking
    func startPerformanceTracking(nodeId: UUID)
    func endPerformanceTracking(nodeId: UUID)
    func recordNodePerformance(nodeId: UUID, executionTime: TimeInterval, memoryUsage: Int64)
}
```

### 3. UI Layer

#### NodeView
```swift
struct NodeView: View {
    @Binding var node: GraphNode
    let isSelected: Bool
    let offset: CGPoint
    let zoom: CGFloat
    
    // Enhanced features
    @State private var showTooltip = false
    @State private var showDocumentation = false
    @State private var showDeleteConfirmation = false
    @State private var showTypeReplacement = false
    @State private var isDragging = false
    
    // Computed properties
    var nodeColor: Color
    var nodeIcon: String
    var hasBreakpoint: Bool
    var isCurrentlyExecuting: Bool
    var effectiveNodeSize: CGSize
    
    // View components
    var headerView: some View
    var previewSection: some View
    var bodySection: some View
    var contextMenuItems: some View
}
```

#### EnhancedPortView
```swift
struct EnhancedPortView: View {
    let port: NodePort
    let isOutput: Bool
    let nodeId: UUID
    
    @State private var isHovering = false
    @State private var showDefaultValueEditor = false
    @State private var showTooltip = false
    @AppStorage("showPortNames") private var showPortNames = true
    
    // Features
    - Port value display
    - Default value editing
    - Enhanced tooltips
    - Context menu options
    - Error indicators
}
```

#### ConnectionRenderer
```swift
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
    
    // Rendering styles
    - Bezier curves
    - Straight lines
    - Orthogonal paths
    
    // Visual features
    - Flow animation
    - Execution order display
    - Validation error indicators
    - Selection highlighting
}
```

## Node Type System

### Categories
- **Events**: Event triggers and handlers
- **Flow Control**: Control flow and logic
- **Math**: Mathematical operations
- **Vector**: Vector mathematics
- **Matrix**: Matrix operations
- **Logic**: Logical operations
- **Comparison**: Value comparisons
- **String**: String manipulation
- **Data**: Data structures
- **Type System**: Type operations
- **Assets**: Asset management
- **Physics**: Physics operations
- **Audio**: Audio operations
- **Animation**: Animation control
- **UI**: User interface
- **File I/O**: File operations
- **Network**: Network operations
- **Debug**: Debug utilities
- **Utility**: Utility functions
- **Advanced**: Advanced features
- **Legacy**: Legacy nodes

### Node Type Examples
```swift
// Event nodes
case onStart, onUpdate, onDestroy, customEvent

// Math nodes
case mathAdd, mathSubtract, mathMultiply, mathDivide, mathPower

// Vector nodes
case vectorAdd, vectorSubtract, vectorDot, vectorCross, vectorNormalize

// Logic nodes
case logicAnd, logicOr, logicNot, logicXor

// String nodes
case stringConcat, stringSubstring, stringLength, stringReplace

// Data structure nodes
case arrayMake, arrayGet, arraySet, dictMake, dictGet, dictSet
```

## Port Type System

### Basic Types
- **flow**: Execution flow (triangular, white)
- **bool**: Boolean (circular, red)
- **int**: Integer (circular, cyan)
- **float**: Floating point (circular, green)
- **string**: Text string (circular, magenta)
- **object**: Object reference (circular, blue)

### Advanced Types
- **vector2, vector3, vector4**: Vector types (circular, yellow/orange)
- **quaternion**: Rotation (circular, orange)
- **array**: Array collection (dashed circle, orange)
- **dictionary**: Key-value pairs (circular, purple)
- **enumType**: Enumeration (circular, teal)
- **structType, classType**: Custom types (circular, purple)
- **optional**: Nullable type (circular, gray)
- **generic**: Generic type (circular, gray)
- **any**: Universal type (circular, gray)

### Port Features
- **Type compatibility checking**
- **Automatic type conversion**
- **Default value editing**
- **Real-time value display**
- **Error indicators**
- **Enhanced tooltips**

## Debugging System

### Debug Panels
1. **Call Stack**: Display execution call stack with function names and line numbers
2. **Variables**: Watch local and instance variables with expression evaluation
3. **Breakpoints**: Manage breakpoints, conditional breakpoints, hit count breakpoints, and log points
4. **Performance**: Per-node performance metrics including execution time and memory usage
5. **Console**: Real-time console output with categorized messages

### Breakpoint Types
- **Simple Breakpoints**: Toggle breakpoints on nodes
- **Conditional Breakpoints**: Break when condition is met
- **Hit Count Breakpoints**: Break after specific number of executions
- **Log Points**: Log messages without breaking execution

### Execution Control
- **Start/Stop**: Begin and end debugging sessions
- **Pause/Continue**: Pause and resume execution
- **Step Through**: Execute next node
- **Step Into**: Step into function calls
- **Step Over**: Skip function calls
- **Step Out**: Exit current function

## Search and Quick Add

### Fuzzy Search
```swift
struct FuzzySearch {
    static func fuzzyMatch(_ pattern: String, _ text: String) -> Double
    static func search<T: Identifiable>(_ pattern: String, in items: [T], keyPath: KeyPath<T, String>) -> [(item: T, score: Double)]
    static func highlightMatches(in pattern: String, text: String) -> [(character: Character, isMatch: Bool)]
    static func suggestCorrections(for pattern: String, in candidates: [String], maxSuggestions: Int = 5) -> [(suggestion: String, score: Double)]
}
```

### Search Features
- **Pattern matching**: Exact, prefix, contains, and fuzzy matching
- **Score-based ranking**: Results sorted by match quality
- **Highlight matching**: Visual highlighting of matched characters
- **Search history**: Recent and favorite searches
- **Performance caching**: Cached search results for performance

### Quick Add System
- **Keyboard shortcuts**: Single-key node creation
- **Fuzzy search**: Intelligent node type searching
- **Categorized results**: Organized by node categories
- **Recent nodes**: Quick access to recently used types

## Connection System

### Connection Styles
- **Bezier**: Smooth curved connections
- **Straight**: Direct line connections
- **Orthogonal**: Right-angle connections

### Connection Features
- **Reroute points**: Drag to add routing points
- **Flow animation**: Animated data flow visualization
- **Execution order**: Visual execution order display
- **Validation**: Real-time connection validation
- **Auto-insert**: Smart node insertion between connections
- **Merge**: Combine multiple connections

### Connection Validation
- **Type compatibility**: Check port type compatibility
- **Loop detection**: Prevent infinite loops
- **Duplicate prevention**: Prevent multiple connections to input ports
- **Error reporting**: Detailed validation error messages

## Performance Optimization

### Memory Management
- **Efficient data structures**: Use of value types where appropriate
- **Lazy loading**: Load resources on demand
- **Memory pooling**: Reuse objects to reduce allocations
- **Weak references**: Avoid retain cycles

### Rendering Optimization
- **View recycling**: Reuse view components
- **Dirty flagging**: Only update changed views
- **Level of detail**: Simplify rendering at zoom levels
- **Culling**: Skip off-screen rendering

### Search Performance
- **Result caching**: Cache search results
- **Incremental search**: Update results as user types
- **Background processing**: Perform heavy operations in background
- **Debouncing**: Reduce unnecessary searches

## Testing Strategy

### Unit Tests
- **Node operations**: Creation, deletion, modification
- **Connection management**: Creation, validation, deletion
- **Debug system**: Breakpoint management, variable tracking
- **Search functionality**: Fuzzy matching, result ranking
- **Type compatibility**: Port type checking and conversion

### Integration Tests
- **Full graph execution**: Complete node graph execution
- **Debugging workflow**: Start, step, stop debugging
- **UI interaction**: Node manipulation, connection creation
- **Performance testing**: Large graph handling

### UI Tests
- **Node rendering**: Visual correctness
- **Interaction**: Drag, drop, click operations
- **Accessibility**: VoiceOver, keyboard navigation
- **Responsive design**: Different screen sizes

## Usage Examples

### Creating a Simple Graph
```swift
// Create nodes
let eventNode = nodeGraphManager.addNode(type: .onStart, at: CGPoint(x: 100, y: 100))
let mathNode = nodeGraphManager.addNode(type: .mathAdd, at: CGPoint(x: 300, y: 100))
let variableNode = nodeGraphManager.addNode(type: .variable, at: CGPoint(x: 500, y: 100))

// Create connections
nodeGraphManager.createConnection(from: eventNode.id, fromPort: 0, to: mathNode.id, toPort: 0)
nodeGraphManager.createConnection(from: mathNode.id, fromPort: 0, to: variableNode.id, toPort: 0)
```

### Setting Up Debugging
```swift
// Start debugging
debugManager.startDebugging()

// Set breakpoints
debugManager.toggleBreakpoint(on: mathNode.id)
debugManager.setConditionalBreakpoint(on: variableNode.id, condition: "result > 10")

// Step through execution
debugManager.stepThrough()
debugManager.stepInto()

// Monitor variables
let variables = debugManager.localVariables
let result = debugManager.evaluateExpression("result")
```

### Using Quick Add
```swift
// Keyboard shortcut
let nodeType = NodeKeyboardShortcuts.shared.getNodeForKey("E") // Returns .event

// Fuzzy search
let results = FuzzySearch.search("math add", in: NodeType.allCases) { "\($0)" }
let bestMatch = results.first?.item // Returns .mathAdd
```

## Best Practices

### Node Design
- **Single responsibility**: Each node should do one thing well
- **Clear naming**: Use descriptive names for nodes and ports
- **Type safety**: Use specific port types for better validation
- **Documentation**: Provide descriptions and documentation for nodes

### Graph Organization
- **Logical grouping**: Group related nodes together
- **Comments**: Use comment nodes to explain complex logic
- **Subgraphs**: Extract complex logic into subgraphs
- **Naming**: Use meaningful names for variables and functions

### Performance
- **Avoid large graphs**: Break large graphs into smaller subgraphs
- **Minimize connections**: Reduce unnecessary connections
- **Use appropriate types**: Choose the most specific port types
- **Profile performance**: Use built-in profiling to identify bottlenecks

### Debugging
- **Strategic breakpoints**: Place breakpoints at key points
- **Use conditional breakpoints**: Break only on specific conditions
- **Monitor variables**: Watch important variables during execution
- **Use log points**: Log information without breaking execution

## Future Enhancements

### Planned Features
- **Collaborative editing**: Real-time collaboration
- **Version control**: Graph versioning and diffing
- **Plugin system**: Custom node plugins
- **Scripting**: Script-based node creation
- **Import/Export**: Support for external graph formats

### Performance Improvements
- **GPU acceleration**: Use Metal for rendering
- **Multithreading**: Parallel execution where possible
- **Incremental compilation**: Compile only changed parts
- **Smart caching**: Intelligent result caching

### UI Enhancements
- **Dark mode**: Complete dark mode support
- **Themes**: Customizable color themes
- **Accessibility**: Enhanced accessibility features
- **Touch support**: Better touch interaction

## Conclusion

The NodeGraphEditor provides a comprehensive, professional-grade visual scripting system with advanced debugging capabilities, extensive node type support, and a sophisticated user interface. The architecture is designed for scalability, performance, and maintainability, making it suitable for both simple visual scripting tasks and complex node-based programming workflows.

The system's modular design allows for easy extension and customization, while the comprehensive testing strategy ensures reliability and correctness. The debugging system provides professional-grade capabilities that rival traditional IDE debuggers, making the NodeGraphEditor a powerful tool for visual programming.
