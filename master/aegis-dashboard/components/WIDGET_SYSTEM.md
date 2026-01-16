# AEGIS Widget System Specification

## Overview

The AEGIS Widget System is a comprehensive, modular visualization framework designed to display any type of data with appropriate rendering. Each widget is a self-contained component that can be arranged, configured, and extended to create custom dashboard experiences.

## Widget Architecture

### Core Widget Protocol

```swift
public protocol Widget: View {
    associatedtype DataType: DataPoint
    var dataSource: any DataSource { get }
    var configuration: WidgetConfiguration { get }
}

// Base widget implementation
public struct BaseWidget<DataType: DataPoint>: Widget {
    @StateObject private var dataController: DataController<DataType>
    public let configuration: WidgetConfiguration
    
    public var body: some View {
        GlassCard(size: configuration.size) {
            content
                .onReceive(dataController.$latestValue) { _ in
                    handleDataUpdate()
                }
        }
    }
}
```

## Widget Types

### 1. Metric Widget

Displays single numerical values with trend indicators and sparklines.

```swift
public struct MetricWidget<DataType: Numeric & DataPoint>: Widget {
    public var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Text(configuration.title)
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                TrendIndicator(data: dataController.latestValue)
            }
            
            Spacer()
            
            HStack(alignment: .lastTextBaseline) {
                Text("\(dataController.latestValue?.value ?? 0)")
                    .font(.system(size: 48, weight: .bold, design: .rounded))
                    .monospacedDigit()
                    .contentTransition(.numericValue())
                
                if let unit = dataController.latestValue?.metadata?["unit"] as? String {
                    Text(unit)
                        .font(.title3)
                        .foregroundColor(.secondary)
                }
            }
            
            if configuration.visualization == .sparkline {
                SparklineView(
                    data: dataController.recentValues,
                    color: configuration.colorScheme.primary
                )
                .frame(height: 40)
            }
        }
        .padding(16)
    }
}
```

**Configuration Options:**
- `title`: Display title
- `value`: Current numerical value
- `unit`: Optional unit suffix
- `trend`: Optional trend indicator
- `status`: Visual status (success, warning, error, info)
- `sparkline`: Optional mini chart data
- `format`: Number formatting options

### 2. Chart Widget

Supports multiple chart types for data visualization.

```swift
public struct ChartWidget<DataType: DataPoint>: Widget {
    public var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Text(configuration.title)
                    .font(.headline)
                Spacer()
                ChartControls(view: configuration.chartType)
            }
            
            ChartView(
                data: dataController.values,
                type: configuration.chartType,
                colorScheme: configuration.colorScheme
            )
        }
        .padding(16)
    }
}

public enum ChartType {
    case line
    case bar
    case area
    case scatter
    case heatmap
    case network
}
```

**Chart Types:**
- **Line Chart**: Time-series data with smooth curves
- **Bar Chart**: Categorical comparisons
- **Area Chart**: Cumulative data over time
- **Scatter Plot**: Correlation analysis
- **Heatmap**: Density and pattern visualization
- **Network Graph**: Node-link relationship diagrams

### 3. Log Stream Widget

Real-time text feed for logs and events.

```swift
public struct LogStreamWidget: Widget {
    @State private var autoScroll = true
    
    public var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Text(configuration.title)
                    .font(.headline)
                Spacer()
                Toggle("Auto-scroll", isOn: $autoScroll)
                    .toggleStyle(.switch)
            }
            
            LogView(
                entries: dataController.values,
                autoScroll: autoScroll,
                filter: configuration.logFilter
            )
            .frame(maxHeight: .infinity)
        }
        .padding(16)
    }
}

public struct LogEntry: DataPoint {
    public let id: UUID
    public let timestamp: Date
    public let level: LogLevel
    public let message: String
    public let metadata: [String: Any]?
}
```

**Features:**
- Real-time log streaming
- Log level filtering
- Search and highlighting
- Auto-scroll toggle
- Export functionality

### 4. Gauge Widget

Circular progress indicators for metrics.

```swift
public struct GaugeWidget<DataType: Numeric & DataPoint>: Widget {
    public var body: some View {
        VStack {
            GaugeView(
                value: dataController.latestValue?.value ?? 0,
                minValue: configuration.minValue,
                maxValue: configuration.maxValue,
                color: configuration.colorScheme.primary
            )
            .frame(width: 120, height: 120)
            
            Text(configuration.title)
                .font(.caption)
                .foregroundColor(.secondary)
        }
        .padding(16)
    }
}
```

**Gauge Types:**
- **Circular**: Full circle progress
- **Semi-circular**: Half-circle display
- **Linear**: Horizontal/vertical bar
- **Radial**: Multi-axis radar chart

### 5. Status Indicator Widget

Visual system status displays.

