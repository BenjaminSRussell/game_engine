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

// MARK: - Supporting Data Models

// Color Space Types
enum ColorSpace: String, CaseIterable {
    case sRGB = "sRGB"
    case linear = "Linear"
    case hsv = "HSV"
    
    var description: String {
        switch self {
        case .sRGB: return "Standard RGB"
        case .linear: return "Linear RGB"
        case .hsv: return "Hue, Saturation, Value"
        }
    }
}

// Color Blindness Types
enum ColorBlindnessType: String, CaseIterable {
    case none = "Normal"
    case protanopia = "Protanopia (Red-Blind)"
    case deuteranopia = "Deuteranopia (Green-Blind)"
    case tritanopia = "Tritanopia (Blue-Blind)"
    case achromatopsia = "Achromatopsia (Monochrome)"
}

// Color Harmony Types
enum ColorHarmony: String, CaseIterable {
    case complementary = "Complementary"
    case analogous = "Analogous"
    case triadic = "Triadic"
    case tetradic = "Tetradic"
    case splitComplementary = "Split-Complementary"
    case monochromatic = "Monochromatic"
}

// Color History Entry
struct ColorHistoryEntry: Identifiable {
    let id = UUID()
    let color: Color
    let timestamp: Date
    let name: String?
    
    init(color: Color, name: String? = nil) {
        self.color = color
        self.timestamp = Date()
        self.name = name
    }
}

// Color Swatch
struct ColorSwatch: Identifiable {
    let id = UUID()
    let name: String
    let color: Color
    let category: String
    
    static let defaultSwatches: [ColorSwatch] = [
        .init(name: "Red", color: .red, category: "Primary"),
        .init(name: "Green", color: .green, category: "Primary"),
        .init(name: "Blue", color: .blue, category: "Primary"),
        .init(name: "Yellow", color: .yellow, category: "Primary"),
        .init(name: "Orange", color: .orange, category: "Secondary"),
        .init(name: "Purple", color: .purple, category: "Secondary"),
        .init(name: "Pink", color: .pink, category: "Secondary"),
        .init(name: "Cyan", color: .cyan, category: "Secondary"),
        .init(name: "White", color: .white, category: "Neutral"),
        .init(name: "Black", color: .black, category: "Neutral"),
        .init(name: "Gray", color: .gray, category: "Neutral"),
        .init(name: "Brown", color: .brown, category: "Earth")
    ]
}

// Object Reference Types
enum ObjectType: String, CaseIterable {
    case gameObject = "Game Object"
    case component = "Component"
    case material = "Material"
    case texture = "Texture"
    case mesh = "Mesh"
    case audio = "Audio"
    case animation = "Animation"
    case prefab = "Prefab"
    case script = "Script"
    
    var icon: String {
        switch self {
        case .gameObject: return "cube"
        case .component: return "puzzlepiece"
        case .material: return "paintpalette"
        case .texture: return "photo"
        case .mesh: return "cube.transparent"
        case .audio: return "speaker.wave.2"
        case .animation: return "figure.walk"
        case .prefab: return "cube.box"
        case .script: return "doc.text"
        }
    }
}

// Scene Hierarchy Node
struct SceneHierarchyNode: Identifiable {
    let id = UUID()
    let name: String
    let type: ObjectType
    let children: [SceneHierarchyNode]
    let isActive: Bool
    let isPrefab: Bool
    let reference: String?
    
    static func mockHierarchy() -> [SceneHierarchyNode] {
        return [
            SceneHierarchyNode(name: "Root", type: .gameObject, children: [
                SceneHierarchyNode(name: "Player", type: .gameObject, children: [
                    SceneHierarchyNode(name: "Mesh Renderer", type: .component, children: [], isActive: true, isPrefab: false, reference: "player_mesh"),
                    SceneHierarchyNode(name: "Player Controller", type: .component, children: [], isActive: true, isPrefab: false, reference: "player_controller")
                ], isActive: true, isPrefab: true, reference: "player_prefab"),
                SceneHierarchyNode(name: "Camera", type: .gameObject, children: [
                    SceneHierarchyNode(name: "Camera Component", type: .component, children: [], isActive: true, isPrefab: false, reference: "main_camera")
                ], isActive: true, isPrefab: false, reference: nil)
            ], isActive: true, isPrefab: false, reference: nil)
        ]
    }
}

// MARK: - Color Property Editor (TODO-1561 to TODO-1575)
struct EnhancedColorPropertyEditor: View {
    let label: String
    @Binding var value: Color
    @State private var showAdvancedEditor = false
    @State private var selectedColorSpace: ColorSpace = .sRGB
    @State private var hexInput = ""
    @State private var showEyedropper = false
    @State private var alphaEnabled = true
    @State private var exposure: Float = 0.0
    @State private var temperature: Float = 6500
    @State private var tint: Float = 0
    @State private var selectedHarmony: ColorHarmony = .complementary
    @State private var colorHistory: [ColorHistoryEntry] = []
    @State private var customSwatches: [ColorSwatch] = ColorSwatch.defaultSwatches
    @State private var showBatchOperations = false
    @State private var selectedBlindnessType: ColorBlindnessType = .none
    @State private var hdrEnabled = false
    @State private var gradientMode = false
    @State private var gradientEndColor: Color = .blue
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                // HDR Badge
                if hdrEnabled {
                    HStack(spacing: 2) {
                        Image(systemName: "star.fill")
                            .font(.system(size: 8))
                        Text("HDR")
                            .font(DesignSystem.Typography.small)
                    }
                    .foregroundColor(DesignSystem.Colors.accentWarning)
                }
                
