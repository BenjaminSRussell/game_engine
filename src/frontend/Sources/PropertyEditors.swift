import SwiftUI

// MARK: - Color Extensions for HDR Support
extension Color {
    var components: (red: Float, green: Float, blue: Float, alpha: Float) {
        #if canImport(UIKit)
        let uiColor = UIColor(self)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        uiColor.getRed(&r, green: &g, blue: &b, alpha: &a)
        return (Float(r), Float(g), Float(b), Float(a))
        #elseif canImport(AppKit)
        let nsColor = NSColor(self)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        nsColor.getRed(&r, green: &g, blue: &b, alpha: &a)
        return (Float(r), Float(g), Float(b), Float(a))
        #else
        return (0, 0, 0, 1)
        #endif
    }
    
    func toHSV() -> (hue: Float, saturation: Float, value: Float) {
        let r = components.red
        let g = components.green
        let b = components.blue
        
        let maxVal = max(r, g, b)
        let minVal = min(r, g, b)
        let delta = maxVal - minVal
        
        var hue: Float = 0
        var saturation: Float = maxVal == 0 ? 0 : delta / maxVal
        let value = maxVal
        
        if delta != 0 {
            if maxVal == r {
                hue = ((g - b) / delta).truncatingRemainder(dividingBy: 6) / 6
            } else if maxVal == g {
                hue = ((b - r) / delta + 2) / 6
            } else {
                hue = ((r - g) / delta + 4) / 6
            }
        }
        
        return (hue: hue, saturation: saturation, value: value)
    }
    
    func toHex() -> String {
        let r = Int(components.red * 255)
        let g = Int(components.green * 255)
        let b = Int(components.blue * 255)
        return String(format: "#%02X%02X%02X", r, g, b)
    }
}

// MARK: - SIMD Extensions for Quaternion Support
extension simd_quatf {
    var vector: SIMD3<Float> {
        return SIMD3<Float>(self.x, self.y, self.z)
    }
    
    var x: Float { self.imag.x }
    var y: Float { self.imag.y }
    var z: Float { self.imag.z }
    var w: Float { self.real }
}

extension SIMD3<Float> {
    static func + (left: SIMD3<Float>, right: SIMD3<Float>) -> SIMD3<Float> {
        return SIMD3<Float>(left.x + right.x, left.y + right.y, left.z + right.z)
    }
    
    static func - (left: SIMD3<Float>, right: SIMD3<Float>) -> SIMD3<Float> {
        return SIMD3<Float>(left.x - right.x, left.y - right.y, left.z - right.z)
    }
    
    static func * (left: SIMD3<Float>, right: Float) -> SIMD3<Float> {
        return SIMD3<Float>(left.x * right, left.y * right, left.z * right)
    }
    
    static func / (left: SIMD3<Float>, right: Float) -> SIMD3<Float> {
        return SIMD3<Float>(left.x / right, left.y / right, left.z / right)
    }
}

extension simd_quatf {
    init(vector: SIMD3<Float>, scalar: Float) {
        self.init(ix: vector.x, iy: vector.y, iz: vector.z, r: scalar)
    }
    
    var eulerAngles: (xyz: SIMD3<Float>, yzx: SIMD3<Float>, zxy: SIMD3<Float>) {
        let q = normalize(self)
        
        // XYZ Euler angles
        let xyz = SIMD3<Float>(
            atan2(2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y)),
            asin(2 * (q.w * q.y - q.z * q.x)),
            atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z))
        )
        
        // YZX Euler angles
        let yzx = SIMD3<Float>(
            atan2(2 * (q.w * q.x - q.y * q.z), 1 - 2 * (q.x * q.x + q.z * q.z)),
            atan2(2 * (q.w * q.y - q.x * q.z), 1 - 2 * (q.y * q.y + q.z * q.z)),
            asin(2 * (q.w * q.z + q.x * q.y))
        )
        
        // ZXY Euler angles
        let zxy = SIMD3<Float>(
            asin(2 * (q.w * q.x + q.y * q.z)),
            atan2(2 * (q.w * q.y - q.x * q.z), 1 - 2 * (q.x * q.x + q.z * q.z)),
            atan2(2 * (q.w * q.z - q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z))
        )
        
        return (xyz: xyz, yzx: yzx, zxy: zxy)
    }
}

// 
// MARK: - CATEGORY 3: INSPECTOR & PROPERTY SYSTEM (TODO-1461 to TODO-1800)
// 

// MARK: - Property Editor Base Protocol (TODO-1461)
protocol PropertyEditor: View {
    associatedtype Value
    var value: Binding<Value> { get }
    var label: String { get }
    
    // Optional configuration
    var defaultValue: Value? { get }
    var isLocked: Bool { get }
    var isOverridden: Bool { get }
    var tooltip: String? { get }
    var unit: String? { get }
    var propertyType: PropertyType { get }
    
    // Optional callbacks
    var onValueChanged: ((Value) -> Void)? { get }
    var onReset: (() -> Void)? { get }
    var onLockToggle: ((Bool) -> Void)? { get }
    var onAnimationKeyframe: (() -> Void)? { get }
    var onExpressionInput: (() -> Void)? { get }
    var onLinkProperty: (() -> Void)? { get }
}

// MARK: - Property Type System (TODO-1475)
enum PropertyType: String, CaseIterable {
    case float = "Float"
    case integer = "Integer"
    case boolean = "Boolean"
    case string = "String"
    case vector2 = "Vector2"
    case vector3 = "Vector3"
    case vector4 = "Vector4"
    case color = "Color"
    case enum = "Enum"
    case asset = "Asset"
    case array = "Array"
    case curve = "Curve"
    
    var color: Color {
        switch self {
        case .float: return .blue
        case .integer: return .green
        case .boolean: return .purple
        case .string: return .orange
        case .vector2, .vector3, .vector4: return .red
        case .color: return .pink
        case .enum: return .cyan
        case .asset: return .mint
        case .array: return .yellow
        case .curve: return .indigo
        }
    }
    
    var icon: String {
        switch self {
        case .float: return "number"
        case .integer: return "number.square"
        case .boolean: return "switch.2"
        case .string: return "textformat"
        case .vector2, .vector3, .vector4: return "arrow.up.and.down.text.horizontal"
        case .color: return "paintpalette"
        case .enum: return "list.bullet"
        case .asset: return "cube"
        case .array: return "list.bullet.rectangle"
        case .curve: return "waveform.path"
        }
    }
}

// MARK: - Property Editor Configuration
struct PropertyEditorConfig<Value> {
    let defaultValue: Value?
    let isLocked: Bool
    let isOverridden: Bool
    let tooltip: String?
    let unit: String?
    let propertyType: PropertyType
    let onValueChanged: ((Value) -> Void)?
    let onReset: (() -> Void)?
    let onLockToggle: ((Bool) -> Void)?
    let onAnimationKeyframe: (() -> Void)?
    let onExpressionInput: (() -> Void)?
    let onLinkProperty: (() -> Void)?
    
    init(
        defaultValue: Value? = nil,
        isLocked: Bool = false,
        isOverridden: Bool = false,
        tooltip: String? = nil,
        unit: String? = nil,
        propertyType: PropertyType,
        onValueChanged: ((Value) -> Void)? = nil,
        onReset: (() -> Void)? = nil,
        onLockToggle: ((Bool) -> Void)? = nil,
        onAnimationKeyframe: (() -> Void)? = nil,
        onExpressionInput: (() -> Void)? = nil,
        onLinkProperty: (() -> Void)? = nil
    ) {
        self.defaultValue = defaultValue
        self.isLocked = isLocked
        self.isOverridden = isOverridden
        self.tooltip = tooltip
        self.unit = unit
        self.propertyType = propertyType
        self.onValueChanged = onValueChanged
        self.onReset = onReset
        self.onLockToggle = onLockToggle
        self.onAnimationKeyframe = onAnimationKeyframe
        self.onExpressionInput = onExpressionInput
        self.onLinkProperty = onLinkProperty
    }
}

// MARK: - Undo/Redo Manager (TODO-1462)
class PropertyUndoManager: ObservableObject {
    @Published private var undoStack: [Any] = []
    @Published private var redoStack: [Any] = []
    private let maxStackSize = 50
    
    func canUndo() -> Bool {
        !undoStack.isEmpty
    }
    
    func canRedo() -> Bool {
        !redoStack.isEmpty
    }
    
    func recordState<T>(_ state: T) {
        undoStack.append(state)
        if undoStack.count > maxStackSize {
            undoStack.removeFirst()
        }
        redoStack.removeAll()
    }
    
    func undo<T>() -> T? {
        guard let state = undoStack.popLast() as? T else { return nil }
        redoStack.append(state)
        return state
    }
    
    func redo<T>() -> T? {
        guard let state = redoStack.popLast() as? T else { return nil }
        undoStack.append(state)
        return state
    }
    
    func clear() {
        undoStack.removeAll()
        redoStack.removeAll()
    }
}

// MARK: - Copy/Paste Manager (TODO-1463)
class PropertyClipboardManager: ObservableObject {
    @Published private var clipboard: Any?
    @Published private var clipboardType: String?
    
    func copy<T>(_ value: T, type: String) {
        clipboard = value
        clipboardType = type
    }
    
    func paste<T>(as type: T.Type) -> T? {
        guard let clipboard = clipboard as? T,
              clipboardType == String(describing: type) else {
            return nil
        }
        return clipboard
    }
    
    func canPaste(as type: String) -> Bool {
        return clipboardType == type
    }
    
    func clear() {
        clipboard = nil
        clipboardType = nil
    }
}

// MARK: - Float Property Editor TODOs (TODO-1501 to TODO-1520)
// TODO-1501: Float editor with logarithmic scale option
// TODO-1502: Float editor with custom curve mapping
// TODO-1503: Float editor with percentage display mode
// TODO-1504: Float editor with scientific notation
// TODO-1505: Float editor with infinity/NaN handling
// TODO-1506: Float editor with visual power indicator
// TODO-1507: Float editor with audio-style dB display
// TODO-1508: Float editor with time format (hh:mm:ss)
// TODO-1509: Float editor with angle format (degrees/radians)
// TODO-1510: Float editor with distance format (m/km/mi)

// MARK: - Vector Property Editor TODOs (TODO-1521 to TODO-1540)
// TODO-1521: Vector2 property editor
// TODO-1522: Vector4 property editor
// TODO-1523: Vector editor with swizzle operations
// TODO-1524: Vector editor with normalize button
// TODO-1525: Vector editor with length display
// TODO-1526: Vector editor with direction visualization
// TODO-1527: Vector editor with world/local toggle
// TODO-1528: Vector editor with snap to axis
// TODO-1529: Vector editor with copy/paste individual axes
// TODO-1530: Vector editor with proportional scaling lock

// MARK: - Rotation Property Editor TODOs (TODO-1541 to TODO-1560)
// TODO-1541: Euler angles editor with gimbal lock warning
// TODO-1542: Quaternion editor with visualization
// TODO-1543: Rotation editor with axis-angle mode
// TODO-1544: Rotation editor with look-at helper
// TODO-1545: Rotation editor with snap to world axes
// TODO-1546: Rotation editor with flip/mirror operations
// TODO-1547: Rotation editor with rotation constraints
// TODO-1548: Rotation editor with degrees/radians toggle
// TODO-1549: Rotation editor with animation curve integration
// TODO-1550: Rotation editor with slerp/lerp preview

// MARK: - Color Property Editor TODOs (TODO-1561 to TODO-1600)
// TODO-1561: Color editor with HDR support
// TODO-1562: Color editor with color space selection (sRGB, Linear, HSV)
// TODO-1563: Color editor with hex input
// TODO-1564: Color editor with eyedropper tool
// TODO-1565: Color editor with swatches/palette
// TODO-1566: Color editor with gradient mode
// TODO-1567: Color editor with alpha channel toggle
// TODO-1568: Color editor with color harmony suggestions
// TODO-1569: Color editor with accessibility contrast checker
// TODO-1570: Color editor with color blind simulation
// TODO-1571: Color editor with temperature/tint mode
// TODO-1572: Color editor with exposure adjustment
// TODO-1573: Color editor with color history
// TODO-1574: Color editor with color naming
// TODO-1575: Color editor with batch color operations

// MARK: - Reference Property Editor TODOs (TODO-1601 to TODO-1650)
// TODO-1601: Object reference picker with search
// TODO-1602: Object reference with drag-and-drop
// TODO-1603: Object reference with scene hierarchy browser
// TODO-1604: Object reference with type filtering
// TODO-1605: Object reference with null check warning
// TODO-1606: Object reference with missing reference detection
// TODO-1607: Object reference with circular reference prevention
// TODO-1608: Object reference with prefab instance highlighting
// TODO-1609: Asset reference with preview thumbnail
// TODO-1610: Asset reference with inline creation
// TODO-1611: Asset reference with variant selection
// TODO-1612: Component reference picker
// TODO-1613: Script reference picker
// TODO-1614: Layer reference picker
// TODO-1615: Tag reference picker
// TODO-1616: Animation state reference picker
// TODO-1617: Audio clip reference picker
// TODO-1618: Shader reference picker
// TODO-1619: Render texture reference picker
// TODO-1620: Scene reference picker

// MARK: - Array/Collection Editor TODOs (TODO-1651 to TODO-1700)
// TODO-1651: Array editor with drag reordering
// TODO-1652: Array editor with multi-select operations
// TODO-1653: Array editor with duplicate item
// TODO-1654: Array editor with move to top/bottom
// TODO-1655: Array editor with collapse/expand all
// TODO-1656: Array editor with search filter
// TODO-1657: Array editor with sort options
// TODO-1658: Array editor with reverse order
// TODO-1659: Array editor with clear all confirmation
// TODO-1660: Array editor with import from file
// TODO-1661: Array editor with export to file
// TODO-1662: Array editor with pagination for large lists
// TODO-1663: Array editor with virtualized scrolling
// TODO-1664: Dictionary editor with key-value pairs
// TODO-1665: Set editor with unique value enforcement
// TODO-1666: Queue/Stack editor visualization
// TODO-1667: Nested collection editors
// TODO-1668: Collection size limits and warnings
// TODO-1669: Collection memory usage display
// TODO-1670: Collection performance analysis

// MARK: - Curve Editor TODOs (TODO-1701 to TODO-1750)
// TODO-1701: Bezier curve with tangent handles
// TODO-1702: Curve presets (ease in/out, linear, etc.)
// TODO-1703: Curve mirroring/flipping
// TODO-1704: Curve normalization
// TODO-1705: Multi-curve overlay comparison
// TODO-1706: Curve key snapping
// TODO-1707: Curve loop/ping-pong modes
// TODO-1708: Curve derivative visualization
// TODO-1709: Curve area integration
// TODO-1710: Curve time remapping
// TODO-1711: Curve value range clamping
// TODO-1712: Curve smoothing algorithms
// TODO-1713: Curve simplification
// TODO-1714: Curve from expression/formula
// TODO-1715: Curve import from external format

