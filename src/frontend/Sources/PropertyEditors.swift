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
struct LogarithmicFloatEditor: View {
    @Binding var value: Float
    let range: ClosedRange<Float>
    let logarithmicBase: Float
    
    init(value: Binding<Float>, in range: ClosedRange<Float> = 0.001...1000, base: Float = 10) {
        self._value = value
        self.range = range
        self.logarithmicBase = base
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Log Scale")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(String(format: "%.3f", value))
                    .font(.caption.monospaced())
            }
            
            Slider(
                value: Binding(
                    get: {
                        guard value > 0 else { return 0 }
                        return log(value) / log(logarithmicBase)
                    },
                    set: { newValue in
                        value = pow(logarithmicBase, newValue)
                        value = max(range.lowerBound, min(range.upperBound, value))
                    }
                ),
                in: log(range.lowerBound) / log(logarithmicBase)...log(range.upperBound) / log(logarithmicBase)
            )
        }
    }
}

// TODO-1502: Float editor with custom curve mapping
struct CurveMappedFloatEditor: View {
    @Binding var value: Float
    let curve: (Float) -> Float
    let range: ClosedRange<Float>
    
    init(value: Binding<Float>, in range: ClosedRange<Float> = 0...1, curve: @escaping (Float) -> Float = { $0 }) {
        self._value = value
        self.range = range
        self.curve = curve
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Curve Mapped")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(String(format: "%.3f", value))
                    .font(.caption.monospaced())
            }
            
            Slider(
                value: Binding(
                    get: {
                        let normalized = (value - range.lowerBound) / (range.upperBound - range.lowerBound)
                        return curve(normalized)
                    },
                    set: { newValue in
                        let denormalized = newValue * (range.upperBound - range.lowerBound) + range.lowerBound
                        value = max(range.lowerBound, min(range.upperBound, denormalized))
                    }
                ),
                in: 0...1
            )
        }
    }
}

// TODO-1503: Float editor with percentage display mode
struct PercentageFloatEditor: View {
    @Binding var value: Float
    let range: ClosedRange<Float>
    
    init(value: Binding<Float>, in range: ClosedRange<Float> = 0...1) {
        self._value = value
        self.range = range
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Percentage")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(String(format: "%.1f%%", value * 100))
                    .font(.caption.monospaced())
            }
            
            Slider(value: $value, in: range)
        }
    }
}

// TODO-1504: Float editor with scientific notation
struct ScientificFloatEditor: View {
    @Binding var value: Float
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Scientific")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(value.formatted(.number.precision(.fractionLength(2)).notation(.scientific)))
                    .font(.caption.monospaced())
            }
            
            TextField("Value", value: $value, format: .number.precision(.fractionLength(6)).notation(.scientific))
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .keyboardType(.decimalPad)
        }
    }
}

// TODO-1505: Float editor with infinity/NaN handling
struct RobustFloatEditor: View {
    @Binding var value: Float
    @State private var textValue: String = ""
    @State private var isValid: Bool = true
    
    init(value: Binding<Float>) {
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Value")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                if !isValid {
                    Image(systemName: "exclamationmark.triangle")
                        .foregroundColor(.orange)
                        .font(.caption)
                }
                
                Spacer()
                
                Menu {
                    Button("Infinity") { value = .infinity }
                    Button("-Infinity") { value = -.infinity }
                    Button("NaN") { value = .nan }
                    Button("Zero") { value = 0 }
                } label: {
                    Image(systemName: "ellipsis.circle")
                        .font(.caption)
                }
            }
            
            TextField("Enter value", text: $textValue)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(isValid ? Color.gray : Color.orange, lineWidth: 1)
                )
                .onSubmit {
                    if let newValue = Float(textValue) {
                        value = newValue
                        isValid = true
                    } else {
                        isValid = false
                    }
                }
        }
        .onAppear {
            updateTextFromValue()
        }
        .onChange(of: value) { _ in
            updateTextFromValue()
        }
    }
    
    private func updateTextFromValue() {
        if value.isInfinity {
            textValue = value > 0 ? "∞" : "-∞"
        } else if value.isNaN {
            textValue = "NaN"
        } else {
            textValue = String(format: "%.6f", value)
        }
        isValid = true
    }
}

// TODO-1506: Float editor with visual power indicator
struct PowerIndicatorFloatEditor: View {
    @Binding var value: Float
    let maxPower: Float
    
    init(value: Binding<Float>, maxPower: Float = 100) {
        self._value = value
        self.maxPower = maxPower
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Power")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(String(format: "%.1f / %.1f", value, maxPower))
                    .font(.caption.monospaced())
            }
            
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    // Background
                    Rectangle()
                        .fill(Color.gray.opacity(0.3))
                        .frame(height: 8)
                        .cornerRadius(4)
                    
                    // Power fill
                    Rectangle()
                        .fill(powerGradient)
                        .frame(width: geometry.size.width * CGFloat(min(value, maxPower) / maxPower), height: 8)
                        .cornerRadius(4)
                    
                    // Power level markers
                    ForEach(0..<5) { i in
                        let x = geometry.size.width * CGFloat(i) / 4
                        Rectangle()
                            .fill(Color.black.opacity(0.3))
                            .frame(width: 1, height: 12)
                            .position(x: x, y: 4)
                    }
                }
            }
            .frame(height: 12)
            
            Slider(value: $value, in: 0...maxPower)
        }
    }
    
    private var powerGradient: LinearGradient {
        let ratio = min(value, maxPower) / maxPower
        let color: Color
        if ratio < 0.25 {
            color = .green
        } else if ratio < 0.5 {
            color = .yellow
        } else if ratio < 0.75 {
            color = .orange
        } else {
            color = .red
        }
        return LinearGradient(colors: [color.opacity(0.7), color], startPoint: .leading, endPoint: .trailing)
    }
}

// TODO-1507: Float editor with audio-style dB display
struct DecibelFloatEditor: View {
    @Binding var value: Float // Value in dB
    let range: ClosedRange<Float> // dB range (typically -60 to 0)
    
    init(value: Binding<Float>, in range: ClosedRange<Float> = -60...0) {
        self._value = value
        self.range = range
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Level (dB)")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(String(format: "%.1f dB", value))
                    .font(.caption.monospaced())
            }
            
            // dB meter visualization
            GeometryReader { geometry in
                HStack(spacing: 0) {
                    ForEach(0..<Int(geometry.size.width)) { i in
                        let normalizedPos = Float(i) / Float(geometry.size.width)
                        let dbValue = range.lowerBound + (range.upperBound - range.lowerBound) * normalizedPos
                        let isActive = value >= dbValue
                        let color: Color
                        
                        if dbValue < -40 {
                            color = isActive ? Color.green : Color.green.opacity(0.2)
                        } else if dbValue < -20 {
                            color = isActive ? Color.yellow : Color.yellow.opacity(0.2)
                        } else if dbValue < -6 {
                            color = isActive ? Color.orange : Color.orange.opacity(0.2)
                        } else {
                            color = isActive ? Color.red : Color.red.opacity(0.2)
                        }
                        
                        Rectangle()
                            .fill(color)
                            .frame(width: 1)
                    }
                }
            }
            .frame(height: 8)
            
            Slider(value: $value, in: range)
        }
    }
}

// TODO-1508: Float editor with time format (hh:mm:ss)
struct TimeFormatFloatEditor: View {
    @Binding var value: Float // Value in seconds
    let allowNegative: Bool
    
    init(value: Binding<Float>, allowNegative: Bool = false) {
        self._value = value
        self.allowNegative = allowNegative
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Time")
                    .font(.caption)
                    .foregroundColor(.secondary)
                Spacer()
                Text(formatTime(value))
                    .font(.caption.monospaced())
            }
            
            HStack {
                TextField("Hours", value: Binding(
                    get: { Int(abs(value) / 3600) },
                    set: { newValue in
                        let seconds = value.truncatingRemainder(dividingBy: 3600)
                        value = Float(newValue) * 3600 + seconds
                        if !allowNegative && value < 0 { value = 0 }
                    }
                ), format: .number)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .frame(width: 60)
                
                Text(":")
                
                TextField("Min", value: Binding(
                    get: { Int(abs(value).truncatingRemainder(dividingBy: 3600) / 60) },
                    set: { newValue in
                        let seconds = value.truncatingRemainder(dividingBy: 60)
                        let hours = Int(value / 3600)
                        value = Float(hours) * 3600 + Float(newValue) * 60 + seconds
                        if !allowNegative && value < 0 { value = 0 }
                    }
                ), format: .number)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .frame(width: 60)
                
                Text(":")
                
                TextField("Sec", value: Binding(
                    get: { Int(abs(value).truncatingRemainder(dividingBy: 60)) },
                    set: { newValue in
                        let minutes = Int(value / 60)
                        let hours = Int(value / 3600)
                        value = Float(hours) * 3600 + Float(minutes) * 60 + Float(newValue)
                        if !allowNegative && value < 0 { value = 0 }
                    }
                ), format: .number)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .frame(width: 60)
            }
            
            Slider(value: $value, in: allowNegative ? -3600...3600 : 0...3600)
        }
    }
    
    private func formatTime(_ seconds: Float) -> String {
        let absSeconds = abs(seconds)
        let hours = Int(absSeconds / 3600)
        let minutes = Int(absSeconds.truncatingRemainder(dividingBy: 3600) / 60)
        let secs = Int(absSeconds.truncatingRemainder(dividingBy: 60))
        let sign = seconds < 0 ? "-" : ""
        return String(format: "%@%02d:%02d:%02d", sign, hours, minutes, secs)
    }
}

// TODO-1509: Float editor with angle format (degrees/radians)
struct AngleFloatEditor: View {
    @Binding var value: Float // Value in radians
    @State private var useDegrees: Bool = true
    
    init(value: Binding<Float>) {
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Angle")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Toggle("°", isOn: $useDegrees)
                    .toggleStyle(ButtonToggleStyle())
                    .frame(width: 30)
                
                Text(displayValue)
                    .font(.caption.monospaced())
            }
            
            // Angle visualization
            GeometryReader { geometry in
                ZStack {
                    Circle()
                        .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                    
                    // Angle arc
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let radius = min(geometry.size.width, geometry.size.height) / 2 - 10
                        path.move(to: center)
                        path.addArc(center: center, radius: radius, startAngle: .zero, endAngle: .radians(Double(value)), clockwise: false)
                    }
                    .stroke(Color.blue, lineWidth: 2)
                    
                    // Angle line
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let radius = min(geometry.size.width, geometry.size.height) / 2 - 10
                        let end = CGPoint(
                            x: center.x + radius * cos(value),
                            y: center.y + radius * sin(value)
                        )
                        path.move(to: center)
                        path.addLine(to: end)
                    }
                    .stroke(Color.blue, lineWidth: 2)
                }
            }
            .frame(height: 60)
            
            TextField("Value", value: Binding(
                get: { useDegrees ? value * 180 / .pi : value },
                set: { newValue in
                    value = useDegrees ? newValue * .pi / 180 : newValue
                }
            ), format: .number.precision(.fractionLength(3)))
            .textFieldStyle(RoundedBorderTextFieldStyle())
        }
    }
    
    private var displayValue: String {
        if useDegrees {
            return String(format: "%.1f°", value * 180 / .pi)
        } else {
            return String(format: "%.3f rad", value)
        }
    }
}

// TODO-1510: Float editor with distance format (m/km/mi)
struct DistanceFloatEditor: View {
    @Binding var value: Float // Value in meters
    @State private var unit: DistanceUnit = .meters
    
    enum DistanceUnit: String, CaseIterable {
        case meters = "m"
        case kilometers = "km"
        case miles = "mi"
        case feet = "ft"
        case yards = "yd"
        
        var conversionToMeters: Float {
            switch self {
            case .meters: return 1.0
            case .kilometers: return 1000.0
            case .miles: return 1609.34
            case .feet: return 0.3048
            case .yards: return 0.9144
            }
        }
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Distance")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Picker("Unit", selection: $unit) {
                    ForEach(DistanceUnit.allCases, id: \.self) { unit in
                        Text(unit.rawValue).tag(unit)
                    }
                }
                .pickerStyle(SegmentedPickerStyle())
                .frame(width: 200)
            }
            
            HStack {
                TextField("Value", value: Binding(
                    get: { value / unit.conversionToMeters },
                    set: { value = $0 * unit.conversionToMeters }
                ), format: .number.precision(.fractionLength(2)))
                .textFieldStyle(RoundedBorderTextFieldStyle())
                
                Text(unit.rawValue)
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Text("≈ \(String(format: "%.2f", value))m")
                    .font(.caption.monospaced())
                    .foregroundColor(.secondary)
            }
        }
    }
}