                // Advanced Editor Button
                EditorIconButton(icon: "slider.horizontal.3", tooltip: "Advanced Color Editor") {
                    showAdvancedEditor.toggle()
                }
            }
            
            // Main Color Display
            HStack(spacing: 12) {
                // Color Preview
                ZStack {
                    RoundedRectangle(cornerRadius: 8)
                        .fill(value)
                        .frame(width: 60, height: 40)
                        .overlay(
                            RoundedRectangle(cornerRadius: 8)
                                .stroke(DesignSystem.Colors.border, lineWidth: 1)
                        )
                    
                    // Alpha Checkerboard
                    if alphaEnabled {
                        RoundedRectangle(cornerRadius: 8)
                            .fill(
                                LinearGradient(
                                    colors: [Color.white, Color.gray],
                                    startPoint: .topLeading,
                                    endPoint: .bottomTrailing
                                )
                            )
                            .frame(width: 60, height: 40)
                            .opacity(0.3)
                    }
                }
                
                // Quick Actions
                VStack(spacing: 4) {
                    HStack(spacing: 4) {
                        EditorIconButton(icon: "eyedropper", tooltip: "Eyedropper Tool") {
                            showEyedropper.toggle()
                        }
                        
                        EditorIconButton(icon: "paintpalette", tooltip: "Color Swatches") {
                            // Show swatches
                        }
                        
                        EditorIconButton(icon: "clock", tooltip: "Color History") {
                            // Show history
                        }
                    }
                    
                    // Hex Input
                    TextField("#RRGGBB", text: $hexInput)
                        .textFieldStyle(.roundedBorder)
                        .font(DesignSystem.Typography.monoSmall)
                        .frame(width: 100)
                }
                
                Spacer()
                
                // Color Picker
                ColorPicker("", selection: $value)
                    .labelsHidden()
                    .scaleEffect(0.8)
            }
            
            // Gradient Mode Toggle
            if gradientMode {
                HStack {
                    Text("Gradient End:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    ColorPicker("", selection: $gradientEndColor)
                        .labelsHidden()
                        .scaleEffect(0.7)
                }
            }
        }
        .sheet(isPresented: $showAdvancedEditor) {
            AdvancedColorEditor(
                color: $value,
                colorSpace: $selectedColorSpace,
                alphaEnabled: $alphaEnabled,
                exposure: $exposure,
                temperature: $temperature,
                tint: $tint,
                harmony: $selectedHarmony,
                blindnessType: $selectedBlindnessType,
                hdrEnabled: $hdrEnabled,
                gradientMode: $gradientMode,
                gradientEndColor: $gradientEndColor,
                colorHistory: $colorHistory,
                customSwatches: $customSwatches
            )
        }
        .onAppear {
            updateHexInput()
        }
        .onChange(of: value) { _ in
            updateHexInput()
            addToHistory()
        }
        .onChange(of: hexInput) { newValue in
            updateColorFromHex()
        }
    }
    
    private func updateHexInput() {
        // Convert SwiftUI Color to hex
        let uiColor = UIColor(value)
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        var alpha: CGFloat = 0
        
        uiColor.getRed(&red, green: &green, blue: &blue, alpha: &alpha)
        
        if alphaEnabled {
            hexInput = String(format: "#%02lX%02lX%02lX%02lX",
                             lroundf(Float(red * 255)),
                             lroundf(Float(green * 255)),
                             lroundf(Float(blue * 255)),
                             lroundf(Float(alpha * 255)))
        } else {
            hexInput = String(format: "#%02lX%02lX%02lX",
                             lroundf(Float(red * 255)),
                             lroundf(Float(green * 255)),
                             lroundf(Float(blue * 255)))
        }
    }
    
    private func updateColorFromHex() {
        guard hexInput.hasPrefix("#") else { return }
        
        let hexString = String(hexInput.dropFirst())
        guard hexString.count == 6 || hexString.count == 8 else { return }
        
        var rgb: UInt64 = 0
        Scanner(string: hexString).scanHexInt64(&rgb)
        
        if hexString.count == 8 {
            let red = CGFloat((rgb & 0xFF000000) >> 24) / 255
            let green = CGFloat((rgb & 0x00FF0000) >> 16) / 255
            let blue = CGFloat((rgb & 0x0000FF00) >> 8) / 255
            let alpha = CGFloat(rgb & 0x000000FF) / 255
            value = Color(.sRGB, red: red, green: green, blue: blue, opacity: alpha)
        } else {
            let red = CGFloat((rgb & 0xFF0000) >> 16) / 255
            let green = CGFloat((rgb & 0x00FF00) >> 8) / 255
            let blue = CGFloat(rgb & 0x0000FF) / 255
            value = Color(.sRGB, red: red, green: green, blue: blue, opacity: 1.0)
        }
    }
    
    private func addToHistory() {
        let entry = ColorHistoryEntry(color: value, name: generateColorName())
        colorHistory.insert(entry, at: 0)
        if colorHistory.count > 20 {
            colorHistory.removeLast()
        }
    }
    
    private func generateColorName() -> String {
        // Simple color naming based on hue
        let uiColor = UIColor(value)
        var hue: CGFloat = 0
        var saturation: CGFloat = 0
        var brightness: CGFloat = 0
        
        uiColor.getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: nil)
        
        let hueDegrees = hue * 360
        
        switch hueDegrees {
        case 0..<15, 345..<360: return "Red"
        case 15..<45: return "Orange"
        case 45..<75: return "Yellow"
        case 75..<150: return "Green"
        case 150..<210: return "Cyan"
        case 210..<270: return "Blue"
        case 270..<330: return "Purple"
        default: return "Magenta"
        }
    }
}