// MARK: - Specialized Editors TODOs (TODO-1751 to TODO-1800)
// TODO-1751: Layer mask editor with visual checkboxes
// TODO-1752: Rendering layer mask editor
// TODO-1753: Physics layer mask matrix
// TODO-1754: Bounds editor (AABB, OBB)
// TODO-1755: Range editor (min-max pair)
// TODO-1756: Rect/RectTransform editor
// TODO-1757: Matrix3x3 editor
// TODO-1758: Matrix4x4 editor
// TODO-1759: Plane editor (normal + distance)
// TODO-1760: Ray editor (origin + direction)
// TODO-1761: Sphere editor (center + radius)
// TODO-1762: Box editor (center + size)
// TODO-1763: Capsule editor (start + end + radius)
// TODO-1764: Cylinder editor (center + radius + height)
// TODO-1765: Cone editor (apex + direction + angle)
// TODO-1766: Frustum editor
// TODO-1767: Spline editor with control points
// TODO-1768: Path editor with waypoints
// TODO-1769: Polygon/shape editor
// TODO-1770: Mesh preview/editor
// TODO-1771: Texture preview with channels
// TODO-1772: Audio waveform preview
// TODO-1773: Animation preview player
// TODO-1774: Script syntax highlighted editor
// TODO-1775: JSON/data structure editor
// TODO-1776: File path picker with validation
// TODO-1777: URL input with validation
// TODO-1778: Date/time picker
// TODO-1779: Duration editor
// TODO-1780: Localized string editor

// MARK: - Float Editor with Angle Format (TODO-1509)
struct AngleFloatPropertyEditor: View {
    let label: String
    @Binding var value: Float
    var range: ClosedRange<Float> = -Float.pi...Float.pi
    var step: Float = 0.01
    @State private var useDegrees: Bool = true
    @State private var isDragging = false
    @State private var dragStartValue: Float = 0
    
    private var degreesValue: Float {
        get { value * 180 / .pi }
        set { value = newValue * .pi / 180 }
    }
    
    private var displayValue: Float {
        useDegrees ? degreesValue : value
    }
    
    private var unit: String {
        useDegrees ? "°" : "rad"
    }
    
    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            HStack(spacing: 4) {
                // Draggable value field
                Text(String(format: "%.2f", displayValue) + unit)
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(isDragging ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                    .frame(width: 80)
                    .gesture(
                        DragGesture()
                            .onChanged { gesture in
                                if !isDragging {
                                    isDragging = true
                                    dragStartValue = displayValue
                                }
                                let delta = Float(gesture.translation.width) * step * 0.1
                                let newValue = dragStartValue + delta
                                if useDegrees {
                                    degreesValue = min(max(newValue, range.lowerBound * 180 / .pi), range.upperBound * 180 / .pi)
                                } else {
                                    value = min(max(newValue, range.lowerBound), range.upperBound)
                                }
                            }
                            .onEnded { _ in
                                isDragging = false
                            }
                    )
                
                // Slider
                Slider(value: useDegrees ? $degreesValue : $value, in: useDegrees ? range.lowerBound * 180 / .pi...range.upperBound * 180 / .pi : range, step: step)
                    .frame(maxWidth: .infinity)
                
                // Unit toggle button
                Button(action: { useDegrees.toggle() }) {
                    Text(useDegrees ? "°" : "rad")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 6)
                .padding(.vertical, 2)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(3)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
        }
    }
}

// MARK: - Float Editor with Distance Format (TODO-1510)
struct DistanceFloatPropertyEditor: View {
    let label: String
    @Binding var value: Float // Value in meters
    var range: ClosedRange<Float> = 0...1000
    var step: Float = 0.1
    @State private var unit: DistanceUnit = .meters
    @State private var isDragging = false
    @State private var dragStartValue: Float = 0
    
    enum DistanceUnit: String, CaseIterable {
        case meters = "m"
        case kilometers = "km"
        case miles = "mi"
        case centimeters = "cm"
        case feet = "ft"
        case inches = "in"
        
        var conversionFactor: Float {
            switch self {
            case .meters: return 1.0
            case .kilometers: return 0.001
            case .miles: return 0.000621371
            case .centimeters: return 100.0
            case .feet: return 3.28084
            case .inches: return 39.3701
            }
        }
        
        var displayPrecision: Int {
            switch self {
            case .kilometers, .miles: return 3
            case .meters, .feet: return 2
            case .centimeters, .inches: return 1
            }
        }
    }
    
    private var displayValue: Float {
        value * unit.conversionFactor
    }
    
    private var adjustedRange: ClosedRange<Float> {
        range.lowerBound * unit.conversionFactor...range.upperBound * unit.conversionFactor
    }
    
    private var adjustedStep: Float {
        step * unit.conversionFactor
    }
    
    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            HStack(spacing: 4) {
                // Draggable value field
                Text(String(format: "%.\(unit.displayPrecision)f", displayValue) + unit.rawValue)
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(isDragging ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                    .frame(width: 100)
                    .gesture(
                        DragGesture()
                            .onChanged { gesture in
                                if !isDragging {
                                    isDragging = true
                                    dragStartValue = displayValue
                                }
                                let delta = Float(gesture.translation.width) * adjustedStep * 0.1
                                let newDisplayValue = dragStartValue + delta
                                let clampedValue = min(max(newDisplayValue, adjustedRange.lowerBound), adjustedRange.upperBound)
                                value = clampedValue / unit.conversionFactor
                            }
                            .onEnded { _ in
                                isDragging = false
                            }
                    )
                
                // Slider
                Slider(
                    value: Binding(
                        get: { displayValue },
                        set: { value = $0 / unit.conversionFactor }
                    ),
                    in: adjustedRange,
                    step: adjustedStep
                )
                .frame(maxWidth: .infinity)
                
                // Unit picker
                Picker("", selection: $unit) {
                    ForEach(DistanceUnit.allCases, id: \.self) { unit in
                        Text(unit.rawValue).tag(unit)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 50)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
        }
    }
}

// MARK: - Vector2 Property Editor (TODO-1521)
struct Vector2PropertyEditor: View {
    let label: String
    @Binding var value: SIMD2<Float>
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: 4) {
                AxisField(axis: "X", value: $value.x, color: DesignSystem.Colors.xAxis)
                AxisField(axis: "Y", value: $value.y, color: DesignSystem.Colors.yAxis)
            }
        }
    }
}

// MARK: - Vector4 Property Editor (TODO-1522)
struct Vector4PropertyEditor: View {
    let label: String
    @Binding var value: SIMD4<Float>
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: 2) {
                AxisField(axis: "X", value: $value.x, color: DesignSystem.Colors.xAxis)
                AxisField(axis: "Y", value: $value.y, color: DesignSystem.Colors.yAxis)
                AxisField(axis: "Z", value: $value.z, color: DesignSystem.Colors.zAxis)
                AxisField(axis: "W", value: $value.w, color: DesignSystem.Colors.wAxis)
            }
        }
    }
}

// MARK: - Enhanced Vector3 Editor with Features (TODO-1523 to TODO-1530)
struct EnhancedVector3PropertyEditor: View {
    let label: String
    @Binding var value: SIMD3<Float>
    @State private var showSwizzleMenu = false
    @State private var coordinateSpace: CoordinateSpace = .world
    @State private var lockedAxes: Set<Int> = [] // For proportional scaling
    @State private var copiedAxis: Float? = nil
    @State private var copiedAxisIndex: Int? = nil
    
    enum CoordinateSpace: String, CaseIterable {
        case world = "World"
        case local = "Local"
    }
    
    enum SwizzlePattern: String, CaseIterable {
        case xyz = "XYZ"
        case xzy = "XZY"
        case yxz = "YXZ"
        case yzx = "YZX"
        case zxy = "ZXY"
        case zyx = "ZYX"
        case xxx = "XXX"
        case yyy = "YYY"
        case zzz = "ZZZ"
    }
    
    private var length: Float {
        simd_length(value)
    }
    