// MARK: - Vector Property Editor TODOs (TODO-1521 to TODO-1540)

// TODO-1521: Vector2 property editor
struct Vector2Editor: View {
    @Binding var value: SIMD2<Float>
    let label: String
    
    init(_ label: String = "Vector2", value: Binding<SIMD2<Float>>) {
        self.label = label
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                VStack {
                    Text("X")
                        .font(.caption2)
                    TextField("X", value: $value.x, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 80)
                }
                
                VStack {
                    Text("Y")
                        .font(.caption2)
                    TextField("Y", value: $value.y, format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 80)
                }
                
                Spacer()
                
                VStack {
                    Text("Length")
                        .font(.caption2)
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                }
            }
        }
    }
    
    private func length(_ vector: SIMD2<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y)
    }
}

// TODO-1522: Vector4 property editor
struct Vector4Editor: View {
    @Binding var value: SIMD4<Float>
    let label: String
    
    init(_ label: String = "Vector4", value: Binding<SIMD4<Float>>) {
        self.label = label
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                ForEach(0..<4, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z", "W"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z", "W"][i], value: Binding(
                            get: { value[i] },
                            set: { value[i] = $0 }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
                
                VStack {
                    Text("Length")
                        .font(.caption2)
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                }
            }
        }
    }
    
    private func length(_ vector: SIMD4<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w)
    }
}

// TODO-1523: Vector editor with swizzle operations
struct SwizzleVectorEditor: View {
    @Binding var value: SIMD3<Float>
    @State private var swizzlePattern: String = "xyz"
    @State private var swizzledValue: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Vector with Swizzle")
                .font(.caption)
                .foregroundColor(.secondary)
            
            // Original vector
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { value[i] = $0 }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
            }
            
            // Swizzle controls
            HStack {
                Text("Swizzle:")
                    .font(.caption)
                
                TextField("Pattern", text: $swizzlePattern)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .frame(width: 60)
                    .onChange(of: swizzlePattern) { _ in
                        updateSwizzledValue()
                    }
                
                Button("Apply") {
                    applySwizzle()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Text("Result: (\(String(format: "%.2f", swizzledValue.x)), \(String(format: "%.2f", swizzledValue.y)), \(String(format: "%.2f", swizzledValue.z)))")
                    .font(.caption.monospaced())
                    .foregroundColor(.secondary)
            }
        }
        .onAppear {
            updateSwizzledValue()
        }
    }
    
    private func updateSwizzledValue() {
        let pattern = swizzlePattern.lowercased()
        swizzledValue = SIMD3<Float>(
            getValueForComponent(pattern.first ?? "x"),
            getValueForComponent(pattern.count > 1 ? pattern[pattern.index(pattern.startIndex, offsetBy: 1)] : "x"),
            getValueForComponent(pattern.count > 2 ? pattern[pattern.index(pattern.startIndex, offsetBy: 2)] : "x")
        )
    }
    
    private func getValueForComponent(_ component: Character) -> Float {
        switch component {
        case "x": return value.x
        case "y": return value.y
        case "z": return value.z
        case "0": return 0
        case "1": return 1
        default: return 0
        }
    }
    
    private func applySwizzle() {
        value = swizzledValue
    }
}

// TODO-1524: Vector editor with normalize button
struct NormalizableVectorEditor: View {
    @Binding var value: SIMD3<Float>
    let label: String
    
    init(_ label: String = "Vector3", value: Binding<SIMD3<Float>>) {
        self.label = label
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { value[i] = $0 }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
                
                VStack {
                    Text("Length")
                        .font(.caption2)
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                    
                    Button("Normalize") {
                        normalizeVector()
                    }
                    .buttonStyle(.bordered)
                    .font(.caption)
                }
            }
        }
    }
    
    private func length(_ vector: SIMD3<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
    }
    
    private func normalizeVector() {
        let len = length(value)
        if len > 0.001 {
            value = value / len
        }
    }
}

// TODO-1525: Vector editor with length display
struct VectorWithLengthEditor: View {
    @Binding var value: SIMD3<Float>
    let label: String
    
    init(_ label: String = "Vector3", value: Binding<SIMD3<Float>>) {
        self.label = label
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { value[i] = $0 }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Length")
                        .font(.caption2)
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                    
                    Text("Length²")
                        .font(.caption2)
                    Text(String(format: "%.3f", lengthSquared(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                }
            }
        }
    }
    
    private func length(_ vector: SIMD3<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
    }
    
    private func lengthSquared(_ vector: SIMD3<Float>) -> Float {
        return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z
    }
}

// TODO-1526: Vector editor with direction visualization
struct VectorDirectionEditor: View {
    @Binding var value: SIMD3<Float>
    let label: String
    
    init(_ label: String = "Vector3", value: Binding<SIMD3<Float>>) {
        self.label = label
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { value[i] = $0 }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
                
                // Direction visualization
                VStack {
                    Text("Direction")
                        .font(.caption2)
                    
                    GeometryReader { geometry in
                        ZStack {
                            // Background circle
                            Circle()
                                .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                            
                            // Grid lines
                            Path { path in
                                let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                                let radius = min(geometry.size.width, geometry.size.height) / 2 - 10
                                
                                // X axis
                                path.move(to: CGPoint(x: center.x - radius, y: center.y))
                                path.addLine(to: CGPoint(x: center.x + radius, y: center.y))
                                
                                // Y axis
                                path.move(to: CGPoint(x: center.x, y: center.y - radius))
                                path.addLine(to: CGPoint(x: center.x, y: center.y + radius))
                            }
                            .stroke(Color.gray.opacity(0.2), lineWidth: 0.5)
                            
                            // Direction arrow
                            Path { path in
                                let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                                let radius = min(geometry.size.width, geometry.size.height) / 2 - 15
                                let normalized = normalize(value)
                                let end = CGPoint(
                                    x: center.x + radius * CGFloat(normalized.x),
                                    y: center.y - radius * CGFloat(normalized.y) // Flip Y for visualization
                                )
                                
                                path.move(to: center)
                                path.addLine(to: end)
                                
                                // Arrow head
                                let arrowSize: CGFloat = 8
                                let angle = atan2(end.y - center.y, end.x - center.x)
                                path.move(to: end)
                                path.addLine(to: CGPoint(
                                    x: end.x - arrowSize * cos(angle - .pi / 6),
                                    y: end.y - arrowSize * sin(angle - .pi / 6)
                                ))
                                path.move(to: end)
                                path.addLine(to: CGPoint(
                                    x: end.x - arrowSize * cos(angle + .pi / 6),
                                    y: end.y - arrowSize * sin(angle + .pi / 6)
                                ))
                            }
                            .stroke(Color.blue, lineWidth: 2)
                        }
                    }
                    .frame(width: 80, height: 80)
                }
            }
        }
    }
    
    private func normalize(_ vector: SIMD3<Float>) -> SIMD3<Float> {
        let len = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
        return len > 0.001 ? vector / len : SIMD3<Float>(0, 0, 1)
    }
}

// TODO-1527: Vector editor with world/local toggle
struct WorldLocalVectorEditor: View {
    @Binding var value: SIMD3<Float>
    @State private var coordinateSpace: CoordinateSpace = .world
    
    enum CoordinateSpace: String, CaseIterable {
        case world = "World"
        case local = "Local"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Vector")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Picker("Space", selection: $coordinateSpace) {
                    ForEach(CoordinateSpace.allCases, id: \.self) { space in
                        Text(space.rawValue).tag(space)
                    }
                }
                .pickerStyle(SegmentedPickerStyle())
                .frame(width: 120)
            }
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { value[i] = $0 }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 2) {
                    Text(coordinateSpace.rawValue)
                        .font(.caption2)
                        .foregroundColor(.blue)
                    
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                }
            }
        }
    }
    
    private func length(_ vector: SIMD3<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
    }
}

// TODO-1528: Vector editor with snap to axis
struct SnapToAxisVectorEditor: View {
    @Binding var value: SIMD3<Float>
    let snapThreshold: Float
    
    init(value: Binding<SIMD3<Float>>, snapThreshold: Float = 0.1) {
        self._value = value
        self.snapThreshold = snapThreshold
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Vector (Snap to Axis)")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Button("Snap X") { snapToAxis(.x) }
                    .buttonStyle(.bordered)
                    .font(.caption)
                
                Button("Snap Y") { snapToAxis(.y) }
                    .buttonStyle(.bordered)
                    .font(.caption)
                
                Button("Snap Z") { snapToAxis(.z) }
                    .buttonStyle(.bordered)
                    .font(.caption)
            }
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { newValue in
                                value[i] = newValue
                                autoSnapToAxis(axis: i)
                            }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Length")
                        .font(.caption2)
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                    
                    if isNearAxis {
                        Text("Near Axis")
                            .font(.caption2)
                            .foregroundColor(.orange)
                    }
                }
            }
        }
    }
    
    private func length(_ vector: SIMD3<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
    }
    
    private func snapToAxis(_ axis: Int) {
        var snapped = value
        for i in 0..<3 {
            if i != axis {
                snapped[i] = 0
            }
        }
        value = snapped
    }
    
    private func autoSnapToAxis(axis: Int) {
        for i in 0..<3 {
            if i != axis && abs(value[i]) < snapThreshold {
                value[i] = 0
            }
        }
    }
    
    private var isNearAxis: Bool {
        let nonZeroAxes = (0..<3).filter { abs(value[$0]) > snapThreshold }.count
        return nonZeroAxes <= 1
    }
}

// TODO-1529: Vector editor with copy/paste individual axes
struct CopyPasteVectorEditor: View {
    @Binding var value: SIMD3<Float>
    @State private var copiedAxis: Int?
    @State private var copiedValue: Float = 0
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Vector (Copy/Paste Axes)")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        HStack {
                            Text(["X", "Y", "Z"][i])
                                .font(.caption2)
                            
                            Spacer()
                            
                            Button(copiedAxis == i ? "✓" : "📋") {
                                copyAxis(i)
                            }
                            .buttonStyle(.plain)
                            .font(.caption)
                            .foregroundColor(copiedAxis == i ? .green : .blue)
                            
                            if copiedAxis != nil {
                                Button("📄") {
                                    pasteToAxis(i)
                                }
                                .buttonStyle(.plain)
                                .font(.caption)
                                .foregroundColor(.purple)
                            }
                        }
                        
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { value[i] = $0 }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 80)
                    }
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Length")
                        .font(.caption2)
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                    
                    if let copiedAxis = copiedAxis {
                        Text("Copied: \(["X", "Y", "Z"][copiedAxis]) = \(String(format: "%.3f", copiedValue))")
                            .font(.caption2)
                            .foregroundColor(.green)
                    }
                }
            }
        }
    }
    
    private func length(_ vector: SIMD3<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
    }
    
    private func copyAxis(_ axis: Int) {
        copiedAxis = axis
        copiedValue = value[axis]
    }
    
    private func pasteToAxis(_ axis: Int) {
        if let copiedAxis = copiedAxis {
            value[axis] = copiedValue
        }
    }
}

// TODO-1530: Vector editor with proportional scaling lock
struct ProportionalVectorEditor: View {
    @Binding var value: SIMD3<Float>
    @State private var proportionalScale: Bool = false
    @State private var originalRatio: SIMD3<Float> = SIMD3<Float>(1, 1, 1)
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Vector (Proportional)")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Toggle("🔗", isOn: $proportionalScale)
                    .toggleStyle(ButtonToggleStyle())
                    .help("Proportional scaling")
            }
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i])
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: { value[i] },
                            set: { newValue in
                                if proportionalScale {
                                    updateProportional(axis: i, newValue: newValue)
                                } else {
                                    value[i] = newValue
                                    updateRatio()
                                }
                            }
                        ), format: .number.precision(.fractionLength(3)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 70)
                    }
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Length")
                        .font(.caption2)
                    Text(String(format: "%.3f", length(value)))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                    
                    if proportionalScale {
                        Text("🔗 Locked")
                            .font(.caption2)
                            .foregroundColor(.blue)
                    }
                }
            }
        }
        .onAppear {
            updateRatio()
        }
    }
    
    private func length(_ vector: SIMD3<Float>) -> Float {
        return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
    }
    
    private func updateRatio() {
        let maxComponent = max(abs(value.x), max(abs(value.y), abs(value.z)))
        if maxComponent > 0.001 {
            originalRatio = value / maxComponent
        } else {
            originalRatio = SIMD3<Float>(1, 1, 1)
        }
    }
    
    private func updateProportional(axis: Int, newValue: Float) {
        if abs(newValue) < 0.001 {
            value = SIMD3<Float>(0, 0, 0)
            return
        }
        
        let scale = newValue / value[axis]
        value = value * scale
        updateRatio()
    }
}