// MARK: - Advanced Color Editor
struct AdvancedColorEditor: View {
    @Binding var color: Color
    @Binding var colorSpace: ColorSpace
    @Binding var alphaEnabled: Bool
    @Binding var exposure: Float
    @Binding var temperature: Float
    @Binding var tint: Float
    @Binding var harmony: ColorHarmony
    @Binding var blindnessType: ColorBlindnessType
    @Binding var hdrEnabled: Bool
    @Binding var gradientMode: Bool
    @Binding var gradientEndColor: Color
    @Binding var colorHistory: [ColorHistoryEntry]
    @Binding var customSwatches: [ColorSwatch]
    
    @Environment(\.dismiss) private var dismiss
    @State private var selectedTab = 0
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Tab Bar
                Picker("Editor Tab", selection: $selectedTab) {
                    Text("Basic").tag(0)
                    Text("Advanced").tag(1)
                    Text("Swatches").tag(2)
                    Text("Harmony").tag(3)
                    Text("Accessibility").tag(4)
                }
                .pickerStyle(.segmented)
                .padding()
                
                // Tab Content
                TabView(selection: $selectedTab) {
                    BasicColorTab(color: $color, alphaEnabled: $alphaEnabled, colorSpace: $colorSpace)
                        .tag(0)
                    
                    AdvancedColorTab(
                        color: $color,
                        exposure: $exposure,
                        temperature: $temperature,
                        tint: $tint,
                        hdrEnabled: $hdrEnabled,
                        gradientMode: $gradientMode,
                        gradientEndColor: $gradientEndColor
                    )
                    .tag(1)
                    
                    SwatchesTab(
                        color: $color,
                        customSwatches: $customSwatches,
                        colorHistory: $colorHistory
                    )
                    .tag(2)
                    
                    HarmonyTab(color: $color, harmony: $harmony)
                        .tag(3)
                    
                    AccessibilityTab(
                        color: $color,
                        blindnessType: $blindnessType
                    )
                    .tag(4)
                }
                .tabViewStyle(.page(indexDisplayMode: .never))
            }
            .navigationTitle("Advanced Color Editor")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
        }
        .frame(width: 600, height: 500)
    }
}

// MARK: - Basic Color Tab
struct BasicColorTab: View {
    @Binding var color: Color
    @Binding var alphaEnabled: Bool
    @Binding var colorSpace: ColorSpace
    
    var body: some View {
        VStack(spacing: 20) {
            // Color Space Selection
            HStack {
                Text("Color Space:")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Picker("Color Space", selection: $colorSpace) {
                    ForEach(ColorSpace.allCases, id: \.self) { space in
                        Text(space.description).tag(space)
                    }
                }
                .pickerStyle(.menu)
                .frame(width: 150)
                
                Spacer()
            }
            
            // Alpha Toggle
            Toggle("Enable Alpha Channel", isOn: $alphaEnabled)
                .font(DesignSystem.Typography.body)
            
            // Color Picker
            VStack {
                Text("Color:")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                ColorPicker("Select Color", selection: $color)
                    .labelsHidden()
                    .frame(height: 100)
            }
            
            Spacer()
        }
        .padding()
    }
}

// MARK: - Advanced Color Tab
struct AdvancedColorTab: View {
    @Binding var color: Color
    @Binding var exposure: Float
    @Binding var temperature: Float
    @Binding var tint: Float
    @Binding var hdrEnabled: Bool
    @Binding var gradientMode: Bool
    @Binding var gradientEndColor: Color
    