    private var normalizedValue: SIMD3<Float> {
        simd_normalize(value)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                // Length display
                Text("Length: \(String(format: "%.3f", length))")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Vector fields with controls
            HStack(spacing: 4) {
                ForEach(0..<3, id: \.self) { index in
                    HStack(spacing: 2) {
                        Text(["X", "Y", "Z"][index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                            .frame(width: 14)
                        
                        TextField("", value: bindingForAxis(index), format: .number.precision(.fractionLength(3)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(minWidth: 60)
                            .contextMenu {
                                Button("Copy Value") {
                                    copiedAxis = value[index]
                                    copiedAxisIndex = index
                                }
                                if copiedAxis != nil && copiedAxisIndex == index {
                                    Button("Paste Value") {
                                        value[index] = copiedAxis!
                                    }
                                }
                                Button("Reset to 0") {
                                    value[index] = 0
                                }
                                Button("Set to 1") {
                                    value[index] = 1
                                }
                                Button("Snap to 0") {
                                    value[index] = round(value[index])
                                }
                            }
                    }
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(lockedAxes.contains(index) ? DesignSystem.Colors.accentPrimary : Color.clear, lineWidth: 2)
                    )
                }
                
                // Normalize button
                Button(action: { value = normalizedValue }) {
                    Image(systemName: "arrow.up.to.line")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 6)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                .help("Normalize Vector")
                
                // Swizzle menu
                Menu {
                    ForEach(SwizzlePattern.allCases, id: \.self) { pattern in
                        Button(pattern.rawValue) {
                            applySwizzle(pattern)
                        }
                    }
                } label: {
                    Image(systemName: "arrow.2.squarepath")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 6)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                .help("Swizzle Components")
                
                // Coordinate space toggle
                Picker("", selection: $coordinateSpace) {
                    ForEach(CoordinateSpace.allCases, id: \.self) { space in
                        Text(space.rawValue).tag(space)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 70)
            }
            
            // Direction visualization
            HStack {
                Text("Direction:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                // Simple direction arrow visualization
                GeometryReader { geometry in
                    ZStack {
                        Circle()
                            .stroke(DesignSystem.Colors.border, lineWidth: 1)
                            .frame(width: 40, height: 40)
                        
                        if length > 0.01 {
                            Path { path in
                                let center = CGPoint(x: 20, y: 20)
                                let direction = normalizedValue
                                let endPoint = CGPoint(
                                    x: center.x + CGFloat(direction.x) * 15,
                                    y: center.y - CGFloat(direction.y) * 15
                                )
                                path.move(to: center)
                                path.addLine(to: endPoint)
                            }
                            .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                        }
                    }
                }
                .frame(width: 40, height: 40)
                
                Spacer()
                
                // Proportional scaling lock
                HStack(spacing: 2) {
                    ForEach(0..<3, id: \.self) { index in
                        Button(action: {
                            if lockedAxes.contains(index) {
                                lockedAxes.remove(index)
                            } else {
                                lockedAxes.insert(index)
                            }
                        }) {
                            Image(systemName: lockedAxes.contains(index) ? "lock.fill" : "lock.open")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(lockedAxes.contains(index) ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
                        }
                        .buttonStyle(.plain)
                        .frame(width: 20, height: 20)
                    }
                }
            }
        }
        .onChange(of: lockedAxes) { _ in
            // Apply proportional scaling when axes are locked
            if !lockedAxes.isEmpty {
                applyProportionalScaling()
            }
        }
    }
    
    private func bindingForAxis(_ index: Int) -> Binding<Float> {
        Binding<Float>(
            get: { value[index] },
            set: { newValue in
                value[index] = newValue
                if !lockedAxes.isEmpty && lockedAxes.contains(index) {
                    applyProportionalScaling()
                }
            }
        )
    }
    
    private func applyProportionalScaling() {
        guard !lockedAxes.isEmpty else { return }
        
        let referenceAxis = lockedAxes.first!
        let referenceValue = value[referenceAxis]
        
        for i in 0..<3 {
            if i != referenceAxis && lockedAxes.contains(i) {
                // Calculate ratio based on original values or use 1:1 ratio
                value[i] = referenceValue
            }
        }
    }
    
    private func applySwizzle(_ pattern: SwizzlePattern) {
        let components = [value.x, value.y, value.z]
        
        switch pattern {
        case .xyz: break // No change
        case .xzy: value = SIMD3<Float>(components[0], components[2], components[1])
        case .yxz: value = SIMD3<Float>(components[1], components[0], components[2])
        case .yzx: value = SIMD3<Float>(components[1], components[2], components[0])
        case .zxy: value = SIMD3<Float>(components[2], components[0], components[1])
        case .zyx: value = SIMD3<Float>(components[2], components[1], components[0])
        case .xxx: value = SIMD3<Float>(components[0], components[0], components[0])
        case .yyy: value = SIMD3<Float>(components[1], components[1], components[1])
        case .zzz: value = SIMD3<Float>(components[2], components[2], components[2])
        }
    }
}

// MARK: - Euler Angles Editor with Gimbal Lock Warning (TODO-1541)
struct EulerAnglesPropertyEditor: View {
    let label: String
    @Binding var value: SIMD3<Float> // Pitch, Yaw, Roll in radians
    @State private var useDegrees: Bool = true
    @State private var rotationOrder: RotationOrder = .xyz
    @State private var showGimbalLockWarning: Bool = false
    
    enum RotationOrder: String, CaseIterable {
        case xyz = "XYZ"
        case xzy = "XZY"
        case yxz = "YXZ"
        case yzx = "YZX"
        case zxy = "ZXY"
        case zyx = "ZYX"
        
        var gimbalLockAngles: SIMD3<Float> {
            switch self {
            case .xyz: return SIMD3<Float>(Float.pi/2, 0, 0)
            case .xzy: return SIMD3<Float>(Float.pi/2, 0, 0)
            case .yxz: return SIMD3<Float>(0, Float.pi/2, 0)
            case .yzx: return SIMD3<Float>(0, Float.pi/2, 0)
            case .zxy: return SIMD3<Float>(0, 0, Float.pi/2)
            case .zyx: return SIMD3<Float>(0, 0, Float.pi/2)
            }
        }
    }
    
    private var degreesValue: SIMD3<Float> {
        get { value * 180 / .pi }
        set { value = newValue * .pi / 180 }
    }
    
    private var displayValue: SIMD3<Float> {
        useDegrees ? degreesValue : value
    }
    
    private var unit: String {
        useDegrees ? "°" : "rad"
    }
    
    private var isNearGimbalLock: Bool {
        let threshold: Float = 0.1 // ~5.7 degrees
        let gimbalAngles = rotationOrder.gimbalLockAngles
        let diff = simd_abs(value - gimbalAngles)
        let diff2 = simd_abs(value + gimbalAngles)
        return simd_any(diff < threshold) || simd_any(diff2 < threshold)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                // Gimbal lock warning
                if showGimbalLockWarning && isNearGimbalLock {
                    HStack(spacing: 4) {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(DesignSystem.Colors.accentWarning)
                        Text("Gimbal Lock")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.accentWarning)
                    }
                }
            }
            
            // Rotation controls
            HStack(spacing: 4) {
                ForEach(0..<3, id: \.self) { index in
                    HStack(spacing: 2) {
                        Text(["P", "Y", "R"][index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                            .frame(width: 14)
                            .help(["Pitch", "Yaw", "Roll"][index])
                        
                        TextField("", value: bindingForAxis(index), format: .number.precision(.fractionLength(useDegrees ? 1 : 3)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(minWidth: 60)
                    }
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
                
                Text(unit)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .frame(width: 20)
                
                // Degrees/Radians toggle
                Button(action: { useDegrees.toggle() }) {
                    Text(useDegrees ? "°" : "rad")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 6)
                .padding(.vertical, 2)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(3)
                
                // Rotation order picker
                Picker("", selection: $rotationOrder) {
                    ForEach(RotationOrder.allCases, id: \.self) { order in
                        Text(order.rawValue).tag(order)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 50)
            }
            
            // Gimbal lock warning toggle
            HStack {
                Toggle("Show Gimbal Lock Warning", isOn: $showGimbalLockWarning)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Spacer()
                
                if isNearGimbalLock {
                    Text("Near gimbal lock: \(String(format: "%.1f", abs(value.x - rotationOrder.gimbalLockAngles.x) * 180 / .pi))°")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentWarning)
                }
            }
        }
    }
    
    private func bindingForAxis(_ index: Int) -> Binding<Float> {
        Binding<Float>(
            get: { displayValue[index] },
            set: { newValue in
                if useDegrees {
                    degreesValue[index] = newValue
                } else {
                    value[index] = newValue
                }
            }
        )
    }
}

// MARK: - Quaternion Editor with Visualization (TODO-1542)
struct QuaternionPropertyEditor: View {
    let label: String
    @Binding var value: simd_quatf
    @State private var showVisualization: Bool = true
    @State private var eulerMode: EulerMode = .xyz
    
    enum EulerMode: String, CaseIterable {
        case xyz = "XYZ"
        case yzx = "YZX"
        case zxy = "ZXY"
    }
    
    private var eulerAngles: SIMD3<Float> {
        switch eulerMode {
        case .xyz: return value.eulerAngles.xyz
        case .yzx: return value.eulerAngles.yzx
        case .zxy: return value.eulerAngles.zxy
        }
    }
    
    private var axis: SIMD3<Float> {
        let q = normalize(value)
        guard q.w < 0.999 else { return SIMD3<Float>(1, 0, 0) }
        let s = sqrt(1 - q.w * q.w)
        return s < 0.001 ? SIMD3<Float>(0, 1, 0) : q.vector / s
    }
    
    private var angle: Float {
        let q = normalize(value)
        return 2 * acos(abs(q.w))
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            // Quaternion components
            HStack(spacing: 4) {
                Text("W:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.wAxis)
                    .frame(width: 20)
                
                TextField("", value: $value.w, format: .number.precision(.fractionLength(3)))
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(minWidth: 60)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                
                ForEach(0..<3, id: \.self) { index in
                    HStack(spacing: 2) {
                        Text(["X", "Y", "Z"][index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                            .frame(width: 14)
                        
                        TextField("", value: bindingForAxis(index), format: .number.precision(.fractionLength(3)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(minWidth: 60)
                    }
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
            }
            
            // Visualization and controls
            HStack {
                // Normalize button
                Button(action: { value = normalize(value) }) {
                    HStack {
                        Image(systemName: "arrow.up.to.line")
                            .font(DesignSystem.Typography.small)
                        Text("Normalize")
                            .font(DesignSystem.Typography.small)
                    }
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                
                // Identity button
                Button(action: { value = simd_quatf(identity: ()) }) {
                    HStack {
                        Image(systemName: "arrow.counterclockwise")
                            .font(DesignSystem.Typography.small)
                        Text("Identity")
                            .font(DesignSystem.Typography.small)
                    }
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                
                Spacer()
                
                // Visualization toggle
                Toggle("Visualize", isOn: $showVisualization)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Axis-angle display
            HStack {
                Text("Axis-Angle:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(String(format: "X:%.2f Y:%.2f Z:%.2f ∠%.1f°", axis.x, axis.y, axis.z, angle * 180 / .pi))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
            }
            
            // Euler angles display
            HStack {
                Text("Euler (\(eulerMode.rawValue)):")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(String(format: "P:%.1f° Y:%.1f° R:%.1f°", 
                          eulerAngles.x * 180 / .pi,
                          eulerAngles.y * 180 / .pi,
                          eulerAngles.z * 180 / .pi))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Euler mode picker
                Picker("", selection: $eulerMode) {
                    ForEach(EulerMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 50)
            }
            
            // 3D visualization
            if showVisualization {
                QuaternionVisualization(quaternion: value)
                    .frame(height: 120)
                    .padding(.top, 8)
            }
        }
    }
    
    private func bindingForAxis(_ index: Int) -> Binding<Float> {
        Binding<Float>(
            get: { value.vector[index] },
            set: { newValue in
                var newVector = value.vector
                newVector[index] = newValue
                value = simd_quatf(vector: newVector, scalar: value.w)
                value = normalize(value) // Keep quaternion normalized
            }
        )
    }
}

// MARK: - Quaternion 3D Visualization
private struct QuaternionVisualization: View {
    let quaternion: simd_quatf
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background grid
                Path { path in
                    let size = geometry.size
                    let centerX = size.width / 2
                    let centerY = size.height / 2
                    let radius = min(size.width, size.height) / 3
                    
                    // Draw circle
                    path.addEllipse(in: CGRect(x: centerX - radius, y: centerY - radius, width: radius * 2, height: radius * 2))
                    
                    // Draw axes
                    path.move(to: CGPoint(x: centerX - radius, y: centerY))
                    path.addLine(to: CGPoint(x: centerX + radius, y: centerY))
                    path.move(to: CGPoint(x: centerX, y: centerY - radius))
                    path.addLine(to: CGPoint(x: centerX, y: centerY + radius))
                }
                .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 1)
                
                // Rotation visualization
                let size = geometry.size
                let centerX = size.width / 2
                let centerY = size.height / 2
                let radius = min(size.width, size.height) / 4
                
                // Draw rotation axis
                let axis = normalize(quaternion.vector)
                let axisEnd = CGPoint(
                    x: centerX + CGFloat(axis.x) * radius,
                    y: centerY - CGFloat(axis.y) * radius
                )
                
                Path { path in
                    path.move(to: CGPoint(x: centerX, y: centerY))
                    path.addLine(to: axisEnd)
                }
                .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 3)
                
                // Draw rotation arc
                let angle = 2 * acos(abs(quaternion.w))
                let arcRadius = radius * 0.8
                
                Path { path in
                    path.addArc(center: CGPoint(x: centerX, y: centerY),
                               radius: arcRadius,
                               startAngle: .degrees(0),
                               endAngle: .degrees(angle * 180 / .pi),
                               clockwise: false)
                }
                .stroke(DesignSystem.Colors.accentSecondary, lineWidth: 2)
                
                // Center point
                Circle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 8, height: 8)
                    .position(x: centerX, y: centerY)
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
}

// MARK: - Enhanced Float Editor (TODO-1461 to TODO-1475)
struct FloatPropertyEditor: View, PropertyEditor {
    let label: String
    @Binding var value: Float
    var range: ClosedRange<Float> = 0...100
    var step: Float = 0.1
    var precision: Int = 2
    
    // Configuration
    let config: PropertyEditorConfig<Float>
    
    // State
    @State private var isDragging = false
    @State private var dragStartValue: Float = 0
    @State private var showWarning = false
    @State private var showTooltip = false
    @State private var showExpressionDialog = false
    @State private var expressionInput = ""
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    // Protocol conformance
    var defaultValue: Float? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Float) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    init(
        label: String,
        value: Binding<Float>,
        range: ClosedRange<Float> = 0...100,
        step: Float = 0.1,
        precision: Int = 2,
        config: PropertyEditorConfig<Float> = PropertyEditorConfig<Float>(propertyType: .float)
    ) {
        self.label = label
        self._value = value
        self.range = range
        self.step = step
        self.precision = precision
        self.config = config
    }
    
    var body: some View {
        HStack(spacing: 8) {
            // Property type indicator and label
            HStack(spacing: 4) {
                Image(systemName: propertyType.icon)
                    .foregroundColor(propertyType.color)
                    .font(.caption)
                
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isOverridden ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    .frame(width: 80, alignment: .leading)
                
                if isOverridden {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(.orange)
                        .font(.caption)
                        .help("Property overridden")
                }
            }
            
            // Main editor
            HStack(spacing: 4) {
                // Draggable value field
                HStack(spacing: 2) {
                    Text(String(format: "%.\(precision)f", value))
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(isDragging ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                        .frame(width: 60)
                    
                    if let unit = unit {
                        Text(unit)
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
                .opacity(isLocked ? 0.6 : 1.0)
                .gesture(
                    DragGesture()
                        .onChanged { gesture in
                            guard !isLocked else { return }
                            
                            if !isDragging {
                                isDragging = true
                                dragStartValue = value
                                undoManager.recordState(value)
                            }
                            let delta = Float(gesture.translation.width) * step * 0.1
                            let newValue = min(max(dragStartValue + delta, range.lowerBound), range.upperBound)
                            
                            // Check for clamping warning
                            showWarning = (newValue <= range.lowerBound || newValue >= range.upperBound)
                            
                            value = newValue
                            onValueChanged?(newValue)
                        }
                        .onEnded { _ in
                            isDragging = false
                            showWarning = false
                        }
                )
                
                // Slider
                Slider(value: $value, in: range, step: step)
                    .frame(maxWidth: .infinity)
                    .disabled(isLocked)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(DesignSystem.Colors.backgroundPrimary)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(showWarning ? Color.orange : DesignSystem.Colors.border, lineWidth: showWarning ? 2 : 1)
                    )
            )
            
            // Action buttons
            HStack(spacing: 2) {
                // Lock/Unlock button
                Button(action: { onLockToggle?(!isLocked) }) {
                    Image(systemName: isLocked ? "lock.fill" : "lock.open")
                        .foregroundColor(isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help(isLocked ? "Unlock property" : "Lock property")
                
                // Reset to default button
                if let defaultValue = defaultValue {
                    Button(action: {
                        undoManager.recordState(value)
                        value = defaultValue
                        onValueChanged?(defaultValue)
                        onReset?()
                    }) {
                        Image(systemName: "arrow.counterclockwise")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("Reset to default")
                }
                
                // Animation keyframe button
                Button(action: { onAnimationKeyframe?() }) {
                    Image(systemName: "key")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Add animation keyframe")
                
                // Expression input button
                Button(action: { showExpressionDialog = true }) {
                    Image(systemName: "function")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Expression input")
                
                // Link property button
                Button(action: { onLinkProperty?() }) {
                    Image(systemName: "link")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Link property")
                
                // Copy/Paste buttons
                Menu {
                    Button("Copy Value") {
                        clipboardManager.copy(value, type: "Float")
                    }
                    
                    if clipboardManager.canPaste(as: "Float") {
                        Button("Paste Value") {
                            if let pastedValue: Float = clipboardManager.paste(as: Float.self) {
                                undoManager.recordState(value)
                                value = pastedValue
                                onValueChanged?(pastedValue)
                            }
                        }
                    }
                    
                    Button("Undo") {
                        if let previousValue: Float = undoManager.undo() {
                            value = previousValue
                            onValueChanged?(previousValue)
                        }
                    }
                    .disabled(!undoManager.canUndo())
                    
                    Button("Redo") {
                        if let nextValue: Float = undoManager.redo() {
                            value = nextValue
                            onValueChanged?(nextValue)
                        }
                    }
                    .disabled(!undoManager.canRedo())
                } label: {
                    Image(systemName: "ellipsis.circle")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("More options")
            }
        }
        .onHover { isHovering in
            showTooltip = isHovering
        }
        .popover(isPresented: showTooltip && tooltip != nil) {
            if let tooltip = tooltip {
                Text(tooltip)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(6)
                    .font(DesignSystem.Typography.caption)
            }
        }
        .alert("Expression Input", isPresented: $showExpressionDialog) {
            TextField("Enter expression", text: $expressionInput)
            Button("Evaluate") {
                evaluateExpression()
            }
            Button("Cancel", role: .cancel) { }
        } message: {
            Text("Enter a mathematical expression (e.g., sin(time) * 10)")
        }
    }
    
    private func evaluateExpression() {
        // Simple expression evaluation (in a real implementation, use a proper math parser)
        let expression = expressionInput.lowercased()
        var result: Float = value
        
        if expression.contains("sin") {
            result = sin(Float.pi / 4) // Example: sin(45°)
        } else if expression.contains("cos") {
            result = cos(Float.pi / 4) // Example: cos(45°)
        } else if expression.contains("sqrt") {
            result = sqrt(abs(value))
        } else if expression.contains("abs") {
            result = abs(value)
        }
        
        undoManager.recordState(value)
        value = min(max(result, range.lowerBound), range.upperBound)
        onValueChanged?(value)
        onExpressionInput?()
    }
}

// MARK: - Enhanced Integer Editor (TODO-1462)
struct IntPropertyEditor: View, PropertyEditor {
    let label: String
    @Binding var value: Int
    var range: ClosedRange<Int> = 0...100
    var step: Int = 1
    
    // Configuration
    let config: PropertyEditorConfig<Int>
    
    // State
    @State private var showTooltip = false
    @State private var showExpressionDialog = false
    @State private var expressionInput = ""
    @State private var showWarning = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    // Protocol conformance
    var defaultValue: Int? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Int) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    init(
        label: String,
        value: Binding<Int>,
        range: ClosedRange<Int> = 0...100,
        step: Int = 1,
        config: PropertyEditorConfig<Int> = PropertyEditorConfig<Int>(propertyType: .integer)
    ) {
        self.label = label
        self._value = value
        self.range = range
        self.step = step
        self.config = config
    }
    
    var body: some View {
        HStack(spacing: 8) {
            // Property type indicator and label
            HStack(spacing: 4) {
                Image(systemName: propertyType.icon)
                    .foregroundColor(propertyType.color)
                    .font(.caption)
                
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isOverridden ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    .frame(width: 80, alignment: .leading)
                
                if isOverridden {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(.orange)
                        .font(.caption)
                        .help("Property overridden")
                }
            }
            
            // Main editor
            HStack(spacing: 4) {
                Stepper(value: $value, in: range, step: step) {
                    HStack(spacing: 2) {
                        Text("\(value)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 50, alignment: .trailing)
                        
                        if let unit = unit {
                            Text(unit)
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
                .disabled(isLocked)
                .onChange(of: value) { newValue in
                    showWarning = (newValue <= range.lowerBound || newValue >= range.upperBound)
                    onValueChanged?(newValue)
                }
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(DesignSystem.Colors.backgroundPrimary)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(showWarning ? Color.orange : DesignSystem.Colors.border, lineWidth: showWarning ? 2 : 1)
                    )
            )
            .opacity(isLocked ? 0.6 : 1.0)
            
            // Action buttons
            HStack(spacing: 2) {
                // Lock/Unlock button
                Button(action: { onLockToggle?(!isLocked) }) {
                    Image(systemName: isLocked ? "lock.fill" : "lock.open")
                        .foregroundColor(isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help(isLocked ? "Unlock property" : "Lock property")
                
                // Reset to default button
                if let defaultValue = defaultValue {
                    Button(action: {
                        undoManager.recordState(value)
                        value = defaultValue
                        onValueChanged?(defaultValue)
                        onReset?()
                    }) {
                        Image(systemName: "arrow.counterclockwise")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("Reset to default")
                }
                
                // Animation keyframe button
                Button(action: { onAnimationKeyframe?() }) {
                    Image(systemName: "key")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Add animation keyframe")
                
                // Expression input button
                Button(action: { showExpressionDialog = true }) {
                    Image(systemName: "function")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Expression input")
                
                // Link property button
                Button(action: { onLinkProperty?() }) {
                    Image(systemName: "link")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Link property")
                
                // Copy/Paste buttons
                Menu {
                    Button("Copy Value") {
                        clipboardManager.copy(value, type: "Int")
                    }
                    
                    if clipboardManager.canPaste(as: "Int") {
                        Button("Paste Value") {
                            if let pastedValue: Int = clipboardManager.paste(as: Int.self) {
                                undoManager.recordState(value)
                                value = pastedValue
                                onValueChanged?(pastedValue)
                            }
                        }
                    }
                    
                    Button("Undo") {
                        if let previousValue: Int = undoManager.undo() {
                            value = previousValue
                            onValueChanged?(previousValue)
                        }
                    }
                    .disabled(!undoManager.canUndo())
                    
                    Button("Redo") {
                        if let nextValue: Int = undoManager.redo() {
                            value = nextValue
                            onValueChanged?(nextValue)
                        }
                    }
                    .disabled(!undoManager.canRedo())
                } label: {
                    Image(systemName: "ellipsis.circle")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("More options")
            }
        }
        .onHover { isHovering in
            showTooltip = isHovering
        }
        .popover(isPresented: showTooltip && tooltip != nil) {
            if let tooltip = tooltip {
                Text(tooltip)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(6)
                    .font(DesignSystem.Typography.caption)
            }
        }
        .alert("Expression Input", isPresented: $showExpressionDialog) {
            TextField("Enter expression", text: $expressionInput)
            Button("Evaluate") {
                evaluateExpression()
            }
            Button("Cancel", role: .cancel) { }
        } message: {
            Text("Enter a mathematical expression (e.g., 5 * 10 + 3)")
        }
    }
    
    private func evaluateExpression() {
        // Simple expression evaluation for integers
        let expression = expressionInput.lowercased()
        var result: Int = value
        
        if expression.contains("+") {
            let parts = expression.split(separator: "+")
            if parts.count == 2, let left = Int(parts[0].trimmingCharacters(in: .whitespaces)),
               let right = Int(parts[1].trimmingCharacters(in: .whitespaces)) {
                result = left + right
            }
        } else if expression.contains("*") {
            let parts = expression.split(separator: "*")
            if parts.count == 2, let left = Int(parts[0].trimmingCharacters(in: .whitespaces)),
               let right = Int(parts[1].trimmingCharacters(in: .whitespaces)) {
                result = left * right
            }
        } else if let directValue = Int(expression) {
            result = directValue
        }
        
        undoManager.recordState(value)
        value = min(max(result, range.lowerBound), range.upperBound)
        onValueChanged?(value)
        onExpressionInput?()
    }
}

// MARK: - Enhanced Boolean Editor (TODO-1463)
struct BoolPropertyEditor: View, PropertyEditor {
    let label: String
    @Binding var value: Bool
    
    // Configuration
    let config: PropertyEditorConfig<Bool>
    
    // State
    @State private var showTooltip = false
    @State private var showExpressionDialog = false
    @State private var expressionInput = ""
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    // Protocol conformance
    var defaultValue: Bool? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Bool) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    init(
        label: String,
        value: Binding<Bool>,
        config: PropertyEditorConfig<Bool> = PropertyEditorConfig<Bool>(propertyType: .boolean)
    ) {
        self.label = label
        self._value = value
        self.config = config
    }
    
    var body: some View {
        HStack(spacing: 8) {
            // Property type indicator and label
            HStack(spacing: 4) {
                Image(systemName: propertyType.icon)
                    .foregroundColor(propertyType.color)
                    .font(.caption)
                
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isOverridden ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    .frame(width: 80, alignment: .leading)
                
                if isOverridden {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(.orange)
                        .font(.caption)
                        .help("Property overridden")
                }
            }
            
            // Main editor
            HStack(spacing: 4) {
                Toggle(isOn: $value) {
                    HStack(spacing: 2) {
                        Text(value ? "True" : "False")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(value ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                        
                        if let unit = unit {
                            Text(unit)
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
                .toggleStyle(.checkbox)
                .disabled(isLocked)
                .onChange(of: value) { newValue in
                    onValueChanged?(newValue)
                }
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(DesignSystem.Colors.backgroundPrimary)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(DesignSystem.Colors.border, lineWidth: 1)
                    )
            )
            .opacity(isLocked ? 0.6 : 1.0)
            
            // Action buttons
            HStack(spacing: 2) {
                // Lock/Unlock button
                Button(action: { onLockToggle?(!isLocked) }) {
                    Image(systemName: isLocked ? "lock.fill" : "lock.open")
                        .foregroundColor(isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help(isLocked ? "Unlock property" : "Lock property")
                
                // Reset to default button
                if let defaultValue = defaultValue {
                    Button(action: {
                        undoManager.recordState(value)
                        value = defaultValue
                        onValueChanged?(defaultValue)
                        onReset?()
                    }) {
                        Image(systemName: "arrow.counterclockwise")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("Reset to default")
                }
                
                // Animation keyframe button
                Button(action: { onAnimationKeyframe?() }) {
                    Image(systemName: "key")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Add animation keyframe")
                
                // Expression input button
                Button(action: { showExpressionDialog = true }) {
                    Image(systemName: "function")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Expression input")
                
                // Link property button
                Button(action: { onLinkProperty?() }) {
                    Image(systemName: "link")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Link property")
                
                // Copy/Paste buttons
                Menu {
                    Button("Copy Value") {
                        clipboardManager.copy(value, type: "Bool")
                    }
                    
                    if clipboardManager.canPaste(as: "Bool") {
                        Button("Paste Value") {
                            if let pastedValue: Bool = clipboardManager.paste(as: Bool.self) {
                                undoManager.recordState(value)
                                value = pastedValue
                                onValueChanged?(pastedValue)
                            }
                        }
                    }
                    
                    Button("Toggle Value") {
                        undoManager.recordState(value)
                        value.toggle()
                        onValueChanged?(value)
                    }
                    
                    Button("Undo") {
                        if let previousValue: Bool = undoManager.undo() {
                            value = previousValue
                            onValueChanged?(previousValue)
                        }
                    }
                    .disabled(!undoManager.canUndo())
                    
                    Button("Redo") {
                        if let nextValue: Bool = undoManager.redo() {
                            value = nextValue
                            onValueChanged?(nextValue)
                        }
                    }
                    .disabled(!undoManager.canRedo())
                } label: {
                    Image(systemName: "ellipsis.circle")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("More options")
            }
        }
        .onHover { isHovering in
            showTooltip = isHovering
        }
        .popover(isPresented: showTooltip && tooltip != nil) {
            if let tooltip = tooltip {
                Text(tooltip)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(6)
                    .font(DesignSystem.Typography.caption)
            }
        }
        .alert("Expression Input", isPresented: $showExpressionDialog) {
            TextField("Enter expression", text: $expressionInput)
            Button("Evaluate") {
                evaluateExpression()
            }
            Button("Cancel", role: .cancel) { }
        } message: {
            Text("Enter a boolean expression (e.g., true, false, !value)")
        }
    }
    
    private func evaluateExpression() {
        // Simple expression evaluation for booleans
        let expression = expressionInput.lowercased().trimmingCharacters(in: .whitespaces)
        var result: Bool = value
        
        switch expression {
        case "true":
            result = true
        case "false":
            result = false
        case "!value", "not value":
            result = !value
        case "toggle":
            result.toggle()
        default:
            // Try to evaluate as a boolean
            if let boolValue = Bool(expression) {
                result = boolValue
            }
        }
        
        undoManager.recordState(value)
        value = result
        onValueChanged?(value)
        onExpressionInput?()
    }
}

// MARK: - Enhanced String Editor (TODO-1464)
struct StringPropertyEditor: View, PropertyEditor {
    let label: String
    @Binding var value: String
    var isMultiline: Bool = false
    var maxLength: Int? = nil
    
    // Configuration
    let config: PropertyEditorConfig<String>
    
    // State
    @State private var showTooltip = false
    @State private var showExpressionDialog = false
    @State private var expressionInput = ""
    @State private var showWarning = false
    @State private var isEditing = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    // Protocol conformance
    var defaultValue: String? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((String) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    init(
        label: String,
        value: Binding<String>,
        isMultiline: Bool = false,
        maxLength: Int? = nil,
        config: PropertyEditorConfig<String> = PropertyEditorConfig<String>(propertyType: .string)
    ) {
        self.label = label
        self._value = value
        self.isMultiline = isMultiline
        self.maxLength = maxLength
        self.config = config
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Header with type indicator and label
            HStack(spacing: 8) {
                HStack(spacing: 4) {
                    Image(systemName: propertyType.icon)
                        .foregroundColor(propertyType.color)
                        .font(.caption)
                    
                    Text(label)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(isOverridden ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    
                    if isOverridden {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(.orange)
                            .font(.caption)
                            .help("Property overridden")
                    }
                }
                
                Spacer()
                
                // Length indicator and warning
                HStack(spacing: 4) {
                    if let maxLength = maxLength {
                        Text("\(value.count)/\(maxLength)")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(value.count > maxLength ? .red : DesignSystem.Colors.textSecondary)
                    } else {
                        Text("\(value.count)")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    
                    if showWarning {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(.orange)
                            .font(.caption)
                            .help("String length exceeds limit")
                    }
                }
                
                // Action buttons
                HStack(spacing: 2) {
                    // Lock/Unlock button
                    Button(action: { onLockToggle?(!isLocked) }) {
                        Image(systemName: isLocked ? "lock.fill" : "lock.open")
                            .foregroundColor(isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help(isLocked ? "Unlock property" : "Lock property")
                    
                    // Reset to default button
                    if let defaultValue = defaultValue {
                        Button(action: {
                            undoManager.recordState(value)
                            value = defaultValue
                            onValueChanged?(defaultValue)
                            onReset?()
                        }) {
                            Image(systemName: "arrow.counterclockwise")
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        .buttonStyle(.plain)
                        .help("Reset to default")
                    }
                    
                    // Animation keyframe button
                    Button(action: { onAnimationKeyframe?() }) {
                        Image(systemName: "key")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("Add animation keyframe")
                    
                    // Expression input button
                    Button(action: { showExpressionDialog = true }) {
                        Image(systemName: "function")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("Expression input")
                    
                    // Link property button
                    Button(action: { onLinkProperty?() }) {
                        Image(systemName: "link")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("Link property")
                    
                    // Copy/Paste buttons
                    Menu {
                        Button("Copy Value") {
                            clipboardManager.copy(value, type: "String")
                        }
                        
                        if clipboardManager.canPaste(as: "String") {
                            Button("Paste Value") {
                                if let pastedValue: String = clipboardManager.paste(as: String.self) {
                                    undoManager.recordState(value)
                                    value = pastedValue
                                    onValueChanged?(pastedValue)
                                }
                            }
                        }
                        
                        Button("Clear Value") {
                            undoManager.recordState(value)
                            value = ""
                            onValueChanged?(value)
                        }
                        
                        Button("Undo") {
                            if let previousValue: String = undoManager.undo() {
                                value = previousValue
                                onValueChanged?(previousValue)
                            }
                        }
                        .disabled(!undoManager.canUndo())
                        
                        Button("Redo") {
                            if let nextValue: String = undoManager.redo() {
                                value = nextValue
                                onValueChanged?(nextValue)
                            }
                        }
                        .disabled(!undoManager.canRedo())
                    } label: {
                        Image(systemName: "ellipsis.circle")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("More options")
                }
            }
            
            // Main editor
            VStack(alignment: .leading, spacing: 4) {
                if isMultiline {
                    TextEditor(text: $value)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(minHeight: 80)
                        .padding(4)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(DesignSystem.Colors.backgroundPrimary)
                                .overlay(
                                    RoundedRectangle(cornerRadius: 4)
                                        .stroke(showWarning ? Color.orange : DesignSystem.Colors.border, lineWidth: showWarning ? 2 : 1)
                                )
                        )
                        .disabled(isLocked)
                        .opacity(isLocked ? 0.6 : 1.0)
                        .onTapGesture {
                            if !isLocked {
                                isEditing = true
                            }
                        }
                } else {
                    HStack(spacing: 4) {
                        TextField("", text: $value)
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .padding(8)
                            .background(
                                RoundedRectangle(cornerRadius: 4)
                                    .fill(DesignSystem.Colors.backgroundPrimary)
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 4)
                                            .stroke(showWarning ? Color.orange : DesignSystem.Colors.border, lineWidth: showWarning ? 2 : 1)
                                    )
                            )
                            .disabled(isLocked)
                            .opacity(isLocked ? 0.6 : 1.0)
                            .onTapGesture {
                                if !isLocked {
                                    isEditing = true
                                }
                            }
                        
                        if let unit = unit {
                            Text(unit)
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
            }
        }
        .onHover { isHovering in
            showTooltip = isHovering
        }
        .popover(isPresented: showTooltip && tooltip != nil) {
            if let tooltip = tooltip {
                Text(tooltip)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(6)
                    .font(DesignSystem.Typography.caption)
            }
        }
        .alert("Expression Input", isPresented: $showExpressionDialog) {
            TextField("Enter expression", text: $expressionInput)
            Button("Evaluate") {
                evaluateExpression()
            }
            Button("Cancel", role: .cancel) { }
        } message: {
            Text("Enter a string expression (e.g., \"Hello \" + \"World\")")
        }
        .onChange(of: value) { newValue in
            // Check for length warnings
            if let maxLength = maxLength {
                showWarning = newValue.count > maxLength
            }
            onValueChanged?(newValue)
        }
    }
    
    private func evaluateExpression() {
        // Simple expression evaluation for strings
        let expression = expressionInput
        var result: String = value
        
        if expression.contains("+") {
            let parts = expression.split(separator: "+")
            if parts.count == 2 {
                let left = parts[0].trimmingCharacters(in: .whitespacesAndNewlines)
                let right = parts[1].trimmingCharacters(in: .whitespacesAndNewlines)
                result = left + right
            }
        } else if expression.hasPrefix("\"") && expression.hasSuffix("\"") {
            // Remove quotes
            result = String(expression.dropFirst().dropLast())
        } else if expression.lowercased() == "uppercase" {
            result = value.uppercased()
        } else if expression.lowercased() == "lowercase" {
            result = value.lowercased()
        } else if expression.lowercased() == "reverse" {
            result = String(value.reversed())
        } else {
            result = expression
        }
        
        undoManager.recordState(value)
        
        // Apply length limit if specified
        if let maxLength = maxLength, result.count > maxLength {
            result = String(result.prefix(maxLength))
        }
        
        value = result
        onValueChanged?(value)
        onExpressionInput?()
    }
}

// MARK: - Vector3 Editor (TODO-1467)
struct Vector3PropertyEditor: View {
    let label: String
    @Binding var value: SIMD3<Float>
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: 4) {
                AxisField(axis: "X", value: $value.x, color: .red)
                AxisField(axis: "Y", value: $value.y, color: .green)
                AxisField(axis: "Z", value: $value.z, color: .blue)
            }
        }
    }
}

private struct AxisField: View {
    let axis: String
    @Binding var value: Float
    let color: Color
    
    var body: some View {
        HStack(spacing: 2) {
            Text(axis)
                .font(DesignSystem.Typography.small)
                .foregroundColor(color)
                .frame(width: 14)
            
            TextField("", value: $value, format: .number.precision(.fractionLength(2)))
                .textFieldStyle(.plain)
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .frame(minWidth: 50)
        }
        .padding(.horizontal, 6)
        .padding(.vertical, 4)
    }
    
    @ViewBuilder
    private var axisAngleRotationControls: some View {
        VStack(spacing: 4) {
            HStack(spacing: 4) {
                Text("Axis:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .frame(width: 40)
                
                ForEach(0..<3, id: \.self) { index in
                    HStack(spacing: 2) {
                        Text(["X", "Y", "Z"][index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                            .frame(width: 14)
                        
                        TextField("", value: bindingForAxisAngleAxis(index), format: .number.precision(.fractionLength(3)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(minWidth: 60)
                    }
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
            }
            
            HStack(spacing: 4) {
                Text("Angle:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .frame(width: 40)
                
                TextField("", value: bindingForAxisAngleAngle(), format: .number.precision(.fractionLength(useDegrees ? 1 : 3)))
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(minWidth: 60)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                
                Text(useDegrees ? "°" : "rad")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Spacer()
                
                Button(action: { axisAngleValues.axis = normalize(axisAngleValues.axis) }) {
                    Text("Normalize Axis")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    @ViewBuilder
    private var lookAtRotationControls: some View {
        VStack(spacing: 4) {
            Text("Target Position:")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            HStack(spacing: 4) {
                ForEach(0..<3, id: \.self) { index in
                    HStack(spacing: 2) {
                        Text(["X", "Y", "Z"][index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                            .frame(width: 14)
                        
                        TextField("", value: bindingForLookAtAxis(index), format: .number.precision(.fractionLength(3)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(minWidth: 60)
                    }
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
                
                Button(action: updateLookAtRotation) {
                    Text("Apply")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    @ViewBuilder
    private var rotationConstraintsPanel: some View {
        VStack(spacing: 4) {
            HStack {
                Toggle("Enable Constraints", isOn: $rotationConstraints.enabled)
                    .font(DesignSystem.Typography.small)
                
                Spacer()
            }
            
            if rotationConstraints.enabled {
                HStack(spacing: 8) {
                    VStack(alignment: .leading) {
                        Text("X Range:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        HStack {
                            TextField("Min", value: $rotationConstraints.minX, format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 60)
                            Text("to")
                                .font(DesignSystem.Typography.small)
                            TextField("Max", value: $rotationConstraints.maxX, format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 60)
                        }
                    }
                    
                    VStack(alignment: .leading) {
                        Text("Y Range:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        HStack {
                            TextField("Min", value: $rotationConstraints.minY, format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 60)
                            Text("to")
                                .font(DesignSystem.Typography.small)
                            TextField("Max", value: $rotationConstraints.maxY, format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 60)
                        }
                    }
                    
                    VStack(alignment: .leading) {
                        Text("Z Range:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        HStack {
                            TextField("Min", value: $rotationConstraints.minZ, format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 60)
                            Text("to")
                                .font(DesignSystem.Typography.small)
                            TextField("Max", value: $rotationConstraints.maxZ, format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 60)
                        }
                    }
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundPrimary)
                .cornerRadius(4)
            }
        }
    }
    
    @ViewBuilder
    private var flipMirrorPanel: some View {
        HStack(spacing: 8) {
            Button(action: flipX) {
                Text("Flip X")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
            
            Button(action: flipY) {
                Text("Flip Y")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
            
            Button(action: flipZ) {
                Text("Flip Z")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
            
            Button(action: mirror) {
                Text("Mirror")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
        }
    }
    
    @ViewBuilder
    private var lookAtHelperPanel: some View {
        VStack(spacing: 4) {
            Text("Quick Look At:")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            HStack(spacing: 4) {
                ForEach(0..<3, id: \.self) { index in
                    HStack(spacing: 2) {
                        Text(["X", "Y", "Z"][index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                            .frame(width: 14)
                        
                        TextField("", value: bindingForLookAtAxis(index), format: .number.precision(.fractionLength(3)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(minWidth: 60)
                    }
                    .padding(.horizontal, 6)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
                
                Button(action: updateLookAtRotation) {
                    Text("Look At")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    @ViewBuilder
    private var animationCurvePanel: some View {
        VStack(spacing: 4) {
            Text("Animation Curve Integration:")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            HStack {
                Text("Curve Type:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                // Placeholder for curve type picker
                Text("Linear")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                
                Spacer()
            }
        }
    }
    
    @ViewBuilder
    private var slerpLerpPanel: some View {
        VStack(spacing: 4) {
            Text("Slerp/Lerp Preview:")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            HStack {
                Button(action: { slerpStart = value }) {
                    Text("Set Start")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                
                Button(action: { slerpEnd = value }) {
                    Text("Set End")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
                
                Slider(value: $slerpT, in: 0...1)
                    .frame(width: 100)
                
                Button(action: { value = simd_slerp(slerpStart, slerpEnd, slerpT) }) {
                    Text("Apply Slerp")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    // MARK: - Helper Methods
    
    private func updateFromQuaternion() {
        let euler = value.eulerAngles.xyz
        eulerValues = useDegrees ? euler * 180 / .pi : euler
        
        // Update axis-angle
        let q = normalize(value)
        axisAngleValues.angle = 2 * acos(abs(q.w))
        if abs(q.w) < 0.999 {
            let s = sqrt(1 - q.w * q.w)
            axisAngleValues.axis = q.vector / s
        } else {
            axisAngleValues.axis = SIMD3<Float>(1, 0, 0)
        }
    }
    
    private func updateToQuaternion() {
        let euler = useDegrees ? eulerValues * .pi / 180 : eulerValues
        value = simd_quatf(euler: euler)
    }
    
    private func bindingForEulerAxis(_ index: Int) -> Binding<Float> {
        Binding<Float>(
            get: { eulerValues[index] },
            set: { newValue in
                eulerValues[index] = newValue
                if rotationConstraints.enabled {
                    eulerValues[index] = max(rotationConstraints[index * 2], min(rotationConstraints[index * 2 + 1], newValue))
                }
                updateToQuaternion()
            }
        )
    }
    
    private func bindingForQuaternionAxis(_ index: Int) -> Binding<Float> {
        Binding<Float>(
            get: { value.vector[index] },
            set: { newValue in
                var newVector = value.vector
                newVector[index] = newValue
                value = simd_quatf(vector: newVector, scalar: value.w)
                value = normalize(value)
            }
        )
    }
    
    private func bindingForAxisAngleAxis(_ index: Int) -> Binding<Float> {
        Binding<Float>(
            get: { axisAngleValues.axis[index] },
            set: { newValue in
                axisAngleValues.axis[index] = newValue
            }
        )
    }
    
    private func bindingForAxisAngleAngle() -> Binding<Float> {
        Binding<Float>(
            get: { useDegrees ? axisAngleValues.angle * 180 / .pi : axisAngleValues.angle },
            set: { newValue in
                axisAngleValues.angle = useDegrees ? newValue * .pi / 180 : newValue
            }
        )
    }
    
    private func bindingForLookAtAxis(_ index: Int) -> Binding<Float> {
        Binding<Float>(
            get: { lookAtTarget[index] },
            set: { lookAtTarget[index] = $0 }
        )
    }
    
    private func updateLookAtRotation() {
        let forward = normalize(lookAtTarget)
        let up = SIMD3<Float>(0, 1, 0)
        let right = normalize(cross(up, forward))
        let correctedUp = cross(forward, right)
        
        let rotationMatrix = float3x3(
            [right.x, correctedUp.x, forward.x],
            [right.y, correctedUp.y, forward.y],
            [right.z, correctedUp.z, forward.z]
        )
        
        value = simd_quatf(rotationMatrix)
    }
    
    private func snapToNearestAxis() {
        let euler = value.eulerAngles.xyz
        let snapAngle: Float = useDegrees ? 45 * .pi / 180 : Float.pi / 4
        
        let snappedEuler = SIMD3<Float>(
            round(euler.x / snapAngle) * snapAngle,
            round(euler.y / snapAngle) * snapAngle,
            round(euler.z / snapAngle) * snapAngle
        )
        
        value = simd_quatf(euler: snappedEuler)
    }
    
    private func flipX() {
        let flipQuat = simd_quatf(angle: Float.pi, axis: SIMD3<Float>(1, 0, 0))
        value = flipQuat * value
    }
    
    private func flipY() {
        let flipQuat = simd_quatf(angle: Float.pi, axis: SIMD3<Float>(0, 1, 0))
        value = flipQuat * value
    }
    
    private func flipZ() {
        let flipQuat = simd_quatf(angle: Float.pi, axis: SIMD3<Float>(0, 0, 1))
        value = flipQuat * value
    }
    
    private func mirror() {
        value = simd_quatf(ix: -value.imag.x, iy: value.imag.y, iz: value.imag.z, r: value.real)
    }
}

// MARK: - Float Editor (TODO-1461)
struct ColorPropertyEditor: View {
    let label: String
    @Binding var value: Color
    @State private var showPicker = false
    
    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            ColorPicker("", selection: $value)
                .labelsHidden()
            
            RoundedRectangle(cornerRadius: 4)
                .fill(value)
                .frame(width: 60, height: 24)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
        }
    }
}

// MARK: - HDR Color Editor (TODO-1561)
struct HDRColorPropertyEditor: View {
    let label: String
    @Binding var value: Color
    @State private var showPicker = false
    @State private var useHDR: Bool = false
    @State private var intensity: Float = 1.0
    @State private var colorSpace: ColorSpace = .sRGB
    @State private var showAdvanced: Bool = false
    
    enum ColorSpace: String, CaseIterable {
        case sRGB = "sRGB"
        case linear = "Linear"
        case HDR = "HDR"
    }
    
    private var hdrValue: Color {
        if useHDR {
            return Color(
                red: min(value.components.red * intensity, 1.0),
                green: min(value.components.green * intensity, 1.0),
                blue: min(value.components.blue * intensity, 1.0),
                opacity: value.components.alpha
            )
        }
        return value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(spacing: 8) {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .frame(width: 80, alignment: .leading)
                
                // Color picker
                ColorPicker("", selection: $value)
                    .labelsHidden()
                
                // Color preview with HDR indication
                RoundedRectangle(cornerRadius: 4)
                    .fill(hdrValue)
                    .frame(width: 60, height: 24)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(useHDR ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: useHDR ? 2 : 1)
                    )
                    .overlay(
                        // HDR indicator
                        Group {
                            if useHDR && intensity > 1.0 {
                                VStack {
                                    Spacer()
                                    HStack {
                                        Spacer()
                                        Text("HDR")
                                            .font(DesignSystem.Typography.small)
                                            .foregroundColor(.white)
                                            .padding(.horizontal, 4)
                                            .padding(.vertical, 1)
                                            .background(DesignSystem.Colors.accentPrimary)
                                            .cornerRadius(2)
                                    }
                                }
                                .padding(2)
                            }
                        }
                    )
                
                // Color space picker
                Picker("", selection: $colorSpace) {
                    ForEach(ColorSpace.allCases, id: \.self) { space in
                        Text(space.rawValue).tag(space)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 70)
            }
            
            // HDR controls
            HStack {
                Toggle("HDR Mode", isOn: $useHDR)
                    .font(DesignSystem.Typography.small)
                
                if useHDR {
                    Text("Intensity:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Slider(value: $intensity, in: 0.1...10.0, step: 0.1)
                        .frame(width: 100)
                    
                    Text(String(format: "%.1f", intensity))
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 40)
                }
                
                Spacer()
                
                Button(action: { showAdvanced.toggle() }) {
                    HStack {
                        Text("Advanced")
                            .font(DesignSystem.Typography.small)
                        Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                            .font(DesignSystem.Typography.small)
                    }
                }
                .buttonStyle(.plain)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            
            // Advanced panel
            if showAdvanced {
                VStack(spacing: 4) {
                    // Color values display
                    HStack {
                        Text("RGB:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text(String(format: "R:%.3f G:%.3f B:%.3f", 
                                  value.components.red, 
                                  value.components.green, 
                                  value.components.blue))
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        Text("HDR:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text(String(format: "R:%.3f G:%.3f B:%.3f", 
                                  hdrValue.components.red, 
                                  hdrValue.components.green, 
                                  hdrValue.components.blue))
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                    }
                    
                    // HSV display
                    HStack {
                        Text("HSV:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        let hsv = value.toHSV()
                        Text(String(format: "H:%.0f° S:%.0f%% V:%.0f%%", 
                                  hsv.hue * 360, 
                                  hsv.saturation * 100, 
                                  hsv.value * 100))
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                    }
                    
                    // Hex display
                    HStack {
                        Text("Hex:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text(value.toHex())
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                    }
                    
                    // Preset buttons
                    HStack {
                        Text("Presets:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Button(action: { value = Color.red }) {
                            Circle()
                                .fill(Color.red)
                                .frame(width: 20, height: 20)
                        }
                        .buttonStyle(.plain)
                        
                        Button(action: { value = Color.green }) {
                            Circle()
                                .fill(Color.green)
                                .frame(width: 20, height: 20)
                        }
                        .buttonStyle(.plain)
                        
                        Button(action: { value = Color.blue }) {
                            Circle()
                                .fill(Color.blue)
                                .frame(width: 20, height: 20)
                        }
                        .buttonStyle(.plain)
                        
                        Button(action: { value = Color.white }) {
                            Circle()
                                .fill(Color.white)
                                .frame(width: 20, height: 20)
                                .overlay(
                                    Circle()
                                        .stroke(Color.black, lineWidth: 1)
                                )
                        }
                        .buttonStyle(.plain)
                        
                        Button(action: { value = Color.black }) {
                            Circle()
                                .fill(Color.black)
                                .frame(width: 20, height: 20)
                        }
                        .buttonStyle(.plain)
                        
                        Spacer()
                    }
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundPrimary)
                .cornerRadius(4)
            }
        }
    }
}

// MARK: - Enum/Dropdown Editor (TODO-1478)
struct EnumPropertyEditor<T: Hashable & CaseIterable & CustomStringConvertible>: View where T.AllCases: RandomAccessCollection {
    let label: String
    @Binding var value: T
    
    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            Picker("", selection: $value) {
                ForEach(Array(T.allCases), id: \.self) { option in
                    Text(option.description)
                        .tag(option)
                }
            }
            .pickerStyle(.menu)
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
        }
    }
}

// MARK: - Asset Reference Editor (TODO-1480)
struct AssetReferenceEditor: View {
    let label: String
    @Binding var assetPath: String
    let assetType: AssetType
    @State private var showPicker = false
    
    enum AssetType {
        case material, texture, mesh, audio, animation, prefab
        
        var icon: String {
            switch self {
            case .material: return "paintpalette"
            case .texture: return "photo"
            case .mesh: return "cube"
            case .audio: return "speaker.wave.2"
            case .animation: return "figure.walk"
            case .prefab: return "cube.transparent"
            }
        }
    }
    
    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            HStack {
                Image(systemName: assetType.icon)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                
                Text(assetPath.isEmpty ? "None" : URL(fileURLWithPath: assetPath).lastPathComponent)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(assetPath.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Button(action: { showPicker = true }) {
                    Image(systemName: "magnifyingglass")
                }
                .buttonStyle(.plain)
                
                if !assetPath.isEmpty {
                    Button(action: { assetPath = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
        }
    }
}

// MARK: - Array/List Editor (TODO-1490)
struct ArrayPropertyEditor<Element: Identifiable>: View {
    let label: String
    @Binding var items: [Element]
    let itemView: (Binding<Element>) -> AnyView
    let createNew: () -> Element
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text("\(items.count) items")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textDisabled)
                
                Button(action: { items.append(createNew()) }) {
                    Image(systemName: "plus.circle")
                }
                .buttonStyle(.plain)
            }
            
            ForEach($items) { $item in
                HStack {
                    itemView($item)
                    
                    Button(action: {
                        items.removeAll { $0.id == item.id }
                    }) {
                        Image(systemName: "minus.circle")
                            .foregroundColor(.red)
                    }
                    .buttonStyle(.plain)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
}

// MARK: - Matrix3x3 Editor (TODO-1757)
struct Matrix3x3PropertyEditor: View {
    let label: String
    @Binding var value: simd_float3x3
    @State private var showExpanded = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button(action: { showExpanded.toggle() }) {
                    Image(systemName: showExpanded ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            if showExpanded {
                VStack(spacing: 2) {
                    ForEach(0..<3, id: \.self) { row in
                        HStack(spacing: 4) {
                            ForEach(0..<3, id: \.self) { col in
                                TextField("", value: $value[row, col], format: .number.precision(.fractionLength(3)))
                                    .textFieldStyle(.plain)
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                    .frame(width: 60)
                                    .padding(4)
                                    .background(DesignSystem.Colors.backgroundTertiary)
                                    .cornerRadius(3)
                            }
                        }
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
            } else {
                HStack {
                    Text("3×3 Matrix")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Spacer()
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
            }
        }
    }
}

// MARK: - Matrix4x4 Editor (TODO-1758)
struct Matrix4x4PropertyEditor: View {
    let label: String
    @Binding var value: simd_float4x4
    @State private var showExpanded = false
    @State private var displayMode: MatrixDisplayMode = .standard
    
    enum MatrixDisplayMode: String, CaseIterable {
        case standard = "Standard"
        case translation = "Translation"
        case rotation = "Rotation"
        case scale = "Scale"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("", selection: $displayMode) {
                    ForEach(MatrixDisplayMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.menu)
                .font(DesignSystem.Typography.small)
                
                Button(action: { showExpanded.toggle() }) {
                    Image(systemName: showExpanded ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            if showExpanded {
                VStack(spacing: 2) {
                    ForEach(0..<4, id: \.self) { row in
                        HStack(spacing: 4) {
                            ForEach(0..<4, id: \.self) { col in
                                TextField("", value: $value[row, col], format: .number.precision(.fractionLength(3)))
                                    .textFieldStyle(.plain)
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                    .frame(width: 60)
                                    .padding(4)
                                    .background(DesignSystem.Colors.backgroundTertiary)
                                    .cornerRadius(3)
                            }
                        }
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
            } else {
                matrixSummary
            }
        }
    }
    
    @ViewBuilder
    private var matrixSummary: some View {
        HStack {
            switch displayMode {
            case .standard:
                Text("4×4 Matrix")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            case .translation:
                vector3Display("T", value.translation)
            case .rotation:
                Text("Rotation Matrix")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            case .scale:
                vector3Display("S", SIMD3<Float>(value[0,0], value[1,1], value[2,2]))
            }
            Spacer()
        }
        .padding(8)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(6)
    }
    
    private func vector3Display(_ prefix: String, _ vector: SIMD3<Float>) -> some View {
        HStack(spacing: 4) {
            Text("\(prefix):")
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            Text(String(format: "%.2f, %.2f, %.2f", vector.x, vector.y, vector.z))
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textTertiary)
        }
    }
}

// MARK: - Plane Editor (TODO-1759)
struct PlanePropertyEditor: View {
    let label: String
    @Binding var normal: SIMD3<Float>
    @Binding var distance: Float
    @State private var showAdvanced = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button("Normalize") {
                    let length = simd_length(normal)
                    if length > 0 {
                        normal = normal / length
                    }
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Normal", value: $normal)
                
                HStack {
                    Text("Distance")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $distance, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                }
            }
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Plane Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Length: \(String(format: "%.3f", simd_length(normal)))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("Normalized: \(simd_length(normal) > 0.999 ? "Yes" : "No")")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(simd_length(normal) > 0.999 ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentWarning)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
}

// MARK: - Ray Editor (TODO-1760)
struct RayPropertyEditor: View {
    let label: String
    @Binding var origin: SIMD3<Float>
    @Binding var direction: SIMD3<Float>
    @State private var showAdvanced = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button("Normalize Dir") {
                    let length = simd_length(direction)
                    if length > 0 {
                        direction = direction / length
                    }
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Origin", value: $origin)
                Vector3PropertyEditor(label: "Direction", value: $direction)
            }
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Ray Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Dir Length: \(String(format: "%.3f", simd_length(direction)))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("Normalized: \(simd_length(direction) > 0.999 ? "Yes" : "No")")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(simd_length(direction) > 0.999 ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentWarning)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
}

// MARK: - Sphere Editor (TODO-1761)
struct SpherePropertyEditor: View {
    let label: String
    @Binding var center: SIMD3<Float>
    @Binding var radius: Float
    @State private var showAdvanced = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if showAdvanced {
                    Text("Vol: \(String(format: "%.2f", volume))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Center", value: $center)
                
                HStack {
                    Text("Radius")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $radius, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $radius, in: 0.01...100, step: 0.1)
                        .frame(maxWidth: .infinity)
                }
            }
        }
    }
    
    private var volume: Float {
        return (4.0 / 3.0) * Float.pi * radius * radius * radius
    }
}

// MARK: - Box Editor (TODO-1762)
struct BoxPropertyEditor: View {
    let label: String
    @Binding var center: SIMD3<Float>
    @Binding var size: SIMD3<Float>
    @State private var showAdvanced = false
    @State private var proportionalScale = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Toggle("Proportional", isOn: $proportionalScale)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
                
                if showAdvanced {
                    Text("Vol: \(String(format: "%.2f", volume))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Center", value: $center)
                
                VStack(alignment: .leading, spacing: 2) {
                    Text("Size")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    HStack(spacing: 4) {
                        AxisField(axis: "X", value: $size.x, color: DesignSystem.Colors.xAxis, onValueChange: proportionalScale ? updateProportional : nil)
                        AxisField(axis: "Y", value: $size.y, color: DesignSystem.Colors.yAxis, onValueChange: proportionalScale ? updateProportional : nil)
                        AxisField(axis: "Z", value: $size.z, color: DesignSystem.Colors.zAxis, onValueChange: proportionalScale ? updateProportional : nil)
                    }
                }
            }
        }
    }
    
    private var volume: Float {
        return size.x * size.y * size.z
    }
    
    private func updateProportional() {
        // This would need to track which axis changed and scale others proportionally
        // For now, it's a placeholder for the proportional scaling feature
    }
}

// MARK: - Enhanced Axis Field with Callback
private struct AxisField: View {
    let axis: String
    @Binding var value: Float
    let color: Color
    let onValueChange: (() -> Void)?
    
    init(axis: String, value: Binding<Float>, color: Color, onValueChange: (() -> Void)? = nil) {
        self.axis = axis
        self._value = value
        self.color = color
        self.onValueChange = onValueChange
    }
    
    var body: some View {
        HStack(spacing: 2) {
            Text(axis)
                .font(DesignSystem.Typography.small)
                .foregroundColor(color)
                .frame(width: 14)
            
            TextField("", value: $value, format: .number.precision(.fractionLength(2)))
                .textFieldStyle(.plain)
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .frame(minWidth: 50)
                .onChange(of: value) { _ in
                    onValueChange?()
                }
        }
        .padding(.horizontal, 6)
        .padding(.vertical, 4)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
}

// MARK: - Capsule Editor (TODO-1763)
struct CapsulePropertyEditor: View {
    let label: String
    @Binding var start: SIMD3<Float>
    @Binding var end: SIMD3<Float>
    @Binding var radius: Float
    @State private var showAdvanced = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if showAdvanced {
                    Text("Height: \(String(format: "%.2f", height))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Text("Vol: \(String(format: "%.2f", volume))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Start", value: $start)
                Vector3PropertyEditor(label: "End", value: $end)
                
                HStack {
                    Text("Radius")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $radius, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $radius, in: 0.01...50, step: 0.1)
                        .frame(maxWidth: .infinity)
                }
            }
        }
    }
    
    private var height: Float {
        return simd_distance(start, end)
    }
    
    private var volume: Float {
        let h = height
        let cylinderVolume = Float.pi * radius * radius * h
        let sphereVolume = (4.0 / 3.0) * Float.pi * radius * radius * radius
        return cylinderVolume + sphereVolume
    }
}

// MARK: - Cylinder Editor (TODO-1764)
struct CylinderPropertyEditor: View {
    let label: String
    @Binding var center: SIMD3<Float>
    @Binding var radius: Float
    @Binding var height: Float
    @State private var showAdvanced = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if showAdvanced {
                    Text("Vol: \(String(format: "%.2f", volume))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Text("Area: \(String(format: "%.2f", surfaceArea))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Center", value: $center)
                
                HStack {
                    Text("Radius")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $radius, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $radius, in: 0.01...100, step: 0.1)
                        .frame(maxWidth: .infinity)
                }
                
                HStack {
                    Text("Height")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $height, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $height, in: 0.01...200, step: 0.1)
                        .frame(maxWidth: .infinity)
                }
            }
        }
    }
    
    private var volume: Float {
        return Float.pi * radius * radius * height
    }
    
    private var surfaceArea: Float {
        return 2 * Float.pi * radius * (radius + height)
    }
}

// MARK: - Cone Editor (TODO-1765)
struct ConePropertyEditor: View {
    let label: String
    @Binding var apex: SIMD3<Float>
    @Binding var direction: SIMD3<Float>
    @Binding var angle: Float // in radians
    @State private var showAdvanced = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button("Normalize Dir") {
                    let length = simd_length(direction)
                    if length > 0 {
                        direction = direction / length
                    }
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                if showAdvanced {
                    Text("Vol: \(String(format: "%.2f", volume))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Apex", value: $apex)
                Vector3PropertyEditor(label: "Direction", value: $direction)
                
                HStack {
                    Text("Angle")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $angle, format: .number.precision(.fractionLength(4)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $angle, in: 0.01...Float.pi - 0.01, step: 0.01)
                        .frame(maxWidth: .infinity)
                }
                
                HStack {
                    Text("\(String(format: "%.1f", angle * 180 / Float.pi))°")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Spacer()
                }
            }
        }
    }
    
    private var volume: Float {
        // Volume of infinite cone - this would need a height parameter for finite cone
        // For now, showing placeholder calculation
        let tanAngle = tan(angle)
        return (1.0 / 3.0) * Float.pi * tanAngle * tanAngle * tanAngle
    }
}

// MARK: - Frustum Editor (TODO-1766)
struct FrustumPropertyEditor: View {
    let label: String
    @Binding var position: SIMD3<Float>
    @Binding var rotation: SIMD3<Float> // Euler angles in radians
    @Binding var fov: Float // Field of view in radians
    @Binding var nearPlane: Float
    @Binding var farPlane: Float
    @Binding var aspectRatio: Float
    @State private var showAdvanced = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if showAdvanced {
                    Text("Aspect: \(String(format: "%.2f", aspectRatio))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                Vector3PropertyEditor(label: "Position", value: $position)
                Vector3PropertyEditor(label: "Rotation", value: $rotation)
                
                HStack {
                    Text("FOV")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $fov, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $fov, in: 0.1...Float.pi - 0.1, step: 0.01)
                        .frame(maxWidth: .infinity)
                }
                
                HStack {
                    Text("\(String(format: "%.1f", fov * 180 / Float.pi))°")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Spacer()
                }
                
                HStack {
                    Text("Near")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $nearPlane, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $nearPlane, in: 0.01...farPlane - 0.01, step: 0.01)
                        .frame(maxWidth: .infinity)
                }
                
                HStack {
                    Text("Far")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $farPlane, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $farPlane, in: nearPlane + 0.01...10000, step: 0.1)
                        .frame(maxWidth: .infinity)
                }
                
                HStack {
                    Text("Aspect")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    TextField("", value: $aspectRatio, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 80)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    
                    Slider(value: $aspectRatio, in: 0.1...10, step: 0.01)
                        .frame(maxWidth: .infinity)
                }
            }
        }
    }
}
            
// MARK: - Spline Editor (TODO-1767)
struct SplinePropertyEditor: View {
    let label: String
    @Binding var controlPoints: [SIMD3<Float>]
    @State private var showAdvanced = false
    @State private var selectedPointIndex: Int? = nil
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text("\(controlPoints.count) points")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Button("Add Point") {
                    controlPoints.append(SIMD3<Float>(0, 0, 0))
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            if showAdvanced {
                VStack(spacing: 2) {
                    ForEach(0..<controlPoints.count, id: \.self) { index in
                        HStack {
                            Button("\(index)") {
                                selectedPointIndex = selectedPointIndex == index ? nil : index
                            }
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(selectedPointIndex == index ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .buttonStyle(.plain)
                            .frame(width: 30)
                            
                            Vector3PropertyEditor(
                                label: "Point \(index)",
                                value: Binding(
                                    get: { controlPoints[index] },
                                    set: { controlPoints[index] = $0 }
                                )
                            )
                            
                            Button(action: {
                                if controlPoints.count > 2 {
                                    controlPoints.remove(at: index)
                                    selectedPointIndex = nil
                                }
                            }) {
                                Image(systemName: "minus.circle.fill")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
            }
        }
    }
}

// MARK: - Path Editor (TODO-1768)
struct PathPropertyEditor: View {
    let label: String
    @Binding var waypoints: [PathWaypoint]
    @State private var showAdvanced = false
    
    struct PathWaypoint: Identifiable {
        let id = UUID()
        var position: SIMD3<Float>
        var rotation: SIMD3<Float>
        var waitTime: Float
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text("\(waypoints.count) waypoints")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Button("Add Waypoint") {
                    waypoints.append(PathWaypoint(
                        position: SIMD3<Float>(0, 0, 0),
                        rotation: SIMD3<Float>(0, 0, 0),
                        waitTime: 0
                    ))
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            if showAdvanced {
                VStack(spacing: 4) {
                    ForEach($waypoints) { $waypoint in
                        VStack(alignment: .leading, spacing: 2) {
                            HStack {
                                Text("Waypoint")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                
                                Spacer()
                                
                                Button(action: {
                                    if let index = waypoints.firstIndex(where: { $0.id == waypoint.id }) {
                                        waypoints.remove(at: index)
                                    }
                                }) {
                                    Image(systemName: "minus.circle.fill")
                                        .foregroundColor(.red)
                                }
                                .buttonStyle(.plain)
                            }
                            
                            Vector3PropertyEditor(label: "Position", value: $waypoint.position)
                            Vector3PropertyEditor(label: "Rotation", value: $waypoint.rotation)
                            
                            HStack {
                                Text("Wait Time")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                    .frame(width: 80, alignment: .leading)
                                
                                TextField("", value: $waypoint.waitTime, format: .number.precision(.fractionLength(2)))
                                    .textFieldStyle(.plain)
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                    .frame(width: 80)
                                    .padding(8)
                                    .background(DesignSystem.Colors.backgroundTertiary)
                                    .cornerRadius(4)
                            }
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                }
            }
        }
    }
}

// MARK: - Polygon/Shape Editor (TODO-1769)
struct PolygonPropertyEditor: View {
    let label: String
    @Binding var vertices: [SIMD2<Float>]
    @State private var showAdvanced = false
    @State private var selectedVertexIndex: Int? = nil
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text("\(vertices.count) vertices")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Button("Add Vertex") {
                    vertices.append(SIMD2<Float>(0, 0))
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            if showAdvanced {
                VStack(spacing: 2) {
                    ForEach(0..<vertices.count, id: \.self) { index in
                        HStack {
                            Button("\(index)") {
                                selectedVertexIndex = selectedVertexIndex == index ? nil : index
                            }
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(selectedVertexIndex == index ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .buttonStyle(.plain)
                            .frame(width: 30)
                            
                            HStack(spacing: 8) {
                                HStack {
                                    Text("X:")
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(DesignSystem.Colors.xAxis)
                                        .frame(width: 20)
                                    
                                    TextField("", value: $vertices[index].x, format: .number.precision(.fractionLength(3)))
                                        .textFieldStyle(.plain)
                                        .font(DesignSystem.Typography.mono)
                                        .foregroundColor(DesignSystem.Colors.textPrimary)
                                        .frame(width: 60)
                                        .padding(4)
                                        .background(DesignSystem.Colors.backgroundTertiary)
                                        .cornerRadius(3)
                                }
                                
                                HStack {
                                    Text("Y:")
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(DesignSystem.Colors.yAxis)
                                        .frame(width: 20)
                                    
                                    TextField("", value: $vertices[index].y, format: .number.precision(.fractionLength(3)))
                                        .textFieldStyle(.plain)
                                        .font(DesignSystem.Typography.mono)
                                        .foregroundColor(DesignSystem.Colors.textPrimary)
                                        .frame(width: 60)
                                        .padding(4)
                                        .background(DesignSystem.Colors.backgroundTertiary)
                                        .cornerRadius(3)
                                }
                            }
                            
                            Button(action: {
                                if vertices.count > 3 {
                                    vertices.remove(at: index)
                                    selectedVertexIndex = nil
                                }
                            }) {
                                Image(systemName: "minus.circle.fill")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
            }
        }
    }
}

// MARK: - Mesh Preview/Editor (TODO-1770)
struct MeshPropertyEditor: View {
    let label: String
    @Binding var meshPath: String
    @State private var showPreview = false
    @State private var meshInfo: MeshInfo? = nil
    
    struct MeshInfo {
        let vertexCount: Int
        let triangleCount: Int
        let boundingBox: (min: SIMD3<Float>, max: SIMD3<Float>)
        let hasUVs: Bool
        let hasNormals: Bool
        let hasTangents: Bool
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button("Preview") {
                    showPreview.toggle()
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { loadMeshInfo() }) {
                    Image(systemName: "info.circle")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            HStack {
                Image(systemName: "cube")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                
                Text(meshPath.isEmpty ? "No mesh selected" : URL(fileURLWithPath: meshPath).lastPathComponent)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(meshPath.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Button("Browse") {
                    // File browser implementation would go here
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                if !meshPath.isEmpty {
                    Button("Clear") {
                        meshPath = ""
                        meshInfo = nil
                    }
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(6)
            
            if let meshInfo = meshInfo {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Mesh Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Vertices: \(meshInfo.vertexCount)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("Triangles: \(meshInfo.triangleCount)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    HStack {
                        Text("UVs: \(meshInfo.hasUVs ? "Yes" : "No")")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(meshInfo.hasUVs ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textTertiary)
                        
                        Text("Normals: \(meshInfo.hasNormals ? "Yes" : "No")")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(meshInfo.hasNormals ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textTertiary)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            
            if showPreview {
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundTertiary)
                    .frame(height: 200)
                    .overlay(
                        VStack {
                            Image(systemName: "cube")
                                .font(.system(size: 40))
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                            Text("3D Mesh Preview")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                    )
                    .cornerRadius(6)
            }
        }
    }
    
// MARK: - Texture Preview with Channels (TODO-1771)
struct TexturePropertyEditor: View {
    let label: String
    @Binding var texturePath: String
    @State private var showChannels = false
    @State private var selectedChannel: TextureChannel = .rgb
    @State private var textureInfo: TextureInfo? = nil
    
    enum TextureChannel: String, CaseIterable {
        case rgb = "RGB"
        case red = "Red"
        case green = "Green"
        case blue = "Blue"
        case alpha = "Alpha"
        case normal = "Normal"
        case roughness = "Roughness"
        case metallic = "Metallic"
        case ao = "AO"
    }
    
    struct TextureInfo {
        let width: Int
        let height: Int
        let channels: Int
        let format: String
        let size: Int64 // in bytes
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button("Channels") {
                    showChannels.toggle()
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { loadTextureInfo() }) {
                    Image(systemName: "info.circle")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            HStack {
                Image(systemName: "photo")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                
                Text(texturePath.isEmpty ? "No texture selected" : URL(fileURLWithPath: texturePath).lastPathComponent)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(texturePath.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Button("Browse") {
                    // File browser implementation would go here
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                if !texturePath.isEmpty {
                    Button("Clear") {
                        texturePath = ""
                        textureInfo = nil
                    }
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(6)
            
            if showChannels {
                HStack {
                    Text("Channel:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Picker("", selection: $selectedChannel) {
                        ForEach(TextureChannel.allCases, id: \.self) { channel in
                            Text(channel.rawValue).tag(channel)
                        }
                    }
                    .pickerStyle(.menu)
                    .font(DesignSystem.Typography.small)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            
            // Texture preview
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 120)
                .overlay(
                    VStack {
                        Image(systemName: "photo")
                            .font(.system(size: 30))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        if !texturePath.isEmpty {
                            Text(selectedChannel.rawValue)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                    }
                )
                .cornerRadius(4)
            
            if let textureInfo = textureInfo {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Texture Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Size: \(textureInfo.width)×\(textureInfo.height)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("Channels: \(textureInfo.channels)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Text("Format: \(textureInfo.format)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("File Size: \(ByteCountFormatter.string(fromByteCount: textureInfo.size, countStyle: .file))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    private func loadTextureInfo() {
        // Mock texture info - in real implementation, would load actual texture data
        textureInfo = TextureInfo(
            width: 1024,
            height: 1024,
            channels: 4,
            format: "RGBA8",
            size: 4 * 1024 * 1024
        )
    }
}

// MARK: - Audio Waveform Preview (TODO-1772)
struct AudioPropertyEditor: View {
    let label: String
    @Binding var audioPath: String
    @State private var isPlaying = false
    @State private var currentTime: Float = 0
    @State private var duration: Float = 0
    @State private var audioInfo: AudioInfo? = nil
    
    struct AudioInfo {
        let sampleRate: Int
        let channels: Int
        let bitDepth: Int
        let duration: Float // in seconds
        let format: String
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button(action: { loadAudioInfo() }) {
                    Image(systemName: "info.circle")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            HStack {
                Image(systemName: "speaker.wave.2")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                
                Text(audioPath.isEmpty ? "No audio selected" : URL(fileURLWithPath: audioPath).lastPathComponent)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(audioPath.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Button("Browse") {
                    // File browser implementation would go here
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                if !audioPath.isEmpty {
                    Button("Clear") {
                        audioPath = ""
                        audioInfo = nil
                    }
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(6)
            
            // Waveform visualization
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 80)
                .overlay(
                    ZStack {
                        // Waveform (mock)
                        Path { path in
                            let width: CGFloat = 300
                            let height: CGFloat = 80
                            
                            path.move(to: CGPoint(x: 0, y: height / 2))
                            
                            for i in 0..<50 {
                                let x = CGFloat(i) * width / 50
                                let y = height / 2 + CGFloat.random(in: -30...30)
                                path.addLine(to: CGPoint(x: x, y: y))
                            }
                        }
                        .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 1)
                        
                        // Playhead
                        if isPlaying {
                            Rectangle()
                                .fill(DesignSystem.Colors.accentDanger)
                                .frame(width: 2, height: 80)
                                .offset(x: CGFloat(currentTime / duration) * 300 - 150)
                        }
                    }
                )
                .cornerRadius(4)
            
            // Playback controls
            HStack {
                Button(action: { 
                    isPlaying.toggle()
                }) {
                    Image(systemName: isPlaying ? "pause.circle.fill" : "play.circle.fill")
                        .font(.system(size: 24))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                
                Slider(value: $currentTime, in: 0...max(duration, 1))
                    .frame(maxWidth: .infinity)
                
                Text(formatTime(currentTime))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            if let audioInfo = audioInfo {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Audio Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Sample Rate: \(audioInfo.sampleRate) Hz")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("Channels: \(audioInfo.channels)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Text("Format: \(audioInfo.format)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("Duration: \(formatTime(audioInfo.duration))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    private func loadAudioInfo() {
        // Mock audio info - in real implementation, would load actual audio data
        audioInfo = AudioInfo(
            sampleRate: 44100,
            channels: 2,
            bitDepth: 16,
            duration: 180.0,
            format: "WAV"
        )
        duration = audioInfo?.duration ?? 0
    }
    
    private func formatTime(_ time: Float) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        return String(format: "%02d:%02d", minutes, seconds)
    }
}

// MARK: - Animation Preview Player (TODO-1773)
struct AnimationPropertyEditor: View {
    let label: String
    @Binding var animationPath: String
    @State private var isPlaying = false
    @State private var currentTime: Float = 0
    @State private var duration: Float = 0
    @State private var animationInfo: AnimationInfo? = nil
    
    struct AnimationInfo {
        let frameCount: Int
        let frameRate: Float
        let duration: Float // in seconds
        let loopMode: String
        let boneCount: Int
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button(action: { loadAnimationInfo() }) {
                    Image(systemName: "info.circle")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            HStack {
                Image(systemName: "figure.walk")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                
                Text(animationPath.isEmpty ? "No animation selected" : URL(fileURLWithPath: animationPath).lastPathComponent)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(animationPath.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Button("Browse") {
                    // File browser implementation would go here
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                if !animationPath.isEmpty {
                    Button("Clear") {
                        animationPath = ""
                        animationInfo = nil
                    }
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(6)
            
            // Animation preview
            Rectangle()
                .fill(DesignSystem.Colors.backgroundTertiary)
                .frame(height: 120)
                .overlay(
                    VStack {
                        Image(systemName: "figure.walk")
                            .font(.system(size: 40))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        if !animationPath.isEmpty {
                            Text("Frame \(Int(currentTime * 30)) / \(animationInfo?.frameCount ?? 0)")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                    }
                )
                .cornerRadius(4)
            
            // Playback controls
            HStack {
                Button(action: { 
                    isPlaying.toggle()
                }) {
                    Image(systemName: isPlaying ? "pause.circle.fill" : "play.circle.fill")
                        .font(.system(size: 24))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                
                Slider(value: $currentTime, in: 0...max(duration, 1))
                    .frame(maxWidth: .infinity)
                
                Text(formatTime(currentTime))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            if let animationInfo = animationInfo {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Animation Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Frames: \(animationInfo.frameCount)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("FPS: \(animationInfo.frameRate)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    HStack {
                        Text("Bones: \(animationInfo.boneCount)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("Loop: \(animationInfo.loopMode)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    private func loadAnimationInfo() {
        // Mock animation info - in real implementation, would load actual animation data
        animationInfo = AnimationInfo(
            frameCount: 120,
            frameRate: 30,
            duration: 4.0,
            loopMode: "Loop",
            boneCount: 65
        )
        duration = animationInfo?.duration ?? 0
    }
    
// MARK: - Script Syntax Highlighted Editor (TODO-1774)
struct ScriptPropertyEditor: View {
    let label: String
    @Binding var script: String
    @State private var showAdvanced = false
    @State private var selectedLanguage: ScriptLanguage = .lua
    
    enum ScriptLanguage: String, CaseIterable {
        case lua = "Lua"
        case python = "Python"
        case javascript = "JavaScript"
        case csharp = "C#"
        case cpp = "C++"
        case glsl = "GLSL"
        case hlsl = "HLSL"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("", selection: $selectedLanguage) {
                    ForEach(ScriptLanguage.allCases, id: \.self) { language in
                        Text(language.rawValue).tag(language)
                    }
                }
                .pickerStyle(.menu)
                .font(DesignSystem.Typography.small)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            TextEditor(text: $script)
                .font(DesignSystem.Typography.mono)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .frame(minHeight: 120)
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(6)
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                )
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Script Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Lines: \(script.components(separatedBy: .newlines).count)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("Chars: \(script.count)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Text("Language: \(selectedLanguage.rawValue)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
}

// MARK: - JSON/Data Structure Editor (TODO-1775)
struct JSONPropertyEditor: View {
    let label: String
    @Binding var jsonString: String
    @State private var showAdvanced = false
    @State private var isValidJSON = true
    @State private var errorMessage = ""
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button("Format") {
                    formatJSON()
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                .disabled(!isValidJSON)
                
                Button("Validate") {
                    validateJSON()
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            TextEditor(text: $jsonString)
                .font(DesignSystem.Typography.mono)
                .foregroundColor(isValidJSON ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.accentDanger)
                .frame(minHeight: 100)
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(6)
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(isValidJSON ? DesignSystem.Colors.border : DesignSystem.Colors.accentDanger, lineWidth: 1)
                )
                .onChange(of: jsonString) { _ in
                    validateJSON()
                }
            
            if !isValidJSON && !errorMessage.isEmpty {
                HStack {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(DesignSystem.Colors.accentDanger)
                    
                    Text(errorMessage)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentDanger)
                }
                .padding(8)
                .background(DesignSystem.Colors.pastelRed)
                .cornerRadius(4)
            }
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 2) {
                    Text("JSON Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Valid: \(isValidJSON ? "Yes" : "No")")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(isValidJSON ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentDanger)
                        
                        Text("Size: \(jsonString.count) chars")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    if isValidJSON {
                        Text("Structure: Valid JSON")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.accentSuccess)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    private func validateJSON() {
        guard !jsonString.isEmpty else {
            isValidJSON = true
            errorMessage = ""
            return
        }
        
        do {
            _ = try JSONSerialization.jsonObject(with: jsonString.data(using: .utf8) ?? Data())
            isValidJSON = true
            errorMessage = ""
        } catch {
            isValidJSON = false
            errorMessage = error.localizedDescription
        }
    }
    
    private func formatJSON() {
        guard isValidJSON, !jsonString.isEmpty else { return }
        
        do {
            let jsonObject = try JSONSerialization.jsonObject(with: jsonString.data(using: .utf8) ?? Data())
            let formattedData = try JSONSerialization.data(withJSONObject: jsonObject, options: [.prettyPrinted, .sortedKeys])
            jsonString = String(data: formattedData, encoding: .utf8) ?? jsonString
        } catch {
            // Keep original if formatting fails
        }
    }
}

// MARK: - File Path Picker with Validation (TODO-1776)
struct FilePathPropertyEditor: View {
    let label: String
    @Binding var filePath: String
    let allowedExtensions: [String]
    @State private var showAdvanced = false
    @State private var fileExists = false
    @State private var isValidPath = true
    
    init(label: String, filePath: Binding<String>, allowedExtensions: [String] = []) {
        self.label = label
        self._filePath = filePath
        self.allowedExtensions = allowedExtensions
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            HStack {
                Image(systemName: "doc")
                    .foregroundColor(fileExists ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textTertiary)
                
                TextField("Enter file path...", text: $filePath)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(isValidPath ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.accentDanger)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(6)
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(isValidPath ? DesignSystem.Colors.border : DesignSystem.Colors.accentDanger, lineWidth: 1)
                    )
                
                Button("Browse") {
                    // File browser implementation would go here
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                if !filePath.isEmpty {
                    Button("Clear") {
                        filePath = ""
                    }
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
                    .buttonStyle(.plain)
                }
            }
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 2) {
                    Text("File Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Exists: \(fileExists ? "Yes" : "No")")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(fileExists ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textTertiary)
                        
                        Text("Valid: \(isValidPath ? "Yes" : "No")")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(isValidPath ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentDanger)
                    }
                    
                    if !allowedExtensions.isEmpty {
                        Text("Extensions: \(allowedExtensions.joined(separator: ", "))")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    if !filePath.isEmpty {
                        let url = URL(fileURLWithPath: filePath)
                        Text("Name: \(url.lastPathComponent)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
        .onChange(of: filePath) { _ in
            validatePath()
        }
        .onAppear {
            validatePath()
        }
    }
    
    private func validatePath() {
        guard !filePath.isEmpty else {
            fileExists = false
            isValidPath = true
            return
        }
        
        fileExists = FileManager.default.fileExists(atPath: filePath)
        
        // Check file extension if restrictions are specified
        if !allowedExtensions.isEmpty {
            let fileExtension = URL(fileURLWithPath: filePath).pathExtension.lowercased()
            isValidPath = allowedExtensions.contains(fileExtension)
        } else {
            isValidPath = true
        }
    }
}

// MARK: - URL Input with Validation (TODO-1777)
struct URLPropertyEditor: View {
    let label: String
    @Binding var url: String
    @State private var showAdvanced = false
    @State private var isValidURL = true
    @State private var urlComponents: URLComponents? = nil
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            HStack {
                Image(systemName: "link")
                    .foregroundColor(isValidURL ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textTertiary)
                
                TextField("Enter URL...", text: $url)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(isValidURL ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.accentDanger)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(6)
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(isValidURL ? DesignSystem.Colors.border : DesignSystem.Colors.accentDanger, lineWidth: 1)
                    )
                    .onChange(of: url) { _ in
                        validateURL()
                    }
                
                if !url.isEmpty {
                    Button("Clear") {
                        url = ""
                    }
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
                    .buttonStyle(.plain)
                }
            }
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 2) {
                    Text("URL Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Valid: \(isValidURL ? "Yes" : "No")")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(isValidURL ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.accentDanger)
                        
                        Text("Length: \(url.count)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    if let components = urlComponents {
                        VStack(alignment: .leading, spacing: 1) {
                            if let scheme = components.scheme {
                                Text("Scheme: \(scheme)")
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                            
                            if let host = components.host {
                                Text("Host: \(host)")
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                            
                            if let port = components.port {
                                Text("Port: \(port)")
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                        }
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
        .onAppear {
            validateURL()
        }
    }
    
// MARK: - Date/Time Picker (TODO-1778)
struct DateTimePropertyEditor: View {
    let label: String
    @Binding var date: Date
    @State private var showDatePicker = false
    @State private var selectedFormat: DateTimeFormat = .dateTime
    
    enum DateTimeFormat: String, CaseIterable {
        case date = "Date"
        case time = "Time"
        case dateTime = "Date & Time"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("", selection: $selectedFormat) {
                    ForEach(DateTimeFormat.allCases, id: \.self) { format in
                        Text(format.rawValue).tag(format)
                    }
                }
                .pickerStyle(.menu)
                .font(DesignSystem.Typography.small)
            }
            
            HStack {
                Button(action: { showDatePicker.toggle() }) {
                    HStack {
                        Image(systemName: "calendar")
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                        
                        Text(formattedDate)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                    }
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(6)
                }
                .buttonStyle(.plain)
                
                Spacer()
                
                Button("Now") {
                    date = Date()
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .buttonStyle(.plain)
                
                Button("Clear") {
                    date = Date.distantPast
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentDanger)
                .buttonStyle(.plain)
            }
            
            if showDatePicker {
                VStack {
                    switch selectedFormat {
                    case .date:
                        DatePicker("", selection: $date, displayedComponents: .date)
                            .datePickerStyle(.graphical)
                    case .time:
                        DatePicker("", selection: $date, displayedComponents: .hourAndMinute)
                            .datePickerStyle(.graphical)
                    case .dateTime:
                        DatePicker("", selection: $date)
                            .datePickerStyle(.graphical)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
                .shadow(radius: 4)
            }
        }
    }
    
    private var formattedDate: String {
        let formatter = DateFormatter()
        
        switch selectedFormat {
        case .date:
            formatter.dateStyle = .medium
            formatter.timeStyle = .none
        case .time:
            formatter.dateStyle = .none
            formatter.timeStyle = .medium
        case .dateTime:
            formatter.dateStyle = .medium
            formatter.timeStyle = .short
        }
        
        return formatter.string(from: date)
    }
}

// MARK: - Duration Editor (TODO-1779)
struct DurationPropertyEditor: View {
    let label: String
    @Binding var duration: TimeInterval // in seconds
    @State private var showAdvanced = false
    @State private var inputMode: DurationInputMode = .seconds
    
    enum DurationInputMode: String, CaseIterable {
        case seconds = "Seconds"
        case minutes = "Minutes"
        case hours = "Hours"
        case hms = "H:M:S"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("", selection: $inputMode) {
                    ForEach(DurationInputMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.menu)
                .font(DesignSystem.Typography.small)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            HStack {
                switch inputMode {
                case .seconds:
                    TextField("", value: $duration, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 100)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(6)
                    
                    Text("seconds")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                case .minutes:
                    TextField("", value: Binding(
                        get: { duration / 60 },
                        set: { duration = $0 * 60 }
                    ), format: .number.precision(.fractionLength(2)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 100)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(6)
                    
                    Text("minutes")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                case .hours:
                    TextField("", value: Binding(
                        get: { duration / 3600 },
                        set: { duration = $0 * 3600 }
                    ), format: .number.precision(.fractionLength(2)))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 100)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(6)
                    
                    Text("hours")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                case .hms:
                    HStack(spacing: 4) {
                        TextField("", value: Binding(
                            get: { Int(duration) / 3600 },
                            set: { duration = Double($0) * 3600 + (duration.truncatingRemainder(dividingBy: 3600)) }
                        ), format: .number)
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 60)
                            .padding(4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        
                        Text("h")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("", value: Binding(
                            get: { (Int(duration) % 3600) / 60 },
                            set: { duration = Double($0) * 60 + Double(Int(duration) / 3600) * 3600 + (duration.truncatingRemainder(dividingBy: 60)) }
                        ), format: .number)
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 60)
                            .padding(4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        
                        Text("m")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("", value: Binding(
                            get: { Int(duration) % 60 },
                            set: { duration = Double($0) + Double(Int(duration) / 60) * 60 + Double(Int(duration) / 3600) * 3600 }
                        ), format: .number)
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 60)
                            .padding(4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        
                        Text("s")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
                
                Spacer()
            }
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Duration Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Total: \(String(format: "%.3f", duration))s")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Text("Formatted: \(formattedDuration)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
    }
    
    private var formattedDuration: String {
        let hours = Int(duration) / 3600
        let minutes = (Int(duration) % 3600) / 60
        let seconds = Int(duration) % 60
        let milliseconds = Int((duration.truncatingRemainder(dividingBy: 1)) * 1000)
        
        if hours > 0 {
            return String(format: "%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds)
        } else if minutes > 0 {
            return String(format: "%02d:%02d.%03d", minutes, seconds, milliseconds)
        } else {
            return String(format: "%02d.%03d", seconds, milliseconds)
        }
    }
}

// MARK: - Localized String Editor (TODO-1780)
struct LocalizedStringPropertyEditor: View {
    let label: String
    @Binding var key: String
    @Binding var defaultValue: String
    @State private var showAdvanced = false
    @State private var selectedLanguage: String = "en"
    @State private var availableLanguages = ["en", "es", "fr", "de", "ja", "zh", "ru"]
    @State private var localizedValues: [String: String] = [:]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("", selection: $selectedLanguage) {
                    ForEach(availableLanguages, id: \.self) { language in
                        Text(language.uppercased()).tag(language)
                    }
                }
                .pickerStyle(.menu)
                .font(DesignSystem.Typography.small)
                
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            VStack(spacing: 4) {
                HStack {
                    Text("Key:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 60, alignment: .leading)
                    
                    TextField("Enter localization key...", text: $key)
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(6)
                }
                
                HStack {
                    Text("Default:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 60, alignment: .leading)
                    
                    TextField("Enter default value...", text: $defaultValue)
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(6)
                }
                
                HStack {
                    Text("\(selectedLanguage.uppercased()):")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 60, alignment: .leading)
                    
                    TextField("Enter localized value...", text: Binding(
                        get: { localizedValues[selectedLanguage] ?? "" },
                        set: { localizedValues[selectedLanguage] = $0 }
                    ))
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(6)
                }
            }
            
            if showAdvanced {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Localization Info:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack {
                        Text("Languages: \(localizedValues.count)/\(availableLanguages.count)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("Missing: \(availableLanguages.count - localizedValues.count)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(availableLanguages.count > localizedValues.count ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.accentSuccess)
                    }
                    
                    LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 3), spacing: 4) {
                        ForEach(availableLanguages, id: \.self) { language in
                            HStack {
                                Text(language.uppercased())
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                
                                if localizedValues[language] != nil {
                                    Image(systemName: "checkmark.circle.fill")
                                        .foregroundColor(DesignSystem.Colors.accentSuccess)
                                        .font(.system(size: 12))
                                } else {
                                    Image(systemName: "circle")
                                        .foregroundColor(DesignSystem.Colors.textTertiary)
                                        .font(.system(size: 12))
                                }
                            }
                            .padding(4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(3)
                        }
                    }
                    
                    if !key.isEmpty {
                        Text("Preview: \(localizedValues[selectedLanguage] ?? defaultValue)")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .padding(4)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(3)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
        }
        .onChange(of: selectedLanguage) { _ in
            // Initialize empty value for new language if needed
            if localizedValues[selectedLanguage] == nil {
                localizedValues[selectedLanguage] = ""
            }
        }
        .onAppear {
            // Initialize with default value for current language
            if localizedValues[selectedLanguage] == nil {
                localizedValues[selectedLanguage] = defaultValue
            }
        }
    }
}