```swift
public struct StatusIndicatorWidget: Widget {
    public var body: some View {
        HStack {
            StatusIndicator(
                status: dataController.latestValue?.status ?? .unknown,
                size: configuration.indicatorSize,
                pulse: configuration.pulseAnimation
            )
            
            VStack(alignment: .leading) {
                Text(configuration.title)
                    .font(.headline)
                Text(dataController.latestValue?.message ?? "Unknown")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            Spacer()
        }
        .padding(16)
    }
}

public enum Status {
    case running
    case stopped
    case warning
    case error
    case unknown
}
```

### 6. Network Map Widget

Node-link diagrams for network topology.

```swift
public struct NetworkMapWidget: Widget {
    @State private var selectedNode: NetworkNode?
    
    public var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Text(configuration.title)
                    .font(.headline)
                Spacer()
                NetworkControls()
            }
            
            NetworkView(
                nodes: dataController.nodes,
                edges: dataController.edges,
                selectedNode: $selectedNode,
                layout: configuration.networkLayout
            )
        }
        .padding(16)
    }
}

public struct NetworkNode: Identifiable {
    public let id: String
    public let label: String
    public let position: CGPoint
    public let status: Status
    public let metadata: [String: Any]?
}
```

**Network Layouts:**
- **Force-directed**: Physics-based node positioning
- **Hierarchical**: Tree-like structure layout
- **Circular**: Radial node arrangement
- **Grid**: Matrix-based positioning

### 7. Control Panel Widget

Interactive controls for system manipulation.

```swift
public struct ControlPanelWidget: Widget {
    @State private var isExecuting = false
    
    public var body: some View {
        VStack(alignment: .leading) {
            Text(configuration.title)
                .font(.headline)
            
            ForEach(configuration.controls) { control in
                ControlView(
                    control: control,
                    isEnabled: !isExecuting
                ) { action in
                    executeControl(action)
                }
            }
        }
        .padding(16)
    }
    
    private func executeControl(_ action: ControlAction) {
        isExecuting = true
        Task {
            await controlService.execute(action)
            isExecuting = false
        }
    }
}

public enum ControlType {
    case button
    case slider
    case toggle
    case dropdown
    case textInput
}
```

**Control Types:**
- **Button**: Single action triggers
- **Slider**: Range value selection
- **Toggle**: Binary state switches
- **Dropdown**: Multiple choice selection
- **Text Input**: Free-form text entry

### 8. 3D Visualization Widget

Advanced 3D data representations using Metal/WebGL.

```swift
public struct Visualization3DWidget: Widget {
    @State private var cameraPosition: Vector3 = .zero
    @State private var rotation: Rotation3D = .identity
    
    public var body: some View {
        VStack(alignment: .leading) {
            HStack {
                Text(configuration.title)
                    .font(.headline)
                Spacer()
                Visualization3DControls()
            }
            
            Visualization3DView(
                data: dataController.values,
                visualizationType: configuration.visualization3DType,
                cameraPosition: $cameraPosition,
                rotation: $rotation
            )
        }
        .padding(16)
    }
}

public enum Visualization3DType {
    case surfacePlot
    case scatterPlot
    case barChart
    case networkGraph
    case volumeRendering
}
```

**3D Visualization Types:**
- **Surface Plot**: 3D surface representations
- **Scatter Plot**: 3D point clouds
- **Bar Chart**: 3D bar graphs
- **Network Graph**: 3D node-link diagrams
- **Volume Rendering**: Volumetric data display

## Widget Configuration System

### Configuration Schema

```swift
public struct WidgetConfiguration {
    public let id: UUID
    public let title: String
    public let description: String?
    public let size: WidgetSize
    public let visualization: VisualizationType
    public let colorScheme: ColorScheme
    public let refreshRate: TimeInterval
    public let dataSource: DataSourceConfiguration
    public let style: WidgetStyle
    public let interactions: [InteractionType]
    public let permissions: WidgetPermissions
}

public struct WidgetStyle {
    public let background: BackgroundStyle
    public let border: BorderStyle
    public let shadow: ShadowStyle
    public let cornerRadius: CGFloat
    public let padding: EdgeInsets
}

public struct WidgetPermissions {
    public let canView: [UserRole]
    public let canEdit: [UserRole]
    public let canDelete: [UserRole]
}
```

### Configuration Management

```swift
public final class WidgetConfigurationManager {
    private var configurations: [UUID: WidgetConfiguration] = [:]
    
    public func saveConfiguration(_ configuration: WidgetConfiguration) {
        configurations[configuration.id] = configuration
        persistToDisk()
    }
    
    public func loadConfiguration(id: UUID) -> WidgetConfiguration? {
        return configurations[id]
    }
    
    public func exportConfiguration(id: UUID) -> Data? {
        guard let config = configurations[id] else { return nil }
        return try? JSONEncoder().encode(config)
    }
    
    public func importConfiguration(data: Data) -> WidgetConfiguration? {
        return try? JSONDecoder().decode(WidgetConfiguration.self, from: data)
    }
}
```

## Widget Layout System

### Bento Box Layout