    var body: some View {
        ScrollView {
            VStack(spacing: 20) {
                // HDR Toggle
                Toggle("Enable HDR", isOn: $hdrEnabled)
                    .font(DesignSystem.Typography.body)
                
                // Gradient Mode
                Toggle("Gradient Mode", isOn: $gradientMode)
                    .font(DesignSystem.Typography.body)
                
                if gradientMode {
                    HStack {
                        Text("End Color:")
                            .font(DesignSystem.Typography.body)
                        ColorPicker("", selection: $gradientEndColor)
                            .labelsHidden()
                    }
                }
                
                // Exposure
                VStack(alignment: .leading) {
                    Text("Exposure: \(String(format: "%.2f", exposure))")
                        .font(DesignSystem.Typography.body)
                    Slider(value: $exposure, in: -2...2)
                }
                
                // Temperature
                VStack(alignment: .leading) {
                    Text("Temperature: \(Int(temperature))K")
                        .font(DesignSystem.Typography.body)
                    Slider(value: $temperature, in: 1000...40000)
                }
                
                // Tint
                VStack(alignment: .leading) {
                    Text("Tint: \(String(format: "%.0f", tint))")
                        .font(DesignSystem.Typography.body)
                    Slider(value: $tint, in: -100...100)
                }
            }
            .padding()
        }
    }
}

// MARK: - Swatches Tab
struct SwatchesTab: View {
    @Binding var color: Color
    @Binding var customSwatches: [ColorSwatch]
    @Binding var colorHistory: [ColorHistoryEntry]
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Color History
                VStack(alignment: .leading) {
                    Text("Recent Colors")
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 8), spacing: 8) {
                        ForEach(colorHistory.prefix(16)) { entry in
                            Button(action: { color = entry.color }) {
                                RoundedRectangle(cornerRadius: 4)
                                    .fill(entry.color)
                                    .frame(width: 30, height: 30)
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 4)
                                            .stroke(DesignSystem.Colors.border, lineWidth: 1)
                                    )
                            }
                            .buttonStyle(.plain)
                            .help(entry.name ?? "Unnamed Color")
                        }
                    }
                }
                
                // Custom Swatches
                VStack(alignment: .leading) {
                    Text("Color Swatches")
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    ForEach(groupSwatchesByCategory(), id: \.category) { category in
                        VStack(alignment: .leading) {
                            Text(category.category)
                                .font(DesignSystem.Typography.subheadline)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 8), spacing: 8) {
                                ForEach(category.swatches) { swatch in
                                    Button(action: { color = swatch.color }) {
                                        RoundedRectangle(cornerRadius: 4)
                                            .fill(swatch.color)
                                            .frame(width: 30, height: 30)
                                            .overlay(
                                                RoundedRectangle(cornerRadius: 4)
                                                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
                                            )
                                    }
                                    .buttonStyle(.plain)
                                    .help(swatch.name)
                                }
                            }
                        }
                    }
                }
            }
            .padding()
        }
    }
    
    private func groupSwatchesByCategory() -> [(category: String, swatches: [ColorSwatch])] {
        let grouped = Dictionary(grouping: customSwatches) { $0.category }
        return grouped.map { (category: $0.key, swatches: $0.value) }
            .sorted { $0.category < $1.category }
    }
}

// MARK: - Harmony Tab
struct HarmonyTab: View {
    @Binding var color: Color
    @Binding var harmony: ColorHarmony
    
    var body: some View {
        VStack(spacing: 20) {
            // Harmony Type Selection
            Picker("Color Harmony", selection: $harmony) {
                ForEach(ColorHarmony.allCases, id: \.self) { type in
                    Text(type.rawValue).tag(type)
                }
            }
            .pickerStyle(.menu)
            
            // Harmony Preview
            VStack(alignment: .leading) {
                Text("Harmony Colors")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack(spacing: 12) {
                    ForEach(generateHarmonyColors(), id: \.self) { harmonyColor in
                        VStack(spacing: 4) {
                            RoundedRectangle(cornerRadius: 6)
                                .fill(harmonyColor)
                                .frame(width: 50, height: 50)
                                .overlay(
                                    RoundedRectangle(cornerRadius: 6)
                                        .stroke(DesignSystem.Colors.border, lineWidth: 1)
                                )
                            
                            Text(colorToHex(harmonyColor))
                                .font(DesignSystem.Typography.monoSmall)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        .onTapGesture {
                            color = harmonyColor
                        }
                    }
                }
            }
            
            Spacer()
        }
        .padding()
    }
    
    private func generateHarmonyColors() -> [Color] {
        // Simplified harmony generation
        let baseColor = color
        
        switch harmony {
        case .complementary:
            return [baseColor, complementColor(baseColor)]
        case .analogous:
            return [baseColor, shiftHue(baseColor, -30), shiftHue(baseColor, 30)]
        case .triadic:
            return [baseColor, shiftHue(baseColor, 120), shiftHue(baseColor, 240)]
        case .tetradic:
            return [baseColor, shiftHue(baseColor, 90), shiftHue(baseColor, 180), shiftHue(baseColor, 270)]
        case .splitComplementary:
            return [baseColor, shiftHue(baseColor, 150), shiftHue(baseColor, 210)]
        case .monochromatic:
            return [baseColor, adjustBrightness(baseColor, 0.3), adjustBrightness(baseColor, 0.7)]
        }
    }
    
    private func complementColor(_ color: Color) -> Color {
        return shiftHue(color, 180)
    }
    
    private func shiftHue(_ color: Color, _ degrees: Float) -> Color {
        let uiColor = UIColor(color)
        var hue: CGFloat = 0
        var saturation: CGFloat = 0
        var brightness: CGFloat = 0
        var alpha: CGFloat = 0
        
        uiColor.getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: &alpha)
        
        let newHue = (hue + CGFloat(degrees / 360)).truncatingRemainder(dividingBy: 1)
        
        return Color(.sRGB, hue: newHue, saturation: saturation, brightness: brightness, opacity: alpha)
    }
    