// MARK: - Rotation Property Editor TODOs (TODO-1541 to TODO-1560)

// TODO-1541: Euler angles editor with gimbal lock warning
struct EulerAnglesEditor: View {
    @Binding var value: SIMD3<Float> // XYZ Euler angles in radians
    @State private var useDegrees: Bool = true
    @State private var showGimbalWarning: Bool = false
    
    init(value: Binding<SIMD3<Float>>) {
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Euler Angles")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Toggle("°", isOn: $useDegrees)
                    .toggleStyle(ButtonToggleStyle())
                    .frame(width: 30)
                
                if showGimbalWarning {
                    HStack {
                        Image(systemName: "exclamationmark.triangle")
                            .foregroundColor(.orange)
                            .font(.caption)
                        Text("Gimbal Lock")
                            .font(.caption2)
                            .foregroundColor(.orange)
                    }
                }
            }
            
            HStack {
                ForEach(0..<3, id: \.self) { i in
                    VStack {
                        Text(["X", "Y", "Z"][i] + " (" + ["Pitch", "Yaw", "Roll"][i] + ")")
                            .font(.caption2)
                        TextField(["X", "Y", "Z"][i], value: Binding(
                            get: {
                                let angle = value[i]
                                return useDegrees ? angle * 180 / .pi : angle
                            },
                            set: { newValue in
                                let radians = useDegrees ? newValue * .pi / 180 : newValue
                                value[i] = radians
                                checkGimbalLock()
                            }
                        ), format: .number.precision(.fractionLength(2)))
                        .textFieldStyle(RoundedBorderTextFieldStyle())
                        .frame(width: 80)
                    }
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Quaternion")
                        .font(.caption2)
                    Text(String(format: "(%.3f, %.3f, %.3f, %.3f)", quaternion.x, quaternion.y, quaternion.z, quaternion.w))
                        .font(.caption.monospaced())
                        .foregroundColor(.secondary)
                }
            }
            
            // Euler angle visualization
            GeometryReader { geometry in
                ZStack {
                    // Gimbal visualization
                    ForEach(0..<3, id: \.self) { i in
                        Path { path in
                            let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                            let radius = min(geometry.size.width, geometry.size.height) / 2 - CGFloat(20 + i * 15)
                            
                            path.addEllipse(
                                center: center,
                                radii: CGSize(width: radius, height: radius),
                                startAngle: .zero,
                                endAngle: .radians(2 * .pi),
                        clockwise: false
                            )
                        }
                        .stroke([Color.red, Color.green, Color.blue][i], lineWidth: 2)
                        .opacity(0.6)
                    }
                }
            }
            .frame(height: 100)
        }
        .onAppear {
            checkGimbalLock()
        }
        .onChange(of: value) { _ in
            checkGimbalLock()
        }
    }
    
    private var quaternion: simd_quatf {
        return simd_quatf(
            angle: value.y,
            axis: SIMD3<Float>(0, 1, 0)
        ) * simd_quatf(
            angle: value.x,
            axis: SIMD3<Float>(1, 0, 0)
        ) * simd_quatf(
            angle: value.z,
            axis: SIMD3<Float>(0, 0, 1)
        )
    }
    
    private func checkGimbalLock() {
        // Check if we're close to gimbal lock (when Y angle approaches ±90°)
        let pitch = abs(value.x)
        let yaw = abs(value.y)
        let roll = abs(value.z)
        
        // Gimbal lock occurs when pitch approaches ±90° (π/2 radians)
        let gimbalThreshold: Float = .pi / 2 - 0.1 // Within 0.1 radians of 90°
        showGimbalWarning = pitch > gimbalThreshold
    }
}

// TODO-1542: Quaternion editor with visualization
struct QuaternionEditor: View {
    @Binding var value: simd_quatf
    @State private var useEuler: Bool = false
    @State private var eulerAngles: SIMD3<Float> = SIMD3<Float>(0, 0, 0)
    
    init(value: Binding<simd_quatf>) {
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Quaternion")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Toggle("Euler", isOn: $useEuler)
                    .toggleStyle(ButtonToggleStyle())
                    .frame(width: 60)
                
                Button("Normalize") {
                    normalizeQuaternion()
                }
                .buttonStyle(.bordered)
                .font(.caption)
            }
            
            if useEuler {
                // Euler angles mode
                HStack {
                    ForEach(0..<3, id: \.self) { i in
                        VStack {
                            Text(["X", "Y", "Z"][i] + " (deg)")
                                .font(.caption2)
                            TextField(["X", "Y", "Z"][i], value: Binding(
                                get: { eulerAngles[i] * 180 / .pi },
                                set: { newValue in
                                    eulerAngles[i] = newValue * .pi / 180
                                    updateQuaternionFromEuler()
                                }
                            ), format: .number.precision(.fractionLength(2)))
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .frame(width: 70)
                        }
                    }
                    
                    Spacer()
                }
            } else {
                // Quaternion components mode
                HStack {
                    ForEach(0..<4, id: \.self) { i in
                        VStack {
                            Text(["X", "Y", "Z", "W"][i])
                                .font(.caption2)
                            TextField(["X", "Y", "Z", "W"][i], value: Binding(
                                get: { [value.x, value.y, value.z, value.w][i] },
                                set: { newValue in
                                    switch i {
                                    case 0: value.x = newValue
                                    case 1: value.y = newValue
                                    case 2: value.z = newValue
                                    case 3: value.w = newValue
                                    default: break
                                    }
                                }
                            ), format: .number.precision(.fractionLength(3)))
                            .textFieldStyle(RoundedBorderTextFieldStyle())
                            .frame(width: 70)
                        }
                    }
                    
                    Spacer()
                    
                    VStack(alignment: .trailing, spacing: 2) {
                        Text("Length")
                            .font(.caption2)
                        Text(String(format: "%.3f", quaternionLength))
                            .font(.caption.monospaced())
                            .foregroundColor(quaternionLength > 0.99 && quaternionLength < 1.01 ? .green : .orange)
                    }
                }
            }
            
            // Quaternion visualization
            GeometryReader { geometry in
                ZStack {
                    // Unit sphere
                    Circle()
                        .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                    
                    // Rotation axis and angle
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let radius = min(geometry.size.width, geometry.size.height) / 2 - 20
                        
                        // Rotation axis
                        let axis = normalize(value.vector)
                        let axisEnd = CGPoint(
                            x: center.x + radius * CGFloat(axis.x),
                            y: center.y - radius * CGFloat(axis.y)
                        )
                        
                        path.move(to: center)
                        path.addLine(to: axisEnd)
                        
                        // Axis arrow
                        let arrowSize: CGFloat = 8
                        let angle = atan2(axisEnd.y - center.y, axisEnd.x - center.x)
                        path.move(to: axisEnd)
                        path.addLine(to: CGPoint(
                            x: axisEnd.x - arrowSize * cos(angle - .pi / 6),
                            y: axisEnd.y - arrowSize * sin(angle - .pi / 6)
                        ))
                        path.move(to: axisEnd)
                        path.addLine(to: CGPoint(
                            x: axisEnd.x - arrowSize * cos(angle + .pi / 6),
                            y: axisEnd.y - arrowSize * sin(angle + .pi / 6)
                        ))
                    }
                    .stroke(Color.blue, lineWidth: 2)
                    
                    // Rotation arc
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let radius = min(geometry.size.width, geometry.size.height) / 2 - 30
                        let angle = acos(value.w) * 2
                        
                        path.addArc(
                            center: center,
                            radius: radius,
                            startAngle: .zero,
                            endAngle: .radians(Double(angle)),
                            clockwise: false
                        )
                    }
                    .stroke(Color.orange, lineWidth: 2)
                }
            }
            .frame(height: 80)
        }
        .onAppear {
            updateEulerFromQuaternion()
        }
        .onChange(of: value) { _ in
            updateEulerFromQuaternion()
        }
    }
    
    private var quaternionLength: Float {
        return sqrt(value.x * value.x + value.y * value.y + value.z * value.z + value.w * value.w)
    }
    
    private func normalizeQuaternion() {
        let len = quaternionLength
        if len > 0.001 {
            value = value / len
        }
    }
    
    private func updateEulerFromQuaternion() {
        // Convert quaternion to Euler angles
        let q = normalize(value)
        
        // Roll (X-axis rotation)
        let sinr_cosp = 2 * (q.w * q.x + q.y * q.z)
        let cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y)
        eulerAngles.x = atan2(sinr_cosp, cosr_cosp)
        
        // Pitch (Y-axis rotation)
        let sinp = 2 * (q.w * q.y - q.z * q.x)
        eulerAngles.y = abs(sinp) >= 1 ? .pi / 2 : asin(sinp)
        
        // Yaw (Z-axis rotation)
        let siny_cosp = 2 * (q.w * q.z + q.x * q.y)
        let cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
        eulerAngles.z = atan2(siny_cosp, cosy_cosp)
    }
    
    private func updateQuaternionFromEuler() {
        // Convert Euler angles to quaternion
        let halfAngles = eulerAngles / 2
        
        let cx = cos(halfAngles.x)
        let sx = sin(halfAngles.x)
        let cy = cos(halfAngles.y)
        let sy = sin(halfAngles.y)
        let cz = cos(halfAngles.z)
        let sz = sin(halfAngles.z)
        
        value = simd_quatf(
            ix: sx * cy * cz - cx * sy * sz,
            iy: cx * sy * cz + sx * cy * sz,
            iz: cx * cy * sz - sx * sy * cz,
            r: cx * cy * cz + sx * sy * sz
        )
    }
    
    private func normalize(_ q: simd_quatf) -> simd_quatf {
        let len = sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w)
        return len > 0.001 ? q / len : simd_quatf(ix: 0, iy: 0, iz: 0, r: 1)
    }
    
    private func normalize(_ vector: SIMD3<Float>) -> SIMD3<Float> {
        let len = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
        return len > 0.001 ? vector / len : SIMD3<Float>(0, 0, 1)
    }
}

// TODO-1543: Rotation editor with axis-angle mode
struct AxisAngleEditor: View {
    @Binding var value: simd_quatf
    @State private var axis: SIMD3<Float> = SIMD3<Float>(0, 1, 0)
    @State private var angle: Float = 0
    @State private var useDegrees: Bool = true
    
    init(value: Binding<simd_quatf>) {
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Axis-Angle")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Toggle("°", isOn: $useDegrees)
                    .toggleStyle(ButtonToggleStyle())
                    .frame(width: 30)
                
                Button("Apply") {
                    applyAxisAngle()
                }
                .buttonStyle(.bordered)
                    .font(.caption)
            }
            
            // Axis controls
            HStack {
                Text("Axis:")
                    .font(.caption)
                
                ForEach(0..<3, id: \.self) { i in
                    TextField(["X", "Y", "Z"][i], value: Binding(
                        get: { axis[i] },
                        set: { axis[i] = $0 }
                    ), format: .number.precision(.fractionLength(3)))
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .frame(width: 60)
                }
                
                Button("Normalize") {
                    normalizeAxis()
                }
                .buttonStyle(.bordered)
                .font(.caption)
                
                Spacer()
                
                Text(String(format: "%.3f", axisLength))
                    .font(.caption.monospaced())
                    .foregroundColor(axisLength > 0.99 && axisLength < 1.01 ? .green : .orange)
            }
            