```swift
public struct BentoBoxLayout {
    private let gridSize: CGSize
    private let cellSize: CGSize
    private let spacing: CGFloat
    
    public func layout(widgets: [WidgetConfiguration]) -> [WidgetPlacement] {
        var placements: [WidgetPlacement] = []
        var occupiedCells: Set<GridCell> = []
        
        for widget in widgets.sorted(by: { $0.priority > $1.priority }) {
            if let position = findOptimalPosition(
                for: widget,
                avoiding: occupiedCells
            ) {
                let placement = WidgetPlacement(
                    widget: widget,
                    position: position,
                    frame: calculateFrame(for: widget, at: position)
                )
                placements.append(placement)
                occupiedCells.formUnion(cellsOccupied(by: placement))
            }
        }
        
        return placements
    }
}
```

### Responsive Layout

```swift
public struct ResponsiveLayoutEngine {
    public func adaptLayout(for size: CGSize) -> LayoutStrategy {
        switch size.width {
        case 0...768:
            return .mobileStack
        case 768...1024:
            return .tabletGrid
        case 1024...1440:
            return .desktopGrid
        default:
            return .largeDesktopGrid
        }
    }
}
```

## Widget Interaction System

### Interaction Types

```swift
public enum InteractionType {
    case hover
    case click
    case doubleClick
    case drag
    case resize
    case contextMenu
    case keyboardShortcut
}

public struct WidgetInteraction {
    public let type: InteractionType
    public let handler: (InteractionContext) -> Void
    public let enabled: Bool
}
```

### Gesture Recognition

```swift
public final class WidgetGestureRecognizer {
    public func addGestureRecognition(to widget: Widget) -> some View {
        widget
            .onHover { isHovered in
                handleHover(isHovered)
            }
            .gesture(
                TapGesture(count: 1)
                    .onEnded { handleClick() }
            )
            .gesture(
                TapGesture(count: 2)
                    .onEnded { handleDoubleClick() }
            )
            .gesture(
                DragGesture()
                    .onChanged { value in
                        handleDrag(value)
                    }
            )
    }
}
```

## Widget Performance Optimization

### Rendering Optimization

```swift
public struct OptimizedWidget: View {
    @StateObject private var dataController: DataController
    
    public var body: some View {
        content
            .drawingGroup(opaque: false, colorMode: .extendedLinear)
            .allowsHitTesting(isInteractive)
            .onReceive(dataController.$values.throttle(for: .milliseconds(16), scheduler: RunLoop.main, latest: true)) { _ in
                // Throttled updates for 60fps
            }
    }
}
```

### Memory Management

```swift
public final class WidgetMemoryManager {
    private let cache = NSCache<NSString, AnyObject>()
    
    public func cacheWidget(_ widget: Widget, for key: String) {
        cache.setObject(widget as AnyObject, forKey: key as NSString)
    }
    
    public func getCachedWidget(for key: String) -> Widget? {
        return cache.object(forKey: key as NSString) as? Widget
    }
    
    public func clearCache() {
        cache.removeAllObjects()
    }
}
```

## Widget Testing Framework

### Unit Tests

```swift
class WidgetTests: XCTestCase {
    func testMetricWidgetRendering() {
        let widget = MetricWidget<TestDataPoint>(
            dataSource: mockDataSource,
            configuration: testConfiguration
        )
        
        let view = widget.body
        XCTAssertNotNil(view)
        // Additional rendering tests...
    }
    
    func testWidgetInteraction() {
        let expectation = XCTestExpectation(description: "Widget interaction")
        
        let widget = TestWidget { context in
            expectation.fulfill()
        }
        
        // Simulate interaction
        widget.simulateClick()
        
        wait(for: [expectation], timeout: 1.0)
    }
}
```

### Performance Tests

```swift
class WidgetPerformanceTests: XCTestCase {
    func testWidgetRenderingPerformance() {
        measure {
            let widget = MetricWidget<TestDataPoint>(
                dataSource: mockDataSource,
                configuration: testConfiguration
            )
            
            let view = widget.body
            _ = view // Use the view to force rendering
        }
    }
}
```

## Widget Extensibility

### Plugin Architecture

```swift
public protocol WidgetPlugin {
    var name: String { get }
    var version: String { get }
    var author: String { get }
    
    func createWidget(configuration: [String: Any]) -> AnyWidget
    func registerWidgetTypes()
}

public final class WidgetPluginManager {
    private var plugins: [String: WidgetPlugin] = [:]
    
    public func loadPlugin(_ plugin: WidgetPlugin) {
        plugins[plugin.name] = plugin
        plugin.registerWidgetTypes()
    }
    
    public func unloadPlugin(name: String) {
        plugins.removeValue(forKey: name)
    }
}
```

### Custom Widget Development

```swift
public struct CustomWidget: Widget {
    public let dataSource: any DataSource
    public let configuration: WidgetConfiguration
    
    public var body: some View {
        // Custom widget implementation
        Text("Custom Widget: \(configuration.title)")
            .padding()
            .background(Color.blue)
            .cornerRadius(8)
    }
}
```

This widget system provides a comprehensive foundation for building a flexible, performant, and extensible dashboard interface that can adapt to any data visualization requirement.