    private func adjustBrightness(_ color: Color, _ factor: Float) -> Color {
        let uiColor = UIColor(color)
        var hue: CGFloat = 0
        var saturation: CGFloat = 0
        var brightness: CGFloat = 0
        var alpha: CGFloat = 0
        
        uiColor.getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: &alpha)
        
        let newBrightness = max(0, min(1, brightness * CGFloat(factor)))
        
        return Color(.sRGB, hue: hue, saturation: saturation, brightness: newBrightness, opacity: alpha)
    }
    
    private func colorToHex(_ color: Color) -> String {
        let uiColor = UIColor(color)
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        
        uiColor.getRed(&red, green: &green, blue: &blue, alpha: nil)
        
        return String(format: "#%02lX%02lX%02lX",
                     lroundf(Float(red * 255)),
                     lroundf(Float(green * 255)),
                     lroundf(Float(blue * 255)))
    }
}

// MARK: - Accessibility Tab
struct AccessibilityTab: View {
    @Binding var color: Color
    @Binding var blindnessType: ColorBlindnessType
    
    var body: some View {
        VStack(spacing: 20) {
            // Color Blindness Simulation
            VStack(alignment: .leading) {
                Text("Color Blindness Simulation")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Picker("Type", selection: $blindnessType) {
                    ForEach(ColorBlindnessType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type)
                    }
                }
                .pickerStyle(.menu)
                
                HStack(spacing: 20) {
                    VStack {
                        Text("Normal")
                            .font(DesignSystem.Typography.small)
                        RoundedRectangle(cornerRadius: 8)
                            .fill(color)
                            .frame(width: 80, height: 60)
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
                            )
                    }
                    
                    VStack {
                        Text(blindnessType.rawValue)
                            .font(DesignSystem.Typography.small)
                        RoundedRectangle(cornerRadius: 8)
                            .fill(simulateColorBlindness(color, type: blindnessType))
                            .frame(width: 80, height: 60)
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
                            )
                    }
                }
            }
            
            // Contrast Checker
            VStack(alignment: .leading) {
                Text("Contrast Checker")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                HStack(spacing: 20) {
                    VStack {
                        Text("Text Color")
                            .font(DesignSystem.Typography.small)
                        RoundedRectangle(cornerRadius: 8)
                            .fill(color)
                            .frame(width: 80, height: 60)
                    }
                    
                    VStack {
                        Text("Background")
                            .font(DesignSystem.Typography.small)
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.white)
                            .frame(width: 80, height: 60)
                    }
                }
                
                let contrast = calculateContrast(color, Color.white)
                Text("Contrast Ratio: \(String(format: "%.2f", contrast))")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(contrast >= 4.5 ? .green : .orange)
                
                Text(contrast >= 4.5 ? "WCAG AA Compliant" : "Not WCAG Compliant")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(contrast >= 4.5 ? .green : .orange)
            }
            
            Spacer()
        }
        .padding()
    }
    
    private func simulateColorBlindness(_ color: Color, type: ColorBlindnessType) -> Color {
        // Simplified color blindness simulation
        let uiColor = UIColor(color)
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        var alpha: CGFloat = 0
        
        uiColor.getRed(&red, green: &green, blue: &blue, alpha: &alpha)
        
        switch type {
        case .none:
            return color
        case .protanopia:
            // Red-blind: reduce red component
            return Color(.sRGB, red: green * 0.5, green: green, blue: blue, opacity: alpha)
        case .deuteranopia:
            // Green-blind: reduce green component
            return Color(.sRGB, red: red, green: red * 0.5, blue: blue, opacity: alpha)
        case .tritanopia:
            // Blue-blind: reduce blue component
            return Color(.sRGB, red: red, green: green, blue: red * 0.5, opacity: alpha)
        case .achromatopsia:
            // Monochrome: convert to grayscale
            let gray = (red + green + blue) / 3
            return Color(.sRGB, red: gray, green: gray, blue: gray, opacity: alpha)
        }
    }
    
    private func calculateContrast(_ color1: Color, _ color2: Color) -> Float {
        let uiColor1 = UIColor(color1)
        let uiColor2 = UIColor(color2)
        
        var r1: CGFloat = 0, g1: CGFloat = 0, b1: CGFloat = 0
        var r2: CGFloat = 0, g2: CGFloat = 0, b2: CGFloat = 0
        
        uiColor1.getRed(&r1, green: &g1, blue: &b1, alpha: nil)
        uiColor2.getRed(&r2, green: &g2, blue: &b2, alpha: nil)
        
        let l1 = (0.299 * r1 + 0.587 * g1 + 0.114 * b1)
        let l2 = (0.299 * r2 + 0.587 * g2 + 0.114 * b2)
        
        let lighter = max(l1, l2)
        let darker = min(l1, l2)
        
        return Float((lighter + 0.05) / (darker + 0.05))
    }
}