            // Angle control
            HStack {
                Text("Angle:")
                    .font(.caption)
                
                TextField("Angle", value: Binding(
                    get: { useDegrees ? angle * 180 / .pi : angle },
                    set: { angle = useDegrees ? $0 * .pi / 180 : $0 }
                ), format: .number.precision(.fractionLength(2)))
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .frame(width: 80)
                
                Spacer()
                
                Text(useDegrees ? "deg" : "rad")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            // Quick angle presets
            HStack {
                Text("Quick:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                ForEach([0, 45, 90, 180, 270, 360], id: \.self) { presetAngle in
                    Button("\(presetAngle)°") {
                        angle = Float(presetAngle) * .pi / 180
                        applyAxisAngle()
                    }
                    .buttonStyle(.bordered)
                    .font(.caption2)
                }
                
                Spacer()
            }
            
            // Visualization
            GeometryReader { geometry in
                ZStack {
                    // Reference circle
                    Circle()
                        .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                    
                    // Axis vector
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let radius = min(geometry.size.width, geometry.size.height) / 2 - 20
                        let normalizedAxis = normalize(axis)
                        let axisEnd = CGPoint(
                            x: center.x + radius * CGFloat(normalizedAxis.x),
                            y: center.y - radius * CGFloat(normalizedAxis.y)
                        )
                        
                        path.move(to: center)
                        path.addLine(to: axisEnd)
                    }
                    .stroke(Color.blue, lineWidth: 2)
                    
                    // Rotation arc
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let radius = min(geometry.size.width, geometry.size.height) / 2 - 30
                        
                        path.addArc(
                            center: center,
                            radius: radius,
                            startAngle: .zero,
                            endAngle: .radians(Double(angle)),
                            clockwise: false
                        )
                    }
                    .stroke(Color.orange, lineWidth: 2)
                }
            }
            .frame(height: 80)
        }
        .onAppear {
            updateAxisAngleFromQuaternion()
        }
        .onChange(of: value) { _ in
            updateAxisAngleFromQuaternion()
        }
    }
    
    private var axisLength: Float {
        return sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z)
    }
    
    private func normalizeAxis() {
        let len = axisLength
        if len > 0.001 {
            axis = axis / len
        }
    }
    
    private func applyAxisAngle() {
        if axisLength > 0.001 {
            let normalizedAxis = axis / axisLength
            value = simd_quatf(angle: angle, axis: normalizedAxis)
        }
    }
    
    private func updateAxisAngleFromQuaternion() {
        // Extract axis and angle from quaternion
        let q = normalize(value)
        
        angle = acos(max(-1, min(1, q.w))) * 2
        
        if abs(angle) > 0.001 {
            let sinHalfAngle = sin(angle / 2)
            axis = SIMD3<Float>(q.x, q.y, q.z) / sinHalfAngle
        } else {
            axis = SIMD3<Float>(0, 1, 0) // Default axis
        }
    }
    
    private func normalize(_ q: simd_quatf) -> simd_quatf {
        let len = sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w)
        return len > 0.001 ? q / len : simd_quatf(ix: 0, iy: 0, iz: 0, r: 1)
    }
    
    private func normalize(_ vector: SIMD3<Float>) -> SIMD3<Float> {
        let len = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
        return len > 0.001 ? vector / len : SIMD3<Float>(0, 0, 1)
    }
}

// TODO-1544: Rotation editor with look-at helper
struct LookAtRotationEditor: View {
    @Binding var value: simd_quatf
    @State private var target: SIMD3<Float> = SIMD3<Float>(0, 0, 1)
    @State private var upVector: SIMD3<Float> = SIMD3<Float>(0, 1, 0)
    
    init(value: Binding<simd_quatf>) {
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Look At")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Button("Apply Look At") {
                    applyLookAt()
                }
                .buttonStyle(.bordered)
                .font(.caption)
            }
            
            // Target position
            HStack {
                Text("Target:")
                    .font(.caption)
                
                ForEach(0..<3, id: \.self) { i in
                    TextField(["X", "Y", "Z"][i], value: Binding(
                        get: { target[i] },
                        set: { target[i] = $0 }
                    ), format: .number.precision(.fractionLength(2)))
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .frame(width: 70)
                }
                
                Spacer()
            }
            
            // Up vector
            HStack {
                Text("Up:")
                    .font(.caption)
                
                ForEach(0..<3, id: \.self) { i in
                    TextField(["X", "Y", "Z"][i], value: Binding(
                        get: { upVector[i] },
                        set: { upVector[i] = $0 }
                    ), format: .number.precision(.fractionLength(2)))
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .frame(width: 70)
                }
                
                Button("Normalize") {
                    normalizeUpVector()
                }
                .buttonStyle(.bordered)
                .font(.caption)
                
                Spacer()
                
                Text(String(format: "%.3f", upVectorLength))
                    .font(.caption.monospaced())
                    .foregroundColor(upVectorLength > 0.99 && upVectorLength < 1.01 ? .green : .orange)
            }
            
            // Quick targets
            HStack {
                Text("Quick:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Button("+X") { target = SIMD3<Float>(1, 0, 0); applyLookAt() }
                Button("+Y") { target = SIMD3<Float>(0, 1, 0); applyLookAt() }
                Button("+Z") { target = SIMD3<Float>(0, 0, 1); applyLookAt() }
                Button("-X") { target = SIMD3<Float>(-1, 0, 0); applyLookAt() }
                Button("-Y") { target = SIMD3<Float>(0, -1, 0); applyLookAt() }
                Button("-Z") { target = SIMD3<Float>(0, 0, -1); applyLookAt() }
                
                ForEach(["+X", "+Y", "+Z", "-X", "-Y", "-Z"], id: \.self) { direction in
                    Button(direction) {
                        switch direction {
                        case "+X": target = SIMD3<Float>(1, 0, 0)
                        case "+Y": target = SIMD3<Float>(0, 1, 0)
                        case "+Z": target = SIMD3<Float>(0, 0, 1)
                        case "-X": target = SIMD3<Float>(-1, 0, 0)
                        case "-Y": target = SIMD3<Float>(0, -1, 0)
                        case "-Z": target = SIMD3<Float>(0, 0, -1)
                        default: break
                        }
                        applyLookAt()
                    }
                    .buttonStyle(.bordered)
                    .font(.caption2)
                }
                
                Spacer()
            }
            
            // Visualization
            GeometryReader { geometry in
                ZStack {
                    // Coordinate system
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let radius = min(geometry.size.width, geometry.size.height) / 2 - 20
                        
                        // X axis (red)
                        path.move(to: center)
                        path.addLine(to: CGPoint(x: center.x + radius, y: center.y))
                        
                        // Y axis (green)
                        path.move(to: center)
                        path.addLine(to: CGPoint(x: center.x, y: center.y - radius))
                        
                        // Z axis (blue) - represented diagonally
                        path.move(to: center)
                        path.addLine(to: CGPoint(x: center.x + radius * 0.7, y: center.y + radius * 0.7))
                    }
                    .stroke(Color.gray.opacity(0.3), lineWidth: 1)
                    
                    // Look direction
                    Path { path in
                        let center = CGPoint(x: geometry.size.width / 2, y: geometry.size.height / 2)
                        let normalizedTarget = normalize(target)
                        let targetEnd = CGPoint(
                            x: center.x + CGFloat(normalizedTarget.x) * 40,
                            y: center.y - CGFloat(normalizedTarget.y) * 40
                        )
                        
                        path.move(to: center)
                        path.addLine(to: targetEnd)
                    }
                    .stroke(Color.orange, lineWidth: 3)
                }
            }
            .frame(height: 80)
        }
    }
    
    private var upVectorLength: Float {
        return sqrt(upVector.x * upVector.x + upVector.y * upVector.y + upVector.z * upVector.z)
    }
    
    private func normalizeUpVector() {
        let len = upVectorLength
        if len > 0.001 {
            upVector = upVector / len
        }
    }
    
    private func applyLookAt() {
        let normalizedTarget = normalize(target)
        let normalizedUp = normalize(upVector)
        
        // Calculate look-at rotation
        let forward = normalizedTarget
        let right = normalize(cross(normalizedUp, forward))
        let actualUp = cross(forward, right)
        
        // Create rotation matrix from basis vectors
        let rotationMatrix = SIMD4<Float>(
            SIMD4<Float>(right.x, actualUp.x, forward.x, 0),
            SIMD4<Float>(right.y, actualUp.y, forward.y, 0),
            SIMD4<Float>(right.z, actualUp.z, forward.z, 0),
            SIMD4<Float>(0, 0, 0, 1)
        )
        
        // Convert to quaternion
        value = quaternionFromMatrix(rotationMatrix)
    }
    
    private func quaternionFromMatrix(_ matrix: SIMD4<Float>) -> simd_quatf {
        let trace = matrix[0][0] + matrix[1][1] + matrix[2][2]
        
        if trace > 0 {
            let s = sqrt(trace + 1.0) * 2
            return simd_quatf(
                ix: (matrix[2][1] - matrix[1][2]) / s,
                iy: (matrix[0][2] - matrix[2][0]) / s,
                iz: (matrix[1][0] - matrix[0][1]) / s,
                r: 0.25 * s
            )
        } else if matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2] {
            let s = sqrt(1.0 + matrix[0][0] - matrix[1][1] - matrix[2][2]) * 2
            return simd_quatf(
                ix: 0.25 * s,
                iy: (matrix[0][1] + matrix[1][0]) / s,
                iz: (matrix[0][2] + matrix[2][0]) / s,
                r: (matrix[2][1] - matrix[1][2]) / s
            )
        } else if matrix[1][1] > matrix[2][2] {
            let s = sqrt(1.0 + matrix[1][1] - matrix[0][0] - matrix[2][2]) * 2
            return simd_quatf(
                ix: (matrix[0][1] + matrix[1][0]) / s,
                iy: 0.25 * s,
                iz: (matrix[1][2] + matrix[2][1]) / s,
                r: (matrix[0][2] - matrix[2][0]) / s
            )
        } else {
            let s = sqrt(1.0 + matrix[2][2] - matrix[0][0] - matrix[1][1]) * 2
            return simd_quatf(
                ix: (matrix[0][2] + matrix[2][0]) / s,
                iy: (matrix[1][2] + matrix[2][1]) / s,
                iz: 0.25 * s,
                r: (matrix[1][0] - matrix[0][1]) / s
            )
        }
    }
    
    private func normalize(_ vector: SIMD3<Float>) -> SIMD3<Float> {
        let len = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z)
        return len > 0.001 ? vector / len : SIMD3<Float>(0, 0, 1)
    }
    
    private func cross(_ a: SIMD3<Float>, _ b: SIMD3<Float>) -> SIMD3<Float> {
        return SIMD3<Float>(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        )
    }
}

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

// MARK: - Enhanced Array/List Editor with Comprehensive Features (TODO-1651 to TODO-1668)
struct EnhancedArrayPropertyEditor<Element: Identifiable & Codable>: View, PropertyEditor {
    let label: String
    @Binding var items: [Element]
    let itemView: (Binding<Element>) -> AnyView
    let createNew: () -> Element
    let itemDescription: (Element) -> String
    
    // Configuration
    let config: PropertyEditorConfig<[Element]>
    let maxItems: Int?
    let enableVirtualization: Bool
    let pageSize: Int
    
    // Protocol conformance
    var defaultValue: [Element]? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: (([Element]) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var selectedItems: Set<Element.ID> = []
    @State private var isMultiSelecting = false
    @State private var draggedItem: Element?
    @State private var searchFilter = ""
    @State private var sortOrder: SortOrder = .none
    @State private var isReversed = false
    @State private var allExpanded = true
    @State private var showClearConfirmation = false
    @State private var showImportDialog = false
    @State private var showExportDialog = false
    @State private var currentPage = 0
    @State private var showSizeWarning = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum SortOrder: String, CaseIterable {
        case none = "None"
        case ascending = "A-Z"
        case descending = "Z-A"
        case custom = "Custom"
    }
    
    private var filteredAndSortedItems: [Element] {
        var result = items
        
        // Apply search filter
        if !searchFilter.isEmpty {
            result = result.filter { itemDescription($0).localizedCaseInsensitiveContains(searchFilter) }
        }
        
        // Apply sort order
        switch sortOrder {
        case .ascending:
            result.sort { itemDescription($0) < itemDescription($1) }
        case .descending:
            result.sort { itemDescription($0) > itemDescription($1) }
        case .none, .custom:
            break
        }
        
        // Apply reverse
        if isReversed {
            result.reverse()
        }
        
        return result
    }
    
    private var paginatedItems: [Element] {
        if enableVirtualization && filteredAndSortedItems.count > pageSize {
            let startIndex = currentPage * pageSize
            let endIndex = min(startIndex + pageSize, filteredAndSortedItems.count)
            return Array(filteredAndSortedItems[startIndex..<endIndex])
        }
        return filteredAndSortedItems
    }
    
    private var totalPages: Int {
        if enableVirtualization {
            return Int(ceil(Double(filteredAndSortedItems.count) / Double(pageSize)))
        }
        return 1
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            // Header with controls
            headerSection
            
            // Size warning
            if showSizeWarning {
                sizeWarningSection
            }
            
            // Search and filter controls
            searchAndFilterSection
            
            // Items list
            itemsSection
            
            // Pagination controls
            if enableVirtualization && totalPages > 1 {
                paginationSection
            }
        }
        .sheet(isPresented: $showImportDialog) {
            ImportExportDialog(isImport: true, items: $items, itemDescription: itemDescription)
        }
        .sheet(isPresented: $showExportDialog) {
            ImportExportDialog(isImport: false, items: $items, itemDescription: itemDescription)
        }
        .alert("Clear All Items", isPresented: $showClearConfirmation) {
            Button("Cancel", role: .cancel) { }
            Button("Clear All", role: .destructive) {
                undoManager.recordState(items)
                items.removeAll()
                selectedItems.removeAll()
            }
        } message: {
            Text("Are you sure you want to remove all \(items.count) items? This action cannot be undone.")
        }
    }
    
