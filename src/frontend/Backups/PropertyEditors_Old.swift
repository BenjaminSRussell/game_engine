import SwiftUI

// 
// MARK: - CATEGORY 3: INSPECTOR & PROPERTY SYSTEM (TODO-1461 to TODO-1800)
// 

// MARK: - Property Editor Protocol (TODO-1461 to TODO-1500)
protocol PropertyEditor: View {
    associatedtype Value
    var value: Binding<Value> { get }
    var label: String { get }
    
    // TODO-1461: Implement property editor base protocol
    // TODO-1462: Add undo/redo integration for all property changes
    // TODO-1463: Add copy/paste support for property values
    // TODO-1464: Add property reset to default button
    // TODO-1465: Add property locking mechanism
    // TODO-1466: Add property animation keyframe button
    // TODO-1467: Add property expression/formula input
    // TODO-1468: Add property linking between objects
    // TODO-1469: Add property override indication
    // TODO-1470: Add property tooltip/help text
    // TODO-1471: Add property unit display (meters, degrees, etc.)
    // TODO-1472: Add property min/max clamping with warnings
    // TODO-1473: Add property step/increment customization
    // TODO-1474: Add property precision customization
    // TODO-1475: Add property color coding by type
    // TODO-1476: Add property search/filter in inspector
    // TODO-1477: Add property grouping with collapsible sections
    // TODO-1478: Add property favorites/pinning
    // TODO-1479: Add property comparison between objects
    // TODO-1480: Add property batch editing for multi-selection
    // TODO-1481: Add property scripting hooks
    // TODO-1482: Add property validation with error display
    // TODO-1483: Add property conditional visibility
    // TODO-1484: Add property dependencies/relationships
    // TODO-1485: Add property presets/saved values
    // TODO-1486: Add property history/recent values
    // TODO-1487: Add property random value generation
    // TODO-1488: Add property math operations (add, multiply, etc.)
    // TODO-1489: Add property value snapping
    // TODO-1490: Add property value rounding
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

// MARK: - Float Editor (TODO-1461)
struct FloatPropertyEditor: View {
    let label: String
    @Binding var value: Float
    var range: ClosedRange<Float> = 0...100
    var step: Float = 0.1
    @State private var isDragging = false
    @State private var dragStartValue: Float = 0
    
    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            HStack(spacing: 4) {
                // Draggable value field
                Text(String(format: "%.2f", value))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(isDragging ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                    .frame(width: 60)
                    .gesture(
                        DragGesture()
                            .onChanged { gesture in
                                if !isDragging {
                                    isDragging = true
                                    dragStartValue = value
                                }
                                let delta = Float(gesture.translation.width) * step * 0.1
                                value = min(max(dragStartValue + delta, range.lowerBound), range.upperBound)
                            }
                            .onEnded { _ in
                                isDragging = false
                            }
                    )
                
                // Slider
                Slider(value: $value, in: range, step: step)
                    .frame(maxWidth: .infinity)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
        }
    }
}

// MARK: - Integer Editor (TODO-1462)
struct IntPropertyEditor: View {
    let label: String
    @Binding var value: Int
    var range: ClosedRange<Int> = 0...100
    
    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .frame(width: 80, alignment: .leading)
            
            Stepper(value: $value, in: range) {
                Text("\(value)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(width: 50, alignment: .trailing)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
        }
    }
}

// MARK: - Boolean Editor (TODO-1463)
struct BoolPropertyEditor: View {
    let label: String
    @Binding var value: Bool
    
    var body: some View {
        Toggle(isOn: $value) {
            Text(label)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
        }
        .toggleStyle(.checkbox)
    }
}

// MARK: - String Editor (TODO-1464)
struct StringPropertyEditor: View {
    let label: String
    @Binding var value: String
    var isMultiline: Bool = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            if isMultiline {
                TextEditor(text: $value)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(minHeight: 80)
                    .padding(4)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
            } else {
                TextField("", text: $value)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
            }
        }
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
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
}

// MARK: - Color Editor (TODO-1470)
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

// MARK: - Curve Editor (TODO-1472)
struct CurvePropertyEditor: View {
    let label: String
    @Binding var points: [CGPoint]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            GeometryReader { geometry in
                ZStack {
                    // Grid
                    Path { path in
                        for i in 0...4 {
                            let x = geometry.size.width * CGFloat(i) / 4
                            path.move(to: CGPoint(x: x, y: 0))
                            path.addLine(to: CGPoint(x: x, y: geometry.size.height))
                            
                            let y = geometry.size.height * CGFloat(i) / 4
                            path.move(to: CGPoint(x: 0, y: y))
                            path.addLine(to: CGPoint(x: geometry.size.width, y: y))
                        }
                    }
                    .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 1)
                    
                    // Curve
                    Path { path in
                        guard !points.isEmpty else { return }
                        let sortedPoints = points.sorted { $0.x < $1.x }
                        path.move(to: CGPoint(
                            x: sortedPoints[0].x * geometry.size.width,
                            y: (1 - sortedPoints[0].y) * geometry.size.height
                        ))
                        for point in sortedPoints.dropFirst() {
                            path.addLine(to: CGPoint(
                                x: point.x * geometry.size.width,
                                y: (1 - point.y) * geometry.size.height
                            ))
                        }
                    }
                    .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                    
                    // Control points
                    ForEach(0..<points.count, id: \.self) { index in
                        Circle()
                            .fill(DesignSystem.Colors.accentPrimary)
                            .frame(width: 10, height: 10)
                            .position(
                                x: points[index].x * geometry.size.width,
                                y: (1 - points[index].y) * geometry.size.height
                            )
                            .gesture(
                                DragGesture()
                                    .onChanged { value in
                                        points[index] = CGPoint(
                                            x: max(0, min(1, value.location.x / geometry.size.width)),
                                            y: max(0, min(1, 1 - value.location.y / geometry.size.height))
                                        )
                                    }
                            )
                    }
                }
            }
            .frame(height: 120)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(4)
        }
    }
}