// MARK: - Object Reference Editor (TODO-1601 to TODO-1608)
struct ObjectReferenceEditor: View {
    let label: String
    @Binding var reference: String?
    let allowedTypes: [ObjectType]
    let currentObjectId: String?
    
    @State private var showPicker = false
    @State private var searchText = ""
    @State private var selectedType: ObjectType = .gameObject
    @State private var sceneHierarchy: [SceneHierarchyNode] = []
    @State private var filteredHierarchy: [SceneHierarchyNode] = []
    @State private var showHierarchyBrowser = false
    @State private var isDragging = false
    @State private var dragOver = false
    @State private var circularReferenceWarning = false
    @State private var missingReferenceWarning = false
    @State private var nullReferenceWarning = false
    
    init(label: String, reference: Binding<String?>, allowedTypes: [ObjectType] = ObjectType.allCases, currentObjectId: String? = nil) {
        self.label = label
        self._reference = reference
        self.allowedTypes = allowedTypes
        self.currentObjectId = currentObjectId
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                // Type Filter
                if allowedTypes.count > 1 {
                    Picker("Type", selection: $selectedType) {
                        ForEach(allowedTypes, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                    .frame(width: 120)
                }
            }
            
            // Reference Display
            HStack(spacing: 8) {
                // Object Info
                VStack(alignment: .leading, spacing: 2) {
                    if let ref = reference, !ref.isEmpty {
                        HStack(spacing: 4) {
                            Image(systemName: getObjectTypeIcon(ref))
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                                .font(.system(size: 12))
                            
                            Text(URL(fileURLWithPath: ref).lastPathComponent)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            
                            // Prefab Badge
                            if isPrefabInstance(ref) {
                                HStack(spacing: 2) {
                                    Image(systemName: "cube.box.fill")
                                        .font(.system(size: 8))
                                    Text("Prefab")
                                        .font(DesignSystem.Typography.small)
                                }
                                .foregroundColor(DesignSystem.Colors.accentWarning)
                                .padding(.horizontal, 4)
                                .padding(.vertical, 1)
                                .background(DesignSystem.Colors.accentWarning.opacity(0.2))
                                .cornerRadius(3)
                            }
                        }
                        
                        Text(ref)
                            .font(DesignSystem.Typography.monoSmall)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    } else {
                        HStack(spacing: 4) {
                            Image(systemName: "questionmark.circle")
                                .foregroundColor(DesignSystem.Colors.textDisabled)
                            Text("None")
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textDisabled)
                        }
                    }
                }
                
                Spacer()
                
                // Action Buttons
                HStack(spacing: 4) {
                    EditorIconButton(icon: "magnifyingglass", tooltip: "Browse Objects") {
                        showPicker.toggle()
                    }
                    
                    EditorIconButton(icon: "list.bullet", tooltip: "Scene Hierarchy") {
                        showHierarchyBrowser.toggle()
                    }
                    
                    if reference != nil && !reference!.isEmpty {
                        EditorIconButton(icon: "xmark.circle.fill", tooltip: "Clear Reference") {
                            reference = nil
                        }
                    }
                }
            }
            .padding(8)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(dragOver ? DesignSystem.Colors.hover : DesignSystem.Colors.backgroundPrimary)
                    .stroke(
                        getReferenceBorderColor(),
                        lineWidth: getReferenceBorderWidth()
                    )
            )
            .onDrop(of: [.text], isTargeted: $dragOver) { providers in
                handleDrop(providers: providers)
            }
            
            // Warnings
            VStack(alignment: .leading, spacing: 4) {
                if circularReferenceWarning {
                    HStack(spacing: 4) {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(DesignSystem.Colors.accentError)
                        Text("Circular reference detected")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.accentError)
                    }
                }
                