    @ViewBuilder
    private var headerSection: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            // Item count with warning
            HStack(spacing: 4) {
                Text("\(items.count)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(itemsCountColor)
                
                if let maxItems = maxItems {
                    Text("/ \(maxItems)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Text("items")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Add button
            Button(action: addItem) {
                Image(systemName: "plus.circle")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .disabled(isLocked || (maxItems != nil && items.count >= maxItems!))
        }
    }
    
    private var itemsCountColor: Color {
        if let maxItems = maxItems {
            if items.count >= maxItems {
                return DesignSystem.Colors.accentDanger
            } else if items.count >= Int(Double(maxItems) * 0.8) {
                return DesignSystem.Colors.accentWarning
            }
        }
        return DesignSystem.Colors.textPrimary
    }
    
    @ViewBuilder
    private var sizeWarningSection: some View {
        HStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(DesignSystem.Colors.accentWarning)
            
            Text("Large array detected (\(items.count) items). Consider using pagination or virtualization for better performance.")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            Button("Dismiss") {
                showSizeWarning = false
            }
            .font(DesignSystem.Typography.small)
            .foregroundColor(DesignSystem.Colors.accentPrimary)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.pastelRed)
        .cornerRadius(DesignSystem.CornerRadius.small)
    }
    
    @ViewBuilder
    private var searchAndFilterSection: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Search field
            HStack(spacing: 6) {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .font(DesignSystem.Typography.small)
                
                TextField("Filter items...", text: $searchFilter)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                if !searchFilter.isEmpty {
                    Button(action: { searchFilter = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(DesignSystem.Spacing.xs)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.small)
            
            // Sort order picker
            Picker("", selection: $sortOrder) {
                ForEach(SortOrder.allCases, id: \.self) { order in
                    Text(order.rawValue).tag(order)
                }
            }
            .pickerStyle(.menu)
            .frame(width: 80)
            
            // Reverse button
            Button(action: { isReversed.toggle() }) {
                Image(systemName: isReversed ? "arrow.up.arrow.down" : "arrow.up.arrow.down")
                    .foregroundColor(isReversed ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
            
            // Multi-select toggle
            Button(action: { isMultiSelecting.toggle() }) {
                Image(systemName: isMultiSelecting ? "checkmark.square" : "square")
                    .foregroundColor(isMultiSelecting ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
            
            // Expand/Collapse all
            Button(action: { allExpanded.toggle() }) {
                Image(systemName: allExpanded ? "chevron.up.square" : "chevron.down.square")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
            
            Spacer()
            
            // Action menu
            Menu {
                Button("Duplicate Selected") { duplicateSelected() }
                    .disabled(selectedItems.isEmpty)
                
                Button("Move Selected to Top") { moveSelectedToTop() }
                    .disabled(selectedItems.isEmpty)
                
                Button("Move Selected to Bottom") { moveSelectedToBottom() }
                    .disabled(selectedItems.isEmpty)
                
                Divider()
                
                Button("Import from File") { showImportDialog = true }
                
                Button("Export to File") { showExportDialog = true }
                    .disabled(items.isEmpty)
                
                Divider()
                
                Button("Clear All", role: .destructive) { showClearConfirmation = true }
                    .disabled(items.isEmpty)
            } label: {
                Image(systemName: "ellipsis.circle")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
        }
    }
    
    @ViewBuilder
    private var itemsSection: some View {
        ScrollView {
            LazyVStack(spacing: DesignSystem.Spacing.xs) {
                ForEach(paginatedItems) { item in
                    ItemRow(
                        item: binding(for: item),
                        isSelected: selectedItems.contains(item.id),
                        isMultiSelecting: isMultiSelecting,
                        allExpanded: allExpanded,
                        itemView: itemView,
                        itemDescription: itemDescription,
                        onTap: { toggleSelection(item.id) },
                        onDrag: { draggedItem = item },
                        onDrop: { handleDrop(at: item) },
                        onDuplicate: { duplicateItem(item) },
                        onDelete: { deleteItem(item) }
                    )
                    .onDrag {
                        draggedItem = item
                        return NSItemProvider(object: String(item.id.hashValue) as NSString)
                    }
                    .onDrop(of: [.text], delegate: DropDelegate(
                        items: $items,
                        draggedItem: $draggedItem,
                        targetItem: item
                    ))
                }
            }
        }
        .frame(maxHeight: enableVirtualization ? 300 : .infinity)
    }
    
    @ViewBuilder
    private var paginationSection: some View {
        HStack {
            Button(action: { if currentPage > 0 { currentPage -= 1 } }) {
                Image(systemName: "chevron.left")
            }
            .buttonStyle(.plain)
            .disabled(currentPage == 0)
            
            Text("Page \(currentPage + 1) of \(totalPages)")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Button(action: { if currentPage < totalPages - 1 { currentPage += 1 } }) {
                Image(systemName: "chevron.right")
            }
            .buttonStyle(.plain)
            .disabled(currentPage >= totalPages - 1)
            
            Spacer()
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
    }
    
    // MARK: - Helper Methods
    
    private func binding(for item: Element) -> Binding<Element> {
        guard let index = items.firstIndex(where: { $0.id == item.id }) else {
            fatalError("Item not found in array")
        }
        return $items[index]
    }
    
    private func addItem() {
        guard let maxItems = maxItems else {
            items.append(createNew())
            return
        }
        
        if items.count < maxItems {
            items.append(createNew())
        } else {
            showSizeWarning = true
        }
    }
    
    private func toggleSelection(_ itemId: Element.ID) {
        if selectedItems.contains(itemId) {
            selectedItems.remove(itemId)
        } else {
            selectedItems.insert(itemId)
        }
    }
    
    private func duplicateItem(_ item: Element) {
        guard let index = items.firstIndex(where: { $0.id == item.id }) else { return }
        
        // Create a deep copy (this is a simplified approach)
        let encoder = JSONEncoder()
        let decoder = JSONDecoder()
        
        if let data = try? encoder.encode(item),
           let copy = try? decoder.decode(Element.self, from: data) {
            items.insert(copy, at: index + 1)
        }
    }
    
    private func deleteItem(_ item: Element) {
        items.removeAll { $0.id == item.id }
        selectedItems.remove(item.id)
    }
    
    private func duplicateSelected() {
        var newItems: [Element] = []
        
        for itemId in selectedItems {
            if let item = items.first(where: { $0.id == itemId }) {
                let encoder = JSONEncoder()
                let decoder = JSONDecoder()
                
                if let data = try? encoder.encode(item),
                   let copy = try? decoder.decode(Element.self, from: data) {
                    newItems.append(copy)
                }
            }
        }
        
        items.append(contentsOf: newItems)
        selectedItems.removeAll()
    }
    
    private func moveSelectedToTop() {
        let selectedItemsArray = items.filter { selectedItems.contains($0.id) }
        let otherItems = items.filter { !selectedItems.contains($0.id) }
        items = selectedItemsArray + otherItems
    }
    
    private func moveSelectedToBottom() {
        let selectedItemsArray = items.filter { selectedItems.contains($0.id) }
        let otherItems = items.filter { !selectedItems.contains($0.id) }
        items = otherItems + selectedItemsArray
    }
    
    private func handleDrop(at targetItem: Element) {
        guard let draggedItem = draggedItem,
              let draggedIndex = items.firstIndex(where: { $0.id == draggedItem.id }),
              let targetIndex = items.firstIndex(where: { $0.id == targetItem.id }) else {
            return
        }
        
        let item = items.remove(at: draggedIndex)
        items.insert(item, at: targetIndex)
        self.draggedItem = nil
    }
}

// MARK: - Item Row Component
private struct ItemRow<Element: Identifiable>: View {
    @Binding var item: Element
    let isSelected: Bool
    let isMultiSelecting: Bool
    let allExpanded: Bool
    let itemView: (Binding<Element>) -> AnyView
    let itemDescription: (Element) -> String
    let onTap: () -> Void
    let onDrag: () -> Void
    let onDrop: () -> Void
    let onDuplicate: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Multi-select checkbox
            if isMultiSelecting {
                Button(action: onTap) {
                    Image(systemName: isSelected ? "checkmark.square.fill" : "square")
                        .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
            }
            
            // Drag handle
            Image(systemName: "line.3.horizontal")
                .foregroundColor(DesignSystem.Colors.textTertiary)
                .font(DesignSystem.Typography.small)
                .onDrag { onDrag(); return NSItemProvider() }
            
            // Item content
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.xxs) {
                if allExpanded {
                    itemView($item)
                } else {
                    Text(itemDescription(item))
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                }
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            
            // Action buttons
            HStack(spacing: 4) {
                Button(action: onDuplicate) {
                    Image(systemName: "doc.on.doc")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                
                Button(action: onDelete) {
                    Image(systemName: "trash")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.accentDanger)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .fill(isSelected ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
        )
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - Drop Delegate
private struct DropDelegate<Element: Identifiable>: DropDelegate {
    @Binding var items: [Element]
    @Binding var draggedItem: Element?
    let targetItem: Element
    
    func performDrop(info: DropInfo) -> Bool {
        draggedItem = nil
        return true
    }
    
    func dropEntered(info: DropInfo) {
        guard let draggedItem = draggedItem,
              let draggedIndex = items.firstIndex(where: { $0.id == draggedItem.id }),
              let targetIndex = items.firstIndex(where: { $0.id == targetItem.id }),
              draggedIndex != targetIndex else {
            return
        }
        
        let item = items.remove(at: draggedIndex)
        items.insert(item, at: targetIndex)
    }
}

// MARK: - Import/Export Dialog
private struct ImportExportDialog<Element: Identifiable & Codable>: View {
    let isImport: Bool
    @Binding var items: [Element]
    let itemDescription: (Element) -> String
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedFile: URL?
    @State private var importText = ""
    @State private var exportText = ""
    @State private var showAlert = false
    @State private var alertMessage = ""
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Text(isImport ? "Import Items" : "Export Items")
                .font(DesignSystem.Typography.title3)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            if isImport {
                importSection
            } else {
                exportSection
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button(isImport ? "Import" : "Export") {
                    if isImport {
                        performImport()
                    } else {
                        performExport()
                    }
                }
                .buttonStyle(.borderedProminent)
                .disabled(isImport ? importText.isEmpty : false)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 500, height: 400)
        .alert("Import/Export", isPresented: $showAlert) {
            Button("OK") { }
        } message: {
            Text(alertMessage)
        }
    }
    
    @ViewBuilder
    private var importSection: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text("Paste JSON data or select a file:")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            TextEditor(text: $importText)
                .font(DesignSystem.Typography.mono)
                .padding(DesignSystem.Spacing.xs)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(DesignSystem.CornerRadius.small)
        }
    }
    
    @ViewBuilder
    private var exportSection: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            Text("JSON data for \(items.count) items:")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            ScrollView {
                Text(exportText)
                    .font(DesignSystem.Typography.mono)
                    .frame(maxWidth: .infinity, alignment: .leading)
            }
            .padding(DesignSystem.Spacing.xs)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.small)
        }
        .onAppear {
            generateExportText()
        }
    }
    
    private func performImport() {
        do {
            guard let data = importText.data(using: .utf8) else {
                throw ImportError.invalidData
            }
            
            let decoder = JSONDecoder()
            let importedItems = try decoder.decode([Element].self, from: data)
            items.append(contentsOf: importedItems)
            dismiss()
        } catch {
            alertMessage = "Failed to import data: \(error.localizedDescription)"
            showAlert = true
        }
    }
    
    private func performExport() {
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.json]
        panel.nameFieldStringValue = "array_export.json"
        
        if panel.runModal() == .OK, let url = panel.url {
            do {
                let data = try JSONEncoder().encode(items)
                try data.write(to: url)
                dismiss()
            } catch {
                alertMessage = "Failed to export data: \(error.localizedDescription)"
                showAlert = true
            }
        }
    }
    
    private func generateExportText() {
        do {
            let data = try JSONEncoder().encode(items)
            exportText = String(data: data, encoding: .utf8) ?? "Failed to encode data"
        } catch {
            exportText = "Failed to encode data: \(error.localizedDescription)"
        }
    }
    
    enum ImportError: LocalizedError {
        case invalidData
        
        var errorDescription: String? {
            switch self {
            case .invalidData:
                return "Invalid data format"
            }
        }
    }
}

// MARK: - Dictionary Editor with Key-Value Pairs (TODO-1664)
struct DictionaryPropertyEditor<Key: Hashable & Codable, Value: Codable & Identifiable>: View, PropertyEditor {
    let label: String
    @Binding var dictionary: [Key: Value]
    let keyView: (Binding<Key>) -> AnyView
    let valueView: (Binding<Value>) -> AnyView
    let createNewKey: () -> Key
    let createNewValue: () -> Value
    
    // Configuration
    let config: PropertyEditorConfig<[Key: Value]>
    let maxItems: Int?
    
    // Protocol conformance
    var defaultValue: [Key: Value]? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: (([Key: Value]) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var selectedKeys: Set<Key> = []
    @State private var searchFilter = ""
    @State private var sortOrder: DictionarySortOrder = .keyAscending
    @State private var showClearConfirmation = false
    @State private var showAddDialog = false
    @State private var newKey = ""
    @State private var showSizeWarning = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    
    enum DictionarySortOrder: String, CaseIterable {
        case keyAscending = "Key ↑"
        case keyDescending = "Key ↓"
        case valueAscending = "Value ↑"
        case valueDescending = "Value ↓"
        case none = "None"
    }
    
    private var filteredAndSortedItems: [(key: Key, value: Value)] {
        var result = Array(dictionary)
        
        // Apply search filter
        if !searchFilter.isEmpty {
            result = result.filter { pair in
                "\(pair.key)".localizedCaseInsensitiveContains(searchFilter) ||
                "\(pair.value)".localizedCaseInsensitiveContains(searchFilter)
            }
        }
        
        // Apply sort order
        switch sortOrder {
        case .keyAscending:
            result.sort { "\($0.key)" < "\($1.key)" }
        case .keyDescending:
            result.sort { "\($0.key)" > "\($1.key)" }
        case .valueAscending:
            result.sort { "\($0.value)" < "\($1.value)" }
        case .valueDescending:
            result.sort { "\($0.value)" > "\($1.value)" }
        case .none:
            break
        }
        
        return result
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            // Header
            headerSection
            
            // Size warning
            if showSizeWarning {
                sizeWarningSection
            }
            
            // Search and filter controls
            searchAndFilterSection
            
            // Dictionary entries
            entriesSection
        }
        .sheet(isPresented: $showAddDialog) {
            AddKeyValueDialog(
                newKey: $newKey,
                onAdd: { key, value in
                    dictionary[key] = value
                    showAddDialog = false
                    newKey = ""
                },
                createNewKey: createNewKey,
                createNewValue: createNewValue
            )
        }
        .alert("Clear All Entries", isPresented: $showClearConfirmation) {
            Button("Cancel", role: .cancel) { }
            Button("Clear All", role: .destructive) {
                undoManager.recordState(dictionary)
                dictionary.removeAll()
                selectedKeys.removeAll()
            }
        } message: {
            Text("Are you sure you want to remove all \(dictionary.count) entries? This action cannot be undone.")
        }
    }
    
    @ViewBuilder
    private var headerSection: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            // Entry count with warning
            HStack(spacing: 4) {
                Text("\(dictionary.count)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(entryCountColor)
                
                if let maxItems = maxItems {
                    Text("/ \(maxItems)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Text("entries")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Add button
            Button(action: { showAddDialog = true }) {
                Image(systemName: "plus.circle")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .disabled(isLocked || (maxItems != nil && dictionary.count >= maxItems!))
        }
    }
    
    private var entryCountColor: Color {
        if let maxItems = maxItems {
            if dictionary.count >= maxItems {
                return DesignSystem.Colors.accentDanger
            } else if dictionary.count >= Int(Double(maxItems) * 0.8) {
                return DesignSystem.Colors.accentWarning
            }
        }
        return DesignSystem.Colors.textPrimary
    }
    
    @ViewBuilder
    private var sizeWarningSection: some View {
        HStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(DesignSystem.Colors.accentWarning)
            
            Text("Large dictionary detected (\(dictionary.count) entries). Consider using pagination for better performance.")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            Button("Dismiss") {
                showSizeWarning = false
            }
            .font(DesignSystem.Typography.small)
            .foregroundColor(DesignSystem.Colors.accentPrimary)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.pastelRed)
        .cornerRadius(DesignSystem.CornerRadius.small)
    }
    
    @ViewBuilder
    private var searchAndFilterSection: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Search field
            HStack(spacing: 6) {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .font(DesignSystem.Typography.small)
                
                TextField("Filter keys/values...", text: $searchFilter)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                if !searchFilter.isEmpty {
                    Button(action: { searchFilter = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(DesignSystem.Spacing.xs)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.small)
            
            // Sort order picker
            Picker("", selection: $sortOrder) {
                ForEach(DictionarySortOrder.allCases, id: \.self) { order in
                    Text(order.rawValue).tag(order)
                }
            }
            .pickerStyle(.menu)
            .frame(width: 100)
            
            Spacer()
            
            // Action menu
            Menu {
                Button("Clear All", role: .destructive) { showClearConfirmation = true }
                    .disabled(dictionary.isEmpty)
            } label: {
                Image(systemName: "ellipsis.circle")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
        }
    }
    
    @ViewBuilder
    private var entriesSection: some View {
        ScrollView {
            LazyVStack(spacing: DesignSystem.Spacing.xs) {
                ForEach(filteredAndSortedItems, id: \.key) { pair in
                    DictionaryEntryRow(
                        key: bindingForKey(pair.key),
                        value: bindingForValue(pair.key),
                        isSelected: selectedKeys.contains(pair.key),
                        keyString: "\(pair.key)",
                        valueString: "\(pair.value)",
                        onSelect: { toggleSelection(pair.key) },
                        onDelete: { deleteEntry(pair.key) }
                    )
                }
            }
        }
        .frame(maxHeight: 300)
    }
    
    // MARK: - Helper Methods
    
    private func bindingForKey(_ key: Key) -> Binding<Key> {
        Binding<Key>(
            get: { key },
            set: { newValue in
                if let value = dictionary[key] {
                    dictionary.removeValue(forKey: key)
                    dictionary[newValue] = value
                }
            }
        )
    }
    
    private func bindingForValue(_ key: Key) -> Binding<Value> {
        Binding<Value>(
            get: { dictionary[key]! },
            set: { dictionary[key] = $0 }
        )
    }
    
    private func toggleSelection(_ key: Key) {
        if selectedKeys.contains(key) {
            selectedKeys.remove(key)
        } else {
            selectedKeys.insert(key)
        }
    }
    
    private func deleteEntry(_ key: Key) {
        dictionary.removeValue(forKey: key)
        selectedKeys.remove(key)
    }
}

// MARK: - Dictionary Entry Row
private struct DictionaryEntryRow<Key: Hashable, Value: Identifiable>: View {
    @Binding var key: Key
    @Binding var value: Value
    let isSelected: Bool
    let keyString: String
    let valueString: String
    let onSelect: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Selection checkbox
            Button(action: onSelect) {
                Image(systemName: isSelected ? "checkmark.square.fill" : "square")
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
            
            // Key column
            VStack(alignment: .leading, spacing: 2) {
                Text("Key:")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(keyString)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            
            // Separator
            Rectangle()
                .fill(DesignSystem.Colors.border)
                .frame(width: 1)
                .padding(.horizontal, DesignSystem.Spacing.xs)
            
            // Value column
            VStack(alignment: .leading, spacing: 2) {
                Text("Value:")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(valueString)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            
            // Delete button
            Button(action: onDelete) {
                Image(systemName: "trash")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
            }
            .buttonStyle(.plain)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .fill(isSelected ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
        )
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - Add Key-Value Dialog
private struct AddKeyValueDialog<Key: Hashable, Value: Identifiable>: View {
    @Binding var newKey: String
    let onAdd: (Key, Value) -> Void
    let createNewKey: () -> Key
    let createNewValue: () -> Value
    @Environment(\.dismiss) private var dismiss
    
    @State private var keyValue: Key
    @State private var valueValue: Value
    
    init(newKey: Binding<String>, onAdd: @escaping (Key, Value) -> Void, createNewKey: @escaping () -> Key, createNewValue: @escaping () -> Value) {
        self._newKey = newKey
        self.onAdd = onAdd
        self.createNewKey = createNewKey
        self.createNewValue = createNewValue
        self._keyValue = State(initialValue: createNewKey())
        self._valueValue = State(initialValue: createNewValue())
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Text("Add Dictionary Entry")
                .font(DesignSystem.Typography.title3)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("Key:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                TextField("Enter key...", text: $newKey)
                    .textFieldStyle(.roundedBorder)
                
                Text("Value:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Text("\(valueValue)")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(DesignSystem.CornerRadius.small)
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    onAdd(keyValue, valueValue)
                }
                .buttonStyle(.borderedProminent)
                .disabled(newKey.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400)
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

// MARK: - Layer Mask Editor with Visual Checkboxes (TODO-1751)
struct LayerMaskPropertyEditor: View {
    let label: String
    @Binding var mask: Int32
    @State private var layerNames: [String] = [
        "Default", "TransparentFX", "Ignore Raycast", "Water", "UI", 
        "PostProcessing", "Player", "Enemy", "Pickup", "Scenery",
        "Projectile", "Static", "Dynamic", "Trigger", "NoCollision"
    ]
    @State private var layerColors: [Color] = [
        .gray, .cyan, .yellow, .blue, .purple,
        .orange, .green, .red, .mint, .indigo,
        .pink, .brown, .teal, .white, .black
    ]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 3), spacing: 4) {
                ForEach(0..<min(layerNames.count, 32), id: \.self) { index in
                    HStack(spacing: 4) {
                        Button(action: { toggleLayer(index) }) {
                            Image(systemName: isLayerActive(index) ? "checkmark.square.fill" : "square")
                                .foregroundColor(isLayerActive(index) ? layerColors[index % layerColors.count] : DesignSystem.Colors.textTertiary)
                                .font(DesignSystem.Typography.small)
                        }
                        .buttonStyle(.plain)
                        
                        Text(layerNames[index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        Text("\(index)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .frame(width: 20, alignment: .trailing)
                    }
                    .padding(.horizontal, 8)
                    .padding(.vertical, 2)
                    .background(isLayerActive(index) ? layerColors[index % layerColors.count].opacity(0.1) : DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
            }
            
            HStack {
                Text("Mask Value:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Text(String(mask, radix: 2).prefix(32))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Text("Decimal: \(mask)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(4)
        }
    }
    
    private func isLayerActive(_ index: Int) -> Bool {
        return (mask & (1 << index)) != 0
    }
    
    private func toggleLayer(_ index: Int) {
        if isLayerActive(index) {
            mask &= ~(1 << index)
        } else {
            mask |= (1 << index)
        }
    }
}

// MARK: - Set Editor with Unique Value Enforcement (TODO-1665)
struct SetPropertyEditor<Element: Hashable & Codable & Identifiable>: View, PropertyEditor {
    let label: String
    @Binding var set: Set<Element>
    let itemView: (Binding<Element>) -> AnyView
    let createNew: () -> Element
    
    // Configuration
    let config: PropertyEditorConfig<Set<Element>>
    let maxItems: Int?
    
    // Protocol conformance
    var defaultValue: Set<Element>? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Set<Element>) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var selectedItems: Set<Element> = []
    @State private var searchFilter = ""
    @State private var sortOrder: SetSortOrder = .none
    @State private var showClearConfirmation = false
    @State private var showAddDialog = false
    @State private var duplicateWarning: String? = nil
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    
    enum SetSortOrder: String, CaseIterable {
        case none = "None"
        case ascending = "A-Z"
        case descending = "Z-A"
    }
    
    private var filteredAndSortedItems: [Element] {
        var result = Array(set)
        
        // Apply search filter
        if !searchFilter.isEmpty {
            result = result.filter { item in
                "\(item)".localizedCaseInsensitiveContains(searchFilter)
            }
        }
        
        // Apply sort order
        switch sortOrder {
        case .ascending:
            result.sort { "\($0)" < "\($1)" }
        case .descending:
            result.sort { "\($0)" > "\($1)" }
        case .none:
            break
        }
        
        return result
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            // Header
            headerSection
            
            // Duplicate warning
            if let warning = duplicateWarning {
                duplicateWarningSection(warning)
            }
            
            // Search and filter controls
            searchAndFilterSection
            
            // Set entries
            entriesSection
        }
        .sheet(isPresented: $showAddDialog) {
            AddSetItemDialog(
                onAdd: { item in
                    if set.contains(item) {
                        duplicateWarning = "Item already exists in set"
                        DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
                            duplicateWarning = nil
                        }
                    } else {
                        set.insert(item)
                        showAddDialog = false
                    }
                },
                createNew: createNew
            )
        }
        .alert("Clear All Items", isPresented: $showClearConfirmation) {
            Button("Cancel", role: .cancel) { }
            Button("Clear All", role: .destructive) {
                undoManager.recordState(set)
                set.removeAll()
                selectedItems.removeAll()
            }
        } message: {
            Text("Are you sure you want to remove all \(set.count) unique items? This action cannot be undone.")
        }
    }
    
    @ViewBuilder
    private var headerSection: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            // Item count with warning
            HStack(spacing: 4) {
                Text("\(set.count)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(entryCountColor)
                
                if let maxItems = maxItems {
                    Text("/ \(maxItems)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Text("unique items")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Add button
            Button(action: { showAddDialog = true }) {
                Image(systemName: "plus.circle")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .disabled(isLocked || (maxItems != nil && set.count >= maxItems!))
        }
    }
    
    private var entryCountColor: Color {
        if let maxItems = maxItems {
            if set.count >= maxItems {
                return DesignSystem.Colors.accentDanger
            } else if set.count >= Int(Double(maxItems) * 0.8) {
                return DesignSystem.Colors.accentWarning
            }
        }
        return DesignSystem.Colors.textPrimary
    }
    
    @ViewBuilder
    private func duplicateWarningSection(_ warning: String) -> some View {
        HStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(DesignSystem.Colors.accentWarning)
            
            Text(warning)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.pastelRed)
        .cornerRadius(DesignSystem.CornerRadius.small)
    }
    
    @ViewBuilder
    private var searchAndFilterSection: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Search field
            HStack(spacing: 6) {
                Image(systemName: "magnifyingglass")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                    .font(DesignSystem.Typography.small)
                
                TextField("Filter items...", text: $searchFilter)
                    .textFieldStyle(.plain)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                if !searchFilter.isEmpty {
                    Button(action: { searchFilter = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(DesignSystem.Spacing.xs)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.small)
            
            // Sort order picker
            Picker("", selection: $sortOrder) {
                ForEach(SetSortOrder.allCases, id: \.self) { order in
                    Text(order.rawValue).tag(order)
                }
            }
            .pickerStyle(.menu)
            .frame(width: 80)
            
            Spacer()
            
            // Action menu
            Menu {
                Button("Clear All", role: .destructive) { showClearConfirmation = true }
                    .disabled(set.isEmpty)
            } label: {
                Image(systemName: "ellipsis.circle")
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
        }
    }
    
    @ViewBuilder
    private var entriesSection: some View {
        ScrollView {
            LazyVStack(spacing: DesignSystem.Spacing.xs) {
                ForEach(filteredAndSortedItems, id: \.self) { item in
                    SetItemRow(
                        item: bindingForItem(item),
                        isSelected: selectedItems.contains(item),
                        itemString: "\(item)",
                        onSelect: { toggleSelection(item) },
                        onDelete: { deleteItem(item) }
                    )
                }
            }
        }
        .frame(maxHeight: 300)
    }
    
    // MARK: - Helper Methods
    
    private func bindingForItem(_ item: Element) -> Binding<Element> {
        Binding<Element>(
            get: { item },
            set: { newValue in
                set.remove(item)
                set.insert(newValue)
            }
        )
    }
    
    private func toggleSelection(_ item: Element) {
        if selectedItems.contains(item) {
            selectedItems.remove(item)
        } else {
            selectedItems.insert(item)
        }
    }
    
    private func deleteItem(_ item: Element) {
        set.remove(item)
        selectedItems.remove(item)
    }
}

// MARK: - Set Item Row
private struct SetItemRow<Element: Hashable & Identifiable>: View {
    @Binding var item: Element
    let isSelected: Bool
    let itemString: String
    let onSelect: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Selection checkbox
            Button(action: onSelect) {
                Image(systemName: isSelected ? "checkmark.square.fill" : "square")
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textTertiary)
            }
            .buttonStyle(.plain)
            
            // Item content
            VStack(alignment: .leading, spacing: 2) {
                Text("Item:")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(itemString)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
            
            // Delete button
            Button(action: onDelete) {
                Image(systemName: "trash")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentDanger)
            }
            .buttonStyle(.plain)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .fill(isSelected ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
        )
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .stroke(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - Add Set Item Dialog
private struct AddSetItemDialog<Element: Hashable & Identifiable>: View {
    let onAdd: (Element) -> Void
    let createNew: () -> Element
    @Environment(\.dismiss) private var dismiss
    
    @State private var newItem: Element
    
    init(onAdd: @escaping (Element) -> Void, createNew: @escaping () -> Element) {
        self.onAdd = onAdd
        self.createNew = createNew
        self._newItem = State(initialValue: createNew())
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Text("Add Set Item")
                .font(DesignSystem.Typography.title3)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("Item:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Text("\(newItem)")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(DesignSystem.CornerRadius.small)
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    onAdd(newItem)
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400)
    }
}

// MARK: - Queue/Stack Editor Visualization (TODO-1666)
struct CollectionStructurePropertyEditor<Element: Codable & Identifiable>: View, PropertyEditor {
    let label: String
    @Binding var collection: [Element]
    let structureType: CollectionStructureType
    let itemView: (Binding<Element>) -> AnyView
    let createNew: () -> Element
    
    enum CollectionStructureType {
        case queue
        case stack
        
        var name: String {
            switch self {
            case .queue: return "Queue"
            case .stack: return "Stack"
            }
        }
        
        var iconName: String {
            switch self {
            case .queue: return "arrow.right.circle"
            case .stack: return "arrow.up.circle"
            }
        }
        
        var operationName: String {
            switch self {
            case .queue: return "Enqueue"
            case .stack: return "Push"
            }
        }
        
        var removeOperationName: String {
            switch self {
            case .queue: return "Dequeue"
            case .stack: return "Pop"
            }
        }
    }
    
    // Configuration
    let config: PropertyEditorConfig<[Element]>
    let maxItems: Int?
    
    // Protocol conformance
    var defaultValue: [Element]? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: (([Element]) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var showAddDialog = false
    @State private var showRemoveConfirmation = false
    @State private var showClearConfirmation = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            // Header
            headerSection
            
            // Structure visualization
            structureVisualization
            
            // Controls
            controlsSection
        }
        .sheet(isPresented: $showAddDialog) {
            AddCollectionItemDialog(
                structureType: structureType,
                onAdd: { item in
                    if structureType == .queue {
                        collection.append(item)
                    } else {
                        collection.insert(item, at: 0)
                    }
                    showAddDialog = false
                },
                createNew: createNew
            )
        }
        .alert("Remove Item", isPresented: $showRemoveConfirmation) {
            Button("Cancel", role: .cancel) { }
            Button("Remove", role: .destructive) {
                if !collection.isEmpty {
                    undoManager.recordState(collection)
                    if structureType == .queue {
                        collection.removeFirst()
                    } else {
                        collection.removeFirst()
                    }
                }
            }
        } message: {
            Text("Are you sure you want to \(structureType.removeOperationName.lowercased()) the top item?")
        }
        .alert("Clear All Items", isPresented: $showClearConfirmation) {
            Button("Cancel", role: .cancel) { }
            Button("Clear All", role: .destructive) {
                undoManager.recordState(collection)
                collection.removeAll()
            }
        } message: {
            Text("Are you sure you want to clear all \(collection.count) items from the \(structureType.name.lowercased())?")
        }
    }
    
    @ViewBuilder
    private var headerSection: some View {
        HStack {
            HStack(spacing: 6) {
                Image(systemName: structureType.iconName)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Text("(\(structureType.name))")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            Spacer()
            
            // Item count
            HStack(spacing: 4) {
                Text("\(collection.count)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(entryCountColor)
                
                if let maxItems = maxItems {
                    Text("/ \(maxItems)")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Text("items")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
        }
    }
    
    private var entryCountColor: Color {
        if let maxItems = maxItems {
            if collection.count >= maxItems {
                return DesignSystem.Colors.accentDanger
            } else if collection.count >= Int(Double(maxItems) * 0.8) {
                return DesignSystem.Colors.accentWarning
            }
        }
        return DesignSystem.Colors.textPrimary
    }
    
    @ViewBuilder
    private var structureVisualization: some View {
        VStack(spacing: DesignSystem.Spacing.xs) {
            // Top/Bottom indicators
            HStack {
                if structureType == .stack {
                    Text("Top")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                } else {
                    Text("Front")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                Spacer()
                if structureType == .stack {
                    Text("Bottom")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                } else {
                    Text("Back")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
            }
            
            // Visual stack/queue representation
            ScrollView {
                LazyVStack(spacing: 2) {
                    ForEach(Array(collection.enumerated()), id: \.element.id) { index, item in
                        CollectionStructureItemRow(
                            item: bindingForItem(index),
                            itemString: "\(item)",
                            index: index,
                            isTop: structureType == .stack ? index == 0 : index == collection.count - 1,
                            structureType: structureType
                        )
                    }
                }
            }
            .frame(maxHeight: 200)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(DesignSystem.CornerRadius.small)
    }
    
    @ViewBuilder
    private var controlsSection: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Add button
            Button(action: { showAddDialog = true }) {
                HStack(spacing: 4) {
                    Image(systemName: "plus.circle")
                    Text(structureType.operationName)
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            .disabled(isLocked || (maxItems != nil && collection.count >= maxItems!))
            
            // Remove button
            Button(action: { showRemoveConfirmation = true }) {
                HStack(spacing: 4) {
                    Image(systemName: "minus.circle")
                    Text(structureType.removeOperationName)
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentWarning)
            }
            .buttonStyle(.plain)
            .disabled(isLocked || collection.isEmpty)
            
            Spacer()
            
            // Clear button
            Button(action: { showClearConfirmation = true }) {
                HStack(spacing: 4) {
                    Image(systemName: "trash")
                    Text("Clear")
                }
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.accentDanger)
            }
            .buttonStyle(.plain)
            .disabled(isLocked || collection.isEmpty)
        }
    }
    
    // MARK: - Helper Methods
    
    private func bindingForItem(_ index: Int) -> Binding<Element> {
        Binding<Element>(
            get: { collection[index] },
            set: { collection[index] = $0 }
        )
    }
}

// MARK: - Collection Structure Item Row
private struct CollectionStructureItemRow<Element: Identifiable>: View {
    @Binding var item: Element
    let itemString: String
    let index: Int
    let isTop: Bool
    let structureType: CollectionStructurePropertyEditor<Element>.CollectionStructureType
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.xs) {
            // Position indicator
            VStack {
                if isTop {
                    Image(systemName: structureType == .stack ? "arrow.up.circle.fill" : "arrow.right.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                        .font(DesignSystem.Typography.small)
                } else {
                    Circle()
                        .fill(DesignSystem.Colors.border)
                        .frame(width: 8, height: 8)
                }
            }
            .frame(width: 20)
            
            // Item content
            VStack(alignment: .leading, spacing: 2) {
                Text("Item \(index)")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(itemString)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                    .truncationMode(.tail)
            }
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .fill(isTop ? DesignSystem.Colors.accentPrimary.opacity(0.1) : DesignSystem.Colors.backgroundPrimary)
        )
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                .stroke(isTop ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: 1)
        )
    }
}

// MARK: - Add Collection Item Dialog
private struct AddCollectionItemDialog<Element: Identifiable>: View {
    let structureType: CollectionStructurePropertyEditor<Element>.CollectionStructureType
    let onAdd: (Element) -> Void
    let createNew: () -> Element
    @Environment(\.dismiss) private var dismiss
    
    @State private var newItem: Element
    
    init(structureType: CollectionStructurePropertyEditor<Element>.CollectionStructureType, onAdd: @escaping (Element) -> Void, createNew: @escaping () -> Element) {
        self.structureType = structureType
        self.onAdd = onAdd
        self.createNew = createNew
        self._newItem = State(initialValue: createNew())
    }
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Text("\(structureType.operationName) Item")
                .font(DesignSystem.Typography.title3)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("New Item:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Text("\(newItem)")
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(DesignSystem.Spacing.xs)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(DesignSystem.CornerRadius.small)
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button(structureType.operationName) {
                    onAdd(newItem)
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400)
    }
}

// MARK: - Nested Collection Editors (TODO-1667)
struct NestedCollectionPropertyEditor: View {
    let label: String
    @Binding var nestedData: NestedCollectionData
    let maxDepth: Int
    let maxItemsPerLevel: Int
    
    struct NestedCollectionData: Codable {
        var arrays: [String: [String]]
        var dictionaries: [String: [String: String]]
        var sets: [String: Set<String>]
        
        init() {
            arrays = [:]
            dictionaries = [:]
            sets = [:]
        }
    }
    
    // State
    @State private var expandedSections: Set<String> = []
    @State private var selectedPath: String? = nil
    @State private var showAddDialog = false
    @State private var newCollectionType: CollectionType = .array
    @State private var newCollectionName = ""
    
    enum CollectionType: String, CaseIterable {
        case array = "Array"
        case dictionary = "Dictionary"
        case set = "Set"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
            // Header
            headerSection
            
            // Nested collection tree
            nestedCollectionTree
        }
        .sheet(isPresented: $showAddDialog) {
            AddNestedCollectionDialog(
                collectionType: $newCollectionType,
                collectionName: $newCollectionName,
                onAdd: { type, name in
                    addNestedCollection(type: type, name: name)
                    showAddDialog = false
                    newCollectionName = ""
                }
            )
        }
    }
    
    @ViewBuilder
    private var headerSection: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            // Total count
            Text("\(nestedData.arrays.count + nestedData.dictionaries.count + nestedData.sets.count) collections")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            // Add button
            Button(action: { showAddDialog = true }) {
                Image(systemName: "plus.circle")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
        }
    }
    
    @ViewBuilder
    private var nestedCollectionTree: some View {
        ScrollView {
            LazyVStack(spacing: DesignSystem.Spacing.xs) {
                // Arrays section
                NestedCollectionSection(
                    title: "Arrays",
                    items: Array(nestedData.arrays.keys),
                    expandedSections: $expandedSections,
                    selectedPath: $selectedPath,
                    sectionKey: "arrays",
                    icon: "list.bullet",
                    color: DesignSystem.Colors.accentPrimary
                )
                
                // Dictionaries section
                NestedCollectionSection(
                    title: "Dictionaries",
                    items: Array(nestedData.dictionaries.keys),
                    expandedSections: $expandedSections,
                    selectedPath: $selectedPath,
                    sectionKey: "dictionaries",
                    icon: "list.bullet.indent",
                    color: DesignSystem.Colors.accentSecondary
                )
                
                // Sets section
                NestedCollectionSection(
                    title: "Sets",
                    items: Array(nestedData.sets.keys),
                    expandedSections: $expandedSections,
                    selectedPath: $selectedPath,
                    sectionKey: "sets",
                    icon: "list.bullet.rectangle",
                    color: DesignSystem.Colors.accentTertiary
                )
            }
        }
        .frame(maxHeight: 400)
    }
    
    // MARK: - Helper Methods
    
    private func addNestedCollection(type: CollectionType, name: String) {
        guard !name.isEmpty else { return }
        
        switch type {
        case .array:
            nestedData.arrays[name] = []
        case .dictionary:
            nestedData.dictionaries[name] = [:]
        case .set:
            nestedData.sets[name] = []
        }
        
        expandedSections.insert("\(type.rawValue)_\(name)")
    }
}

// MARK: - Nested Collection Section
private struct NestedCollectionSection: View {
    let title: String
    let items: [String]
    @Binding var expandedSections: Set<String>
    @Binding var selectedPath: String?
    let sectionKey: String
    let icon: String
    let color: Color
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.xs) {
            // Section header
            Button(action: {
                if expandedSections.contains(sectionKey) {
                    expandedSections.remove(sectionKey)
                } else {
                    expandedSections.insert(sectionKey)
                }
            }) {
                HStack(spacing: 6) {
                    Image(systemName: expandedSections.contains(sectionKey) ? "chevron.down" : "chevron.right")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .font(DesignSystem.Typography.micro)
                    
                    Image(systemName: icon)
                        .foregroundColor(color)
                        .font(DesignSystem.Typography.small)
                    
                    Text(title)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("(\(items.count))")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Spacer()
                }
            }
            .buttonStyle(.plain)
            
            // Section items
            if expandedSections.contains(sectionKey) {
                VStack(spacing: 2) {
                    ForEach(items, id: \.self) { item in
                        NestedCollectionItem(
                            name: item,
                            path: "\(sectionKey).\(item)",
                            selectedPath: $selectedPath,
                            color: color
                        )
                    }
                }
                .padding(.leading, DesignSystem.Spacing.lg)
            }
        }
    }
}

// MARK: - Nested Collection Item
private struct NestedCollectionItem: View {
    let name: String
    let path: String
    @Binding var selectedPath: String?
    let color: Color
    
    var body: some View {
        Button(action: {
            selectedPath = path
        }) {
            HStack(spacing: 6) {
                Circle()
                    .fill(color)
                    .frame(width: 6, height: 6)
                
                Text(name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                    .truncationMode(.tail)
                
                Spacer()
            }
            .padding(.horizontal, DesignSystem.Spacing.sm)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                    .fill(selectedPath == path ? DesignSystem.Colors.selection : DesignSystem.Colors.backgroundTertiary)
            )
            .overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.small)
                    .stroke(selectedPath == path ? color : DesignSystem.Colors.border, lineWidth: 1)
            )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Add Nested Collection Dialog
private struct AddNestedCollectionDialog: View {
    @Binding var collectionType: NestedCollectionPropertyEditor.CollectionType
    @Binding var collectionName: String
    let onAdd: (NestedCollectionPropertyEditor.CollectionType, String) -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.md) {
            Text("Add Nested Collection")
                .font(DesignSystem.Typography.title3)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            VStack(alignment: .leading, spacing: DesignSystem.Spacing.sm) {
                Text("Collection Type:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Picker("", selection: $collectionType) {
                    ForEach(NestedCollectionPropertyEditor.CollectionType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type)
                    }
                }
                .pickerStyle(.segmented)
                
                Text("Collection Name:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                TextField("Enter collection name...", text: $collectionName)
                    .textFieldStyle(.roundedBorder)
            }
            
            HStack {
                Button("Cancel") { dismiss() }
                    .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Add") {
                    onAdd(collectionType, collectionName)
                }
                .buttonStyle(.borderedProminent)
                .disabled(collectionName.isEmpty)
            }
        }
        .padding(DesignSystem.Spacing.lg)
        .frame(width: 400)
    }
}

// MARK: - Rendering Layer Mask Editor (TODO-1752)
struct RenderingLayerMaskPropertyEditor: View {
    let label: String
    @Binding var mask: Int32
    @State private var renderingLayers: [String] = [
        "Background", "Skybox", "Geometry", "Transparent", "Effects",
        "Overlay", "UI", "PostProcess", "Debug", "Wireframe"
    ]
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 2), spacing: 4) {
                ForEach(0..<min(renderingLayers.count, 32), id: \.self) { index in
                    HStack(spacing: 4) {
                        Toggle("", isOn: Binding(
                            get: { isLayerActive(index) },
                            set: { _ in toggleLayer(index) }
                        ))
                        .toggleStyle(.switch)
                        
                        Text(renderingLayers[index])
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        Text("\(index)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .frame(width: 20, alignment: .trailing)
                    }
                    .padding(.horizontal, 8)
                    .padding(.vertical, 2)
                    .background(isLayerActive(index) ? DesignSystem.Colors.accentPrimary.opacity(0.1) : DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
            }
        }
    }
    
    private func isLayerActive(_ index: Int) -> Bool {
        return (mask & (1 << index)) != 0
    }
    
    private func toggleLayer(_ index: Int) {
        if isLayerActive(index) {
            mask &= ~(1 << index)
        } else {
            mask |= (1 << index)
        }
    }
}

// MARK: - Physics Layer Mask Matrix (TODO-1753)
struct PhysicsLayerMatrixPropertyEditor: View {
    let label: String
    @Binding var matrix: [[Bool]]
    @State private var layerNames: [String] = [
        "Default", "Player", "Enemy", "Pickup", "Scenery",
        "Projectile", "Static", "Dynamic", "Trigger", "Water"
    ]
    
    init(label: String, matrix: Binding<[[Bool]]>) {
        self.label = label
        self._matrix = matrix
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            ScrollView([.horizontal, .vertical]) {
                VStack(spacing: 2) {
                    // Header row
                    HStack(spacing: 2) {
                        Text("")
                            .frame(width: 60, alignment: .trailing)
                        
                        ForEach(0..<layerNames.count, id: \.self) { col in
                            VStack {
                                Text("\(col)")
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                                    .frame(width: 40)
                                
                                Text(layerNames[col])
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                    .frame(width: 40)
                                    .lineLimit(1)
                                    .truncationMode(.tail)
                            }
                        }
                    }
                    
                    // Matrix rows
                    ForEach(0..<matrix.count, id: \.self) { row in
                        HStack(spacing: 2) {
                            Text("\(row)")
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                                .frame(width: 20, alignment: .trailing)
                            
                            Text(layerNames[row])
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                                .frame(width: 40)
                                .lineLimit(1)
                                .truncationMode(.tail)
                            
                            ForEach(0..<matrix[row].count, id: \.self) { col in
                                Button(action: {
                                    matrix[row][col].toggle()
                                }) {
                                    Image(systemName: matrix[row][col] ? "checkmark.square.fill" : "square")
                                        .foregroundColor(matrix[row][col] ? DesignSystem.Colors.accentSuccess : DesignSystem.Colors.textTertiary)
                                        .font(DesignSystem.Typography.caption)
                                }
                                .buttonStyle(.plain)
                                .frame(width: 30, height: 30)
                                .background(matrix[row][col] ? DesignSystem.Colors.accentSuccess.opacity(0.1) : DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                            }
                        }
                    }
                }
                .padding(8)
            }
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(6)
            .frame(maxHeight: 300)
        }
    }
}

// MARK: - Bounds Editor (AABB, OBB) (TODO-1754)
struct BoundsPropertyEditor: View {
    let label: String
    @Binding var center: SIMD3<Float>
    @Binding var size: SIMD3<Float>
    @State private var boundsType: BoundsType = .aabb
    
    enum BoundsType: String, CaseIterable {
        case aabb = "AABB"
        case obb = "OBB"
        case sphere = "Sphere"
        case capsule = "Capsule"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("", selection: $boundsType) {
                    ForEach(BoundsType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type)
                    }
                }
                .pickerStyle(.segmented)
                .font(DesignSystem.Typography.small)
            }
            
            VStack(spacing: 8) {
                // Center
                VStack(alignment: .leading, spacing: 2) {
                    Text("Center")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack(spacing: 4) {
                        ForEach(0..<3, id: \.self) { index in
                            HStack(spacing: 2) {
                                Text(["X", "Y", "Z"][index])
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                                    .frame(width: 12)
                                
                                TextField("", value: $center[index], format: .number.precision(.fractionLength(2)))
                                    .textFieldStyle(.plain)
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                    .frame(width: 60)
                            }
                            .padding(.horizontal, 4)
                            .padding(.vertical, 2)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        }
                    }
                }
                
                // Size
                VStack(alignment: .leading, spacing: 2) {
                    Text("Size")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack(spacing: 4) {
                        ForEach(0..<3, id: \.self) { index in
                            HStack(spacing: 2) {
                                Text(["X", "Y", "Z"][index])
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor([DesignSystem.Colors.xAxis, DesignSystem.Colors.yAxis, DesignSystem.Colors.zAxis][index])
                                    .frame(width: 12)
                                
                                TextField("", value: $size[index], format: .number.precision(.fractionLength(2)))
                                    .textFieldStyle(.plain)
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                    .frame(width: 60)
                            }
                            .padding(.horizontal, 4)
                            .padding(.vertical, 2)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        }
                    }
                }
                
                // Bounds info
                HStack {
                    Text("Volume: \(String(format: "%.2f", size.x * size.y * size.z))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Spacer()
                    
                    Text("Surface Area: \(String(format: "%.2f", 2 * (size.x * size.y + size.x * size.z + size.y * size.z)))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(4)
            }
        }
    }
}