                if missingReferenceWarning {
                    HStack(spacing: 4) {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(DesignSystem.Colors.accentWarning)
                        Text("Referenced object not found")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.accentWarning)
                    }
                }
                
                if nullReferenceWarning {
                    HStack(spacing: 4) {
                        Image(systemName: "info.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Text("Reference is null")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
            }
        }
        .sheet(isPresented: $showPicker) {
            ObjectPickerSheet(
                reference: $reference,
                allowedTypes: allowedTypes,
                currentObjectId: currentObjectId,
                searchText: $searchText,
                selectedType: $selectedType
            )
        }
        .sheet(isPresented: $showHierarchyBrowser) {
            SceneHierarchyBrowserSheet(
                reference: $reference,
                allowedTypes: allowedTypes,
                currentObjectId: currentObjectId,
                hierarchy: sceneHierarchy
            )
        }
        .onAppear {
            loadSceneHierarchy()
            validateReference()
        }
        .onChange(of: reference) { _ in
            validateReference()
        }
    }
    
    private func getObjectTypeIcon(_ ref: String) -> String {
        // Simple heuristic based on path
        if ref.contains("component") { return "puzzlepiece" }
        if ref.contains("material") { return "paintpalette" }
        if ref.contains("texture") { return "photo" }
        if ref.contains("mesh") { return "cube.transparent" }
        if ref.contains("audio") { return "speaker.wave.2" }
        if ref.contains("animation") { return "figure.walk" }
        if ref.contains("prefab") { return "cube.box" }
        if ref.contains("script") { return "doc.text" }
        return "cube"
    }
    
    private func isPrefabInstance(_ ref: String) -> Bool {
        return ref.contains("prefab")
    }
    
    private func getReferenceBorderColor() -> Color {
        if circularReferenceWarning { return DesignSystem.Colors.accentError }
        if missingReferenceWarning { return DesignSystem.Colors.accentWarning }
        if nullReferenceWarning { return DesignSystem.Colors.textSecondary }
        return DesignSystem.Colors.border
    }
    
    private func getReferenceBorderWidth() -> CGFloat {
        if circularReferenceWarning || missingReferenceWarning { return 2 }
        return 1
    }
    
    private func handleDrop(providers: [NSItemProvider]) -> Bool {
        guard let provider = providers.first else { return false }
        
        provider.loadObject(ofClass: NSString.self) { string, error in
            if let droppedString = string as String {
                DispatchQueue.main.async {
                    // Validate dropped reference
                    if isValidReference(droppedString) {
                        reference = droppedString
                    }
                }
            }
        }
        return true
    }
    
    private func isValidReference(_ ref: String) -> Bool {
        // Check if reference is valid and not circular
        guard !ref.isEmpty else { return false }
        guard let currentId = currentObjectId else { return true }
        return ref != currentId
    }
    
    private func loadSceneHierarchy() {
        sceneHierarchy = SceneHierarchyNode.mockHierarchy()
        filteredHierarchy = sceneHierarchy
    }
    
    private func validateReference() {
        guard let ref = reference, !ref.isEmpty else {
            nullReferenceWarning = true
            missingReferenceWarning = false
            circularReferenceWarning = false
            return
        }
        
        nullReferenceWarning = false
        
        // Check for missing reference (simplified)
        missingReferenceWarning = !objectExists(ref)
        
        // Check for circular reference
        if let currentId = currentObjectId {
            circularReferenceWarning = wouldCreateCircularReference(ref, currentId)
        }
    }
    
    private func objectExists(_ ref: String) -> Bool {
        // Simplified existence check
        return sceneHierarchy.contains { node in
            node.reference == ref || node.children.contains { $0.reference == ref }
        }
    }
    
    private func wouldCreateCircularReference(_ newRef: String, _ currentId: String) -> Bool {
        // Simplified circular reference check
        return newRef == currentId
    }
}

// MARK: - Object Picker Sheet
struct ObjectPickerSheet: View {
    @Binding var reference: String?
    let allowedTypes: [ObjectType]
    let currentObjectId: String?
    @Binding var searchText: String
    @Binding var selectedType: ObjectType
    
    @Environment(\.dismiss) private var dismiss
    @State private var availableObjects: [String] = []
    @State private var filteredObjects: [String] = []
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Search Bar
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    TextField("Search objects...", text: $searchText)
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.body)
                    
                    if !searchText.isEmpty {
                        Button(action: { searchText = "" }) {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
                .padding()
                
                // Object List
                List(filteredObjects, id: \.self) { object in
                    HStack {
                        Image(systemName: getObjectTypeIcon(object))
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                            .frame(width: 20)
                        
                        VStack(alignment: .leading) {
                            Text(URL(fileURLWithPath: object).lastPathComponent)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                            
                            Text(object)
                                .font(DesignSystem.Typography.monoSmall)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                        
                        Spacer()
                        
                        if reference == object {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(DesignSystem.Colors.accentSuccess)
                        }
                    }
                    .contentShape(Rectangle())
                    .onTapGesture {
                        reference = object
                        dismiss()
                    }
                }
                .listStyle(.plain)
            }
            .navigationTitle("Select Object")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        dismiss()
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Clear") {
                        reference = nil
                        dismiss()
                    }
                    .disabled(reference == nil)
                }
            }
        }
        .frame(width: 500, height: 600)
        .onAppear {
            loadAvailableObjects()
        }
        .onChange(of: searchText) { _ in
            filterObjects()
        }
        .onChange(of: selectedType) { _ in
            filterObjects()
        }
    }
    
    private func getObjectTypeIcon(_ object: String) -> String {
        if object.contains("component") { return "puzzlepiece" }
        if object.contains("material") { return "paintpalette" }
        if object.contains("texture") { return "photo" }
        if object.contains("mesh") { return "cube.transparent" }
        if object.contains("audio") { return "speaker.wave.2" }
        if object.contains("animation") { return "figure.walk" }
        if object.contains("prefab") { return "cube.box" }
        if object.contains("script") { return "doc.text" }
        return "cube"
    }
    
    private func loadAvailableObjects() {
        // Mock data for available objects
        availableObjects = [
            "game_objects/player/player_prefab",
            "game_objects/camera/main_camera",
            "game_objects/lighting/directional_light",
            "components/mesh_renderer/player_mesh",
            "components/physics/player_collider",
            "materials/player/player_material",
            "materials/environment/ground_material",
            "textures/player/player_diffuse",
            "textures/environment/ground_diffuse",
            "meshes/characters/player_model",
            "meshes/environment/ground_mesh",
            "audio/sfx/player_footsteps",
            "audio/music/background_music",
            "animations/player/idle_animation",
            "animations/player/walk_animation",
            "scripts/player/player_controller",
            "scripts/game/game_manager"
        ]
        
        filterObjects()
    }
    
    private func filterObjects() {
        filteredObjects = availableObjects.filter { object in
            // Type filter
            let matchesType = allowedTypes.contains { type in
                object.contains(type.rawValue.lowercased())
            }
            
            // Search filter
            let matchesSearch = searchText.isEmpty || 
                object.localizedCaseInsensitiveContains(searchText)
            
            return matchesType && matchesSearch
        }
    }
}

// MARK: - Scene Hierarchy Browser Sheet
struct SceneHierarchyBrowserSheet: View {
    @Binding var reference: String?
    let allowedTypes: [ObjectType]
    let currentObjectId: String?
    let hierarchy: [SceneHierarchyNode]
    
    @Environment(\.dismiss) private var dismiss
    @State private var expandedNodes: Set<String> = []
    @State private var searchText = ""
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Search Bar
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    TextField("Search hierarchy...", text: $searchText)
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.body)
                    
                    if !searchText.isEmpty {
                        Button(action: { searchText = "" }) {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(6)
                .padding()
                
                // Hierarchy Tree
                List {
                    ForEach(filteredHierarchy) { node in
                        HierarchyNodeRow(
                            node: node,
                            reference: $reference,
                            allowedTypes: allowedTypes,
                            expandedNodes: $expandedNodes,
                            searchText: searchText
                        )
                    }
                }
                .listStyle(.plain)
            }
            .navigationTitle("Scene Hierarchy")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        dismiss()
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Clear") {
                        reference = nil
                        dismiss()
                    }
                    .disabled(reference == nil)
                }
            }
        }
        .frame(width: 600, height: 700)
    }
    
    private var filteredHierarchy: [SceneHierarchyNode] {
        guard !searchText.isEmpty else { return hierarchy }
        
        return hierarchy.filter { node in
            nodeMatchesSearch(node)
        }
    }
    
    private func nodeMatchesSearch(_ node: SceneHierarchyNode) -> Bool {
        if node.name.localizedCaseInsensitiveContains(searchText) {
            return true
        }
        
        return node.children.contains { child in
            child.name.localizedCaseInsensitiveContains(searchText)
        }
    }
}

// MARK: - Hierarchy Node Row
struct HierarchyNodeRow: View {
    let node: SceneHierarchyNode
    @Binding var reference: String?
    let allowedTypes: [ObjectType]
    @Binding var expandedNodes: Set<String>
    let searchText: String
    
    private var isExpanded: Bool {
        expandedNodes.contains(node.id.uuidString)
    }
    
    private var isAllowedType: Bool {
        allowedTypes.contains(node.type)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Node Row
            HStack(spacing: 8) {
                // Expand/Collapse Button
                if !node.children.isEmpty {
                    Button(action: {
                        if isExpanded {
                            expandedNodes.remove(node.id.uuidString)
                        } else {
                            expandedNodes.insert(node.id.uuidString)
                        }
                    }) {
                        Image(systemName: isExpanded ? "chevron.down" : "chevron.right")
                            .font(.system(size: 10, weight: .medium))
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .frame(width: 16)
                } else {
                    Spacer()
                        .frame(width: 16)
                }
                
                // Node Icon
                Image(systemName: node.type.icon)
                    .foregroundColor(isAllowedType ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textDisabled)
                    .frame(width: 16)
                
                // Node Name
                Text(node.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(isAllowedType ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textDisabled)
                
                // Status Indicators
                HStack(spacing: 4) {
                    if !node.isActive {
                        Image(systemName: "eye.slash")
                            .font(.system(size: 10))
                            .foregroundColor(DesignSystem.Colors.textDisabled)
                    }
                    
                    if node.isPrefab {
                        HStack(spacing: 2) {
                            Image(systemName: "cube.box.fill")
                                .font(.system(size: 8))
                            Text("Prefab")
                                .font(DesignSystem.Typography.small)
                        }
                        .foregroundColor(DesignSystem.Colors.accentWarning)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(DesignSystem.Colors.accentWarning.opacity(0.2))
                        .cornerRadius(3)
                    }
                }
                
                Spacer()
                
                // Selection Indicator
                if reference == node.reference {
                    Image(systemName: "checkmark.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentSuccess)
                }
            }
            .contentShape(Rectangle())
            .onTapGesture {
                if isAllowedType {
                    reference = node.reference
                }
            }
            .padding(.vertical, 2)
            
            // Children
            if isExpanded && !node.children.isEmpty {
                ForEach(node.children) { child in
                    HierarchyNodeRow(
                        node: child,
                        reference: $reference,
                        allowedTypes: allowedTypes,
                        expandedNodes: $expandedNodes,
                        searchText: searchText
                    )
                    .padding(.leading, 24)
                }
            }
        }
    }
}

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
