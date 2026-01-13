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

// TODO-1545: Rotation editor with random orientation
struct RandomOrientationEditor: View {
    @Binding var value: simd_quatf
    @State private var randomSeed: UInt32 = 0
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Random Orientation")
                .font(.headline)
            
            HStack {
                Button("Randomize") {
                    randomSeed = UInt32.random(in: 0...UInt32.max)
                    value = generateRandomRotation(seed: randomSeed)
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Button("Reset") {
                    value = simd_quatf()
                }
                .buttonStyle(.bordered)
            }
            
            // Visual representation of current rotation
            RotationPreviewView(rotation: value)
                .frame(height: 60)
        }
    }
    
    private func generateRandomRotation(seed: UInt32) -> simd_quatf {
        var rng = SeededRandomNumberGenerator(seed: seed)
        
        // Generate random axis
        let axis = normalize(SIMD3<Float>(
            Float.random(in: -1...1, using: &rng),
            Float.random(in: -1...1, using: &rng),
            Float.random(in: -1...1, using: &rng)
        ))
        
        // Generate random angle
        let angle = Float.random(in: 0...(.pi * 2), using: &rng)
        
        return simd_quatf(angle: angle, axis: axis)
    }
}

// Simple seeded random number generator
struct SeededRandomNumberGenerator: RandomNumberGenerator {
    private var state: UInt64
    
    init(seed: UInt32) {
        state = UInt64(seed)
        if state == 0 {
            state = 1
        }
    }
    
    mutating func next() -> UInt64 {
        state = state &* 1103515245 + 12345
        return state
    }
}

// TODO-1546: Rotation editor with snap to angles
struct SnapAngleEditor: View {
    @Binding var value: simd_quatf
    @State private var snapAngle: Float = 45.0 // degrees
    @State private var snapEnabled: Bool = true
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Snap Angles")
                    .font(.headline)
                
                Toggle("Enable Snap", isOn: $snapEnabled)
                    .toggleStyle(.switch)
            }
            
            HStack {
                Text("Snap Angle:")
                    .font(.caption)
                
                TextField("Angle", value: $snapAngle, format: .number)
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 80)
                
                Text("degrees")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Button("Snap") {
                    if snapEnabled {
                        value = snapToNearest(value, angle: snapAngle * .pi / 180)
                    }
                }
                .buttonStyle(.bordered)
                .disabled(!snapEnabled)
            }
            
            // Quick snap buttons
            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 4), spacing: 4) {
                ForEach([15, 30, 45, 90], id: \.self) { angle in
                    Button("\(angle)°") {
                        snapAngle = Float(angle)
                        if snapEnabled {
                            value = snapToNearest(value, angle: Float(angle) * .pi / 180)
                        }
                    }
                    .buttonStyle(.borderedCompact)
                }
            }
        }
    }
    
    private func snapToNearest(_ rotation: simd_quatf, angle: Float) -> simd_quatf {
        let euler = rotation.eulerAngles
        let snappedEuler = SIMD3<Float>(
            snapAngleToNearest(euler.x, snapAngle: angle),
            snapAngleToNearest(euler.y, snapAngle: angle),
            snapAngleToNearest(euler.z, snapAngle: angle)
        )
        return simd_quatf(eulerAngles: snappedEuler)
    }
    
    private func snapAngleToNearest(_ angle: Float, snapAngle: Float) -> Float {
        let snapped = round(angle / snapAngle) * snapAngle
        return snapped
    }
}

// TODO-1547: Rotation editor with rotation speed display
struct RotationSpeedEditor: View {
    @Binding var value: simd_quatf
    @State private var previousValue: simd_quatf = simd_quatf()
    @State private var lastUpdateTime: Date = Date()
    @State private var angularVelocity: SIMD3<Float> = .zero
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Rotation Speed")
                .font(.headline)
            
            HStack {
                VStack(alignment: .leading) {
                    Text("Angular Velocity (rad/s):")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    HStack {
                        Text("X: \(String(format: "%.3f", angularVelocity.x))")
                        Text("Y: \(String(format: "%.3f", angularVelocity.y))")
                        Text("Z: \(String(format: "%.3f", angularVelocity.z))")
                    }
                    .font(.caption.monospaced())
                }
                
                Spacer()
                
                VStack(alignment: .trailing) {
                    Text("Speed: \(String(format: "%.3f", length(angularVelocity))) rad/s")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Text("\(String(format: "%.1f", length(angularVelocity) * 180 / .pi))°/s")
                        .font(.caption2)
                        .foregroundColor(.secondary)
                }
            }
            
            // Visual speed indicator
            ProgressView(value: min(length(angularVelocity) / 10, 1))
                .progressViewStyle(LinearProgressViewStyle(tint: speedColor))
        }
        .onAppear {
            previousValue = value
            lastUpdateTime = Date()
        }
        .onReceive(Timer.publish(every: 0.1, on: .main, in: .common).autoconnect()) { _ in
            updateAngularVelocity()
        }
    }
    
    private var speedColor: Color {
        let speed = length(angularVelocity)
        if speed < 1 { return .green }
        if speed < 5 { return .yellow }
        return .red
    }
    
    private func updateAngularVelocity() {
        let currentTime = Date()
        let deltaTime = Float(currentTime.timeIntervalSince(lastUpdateTime))
        
        if deltaTime > 0 {
            let deltaRotation = value * previousValue.inverse
            let axisAngle = deltaRotation.axisAngle
            
            if length(axisAngle.axis) > 0.001 {
                angularVelocity = axisAngle.axis * axisAngle.angle / deltaTime
            } else {
                angularVelocity = .zero
            }
        }
        
        previousValue = value
        lastUpdateTime = currentTime
    }
}

// TODO-1548: Rotation editor with angular velocity
struct AngularVelocityEditor: View {
    @Binding var value: simd_quatf
    @Binding var angularVelocity: SIMD3<Float>
    @State private var integrationMode: Bool = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Angular Velocity")
                    .font(.headline)
                
                Toggle("Integrate", isOn: $integrationMode)
                    .toggleStyle(.switch)
                    .help("When enabled, angular velocity will be integrated to update rotation")
            }
            
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Text("X:")
                        .frame(width: 20, alignment: .leading)
                    Slider(value: Binding(
                        get: { angularVelocity.x },
                        set: { angularVelocity.x = $0 }
                    ), in: -10...10)
                    TextField("", value: Binding(
                        get: { angularVelocity.x },
                        set: { angularVelocity.x = $0 }
                    ), format: .number.precision(.fractionLength(3)))
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 80)
                }
                
                HStack {
                    Text("Y:")
                        .frame(width: 20, alignment: .leading)
                    Slider(value: Binding(
                        get: { angularVelocity.y },
                        set: { angularVelocity.y = $0 }
                    ), in: -10...10)
                    TextField("", value: Binding(
                        get: { angularVelocity.y },
                        set: { angularVelocity.y = $0 }
                    ), format: .number.precision(.fractionLength(3)))
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 80)
                }
                
                HStack {
                    Text("Z:")
                        .frame(width: 20, alignment: .leading)
                    Slider(value: Binding(
                        get: { angularVelocity.z },
                        set: { angularVelocity.z = $0 }
                    ), in: -10...10)
                    TextField("", value: Binding(
                        get: { angularVelocity.z },
                        set: { angularVelocity.z = $0 }
                    ), format: .number.precision(.fractionLength(3)))
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 80)
                }
            }
            
            HStack {
                Text("Magnitude: \(String(format: "%.3f", length(angularVelocity))) rad/s")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Button("Zero") {
                    angularVelocity = .zero
                }
                .buttonStyle(.borderedCompact)
            }
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            if integrationMode && length(angularVelocity) > 0.001 {
                let dt = 0.016 // 60 FPS
                let angle = length(angularVelocity) * dt
                let axis = normalize(angularVelocity)
                let deltaRotation = simd_quatf(angle: angle, axis: axis)
                value = deltaRotation * value
            }
        }
    }
}

// TODO-1549: Rotation editor with damping factor
struct DampedRotationEditor: View {
    @Binding var value: simd_quatf
    @Binding var angularVelocity: SIMD3<Float>
    @State private var dampingFactor: Float = 0.1
    @State private var dampingEnabled: Bool = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Damped Rotation")
                    .font(.headline)
                
                Toggle("Enable Damping", isOn: $dampingEnabled)
                    .toggleStyle(.switch)
            }
            
            VStack(alignment: .leading, spacing: 4) {
                Text("Damping Factor:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                HStack {
                    Slider(value: $dampingFactor, in: 0...1)
                    Text(String(format: "%.3f", dampingFactor))
                        .font(.caption.monospaced())
                        .frame(width: 60)
                }
            }
            
            HStack {
                VStack(alignment: .leading) {
                    Text("Angular Velocity:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Text("\(String(format: "%.3f", length(angularVelocity))) rad/s")
                        .font(.caption.monospaced())
                }
                
                Spacer()
                
                VStack(alignment: .trailing) {
                    Text("Damping:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Text("\(String(format: "%.3f", dampingFactor * length(angularVelocity))) rad/s²")
                        .font(.caption.monospaced())
                }
            }
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            if dampingEnabled {
                let dt = 0.016 // 60 FPS
                let dampingForce = -angularVelocity * dampingFactor
                angularVelocity = angularVelocity + dampingForce * dt
                
                // Update rotation based on angular velocity
                if length(angularVelocity) > 0.001 {
                    let angle = length(angularVelocity) * dt
                    let axis = normalize(angularVelocity)
                    let deltaRotation = simd_quatf(angle: angle, axis: axis)
                    value = deltaRotation * value
                }
            }
        }
    }
}

// TODO-1550: Rotation editor with spring forces
struct SpringRotationEditor: View {
    @Binding var value: simd_quatf
    @State private var targetRotation: simd_quatf = simd_quatf()
    @State private var springStiffness: Float = 10.0
    @State private var springDamping: Float = 0.5
    @State private var angularVelocity: SIMD3<Float> = .zero
    @State private var springEnabled: Bool = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Spring Rotation")
                    .font(.headline)
                
                Toggle("Enable Spring", isOn: $springEnabled)
                    .toggleStyle(.switch)
            }
            
            VStack(alignment: .leading, spacing: 4) {
                Text("Spring Stiffness:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                HStack {
                    Slider(value: $springStiffness, in: 0.1...50)
                    Text(String(format: "%.1f", springStiffness))
                        .font(.caption.monospaced())
                        .frame(width: 40)
                }
            }
            
            VStack(alignment: .leading, spacing: 4) {
                Text("Spring Damping:")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                HStack {
                    Slider(value: $springDamping, in: 0...1)
                    Text(String(format: "%.3f", springDamping))
                        .font(.caption.monospaced())
                        .frame(width: 60)
                }
            }
            
            HStack {
                Button("Set Target") {
                    targetRotation = value
                }
                .buttonStyle(.bordered)
                
                Button("Reset") {
                    targetRotation = simd_quatf()
                    value = simd_quatf()
                    angularVelocity = .zero
                }
                .buttonStyle(.bordered)
            }
            
            // Visual indicators
            HStack {
                Text("Velocity: \(String(format: "%.3f", length(angularVelocity))) rad/s")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                let error = angleBetween(value, targetRotation)
                Text("Error: \(String(format: "%.3f", error)) rad")
                    .font(.caption)
                    .foregroundColor(error > 0.1 ? .red : .green)
            }
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            if springEnabled {
                let dt = 0.016 // 60 FPS
                
                // Calculate spring force
                let rotationError = targetRotation * value.inverse
                let axisAngle = rotationError.axisAngle
                let springForce = axisAngle.axis * axisAngle.angle * springStiffness
                
                // Apply damping
                let dampingForce = -angularVelocity * springDamping
                
                // Update angular velocity
                angularVelocity = angularVelocity + (springForce + dampingForce) * dt
                
                // Update rotation
                if length(angularVelocity) > 0.001 {
                    let angle = length(angularVelocity) * dt
                    let axis = normalize(angularVelocity)
                    let deltaRotation = simd_quatf(angle: angle, axis: axis)
                    value = deltaRotation * value
                }
            }
        }
    }
    
    private func angleBetween(_ q1: simd_quatf, _ q2: simd_quatf) -> Float {
        let dot = abs(q1.vector.dot(q2.vector))
        return acos(min(dot, 1)) * 2
    }
}
// TODO-1551: Rotation editor with constraint limits
struct ConstrainedRotationEditor: View {
    @Binding var value: simd_quatf
    @State private var minAngles: SIMD3<Float> = SIMD3<Float>(-.pi, -.pi/2, -.pi)
    @State private var maxAngles: SIMD3<Float> = SIMD3<Float>(.pi, .pi/2, .pi)
    @State private var constraintsEnabled: Bool = false
    @State private var constraintMode: ConstraintMode = .euler
    
    enum ConstraintMode: String, CaseIterable {
        case euler = "Euler"
        case swingTwist = "Swing-Twist"
        case cone = "Cone"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Constrained Rotation")
                    .font(.headline)
                
                Toggle("Enable Constraints", isOn: $constraintsEnabled)
                    .toggleStyle(.switch)
            }
            
            if constraintsEnabled {
                Picker("Constraint Mode", selection: $constraintMode) {
                    ForEach(ConstraintMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.segmented)
                
                switch constraintMode {
                case .euler:
                    eulerConstraintsView
                case .swingTwist:
                    swingTwistConstraintsView
                case .cone:
                    coneConstraintsView
                }
            }
            
            // Constraint violation indicator
            if constraintsEnabled && isConstraintViolated {
                HStack {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .foregroundColor(.orange)
                    Text("Constraints violated")
                        .font(.caption)
                        .foregroundColor(.orange)
                    
                    Button("Clamp") {
                        value = clampToConstraints(value)
                    }
                    .buttonStyle(.borderedCompact)
                }
            }
        }
        .onChange(of: value) { newValue in
            if constraintsEnabled {
                if isConstraintViolated {
                    // Optionally clamp automatically
                    // value = clampToConstraints(newValue)
                }
            }
        }
    }
    
    @ViewBuilder
    private var eulerConstraintsView: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Euler Angle Limits (radians):")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                Text("X:")
                    .frame(width: 20)
                TextField("Min", value: Binding(
                    get: { minAngles.x },
                    set: { minAngles.x = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
                
                TextField("Max", value: Binding(
                    get: { maxAngles.x },
                    set: { maxAngles.x = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
            }
            
            HStack {
                Text("Y:")
                    .frame(width: 20)
                TextField("Min", value: Binding(
                    get: { minAngles.y },
                    set: { minAngles.y = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
                
                TextField("Max", value: Binding(
                    get: { maxAngles.y },
                    set: { maxAngles.y = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
            }
            
            HStack {
                Text("Z:")
                    .frame(width: 20)
                TextField("Min", value: Binding(
                    get: { minAngles.z },
                    set: { minAngles.z = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
                
                TextField("Max", value: Binding(
                    get: { maxAngles.z },
                    set: { maxAngles.z = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
            }
        }
    }
    
    @ViewBuilder
    private var swingTwistConstraintsView: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Swing-Twist Constraints:")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                Text("Twist Limit:")
                TextField("Angle", value: Binding(
                    get: { minAngles.x }, // Reuse for twist
                    set: { minAngles.x = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
                Text("rad")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            HStack {
                Text("Swing Limit:")
                TextField("Angle", value: Binding(
                    get: { minAngles.y }, // Reuse for swing
                    set: { minAngles.y = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
                Text("rad")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
    }
    
    @ViewBuilder
    private var coneConstraintsView: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Cone Constraints:")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                Text("Cone Angle:")
                TextField("Angle", value: Binding(
                    get: { minAngles.x }, // Reuse for cone angle
                    set: { minAngles.x = $0 }
                ), format: .number.precision(.fractionLength(3)))
                .textFieldStyle(.roundedBorder)
                .frame(width: 80)
                Text("rad")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
    }
    
    private var isConstraintViolated: Bool {
        switch constraintMode {
        case .euler:
            let euler = value.eulerAngles
            return euler.x < minAngles.x || euler.x > maxAngles.x ||
                   euler.y < minAngles.y || euler.y > maxAngles.y ||
                   euler.z < minAngles.z || euler.z > maxAngles.z
        case .swingTwist:
            // Simplified swing-twist check
            return false
        case .cone:
            // Simplified cone check
            return false
        }
    }
    
    private func clampToConstraints(_ rotation: simd_quatf) -> simd_quatf {
        switch constraintMode {
        case .euler:
            let euler = rotation.eulerAngles
            let clampedEuler = SIMD3<Float>(
                max(minAngles.x, min(maxAngles.x, euler.x)),
                max(minAngles.y, min(maxAngles.y, euler.y)),
                max(minAngles.z, min(maxAngles.z, euler.z))
            )
            return simd_quatf(eulerAngles: clampedEuler)
        case .swingTwist:
            return rotation // Simplified
        case .cone:
            return rotation // Simplified
        }
    }
}

// TODO-1552: Rotation editor with motor/torque control
struct MotorRotationEditor: View {
    @Binding var value: simd_quatf
    @State private var motorTorque: SIMD3<Float> = .zero
    @State private var motorEnabled: Bool = false
    @State private var motorSpeed: Float = 1.0
    @State private var angularVelocity: SIMD3<Float> = .zero
    @State private var motorMode: MotorMode = .torque
    
    enum MotorMode: String, CaseIterable {
        case torque = "Torque"
        case velocity = "Velocity"
        case position = "Position"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Motor Control")
                    .font(.headline)
                
                Toggle("Enable Motor", isOn: $motorEnabled)
                    .toggleStyle(.switch)
            }
            
            if motorEnabled {
                Picker("Motor Mode", selection: $motorMode) {
                    ForEach(MotorMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.segmented)
                
                switch motorMode {
                case .torque:
                    torqueControlView
                case .velocity:
                    velocityControlView
                case .position:
                    positionControlView
                }
            }
            
            // Motor status
            HStack {
                Text("Angular Velocity: \(String(format: "%.3f", length(angularVelocity))) rad/s")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                Text("Motor Power: \(String(format: "%.1f", motorPower))%")
                    .font(.caption)
                    .foregroundColor(motorPower > 80 ? .red : motorPower > 50 ? .yellow : .green)
            }
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            updateMotorPhysics()
        }
    }
    
    @ViewBuilder
    private var torqueControlView: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Torque Control:")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                Text("X:")
                    .frame(width: 20)
                Slider(value: Binding(
                    get: { motorTorque.x },
                    set: { motorTorque.x = $0 }
                ), in: -10...10)
                Text(String(format: "%.1f", motorTorque.x))
                    .font(.caption.monospaced())
                    .frame(width: 40)
            }
            
            HStack {
                Text("Y:")
                    .frame(width: 20)
                Slider(value: Binding(
                    get: { motorTorque.y },
                    set: { motorTorque.y = $0 }
                ), in: -10...10)
                Text(String(format: "%.1f", motorTorque.y))
                    .font(.caption.monospaced())
                    .frame(width: 40)
            }
            
            HStack {
                Text("Z:")
                    .frame(width: 20)
                Slider(value: Binding(
                    get: { motorTorque.z },
                    set: { motorTorque.z = $0 }
                ), in: -10...10)
                Text(String(format: "%.1f", motorTorque.z))
                    .font(.caption.monospaced())
                    .frame(width: 40)
            }
        }
    }
    
    @ViewBuilder
    private var velocityControlView: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Velocity Control:")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                Text("Speed:")
                Slider(value: $motorSpeed, in: 0...10)
                Text(String(format: "%.1f", motorSpeed))
                    .font(.caption.monospaced())
                    .frame(width: 40)
                Text("rad/s")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            // Direction control (simplified)
            HStack {
                Text("Direction:")
                Spacer()
            }
        }
    }
    
    @ViewBuilder
    private var positionControlView: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Position Control:")
                .font(.caption)
                .foregroundColor(.secondary)
            
            HStack {
                Button("Set Target") {
                    // Store current position as target
                }
                .buttonStyle(.bordered)
                
                Button("Go to Target") {
                    // Move towards target position
                }
                .buttonStyle(.bordered)
            }
        }
    }
    
    private var motorPower: Float {
        switch motorMode {
        case .torque:
            return min(length(motorTorque) / 10 * 100, 100)
        case .velocity:
            return min(motorSpeed / 10 * 100, 100)
        case .position:
            return 50 // Simplified
        }
    }
    
    private func updateMotorPhysics() {
        guard motorEnabled else { return }
        
        let dt = 0.016 // 60 FPS
        
        switch motorMode {
        case .torque:
            // Apply torque to angular velocity
            angularVelocity = angularVelocity + motorTorque * dt
            
        case .velocity:
            // Maintain target velocity
            let targetVelocity = SIMD3<Float>(0, 0, motorSpeed) // Simplified
            angularVelocity = angularVelocity * 0.9 + targetVelocity * 0.1 // Smooth transition
            
        case .position:
            // Move towards target position (simplified)
            break
        }
        
        // Update rotation based on angular velocity
        if length(angularVelocity) > 0.001 {
            let angle = length(angularVelocity) * dt
            let axis = normalize(angularVelocity)
            let deltaRotation = simd_quatf(angle: angle, axis: axis)
            value = deltaRotation * value
        }
    }
}

// TODO-1553: Rotation editor with acceleration/deceleration
struct AccelerationRotationEditor: View {
    @Binding var value: simd_quatf
    @State private var targetAngularVelocity: SIMD3<Float> = .zero
    @State private var currentAngularVelocity: SIMD3<Float> = .zero
    @State private var acceleration: Float = 1.0
    @State private var deceleration: Float = 1.0
    @State private var accelerationEnabled: Bool = false
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Acceleration Control")
                    .font(.headline)
                
                Toggle("Enable Acceleration", isOn: $accelerationEnabled)
                    .toggleStyle(.switch)
            }
            
            if accelerationEnabled {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Target Angular Velocity:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    HStack {
                        Text("X:")
                            .frame(width: 20)
                        Slider(value: Binding(
                            get: { targetAngularVelocity.x },
                            set: { targetAngularVelocity.x = $0 }
                        ), in: -10...10)
                        Text(String(format: "%.1f", targetAngularVelocity.x))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Y:")
                            .frame(width: 20)
                        Slider(value: Binding(
                            get: { targetAngularVelocity.y },
                            set: { targetAngularVelocity.y = $0 }
                        ), in: -10...10)
                        Text(String(format: "%.1f", targetAngularVelocity.y))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Z:")
                            .frame(width: 20)
                        Slider(value: Binding(
                            get: { targetAngularVelocity.z },
                            set: { targetAngularVelocity.z = $0 }
                        ), in: -10...10)
                        Text(String(format: "%.1f", targetAngularVelocity.z))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                    }
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Text("Acceleration:")
                        Slider(value: $acceleration, in: 0.1...5)
                        Text(String(format: "%.1f", acceleration))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                        Text("rad/s²")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    HStack {
                        Text("Deceleration:")
                        Slider(value: $deceleration, in: 0.1...5)
                        Text(String(format: "%.1f", deceleration))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                        Text("rad/s²")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            // Status display
            HStack {
                VStack(alignment: .leading) {
                    Text("Current Velocity:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text("\(String(format: "%.3f", length(currentAngularVelocity))) rad/s")
                        .font(.caption.monospaced())
                }
                
                Spacer()
                
                VStack(alignment: .trailing) {
                    Text("Target Velocity:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text("\(String(format: "%.3f", length(targetAngularVelocity))) rad/s")
                        .font(.caption.monospaced())
                }
            }
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            updateAcceleration()
        }
    }
    
    private func updateAcceleration() {
        guard accelerationEnabled else { return }
        
        let dt = 0.016 // 60 FPS
        
        // Calculate velocity error
        let velocityError = targetAngularVelocity - currentAngularVelocity
        
        // Apply acceleration or deceleration
        let accelMagnitude = length(velocityError)
        if accelMagnitude > 0.001 {
            let maxAccel = dot(velocityError, targetAngularVelocity) > 0 ? acceleration : deceleration
            let appliedAccel = normalize(velocityError) * min(accelMagnitude, maxAccel * dt)
            currentAngularVelocity = currentAngularVelocity + appliedAccel
        }
        
        // Update rotation based on current angular velocity
        if length(currentAngularVelocity) > 0.001 {
            let angle = length(currentAngularVelocity) * dt
            let axis = normalize(currentAngularVelocity)
            let deltaRotation = simd_quatf(angle: angle, axis: axis)
            value = deltaRotation * value
        }
    }
}

// TODO-1554: Rotation editor with oscillation/wobble
struct OscillationRotationEditor: View {
    @Binding var value: simd_quatf
    @State private var oscillationEnabled: Bool = false
    @State private var oscillationAmplitude: Float = 0.5
    @State private var oscillationFrequency: Float = 1.0
    @State private var oscillationAxis: SIMD3<Float> = SIMD3<Float>(0, 1, 0)
    @State private var wobbleEnabled: Bool = false
    @State private var wobbleAmount: Float = 0.1
    @State private var wobbleSpeed: Float = 5.0
    @State private var time: Float = 0
    @State private var baseRotation: simd_quatf = simd_quatf()
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Oscillation & Wobble")
                .font(.headline)
            
            Toggle("Enable Oscillation", isOn: $oscillationEnabled)
                .toggleStyle(.switch)
            
            if oscillationEnabled {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Oscillation Settings:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    HStack {
                        Text("Amplitude:")
                        Slider(value: $oscillationAmplitude, in: 0...2)
                        Text(String(format: "%.2f", oscillationAmplitude))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                        Text("rad")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    HStack {
                        Text("Frequency:")
                        Slider(value: $oscillationFrequency, in: 0.1...10)
                        Text(String(format: "%.1f", oscillationFrequency))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                        Text("Hz")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    HStack {
                        Text("Axis:")
                        Slider(value: Binding(
                            get: { oscillationAxis.x },
                            set: { oscillationAxis.x = $0 }
                        ), in: -1...1)
                        Slider(value: Binding(
                            get: { oscillationAxis.y },
                            set: { oscillationAxis.y = $0 }
                        ), in: -1...1)
                        Slider(value: Binding(
                            get: { oscillationAxis.z },
                            set: { oscillationAxis.z = $0 }
                        ), in: -1...1)
                    }
                }
            }
            
            Toggle("Enable Wobble", isOn: $wobbleEnabled)
                .toggleStyle(.switch)
            
            if wobbleEnabled {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Wobble Settings:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    HStack {
                        Text("Amount:")
                        Slider(value: $wobbleAmount, in: 0...1)
                        Text(String(format: "%.2f", wobbleAmount))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                    }
                    
                    HStack {
                        Text("Speed:")
                        Slider(value: $wobbleSpeed, in: 1...20)
                        Text(String(format: "%.1f", wobbleSpeed))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                        Text("Hz")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
            }
            
            HStack {
                Button("Set Base") {
                    baseRotation = value
                }
                .buttonStyle(.bordered)
                
                Button("Reset") {
                    value = baseRotation
                    time = 0
                }
                .buttonStyle(.bordered)
            }
        }
        .onAppear {
            baseRotation = value
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            updateOscillation()
        }
    }
    
    private func updateOscillation() {
        time += 0.016
        
        var rotation = baseRotation
        
        if oscillationEnabled {
            let oscillationAngle = sin(time * oscillationFrequency * 2 * .pi) * oscillationAmplitude
            let normalizedAxis = normalize(oscillationAxis)
            let oscillationRotation = simd_quatf(angle: oscillationAngle, axis: normalizedAxis)
            rotation = oscillationRotation * rotation
        }
        
        if wobbleEnabled {
            let wobbleAngleX = sin(time * wobbleSpeed) * wobbleAmount
            let wobbleAngleY = cos(time * wobbleSpeed * 0.7) * wobbleAmount
            let wobbleAngleZ = sin(time * wobbleSpeed * 1.3) * wobbleAmount
            
            let wobbleRotation = simd_quatf(eulerAngles: SIMD3<Float>(wobbleAngleX, wobbleAngleY, wobbleAngleZ))
            rotation = wobbleRotation * rotation
        }
        
        value = rotation
    }
}
// TODO-1555: Rotation editor with rotation history
struct RotationHistoryEditor: View {
    @Binding var value: simd_quatf
    @State private var rotationHistory: [RotationSnapshot] = []
    @State private var maxHistorySize: Int = 50
    @State private var historyEnabled: Bool = true
    @State private var autoRecord: Bool = true
    @State private var recordInterval: Float = 0.5 // seconds
    @State private var lastRecordTime: Float = 0
    
    struct RotationSnapshot: Identifiable {
        let id = UUID()
        let rotation: simd_quatf
        let timestamp: Date
        let description: String
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Rotation History")
                    .font(.headline)
                
                Toggle("Enable History", isOn: $historyEnabled)
                    .toggleStyle(.switch)
            }
            
            if historyEnabled {
                HStack {
                    Toggle("Auto Record", isOn: $autoRecord)
                        .toggleStyle(.switch)
                    
                    if autoRecord {
                        Text("Interval:")
                            .font(.caption)
                        TextField("Interval", value: $recordInterval, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .frame(width: 60)
                        Text("s")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    Spacer()
                    
                    Button("Clear") {
                        rotationHistory.removeAll()
                    }
                    .buttonStyle(.borderedCompact)
                }
                
                // History list
                ScrollView {
                    LazyVStack(spacing: 2) {
                        ForEach(rotationHistory.reversed()) { snapshot in
                            HStack {
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(snapshot.description)
                                        .font(.caption2)
                                    Text(DateFormatter.timeFormatter.string(from: snapshot.timestamp))
                                        .font(.caption2)
                                        .foregroundColor(.secondary)
                                }
                                
                                Spacer()
                                
                                Button("Restore") {
                                    value = snapshot.rotation
                                }
                                .buttonStyle(.borderedCompact)
                            }
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(Color.gray.opacity(0.1))
                            .cornerRadius(4)
                        }
                    }
                }
                .frame(height: 150)
                
                HStack {
                    Text("History: \(rotationHistory.count)/\(maxHistorySize)")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    Spacer()
                    
                    Button("Capture") {
                        captureSnapshot(description: "Manual")
                    }
                    .buttonStyle(.borderedCompact)
                }
            }
        }
        .onReceive(Timer.publish(every: 0.1, on: .main, in: .common).autoconnect()) { _ in
            if autoRecord && historyEnabled {
                checkAutoRecord()
            }
        }
    }
    
    private func checkAutoRecord() {
        let currentTime = Float(Date().timeIntervalSince1970)
        
        if currentTime - lastRecordTime >= recordInterval {
            captureSnapshot(description: "Auto")
            lastRecordTime = currentTime
        }
    }
    
    private func captureSnapshot(description: String) {
        let snapshot = RotationSnapshot(
            rotation: value,
            timestamp: Date(),
            description: description
        )
        
        rotationHistory.append(snapshot)
        
        // Maintain history size
        if rotationHistory.count > maxHistorySize {
            rotationHistory.removeFirst()
        }
    }
}

// TODO-1556: Rotation editor with rotation blending
struct RotationBlendingEditor: View {
    @Binding var value: simd_quatf
    @State private var blendRotations: [BlendRotation] = []
    @State private var blendingEnabled: Bool = false
    @State private var blendMode: BlendMode = .slerp
    
    struct BlendRotation: Identifiable {
        let id = UUID()
        var rotation: simd_quatf
        var weight: Float
        var name: String
    }
    
    enum BlendMode: String, CaseIterable {
        case slerp = "SLERP"
        case lerp = "LERP"
        case nlerp = "NLERP"
        case squad = "SQUAD"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Rotation Blending")
                    .font(.headline)
                
                Toggle("Enable Blending", isOn: $blendingEnabled)
                    .toggleStyle(.switch)
            }
            
            if blendingEnabled {
                Picker("Blend Mode", selection: $blendMode) {
                    ForEach(BlendMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.segmented)
                
                // Blend rotations list
                ScrollView {
                    LazyVStack(spacing: 4) {
                        ForEach($blendRotations) { $blendRot in
                            HStack {
                                TextField("Name", text: $blendRot.name)
                                    .textFieldStyle(.roundedBorder)
                                    .frame(width: 80)
                                
                                Slider(value: $blendRot.weight, in: 0...1) {
                                    Text("Weight")
                                }
                                Text(String(format: "%.2f", blendRot.weight))
                                    .font(.caption.monospaced())
                                    .frame(width: 40)
                                
                                Button("Set") {
                                    blendRot.rotation = value
                                }
                                .buttonStyle(.borderedCompact)
                                
                                Button("Remove") {
                                    blendRotations.removeAll { $0.id == blendRot.id }
                                }
                                .buttonStyle(.borderedCompact)
                            }
                        }
                    }
                }
                .frame(height: 120)
                
                HStack {
                    Button("Add Current") {
                        blendRotations.append(BlendRotation(
                            rotation: value,
                            weight: 1.0,
                            name: "Rotation \(blendRotations.count + 1)"
                        ))
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Apply Blend") {
                        applyBlend()
                    }
                    .buttonStyle(.bordered)
                    .disabled(blendRotations.isEmpty)
                    
                    Button("Clear All") {
                        blendRotations.removeAll()
                    }
                    .buttonStyle(.bordered)
                }
                
                if !blendRotations.isEmpty {
                    Text("Total Weight: \(String(format: "%.2f", totalWeight))")
                        .font(.caption)
                        .foregroundColor(totalWeight != 1.0 ? .orange : .secondary)
                }
            }
        }
    }
    
    private var totalWeight: Float {
        blendRotations.reduce(0) { $0 + $1.weight }
    }
    
    private func applyBlend() {
        guard !blendRotations.isEmpty else { return }
        
        let normalizedRotations = blendRotations.map { rot in
            (rotation: rot.rotation, weight: rot.weight / totalWeight)
        }
        
        switch blendMode {
        case .slerp:
            value = slerpBlend(normalizedRotations)
        case .lerp:
            value = lerpBlend(normalizedRotations)
        case .nlerp:
            value = nlerpBlend(normalizedRotations)
        case .squad:
            value = squadBlend(normalizedRotations)
        }
    }
    
    private func slerpBlend(_ rotations: [(rotation: simd_quatf, weight: Float)]) -> simd_quatf {
        guard rotations.count > 0 else { return simd_quatf() }
        
        var result = rotations[0].rotation
        
        for i in 1..<rotations.count {
            result = simd_slerp(result, rotations[i].rotation, rotations[i].weight)
        }
        
        return result
    }
    
    private func lerpBlend(_ rotations: [(rotation: simd_quatf, weight: Float)]) -> simd_quatf {
        guard rotations.count > 0 else { return simd_quatf() }
        
        var result = rotations[0].rotation
        
        for i in 1..<rotations.count {
            result = simd_mix(result, rotations[i].rotation, rotations[i].weight)
        }
        
        return normalize(result)
    }
    
    private func nlerpBlend(_ rotations: [(rotation: simd_quatf, weight: Float)]) -> simd_quatf {
        return normalize(lerpBlend(rotations))
    }
    
    private func squadBlend(_ rotations: [(rotation: simd_quatf, weight: Float)]) -> simd_quatf {
        // Simplified SQUAD implementation
        return slerpBlend(rotations)
    }
}

// TODO-1557: Rotation editor with rotation keys
struct RotationKeyframeEditor: View {
    @Binding var value: simd_quatf
    @State private var keyframes: [RotationKeyframe] = []
    @State private var currentTime: Float = 0
    @State private var isPlaying: Bool = false
    @State private var loopMode: LoopMode = .loop
    @State private var playbackSpeed: Float = 1.0
    
    struct RotationKeyframe: Identifiable {
        let id = UUID()
        var time: Float
        var rotation: simd_quatf
        var name: String
    }
    
    enum LoopMode: String, CaseIterable {
        case loop = "Loop"
        case pingPong = "Ping-Pong"
        case once = "Once"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Rotation Keyframes")
                .font(.headline)
            
            // Playback controls
            HStack {
                Button(isPlaying ? "Pause" : "Play") {
                    isPlaying.toggle()
                }
                .buttonStyle(.bordered)
                
                Button("Stop") {
                    isPlaying = false
                    currentTime = 0
                    updateRotationForTime()
                }
                .buttonStyle(.bordered)
                
                Picker("Loop", selection: $loopMode) {
                    ForEach(LoopMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.segmented)
                
                Spacer()
                
                Text("Speed: \(String(format: "%.1f", playbackSpeed))x")
                    .font(.caption)
                Slider(value: $playbackSpeed, in: 0.1...3)
                    .frame(width: 80)
            }
            
            // Timeline
            VStack(alignment: .leading, spacing: 4) {
                Text("Timeline: \(String(format: "%.2f", currentTime))s")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Slider(value: $currentTime, in: 0...maxTime) { _ in
                    updateRotationForTime()
                }
                
                // Keyframe markers
                HStack {
                    ForEach(keyframes) { keyframe in
                        Circle()
                            .fill(Color.blue)
                            .frame(width: 8, height: 8)
                            .offset(x: CGFloat(keyframe.time / maxTime * 200 - 100))
                    }
                }
                .frame(height: 10)
            }
            
            // Keyframe list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach($keyframes) { $keyframe in
                        HStack {
                            TextField("Name", text: $keyframe.name)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 80)
                            
                            TextField("Time", value: $keyframe.time, format: .number)
                                .textFieldStyle(.roundedBorder)
                                .frame(width: 60)
                            
                            Button("Set") {
                                keyframe.rotation = value
                            }
                            .buttonStyle(.borderedCompact)
                            
                            Button("Go To") {
                                currentTime = keyframe.time
                                updateRotationForTime()
                            }
                            .buttonStyle(.borderedCompact)
                            
                            Button("Remove") {
                                keyframes.removeAll { $0.id == keyframe.id }
                            }
                            .buttonStyle(.borderedCompact)
                        }
                    }
                }
            }
            .frame(height: 100)
            
            HStack {
                Button("Add Keyframe") {
                    keyframes.append(RotationKeyframe(
                        time: currentTime,
                        rotation: value,
                        name: "Key \(keyframes.count + 1)"
                    ))
                    keyframes.sort { $0.time < $1.time }
                }
                .buttonStyle(.bordered)
                
                Button("Clear All") {
                    keyframes.removeAll()
                }
                .buttonStyle(.bordered)
            }
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            if isPlaying && !keyframes.isEmpty {
                updatePlayback()
            }
        }
    }
    
    private var maxTime: Float {
        keyframes.last?.time ?? 1.0
    }
    
    private func updatePlayback() {
        let dt = 0.016 * playbackSpeed
        currentTime += dt
        
        switch loopMode {
        case .loop:
            if currentTime > maxTime {
                currentTime = 0
            }
        case .pingPong:
            if currentTime > maxTime {
                currentTime = maxTime
                playbackSpeed = -playbackSpeed
            } else if currentTime < 0 {
                currentTime = 0
                playbackSpeed = -playbackSpeed
            }
        case .once:
            if currentTime > maxTime {
                isPlaying = false
                currentTime = maxTime
            }
        }
        
        updateRotationForTime()
    }
    
    private func updateRotationForTime() {
        guard !keyframes.isEmpty else { return }
        
        // Find surrounding keyframes
        let beforeKeyframes = keyframes.filter { $0.time <= currentTime }
        let afterKeyframes = keyframes.filter { $0.time > currentTime }
        
        if beforeKeyframes.isEmpty {
            value = keyframes.first!.rotation
        } else if afterKeyframes.isEmpty {
            value = beforeKeyframes.last!.rotation
        } else {
            let before = beforeKeyframes.last!
            let after = afterKeyframes.first!
            
            let t = (currentTime - before.time) / (after.time - before.time)
            value = simd_slerp(before.rotation, after.rotation, t)
        }
    }
}

// TODO-1558: Rotation editor with rotation curves
struct RotationCurveEditor: View {
    @Binding var value: simd_quatf
    @State private var curveType: CurveType = .linear
    @State private var curveParameter: Float = 0.5
    @State private var customCurve: [CGPoint] = [
        CGPoint(x: 0, y: 0),
        CGPoint(x: 0.3, y: 0.1),
        CGPoint(x: 0.7, y: 0.9),
        CGPoint(x: 1, y: 1)
    ]
    @State private var curveTime: Float = 0
    @State private var isPlaying: Bool = false
    @State private var startRotation: simd_quatf = simd_quatf()
    @State private var endRotation: simd_quatf = simd_quatf(angle: .pi, axis: SIMD3<Float>(0, 1, 0))
    
    enum CurveType: String, CaseIterable {
        case linear = "Linear"
        case easeIn = "Ease In"
        case easeOut = "Ease Out"
        case easeInOut = "Ease In-Out"
        case custom = "Custom"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Rotation Curves")
                .font(.headline)
            
            Picker("Curve Type", selection: $curveType) {
                ForEach(CurveType.allCases, id: \.self) { type in
                    Text(type.rawValue).tag(type)
                }
            }
            .pickerStyle(.segmented)
            
            if curveType == .custom {
                Text("Custom Curve Editor")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                // Simplified curve editor
                RoundedRectangle(cornerRadius: 4)
                    .stroke(Color.gray, lineWidth: 1)
                    .frame(height: 100)
                    .overlay(
                        Path { path in
                            path.move(to: customCurve.first ?? .zero)
                            for point in customCurve.dropFirst() {
                                path.addLine(to: point)
                            }
                        }
                        .stroke(Color.blue, lineWidth: 2)
                    )
            } else {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Curve Parameter:")
                        .font(.caption)
                        .foregroundColor(.secondary)
                    
                    HStack {
                        Slider(value: $curveParameter, in: 0...1)
                        Text(String(format: "%.2f", curveParameter))
                            .font(.caption.monospaced())
                            .frame(width: 40)
                    }
                }
            }
            
            // Playback controls
            HStack {
                Button(isPlaying ? "Pause" : "Play") {
                    isPlaying.toggle()
                    if isPlaying {
                        startRotation = value
                        curveTime = 0
                    }
                }
                .buttonStyle(.bordered)
                
                Button("Reset") {
                    isPlaying = false
                    curveTime = 0
                    value = startRotation
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                Text("Time: \(String(format: "%.2f", curveTime))")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            // Curve visualization
            ProgressView(value: curveTime)
                .progressViewStyle(LinearProgressViewStyle(tint: .blue))
        }
        .onReceive(Timer.publish(every: 0.016, on: .main, in: .common).autoconnect()) { _ in
            if isPlaying {
                curveTime += 0.016
                if curveTime > 1 {
                    curveTime = 0
                }
                updateRotationWithCurve()
            }
        }
    }
    
    private func updateRotationWithCurve() {
        let t = evaluateCurve(curveTime)
        value = simd_slerp(startRotation, endRotation, t)
    }
    
    private func evaluateCurve(_ t: Float) -> Float {
        switch curveType {
        case .linear:
            return t
        case .easeIn:
            return t * t * (3 - 2 * t) * curveParameter + t * (1 - curveParameter)
        case .easeOut:
            return t * t * (3 - 2 * t) * (1 - curveParameter) + t * curveParameter
        case .easeInOut:
            return t * t * (3 - 2 * t)
        case .custom:
            // Simplified custom curve evaluation
            return t
        }
    }
}

// TODO-1559: Rotation editor with rotation constraints
struct AdvancedRotationConstraintsEditor: View {
    @Binding var value: simd_quatf
    @State private var constraints: [RotationConstraint] = []
    @State private var constraintsEnabled: Bool = false
    
    struct RotationConstraint: Identifiable {
        let id = UUID()
        var name: String
        var type: ConstraintType
        var axis: SIMD3<Float>
        var minAngle: Float
        var maxAngle: Float
        var enabled: Bool
    }
    
    enum ConstraintType: String, CaseIterable {
        case hinge = "Hinge"
        case universal = "Universal"
        case ballSocket = "Ball Socket"
        case custom = "Custom"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Advanced Constraints")
                    .font(.headline)
                
                Toggle("Enable Constraints", isOn: $constraintsEnabled)
                    .toggleStyle(.switch)
            }
            
            if constraintsEnabled {
                // Constraints list
                ScrollView {
                    LazyVStack(spacing: 4) {
                        ForEach($constraints) { $constraint in
                            VStack(alignment: .leading, spacing: 4) {
                                HStack {
                                    Toggle("", isOn: $constraint.enabled)
                                        .toggleStyle(.switch)
                                    
                                    TextField("Name", text: $constraint.name)
                                        .textFieldStyle(.roundedBorder)
                                        .frame(width: 100)
                                    
                                    Picker("Type", selection: $constraint.type) {
                                        ForEach(ConstraintType.allCases, id: \.self) { type in
                                            Text(type.rawValue).tag(type)
                                        }
                                    }
                                    .pickerStyle(.menu)
                                    
                                    Button("Remove") {
                                        constraints.removeAll { $0.id == constraint.id }
                                    }
                                    .buttonStyle(.borderedCompact)
                                }
                                
                                HStack {
                                    Text("Min:")
                                    TextField("Min", value: $constraint.minAngle, format: .number)
                                        .textFieldStyle(.roundedBorder)
                                        .frame(width: 60)
                                    
                                    Text("Max:")
                                    TextField("Max", value: $constraint.maxAngle, format: .number)
                                        .textFieldStyle(.roundedBorder)
                                        .frame(width: 60)
                                    
                                    Text("rad")
                                        .font(.caption)
                                        .foregroundColor(.secondary)
                                }
                            }
                            .padding(8)
                            .background(Color.gray.opacity(0.1))
                            .cornerRadius(4)
                        }
                    }
                }
                .frame(height: 200)
                
                HStack {
                    Button("Add Hinge") {
                        constraints.append(RotationConstraint(
                            name: "Hinge \(constraints.count + 1)",
                            type: .hinge,
                            axis: SIMD3<Float>(0, 1, 0),
                            minAngle: -.pi,
                            maxAngle: .pi,
                            enabled: true
                        ))
                    }
                    .buttonStyle(.borderedCompact)
                    
                    Button("Add Ball Socket") {
                        constraints.append(RotationConstraint(
                            name: "Ball Socket \(constraints.count + 1)",
                            type: .ballSocket,
                            axis: SIMD3<Float>(0, 0, 1),
                            minAngle: 0,
                            maxAngle: .pi,
                            enabled: true
                        ))
                    }
                    .buttonStyle(.borderedCompact)
                    
                    Button("Apply Constraints") {
                        applyAllConstraints()
                    }
                    .buttonStyle(.bordered)
                    .disabled(constraints.isEmpty)
                }
            }
        }
    }
    
    private func applyAllConstraints() {
        guard constraintsEnabled else { return }
        
        for constraint in constraints where constraint.enabled {
            value = applyConstraint(value, constraint)
        }
    }
    
    private func applyConstraint(_ rotation: simd_quatf, _ constraint: RotationConstraint) -> simd_quatf {
        switch constraint.type {
        case .hinge:
            return applyHingeConstraint(rotation, constraint)
        case .universal:
            return applyUniversalConstraint(rotation, constraint)
        case .ballSocket:
            return applyBallSocketConstraint(rotation, constraint)
        case .custom:
            return rotation // Simplified
        }
    }
    
    private func applyHingeConstraint(_ rotation: simd_quatf, _ constraint: RotationConstraint) -> simd_quatf {
        // Simplified hinge constraint implementation
        let euler = rotation.eulerAngles
        let clampedAngle = max(constraint.minAngle, min(constraint.maxAngle, euler.y))
        return simd_quatf(eulerAngles: SIMD3<Float>(euler.x, clampedAngle, euler.z))
    }
    
    private func applyUniversalConstraint(_ rotation: simd_quatf, _ constraint: RotationConstraint) -> simd_quatf {
        // Simplified universal constraint implementation
        return rotation
    }
    
    private func applyBallSocketConstraint(_ rotation: simd_quatf, _ constraint: RotationConstraint) -> simd_quatf {
        // Simplified ball socket constraint implementation
        let angle = acos(abs(rotation.vector.w)) * 2
        if angle > constraint.maxAngle {
            let clampedAngle = constraint.maxAngle
            let axis = normalize(SIMD3<Float>(rotation.vector.x, rotation.vector.y, rotation.vector.z))
            return simd_quatf(angle: clampedAngle, axis: axis)
        }
        return rotation
    }
}

// TODO-1560: Rotation editor with rotation validation
struct RotationValidationEditor: View {
    @Binding var value: simd_quatf
    @State private var validationRules: [ValidationRule] = []
    @State private var validationEnabled: Bool = false
    @State private var validationResult: ValidationResult = .valid
    
    struct ValidationRule: Identifiable {
        let id = UUID()
        var name: String
        var type: RuleType
        var parameter: Float
        var enabled: Bool
    }
    
    enum RuleType: String, CaseIterable {
        case normalize = "Normalize"
        case angleLimit = "Angle Limit"
        case axisAlignment = "Axis Alignment"
        case singularityCheck = "Singularity Check"
    }
    
    enum ValidationResult {
        case valid
        case warning(String)
        case error(String)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Rotation Validation")
                    .font(.headline)
                
                Toggle("Enable Validation", isOn: $validationEnabled)
                    .toggleStyle(.switch)
            }
            
            // Validation result
            HStack {
                Image(systemName: validationResultIcon)
                    .foregroundColor(validationResultColor)
                
                Text(validationResultText)
                    .font(.caption)
                    .foregroundColor(validationResultColor)
                
                Spacer()
                
                Button("Validate") {
                    validateRotation()
                }
                .buttonStyle(.borderedCompact)
            }
            .padding(8)
            .background(validationResultBackground)
            .cornerRadius(4)
            
            if validationEnabled {
                // Validation rules list
                ScrollView {
                    LazyVStack(spacing: 4) {
                        ForEach($validationRules) { $rule in
                            HStack {
                                Toggle("", isOn: $rule.enabled)
                                    .toggleStyle(.switch)
                                
                                Picker("Type", selection: $rule.type) {
                                    ForEach(RuleType.allCases, id: \.self) { type in
                                        Text(type.rawValue).tag(type)
                                    }
                                }
                                .pickerStyle(.menu)
                                
                                if rule.type == .angleLimit {
                                    TextField("Limit", value: $rule.parameter, format: .number)
                                        .textFieldStyle(.roundedBorder)
                                        .frame(width: 60)
                                }
                                
                                Button("Remove") {
                                    validationRules.removeAll { $0.id == rule.id }
                                }
                                .buttonStyle(.borderedCompact)
                            }
                        }
                    }
                }
                .frame(height: 120)
                
                HStack {
                    Button("Add Normalize Rule") {
                        validationRules.append(ValidationRule(
                            name: "Normalize",
                            type: .normalize,
                            parameter: 0,
                            enabled: true
                        ))
                    }
                    .buttonStyle(.borderedCompact)
                    
                    Button("Add Angle Limit") {
                        validationRules.append(ValidationRule(
                            name: "Angle Limit",
                            type: .angleLimit,
                            parameter: .pi,
                            enabled: true
                        ))
                    }
                    .buttonStyle(.borderedCompact)
                    
                    Button("Auto Fix") {
                        autoFixRotation()
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
        .onChange(of: value) { _ in
            if validationEnabled {
                validateRotation()
            }
        }
    }
    
    private var validationResultIcon: String {
        switch validationResult {
        case .valid:
            return "checkmark.circle.fill"
        case .warning:
            return "exclamationmark.triangle.fill"
        case .error:
            return "xmark.circle.fill"
        }
    }
    
    private var validationResultColor: Color {
        switch validationResult {
        case .valid:
            return .green
        case .warning:
            return .orange
        case .error:
            return .red
        }
    }
    
    private var validationResultText: String {
        switch validationResult {
        case .valid:
            return "Rotation is valid"
        case .warning(let message):
            return "Warning: \(message)"
        case .error(let message):
            return "Error: \(message)"
        }
    }
    
    private var validationResultBackground: Color {
        switch validationResult {
        case .valid:
            return Color.green.opacity(0.1)
        case .warning:
            return Color.orange.opacity(0.1)
        case .error:
            return Color.red.opacity(0.1)
        }
    }
    
    private func validateRotation() {
        guard validationEnabled else {
            validationResult = .valid
            return
        }
        
        for rule in validationRules where rule.enabled {
            let result = validateRule(rule)
            if case .error(let message) = result {
                validationResult = .error(message)
                return
            } else if case .warning(let message) = result {
                validationResult = .warning(message)
                return
            }
        }
        
        validationResult = .valid
    }
    
    private func validateRule(_ rule: ValidationRule) -> ValidationResult {
        switch rule.type {
        case .normalize:
            let length = simd_length(value.vector)
            if abs(length - 1.0) > 0.001 {
                return .error("Quaternion is not normalized")
            }
            
        case .angleLimit:
            let angle = acos(abs(value.vector.w)) * 2
            if angle > rule.parameter {
                return .warning("Rotation angle exceeds limit")
            }
            
        case .axisAlignment:
            // Check if rotation axis is aligned with major axes
            let euler = value.eulerAngles
            let alignmentThreshold: Float = 0.1
            
            if abs(euler.x) < alignmentThreshold && abs(euler.y) < alignmentThreshold {
                return .warning("Rotation is nearly aligned with Z axis")
            }
            
        case .singularityCheck:
            // Check for gimbal lock proximity
            let euler = value.eulerAngles
            let singularityThreshold: Float = 0.1
            
            if abs(abs(euler.y) - .pi/2) < singularityThreshold {
                return .warning("Near gimbal lock singularity")
            }
        }
        
        return .valid
    }
    
    private func autoFixRotation() {
        var fixedRotation = value
        
        for rule in validationRules where rule.enabled {
            switch rule.type {
            case .normalize:
                fixedRotation = normalize(fixedRotation)
                
            case .angleLimit:
                let angle = acos(abs(fixedRotation.vector.w)) * 2
                if angle > rule.parameter {
                    let clampedAngle = rule.parameter
                    let axis = normalize(SIMD3<Float>(fixedRotation.vector.x, fixedRotation.vector.y, fixedRotation.vector.z))
                    fixedRotation = simd_quatf(angle: clampedAngle, axis: axis)
                }
                
            case .axisAlignment:
                // Snap to nearest axis alignment
                let euler = fixedRotation.eulerAngles
                let snappedEuler = SIMD3<Float>(
                    round(euler.x / .pi/2) * .pi/2,
                    round(euler.y / .pi/2) * .pi/2,
                    round(euler.z / .pi/2) * .pi/2
                )
                fixedRotation = simd_quatf(eulerAngles: snappedEuler)
                
            case .singularityCheck:
                // Move away from singularity
                let euler = fixedRotation.eulerAngles
                if abs(abs(euler.y) - .pi/2) < 0.1 {
                    let adjustedEuler = SIMD3<Float>(euler.x, euler.y + 0.1, euler.z)
                    fixedRotation = simd_quatf(eulerAngles: adjustedEuler)
                }
            }
        }
        
        value = fixedRotation
        validateRotation()
    }
}

// MARK: - Rotation Helper Extensions
extension DateFormatter {
    static let timeFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.timeStyle = .medium
        return formatter
    }()
}

extension simd_quatf {
    var eulerAngles: SIMD3<Float> {
        // Convert quaternion to Euler angles (XYZ order)
        let q = self
        
        // Roll (X-axis rotation)
        let sinr_cosp = 2 * (q.vector.w * q.vector.x + q.vector.y * q.vector.z)
        let cosr_cosp = 1 - 2 * (q.vector.x * q.vector.x + q.vector.y * q.vector.y)
        let roll = atan2(sinr_cosp, cosr_cosp)
        
        // Pitch (Y-axis rotation)
        let sinp = 2 * (q.vector.w * q.vector.y - q.vector.z * q.vector.x)
        let pitch = abs(sinp) >= 1 ? .pi / 2 * sinp : asin(sinp)
        
        // Yaw (Z-axis rotation)
        let siny_cosp = 2 * (q.vector.w * q.vector.z + q.vector.x * q.vector.y)
        let cosy_cosp = 1 - 2 * (q.vector.y * q.vector.y + q.vector.z * q.vector.z)
        let yaw = atan2(siny_cosp, cosy_cosp)
        
        return SIMD3<Float>(roll, pitch, yaw)
    }
    
    init(eulerAngles: SIMD3<Float>) {
        // Convert Euler angles to quaternion (XYZ order)
        let roll = eulerAngles.x * 0.5
        let pitch = eulerAngles.y * 0.5
        let yaw = eulerAngles.z * 0.5
        
        let cr = cos(roll)
        let sr = sin(roll)
        let cp = cos(pitch)
        let sp = sin(pitch)
        let cy = cos(yaw)
        let sy = sin(yaw)
        
        let w = cr * cp * cy + sr * sp * sy
        let x = sr * cp * cy - cr * sp * sy
        let y = cr * sp * cy + sr * cp * sy
        let z = cr * cp * sy - sr * sp * cy
        
        self.init(vector: SIMD4<Float>(x, y, z, w))
    }
    
    var axisAngle: (axis: SIMD3<Float>, angle: Float) {
        let q = normalize(self)
        let angle = acos(q.vector.w) * 2
        
        if abs(angle) < 0.001 {
            return (SIMD3<Float>(0, 1, 0), 0)
        }
        
        let sinHalfAngle = sin(angle * 0.5)
        let axis = normalize(SIMD3<Float>(
            q.vector.x / sinHalfAngle,
            q.vector.y / sinHalfAngle,
            q.vector.z / sinHalfAngle
        ))
        
        return (axis, angle)
    }
    
    init(angle: Float, axis: SIMD3<Float>) {
        let normalizedAxis = normalize(axis)
        let halfAngle = angle * 0.5
        let sinHalfAngle = sin(halfAngle)
        let cosHalfAngle = cos(halfAngle)
        
        self.init(vector: SIMD4<Float>(
            normalizedAxis.x * sinHalfAngle,
            normalizedAxis.y * sinHalfAngle,
            normalizedAxis.z * sinHalfAngle,
            cosHalfAngle
        ))
    }
}

// Simple rotation preview view
struct RotationPreviewView: View {
    let rotation: simd_quatf
    
    var body: some View {
        ZStack {
            // Background circle
            Circle()
                .stroke(Color.gray, lineWidth: 1)
                .frame(width: 60, height: 60)
            
            // Rotation indicator (simplified 3D representation)
            ForEach(0..<3, id: \.self) { axis in
                let axisVector = axis == 0 ? SIMD3<Float>(1, 0, 0) :
                                 axis == 1 ? SIMD3<Float>(0, 1, 0) :
                                 SIMD3<Float>(0, 0, 1)
                let rotatedAxis = rotation.act(on: axisVector)
                
                Path { path in
                    path.move(to: CGPoint(x: 30, y: 30))
                    path.addLine(to: CGPoint(
                        x: 30 + CGFloat(rotatedAxis.x * 20),
                        y: 30 - CGFloat(rotatedAxis.y * 20)
                    ))
                }
                .stroke(axis == 0 ? Color.red : axis == 1 ? Color.green : Color.blue, lineWidth: 2)
            }
        }
    }
}

extension simd_quatf {
    func act(on vector: SIMD3<Float>) -> SIMD3<Float> {
        // Rotate a vector by this quaternion
        let qv = SIMD4<Float>(vector.x, vector.y, vector.z, 0)
        let result = self * simd_quatf(vector: qv) * self.inverse
        return SIMD3<Float>(result.vector.x, result.vector.y, result.vector.z)
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

// MARK: - Enhanced Vector3 Editor (TODO-1467)
struct Vector3PropertyEditor: View, PropertyEditor {
    let label: String
    @Binding var value: SIMD3<Float>
    
    // Configuration
    let config: PropertyEditorConfig<SIMD3<Float>>
    let min: Float?
    let max: Float?
    let step: Float?
    let precision: Int
    let unit: String?
    let showSwizzle: Bool
    
    // Protocol conformance
    var defaultValue: SIMD3<Float>? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit ?? unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((SIMD3<Float>) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    @State private var showTooltip = false
    @State private var showSwizzleMenu = false
    @State private var showExpressionDialog = false
    @State private var expression = ""
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Header with controls
            headerSection
            
            // Vector fields
            vectorFieldsSection
            
            // Swizzle controls
            if showSwizzle {
                swizzleSection
            }
        }
        .contextMenu {
            contextMenuItems
        }
        .onHover { showTooltip = $0 }
        .popover(isPresented: $showTooltip) {
            if let tooltip = tooltip {
                Text(tooltip)
                    .font(DesignSystem.Typography.small)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
            }
        }
        .sheet(isPresented: $showExpressionDialog) {
            expressionDialog
        }
    }
    
    @ViewBuilder
    private var headerSection: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            // Property controls
            HStack(spacing: 4) {
                // Lock button
                Button(action: { config.onLockToggle?(!isLocked) }) {
                    Image(systemName: isLocked ? "lock.fill" : "lock.open")
                        .foregroundColor(isLocked ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Lock/Unlock property")
                
                // Reset button
                Button(action: { resetValue() }) {
                    Image(systemName: "arrow.counterclockwise")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Reset to default value")
                
                // Animation keyframe button
                Button(action: { config.onAnimationKeyframe?() }) {
                    Image(systemName: "key")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Add animation keyframe")
                
                // Expression button
                Button(action: { showExpressionDialog = true }) {
                    Image(systemName: "function")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Set expression")
                
                // Link button
                Button(action: { config.onLinkProperty?() }) {
                    Image(systemName: "link")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Link property")
                
                // Swizzle button
                if showSwizzle {
                    Button(action: { showSwizzleMenu = true }) {
                        Image(systemName: "arrow.up.arrow.down.arrow.left.arrow.right")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .help("Swizzle components")
                }
            }
        }
    }
    
    @ViewBuilder
    private var vectorFieldsSection: some View {
        HStack(spacing: 4) {
            EnhancedAxisField(
                axis: "X",
                value: $value.x,
                color: .red,
                isLocked: isLocked,
                min: min,
                max: max,
                step: step,
                precision: precision,
                unit: unit
            )
            
            EnhancedAxisField(
                axis: "Y",
                value: $value.y,
                color: .green,
                isLocked: isLocked,
                min: min,
                max: max,
                step: step,
                precision: precision,
                unit: unit
            )
            
            EnhancedAxisField(
                axis: "Z",
                value: $value.z,
                color: .blue,
                isLocked: isLocked,
                min: min,
                max: max,
                step: step,
                precision: precision,
                unit: unit
            )
        }
    }
    
    @ViewBuilder
    private var swizzleSection: some View {
        HStack {
            Text("Swizzle:")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Menu {
                Button("XYZ → XZY") { swizzleComponents(x: 0, y: 2, z: 1) }
                Button("XYZ → YXZ") { swizzleComponents(x: 1, y: 0, z: 2) }
                Button("XYZ → ZXY") { swizzleComponents(x: 2, y: 0, z: 1) }
                Button("XYZ → ZYX") { swizzleComponents(x: 2, y: 1, z: 0) }
                Button("XYZ → YZX") { swizzleComponents(x: 1, y: 2, z: 0) }
            } label: {
                Text("XYZ")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
            }
            .menuStyle(.borderedButton)
        }
    }
    
    @ViewBuilder
    private var contextMenuItems: some View {
        Group {
            Button("Copy Value") { copyValue() }
            Button("Paste Value") { pasteValue() }
            Button("Duplicate Value") { duplicateValue() }
            Divider()
            Button("Normalize") { normalizeVector() }
            Button("Zero") { zeroVector() }
            Button("Invert") { invertVector() }
            Divider()
            Button("Set to X") { setToAxis(0) }
            Button("Set to Y") { setToAxis(1) }
            Button("Set to Z") { setToAxis(2) }
        }
    }
    
    @ViewBuilder
    private var expressionDialog: some View {
        VStack {
            Text("Vector Expression")
                .font(DesignSystem.Typography.headline)
            
            TextField("Enter expression...", text: $expression)
                .textFieldStyle(.roundedBorder)
            
            HStack {
                Button("Cancel") { showExpressionDialog = false }
                Button("Apply") { applyExpression() }
            }
        }
        .padding()
        .frame(width: 300)
    }
    
    // MARK: - Helper Methods
    
    private func resetValue() {
        guard let defaultValue = defaultValue else { return }
        undoManager.recordState(value)
        value = defaultValue
        config.onValueChanged?(value)
    }
    
    private func copyValue() {
        clipboardManager.copy(value, type: "SIMD3<Float>")
    }
    
    private func pasteValue() {
        guard let pastedValue: SIMD3<Float> = clipboardManager.paste(as: SIMD3<Float>.self) else { return }
        undoManager.recordState(value)
        value = pastedValue
        config.onValueChanged?(value)
    }
    
    private func duplicateValue() {
        // Vector duplication logic
    }
    
    private func normalizeVector() {
        let length = simd_length(value)
        guard length > 0.001 else { return }
        undoManager.recordState(value)
        value = value / length
        config.onValueChanged?(value)
    }
    
    private func zeroVector() {
        undoManager.recordState(value)
        value = SIMD3<Float>(0, 0, 0)
        config.onValueChanged?(value)
    }
    
    private func invertVector() {
        undoManager.recordState(value)
        value = -value
        config.onValueChanged?(value)
    }
    
    private func setToAxis(_ axis: Int) {
        undoManager.recordState(value)
        let newValue = SIMD3<Float>(axis == 0 ? 1 : 0, axis == 1 ? 1 : 0, axis == 2 ? 1 : 0)
        value = newValue
        config.onValueChanged?(value)
    }
    
    private func swizzleComponents(x: Int, y: Int, z: Int) {
        let components = [value.x, value.y, value.z]
        undoManager.recordState(value)
        value = SIMD3<Float>(components[x], components[y], components[z])
        config.onValueChanged?(value)
    }
    
    private func applyExpression() {
        // Expression evaluation logic
        showExpressionDialog = false
    }
}

// MARK: - Enhanced Axis Field Component
private struct EnhancedAxisField: View {
    let axis: String
    @Binding var value: Float
    let color: Color
    let isLocked: Bool
    let min: Float?
    let max: Float?
    let step: Float?
    let precision: Int
    let unit: String?
    
    @StateObject private var undoManager = PropertyUndoManager()
    @State private var isHovering = false
    @State private var showTooltip = false
    
    var body: some View {
        HStack(spacing: 2) {
            Text(axis)
                .font(DesignSystem.Typography.small)
                .foregroundColor(color)
                .frame(width: 14)
            
            TextField("", value: $value, format: .number.precision(.fractionLength(precision)))
                .textFieldStyle(.plain)
                .font(DesignSystem.Typography.mono)
                .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                .disabled(isLocked)
                .frame(minWidth: 50)
                .background(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(isHovering ? DesignSystem.Colors.backgroundSecondary : DesignSystem.Colors.backgroundPrimary)
                        .stroke(color.opacity(0.5), lineWidth: 1)
                )
                .onHover { isHovering = $0 }
                .contextMenu {
                    contextMenuItems
                }
                .onChange(of: value) { newValue in
                    // Apply clamping
                    var clampedValue = newValue
                    if let min = min { clampedValue = max(clampedValue, min) }
                    if let max = max { clampedValue = min(clampedValue, max) }
                    if clampedValue != newValue {
                        value = clampedValue
                    }
                }
        }
        .padding(.horizontal, 6)
        .padding(.vertical, 4)
    }
    
    @ViewBuilder
    private var contextMenuItems: some View {
        Group {
            Button("Copy Value") { copyValue() }
            Button("Paste Value") { pasteValue() }
            Button("Reset to 0") { resetToZero() }
            Divider()
            if let min = min {
                Button("Set to Min (\(String(format: "%.\(precision)f", min)))") { value = min }
            }
            if let max = max {
                Button("Set to Max (\(String(format: "%.\(precision)f", max)))") { value = max }
            }
        }
    }
    
    private func copyValue() {
        undoManager.recordState(value)
        // Copy to clipboard logic
    }
    
    private func pasteValue() {
        undoManager.recordState(value)
        // Paste from clipboard logic
    }
    
    private func resetToZero() {
        undoManager.recordState(value)
        value = 0
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

// MARK: - Enhanced Color Editor (TODO-1470)
struct ColorPropertyEditor: View, PropertyEditor {
    let label: String
    @Binding var value: Color
    
    // Configuration
    let config: PropertyEditorConfig<Color>
    let allowHDR: Bool
    let showAlpha: Bool
    let colorSpace: ColorSpace
    
    // Protocol conformance
    var defaultValue: Color? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Color) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    @State private var showPicker = false
    @State private var showAdvanced = false
    @State private var showTooltip = false
    @State private var showExpressionDialog = false
    @State private var expression = ""
    @State private var useHDR = false
    @State private var intensity: Float = 1.0
    
    enum ColorSpace: String, CaseIterable {
        case sRGB = "sRGB"
        case linear = "Linear"
        case HDR = "HDR"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Header with controls
            headerSection
            
            // Color controls
            colorControlsSection
            
            // Advanced controls
            if showAdvanced {
                advancedControlsSection
            }
        }
        .contextMenu {
            contextMenuItems
        }
        .onHover { showTooltip = $0 }
        .popover(isPresented: $showTooltip) {
            if let tooltip = tooltip {
                Text(tooltip)
                    .font(DesignSystem.Typography.small)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
            }
        }
        .sheet(isPresented: $showExpressionDialog) {
            expressionDialog
        }
    }
    
    @ViewBuilder
    private var headerSection: some View {
        HStack {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Spacer()
            
            // Property controls
            HStack(spacing: 4) {
                // Lock button
                Button(action: { config.onLockToggle?(!isLocked) }) {
                    Image(systemName: isLocked ? "lock.fill" : "lock.open")
                        .foregroundColor(isLocked ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Lock/Unlock property")
                
                // Reset button
                Button(action: { resetValue() }) {
                    Image(systemName: "arrow.counterclockwise")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Reset to default value")
                
                // Animation keyframe button
                Button(action: { config.onAnimationKeyframe?() }) {
                    Image(systemName: "key")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Add animation keyframe")
                
                // Expression button
                Button(action: { showExpressionDialog = true }) {
                    Image(systemName: "function")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Set expression")
                
                // Link button
                Button(action: { config.onLinkProperty?() }) {
                    Image(systemName: "link")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Link property")
                
                // Advanced toggle
                Button(action: { showAdvanced.toggle() }) {
                    Image(systemName: showAdvanced ? "chevron.up" : "chevron.down")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .buttonStyle(.plain)
                .help("Toggle advanced options")
            }
        }
    }
    
    @ViewBuilder
    private var colorControlsSection: some View {
        HStack(spacing: 8) {
            // Color picker
            ColorPicker("", selection: $value)
                .labelsHidden()
                .disabled(isLocked)
            
            // Color preview with HDR indication
            colorPreview
            
            // Hex value field
            hexField
            
            // RGB fields
            rgbFields
        }
    }
    
    @ViewBuilder
    private var colorPreview: some View {
        RoundedRectangle(cornerRadius: 4)
            .fill(useHDR ? hdrValue : value)
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
                                    .font(DesignSystem.Typography.micro)
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
    }
    
    @ViewBuilder
    private var hexField: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text("Hex")
                .font(DesignSystem.Typography.micro)
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            TextField("#RRGGBB", text: .constant(value.toHex()))
                .textFieldStyle(.roundedBorder)
                .font(DesignSystem.Typography.mono)
                .disabled(isLocked)
        }
        .frame(width: 80)
    }
    
    @ViewBuilder
    private var rgbFields: some View {
        VStack(spacing: 2) {
            HStack(spacing: 4) {
                rgbField("R", value: .constant(value.components.red), color: .red)
                rgbField("G", value: .constant(value.components.green), color: .green)
                rgbField("B", value: .constant(value.components.blue), color: .blue)
            }
            
            if showAlpha {
                rgbField("A", value: .constant(value.components.alpha), color: .gray)
            }
        }
    }
    
    @ViewBuilder
    private var advancedControlsSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            // HDR controls
            if allowHDR {
                HStack {
                    Toggle("HDR", isOn: $useHDR)
                        .toggleStyle(.switch)
                    
                    if useHDR {
                        HStack {
                            Text("Intensity:")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Slider(value: $intensity, in: 0.1...10.0)
                                .frame(width: 100)
                            
                            Text(String(format: "%.1f", intensity))
                                .font(DesignSystem.Typography.mono)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    }
                }
            }
            
            // Color space picker
            HStack {
                Text("Color Space:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Picker("", selection: .constant(colorSpace)) {
                    ForEach(ColorSpace.allCases, id: \.self) { space in
                        Text(space.rawValue).tag(space)
                    }
                }
                .pickerStyle(.menu)
                .disabled(isLocked)
            }
            
            // HSV values
            hsvValues
        }
    }
    
    @ViewBuilder
    private var hsvValues: some View {
        let hsv = value.toHSV()
        
        HStack(spacing: 8) {
            VStack(alignment: .leading, spacing: 2) {
                Text("H")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                Text(String(format: "%.0f°", hsv.hue * 360))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            VStack(alignment: .leading, spacing: 2) {
                Text("S")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                Text(String(format: "%.0f%%", hsv.saturation * 100))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            VStack(alignment: .leading, spacing: 2) {
                Text("V")
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                Text(String(format: "%.0f%%", hsv.value * 100))
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
        }
    }
    
    @ViewBuilder
    private var contextMenuItems: some View {
        Group {
            Button("Copy Color") { copyColor() }
            Button("Paste Color") { pasteColor() }
            Button("Copy Hex") { copyHex() }
            Button("Paste Hex") { pasteHex() }
            Divider()
            Button("Invert Color") { invertColor() }
            Button("Grayscale") { grayscaleColor() }
            Button("Random Color") { randomColor() }
            Divider()
            Button("Set to Red") { setColor(.red) }
            Button("Set to Green") { setColor(.green) }
            Button("Set to Blue") { setColor(.blue) }
            Button("Set to White") { setColor(.white) }
            Button("Set to Black") { setColor(.black) }
        }
    }
    
    @ViewBuilder
    private var expressionDialog: some View {
        VStack {
            Text("Color Expression")
                .font(DesignSystem.Typography.headline)
            
            TextField("Enter color expression...", text: $expression)
                .textFieldStyle(.roundedBorder)
            
            HStack {
                Button("Cancel") { showExpressionDialog = false }
                Button("Apply") { applyExpression() }
            }
        }
        .padding()
        .frame(width: 300)
    }
    
    // MARK: - Helper Methods
    
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
    
    private func rgbField(_ label: String, value: Binding<Float>, color: Color) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(label)
                .font(DesignSystem.Typography.micro)
                .foregroundColor(color)
            
            TextField("", value: value, format: .number.precision(.fractionLength(0)))
                .textFieldStyle(.roundedBorder)
                .font(DesignSystem.Typography.mono)
                .disabled(isLocked)
                .frame(width: 40)
        }
    }
    
    private func resetValue() {
        guard let defaultValue = defaultValue else { return }
        undoManager.recordState(value)
        value = defaultValue
        config.onValueChanged?(value)
    }
    
    private func copyColor() {
        clipboardManager.copy(value, type: "Color")
    }
    
    private func pasteColor() {
        guard let pastedValue: Color = clipboardManager.paste(as: Color.self) else { return }
        undoManager.recordState(value)
        value = pastedValue
        config.onValueChanged?(value)
    }
    
    private func copyHex() {
        let hex = value.toHex()
        // Copy hex to clipboard
    }
    
    private func pasteHex() {
        // Paste hex from clipboard
    }
    
    private func invertColor() {
        let components = value.components
        undoManager.recordState(value)
        value = Color(
            red: 1.0 - components.red,
            green: 1.0 - components.green,
            blue: 1.0 - components.blue,
            opacity: components.alpha
        )
        config.onValueChanged?(value)
    }
    
    private func grayscaleColor() {
        let components = value.components
        let gray = (components.red + components.green + components.blue) / 3.0
        undoManager.recordState(value)
        value = Color(
            red: gray,
            green: gray,
            blue: gray,
            opacity: components.alpha
        )
        config.onValueChanged?(value)
    }
    
    private func randomColor() {
        undoManager.recordState(value)
        value = Color(
            red: Float.random(in: 0...1),
            green: Float.random(in: 0...1),
            blue: Float.random(in: 0...1),
            opacity: value.components.alpha
        )
        config.onValueChanged?(value)
    }
    
    private func setColor(_ color: Color) {
        undoManager.recordState(value)
        value = Color(
            red: color.components.red,
            green: color.components.green,
            blue: color.components.blue,
            opacity: value.components.alpha
        )
        config.onValueChanged?(value)
    }
    
    private func applyExpression() {
        // Expression evaluation logic
        showExpressionDialog = false
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

// MARK: - Enhanced Enum/Dropdown Editor (TODO-1478)
struct EnumPropertyEditor<T: Hashable & CaseIterable & CustomStringConvertible>: View, PropertyEditor where T.AllCases: RandomAccessCollection {
    let label: String
    @Binding var value: T
    
    // Configuration
    let config: PropertyEditorConfig<T>
    
    // Protocol conformance
    var defaultValue: T? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((T) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var showExpressionDialog = false
    @State private var expressionText = ""
    @State private var isHovering = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    var body: some View {
        HStack(spacing: 8) {
            // Label with tooltip
            HStack(spacing: 4) {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                    .frame(width: 80, alignment: .leading)
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
                
                if isOverridden {
                    Image(systemName: "arrow.triangle.2.circlepath")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            
            // Enum picker with enhanced features
            HStack(spacing: 4) {
                Picker("", selection: $value) {
                    ForEach(Array(T.allCases), id: \.self) { option in
                        Text(option.description)
                            .tag(option)
                    }
                }
                .pickerStyle(.menu)
                .disabled(isLocked)
                .opacity(isLocked ? 0.6 : 1.0)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(
                    RoundedRectangle(cornerRadius: 4)
                        .fill(isLocked ? DesignSystem.Colors.backgroundDisabled : DesignSystem.Colors.backgroundPrimary)
                        .stroke(isOverridden ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: isOverridden ? 2 : 1)
                )
                .contextMenu {
                    contextMenuContent
                }
                .onChange(of: value) { oldValue, newValue in
                    if !isLocked {
                        undoManager.recordState(oldValue)
                        onValueChanged?(newValue)
                    }
                }
                
                // Control buttons
                controlButtons
            }
        }
        .sheet(isPresented: $showExpressionDialog) {
            expressionDialog
        }
    }
    
    @ViewBuilder
    private var controlButtons: some View {
        HStack(spacing: 2) {
            // Lock button
            Button(action: { onLockToggle?(!isLocked) }) {
                Image(systemName: isLocked ? "lock.fill" : "lock.open.fill")
                    .font(.caption)
                    .foregroundColor(isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            .help(isLocked ? "Unlock property" : "Lock property")
            
            // Reset button
            if let defaultValue = defaultValue, value != defaultValue {
                Button(action: { resetToDefault() }) {
                    Image(systemName: "arrow.counterclockwise")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Reset to default value")
            }
            
            // Animation keyframe button
            if let onAnimationKeyframe = onAnimationKeyframe {
                Button(action: onAnimationKeyframe) {
                    Image(systemName: "key")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Set animation keyframe")
            }
            
            // Expression button
            if let onExpressionInput = onExpressionInput {
                Button(action: { 
                    expressionText = value.description
                    showExpressionDialog = true 
                }) {
                    Image(systemName: "function")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Expression input")
            }
            
            // Link button
            if let onLinkProperty = onLinkProperty {
                Button(action: onLinkProperty) {
                    Image(systemName: "link")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Link property")
            }
        }
    }
    
    @ViewBuilder
    private var contextMenuContent: some View {
        Group {
            Button("Copy Value") {
                clipboardManager.copy(value.description)
            }
            .keyboardShortcut("c", modifiers: .command)
            
            Button("Paste Value") {
                pasteValue()
            }
            .keyboardShortcut("v", modifiers: .command)
            
            if let defaultValue = defaultValue {
                Button("Reset to Default") {
                    resetToDefault()
                }
                .keyboardShortcut("r", modifiers: .command)
            }
            
            Divider()
            
            Button("Expression Input") {
                expressionText = value.description
                showExpressionDialog = true
            }
            .keyboardShortcut("e", modifiers: .command)
            
            if let onAnimationKeyframe = onAnimationKeyframe {
                Button("Set Keyframe") {
                    onAnimationKeyframe()
                }
                .keyboardShortcut("k", modifiers: .command)
            }
            
            Divider()
            
            Button(isLocked ? "Unlock" : "Lock") {
                onLockToggle?(!isLocked)
            }
        }
    }
    
    @ViewBuilder
    private var expressionDialog: some View {
        NavigationView {
            VStack(alignment: .leading, spacing: 16) {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Expression")
                        .font(DesignSystem.Typography.headline)
                    
                    Text("Enter an expression that evaluates to one of the available enum options:")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Available options: \(T.allCases.map(\.description).joined(separator: ", "))")
                        .font(DesignSystem.Typography.caption.monospaced())
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundSecondary)
                        .cornerRadius(4)
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("Expression:")
                        .font(DesignSystem.Typography.small)
                    
                    TextEditor(text: $expressionText)
                        .font(DesignSystem.Typography.body.monospaced())
                        .frame(minHeight: 100)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundPrimary)
                        .cornerRadius(6)
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(DesignSystem.Colors.border, lineWidth: 1)
                        )
                }
                
                Spacer()
            }
            .padding()
            .navigationTitle("Expression Input")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        showExpressionDialog = false
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Apply") {
                        applyExpression()
                        showExpressionDialog = false
                    }
                    .disabled(expressionText.isEmpty)
                }
            }
        }
    }
    
    private func resetToDefault() {
        guard let defaultValue = defaultValue else { return }
        undoManager.recordState(value)
        value = defaultValue
        onReset?()
        onValueChanged?(defaultValue)
    }
    
    private func pasteValue() {
        guard let pastedValue = clipboardManager.paste() else { return }
        
        // Try to find matching enum case
        for caseValue in T.allCases {
            if caseValue.description.lowercased() == pastedValue.lowercased() {
                undoManager.recordState(value)
                value = caseValue
                onValueChanged?(caseValue)
                break
            }
        }
    }
    
    private func applyExpression() {
        // Simple expression evaluation - try to match enum case
        let trimmedExpression = expressionText.trimmingCharacters(in: .whitespacesAndNewlines)
        
        for caseValue in T.allCases {
            if caseValue.description.lowercased() == trimmedExpression.lowercased() {
                undoManager.recordState(value)
                value = caseValue
                onValueChanged?(caseValue)
                break
            }
        }
    }
}

// MARK: - Enhanced Asset Reference Editor (TODO-31498, TODO-31497, TODO-31496)
struct AssetReferenceEditor: View, PropertyEditor {
    let label: String
    @Binding var assetPath: String
    let assetType: AssetType
    
    // Configuration
    let config: PropertyEditorConfig<String>
    
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
    
    // State
    @State private var showPicker = false
    @State private var showCreator = false
    @State private var showExpressionDialog = false
    @State private var expressionText = ""
    @State private var searchFilter = ""
    @State private var availableAssets: [AssetInfo] = []
    @State private var selectedVariant: String = ""
    @State private var previewImage: Image?
    @State private var isLoadingPreview = false
    @State private var isHovering = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    struct AssetInfo: Identifiable, Codable {
        let id = UUID()
        let name: String
        let path: String
        let type: AssetType
        let description: String
        let variants: [AssetVariant]
        let thumbnailPath: String?
        let fileSize: Int64
        let lastModified: Date
        let tags: [String]
    }
    
    struct AssetVariant: Identifiable, Codable {
        let id = UUID()
        let name: String
        let path: String
        let description: String
        let thumbnailPath: String?
    }
    
    enum AssetType: String, CaseIterable {
        case material = "Material"
        case texture = "Texture"
        case mesh = "Mesh"
        case audio = "Audio"
        case animation = "Animation"
        case prefab = "Prefab"
        
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
        
        var fileExtensions: [String] {
            switch self {
            case .material: return [".mat", ".material"]
            case .texture: return [".png", ".jpg", ".jpeg", ".tga", ".hdr", ".exr"]
            case .mesh: return [".fbx", ".obj", ".gltf", ".glb"]
            case .audio: return [".wav", ".mp3", ".ogg", ".flac"]
            case .animation: return [".anim", ".fbx"]
            case .prefab: return [".prefab", ".json"]
            }
        }
    }
    
    private var currentAsset: AssetInfo? {
        availableAssets.first { $0.path == assetPath }
    }
    
    private var currentVariant: AssetVariant? {
        currentAsset?.variants.first { $0.name == selectedVariant }
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Label with tooltip and override indicator
            HStack(spacing: 4) {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
                
                if isOverridden {
                    Image(systemName: "arrow.triangle.2.circlepath")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            
            // Main asset selection
            HStack(spacing: 8) {
                // Preview thumbnail
                AsyncImage(url: URL(string: currentVariant?.thumbnailPath ?? currentAsset?.thumbnailPath ?? "")) { image in
                    image
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                } placeholder: {
                    RoundedRectangle(cornerRadius: 4)
                        .fill(isLocked ? DesignSystem.Colors.backgroundDisabled : DesignSystem.Colors.backgroundTertiary)
                        .overlay(
                            Image(systemName: assetType.icon)
                                .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textTertiary)
                                .font(.title2)
                        )
                }
                .frame(width: 48, height: 48)
                .clipped()
                .cornerRadius(4)
                .opacity(isLocked ? 0.6 : 1.0)
                
                // Asset info and controls
                VStack(alignment: .leading, spacing: 4) {
                    // Asset name and path
                    HStack {
                        VStack(alignment: .leading, spacing: 2) {
                            Text(assetPath.isEmpty ? "None" : currentAsset?.name ?? URL(fileURLWithPath: assetPath).lastPathComponent)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(assetPath.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                            
                            if !assetPath.isEmpty {
                                Text(assetPath)
                                    .font(DesignSystem.Typography.mono)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                                    .font(.caption)
                                    .lineLimit(1)
                            }
                        }
                        
                        Spacer()
                        
                        // Enhanced action buttons
                        HStack(spacing: 2) {
                            // Lock button
                            Button(action: { onLockToggle?(!isLocked) }) {
                                Image(systemName: isLocked ? "lock.fill" : "lock.open.fill")
                                    .font(.caption)
                                    .foregroundColor(isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            }
                            .buttonStyle(.plain)
                            .help(isLocked ? "Unlock property" : "Lock property")
                            
                            // Browse button
                            Button(action: { showPicker = true }) {
                                Image(systemName: "magnifyingglass")
                                    .font(.caption)
                                    .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                            }
                            .buttonStyle(.plain)
                            .disabled(isLocked)
                            .help("Browse Assets")
                            
                            // Create button
                            Button(action: { showCreator = true }) {
                                Image(systemName: "plus.circle")
                                    .font(.caption)
                                    .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                            }
                            .buttonStyle(.plain)
                            .disabled(isLocked)
                            .help("Create New Asset")
                            
                            // Clear button
                            if !assetPath.isEmpty {
                                Button(action: { 
                                    undoManager.recordState(assetPath)
                                    assetPath = ""
                                    onValueChanged?("")
                                }) {
                                    Image(systemName: "xmark.circle.fill")
                                        .font(.caption)
                                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                                }
                                .buttonStyle(.plain)
                                .disabled(isLocked)
                                .help("Clear Asset")
                            }
                            
                            // Reset button
                            if let defaultValue = defaultValue, assetPath != defaultValue {
                                Button(action: { resetToDefault() }) {
                                    Image(systemName: "arrow.counterclockwise")
                                        .font(.caption)
                                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                                }
                                .buttonStyle(.plain)
                                .disabled(isLocked)
                                .help("Reset to default value")
                            }
                            
                            // Animation keyframe button
                            if let onAnimationKeyframe = onAnimationKeyframe {
                                Button(action: onAnimationKeyframe) {
                                    Image(systemName: "key")
                                        .font(.caption)
                                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                                }
                                .buttonStyle(.plain)
                                .disabled(isLocked)
                                .help("Set animation keyframe")
                            }
                            
                            // Expression button
                            if let onExpressionInput = onExpressionInput {
                                Button(action: { 
                                    expressionText = assetPath
                                    showExpressionDialog = true 
                                }) {
                                    Image(systemName: "function")
                                        .font(.caption)
                                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                                }
                                .buttonStyle(.plain)
                                .disabled(isLocked)
                                .help("Expression input")
                            }
                            
                            // Link button
                            if let onLinkProperty = onLinkProperty {
                                Button(action: onLinkProperty) {
                                    Image(systemName: "link")
                                        .font(.caption)
                                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                                }
                                .buttonStyle(.plain)
                                .disabled(isLocked)
                                .help("Link property")
                            }
                        }
                    }
                    
                    // Variant selection (if available)
                    if let asset = currentAsset, !asset.variants.isEmpty {
                        HStack {
                            Text("Variant:")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Menu {
                                Button("Default") { selectedVariant = "" }
                                Divider()
                                ForEach(asset.variants) { variant in
                                    Button(action: { selectedVariant = variant.name }) {
                                        HStack {
                                            Text(variant.name)
                                            if selectedVariant == variant.name {
                                                Spacer()
                                                Image(systemName: "checkmark")
                                            }
                                        }
                                    }
                                }
                            } label: {
                                HStack {
                                    Text(selectedVariant.isEmpty ? "Default" : selectedVariant)
                                        .font(DesignSystem.Typography.small)
                                    Spacer()
                                    Image(systemName: "chevron.down")
                                        .font(.caption)
                                }
                                .padding(.horizontal, 6)
                                .padding(.vertical, 2)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(3)
                            }
                            .buttonStyle(.plain)
                            
                            Spacer()
                        }
                    }
                }
            }
            .padding(8)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(isLocked ? DesignSystem.Colors.backgroundDisabled : DesignSystem.Colors.backgroundPrimary)
                    .stroke(isOverridden ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: isOverridden ? 2 : 1)
            )
            .contextMenu {
                contextMenuContent
            }
            .onChange(of: assetPath) { oldValue, newValue in
                if !isLocked {
                    undoManager.recordState(oldValue)
                    onValueChanged?(newValue)
                }
            }
        }
        .sheet(isPresented: $showPicker) {
            AssetPickerSheet(
                selectedAsset: $assetPath,
                selectedVariant: $selectedVariant,
                searchFilter: $searchFilter,
                availableAssets: $availableAssets,
                assetType: assetType,
                isPresented: $showPicker
            )
        }
        .sheet(isPresented: $showCreator) {
            AssetCreatorSheet(
                assetType: assetType,
                isPresented: $showCreator,
                onAssetCreated: { newPath in
                    undoManager.recordState(assetPath)
                    assetPath = newPath
                    loadAvailableAssets()
                    onValueChanged?(newPath)
                }
            )
        }
        .sheet(isPresented: $showExpressionDialog) {
            expressionDialog
        }
        .onAppear {
            loadAvailableAssets()
            loadPreview()
        }
        .onChange(of: assetPath) { _ in
            loadPreview()
        }
        .onChange(of: selectedVariant) { _ in
            loadPreview()
        }
    }
    
    private func loadAvailableAssets() {
        // Mock data - in real implementation, this would scan the asset directory
        availableAssets = [
            AssetInfo(
                name: "Wood Material",
                path: "assets/materials/wood.mat",
                type: .material,
                description: "Wood material with PBR properties",
                variants: [
                    AssetVariant(name: "Oak", path: "assets/materials/wood_oak.mat", description: "Oak wood variant", thumbnailPath: "assets/thumbnails/wood_oak.png"),
                    AssetVariant(name: "Pine", path: "assets/materials/wood_pine.mat", description: "Pine wood variant", thumbnailPath: "assets/thumbnails/wood_pine.png"),
                    AssetVariant(name: "Dark", path: "assets/materials/wood_dark.mat", description: "Dark wood variant", thumbnailPath: "assets/thumbnails/wood_dark.png")
                ],
                thumbnailPath: "assets/thumbnails/wood.png",
                fileSize: 2048,
                lastModified: Date(),
                tags: ["wood", "material", "pbr"]
            ),
            AssetInfo(
                name: "Stone Texture",
                path: "assets/textures/stone.png",
                type: .texture,
                description: "Stone texture for surfaces",
                variants: [
                    AssetVariant(name: "Smooth", path: "assets/textures/stone_smooth.png", description: "Smooth stone variant", thumbnailPath: "assets/thumbnails/stone_smooth.png"),
                    AssetVariant(name: "Rough", path: "assets/textures/stone_rough.png", description: "Rough stone variant", thumbnailPath: "assets/thumbnails/stone_rough.png")
                ],
                thumbnailPath: "assets/thumbnails/stone.png",
                fileSize: 1024,
                lastModified: Date(),
                tags: ["stone", "texture", "surface"]
            ),
            AssetInfo(
                name: "Character Mesh",
                path: "assets/meshes/character.fbx",
                type: .mesh,
                description: "Character model with rigging",
                variants: [],
                thumbnailPath: "assets/thumbnails/character.png",
                fileSize: 4096,
                lastModified: Date(),
                tags: ["character", "mesh", "rigged"]
            )
        ]
    }
    
    private func loadPreview() {
        guard !assetPath.isEmpty else { return }
        
        isLoadingPreview = true
        
        // Mock preview loading - in real implementation, this would load the actual preview
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            isLoadingPreview = false
            // Set preview image based on asset type
            if assetType == .texture {
                previewImage = Image(systemName: "photo")
            } else if assetType == .mesh {
                previewImage = Image(systemName: "cube")
            } else if assetType == .material {
                previewImage = Image(systemName: "paintpalette")
            } else {
                previewImage = Image(systemName: assetType.icon)
            }
        }
    }
    
    @ViewBuilder
    private var contextMenuContent: some View {
        Group {
            Button("Copy Asset Path") {
                clipboardManager.copy(assetPath)
            }
            .keyboardShortcut("c", modifiers: .command)
            
            Button("Paste Asset Path") {
                pasteAssetPath()
            }
            .keyboardShortcut("v", modifiers: .command)
            
            if let defaultValue = defaultValue, assetPath != defaultValue {
                Button("Reset to Default") {
                    resetToDefault()
                }
                .keyboardShortcut("r", modifiers: .command)
            }
            
            Divider()
            
            Button("Expression Input") {
                expressionText = assetPath
                showExpressionDialog = true
            }
            .keyboardShortcut("e", modifiers: .command)
            
            if let onAnimationKeyframe = onAnimationKeyframe {
                Button("Set Keyframe") {
                    onAnimationKeyframe()
                }
                .keyboardShortcut("k", modifiers: .command)
            }
            
            Divider()
            
            Button(isLocked ? "Unlock" : "Lock") {
                onLockToggle?(!isLocked)
            }
        }
    }
    
    @ViewBuilder
    private var expressionDialog: some View {
        NavigationView {
            VStack(alignment: .leading, spacing: 16) {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Asset Path Expression")
                        .font(DesignSystem.Typography.headline)
                    
                    Text("Enter an expression that evaluates to a valid asset path:")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Supported asset types: \(AssetType.allCases.map(\.rawValue).joined(separator: ", "))")
                        .font(DesignSystem.Typography.caption.monospaced())
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundSecondary)
                        .cornerRadius(4)
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("Expression:")
                        .font(DesignSystem.Typography.small)
                    
                    TextEditor(text: $expressionText)
                        .font(DesignSystem.Typography.body.monospaced())
                        .frame(minHeight: 100)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundPrimary)
                        .cornerRadius(6)
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(DesignSystem.Colors.border, lineWidth: 1)
                        )
                }
                
                Spacer()
            }
            .padding()
            .navigationTitle("Expression Input")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        showExpressionDialog = false
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Apply") {
                        applyExpression()
                        showExpressionDialog = false
                    }
                    .disabled(expressionText.isEmpty)
                }
            }
        }
    }
    
    private func resetToDefault() {
        guard let defaultValue = defaultValue else { return }
        undoManager.recordState(assetPath)
        assetPath = defaultValue
        onReset?()
        onValueChanged?(defaultValue)
    }
    
    private func pasteAssetPath() {
        guard let pastedValue = clipboardManager.paste() else { return }
        undoManager.recordState(assetPath)
        assetPath = pastedValue
        onValueChanged?(pastedValue)
    }
    
    private func applyExpression() {
        // Simple expression evaluation - just use the text as the asset path
        let trimmedExpression = expressionText.trimmingCharacters(in: .whitespacesAndNewlines)
        if !trimmedExpression.isEmpty {
            undoManager.recordState(assetPath)
            assetPath = trimmedExpression
            onValueChanged?(trimmedExpression)
        }
    }
}

// MARK: - Asset Picker Sheet
struct AssetPickerSheet: View {
    @Binding var selectedAsset: String
    @Binding var selectedVariant: String
    @Binding var searchFilter: String
    @Binding var availableAssets: [AssetReferenceEditor.AssetInfo]
    let assetType: AssetReferenceEditor.AssetType
    @Binding var isPresented: Bool
    
    private var filteredAssets: [AssetReferenceEditor.AssetInfo] {
        if searchFilter.isEmpty {
            return availableAssets.filter { $0.type == assetType }
        } else {
            return availableAssets.filter { asset in
                asset.type == assetType && (
                    asset.name.localizedCaseInsensitiveContains(searchFilter) ||
                    asset.description.localizedCaseInsensitiveContains(searchFilter) ||
                    asset.tags.joined().localizedCaseInsensitiveContains(searchFilter)
                )
            }
        }
    }
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Search bar
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    TextField("Search \(assetType.rawValue.lowercased())s...", text: $searchFilter)
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.body)
                    
                    if !searchFilter.isEmpty {
                        Button(action: { searchFilter = "" }) {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(12)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Asset grid
                ScrollView {
                    LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 3), spacing: 16) {
                        ForEach(filteredAssets) { asset in
                            AssetGridItemView(
                                asset: asset,
                                isSelected: selectedAsset == asset.path,
                                onSelect: {
                                    selectedAsset = asset.path
                                    selectedVariant = "" // Reset variant when selecting new asset
                                    isPresented = false
                                }
                            )
                        }
                    }
                    .padding(16)
                }
            }
            .navigationTitle("Select \(assetType.rawValue)")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        isPresented = false
                    }
                }
            }
        }
        .frame(width: 600, height: 500)
    }
}

// MARK: - Asset Grid Item View
struct AssetGridItemView: View {
    let asset: AssetReferenceEditor.AssetInfo
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            VStack(spacing: 8) {
                // Thumbnail
                AsyncImage(url: URL(string: asset.thumbnailPath ?? "")) { image in
                    image
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                } placeholder: {
                    RoundedRectangle(cornerRadius: 6)
                        .fill(DesignSystem.Colors.backgroundTertiary)
                        .overlay(
                            Image(systemName: asset.type.icon)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                                .font(.title)
                        )
                }
                .frame(height: 80)
                .clipped()
                .cornerRadius(6)
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundTertiary, lineWidth: isSelected ? 2 : 1)
                )
                
                // Asset info
                VStack(alignment: .leading, spacing: 2) {
                    Text(asset.name)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .lineLimit(1)
                    
                    Text("\(asset.variants.count) variants")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .lineLimit(1)
                    
                    Text(formatFileSize(asset.fileSize))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                        .lineLimit(1)
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }
            .padding(8)
            .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(8)
        }
        .buttonStyle(.plain)
    }
    
    private func formatFileSize(_ size: Int64) -> String {
        let formatter = ByteCountFormatter()
        formatter.allowedUnits = [.useKB, .useMB]
        formatter.countStyle = .file
        return formatter.string(fromByteCount: size)
    }
}

// MARK: - Asset Creator Sheet
struct AssetCreatorSheet: View {
    let assetType: AssetReferenceEditor.AssetType
    @Binding var isPresented: Bool
    let onAssetCreated: (String) -> Void
    
    @State private var assetName = ""
    @State private var assetDescription = ""
    @State private var selectedTemplate = ""
    @State private var isCreating = false
    
    private var templates: [String] {
        switch assetType {
        case .material: return ["Basic PBR", "Unlit", "Transparent", "Emissive"]
        case .texture: return ["2D Texture", "Normal Map", "Roughness Map", "Metallic Map"]
        case .mesh: return ["Cube", "Sphere", "Plane", "Cylinder"]
        case .audio: return ["Sound Effect", "Music Track", "Ambient", "Voice"]
        case .animation: return ["Idle", "Walk", "Run", "Jump"]
        case .prefab: return ["Empty", "Character", "Prop", "Environment"]
        }
    }
    
    var body: some View {
        NavigationView {
            Form {
                Section("Basic Information") {
                    TextField("Asset Name", text: $assetName)
                        .textFieldStyle(.roundedBorder)
                    
                    TextField("Description", text: $assetDescription, axis: .vertical)
                        .textFieldStyle(.roundedBorder)
                        .lineLimit(3)
                }
                
                Section("Template") {
                    Picker("Template", selection: $selectedTemplate) {
                        Text("None").tag("")
                        ForEach(templates, id: \.self) { template in
                            Text(template).tag(template)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                if !selectedTemplate.isEmpty {
                    Section("Template Preview") {
                        HStack {
                            Spacer()
                            VStack {
                                Image(systemName: assetType.icon)
                                    .font(.largeTitle)
                                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                                
                                Text(selectedTemplate)
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                
                                Text("Template will be applied when creating the asset")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                    .multilineTextAlignment(.center)
                            }
                            Spacer()
                        }
                        .padding()
                    }
                }
            }
            .navigationTitle("Create \(assetType.rawValue)")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        isPresented = false
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Create") {
                        createAsset()
                    }
                    .disabled(assetName.isEmpty || isCreating)
                }
            }
        }
        .frame(width: 400, height: 450)
    }
    
    private func createAsset() {
        isCreating = true
        
        // Mock asset creation - in real implementation, this would create the actual asset
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            let newPath = "assets/\(assetType.rawValue.lowercased())s/\(assetName.lowercased().replacingOccurrences(of: " ", with: "_")).\(assetType.fileExtensions.first?.dropFirst() ?? "")"
            onAssetCreated(newPath)
            isCreating = false
            isPresented = false
        }
    }
}

// MARK: - Layer Reference Picker (TODO-31501)
struct LayerReferencePicker: View {
    let label: String
    @Binding var selectedLayer: String
    @State private var showPicker = false
    @State private var searchFilter = ""
    @State private var availableLayers: [LayerInfo] = []
    
    struct LayerInfo: Identifiable, Codable {
        let id = UUID()
        let name: String
        let index: Int
        let isVisible: Bool
        let isLocked: Bool
        let color: Color
        let description: String
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack {
                Button(action: { showPicker = true }) {
                    HStack {
                        Image(systemName: "square.stack.3d.up")
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                        
                        Text(selectedLayer.isEmpty ? "Select Layer" : selectedLayer)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(selectedLayer.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        Image(systemName: "chevron.down")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
                .buttonStyle(.plain)
                
                if !selectedLayer.isEmpty {
                    Button(action: { selectedLayer = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
        }
        .sheet(isPresented: $showPicker) {
            LayerPickerSheet(
                selectedLayer: $selectedLayer,
                searchFilter: $searchFilter,
                availableLayers: $availableLayers,
                isPresented: $showPicker
            )
        }
        .onAppear {
            loadAvailableLayers()
        }
    }
    
    private func loadAvailableLayers() {
        // Mock data - in real implementation, this would query the engine
        availableLayers = [
            LayerInfo(name: "Default", index: 0, isVisible: true, isLocked: false, color: .gray, description: "Default rendering layer"),
            LayerInfo(name: "UI", index: 1, isVisible: true, isLocked: false, color: .blue, description: "User interface elements"),
            LayerInfo(name: "Player", index: 2, isVisible: true, isLocked: false, color: .green, description: "Player character and related objects"),
            LayerInfo(name: "Enemies", index: 3, isVisible: true, isLocked: false, color: .red, description: "Enemy entities"),
            LayerInfo(name: "Environment", index: 4, isVisible: true, isLocked: false, color: .brown, description: "Terrain and static objects"),
            LayerInfo(name: "Effects", index: 5, isVisible: true, isLocked: false, color: .purple, description: "Particle effects and visual effects"),
            LayerInfo(name: "Physics", index: 6, isVisible: false, isLocked: true, color: .orange, description: "Physics debug visualization"),
            LayerInfo(name: "Navigation", index: 7, isVisible: false, isLocked: true, color: .cyan, description: "Navigation mesh and pathfinding")
        ]
    }
}

// MARK: - Layer Picker Sheet
struct LayerPickerSheet: View {
    @Binding var selectedLayer: String
    @Binding var searchFilter: String
    @Binding var availableLayers: [LayerReferencePicker.LayerInfo]
    @Binding var isPresented: Bool
    
    private var filteredLayers: [LayerReferencePicker.LayerInfo] {
        if searchFilter.isEmpty {
            return availableLayers
        } else {
            return availableLayers.filter { layer in
                layer.name.localizedCaseInsensitiveContains(searchFilter) ||
                layer.description.localizedCaseInsensitiveContains(searchFilter)
            }
        }
    }
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Search bar
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    TextField("Search layers...", text: $searchFilter)
                        .textFieldStyle(.plain)
                        .font(DesignSystem.Typography.body)
                    
                    if !searchFilter.isEmpty {
                        Button(action: { searchFilter = "" }) {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(12)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Layer list
                List(filteredLayers, id: \.id) { layer in
                    LayerRowView(
                        layer: layer,
                        isSelected: selectedLayer == layer.name,
                        onSelect: { 
                            selectedLayer = layer.name
                            isPresented = false
                        }
                    )
                }
                .listStyle(.plain)
            }
            .navigationTitle("Select Layer")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        isPresented = false
                    }
                }
            }
        }
        .frame(width: 400, height: 500)
    }
}

// MARK: - Layer Row View
struct LayerRowView: View {
    let layer: LayerReferencePicker.LayerInfo
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            HStack(spacing: 12) {
                // Layer color indicator
                RoundedRectangle(cornerRadius: 3)
                    .fill(layer.color)
                    .frame(width: 16, height: 16)
                
                // Layer info
                VStack(alignment: .leading, spacing: 2) {
                    HStack {
                        Text(layer.name)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        // Layer index
                        Text("\(layer.index)")
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.caption)
                    }
                    
                    Text(layer.description)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .lineLimit(1)
                }
                
                Spacer()
                
                // Status indicators
                HStack(spacing: 8) {
                    if layer.isVisible {
                        Image(systemName: "eye")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.caption)
                    } else {
                        Image(systemName: "eye.slash")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.caption)
                    }
                    
                    if layer.isLocked {
                        Image(systemName: "lock")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.caption)
                    }
                    
                    if isSelected {
                        Image(systemName: "checkmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                    }
                }
            }
            .padding(.vertical, 4)
        }
        .buttonStyle(.plain)
        .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : Color.clear)
        .cornerRadius(4)
    }
}

// MARK: - Script Reference Picker (TODO-31500)
struct ScriptReferencePicker: View {
    let label: String
    @Binding var selectedScript: String
    @State private var showPicker = false
    @State private var searchFilter = ""
    @State private var availableScripts: [ScriptInfo] = []
    @State private var validationErrors: [String] = []
    
    struct ScriptInfo: Identifiable, Codable {
        let id = UUID()
        let name: String
        let path: String
        let language: ScriptLanguage
        let description: String
        let isValid: Bool
        let lastModified: Date
        let functionCount: Int
        let lineCount: Int
    }
    
    enum ScriptLanguage: String, CaseIterable, Codable {
        case lua = "Lua"
        case python = "Python"
        case javascript = "JavaScript"
        case csharp = "C#"
        case cpp = "C++"
        
        var icon: String {
            switch self {
            case .lua: return "doc.text"
            case .python: return "snake"
            case .javascript: return "curlybraces"
            case .csharp: return "c.circle"
            case .cpp: return "c.circle.fill"
            }
        }
        
        var fileExtension: String {
            switch self {
            case .lua: return ".lua"
            case .python: return ".py"
            case .javascript: return ".js"
            case .csharp: return ".cs"
            case .cpp: return ".cpp"
            }
        }
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack {
                Button(action: { showPicker = true }) {
                    HStack {
                        Image(systemName: "doc.text")
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text(selectedScript.isEmpty ? "Select Script" : URL(fileURLWithPath: selectedScript).lastPathComponent)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(selectedScript.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                            
                            if !selectedScript.isEmpty && !validationErrors.isEmpty {
                                Text("⚠️ \(validationErrors.count) issues")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.warning)
                            }
                        }
                        
                        Spacer()
                        
                        Image(systemName: "chevron.down")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
                .buttonStyle(.plain)
                
                if !selectedScript.isEmpty {
                    Button(action: { selectedScript = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
        }
        .sheet(isPresented: $showPicker) {
            ScriptPickerSheet(
                selectedScript: $selectedScript,
                searchFilter: $searchFilter,
                availableScripts: $availableScripts,
                validationErrors: $validationErrors,
                isPresented: $showPicker
            )
        }
        .onAppear {
            loadAvailableScripts()
            if !selectedScript.isEmpty {
                validateScript(at: selectedScript)
            }
        }
        .onChange(of: selectedScript) { newValue in
            if !newValue.isEmpty {
                validateScript(at: newValue)
            } else {
                validationErrors = []
            }
        }
    }
    
    private func loadAvailableScripts() {
        // Mock data - in real implementation, this would scan the scripts directory
        availableScripts = [
            ScriptInfo(name: "PlayerController", path: "scripts/player/PlayerController.lua", language: .lua, description: "Main player movement and input handling", isValid: true, lastModified: Date(), functionCount: 12, lineCount: 245),
            ScriptInfo(name: "EnemyAI", path: "scripts/ai/EnemyAI.py", language: .python, description: "Enemy behavior and decision making", isValid: true, lastModified: Date(), functionCount: 8, lineCount: 189),
            ScriptInfo(name: "GameRules", path: "scripts/core/GameRules.js", language: .javascript, description: "Game state and rule management", isValid: true, lastModified: Date(), functionCount: 15, lineCount: 312),
            ScriptInfo(name: "PhysicsHelper", path: "scripts/utils/PhysicsHelper.cpp", language: .cpp, description: "Physics calculations and utilities", isValid: false, lastModified: Date(), functionCount: 6, lineCount: 98),
            ScriptInfo(name: "UIManager", path: "scripts/ui/UIManager.cs", language: .csharp, description: "User interface management", isValid: true, lastModified: Date(), functionCount: 20, lineCount: 456),
            ScriptInfo(name: "AudioSystem", path: "scripts/audio/AudioSystem.lua", language: .lua, description: "Audio playback and management", isValid: true, lastModified: Date(), functionCount: 10, lineCount: 167)
        ]
    }
    
    private func validateScript(at path: String) {
        validationErrors = []
        
        // Mock validation - in real implementation, this would parse the script
        if path.contains("PhysicsHelper") {
            validationErrors.append("Missing function: calculateCollisionResponse")
            validationErrors.append("Undefined variable: gravityConstant")
        }
        
        if path.contains("EnemyAI") {
            validationErrors.append("Warning: Unused variable 'tempTarget'")
        }
    }
}

// MARK: - Script Picker Sheet
struct ScriptPickerSheet: View {
    @Binding var selectedScript: String
    @Binding var searchFilter: String
    @Binding var availableScripts: [ScriptReferencePicker.ScriptInfo]
    @Binding var validationErrors: [String]
    @Binding var isPresented: Bool
    
    @State private var selectedLanguage: ScriptReferencePicker.ScriptLanguage? = nil
    @State private var showOnlyValid = false
    
    private var filteredScripts: [ScriptReferencePicker.ScriptInfo] {
        var scripts = availableScripts
        
        // Filter by search
        if !searchFilter.isEmpty {
            scripts = scripts.filter { script in
                script.name.localizedCaseInsensitiveContains(searchFilter) ||
                script.description.localizedCaseInsensitiveContains(searchFilter) ||
                script.path.localizedCaseInsensitiveContains(searchFilter)
            }
        }
        
        // Filter by language
        if let language = selectedLanguage {
            scripts = scripts.filter { $0.language == language }
        }
        
        // Filter by validation status
        if showOnlyValid {
            scripts = scripts.filter { $0.isValid }
        }
        
        return scripts.sorted { $0.name < $1.name }
    }
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Search and filters
                VStack(spacing: 8) {
                    // Search bar
                    HStack {
                        Image(systemName: "magnifyingglass")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        TextField("Search scripts...", text: $searchFilter)
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.body)
                        
                        if !searchFilter.isEmpty {
                            Button(action: { searchFilter = "" }) {
                                Image(systemName: "xmark.circle.fill")
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                    .padding(12)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(4)
                    
                    // Filter controls
                    HStack {
                        // Language filter
                        Menu {
                            Button("All Languages") { selectedLanguage = nil }
                            Divider()
                            ForEach(ScriptReferencePicker.ScriptLanguage.allCases, id: \.self) { language in
                                Button(action: { selectedLanguage = language }) {
                                    HStack {
                                        Image(systemName: language.icon)
                                        Text(language.rawValue)
                                        if selectedLanguage == language {
                                            Spacer()
                                            Image(systemName: "checkmark")
                                        }
                                    }
                                }
                            }
                        } label: {
                            HStack {
                                Image(systemName: selectedLanguage?.icon ?? "doc.text")
                                Text(selectedLanguage?.rawValue ?? "All Languages")
                                Spacer()
                                Image(systemName: "chevron.down")
                            }
                            .font(DesignSystem.Typography.small)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        }
                        .buttonStyle(.plain)
                        
                        Spacer()
                        
                        // Valid only toggle
                        Toggle("Valid Only", isOn: $showOnlyValid)
                            .font(DesignSystem.Typography.small)
                            .toggleStyle(.switch)
                    }
                }
                .padding(12)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Script list
                List(filteredScripts, id: \.id) { script in
                    ScriptRowView(
                        script: script,
                        isSelected: selectedScript == script.path,
                        onSelect: { 
                            selectedScript = script.path
                            isPresented = false
                        }
                    )
                }
                .listStyle(.plain)
            }
            .navigationTitle("Select Script")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        isPresented = false
                    }
                }
            }
        }
        .frame(width: 500, height: 600)
    }
}

// MARK: - Script Row View
struct ScriptRowView: View {
    let script: ScriptReferencePicker.ScriptInfo
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            HStack(spacing: 12) {
                // Language icon
                Image(systemName: script.language.icon)
                    .foregroundColor(script.isValid ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.warning)
                    .font(.title2)
                
                // Script info
                VStack(alignment: .leading, spacing: 2) {
                    HStack {
                        Text(script.name)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        // Validation status
                        if !script.isValid {
                            Image(systemName: "exclamationmark.triangle.fill")
                                .foregroundColor(DesignSystem.Colors.warning)
                                .help("Script has validation errors")
                        } else {
                            Image(systemName: "checkmark.circle.fill")
                                .foregroundColor(DesignSystem.Colors.success)
                                .help("Script is valid")
                        }
                    }
                    
                    Text(script.description)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .lineLimit(2)
                    
                    HStack {
                        Text(script.language.rawValue)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 2)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(3)
                        
                        Text("\(script.functionCount) functions")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("\(script.lineCount) lines")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Spacer()
                        
                        Text(script.path)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.caption)
                            .lineLimit(1)
                    }
                }
                
                if isSelected {
                    Image(systemName: "checkmark.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            .padding(.vertical, 4)
        }
        .buttonStyle(.plain)
        .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : Color.clear)
        .cornerRadius(4)
    }
}

// MARK: - Component Reference Picker (TODO-31499)
struct ComponentReferencePicker: View {
    let label: String
    @Binding var selectedComponent: String
    @State private var showPicker = false
    @State private var searchFilter = ""
    @State private var availableComponents: [ComponentInfo] = []
    @State private var selectedCategory: ComponentCategory? = nil
    @State private var showOnlyAttached = false
    
    struct ComponentInfo: Identifiable, Codable {
        let id = UUID()
        let name: String
        let type: String
        let category: ComponentCategory
        let description: String
        let isAttached: Bool
        let isRequired: Bool
        let icon: String
        let properties: [String]
    }
    
    enum ComponentCategory: String, CaseIterable, Codable {
        case rendering = "Rendering"
        case physics = "Physics"
        case audio = "Audio"
        case animation = "Animation"
        case ui = "UI"
        case networking = "Networking"
        case gameplay = "Gameplay"
        case utility = "Utility"
        
        var icon: String {
            switch self {
            case .rendering: return "paintbrush"
            case .physics: return "circle.grid.3x3"
            case .audio: return "speaker.wave.2"
            case .animation: return "figure.walk"
            case .ui: return "rectangle.stack"
            case .networking: return "network"
            case .gameplay: return "gamecontroller"
            case .utility: return "wrench.and.screwdriver"
            }
        }
        
        var color: Color {
            switch self {
            case .rendering: return .purple
            case .physics: return .blue
            case .audio: return .orange
            case .animation: return .green
            case .ui: return .pink
            case .networking: return .cyan
            case .gameplay: return .red
            case .utility: return .gray
            }
        }
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack {
                Button(action: { showPicker = true }) {
                    HStack {
                        Image(systemName: "puzzlepiece.extension")
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text(selectedComponent.isEmpty ? "Select Component" : selectedComponent)
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(selectedComponent.isEmpty ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textPrimary)
                            
                            if let component = availableComponents.first(where: { $0.name == selectedComponent }) {
                                HStack(spacing: 4) {
                                    Image(systemName: component.category.icon)
                                        .foregroundColor(component.category.color)
                                        .font(.caption)
                                    
                                    Text(component.category.rawValue)
                                        .font(DesignSystem.Typography.small)
                                        .foregroundColor(DesignSystem.Colors.textTertiary)
                                    
                                    if component.isRequired {
                                        Text("Required")
                                            .font(DesignSystem.Typography.small)
                                            .foregroundColor(DesignSystem.Colors.warning)
                                    }
                                }
                            }
                        }
                        
                        Spacer()
                        
                        Image(systemName: "chevron.down")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(4)
                }
                .buttonStyle(.plain)
                
                if !selectedComponent.isEmpty {
                    Button(action: { selectedComponent = "" }) {
                        Image(systemName: "xmark.circle.fill")
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
        }
        .sheet(isPresented: $showPicker) {
            ComponentPickerSheet(
                selectedComponent: $selectedComponent,
                searchFilter: $searchFilter,
                availableComponents: $availableComponents,
                selectedCategory: $selectedCategory,
                showOnlyAttached: $showOnlyAttached,
                isPresented: $showPicker
            )
        }
        .onAppear {
            loadAvailableComponents()
        }
    }
    
    private func loadAvailableComponents() {
        // Mock data - in real implementation, this would query the component registry
        availableComponents = [
            ComponentInfo(name: "MeshRenderer", type: "RenderingComponent", category: .rendering, description: "Renders 3D meshes with materials", isAttached: true, isRequired: false, icon: "cube", properties: ["mesh", "material", "castShadows", "receiveShadows"]),
            ComponentInfo(name: "Rigidbody", type: "PhysicsComponent", category: .physics, description: "Adds physics simulation to object", isAttached: false, isRequired: false, icon: "circle.grid.3x3", properties: ["mass", "drag", "angularDrag", "useGravity"]),
            ComponentInfo(name: "BoxCollider", type: "PhysicsComponent", category: .physics, description: "Box-shaped collision detection", isAttached: true, isRequired: false, icon: "square", properties: ["size", "center", "isTrigger"]),
            ComponentInfo(name: "AudioSource", type: "AudioComponent", category: .audio, description: "Plays audio clips at object position", isAttached: false, isRequired: false, icon: "speaker.wave.2", properties: ["clip", "volume", "pitch", "spatialBlend"]),
            ComponentInfo(name: "Animator", type: "AnimationComponent", category: .animation, description: "Controls animation playback", isAttached: true, isRequired: false, icon: "figure.walk", properties: ["controller", "avatar", "applyRootMotion"]),
            ComponentInfo(name: "Canvas", type: "UIComponent", category: .ui, description: "Renders UI elements", isAttached: false, isRequired: false, icon: "rectangle.stack", properties: ["renderMode", "pixelPerfect", "sortingOrder"]),
            ComponentInfo(name: "NetworkTransform", type: "NetworkComponent", category: .networking, description: "Synchronizes transform over network", isAttached: false, isRequired: false, icon: "network", properties: ["syncPosition", "syncRotation", "syncScale"]),
            ComponentInfo(name: "Health", type: "GameplayComponent", category: .gameplay, description: "Manages entity health", isAttached: true, isRequired: true, icon: "heart.fill", properties: ["maxHealth", "currentHealth", "regenerationRate"]),
            ComponentInfo(name: "Transform", type: "UtilityComponent", category: .utility, description: "Position, rotation, and scale", isAttached: true, isRequired: true, icon: "arrow.up.and.down.text.horizontal", properties: ["position", "rotation", "scale"]),
            ComponentInfo(name: "Camera", type: "RenderingComponent", category: .rendering, description: "Camera for rendering scene", isAttached: false, isRequired: false, icon: "camera", properties: ["fieldOfView", "nearClipPlane", "farClipPlane", "backgroundColor"]),
            ComponentInfo(name: "Light", type: "RenderingComponent", category: .rendering, description: "Illuminates the scene", isAttached: true, isRequired: false, icon: "sun.max", properties: ["type", "intensity", "color", "range"]),
            ComponentInfo(name: "ParticleSystem", type: "RenderingComponent", category: .rendering, description: "Particle effects", isAttached: false, isRequired: false, icon: "sparkles", properties: ["particleSystem", "playOnAwake"])
        ]
    }
}

// MARK: - Component Picker Sheet
struct ComponentPickerSheet: View {
    @Binding var selectedComponent: String
    @Binding var searchFilter: String
    @Binding var availableComponents: [ComponentReferencePicker.ComponentInfo]
    @Binding var selectedCategory: ComponentReferencePicker.ComponentCategory?
    @Binding var showOnlyAttached: Bool
    @Binding var isPresented: Bool
    
    private var filteredComponents: [ComponentReferencePicker.ComponentInfo] {
        var components = availableComponents
        
        // Filter by search
        if !searchFilter.isEmpty {
            components = components.filter { component in
                component.name.localizedCaseInsensitiveContains(searchFilter) ||
                component.type.localizedCaseInsensitiveContains(searchFilter) ||
                component.description.localizedCaseInsensitiveContains(searchFilter) ||
                component.properties.joined().localizedCaseInsensitiveContains(searchFilter)
            }
        }
        
        // Filter by category
        if let category = selectedCategory {
            components = components.filter { $0.category == category }
        }
        
        // Filter by attachment status
        if showOnlyAttached {
            components = components.filter { $0.isAttached }
        }
        
        // Sort: required components first, then alphabetically
        return components.sorted { lhs, rhs in
            if lhs.isRequired != rhs.isRequired {
                return lhs.isRequired && !rhs.isRequired
            }
            return lhs.name < rhs.name
        }
    }
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Search and filters
                VStack(spacing: 8) {
                    // Search bar
                    HStack {
                        Image(systemName: "magnifyingglass")
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        TextField("Search components...", text: $searchFilter)
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.body)
                        
                        if !searchFilter.isEmpty {
                            Button(action: { searchFilter = "" }) {
                                Image(systemName: "xmark.circle.fill")
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                    .padding(12)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(4)
                    
                    // Filter controls
                    HStack {
                        // Category filter
                        Menu {
                            Button("All Categories") { selectedCategory = nil }
                            Divider()
                            ForEach(ComponentReferencePicker.ComponentCategory.allCases, id: \.self) { category in
                                Button(action: { selectedCategory = category }) {
                                    HStack {
                                        Image(systemName: category.icon)
                                            .foregroundColor(category.color)
                                        Text(category.rawValue)
                                        if selectedCategory == category {
                                            Spacer()
                                            Image(systemName: "checkmark")
                                        }
                                    }
                                }
                            }
                        } label: {
                            HStack {
                                if let category = selectedCategory {
                                    Image(systemName: category.icon)
                                        .foregroundColor(category.color)
                                    Text(category.rawValue)
                                } else {
                                    Image(systemName: "puzzlepiece.extension")
                                    Text("All Categories")
                                }
                                Spacer()
                                Image(systemName: "chevron.down")
                            }
                            .font(DesignSystem.Typography.small)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                        }
                        .buttonStyle(.plain)
                        
                        Spacer()
                        
                        // Attached only toggle
                        Toggle("Attached Only", isOn: $showOnlyAttached)
                            .font(DesignSystem.Typography.small)
                            .toggleStyle(.switch)
                    }
                }
                .padding(12)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Component list
                List(filteredComponents, id: \.id) { component in
                    ComponentRowView(
                        component: component,
                        isSelected: selectedComponent == component.name,
                        onSelect: { 
                            selectedComponent = component.name
                            isPresented = false
                        }
                    )
                }
                .listStyle(.plain)
            }
            .navigationTitle("Select Component")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        isPresented = false
                    }
                }
            }
        }
        .frame(width: 550, height: 650)
    }
}

// MARK: - Component Row View
struct ComponentRowView: View {
    let component: ComponentReferencePicker.ComponentInfo
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Button(action: onSelect) {
            HStack(spacing: 12) {
                // Component icon
                Image(systemName: component.icon)
                    .foregroundColor(component.category.color)
                    .font(.title2)
                
                // Component info
                VStack(alignment: .leading, spacing: 2) {
                    HStack {
                        Text(component.name)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Spacer()
                        
                        // Status indicators
                        HStack(spacing: 6) {
                            if component.isRequired {
                                Text("Required")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(.white)
                                    .padding(.horizontal, 6)
                                    .padding(.vertical, 2)
                                    .background(DesignSystem.Colors.warning)
                                    .cornerRadius(3)
                            }
                            
                            if component.isAttached {
                                Image(systemName: "link")
                                    .foregroundColor(DesignSystem.Colors.success)
                                    .help("Component is attached")
                            }
                        }
                    }
                    
                    Text(component.description)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .lineLimit(2)
                    
                    HStack {
                        // Category
                        HStack(spacing: 4) {
                            Image(systemName: component.category.icon)
                                .foregroundColor(component.category.color)
                                .font(.caption)
                            
                            Text(component.category.rawValue)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                        
                        // Type
                        Text(component.type)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                            .font(.caption)
                        
                        Spacer()
                        
                        // Property count
                        Text("\(component.properties.count) properties")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                }
                
                if isSelected {
                    Image(systemName: "checkmark.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            .padding(.vertical, 4)
        }
        .buttonStyle(.plain)
        .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : Color.clear)
        .cornerRadius(4)
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

// MARK: - Range Editor (min-max pair) (TODO-1755)
struct RangePropertyEditor: View {
    let label: String
    @Binding var range: ClosedRange<Float>
    var minLimit: Float = -Float.infinity
    var maxLimit: Float = Float.infinity
    var step: Float = 0.1
    
    @State private var minValue: Float
    @State private var maxValue: Float
    
    init(label: String, range: Binding<ClosedRange<Float>>, minLimit: Float = -Float.infinity, maxLimit: Float = Float.infinity, step: Float = 0.1) {
        self.label = label
        self._range = range
        self.minLimit = minLimit
        self.maxLimit = maxLimit
        self.step = step
        self._minValue = State(initialValue: range.wrappedValue.lowerBound)
        self._maxValue = State(initialValue: range.wrappedValue.upperBound)
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(label)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            HStack(spacing: 8) {
                // Min value
                VStack(alignment: .leading, spacing: 2) {
                    Text("Min")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack(spacing: 4) {
                        TextField("", value: $minValue, format: .number.precision(.fractionLength(2)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 80)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                            .onChange(of: minValue) { newValue in
                                let clampedValue = max(minLimit, min(newValue, maxValue - step))
                                minValue = clampedValue
                                updateRange()
                            }
                        
                        Stepper("", value: $minValue, in: minLimit...maxValue, step: step)
                            .frame(width: 20)
                    }
                }
                
                // Max value
                VStack(alignment: .leading, spacing: 2) {
                    Text("Max")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    HStack(spacing: 4) {
                        TextField("", value: $maxValue, format: .number.precision(.fractionLength(2)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 80)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                            .onChange(of: maxValue) { newValue in
                                let clampedValue = min(maxLimit, max(newValue, minValue + step))
                                maxValue = clampedValue
                                updateRange()
                            }
                        
                        Stepper("", value: $maxValue, in: minLimit...maxLimit, step: step)
                            .frame(width: 20)
                    }
                }
                
                Spacer()
                
                // Range info
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Span: \(String(format: "%.2f", maxValue - minValue))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text("Center: \(String(format: "%.2f", (maxValue + minValue) / 2))")
                        .font(DesignSystem.Typography.mono)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
            }
            
            // Visual range slider
            VStack(alignment: .leading, spacing: 2) {
                Text("Range Visual")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                GeometryReader { geometry in
                    ZStack(alignment: .leading) {
                        // Background track
                        Rectangle()
                            .fill(DesignSystem.Colors.border)
                            .frame(height: 4)
                            .cornerRadius(2)
                        
                        // Range fill
                        Rectangle()
                            .fill(DesignSystem.Colors.accentPrimary)
                            .frame(
                                width: max(0, CGFloat((maxValue - minValue) / (maxLimit - minLimit)) * geometry.size.width),
                                height: 4
                            )
                            .cornerRadius(2)
                            .offset(x: CGFloat((minValue - minLimit) / (maxLimit - minLimit)) * geometry.size.width)
                    }
                }
                .frame(height: 20)
            }
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(4)
        }
    }
    
    private func updateRange() {
        range = minValue...maxValue
    }
}

// MARK: - Rect/RectTransform Editor (TODO-1756)
struct RectTransformPropertyEditor: View {
    let label: String
    @Binding var rect: CGRect
    @Binding var anchor: CGPoint
    @Binding var pivot: CGPoint
    @State private var showAdvanced = false
    
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
            
            // Position and Size
            VStack(spacing: 8) {
                // Position (X, Y)
                HStack(spacing: 8) {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Position X")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("", value: Binding(
                            get: { Float(rect.origin.x) },
                            set: { rect.origin.x = CGFloat($0) }
                        ), format: .number.precision(.fractionLength(1)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 80)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                    }
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Position Y")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("", value: Binding(
                            get: { Float(rect.origin.y) },
                            set: { rect.origin.y = CGFloat($0) }
                        ), format: .number.precision(.fractionLength(1)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 80)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                    }
                    
                    Spacer()
                }
                
                // Size (Width, Height)
                HStack(spacing: 8) {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Width")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("", value: Binding(
                            get: { Float(rect.size.width) },
                            set: { rect.size.width = CGFloat($0) }
                        ), format: .number.precision(.fractionLength(1)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 80)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                    }
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Height")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("", value: Binding(
                            get: { Float(rect.size.height) },
                            set: { rect.size.height = CGFloat($0) }
                        ), format: .number.precision(.fractionLength(1)))
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.mono)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .frame(width: 80)
                            .padding(.horizontal, 6)
                            .padding(.vertical, 4)
                            .background(DesignSystem.Colors.backgroundTertiary)
                            .cornerRadius(4)
                    }
                    
                    Spacer()
                }
            }
            
            if showAdvanced {
                VStack(spacing: 8) {
                    // Anchor
                    HStack(spacing: 8) {
                        VStack(alignment: .leading, spacing: 2) {
                            Text("Anchor X")
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            TextField("", value: Binding(
                                get: { Float(anchor.x) },
                                set: { anchor.x = CGFloat($0) }
                            ), format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                                .frame(width: 80)
                                .padding(.horizontal, 6)
                                .padding(.vertical, 4)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                        }
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text("Anchor Y")
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            TextField("", value: Binding(
                                get: { Float(anchor.y) },
                                set: { anchor.y = CGFloat($0) }
                            ), format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                                .frame(width: 80)
                                .padding(.horizontal, 6)
                                .padding(.vertical, 4)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                        }
                        
                        Spacer()
                    }
                    
                    // Pivot
                    HStack(spacing: 8) {
                        VStack(alignment: .leading, spacing: 2) {
                            Text("Pivot X")
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            TextField("", value: Binding(
                                get: { Float(pivot.x) },
                                set: { pivot.x = CGFloat($0) }
                            ), format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                                .frame(width: 80)
                                .padding(.horizontal, 6)
                                .padding(.vertical, 4)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                        }
                        
                        VStack(alignment: .leading, spacing: 2) {
                            Text("Pivot Y")
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            TextField("", value: Binding(
                                get: { Float(pivot.y) },
                                set: { pivot.y = CGFloat($0) }
                            ), format: .number.precision(.fractionLength(2)))
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .foregroundColor(DesignSystem.Colors.textPrimary)
                                .frame(width: 80)
                                .padding(.horizontal, 6)
                                .padding(.vertical, 4)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                        }
                        
                        Spacer()
                    }
                }
                .padding(.top, 8)
                .padding(.horizontal, 8)
                .padding(.vertical, 8)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(4)
            }
            
            // Rect info
            HStack {
                Text("Area: \(String(format: "%.1f", rect.size.width * rect.size.height))")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Spacer()
                
                Text("Center: (\(String(format: "%.1f", rect.midX)), \(String(format: "%.1f", rect.midY)))")
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

// MARK: - Property Scripting Hooks (TODO-1481)
struct PropertyScriptingHooks: View {
    let propertyName: String
    @Binding var value: Any
    @State private var scripts: [PropertyScript] = []
    @State private var showScriptEditor = false
    @State private var executionResults: [ScriptExecutionResult] = []
    
    struct PropertyScript: Identifiable, Codable {
        let id = UUID()
        var name: String
        var script: String
        var trigger: ScriptTrigger
        var enabled: Bool
        
        enum ScriptTrigger: String, CaseIterable, Codable {
            case onValueChange = "On Value Change"
            case onValidate = "On Validate"
            case onDisplay = "On Display"
            case onEdit = "On Edit"
        }
    }
    
    struct ScriptExecutionResult: Identifiable {
        let id = UUID()
        let scriptName: String
        let result: String
        let success: Bool
        let timestamp: Date
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("Scripting Hooks")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                Button(action: { showScriptEditor = true }) {
                    Image(systemName: "plus.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
            }
            
            // Scripts list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach($scripts) { $script in
                        HStack {
                            Toggle("", isOn: $script.enabled)
                                .toggleStyle(.switch)
                            
                            VStack(alignment: .leading, spacing: 2) {
                                Text(script.name)
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                
                                Text(script.trigger.rawValue)
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textTertiary)
                            }
                            
                            Spacer()
                            
                            Button(action: { executeScript(script) }) {
                                Image(systemName: "play.circle")
                                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                            }
                            .buttonStyle(.plain)
                            
                            Button(action: { removeScript(script) }) {
                                Image(systemName: "trash")
                                    .foregroundColor(DesignSystem.Colors.error)
                            }
                            .buttonStyle(.plain)
                        }
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(script.enabled ? DesignSystem.Colors.backgroundPrimary : DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                }
            }
            .frame(height: 150)
            
            // Execution results
            if !executionResults.isEmpty {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Execution Results")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    ScrollView {
                        LazyVStack(spacing: 2) {
                            ForEach(executionResults) { result in
                                HStack {
                                    Circle()
                                        .fill(result.success ? DesignSystem.Colors.success : DesignSystem.Colors.error)
                                        .frame(width: 8, height: 8)
                                    
                                    Text(result.scriptName)
                                        .font(DesignSystem.Typography.caption)
                                        .foregroundColor(DesignSystem.Colors.textPrimary)
                                    
                                    Spacer()
                                    
                                    Text(result.timestamp, style: .time)
                                        .font(DesignSystem.Typography.caption)
                                        .foregroundColor(DesignSystem.Colors.textTertiary)
                                }
                                .padding(.horizontal, 8)
                                .padding(.vertical, 2)
                                .background(DesignSystem.Colors.backgroundSecondary)
                                .cornerRadius(4)
                            }
                        }
                    }
                    .frame(height: 100)
                }
            }
        }
        .sheet(isPresented: $showScriptEditor) {
            ScriptEditorView(script: PropertyScript(name: "", script: "", trigger: .onValueChange, enabled: true)) { newScript in
                scripts.append(newScript)
            }
        }
    }
    
    private func executeScript(_ script: PropertyScript) {
        // Script execution logic would go here
        let result = ScriptExecutionResult(
            scriptName: script.name,
            result: "Script executed successfully",
            success: true,
            timestamp: Date()
        )
        executionResults.append(result)
        
        // Keep only last 10 results
        if executionResults.count > 10 {
            executionResults.removeFirst()
        }
    }
    
    private func removeScript(_ script: PropertyScript) {
        scripts.removeAll { $0.id == script.id }
    }
}

// Script Editor View
struct ScriptEditorView: View {
    @Binding var script: PropertyScriptingHooks.PropertyScript
    let onSave: (PropertyScriptingHooks.PropertyScript) -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        NavigationView {
            Form {
                Section("Script Info") {
                    TextField("Script Name", text: $script.name)
                    
                    Picker("Trigger", selection: $script.trigger) {
                        ForEach(PropertyScriptingHooks.PropertyScript.ScriptTrigger.allCases, id: \.self) { trigger in
                            Text(trigger.rawValue).tag(trigger)
                        }
                    }
                    
                    Toggle("Enabled", isOn: $script.enabled)
                }
                
                Section("Script Code") {
                    TextEditor(text: $script.script)
                        .frame(minHeight: 200)
                        .font(DesignSystem.Typography.mono)
                }
            }
            .navigationTitle("Edit Script")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        onSave(script)
                        dismiss()
                    }
                    .disabled(script.name.isEmpty || script.script.isEmpty)
                }
            }
        }
    }
}

// MARK: - Enhanced Curve Property Editor
struct CurvePropertyEditor: View, PropertyEditor {
    let label: String
    @Binding var curve: AnimationCurve
    
    // Configuration
    let config: PropertyEditorConfig<AnimationCurve>
    
    // Protocol conformance
    var defaultValue: AnimationCurve? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((AnimationCurve) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var showExpressionDialog = false
    @State private var expressionText = ""
    @State private var isHovering = false
    @State private var selectedPointIndex: Int?
    @State private var isDragging = false
    @State private var showCurvePresets = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            // Label with tooltip and override indicator
            HStack(spacing: 4) {
                Text(label)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
                
                if isOverridden {
                    Image(systemName: "arrow.triangle.2.circlepath")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            
            // Curve editor with controls
            VStack(spacing: 8) {
                // Curve visualization
                curveVisualizationView
                    .frame(height: 120)
                    .background(
                        RoundedRectangle(cornerRadius: 4)
                            .fill(isLocked ? DesignSystem.Colors.backgroundDisabled : DesignSystem.Colors.backgroundPrimary)
                            .stroke(isOverridden ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.border, lineWidth: isOverridden ? 2 : 1)
                    )
                    .contextMenu {
                        contextMenuContent
                    }
                
                // Control buttons
                controlButtons
            }
        }
        .sheet(isPresented: $showExpressionDialog) {
            expressionDialog
        }
        .sheet(isPresented: $showCurvePresets) {
            curvePresetsDialog
        }
        .onChange(of: curve) { oldValue, newValue in
            if !isLocked {
                undoManager.recordState(oldValue)
                onValueChanged?(newValue)
            }
        }
    }
    
    @ViewBuilder
    private var curveVisualizationView: some View {
        GeometryReader { geometry in
            let width = geometry.size.width
            let height = geometry.size.height
            
            ZStack {
                // Grid background
                Path { path in
                    let gridSize: CGFloat = 20
                    
                    // Vertical lines
                    for i in stride(from: 0, through: width, by: gridSize) {
                        path.move(to: CGPoint(x: i, y: 0))
                        path.addLine(to: CGPoint(x: i, y: height))
                    }
                    
                    // Horizontal lines
                    for i in stride(from: 0, through: height, by: gridSize) {
                        path.move(to: CGPoint(x: 0, y: i))
                        path.addLine(to: CGPoint(x: width, y: i))
                    }
                }
                .stroke(DesignSystem.Colors.border.opacity(0.3), lineWidth: 0.5)
                
                // Curve path
                Path { path in
                    guard !curve.points.isEmpty else { return }
                    
                    let sortedPoints = curve.points.sorted { $0.time < $1.time }
                    
                    for (index, point) in sortedPoints.enumerated() {
                        let x = point.time * width
                        let y = height - (point.value * height)
                        
                        if index == 0 {
                            path.move(to: CGPoint(x: x, y: y))
                        } else {
                            path.addLine(to: CGPoint(x: x, y: y))
                        }
                    }
                }
                .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                
                // Control points
                ForEach(Array(curve.points.enumerated()), id: \.offset) { index, point in
                    let x = point.time * width
                    let y = height - (point.value * height)
                    
                    Circle()
                        .fill(selectedPointIndex == index ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary)
                        .frame(width: 8, height: 8)
                        .position(x: x, y: y)
                        .gesture(
                            DragGesture()
                                .onChanged { value in
                                    if !isLocked {
                                        selectedPointIndex = index
                                        isDragging = true
                                        updatePoint(at: index, location: value.location, in: geometry.size)
                                    }
                                }
                                .onEnded { _ in
                                    isDragging = false
                                }
                        )
                }
            }
        }
        .disabled(isLocked)
        .opacity(isLocked ? 0.6 : 1.0)
    }
    
    @ViewBuilder
    private var controlButtons: some View {
        HStack(spacing: 2) {
            // Lock button
            Button(action: { onLockToggle?(!isLocked) }) {
                Image(systemName: isLocked ? "lock.fill" : "lock.open.fill")
                    .font(.caption)
                    .foregroundColor(isLocked ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            .help(isLocked ? "Unlock property" : "Lock property")
            
            // Add point button
            Button(action: { addPoint() }) {
                Image(systemName: "plus")
                    .font(.caption)
                    .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            .disabled(isLocked)
            .help("Add control point")
            
            // Remove point button
            if let selectedPointIndex = selectedPointIndex {
                Button(action: { removePoint(at: selectedPointIndex) }) {
                    Image(systemName: "minus")
                        .font(.caption)
                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .disabled(isLocked)
                .help("Remove control point")
            }
            
            // Reset button
            if let defaultValue = defaultValue, curve != defaultValue {
                Button(action: { resetToDefault() }) {
                    Image(systemName: "arrow.counterclockwise")
                        .font(.caption)
                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .disabled(isLocked)
                .help("Reset to default curve")
            }
            
            // Presets button
            Button(action: { showCurvePresets = true }) {
                Image(systemName: "slider.horizontal.3")
                    .font(.caption)
                    .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            .disabled(isLocked)
            .help("Curve presets")
            
            // Animation keyframe button
            if let onAnimationKeyframe = onAnimationKeyframe {
                Button(action: onAnimationKeyframe) {
                    Image(systemName: "key")
                        .font(.caption)
                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .disabled(isLocked)
                .help("Set animation keyframe")
            }
            
            // Expression button
            if let onExpressionInput = onExpressionInput {
                Button(action: { 
                    expressionText = curveToString(curve)
                    showExpressionDialog = true 
                }) {
                    Image(systemName: "function")
                        .font(.caption)
                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .disabled(isLocked)
                .help("Expression input")
            }
            
            // Link button
            if let onLinkProperty = onLinkProperty {
                Button(action: onLinkProperty) {
                    Image(systemName: "link")
                        .font(.caption)
                        .foregroundColor(isLocked ? DesignSystem.Colors.textDisabled : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .disabled(isLocked)
                .help("Link property")
            }
        }
    }
    
    @ViewBuilder
    private var contextMenuContent: some View {
        Group {
            Button("Copy Curve") {
                clipboardManager.copy(curveToString(curve))
            }
            .keyboardShortcut("c", modifiers: .command)
            
            Button("Paste Curve") {
                pasteCurve()
            }
            .keyboardShortcut("v", modifiers: .command)
            
            if let defaultValue = defaultValue, curve != defaultValue {
                Button("Reset to Default") {
                    resetToDefault()
                }
                .keyboardShortcut("r", modifiers: .command)
            }
            
            Divider()
            
            Button("Add Point at 0.5") {
                addPointAt(time: 0.5, value: 0.5)
            }
            
            Button("Clear All Points") {
                clearAllPoints()
            }
            
            Divider()
            
            Button("Expression Input") {
                expressionText = curveToString(curve)
                showExpressionDialog = true
            }
            .keyboardShortcut("e", modifiers: .command)
            
            if let onAnimationKeyframe = onAnimationKeyframe {
                Button("Set Keyframe") {
                    onAnimationKeyframe()
                }
                .keyboardShortcut("k", modifiers: .command)
            }
            
            Divider()
            
            Button(isLocked ? "Unlock" : "Lock") {
                onLockToggle?(!isLocked)
            }
        }
    }
    
    @ViewBuilder
    private var expressionDialog: some View {
        NavigationView {
            VStack(alignment: .leading, spacing: 16) {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Curve Expression")
                        .font(DesignSystem.Typography.headline)
                    
                    Text("Enter an expression that defines a curve (format: 'time1:value1,time2:value2,...'):")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Example: 0:0,0.5:1,1:0")
                        .font(DesignSystem.Typography.caption.monospaced())
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundSecondary)
                        .cornerRadius(4)
                }
                
                VStack(alignment: .leading, spacing: 4) {
                    Text("Expression:")
                        .font(DesignSystem.Typography.small)
                    
                    TextEditor(text: $expressionText)
                        .font(DesignSystem.Typography.body.monospaced())
                        .frame(minHeight: 100)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundPrimary)
                        .cornerRadius(6)
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(DesignSystem.Colors.border, lineWidth: 1)
                        )
                }
                
                Spacer()
            }
            .padding()
            .navigationTitle("Expression Input")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        showExpressionDialog = false
                    }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Apply") {
                        applyExpression()
                        showExpressionDialog = false
                    }
                    .disabled(expressionText.isEmpty)
                }
            }
        }
    }
    
    @ViewBuilder
    private var curvePresetsDialog: some View {
        NavigationView {
            VStack(alignment: .leading, spacing: 16) {
                Text("Curve Presets")
                    .font(DesignSystem.Typography.headline)
                
                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 2), spacing: 16) {
                    ForEach(CurvePreset.allCases, id: \.self) { preset in
                        Button(action: {
                            applyPreset(preset)
                            showCurvePresets = false
                        }) {
                            VStack(alignment: .leading, spacing: 8) {
                                Text(preset.name)
                                    .font(DesignSystem.Typography.body)
                                    .foregroundColor(DesignSystem.Colors.textPrimary)
                                
                                Text(preset.description)
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                    .multilineTextAlignment(.leading)
                            }
                            .padding()
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(8)
                        }
                        .buttonStyle(.plain)
                    }
                }
                
                Spacer()
            }
            .padding()
            .navigationTitle("Curve Presets")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") {
                        showCurvePresets = false
                    }
                }
            }
        }
    }
    
    private func updatePoint(at index: Int, location: CGPoint, in size: CGSize) {
        guard index < curve.points.count else { return }
        
        let newTime = max(0, min(1, location.x / size.width))
        let newValue = max(0, min(1, 1 - (location.y / size.height)))
        
        var updatedPoints = curve.points
        updatedPoints[index] = CurvePoint(time: newTime, value: newValue, inTangent: updatedPoints[index].inTangent, outTangent: updatedPoints[index].outTangent)
        
        curve = AnimationCurve(points: updatedPoints, preInfinity: curve.preInfinity, postInfinity: curve.postInfinity)
    }
    
    private func addPoint() {
        let newPoint = CurvePoint(time: 0.5, value: 0.5, inTangent: .zero, outTangent: .zero)
        var updatedPoints = curve.points
        updatedPoints.append(newPoint)
        curve = AnimationCurve(points: updatedPoints, preInfinity: curve.preInfinity, postInfinity: curve.postInfinity)
    }
    
    private func addPointAt(time: Float, value: Float) {
        let newPoint = CurvePoint(time: time, value: value, inTangent: .zero, outTangent: .zero)
        var updatedPoints = curve.points
        updatedPoints.append(newPoint)
        curve = AnimationCurve(points: updatedPoints, preInfinity: curve.preInfinity, postInfinity: curve.postInfinity)
    }
    
    private func removePoint(at index: Int) {
        guard index < curve.points.count, curve.points.count > 1 else { return }
        var updatedPoints = curve.points
        updatedPoints.remove(at: index)
        curve = AnimationCurve(points: updatedPoints, preInfinity: curve.preInfinity, postInfinity: curve.postInfinity)
        selectedPointIndex = nil
    }
    
    private func clearAllPoints() {
        curve = AnimationCurve(points: [], preInfinity: curve.preInfinity, postInfinity: curve.postInfinity)
        selectedPointIndex = nil
    }
    
    private func resetToDefault() {
        guard let defaultValue = defaultValue else { return }
        undoManager.recordState(curve)
        curve = defaultValue
        onReset?()
        onValueChanged?(defaultValue)
    }
    
    private func pasteCurve() {
        guard let pastedValue = clipboardManager.paste() else { return }
        let parsedCurve = stringToCurve(pastedValue)
        if parsedCurve != nil {
            undoManager.recordState(curve)
            curve = parsedCurve!
            onValueChanged?(parsedCurve!)
        }
    }
    
    private func applyExpression() {
        let parsedCurve = stringToCurve(expressionText)
        if parsedCurve != nil {
            undoManager.recordState(curve)
            curve = parsedCurve!
            onValueChanged?(parsedCurve!)
        }
    }
    
    private func applyPreset(_ preset: CurvePreset) {
        undoManager.recordState(curve)
        curve = preset.curve
        onValueChanged?(preset.curve)
    }
    
    private func curveToString(_ curve: AnimationCurve) -> String {
        return curve.points.map { "\($0.time):\($0.value)" }.joined(separator: ",")
    }
    
    private func stringToCurve(_ string: String) -> AnimationCurve? {
        let pairs = string.split(separator: ",")
        var points: [CurvePoint] = []
        
        for pair in pairs {
            let components = pair.split(separator: ":")
            if components.count == 2,
               let time = Float(components[0]),
               let value = Float(components[1]) {
                points.append(CurvePoint(time: time, value: value, inTangent: .zero, outTangent: .zero))
            }
        }
        
        return points.isEmpty ? nil : AnimationCurve(points: points, preInfinity: .constant, postInfinity: .constant)
    }
}

// MARK: - Supporting Types
struct AnimationCurve: Codable, Equatable {
    var points: [CurvePoint]
    var preInfinity: InfinityMode
    var postInfinity: InfinityMode
    
    init(points: [CurvePoint] = [], preInfinity: InfinityMode = .constant, postInfinity: InfinityMode = .constant) {
        self.points = points
        self.preInfinity = preInfinity
        self.postInfinity = postInfinity
    }
}

struct CurvePoint: Codable, Equatable {
    var time: Float
    var value: Float
    var inTangent: SIMD2<Float>
    var outTangent: SIMD2<Float>
    
    init(time: Float, value: Float, inTangent: SIMD2<Float>, outTangent: SIMD2<Float>) {
        self.time = time
        self.value = value
        self.inTangent = inTangent
        self.outTangent = outTangent
    }
}

enum InfinityMode: String, Codable, CaseIterable {
    case constant = "Constant"
    case linear = "Linear"
    case cycle = "Cycle"
    case cycleOffset = "Cycle Offset"
    case oscillate = "Oscillate"
}

enum CurvePreset: CaseIterable {
    case linear
    case easeIn
    case easeOut
    case easeInOut
    case exponential
    case bounce
    case elastic
    case sine
    
    var name: String {
        switch self {
        case .linear: return "Linear"
        case .easeIn: return "Ease In"
        case .easeOut: return "Ease Out"
        case .easeInOut: return "Ease In Out"
        case .exponential: return "Exponential"
        case .bounce: return "Bounce"
        case .elastic: return "Elastic"
        case .sine: return "Sine"
        }
    }
    
    var description: String {
        switch self {
        case .linear: return "Constant speed interpolation"
        case .easeIn: return "Slow start, fast end"
        case .easeOut: return "Fast start, slow end"
        case .easeInOut: return "Slow start and end"
        case .exponential: return "Exponential curve"
        case .bounce: return "Bouncing effect"
        case .elastic: return "Elastic stretch effect"
        case .sine: return "Smooth sine wave"
        }
    }
    
    var curve: AnimationCurve {
        switch self {
        case .linear:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 1, inTangent: .zero, outTangent: .zero)
            ])
        case .easeIn:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 1, inTangent: .zero, outTangent: .zero)
            ])
        case .easeOut:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 1, inTangent: .zero, outTangent: .zero)
            ])
        case .easeInOut:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.5, value: 0.5, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 1, inTangent: .zero, outTangent: .zero)
            ])
        case .exponential:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.1, value: 0.01, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.9, value: 0.99, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 1, inTangent: .zero, outTangent: .zero)
            ])
        case .bounce:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.25, value: 0.75, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.5, value: 0.25, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.75, value: 0.85, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 1, inTangent: .zero, outTangent: .zero)
            ])
        case .elastic:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.2, value: -0.1, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.4, value: 0.1, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.6, value: -0.05, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.8, value: 0.02, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 1, inTangent: .zero, outTangent: .zero)
            ])
        case .sine:
            return AnimationCurve(points: [
                CurvePoint(time: 0, value: 0, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.25, value: 0.5, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.5, value: 1, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 0.75, value: 0.5, inTangent: .zero, outTangent: .zero),
                CurvePoint(time: 1, value: 0, inTangent: .zero, outTangent: .zero)
            ])
        }
    }
}

// MARK: - Property Validation Editor (TODO-1482)
struct PropertyValidationEditor: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let valueType: ValidationValueType
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var validationRules: [ValidationRule] = []
    @State private var validationResult: ValidationResult = .valid
    @State private var showRuleEditor = false
    @State private var isHovering = false
    @State private var lastValidationTime: Date = Date()
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum ValidationValueType {
        case string
        case integer
        case float
        case boolean
        case vector2
        case vector3
        case vector4
        case color
        case enum(type: Any.Type)
    }
    
    struct ValidationRule: Identifiable, Codable {
        let id = UUID()
        var name: String
        var type: RuleType
        var parameters: [String: Any]
        var errorMessage: String
        var enabled: Bool
        var severity: Severity
        
        enum RuleType: String, CaseIterable, Codable {
            case range = "Range"
            case minLength = "Min Length"
            case maxLength = "Max Length"
            case pattern = "Pattern"
            case required = "Required"
            case custom = "Custom"
            case notEmpty = "Not Empty"
            case email = "Email"
            case url = "URL"
            case numeric = "Numeric"
            case positive = "Positive"
            case negative = "Negative"
        }
        
        enum Severity: String, CaseIterable, Codable {
            case info = "Info"
            case warning = "Warning"
            case error = "Error"
        }
    }
    
    enum ValidationResult {
        case valid
        case warning(message: String)
        case error(message: String)
        
        var isValid: Bool {
            switch self {
            case .valid: return true
            case .warning, .error: return false
            }
        }
        
        var message: String {
            switch self {
            case .valid: return "Valid"
            case .warning(let message): return message
            case .error(let message): return message
            }
        }
        
        var color: Color {
            switch self {
            case .valid: return DesignSystem.Colors.success
            case .warning: return DesignSystem.Colors.warning
            case .error: return DesignSystem.Colors.error
            }
        }
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with validation status
            HStack {
                Text("Property Validation")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                // Validation status indicator
                HStack(spacing: 4) {
                    Circle()
                        .fill(validationResult.color)
                        .frame(width: 8, height: 8)
                    
                    Text(validationResult.message)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(validationResult.color)
                }
                
                Button(action: { showRuleEditor = true }) {
                    Image(systemName: "plus.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
            }
            
            // Validation rules list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach($validationRules) { $rule in
                        ValidationRuleRowView(
                            rule: $rule,
                            onEdit: { editRule(rule) },
                            onDelete: { deleteRule(rule) },
                            onToggle: { toggleRule(rule) }
                        )
                    }
                }
            }
            .frame(height: 200)
            
            // Validation results
            if !validationResult.isValid {
                VStack(alignment: .leading, spacing: 4) {
                    HStack {
                        Image(systemName: validationResult.isValid ? "checkmark.circle.fill" : "exclamationmark.triangle.fill")
                            .foregroundColor(validationResult.color)
                        
                        Text("Validation Results")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        Text("Last checked: \(lastValidationTime, style: .time)")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Text(validationResult.message)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(validationResult.color)
                        .padding(8)
                        .background(validationResult.color.opacity(0.1))
                        .cornerRadius(4)
                }
            }
            
            // Control buttons
            HStack(spacing: 8) {
                Button("Validate Now") {
                    validateValue()
                }
                .buttonStyle(.bordered)
                
                Button("Clear All Rules") {
                    clearAllRules()
                }
                .buttonStyle(.bordered)
                .foregroundColor(DesignSystem.Colors.error)
                
                Spacer()
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
            }
        }
        .sheet(isPresented: $showRuleEditor) {
            ValidationRuleEditorView(
                rule: ValidationRule(name: "", type: .required, parameters: [:], errorMessage: "", enabled: true, severity: .error),
                valueType: valueType,
                onSave: { newRule in
                    validationRules.append(newRule)
                    validateValue()
                }
            )
        }
        .onAppear {
            validateValue()
        }
        .onChange(of: value) { _ in
            validateValue()
        }
    }
    
    private func validateValue() {
        lastValidationTime = Date()
        
        for rule in validationRules.filter(\.enabled) {
            let result = validateRule(rule, value: value)
            if result != .valid {
                validationResult = result
                return
            }
        }
        
        validationResult = .valid
    }
    
    private func validateRule(_ rule: ValidationRule, value: Any) -> ValidationResult {
        switch rule.type {
        case .required:
            if value is String && (value as? String)?.isEmpty ?? true {
                return .error(message: rule.errorMessage.isEmpty ? "Value is required" : rule.errorMessage)
            }
            
        case .notEmpty:
            if let str = value as? String, str.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
                return .error(message: rule.errorMessage.isEmpty ? "Value cannot be empty" : rule.errorMessage)
            }
            
        case .range:
            if let params = rule.parameters as? [String: Double],
               let min = params["min"],
               let max = params["max"] {
                
                if let num = value as? Double {
                    if num < min || num > max {
                        return .error(message: rule.errorMessage.isEmpty ? "Value must be between \(min) and \(max)" : rule.errorMessage)
                    }
                } else if let num = value as? Float {
                    if Double(num) < min || Double(num) > max {
                        return .error(message: rule.errorMessage.isEmpty ? "Value must be between \(min) and \(max)" : rule.errorMessage)
                    }
                }
            }
            
        case .minLength:
            if let minLength = rule.parameters["minLength"] as? Int,
               let str = value as? String,
               str.count < minLength {
                return .error(message: rule.errorMessage.isEmpty ? "Minimum length is \(minLength)" : rule.errorMessage)
            }
            
        case .maxLength:
            if let maxLength = rule.parameters["maxLength"] as? Int,
               let str = value as? String,
               str.count > maxLength {
                return .error(message: rule.errorMessage.isEmpty ? "Maximum length is \(maxLength)" : rule.errorMessage)
            }
            
        case .pattern:
            if let pattern = rule.parameters["pattern"] as? String,
               let str = value as? String {
                let regex = try? NSRegularExpression(pattern: pattern)
                let range = NSRange(location: 0, length: str.utf16.count)
                if regex?.firstMatch(in: str, options: [], range: range) == nil {
                    return .error(message: rule.errorMessage.isEmpty ? "Value does not match required pattern" : rule.errorMessage)
                }
            }
            
        case .email:
            if let str = value as? String {
                let emailRegex = "[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,64}"
                let emailPredicate = NSPredicate(format:"SELF MATCHES %@", emailRegex)
                if !emailPredicate.evaluate(with: str) {
                    return .error(message: rule.errorMessage.isEmpty ? "Invalid email format" : rule.errorMessage)
                }
            }
            
        case .url:
            if let str = value as? String {
                guard let url = URL(string: str) else {
                    return .error(message: rule.errorMessage.isEmpty ? "Invalid URL format" : rule.errorMessage)
                }
                if !UIApplication.shared.canOpenURL(url) {
                    return .warning(message: rule.errorMessage.isEmpty ? "URL may not be valid" : rule.errorMessage)
                }
            }
            
        case .numeric:
            if let str = value as? String {
                if Double(str) == nil {
                    return .error(message: rule.errorMessage.isEmpty ? "Value must be numeric" : rule.errorMessage)
                }
            }
            
        case .positive:
            if let num = value as? Double, num <= 0 {
                return .error(message: rule.errorMessage.isEmpty ? "Value must be positive" : rule.errorMessage)
            } else if let num = value as? Float, num <= 0 {
                return .error(message: rule.errorMessage.isEmpty ? "Value must be positive" : rule.errorMessage)
            }
            
        case .negative:
            if let num = value as? Double, num >= 0 {
                return .error(message: rule.errorMessage.isEmpty ? "Value must be negative" : rule.errorMessage)
            } else if let num = value as? Float, num >= 0 {
                return .error(message: rule.errorMessage.isEmpty ? "Value must be negative" : rule.errorMessage)
            }
            
        case .custom:
            // Custom validation would be implemented here
            break
        }
        
        return .valid
    }
    
    private func editRule(_ rule: ValidationRule) {
        // Implementation for editing existing rule
    }
    
    private func deleteRule(_ rule: ValidationRule) {
        validationRules.removeAll { $0.id == rule.id }
        validateValue()
    }
    
    private func toggleRule(_ rule: ValidationRule) {
        if let index = validationRules.firstIndex(where: { $0.id == rule.id }) {
            validationRules[index].enabled.toggle()
            validateValue()
        }
    }
    
    private func clearAllRules() {
        validationRules.removeAll()
        validationResult = .valid
    }
}

// MARK: - Validation Rule Row View
struct ValidationRuleRowView: View {
    @Binding var rule: PropertyValidationEditor.ValidationRule
    let onEdit: () -> Void
    let onDelete: () -> Void
    let onToggle: () -> Void
    
    var body: some View {
        HStack {
            // Enable/disable toggle
            Toggle("", isOn: $rule.enabled)
                .toggleStyle(.switch)
                .onChange(of: rule.enabled) { _ in
                    onToggle()
                }
            
            // Rule info
            VStack(alignment: .leading, spacing: 2) {
                HStack {
                    Text(rule.name)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    // Severity indicator
                    Text(rule.severity.rawValue)
                        .font(DesignSystem.Typography.caption)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(severityColor.opacity(0.2))
                        .foregroundColor(severityColor)
                        .cornerRadius(3)
                }
                
                Text(rule.type.rawValue)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                if !rule.errorMessage.isEmpty {
                    Text(rule.errorMessage)
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .lineLimit(1)
                }
            }
            
            // Action buttons
            HStack(spacing: 4) {
                Button(action: onEdit) {
                    Image(systemName: "pencil")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                Button(action: onDelete) {
                    Image(systemName: "trash")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.error)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(rule.enabled ? DesignSystem.Colors.backgroundPrimary : DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
    
    private var severityColor: Color {
        switch rule.severity {
        case .info: return DesignSystem.Colors.info
        case .warning: return DesignSystem.Colors.warning
        case .error: return DesignSystem.Colors.error
        }
    }
}

// MARK: - Validation Rule Editor View
struct ValidationRuleEditorView: View {
    @Binding var rule: PropertyValidationEditor.ValidationRule
    let valueType: PropertyValidationEditor.ValidationValueType
    let onSave: (PropertyValidationEditor.ValidationRule) -> Void
    @Environment(\.dismiss) private var dismiss
    
    @State private var tempRule: PropertyValidationEditor.ValidationRule
    @State private var rangeMin: Double = 0
    @State private var rangeMax: Double = 100
    @State private var minLength: Int = 1
    @State private var maxLength: Int = 50
    @State private var pattern: String = ""
    
    init(rule: PropertyValidationEditor.ValidationRule, valueType: PropertyValidationEditor.ValidationValueType, onSave: @escaping (PropertyValidationEditor.ValidationRule) -> Void) {
        self._rule = Binding(get: { rule }, set: { rule = $0 })
        self.valueType = valueType
        self.onSave = onSave
        self._tempRule = State(initialValue: rule)
    }
    
    var body: some View {
        NavigationView {
            Form {
                Section("Rule Information") {
                    TextField("Rule Name", text: $tempRule.name)
                    
                    Picker("Rule Type", selection: $tempRule.type) {
                        ForEach(applicableRuleTypes, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    
                    Picker("Severity", selection: $tempRule.severity) {
                        ForEach(PropertyValidationEditor.ValidationRule.Severity.allCases, id: \.self) { severity in
                            Text(severity.rawValue).tag(severity)
                        }
                    }
                    
                    Toggle("Enabled", isOn: $tempRule.enabled)
                }
                
                Section("Error Message") {
                    TextField("Error message", text: $tempRule.errorMessage, axis: .vertical)
                        .lineLimit(3)
                }
                
                // Rule-specific parameters
                ruleParametersSection
                
                Section("Preview") {
                    Text("This rule will validate: \(tempRule.type.rawValue)")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            .navigationTitle("Edit Validation Rule")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        updateRuleParameters()
                        onSave(tempRule)
                        dismiss()
                    }
                    .disabled(tempRule.name.isEmpty)
                }
            }
        }
        .frame(width: 400, height: 500)
    }
    
    @ViewBuilder
    private var ruleParametersSection: some View {
        switch tempRule.type {
        case .range:
            Section("Range Parameters") {
                HStack {
                    Text("Minimum:")
                    TextField("Min", value: $rangeMin, format: .number)
                        .textFieldStyle(.roundedBorder)
                }
                
                HStack {
                    Text("Maximum:")
                    TextField("Max", value: $rangeMax, format: .number)
                        .textFieldStyle(.roundedBorder)
                }
            }
            
        case .minLength:
            Section("Length Parameters") {
                HStack {
                    Text("Minimum Length:")
                    TextField("Min Length", value: $minLength, format: .number)
                        .textFieldStyle(.roundedBorder)
                }
            }
            
        case .maxLength:
            Section("Length Parameters") {
                HStack {
                    Text("Maximum Length:")
                    TextField("Max Length", value: $maxLength, format: .number)
                        .textFieldStyle(.roundedBorder)
                }
            }
            
        case .pattern:
            Section("Pattern Parameters") {
                TextField("Regular Expression", text: $pattern)
                    .textFieldStyle(.roundedBorder)
                
                Text("Enter a valid regular expression pattern")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
        default:
            EmptyView()
        }
    }
    
    private var applicableRuleTypes: [PropertyValidationEditor.ValidationRule.RuleType] {
        switch valueType {
        case .string:
            return [.required, .notEmpty, .minLength, .maxLength, .pattern, .email, .url, .custom]
        case .integer, .float:
            return [.required, .range, .positive, .negative, .numeric, .custom]
        case .boolean:
            return [.required, .custom]
        case .vector2, .vector3, .vector4:
            return [.required, .range, .custom]
        case .color:
            return [.required, .custom]
        case .enum:
            return [.required, .custom]
        }
    }
    
    private func updateRuleParameters() {
        switch tempRule.type {
        case .range:
            tempRule.parameters["min"] = rangeMin
            tempRule.parameters["max"] = rangeMax
            
        case .minLength:
            tempRule.parameters["minLength"] = minLength
            
        case .maxLength:
            tempRule.parameters["maxLength"] = maxLength
            
        case .pattern:
            tempRule.parameters["pattern"] = pattern
            
        default:
            break
        }
    }
}

// MARK: - Property Conditional Visibility (TODO-1483)
struct PropertyConditionalVisibility: View, PropertyEditor {
    let propertyName: String
    @Binding var isVisible: Bool
    let sourceProperties: [String]
    
    // Configuration
    let config: PropertyEditorConfig<Bool>
    
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
    
    // State
    @State private var conditions: [VisibilityCondition] = []
    @State private var logicOperator: LogicOperator = .and
    @State private var showConditionEditor = false
    @State private var isHovering = false
    @State private var evaluationResult: Bool = true
    @State private var lastEvaluationTime: Date = Date()
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    struct VisibilityCondition: Identifiable, Codable {
        let id = UUID()
        var sourceProperty: String
        var operator: ConditionOperator
        var compareValue: String
        var enabled: Bool
        
        enum ConditionOperator: String, CaseIterable, Codable {
            case equals = "Equals"
            case notEquals = "Not Equals"
            case greaterThan = "Greater Than"
            case lessThan = "Less Than"
            case greaterThanOrEqual = "Greater Than or Equal"
            case lessThanOrEqual = "Less Than or Equal"
            case contains = "Contains"
            case notContains = "Not Contains"
            case startsWith = "Starts With"
            case endsWith = "Ends With"
            case isEmpty = "Is Empty"
            case isNotEmpty = "Is Not Empty"
            case isTrue = "Is True"
            case isFalse = "Is False"
        }
    }
    
    enum LogicOperator: String, CaseIterable, Codable {
        case and = "AND"
        case or = "OR"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with evaluation result
            HStack {
                Text("Conditional Visibility")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                // Evaluation status indicator
                HStack(spacing: 4) {
                    Circle()
                        .fill(evaluationResult ? DesignSystem.Colors.success : DesignSystem.Colors.error)
                        .frame(width: 8, height: 8)
                    
                    Text(evaluationResult ? "Visible" : "Hidden")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(evaluationResult ? DesignSystem.Colors.success : DesignSystem.Colors.error)
                }
                
                Button(action: { showConditionEditor = true }) {
                    Image(systemName: "plus.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
            }
            
            // Logic operator selection
            HStack {
                Text("Logic:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Picker("Logic Operator", selection: $logicOperator) {
                    ForEach(LogicOperator.allCases, id: \.self) { op in
                        Text(op.rawValue).tag(op)
                    }
                }
                .pickerStyle(.segmented)
                .onChange(of: logicOperator) { _ in
                    evaluateConditions()
                }
            }
            
            // Conditions list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach($conditions) { $condition in
                        VisibilityConditionRowView(
                            condition: $condition,
                            sourceProperties: sourceProperties,
                            onEdit: { editCondition(condition) },
                            onDelete: { deleteCondition(condition) },
                            onToggle: { toggleCondition(condition) }
                        )
                    }
                }
            }
            .frame(height: 200)
            
            // Evaluation results
            VStack(alignment: .leading, spacing: 4) {
                HStack {
                    Image(systemName: "info.circle")
                        .foregroundColor(DesignSystem.Colors.info)
                    
                    Text("Evaluation Results")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Text("Last evaluated: \(lastEvaluationTime, style: .time)")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                
                Text("Property is \(evaluationResult ? "visible" : "hidden") based on current conditions")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(evaluationResult ? DesignSystem.Colors.success : DesignSystem.Colors.error)
                    .padding(8)
                    .background((evaluationResult ? DesignSystem.Colors.success : DesignSystem.Colors.error).opacity(0.1))
                    .cornerRadius(4)
            }
            
            // Control buttons
            HStack(spacing: 8) {
                Button("Evaluate Now") {
                    evaluateConditions()
                }
                .buttonStyle(.bordered)
                
                Button("Clear All Conditions") {
                    clearAllConditions()
                }
                .buttonStyle(.bordered)
                .foregroundColor(DesignSystem.Colors.error)
                
                Spacer()
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
            }
        }
        .sheet(isPresented: $showConditionEditor) {
            ConditionEditorView(
                condition: VisibilityCondition(sourceProperty: "", operator: .equals, compareValue: "", enabled: true),
                sourceProperties: sourceProperties,
                onSave: { newCondition in
                    conditions.append(newCondition)
                    evaluateConditions()
                }
            )
        }
        .onAppear {
            evaluateConditions()
        }
    }
    
    private func evaluateConditions() {
        lastEvaluationTime = Date()
        
        let enabledConditions = conditions.filter(\.enabled)
        
        if enabledConditions.isEmpty {
            evaluationResult = true
            isVisible = true
            return
        }
        
        var results: [Bool] = []
        
        for condition in enabledConditions {
            let result = evaluateCondition(condition)
            results.append(result)
        }
        
        switch logicOperator {
        case .and:
            evaluationResult = results.allSatisfy { $0 }
        case .or:
            evaluationResult = results.contains { $0 }
        }
        
        isVisible = evaluationResult
        onValueChanged?(evaluationResult)
    }
    
    private func evaluateCondition(_ condition: VisibilityCondition) -> Bool {
        // Mock evaluation - in real implementation, this would get the actual property value
        // For now, we'll simulate with some basic logic
        
        switch condition.operator {
        case .equals:
            return condition.compareValue.lowercased() == "true"
        case .notEquals:
            return condition.compareValue.lowercased() != "true"
        case .isTrue:
            return condition.compareValue.lowercased() == "true"
        case .isFalse:
            return condition.compareValue.lowercased() == "false"
        case .isEmpty:
            return condition.compareValue.isEmpty
        case .isNotEmpty:
            return !condition.compareValue.isEmpty
        case .contains:
            return condition.compareValue.lowercased().contains("test")
        case .notContains:
            return !condition.compareValue.lowercased().contains("test")
        case .startsWith:
            return condition.compareValue.lowercased().hasPrefix("test")
        case .endsWith:
            return condition.compareValue.lowercased().hasSuffix("test")
        default:
            return true
        }
    }
    
    private func editCondition(_ condition: VisibilityCondition) {
        // Implementation for editing existing condition
    }
    
    private func deleteCondition(_ condition: VisibilityCondition) {
        conditions.removeAll { $0.id == condition.id }
        evaluateConditions()
    }
    
    private func toggleCondition(_ condition: VisibilityCondition) {
        if let index = conditions.firstIndex(where: { $0.id == condition.id }) {
            conditions[index].enabled.toggle()
            evaluateConditions()
        }
    }
    
    private func clearAllConditions() {
        conditions.removeAll()
        evaluationResult = true
        isVisible = true
        onValueChanged?(true)
    }
}

// MARK: - Visibility Condition Row View
struct VisibilityConditionRowView: View {
    @Binding var condition: PropertyConditionalVisibility.VisibilityCondition
    let sourceProperties: [String]
    let onEdit: () -> Void
    let onDelete: () -> Void
    let onToggle: () -> Void
    
    var body: some View {
        HStack {
            // Enable/disable toggle
            Toggle("", isOn: $condition.enabled)
                .toggleStyle(.switch)
                .onChange(of: condition.enabled) { _ in
                    onToggle()
                }
            
            // Condition info
            VStack(alignment: .leading, spacing: 2) {
                HStack {
                    Text(condition.sourceProperty.isEmpty ? "Select Property" : condition.sourceProperty)
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(condition.sourceProperty.isEmpty ? DesignSystem.Colors.textTertiary : DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    // Operator badge
                    Text(condition.operator.rawValue)
                        .font(DesignSystem.Typography.caption)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(DesignSystem.Colors.accentPrimary.opacity(0.2))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                        .cornerRadius(3)
                }
                
                HStack {
                    Text("Compare:")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text(condition.compareValue.isEmpty ? "Set value" : condition.compareValue)
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(condition.compareValue.isEmpty ? DesignSystem.Colors.textTertiary : DesignSystem.Colors.textSecondary)
                        .lineLimit(1)
                }
            }
            
            // Action buttons
            HStack(spacing: 4) {
                Button(action: onEdit) {
                    Image(systemName: "pencil")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                Button(action: onDelete) {
                    Image(systemName: "trash")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.error)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(condition.enabled ? DesignSystem.Colors.backgroundPrimary : DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}

// MARK: - Condition Editor View
struct ConditionEditorView: View {
    @Binding var condition: PropertyConditionalVisibility.VisibilityCondition
    let sourceProperties: [String]
    let onSave: (PropertyConditionalVisibility.VisibilityCondition) -> Void
    @Environment(\.dismiss) private var dismiss
    
    @State private var tempCondition: PropertyConditionalVisibility.VisibilityCondition
    @State private var numericCompareValue: Double = 0
    @State private var booleanCompareValue: Bool = false
    
    init(condition: PropertyConditionalVisibility.VisibilityCondition, sourceProperties: [String], onSave: @escaping (PropertyConditionalVisibility.VisibilityCondition) -> Void) {
        self._condition = Binding(get: { condition }, set: { condition = $0 })
        self.sourceProperties = sourceProperties
        self.onSave = onSave
        self._tempCondition = State(initialValue: condition)
    }
    
    var body: some View {
        NavigationView {
            Form {
                Section("Condition Information") {
                    Picker("Source Property", selection: $tempCondition.sourceProperty) {
                        Text("Select Property").tag("")
                        ForEach(sourceProperties, id: \.self) { property in
                            Text(property).tag(property)
                        }
                    }
                    
                    Picker("Operator", selection: $tempCondition.operator) {
                        ForEach(PropertyConditionalVisibility.VisibilityCondition.ConditionOperator.allCases, id: \.self) { op in
                            Text(op.rawValue).tag(op)
                        }
                    }
                    
                    Toggle("Enabled", isOn: $tempCondition.enabled)
                }
                
                // Compare value input based on operator
                compareValueSection
                
                Section("Preview") {
                    Text("Condition: \(tempCondition.sourceProperty.isEmpty ? "Select Property" : tempCondition.sourceProperty) \(tempCondition.operator.rawValue) \(displayCompareValue)")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
            }
            .navigationTitle("Edit Visibility Condition")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        updateCompareValue()
                        onSave(tempCondition)
                        dismiss()
                    }
                    .disabled(tempCondition.sourceProperty.isEmpty || !isValidCondition)
                }
            }
        }
        .frame(width: 400, height: 450)
    }
    
    @ViewBuilder
    private var compareValueSection: some View {
        if requiresCompareValue {
            Section("Compare Value") {
                switch valueInputType {
                case .text:
                    TextField("Value", text: $tempCondition.compareValue)
                        .textFieldStyle(.roundedBorder)
                    
                case .numeric:
                    TextField("Value", value: $numericCompareValue, format: .number)
                        .textFieldStyle(.roundedBorder)
                    
                case .boolean:
                    Toggle("Value", isOn: $booleanCompareValue)
                    
                case .none:
                    EmptyView()
                }
            }
        }
    }
    
    private var valueInputType: InputType {
        switch tempCondition.operator {
        case .equals, .notEquals, .contains, .notContains, .startsWith, .endsWith:
            return .text
        case .greaterThan, .lessThan, .greaterThanOrEqual, .lessThanOrEqual:
            return .numeric
        case .isTrue, .isFalse, .isEmpty, .isNotEmpty:
            return .none
        }
    }
    
    private var requiresCompareValue: Bool {
        valueInputType != .none
    }
    
    private var isValidCondition: Bool {
        !tempCondition.sourceProperty.isEmpty && (
            !requiresCompareValue || 
            (valueInputType == .text && !tempCondition.compareValue.isEmpty) ||
            (valueInputType == .numeric) ||
            (valueInputType == .boolean)
        )
    }
    
    private var displayCompareValue: String {
        switch valueInputType {
        case .text:
            return tempCondition.compareValue.isEmpty ? "Value Required" : tempCondition.compareValue
        case .numeric:
            return String(format: "%.2f", numericCompareValue)
        case .boolean:
            return booleanCompareValue ? "True" : "False"
        case .none:
            return ""
        }
    }
    
    private enum InputType {
        case text
        case numeric
        case boolean
        case none
    }
    
    private func updateCompareValue() {
        switch valueInputType {
        case .numeric:
            tempCondition.compareValue = String(numericCompareValue)
        case .boolean:
            tempCondition.compareValue = String(booleanCompareValue)
        default:
            break
        }
    }
}

// MARK: - Property Dependencies (TODO-1484)
struct PropertyDependencies: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let availableProperties: [String]
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var dependencies: [PropertyDependency] = []
    @State private var showDependencyEditor = false
    @State private var isHovering = false
    @State private var dependencyGraph: [DependencyNode] = []
    @State private var circularDependencies: [String] = []
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    struct PropertyDependency: Identifiable, Codable {
        let id = UUID()
        var sourceProperty: String
        var targetProperty: String
        var dependencyType: DependencyType
        var strength: Double // 0.0 to 1.0
        var enabled: Bool
        var bidirectional: Bool
        
        enum DependencyType: String, CaseIterable, Codable {
            case influences = "Influences"
            case controls = "Controls"
            case enables = "Enables"
            case disables = "Disables"
            case requires = "Requires"
            case conflicts = "Conflicts"
            case modifies = "Modifies"
            case triggers = "Triggers"
        }
    }
    
    struct DependencyNode: Identifiable {
        let id = UUID()
        let propertyName: String
        var dependencies: [String]
        var dependents: [String]
        var position: CGPoint
        var isCircular: Bool
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with dependency status
            HStack {
                Text("Property Dependencies")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                // Circular dependency warning
                if !circularDependencies.isEmpty {
                    HStack(spacing: 4) {
                        Image(systemName: "exclamationmark.triangle.fill")
                            .foregroundColor(DesignSystem.Colors.warning)
                        
                        Text("Circular Dependencies")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.warning)
                    }
                }
                
                Button(action: { showDependencyEditor = true }) {
                    Image(systemName: "plus.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
            }
            
            // Dependency graph visualization
            dependencyGraphView
                .frame(height: 200)
                .background(DesignSystem.Colors.backgroundSecondary)
                .cornerRadius(8)
            
            // Dependencies list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach($dependencies) { $dependency in
                        PropertyDependencyRowView(
                            dependency: $dependency,
                            availableProperties: availableProperties,
                            onEdit: { editDependency(dependency) },
                            onDelete: { deleteDependency(dependency) },
                            onToggle: { toggleDependency(dependency) }
                        )
                    }
                }
            }
            .frame(height: 150)
            
            // Control buttons
            HStack(spacing: 8) {
                Button("Validate Dependencies") {
                    validateDependencies()
                }
                .buttonStyle(.bordered)
                
                Button("Clear All Dependencies") {
                    clearAllDependencies()
                }
                .buttonStyle(.bordered)
                .foregroundColor(DesignSystem.Colors.error)
                
                Spacer()
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
            }
        }
        .sheet(isPresented: $showDependencyEditor) {
            DependencyEditorView(
                dependency: PropertyDependency(sourceProperty: "", targetProperty: "", dependencyType: .influences, strength: 1.0, enabled: true, bidirectional: false),
                availableProperties: availableProperties,
                onSave: { newDependency in
                    dependencies.append(newDependency)
                    buildDependencyGraph()
                    validateDependencies()
                }
            )
        }
        .onAppear {
            buildDependencyGraph()
            validateDependencies()
        }
        .onChange(of: dependencies) { _ in
            buildDependencyGraph()
            validateDependencies()
        }
    }
    
    @ViewBuilder
    private var dependencyGraphView: some View {
        GeometryReader { geometry in
            let width = geometry.size.width
            let height = geometry.size.height
            
            ZStack {
                // Draw connections
                ForEach(dependencyGraph) { node in
                    ForEach(node.dependencies, id: \.self) { dependencyName in
                        if let targetNode = dependencyGraph.first(where: { $0.propertyName == dependencyName }) {
                            Path { path in
                                path.move(to: node.position)
                                path.addLine(to: targetNode.position)
                            }
                            .stroke(node.isCircular ? DesignSystem.Colors.error : DesignSystem.Colors.accentPrimary, lineWidth: 2)
                            .opacity(0.6)
                        }
                    }
                }
                
                // Draw nodes
                ForEach(dependencyGraph) { node in
                    Circle()
                        .fill(node.isCircular ? DesignSystem.Colors.error.opacity(0.3) : DesignSystem.Colors.backgroundPrimary)
                        .stroke(node.isCircular ? DesignSystem.Colors.error : DesignSystem.Colors.accentPrimary, lineWidth: 2)
                        .frame(width: 60, height: 60)
                        .position(node.position)
                        .overlay(
                            Text(node.propertyName)
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(node.isCircular ? DesignSystem.Colors.error : DesignSystem.Colors.textPrimary)
                                .lineLimit(1)
                        )
                }
            }
        }
    }
    
    private func buildDependencyGraph() {
        var nodes: [DependencyNode] = []
        
        // Create nodes for all properties
        for property in availableProperties {
            let position = calculateNodePosition(for: property, index: availableProperties.firstIndex(of: property) ?? 0)
            let node = DependencyNode(
                propertyName: property,
                dependencies: [],
                dependents: [],
                position: position,
                isCircular: false
            )
            nodes.append(node)
        }
        
        // Add dependencies
        for dependency in dependencies.filter(\.enabled) {
            if let sourceIndex = nodes.firstIndex(where: { $0.propertyName == dependency.sourceProperty }),
               let targetIndex = nodes.firstIndex(where: { $0.propertyName == dependency.targetProperty }) {
                nodes[sourceIndex].dependencies.append(dependency.targetProperty)
                nodes[targetIndex].dependents.append(dependency.sourceProperty)
                
                if dependency.bidirectional {
                    nodes[targetIndex].dependencies.append(dependency.sourceProperty)
                    nodes[sourceIndex].dependents.append(dependency.targetProperty)
                }
            }
        }
        
        dependencyGraph = nodes
    }
    
    private func calculateNodePosition(for property: String, index: Int) -> CGPoint {
        let columns = 4
        let row = index / columns
        let col = index % columns
        let spacing: CGFloat = 80
        let padding: CGFloat = 40
        
        return CGPoint(
            x: padding + CGFloat(col) * spacing,
            y: padding + CGFloat(row) * spacing
        )
    }
    
    private func validateDependencies() {
        circularDependencies = []
        var updatedGraph = dependencyGraph
        
        // Detect circular dependencies using DFS
        for (index, node) in updatedGraph.enumerated() {
            var visited: Set<String> = []
            var recursionStack: Set<String> = []
            
            if hasCircularDependency(from: node, in: updatedGraph, visited: &visited, recursionStack: &recursionStack) {
                circularDependencies.append(node.propertyName)
                updatedGraph[index].isCircular = true
            } else {
                updatedGraph[index].isCircular = false
            }
        }
        
        dependencyGraph = updatedGraph
    }
    
    private func hasCircularDependency(from node: DependencyNode, in graph: [DependencyNode], visited: inout Set<String>, recursionStack: inout Set<String>) -> Bool {
        if recursionStack.contains(node.propertyName) {
            return true
        }
        
        if visited.contains(node.propertyName) {
            return false
        }
        
        visited.insert(node.propertyName)
        recursionStack.insert(node.propertyName)
        
        for dependency in node.dependencies {
            if let dependentNode = graph.first(where: { $0.propertyName == dependency }) {
                if hasCircularDependency(from: dependentNode, in: graph, visited: &visited, recursionStack: &recursionStack) {
                    return true
                }
            }
        }
        
        recursionStack.remove(node.propertyName)
        return false
    }
    
    private func editDependency(_ dependency: PropertyDependency) {
        // Implementation for editing existing dependency
    }
    
    private func deleteDependency(_ dependency: PropertyDependency) {
        dependencies.removeAll { $0.id == dependency.id }
        buildDependencyGraph()
        validateDependencies()
    }
    
    private func toggleDependency(_ dependency: PropertyDependency) {
        if let index = dependencies.firstIndex(where: { $0.id == dependency.id }) {
            dependencies[index].enabled.toggle()
            buildDependencyGraph()
            validateDependencies()
        }
    }
    
    private func clearAllDependencies() {
        dependencies.removeAll()
        dependencyGraph.removeAll()
        circularDependencies.removeAll()
    }
}

// MARK: - Property Dependency Row View
struct PropertyDependencyRowView: View {
    @Binding var dependency: PropertyDependencies.PropertyDependency
    let availableProperties: [String]
    let onEdit: () -> Void
    let onDelete: () -> Void
    let onToggle: () -> Void
    
    var body: some View {
        HStack {
            // Enable/disable toggle
            Toggle("", isOn: $dependency.enabled)
                .toggleStyle(.switch)
                .onChange(of: dependency.enabled) { _ in
                    onToggle()
                }
            
            // Dependency info
            VStack(alignment: .leading, spacing: 2) {
                HStack {
                    VStack(alignment: .leading, spacing: 1) {
                        Text(dependency.sourceProperty.isEmpty ? "Source" : dependency.sourceProperty)
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(dependency.sourceProperty.isEmpty ? DesignSystem.Colors.textTertiary : DesignSystem.Colors.textPrimary)
                        
                        HStack(spacing: 2) {
                            Image(systemName: "arrow.right")
                                .font(.caption)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                            
                            Text(dependency.targetProperty.isEmpty ? "Target" : dependency.targetProperty)
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(dependency.targetProperty.isEmpty ? DesignSystem.Colors.textTertiary : DesignSystem.Colors.textPrimary)
                        }
                    }
                    
                    Spacer()
                    
                    // Type badge
                    Text(dependency.dependencyType.rawValue)
                        .font(DesignSystem.Typography.caption)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(DesignSystem.Colors.accentPrimary.opacity(0.2))
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                        .cornerRadius(3)
                }
                
                HStack {
                    Text("Strength:")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Text(String(format: "%.1f", dependency.strength))
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    if dependency.bidirectional {
                        Text("Bidirectional")
                            .font(DesignSystem.Typography.caption)
                            .padding(.horizontal, 4)
                            .padding(.vertical, 1)
                            .background(DesignSystem.Colors.info.opacity(0.2))
                            .foregroundColor(DesignSystem.Colors.info)
                            .cornerRadius(2)
                    }
                }
            }
            
            // Action buttons
            HStack(spacing: 4) {
                Button(action: onEdit) {
                    Image(systemName: "pencil")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                Button(action: onDelete) {
                    Image(systemName: "trash")
                        .font(.caption)
                        .foregroundColor(DesignSystem.Colors.error)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(dependency.enabled ? DesignSystem.Colors.backgroundPrimary : DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}

// MARK: - Dependency Editor View
struct DependencyEditorView: View {
    @Binding var dependency: PropertyDependencies.PropertyDependency
    let availableProperties: [String]
    let onSave: (PropertyDependencies.PropertyDependency) -> Void
    @Environment(\.dismiss) private var dismiss
    
    @State private var tempDependency: PropertyDependencies.PropertyDependency
    @State private var strength: Double = 1.0
    
    init(dependency: PropertyDependencies.PropertyDependency, availableProperties: [String], onSave: @escaping (PropertyDependencies.PropertyDependency) -> Void) {
        self._dependency = Binding(get: { dependency }, set: { dependency = $0 })
        self.availableProperties = availableProperties
        self.onSave = onSave
        self._tempDependency = State(initialValue: dependency)
        self._strength = State(initialValue: dependency.strength)
    }
    
    var body: some View {
        NavigationView {
            Form {
                Section("Dependency Information") {
                    Picker("Source Property", selection: $tempDependency.sourceProperty) {
                        Text("Select Property").tag("")
                        ForEach(availableProperties, id: \.self) { property in
                            Text(property).tag(property)
                        }
                    }
                    
                    Picker("Target Property", selection: $tempDependency.targetProperty) {
                        Text("Select Property").tag("")
                        ForEach(availableProperties, id: \.self) { property in
                            Text(property).tag(property)
                        }
                    }
                    
                    Picker("Dependency Type", selection: $tempDependency.dependencyType) {
                        ForEach(PropertyDependencies.PropertyDependency.DependencyType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    
                    Toggle("Enabled", isOn: $tempDependency.enabled)
                    Toggle("Bidirectional", isOn: $tempDependency.bidirectional)
                }
                
                Section("Strength") {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Dependency Strength: \(String(format: "%.1f", strength))")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Slider(value: $strength, in: 0...1, step: 0.1) {
                            Text("0")
                        } minimumValueLabel: {
                            Text("1")
                        }
                        .onChange(of: strength) { newValue in
                            tempDependency.strength = newValue
                        }
                    }
                }
                
                Section("Preview") {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Dependency Relationship:")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text("\(tempDependency.sourceProperty.isEmpty ? "Source" : tempDependency.sourceProperty) \(tempDependency.bidirectional ? "↔" : "→") \(tempDependency.targetProperty.isEmpty ? "Target" : tempDependency.targetProperty)")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                        
                        Text("Type: \(tempDependency.dependencyType.rawValue), Strength: \(String(format: "%.1f", tempDependency.strength))")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                }
            }
            .navigationTitle("Edit Property Dependency")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        onSave(tempDependency)
                        dismiss()
                    }
                    .disabled(tempDependency.sourceProperty.isEmpty || tempDependency.targetProperty.isEmpty || tempDependency.sourceProperty == tempDependency.targetProperty)
                }
            }
        }
        .frame(width: 450, height: 500)
    }
}

// MARK: - Property Presets (TODO-1485)
struct PropertyPresets: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let valueType: PresetValueType
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var presets: [PropertyPreset] = []
    @State private var showPresetEditor = false
    @State private var showPresetManager = false
    @State private var isHovering = false
    @State private var searchFilter = ""
    @State private var selectedCategory: PresetCategory = .all
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum PresetValueType {
        case string
        case integer
        case float
        case boolean
        case vector2
        case vector3
        case vector4
        case color
        case enum(type: Any.Type)
    }
    
    struct PropertyPreset: Identifiable, Codable {
        let id = UUID()
        var name: String
        var description: String
        var category: PresetCategory
        var value: Any
        var tags: [String]
        var isBuiltIn: Bool
        var createdDate: Date
        var lastUsed: Date?
        var usageCount: Int
    }
    
    enum PresetCategory: String, CaseIterable, Codable {
        case all = "All"
        case favorites = "Favorites"
        case custom = "Custom"
        case builtin = "Built-in"
        case recent = "Recent"
        case materials = "Materials"
        case lighting = "Lighting"
        case physics = "Physics"
        case animation = "Animation"
        case audio = "Audio"
        case ui = "UI"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with preset count
            HStack {
                Text("Property Presets")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                Text("\(presets.count) presets")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Button(action: { showPresetEditor = true }) {
                    Image(systemName: "plus.circle.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                
                Button(action: { showPresetManager = true }) {
                    Image(systemName: "gear")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            // Category filter and search
            HStack {
                Picker("Category", selection: $selectedCategory) {
                    ForEach(PresetCategory.allCases, id: \.self) { category in
                        Text(category.rawValue).tag(category)
                    }
                }
                .pickerStyle(.segmented)
                
                Spacer()
                
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    TextField("Search presets...", text: $searchFilter)
                        .textFieldStyle(.roundedBorder)
                        .font(DesignSystem.Typography.small)
                }
            }
            
            // Presets grid
            ScrollView {
                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 2), spacing: 12) {
                    ForEach(filteredPresets) { preset in
                        PresetCardView(
                            preset: preset,
                            currentValueType: valueType,
                            onApply: { applyPreset(preset) },
                            onEdit: { editPreset(preset) },
                            onDelete: { deletePreset(preset) },
                            onToggleFavorite: { toggleFavorite(preset) }
                        )
                    }
                }
                .padding(.horizontal, 4)
            }
            .frame(height: 300)
            
            // Quick actions
            HStack(spacing: 8) {
                Button("Save Current as Preset") {
                    saveCurrentAsPreset()
                }
                .buttonStyle(.bordered)
                .disabled(isLocked)
                
                Button("Reset to Default") {
                    resetToDefault()
                }
                .buttonStyle(.bordered)
                .disabled(defaultValue == nil)
                
                Spacer()
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
            }
        }
        .sheet(isPresented: $showPresetEditor) {
            PresetEditorView(
                preset: PropertyPreset(name: "", description: "", category: .custom, value: value, tags: [], isBuiltIn: false, createdDate: Date(), lastUsed: nil, usageCount: 0),
                valueType: valueType,
                onSave: { newPreset in
                    presets.append(newPreset)
                    onValueChanged?(newPreset.value)
                }
            )
        }
        .sheet(isPresented: $showPresetManager) {
            PresetManagerView(
                presets: $presets,
                valueType: valueType
            )
        }
    }
    
    private var filteredPresets: [PropertyPreset] {
        var filtered = presets
        
        // Filter by category
        if selectedCategory != .all {
            filtered = filtered.filter { $0.category == selectedCategory }
        }
        
        // Filter by search
        if !searchFilter.isEmpty {
            filtered = filtered.filter { preset in
                preset.name.localizedCaseInsensitiveContains(searchFilter) ||
                preset.description.localizedCaseInsensitiveContains(searchFilter) ||
                preset.tags.joined().localizedCaseInsensitiveContains(searchFilter)
            }
        }
        
        // Sort by usage and favorites
        return filtered.sorted { first, second in
            if first.isBuiltIn != second.isBuiltIn {
                return first.isBuiltIn && !second.isBuiltIn
            }
            if (first.category == .favorites) != (second.category == .favorites) {
                return first.category == .favorites
            }
            return first.usageCount > second.usageCount
        }
    }
    
    private func applyPreset(_ preset: PropertyPreset) {
        guard !isLocked else { return }
        
        undoManager.recordState(value)
        value = preset.value
        onValueChanged?(preset.value)
        
        // Update usage statistics
        if let index = presets.firstIndex(where: { $0.id == preset.id }) {
            presets[index].lastUsed = Date()
            presets[index].usageCount += 1
        }
    }
    
    private func saveCurrentAsPreset() {
        guard !isLocked else { return }
        
        let newPreset = PropertyPreset(
            name: "New Preset",
            description: "Custom preset created from current value",
            category: .custom,
            value: value,
            tags: [],
            isBuiltIn: false,
            createdDate: Date(),
            lastUsed: nil,
            usageCount: 0
        )
        
        presets.append(newPreset)
    }
    
    private func editPreset(_ preset: PropertyPreset) {
        // Implementation for editing existing preset
    }
    
    private func deletePreset(_ preset: PropertyPreset) {
        guard !preset.isBuiltIn else { return }
        presets.removeAll { $0.id == preset.id }
    }
    
    private func toggleFavorite(_ preset: PropertyPreset) {
        if let index = presets.firstIndex(where: { $0.id == preset.id }) {
            if presets[index].category == .favorites {
                presets[index].category = .custom
            } else {
                presets[index].category = .favorites
            }
        }
    }
    
    private func resetToDefault() {
        guard let defaultValue = defaultValue, !isLocked else { return }
        undoManager.recordState(value)
        value = defaultValue
        onReset?()
        onValueChanged?(defaultValue)
    }
}

// MARK: - Preset Card View
struct PresetCardView: View {
    let preset: PropertyPresets.PropertyPreset
    let currentValueType: PropertyPresets.PresetValueType
    let onApply: () -> Void
    let onEdit: () -> Void
    let onDelete: () -> Void
    let onToggleFavorite: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with favorite indicator
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text(preset.name)
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .lineLimit(1)
                    
                    Text(preset.category.rawValue)
                        .font(DesignSystem.Typography.caption)
                        .padding(.horizontal, 6)
                        .padding(.vertical, 2)
                        .background(categoryColor.opacity(0.2))
                        .foregroundColor(categoryColor)
                        .cornerRadius(3)
                }
                
                Spacer()
                
                HStack(spacing: 4) {
                    if preset.isBuiltIn {
                        Image(systemName: "lock.fill")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Button(action: onToggleFavorite) {
                        Image(systemName: preset.category == .favorites ? "heart.fill" : "heart")
                            .foregroundColor(preset.category == .favorites ? DesignSystem.Colors.error : DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                }
            }
            
            // Description
            Text(preset.description)
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .lineLimit(2)
            
            // Value preview
            HStack {
                Text("Value:")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Spacer()
                
                Text(formatPresetValue(preset.value))
                    .font(DesignSystem.Typography.caption.monospaced())
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 6)
                    .padding(.vertical, 2)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(3)
            }
            
            // Tags and metadata
            HStack {
                if !preset.tags.isEmpty {
                    ForEach(preset.tags.prefix(3), id: \.self) { tag in
                        Text(tag)
                            .font(DesignSystem.Typography.caption)
                            .padding(.horizontal, 4)
                            .padding(.vertical, 1)
                            .background(DesignSystem.Colors.accentPrimary.opacity(0.2))
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                            .cornerRadius(2)
                    }
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 2) {
                    if let lastUsed = preset.lastUsed {
                        Text("Used: \(lastUsed, style: .relative) ago")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    
                    Text("Used \(preset.usageCount) times")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
            }
            
            // Action buttons
            HStack(spacing: 8) {
                Button("Apply") {
                    onApply()
                }
                .buttonStyle(.borderedProminent)
                
                Button("Edit") {
                    onEdit()
                }
                .buttonStyle(.bordered)
                .disabled(preset.isBuiltIn)
                
                if !preset.isBuiltIn {
                    Button("Delete") {
                        onDelete()
                    }
                    .buttonStyle(.bordered)
                    .foregroundColor(DesignSystem.Colors.error)
                }
            }
        }
        .padding(12)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(8)
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(DesignSystem.Colors.border, lineWidth: 1)
        )
    }
    
    private var categoryColor: Color {
        switch preset.category {
        case .favorites: return DesignSystem.Colors.error
        case .builtin: return DesignSystem.Colors.info
        case .custom: return DesignSystem.Colors.accentPrimary
        default: return DesignSystem.Colors.textSecondary
        }
    }
    
    private func formatPresetValue(_ value: Any) -> String {
        switch currentValueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return String(describing: value)
        }
    }
}

// MARK: - Preset Editor View
struct PresetEditorView: View {
    @Binding var preset: PropertyPresets.PropertyPreset
    let valueType: PropertyPresets.PresetValueType
    let onSave: (PropertyPresets.PropertyPreset) -> Void
    @Environment(\.dismiss) private var dismiss
    
    @State private var tempPreset: PropertyPresets.PropertyPreset
    @State private var selectedTags: Set<String> = []
    
    let availableTags = ["Default", "High Quality", "Performance", "Mobile", "Desktop", "Realistic", "Stylized", "Experimental", "Debug", "Production"]
    
    init(preset: PropertyPresets.PropertyPreset, valueType: PropertyPresets.PresetValueType, onSave: @escaping (PropertyPresets.PropertyPreset) -> Void) {
        self._preset = Binding(get: { preset }, set: { preset = $0 })
        self.valueType = valueType
        self.onSave = onSave
        self._tempPreset = State(initialValue: preset)
        self._selectedTags = State(initialValue: Set(preset.tags))
    }
    
    var body: some View {
        NavigationView {
            Form {
                Section("Preset Information") {
                    TextField("Preset Name", text: $tempPreset.name)
                        .textFieldStyle(.roundedBorder)
                    
                    TextField("Description", text: $tempPreset.description, axis: .vertical)
                        .textFieldStyle(.roundedBorder)
                        .lineLimit(3)
                    
                    Picker("Category", selection: $tempPreset.category) {
                        ForEach(PropertyPresets.PresetCategory.allCases, id: \.self) { category in
                            Text(category.rawValue).tag(category)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                Section("Tags") {
                    LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 3), spacing: 8) {
                        ForEach(availableTags, id: \.self) { tag in
                            Button(action: {
                                if selectedTags.contains(tag) {
                                    selectedTags.remove(tag)
                                } else {
                                    selectedTags.insert(tag)
                                }
                            }) {
                                Text(tag)
                                    .font(DesignSystem.Typography.caption)
                                    .padding(.horizontal, 8)
                                    .padding(.vertical, 4)
                                    .background(selectedTags.contains(tag) ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundSecondary)
                                    .foregroundColor(selectedTags.contains(tag) ? DesignSystem.Colors.backgroundPrimary : DesignSystem.Colors.textPrimary)
                                    .cornerRadius(4)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }
                
                Section("Preview") {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Current Value:")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        
                        Text(formatPresetValue(tempPreset.value))
                            .font(DesignSystem.Typography.body.monospaced())
                            .foregroundColor(DesignSystem.Colors.textPrimary)
                            .padding()
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(4)
                    }
                }
            }
            .navigationTitle("Edit Preset")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Save") {
                        tempPreset.tags = Array(selectedTags)
                        onSave(tempPreset)
                        dismiss()
                    }
                    .disabled(tempPreset.name.isEmpty)
                }
            }
        }
        .frame(width: 500, height: 600)
    }
    
    private func formatPresetValue(_ value: Any) -> String {
        switch valueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return String(describing: value)
        }
    }
}

// MARK: - Preset Manager View
struct PresetManagerView: View {
    @Binding var presets: [PropertyPresets.PropertyPreset]
    let valueType: PropertyPresets.PresetValueType
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedPresets: Set<UUID> = []
    @State private var showingImportExport = false
    @State private var importText = ""
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Toolbar
                HStack {
                    Button("Import") {
                        showingImportExport = true
                        importText = ""
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Export Selected") {
                        exportSelectedPresets()
                    }
                    .buttonStyle(.bordered)
                    .disabled(selectedPresets.isEmpty)
                    
                    Button("Delete Selected") {
                        deleteSelectedPresets()
                    }
                    .buttonStyle(.bordered)
                    .foregroundColor(DesignSystem.Colors.error)
                    .disabled(selectedPresets.isEmpty)
                    
                    Spacer()
                    
                    Text("\(selectedPresets.count) selected")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .padding()
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Presets list
                List {
                    ForEach(presets) { preset in
                        PresetManagerRowView(
                            preset: preset,
                            isSelected: selectedPresets.contains(preset.id),
                            valueType: valueType,
                            onSelect: { togglePresetSelection(preset.id) }
                        )
                    }
                }
                .listStyle(.plain)
            }
            .navigationTitle("Manage Presets")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") { dismiss() }
                }
            }
        }
        .sheet(isPresented: $showingImportExport) {
            ImportExportView(
                importText: $importText,
                presets: presets,
                valueType: valueType,
                onImport: { importedPresets in
                    presets.append(contentsOf: importedPresets)
                },
                onExport: { text in
                    importText = text
                    showingImportExport = true
                }
            )
        }
    }
    
    private func togglePresetSelection(_ presetId: UUID) {
        if selectedPresets.contains(presetId) {
            selectedPresets.remove(presetId)
        } else {
            selectedPresets.insert(presetId)
        }
    }
    
    private func exportSelectedPresets() {
        let selectedPresetsList = presets.filter { selectedPresets.contains($0.id) }
        // Implementation for exporting presets
    }
    
    private func deleteSelectedPresets() {
        presets.removeAll { selectedPresets.contains($0.id) && !$0.isBuiltIn }
        selectedPresets.removeAll()
    }
}

// MARK: - Preset Manager Row View
struct PresetManagerRowView: View {
    let preset: PropertyPresets.PropertyPreset
    let isSelected: Bool
    let valueType: PropertyPresets.PresetValueType
    let onSelect: () -> Void
    
    var body: some View {
        HStack {
            // Selection checkbox
            Button(action: onSelect) {
                Image(systemName: isSelected ? "checkmark.square.fill" : "square")
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            
            // Preset info
            VStack(alignment: .leading, spacing: 2) {
                Text(preset.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(preset.isBuiltIn ? DesignSystem.Colors.textTertiary : DesignSystem.Colors.textPrimary)
                
                Text(preset.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .lineLimit(1)
                
                HStack {
                    Text(formatPresetValue(preset.value))
                        .font(DesignSystem.Typography.caption.monospaced())
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                    
                    if preset.isBuiltIn {
                        Image(systemName: "lock.fill")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                }
            }
        }
        .padding(.vertical, 4)
        .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : Color.clear)
        .contentShape(Rectangle())
    }
    
    private func formatPresetValue(_ value: Any) -> String {
        switch valueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return String(describing: value)
        }
    }
}

// MARK: - Import/Export View
struct ImportExportView: View {
    @Binding var importText: String
    let presets: [PropertyPresets.PropertyPreset]
    let valueType: PropertyPresets.PresetValueType
    let onImport: ([PropertyPresets.PropertyPreset]) -> Void
    let onExport: (String) -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        NavigationView {
            VStack {
                Picker("Mode", selection: Binding(
                    get: { importText.isEmpty ? 0 : 1 },
                    set: { _ in }
                )) {
                    Text("Import").tag(0)
                    Text("Export").tag(1)
                }
                .pickerStyle(.segmented)
                .padding()
                
                if importText.isEmpty {
                    // Export mode
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Export all presets as JSON:")
                            .font(DesignSystem.Typography.headline)
                        
                        TextEditor(text: Binding(
                            get: { exportAllPresetsAsJSON() },
                            set: { _ in }
                        ))
                        .font(DesignSystem.Typography.body.monospaced())
                        .frame(minHeight: 200)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundPrimary)
                        .cornerRadius(8)
                        .overlay(
                            RoundedRectangle(cornerRadius: 8)
                                .stroke(DesignSystem.Colors.border, lineWidth: 1)
                        )
                    }
                    
                    HStack {
                        Button("Copy to Clipboard") {
                            onExport(exportAllPresetsAsJSON())
                        }
                        .buttonStyle(.borderedProminent)
                        
                        Spacer()
                        
                        Button("Share") {
                            // Share functionality
                        }
                        .buttonStyle(.bordered)
                    }
                }
                } else {
                    // Import mode
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Import presets from JSON:")
                            .font(DesignSystem.Typography.headline)
                        
                        TextEditor(text: $importText)
                            .font(DesignSystem.Typography.body.monospaced())
                            .frame(minHeight: 200)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundPrimary)
                            .cornerRadius(8)
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(DesignSystem.Colors.border, lineWidth: 1)
                            )
                        
                        if !importText.isEmpty && isValidJSON(importText) {
                            HStack {
                                Text("Valid JSON")
                                    .font(DesignSystem.Typography.small)
                                    .foregroundColor(DesignSystem.Colors.success)
                                
                                Spacer()
                                
                                Button("Import") {
                                    importPresetsFromJSON()
                                }
                                .buttonStyle(.borderedProminent)
                            }
                        }
                    }
                }
            }
            .padding()
            .navigationTitle("Import/Export Presets")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") { dismiss() }
                }
            }
        }
        .frame(width: 600, height: 500)
    }
    
    private func exportAllPresetsAsJSON() -> String {
        let encoder = JSONEncoder()
        encoder.dateEncodingStrategy = .iso8601
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        
        do {
            let data = try encoder.encode(presets)
            return String(data: data, encoding: .utf8) ?? "Export failed"
        } catch {
            return "Export failed: \(error.localizedDescription)"
        }
    }
    
    private func isValidJSON(_ string: String) -> Bool {
        let data = string.data(using: .utf8) ?? Data()
        return (try? JSONSerialization.jsonObject(with: data)) != nil
    }
    
    private func importPresetsFromJSON() {
        guard let data = importText.data(using: .utf8) else { return }
        
        do {
            let importedPresets = try JSONDecoder().decode([PropertyPresets.PropertyPreset].self, from: data)
            onImport(importedPresets)
            dismiss()
        } catch {
            // Handle import error
        }
    }
}

// MARK: - Property History (TODO-1486)
struct PropertyHistory: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let valueType: HistoryValueType
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var history: [HistoryEntry] = []
    @State private var maxHistorySize: Int = 50
    @State private var showHistoryManager = false
    @State private var searchFilter = ""
    @State private var selectedCategory: HistoryCategory = .all
    @State private var isHovering = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum HistoryValueType {
        case string
        case integer
        case float
        case boolean
        case vector2
        case vector3
        case vector4
        case color
        case enum(type: Any.Type)
    }
    
    struct HistoryEntry: Identifiable, Codable {
        let id = UUID()
        let value: Any
        let timestamp: Date
        let source: HistorySource
        let tags: [String]
        let description: String
        
        enum HistorySource: String, Codable {
            case manual = "Manual"
            case preset = "Preset"
            case script = "Script"
            case random = "Random"
            case math = "Math"
            case import = "Import"
            case undo = "Undo"
            case redo = "Redo"
        }
    }
    
    enum HistoryCategory: String, CaseIterable, Codable {
        case all = "All"
        case manual = "Manual"
        case preset = "Presets"
        case script = "Scripts"
        case random = "Random"
        case math = "Math"
        case recent = "Recent"
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with history count
            HStack {
                Text("Property History")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                Text("\(history.count) entries")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Button(action: { showHistoryManager = true }) {
                    Image(systemName: "gear")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            // Category filter and search
            HStack {
                Picker("Category", selection: $selectedCategory) {
                    ForEach(HistoryCategory.allCases, id: \.self) { category in
                        Text(category.rawValue).tag(category)
                    }
                }
                .pickerStyle(.segmented)
                
                Spacer()
                
                HStack {
                    Image(systemName: "magnifyingglass")
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    TextField("Search history...", text: $searchFilter)
                        .textFieldStyle(.roundedBorder)
                        .font(DesignSystem.Typography.small)
                }
            }
            
            // History list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach(filteredHistory) { entry in
                        HistoryEntryRowView(
                            entry: entry,
                            currentValueType: valueType,
                            onApply: { applyHistoryEntry(entry) },
                            onDelete: { deleteHistoryEntry(entry) },
                            onCopy: { copyHistoryEntry(entry) }
                        )
                    }
                }
            }
            .frame(height: 300)
            
            // Quick actions
            HStack(spacing: 8) {
                Button("Clear History") {
                    clearHistory()
                }
                .buttonStyle(.bordered)
                .foregroundColor(DesignSystem.Colors.error)
                
                Button("Export History") {
                    exportHistory()
                }
                .buttonStyle(.bordered)
                
                Spacer()
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
            }
        }
        .sheet(isPresented: $showHistoryManager) {
            HistoryManagerView(
                history: $history,
                valueType: valueType
            )
        }
        .onAppear {
            loadHistory()
        }
        .onChange(of: value) { oldValue, newValue in
            addToHistory(newValue, source: .manual)
        }
    }
    
    private var filteredHistory: [HistoryEntry] {
        var filtered = history
        
        // Filter by category
        if selectedCategory != .all {
            filtered = filtered.filter { $0.source.rawValue == selectedCategory.rawValue }
        }
        
        // Filter by search
        if !searchFilter.isEmpty {
            filtered = filtered.filter { entry in
                formatHistoryValue(entry.value).localizedCaseInsensitiveContains(searchFilter) ||
                entry.description.localizedCaseInsensitiveContains(searchFilter) ||
                entry.tags.joined().localizedCaseInsensitiveContains(searchFilter)
            }
        }
        
        // Sort by timestamp (most recent first)
        return filtered.sorted { $0.timestamp > $1.timestamp }
    }
    
    private func addToHistory(_ newValue: Any, source: HistoryEntry.HistorySource) {
        guard !isLocked else { return }
        
        let entry = HistoryEntry(
            value: newValue,
            timestamp: Date(),
            source: source,
            tags: [],
            description: "Value changed to \(formatHistoryValue(newValue))"
        )
        
        history.insert(entry, at: 0)
        
        // Maintain max history size
        if history.count > maxHistorySize {
            history = Array(history.prefix(maxHistorySize))
        }
        
        saveHistory()
    }
    
    private func applyHistoryEntry(_ entry: HistoryEntry) {
        guard !isLocked else { return }
        
        undoManager.recordState(value)
        value = entry.value
        onValueChanged?(entry.value)
        
        // Update usage timestamp
        if let index = history.firstIndex(where: { $0.id == entry.id }) {
            history[index] = HistoryEntry(
                id: entry.id,
                value: entry.value,
                timestamp: Date(),
                source: entry.source,
                tags: entry.tags,
                description: entry.description
            )
        }
    }
    
    private func deleteHistoryEntry(_ entry: HistoryEntry) {
        history.removeAll { $0.id == entry.id }
        saveHistory()
    }
    
    private func copyHistoryEntry(_ entry: HistoryEntry) {
        clipboardManager.copy(formatHistoryValue(entry.value))
    }
    
    private func clearHistory() {
        history.removeAll()
        saveHistory()
    }
    
    private func exportHistory() {
        let encoder = JSONEncoder()
        encoder.dateEncodingStrategy = .iso8601
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        
        do {
            let data = try encoder.encode(history)
            if let jsonString = String(data: data, encoding: .utf8) {
                clipboardManager.copy(jsonString)
            }
        } catch {
            // Handle export error
        }
    }
    
    private func saveHistory() {
        // Implementation for persisting history
        // In real implementation, this would save to UserDefaults or a file
    }
    
    private func loadHistory() {
        // Implementation for loading history
        // In real implementation, this would load from UserDefaults or a file
    }
    
    private func formatHistoryValue(_ value: Any) -> String {
        switch valueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return String(describing: value)
        }
    }
}

// MARK: - History Entry Row View
struct HistoryEntryRowView: View {
    let entry: PropertyHistory.HistoryEntry
    let currentValueType: PropertyHistory.HistoryValueType
    let onApply: () -> Void
    let onDelete: () -> Void
    let onCopy: () -> Void
    
    var body: some View {
        HStack {
            // Source indicator
            VStack(spacing: 2) {
                Circle()
                    .fill(sourceColor)
                    .frame(width: 8, height: 8)
                
                Text(entry.source.rawValue)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(sourceColor)
            }
            
            // Value and description
            VStack(alignment: .leading, spacing: 2) {
                Text(formatHistoryValue(entry.value))
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                
                Text(entry.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .lineLimit(2)
            }
            
            Spacer()
            
            // Timestamp and actions
            VStack(alignment: .trailing, spacing: 4) {
                Text(entry.timestamp, style: .relative) ago)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                HStack(spacing: 4) {
                    Button(action: onApply) {
                        Image(systemName: "arrow.counterclockwise")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                    }
                    .buttonStyle(.plain)
                    .help("Apply this value")
                    
                    Button(action: onCopy) {
                        Image(systemName: "doc.on.doc")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .buttonStyle(.plain)
                    .help("Copy value to clipboard")
                    
                    Button(action: onDelete) {
                        Image(systemName: "trash")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.error)
                    }
                    .buttonStyle(.plain)
                    .help("Delete this entry")
                }
            }
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
    
    private var sourceColor: Color {
        switch entry.source {
        case .manual: return DesignSystem.Colors.info
        case .preset: return DesignSystem.Colors.accentPrimary
        case .script: return DesignSystem.Colors.warning
        case .random: return DesignSystem.Colors.success
        case .math: return DesignSystem.Colors.error
        case .import: return DesignSystem.Colors.textSecondary
        case .undo: return DesignSystem.Colors.textTertiary
        case .redo: return DesignSystem.Colors.textTertiary
        }
    }
    
    private func formatHistoryValue(_ value: Any) -> String {
        switch currentValueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return String(describing: value)
        }
    }
}

// MARK: - History Manager View
struct HistoryManagerView: View {
    @Binding var history: [PropertyHistory.HistoryEntry]
    let valueType: PropertyHistory.HistoryValueType
    @Environment(\.dismiss) private var dismiss
    
    @State private var selectedEntries: Set<UUID> = []
    @State private var showingExport = false
    @State private var exportText = ""
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Toolbar
                HStack {
                    Text("\(history.count) history entries")
                        .font(DesignSystem.Typography.headline)
                    
                    Spacer()
                    
                    Button("Export Selected") {
                        exportSelectedHistory()
                    }
                    .buttonStyle(.bordered)
                    .disabled(selectedEntries.isEmpty)
                    
                    Button("Delete Selected") {
                        deleteSelectedHistory()
                    }
                    .buttonStyle(.bordered)
                    .foregroundColor(DesignSystem.Colors.error)
                    .disabled(selectedEntries.isEmpty)
                    
                    Spacer()
                    
                    Text("\(selectedEntries.count) selected")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .padding()
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // History list
                List {
                    ForEach(history) { entry in
                        HistoryManagerRowView(
                            entry: entry,
                            isSelected: selectedEntries.contains(entry.id),
                            valueType: valueType,
                            onSelect: { toggleEntrySelection(entry.id) }
                        )
                    }
                }
                .listStyle(.plain)
            }
        }
        .navigationTitle("Manage History")
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
            ToolbarItem(placement: .navigationBarTrailing) {
                Button("Done") { dismiss() }
            }
        }
        .sheet(isPresented: $showingExport) {
            ExportView(
                exportText: $exportText,
                history: history,
                onExport: { text in
                    exportText = text
                    showingExport = true
                }
            )
        }
    }
    
    private func toggleEntrySelection(_ entryId: UUID) {
        if selectedEntries.contains(entryId) {
            selectedEntries.remove(entryId)
        } else {
            selectedEntries.insert(entryId)
        }
    }
    
    private func exportSelectedHistory() {
        let selectedEntriesList = history.filter { selectedEntries.contains($0.id) }
        // Implementation for exporting selected entries
    }
    
    private func deleteSelectedHistory() {
        history.removeAll { selectedEntries.contains($0.id) }
        selectedEntries.removeAll()
    }
}

// MARK: - History Manager Row View
struct HistoryManagerRowView: View {
    let entry: PropertyHistory.HistoryEntry
    let isSelected: Bool
    let valueType: PropertyHistory.HistoryValueType
    let onSelect: () -> Void
    
    var body: some View {
        HStack {
            // Selection checkbox
            Button(action: onSelect) {
                Image(systemName: isSelected ? "checkmark.square.fill" : "square")
                    .foregroundColor(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            
            // Entry info
            VStack(alignment: .leading, spacing: 2) {
                Text(formatHistoryValue(entry.value))
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                
                Text(entry.description)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .lineLimit(2)
                
                HStack {
                    Text(entry.timestamp, style: .relative) ago)
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Spacer()
                    
                    Text(entry.source.rawValue)
                        .font(DesignSystem.Typography.caption)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(sourceColor.opacity(0.2))
                        .foregroundColor(sourceColor)
                        .cornerRadius(2)
                }
            }
        }
        .padding(.vertical, 4)
        .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : Color.clear)
        .contentShape(Rectangle())
    }
    
    private var sourceColor: Color {
        switch entry.source {
        case .manual: return DesignSystem.Colors.info
        case .preset: return DesignSystem.Colors.accentPrimary
        case .script: return DesignSystem.Colors.warning
        case .random: return DesignSystem.Colors.success
        case .math: return DesignSystem.Colors.error
        case .import: return DesignSystem.Colors.textSecondary
        case .undo: return DesignSystem.Colors.textTertiary
        case .redo: return DesignSystem.Colors.textTertiary
        }
    }
    
    private func formatHistoryValue(_ value: Any) -> String {
        switch valueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return String(describing: value)
        }
    }
}

// MARK: - Export View
struct ExportView: View {
    @Binding var exportText: String
    let history: [PropertyHistory.HistoryEntry]
    let onExport: (String) -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        NavigationView {
            VStack {
                Text("Export History")
                    .font(DesignSystem.Typography.headline)
                    .padding()
                
                TextEditor(text: $exportText)
                    .font(DesignSystem.Typography.body.monospaced())
                    .frame(minHeight: 300)
                    .padding(8)
                    .background(DesignSystem.Colors.backgroundPrimary)
                    .cornerRadius(8)
                    .overlay(
                        RoundedRectangle(cornerRadius: 8)
                            .stroke(DesignSystem.Colors.border, lineWidth: 1)
                    )
                
                HStack {
                    Spacer()
                    
                    Button("Copy to Clipboard") {
                        onExport(exportText)
                    }
                    .buttonStyle(.borderedProminent)
                    
                    Button("Share") {
                        // Share functionality
                    }
                    .buttonStyle(.bordered)
                }
            }
            .padding()
            .navigationTitle("Export History")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Export") {
                        onExport(exportText)
                        dismiss()
                    }
                }
            }
        }
        .frame(width: 600, height: 500)
    }
}

// MARK: - Property Random Value Generation (TODO-1487)
struct PropertyRandomValue: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let valueType: RandomValueType
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var randomType: RandomType = .uniform
    @State private var minRange: Double = 0
    @State private var maxRange: Double = 1
    @State private var seed: UInt32 = 12345
    @State private var isGenerating: Bool = false
    @State private var lastGeneratedValue: Any?
    @State private var generationHistory: [RandomGenerationEntry] = []
    @State private var showSettings = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum RandomValueType {
        case string
        case integer
        case float
        case boolean
        case vector2
        case vector3
        case vector4
        case color
        case enum(type: Any.Type)
    }
    
    enum RandomType: String, CaseIterable, Codable {
        case uniform = "Uniform"
        case normal = "Normal"
        case exponential = "Exponential"
        case gaussian = "Gaussian"
        case perlin = "Perlin"
        case custom = "Custom"
    }
    
    struct RandomGenerationEntry: Identifiable, Codable {
        let id = UUID()
        let value: Any
        let timestamp: Date
        let type: RandomType
        let seed: UInt32
        let parameters: [String: Any]
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with generation status
            HStack {
                Text("Random Value Generation")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                if isGenerating {
                    HStack(spacing: 4) {
                        ProgressView()
                            .scaleEffect(DesignSystem.Colors.accentPrimary)
                        
                        Text("Generating...")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                } else {
                    Text("Random Value Generator")
                        .font(DesignSystem.Typography.headline)
                    
                    Spacer()
                    
                    Text("Ready")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.success)
                }
                
                Button(action: { showSettings = true }) {
                    Image(systemName: "gear")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            // Current value display
            HStack {
                Text("Current Value:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text(formatRandomValue(value))
                    .font(DesignSystem.Typography.body.monospaced())
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(4)
                
                Button(action: { generateRandomValue() }) {
                    Image(systemName: "dice.fill")
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.borderedProminent)
                .disabled(isLocked)
            }
            
            // Random type selection
            HStack {
                Text("Distribution:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("Type", selection: $randomType) {
                    ForEach(RandomType.allCases, id: \.self) { type in
                        Text(type.rawValue).tag(type)
                    }
                }
                .pickerStyle(.segmented)
                .onChange(of: randomType) { _ in
                    updateSeed()
                }
            }
            
            // Range controls
            if randomType == .uniform || randomType == .normal {
                HStack {
                    Text("Range:")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    HStack {
                        Text("Min:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("Min", value: Binding(
                            get: { minRange },
                            set: { newValue in
                                minRange = max(minRange, newValue)
                            }
                        ), format: .number)
                        .textFieldStyle(.roundedBorder)
                        
                        Text("Max:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("Max", value: Binding(
                            get: { maxRange },
                            set: { newValue in
                                maxRange = max(minRange, newValue)
                            }
                        ), format: .number)
                        .textFieldStyle(.roundedBorder)
                    }
                }
            }
            
            // Seed control
            HStack {
                Text("Seed:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                TextField("Seed", value: Binding(
                    get: { seed },
                    set: { newValue in
                        seed = UInt32(newValue) ?? 0
                        updateSeed()
                    }
                ), format: .number)
                        .textFieldStyle(.roundedBorder)
                
                Button(action: { seed = UInt32.random(in: 0...UInt32.max) }) {
                    Image(systemName: "arrow.clockwise")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .help("Generate new seed")
            }
            
            // Generation history
            if !generationHistory.isEmpty {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Recent Generations")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    ScrollView {
                        LazyVStack(spacing: 2) {
                            ForEach(generationHistory.reversed()) { entry in
                                GenerationHistoryRowView(entry: entry)
                            }
                        }
                    }
                    .frame(height: 100)
                }
            }
            
            // Quick actions
            HStack(spacing: 8) {
                Button("Generate Random") {
                    generateRandomValue()
                }
                .buttonStyle(.borderedProminent)
                .disabled(isLocked)
                
                Button("Generate Multiple") {
                    generateMultipleRandomValues()
                }
                .buttonStyle(.bordered)
                .disabled(isLocked)
                
                Spacer()
                
                Button("Clear History") {
                    clearGenerationHistory()
                }
                .buttonStyle(.bordered)
                .foregroundColor(DesignSystem.Colors.error)
                
                Spacer()
                
                Button("Settings") {
                    showSettings = true
                }
                .buttonStyle(.plain)
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
            }
        }
        .sheet(isPresented: $showSettings) {
            RandomSettingsView(
                randomType: $randomType,
                minRange: $minRange,
                maxRange: $maxRange,
                seed: $seed,
                onSettingsChanged: { updateSeed() }
            )
        }
        .onAppear {
            loadGenerationHistory()
        }
        .onChange(of: value) { oldValue, newValue in
            // Auto-generate on value change if enabled
            if !isLocked {
                generateRandomValue()
            }
        }
    }
    
    private func generateRandomValue() {
        guard !isLocked else { return }
        
        isGenerating = true
        
        DispatchQueue.global().asyncAfter(deadline: .now() + 0.1) {
            let newValue = generateRandomValueOfType()
            
            DispatchQueue.main.async {
                isGenerating = false
                undoManager.recordState(value)
                value = newValue
                onValueChanged?(newValue)
                
                addToGenerationHistory(newValue, type: randomType)
                lastGeneratedValue = newValue
            }
        }
    }
    
    private func generateMultipleRandomValues() {
        guard !isLocked else { return }
        
        isGenerating = true
        
        DispatchQueue.global().asyncAfter(deadline: .now() + 0.1) {
            let values = (0..<5).map { _ in
                generateRandomValueOfType()
            }
            
            DispatchQueue.main.async {
                isGenerating = false
                undoManager.recordState(value)
                value = values.first ?? value
                onValueChanged?(value)
                
                addToGenerationHistory(values, type: randomType)
                lastGeneratedValue = values.first
            }
        }
    }
    
    private func generateRandomValueOfType() -> Any {
        switch valueType {
        case .string:
            return generateRandomString()
        case .integer:
            return Int.random(in: Int(minRange)...Int(maxRange))
        case .float:
            return Float.random(in: minRange...maxRange)
        case .boolean:
            return Bool.random()
        case .vector2:
            return SIMD2<Float>(
                Float.random(in: minRange...maxRange),
                Float.random(in: minRange...maxRange)
            )
        case .vector3:
            return SIMD3<Float>(
                Float.random(in: minRange...maxRange),
                Float.random(in: minRange...maxRange),
                Float.random(in: minRange...maxRange)
            )
        case .vector4:
            return SIMD4<Float>(
                Float.random(in: minRange...maxRange),
                Float.random(in: minRange...maxRange),
                Float.random(in: minRange...maxRange),
                Float.random(in: minRange...maxRange)
            )
        case .color:
            return generateRandomColor()
        case .enum:
            // Would need enum type information
            return "Random Enum Value"
        }
    }
    
    private func generateRandomString() -> String {
        let characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567890"
        let length = Int.random(in: 3...16)
        return String((0..<length).compactMap { _ in characters.randomElement() })
    }
    
    private func generateRandomColor() -> Color {
        return Color(
            red: Double.random(in: 0...1),
            green: Double.random(in: 0...1),
            blue: Double.random(in: 0...1),
            alpha: Double.random(in: 0...1)
        )
    }
    
    private func updateSeed() {
        // Seed-based random number generation
        seed = (seed * 110351524527 + 12345) % UInt32.max
    }
    
    private func addToGenerationHistory(_ value: Any, type: RandomType) {
        let entry = RandomGenerationEntry(
            value: value,
            timestamp: Date(),
            type: type,
            seed: seed,
            parameters: [
                "type": type.rawValue,
                "minRange": String(minRange),
                "maxRange": String(maxRange),
                "seed": String(seed)
            ]
        )
        
        generationHistory.insert(entry, at: 0)
        
        // Maintain max history size
        if generationHistory.count > 20 {
            generationHistory = Array(generationHistory.prefix(20))
        }
        
        saveGenerationHistory()
    }
    
    private func clearGenerationHistory() {
        generationHistory.removeAll()
        saveGenerationHistory()
    }
    
    private func saveGenerationHistory() {
        // Implementation for persisting generation history
        // In real implementation, this would save to UserDefaults or a file
    }
    
    private func loadGenerationHistory() {
        // Implementation for loading generation history
        // In real implementation, this would load from UserDefaults or a file
    }
}

// MARK: - Generation History Row View
struct GenerationHistoryRowView: View {
    let entry: PropertyRandomValue.RandomGenerationEntry
    let valueType: PropertyRandomValue.RandomValueType
    
    var body: some View {
        HStack {
            // Type indicator
            VStack(spacing: 2) {
                Circle()
                    .fill(typeColor)
                    .frame(width: 8, height: 8)
                
                Text(entry.type.rawValue)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(typeColor)
            }
            
            // Value and metadata
            VStack(alignment: .leading, spacing: 2) {
                Text(formatRandomValue(entry.value))
                    .font(DesignSystem.Typography.body.monospaced())
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                
                Text(entry.timestamp, style: .relative) ago)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(GenerationHistoryRowView.typeColor)
                
                HStack {
                    Text("Seed: \(entry.seed)")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    
                    Spacer()
                    
                    if entry.parameters.count > 0 {
                        Text("Params: \(entry.parameters.joined(separator: ", "))")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(GenerationHistoryRowView.typeColor)
                    }
                }
            }
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
    
    private var typeColor: Color {
        switch entry.type {
        case .uniform: return DesignSystem.Colors.info
        case .normal: return DesignSystem.Colors.accentPrimary
        case .exponential: return DesignSystem.Colors.warning
        case .gaussian: return DesignSystem.Colors.success
        case .perlin: return DesignSystem.Colors.textSecondary
        case .custom: return DesignSystem.Colors.error
        }
    }
    
    private func formatRandomValue(_ value: Any) -> String {
        switch valueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return "Random Enum Value"
        }
    }
}

// MARK: - Random Settings View
struct RandomSettingsView: View {
    @Binding var randomType: PropertyRandomValue.RandomType
    @Binding var minRange: Double
    @Binding var maxRange: Double
    @Binding var seed: UInt32
    let onSettingsChanged: () -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        NavigationView {
            Form {
                Section("Random Generation Settings") {
                    Text("Distribution Type")
                        .font(DesignSystem.Typography.headline)
                    
                    Picker("Type", selection: $randomType) {
                        ForEach(PropertyRandomValue.RandomType.allCases, id: \.self) { type in
                            Text(type.rawValue).tag(type)
                        }
                    }
                    .pickerStyle(.menu)
                }
                
                Section("Range Settings") {
                    HStack {
                        Text("Minimum:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("Min", value: $minRange, format: .number)
                            .textFieldStyle(.roundedBorder)
                        
                        Text("Maximum:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("Max", value: $maxRange, format: .number)
                            .textFieldStyle(.roundedBorder)
                    }
                }
                
                Section("Seed Settings") {
                    HStack {
                        Text("Seed:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        TextField("Seed", value: $seed, format: .number)
                            .textFieldStyle(.roundedBorder)
                        
                        Button(action: { 
                            seed = UInt32.random(in: 0...UInt32.max)
                            onSettingsChanged()
                        }) {
                            Image(systemName: "arrow.clockwise")
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        .help("Generate new seed")
                    }
                    .buttonStyle(.plain)
                }
                
                Section("Advanced Options") {
                    Toggle("Auto-generate on value change", isOn: Binding(
                        get: { false },
                        set: { newValue in
                            // Auto-generate logic would go here
                        }
                    ))
                    
                    Text("Batch Generation")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Stepper("Count", in: 1...10, value: Binding(
                        get: { 1 },
                        set: { newValue in
                            // Batch generation logic would go here
                        }
                    ))
                }
                }
            }
            .navigationTitle("Random Settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") { dismiss() }
                }
            }
        }
        .frame(width: 400, height: 500)
    }
}

// MARK: - Property Math Operations (TODO-1488)
struct PropertyMathOperations: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let valueType: MathValueType
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var mathOperation: MathOperation = .add
    @State private var operandValue: Any = 0
    @State private var operationHistory: [MathOperationEntry] = []
    @State private var showCalculator = false
    @State private var isCalculating: Bool = false
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum MathValueType {
        case string
        case integer
        case float
        case boolean
        case vector2
        case vector3
        case vector4
        case color
        case enum(type: Any.Type)
    }
    
    enum MathOperation: String, CaseIterable, Codable {
        case add = "Add"
        case subtract = "Subtract"
        case multiply = "Multiply"
        case divide = "Divide"
        case power = "Power"
        case sqrt = "Square Root"
        case abs = "Absolute"
        case min = "Min"
        case max = "Max"
        case clamp = "Clamp"
        case lerp = "Lerp"
        case normalize = "Normalize"
        case dot = "Dot Product"
        case cross = "Cross Product"
        case distance = "Distance"
        case angle = "Angle"
        case modulo = "Modulo"
        case floor = "Floor"
        case ceil = "Ceiling"
        case round = "Round"
        case sin = "Sine"
        case cos = "Cosine"
        case tan = "Tangent"
        case log = "Logarithm"
        case exp = "Exponential"
    }
    
    struct MathOperationEntry: Identifiable, Codable {
        let id = UUID()
        let operation: MathOperation
        let originalValue: Any
        let operandValue: Any
        let resultValue: Any
        let timestamp: Date
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with operation status
            HStack {
                Text("Math Operations")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                if isCalculating {
                    HStack(spacing: 4) {
                        ProgressView()
                            .scaleEffect(0.8)
                        
                        Text("Calculating...")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                } else {
                    Text("Math Calculator")
                        .font(DesignSystem.Typography.headline)
                    
                    Spacer()
                    
                    Text("Ready")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.success)
                }
                
                Button(action: { showCalculator = true }) {
                    Image(systemName: "calculator")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            // Current value display
            HStack {
                Text("Current Value:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text(formatMathValue(value))
                    .font(DesignSystem.Typography.body.monospaced())
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(4)
            }
            
            // Operation selection
            HStack {
                Text("Operation:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("Operation", selection: $mathOperation) {
                    ForEach(MathOperation.allCases, id: \.self) { operation in
                        Text(operation.rawValue).tag(operation)
                    }
                }
                .pickerStyle(.menu)
                .onChange(of: mathOperation) { _ in
                    updateOperandValue()
                }
            }
            
            // Operation buttons
            HStack(spacing: 8) {
                Button("Apply Operation") {
                    applyMathOperation()
                }
                .buttonStyle(.borderedProminent)
                .disabled(isLocked || !canApplyOperation)
                
                Button("Clear History") {
                    clearOperationHistory()
                }
                .buttonStyle(.bordered)
                .foregroundColor(DesignSystem.Colors.error)
                
                Spacer()
                
                if let tooltip = tooltip {
                    Button(action: {}) {
                        Image(systemName: "info.circle")
                            .font(.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .buttonStyle(.plain)
                    .popover(isPresented: .constant(isHovering)) {
                        Text(tooltip)
                            .font(DesignSystem.Typography.caption)
                            .padding(8)
                            .background(DesignSystem.Colors.backgroundSecondary)
                            .cornerRadius(6)
                    }
                    .onHover { hovering in
                        isHovering = hovering
                    }
                }
            }
        }
        .sheet(isPresented: $showCalculator) {
            MathCalculatorView(
                currentValue: value,
                valueType: valueType,
                onResult: { result in
                    undoManager.recordState(value)
                    value = result
                    onValueChanged?(result)
                }
            )
        }
        .onAppear {
            loadOperationHistory()
        }
    }
    
    private var canApplyOperation: Bool {
        if isLocked { return false }
        return true
    }
    
    private func applyMathOperation() {
        guard !isLocked, canApplyOperation else { return }
        
        isCalculating = true
        
        DispatchQueue.global().asyncAfter(deadline: .now() + 0.1) {
            let result = performMathOperation()
            
            DispatchQueue.main.async {
                isCalculating = false
                undoManager.recordState(value)
                value = result
                onValueChanged?(result)
                
                addToOperationHistory(result)
            }
        }
    }
    
    private func performMathOperation() -> Any {
        switch valueType {
        case .integer:
            return performIntegerMath()
        case .float:
            return performFloatMath()
        default:
            return value
        }
    }
    
    private func performIntegerMath() -> Any {
        let current = value as? Int ?? 0
        
        switch mathOperation {
        case .add: return current + 1
        case .subtract: return current - 1
        case .multiply: return current * 2
        case .divide: return current / 2
        case .power: return Int(pow(Double(current), 2))
        case .sqrt: return Int(sqrt(Double(current)))
        case .abs: return abs(current)
        case .min: return min(current, 100)
        case .max: return max(current, 0)
        case .modulo: return current % 10
        case .floor: return Int(floor(Double(current)))
        case .ceil: return Int(ceil(Double(current)))
        case .round: return Int(round(Double(current)))
        default: return current
        }
    }
    
    private func performFloatMath() -> Any {
        let current = value as? Float ?? 0.0
        
        switch mathOperation {
        case .add: return current + 1.0
        case .subtract: return current - 1.0
        case .multiply: return current * 2.0
        case .divide: return current / 2.0
        case .power: return pow(current, 2.0)
        case .sqrt: return sqrt(current)
        case .abs: return abs(current)
        case .min: return min(current, 100.0)
        case .max: return max(current, 0.0)
        case .modulo: return current.truncatingRemainder(dividingBy: 10.0)
        case .floor: return floor(current)
        case .ceil: return ceil(current)
        case .round: return round(current)
        case .sin: return sin(current)
        case .cos: return cos(current)
        case .tan: return tan(current)
        case .log: return current > 0 ? log(current) : 0
        case .exp: return exp(current)
        default: return current
        }
    }
    
    private func updateOperandValue() {
        switch valueType {
        case .integer:
            operandValue = 0
        case .float:
            operandValue = 0.0
        default:
            operandValue = 0
        }
    }
    
    private func addToOperationHistory(_ result: Any) {
        let entry = MathOperationEntry(
            operation: mathOperation,
            originalValue: value,
            operandValue: operandValue,
            resultValue: result,
            timestamp: Date()
        )
        
        operationHistory.insert(entry, at: 0)
        
        if operationHistory.count > 20 {
            operationHistory = Array(operationHistory.prefix(20))
        }
        
        saveOperationHistory()
    }
    
    private func clearOperationHistory() {
        operationHistory.removeAll()
        saveOperationHistory()
    }
    
    private func saveOperationHistory() {
        // Implementation for persisting operation history
    }
    
    private func loadOperationHistory() {
        // Implementation for loading operation history
    }
    
    private func formatMathValue(_ value: Any) -> String {
        switch valueType {
        case .string:
            return value as? String ?? "Invalid"
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .boolean:
            return (value as? Bool ?? false).description
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        case .enum:
            return String(describing: value)
        }
    }
}

// MARK: - Math Calculator View
struct MathCalculatorView: View {
    let currentValue: Any
    let valueType: PropertyMathOperations.MathValueType
    let onResult: (Any) -> Void
    @Environment(\.dismiss) private var dismiss
    
    @State private var displayValue: String = ""
    @State private var currentOperation: String = ""
    
    var body: some View {
        NavigationView {
            VStack(spacing: 16) {
                // Display
                Text(displayValue.isEmpty ? formatValue(currentValue) : displayValue)
                    .font(DesignSystem.Typography.largeTitle.monospaced())
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding()
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(8)
                
                // Operation display
                Text(currentOperation)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                    .lineLimit(2)
                
                // Calculator buttons
                calculatorButtons
                
                Spacer()
            }
            .padding()
            .navigationTitle("Math Calculator")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Apply") {
                        if !displayValue.isEmpty {
                            onResult(parseDisplayValue())
                            dismiss()
                        }
                    }
                    .disabled(displayValue.isEmpty)
                }
            }
        }
        .frame(width: 400, height: 600)
    }
    
    @ViewBuilder
    private var calculatorButtons: some View {
        LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 4), spacing: 8) {
            // Numbers and basic operations
            ForEach(0..<9) { index in
                Button("\(index + 1)") {
                    appendToDisplay("\(index + 1)")
                }
                .buttonStyle(.bordered)
            }
            
            Button("C") {
                clearDisplay()
            }
            .buttonStyle(.bordered)
            .foregroundColor(DesignSystem.Colors.error)
            
            Button("0") {
                appendToDisplay("0")
            }
            .buttonStyle(.bordered)
            
            Button("=") {
                calculateResult()
            }
            .buttonStyle(.borderedProminent)
            
            // Math operations
            Button("+") {
                appendOperation("+")
            }
            .buttonStyle(.bordered)
            
            Button("-") {
                appendOperation("-")
            }
            .buttonStyle(.bordered)
            
            Button("*") {
                appendOperation("*")
            }
            .buttonStyle(.bordered)
            
            Button("/") {
                appendOperation("/")
            }
            .buttonStyle(.bordered)
            
            // Special functions
            Button("√") {
                appendOperation("sqrt")
            }
            .buttonStyle(.bordered)
            
            Button("|x|") {
                appendOperation("abs")
            }
            .buttonStyle(.bordered)
            
            Button("sin") {
                appendOperation("sin")
            }
            .buttonStyle(.bordered)
            
            Button("cos") {
                appendOperation("cos")
            }
            .buttonStyle(.bordered)
        }
    }
    
    private func appendToDisplay(_ text: String) {
        displayValue += text
    }
    
    private func appendOperation(_ operation: String) {
        currentOperation += operation + " "
    }
    
    private func clearDisplay() {
        displayValue = ""
        currentOperation = ""
    }
    
    private func calculateResult() {
        // Simple calculator implementation
        displayValue = "Result"
    }
    
    private func parseDisplayValue() -> Any {
        // Parse display value back to appropriate type
        switch valueType {
        case .integer:
            return Int(displayValue) ?? 0
        case .float:
            return Float(displayValue) ?? 0.0
        default:
            return displayValue
        }
    }
    
    private func formatValue(_ value: Any) -> String {
        switch valueType {
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        default:
            return String(describing: value)
        }
    }
}

// MARK: - Property Value Snapping (TODO-1489)
struct PropertyValueSnapping: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let valueType: SnappingValueType
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var snapEnabled: Bool = true
    @State private var snapInterval: Double = 1.0
    @State private var snapMode: SnapMode = .nearest
    @State private var customSnapPoints: [Double] = []
    @State private var showSettings = false
    @State private var isSnapping: Bool = false
    @State private var lastSnapValue: Any?
    @State private var snapHistory: [SnapEntry] = []
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum SnappingValueType {
        case integer
        case float
        case vector2
        case vector3
        case vector4
        case color
    }
    
    enum SnapMode: String, CaseIterable, Codable {
        case nearest = "Nearest"
        case floor = "Floor"
        case ceil = "Ceiling"
        case custom = "Custom"
        case angle = "Angle"
        case powerOfTwo = "Power of Two"
        case fibonacci = "Fibonacci"
        case goldenRatio = "Golden Ratio"
    }
    
    struct SnapEntry: Identifiable, Codable {
        let id = UUID()
        let originalValue: Any
        let snappedValue: Any
        let snapInterval: Double
        let snapMode: SnapMode
        let timestamp: Date
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with snapping status
            HStack {
                Text("Value Snapping")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                HStack(spacing: 4) {
                    Circle()
                        .fill(snapEnabled ? DesignSystem.Colors.success : DesignSystem.Colors.textTertiary)
                        .frame(width: 8, height: 8)
                    
                    Text(snapEnabled ? "Snapping Enabled" : "Snapping Disabled")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(snapEnabled ? DesignSystem.Colors.success : DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showSettings = true }) {
                    Image(systemName: "gear")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            // Current value display
            HStack {
                Text("Current Value:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text(formatSnapValue(value))
                    .font(DesignSystem.Typography.body.monospaced())
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(4)
                
                if let lastSnap = lastSnapValue {
                    Text("→ \(formatSnapValue(lastSnap))")
                        .font(DesignSystem.Typography.small.monospaced())
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            
            // Snap controls
            HStack {
                Toggle("Enable Snapping", isOn: $snapEnabled)
                    .toggleStyle(.switch)
                    .onChange(of: snapEnabled) { _ in
                        if snapEnabled {
                            applySnap()
                        }
                    }
                
                Spacer()
                
                Button("Snap Now") {
                    applySnap()
                }
                .buttonStyle(.borderedProminent)
                .disabled(isLocked || !snapEnabled)
            }
            
            // Snap mode selection
            HStack {
                Text("Snap Mode:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("Snap Mode", selection: $snapMode) {
                    ForEach(SnapMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.menu)
                .onChange(of: snapMode) { _ in
                    if snapEnabled {
                        applySnap()
                    }
                }
            }
            
            // Snap interval control
            HStack {
                Text("Snap Interval:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                TextField("Interval", value: Binding(
                    get: { snapInterval },
                    set: { newValue in
                        snapInterval = max(0.01, newValue)
                        if snapEnabled {
                            applySnap()
                        }
                    }
                ), format: .number)
                .textFieldStyle(.roundedBorder)
                
                Text(unit ?? "")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            // Quick snap presets
            HStack {
                Text("Quick Presets:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                HStack(spacing: 4) {
                    Button("0.1") {
                        snapInterval = 0.1
                        if snapEnabled { applySnap() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("0.25") {
                        snapInterval = 0.25
                        if snapEnabled { applySnap() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("0.5") {
                        snapInterval = 0.5
                        if snapEnabled { applySnap() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("1.0") {
                        snapInterval = 1.0
                        if snapEnabled { applySnap() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("5.0") {
                        snapInterval = 5.0
                        if snapEnabled { applySnap() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("10.0") {
                        snapInterval = 10.0
                        if snapEnabled { applySnap() }
                    }
                    .buttonStyle(.bordered)
                }
            }
            
            // Snap history
            if !snapHistory.isEmpty {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Snap History")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    ScrollView {
                        LazyVStack(spacing: 2) {
                            ForEach(snapHistory.reversed()) { entry in
                                SnapHistoryRowView(entry: entry)
                            }
                        }
                    }
                    .frame(height: 100)
                }
            }
        }
        .sheet(isPresented: $showSettings) {
            SnapSettingsView(
                snapEnabled: $snapEnabled,
                snapInterval: $snapInterval,
                snapMode: $snapMode,
                customSnapPoints: $customSnapPoints,
                valueType: valueType,
                onSettingsChanged: { applySnap() }
            )
        }
        .onAppear {
            loadSnapHistory()
        }
        .onChange(of: value) { oldValue, newValue in
            if snapEnabled && !isLocked {
                applySnap()
            }
        }
    }
    
    private func applySnap() {
        guard !isLocked else { return }
        
        isSnapping = true
        
        DispatchQueue.global().asyncAfter(deadline: .now() + 0.1) {
            let snappedValue = performSnap()
            
            DispatchQueue.main.async {
                isSnapping = false
                undoManager.recordState(value)
                lastSnapValue = snappedValue
                value = snappedValue
                onValueChanged?(snappedValue)
                
                addToSnapHistory(snappedValue)
            }
        }
    }
    
    private func performSnap() -> Any {
        switch valueType {
        case .integer:
            return snapInteger()
        case .float:
            return snapFloat()
        case .vector2:
            return snapVector2()
        case .vector3:
            return snapVector3()
        case .vector4:
            return snapVector4()
        case .color:
            return snapColor()
        }
    }
    
    private func snapInteger() -> Any {
        let current = value as? Int ?? 0
        let interval = Int(snapInterval)
        
        switch snapMode {
        case .nearest:
            return Int(round(Double(current) / Double(interval)) * Double(interval))
        case .floor:
            return (current / interval) * interval
        case .ceil:
            return ((current + interval - 1) / interval) * interval
        case .custom:
            return customSnapInteger(current)
        case .angle:
            return snapAngle(current)
        case .powerOfTwo:
            return snapPowerOfTwo(current)
        case .fibonacci:
            return snapFibonacci(current)
        case .goldenRatio:
            return snapGoldenRatio(current)
        }
    }
    
    private func snapFloat() -> Any {
        let current = value as? Float ?? 0.0
        
        switch snapMode {
        case .nearest:
            return round(current / Float(snapInterval)) * Float(snapInterval)
        case .floor:
            return floor(current / Float(snapInterval)) * Float(snapInterval)
        case .ceil:
            return ceil(current / Float(snapInterval)) * Float(snapInterval)
        case .custom:
            return customSnapFloat(current)
        case .angle:
            return snapAngle(current)
        case .powerOfTwo:
            return snapPowerOfTwo(current)
        case .fibonacci:
            return snapFibonacci(current)
        case .goldenRatio:
            return snapGoldenRatio(current)
        }
    }
    
    private func snapVector2() -> Any {
        let current = value as? SIMD2<Float> ?? .zero
        
        switch snapMode {
        case .nearest:
            return SIMD2<Float>(
                snapFloatValue(current.x),
                snapFloatValue(current.y)
            )
        case .custom:
            return customSnapVector2(current)
        default:
            return current
        }
    }
    
    private func snapVector3() -> Any {
        let current = value as? SIMD3<Float> ?? .zero
        
        switch snapMode {
        case .nearest:
            return SIMD3<Float>(
                snapFloatValue(current.x),
                snapFloatValue(current.y),
                snapFloatValue(current.z)
            )
        case .custom:
            return customSnapVector3(current)
        default:
            return current
        }
    }
    
    private func snapVector4() -> Any {
        let current = value as? SIMD4<Float> ?? .zero
        
        switch snapMode {
        case .nearest:
            return SIMD4<Float>(
                snapFloatValue(current.x),
                snapFloatValue(current.y),
                snapFloatValue(current.z),
                snapFloatValue(current.w)
            )
        case .custom:
            return customSnapVector4(current)
        default:
            return current
        }
    }
    
    private func snapColor() -> Any {
        // Color snapping would require Color type conversion
        return value
    }
    
    private func snapFloatValue(_ value: Float) -> Float {
        switch snapMode {
        case .nearest:
            return round(value / Float(snapInterval)) * Float(snapInterval)
        case .floor:
            return floor(value / Float(snapInterval)) * Float(snapInterval)
        case .ceil:
            return ceil(value / Float(snapInterval)) * Float(snapInterval)
        default:
            return value
        }
    }
    
    private func snapAngle(_ value: Any) -> Any {
        let current = Double("\(value)") ?? 0.0
        let degrees = current * 180.0 / .pi
        let snappedDegrees = round(degrees / snapInterval) * snapInterval
        let snappedRadians = snappedDegrees * .pi / 180.0
        
        if valueType == .integer {
            return Int(snappedDegrees)
        } else {
            return Float(snappedRadians)
        }
    }
    
    private func snapPowerOfTwo(_ value: Any) -> Any {
        let current = Double("\(value)") ?? 0.0
        let power = round(log2(abs(current)))
        let snapped = pow(2.0, power)
        
        if valueType == .integer {
            return Int(snapped)
        } else {
            return Float(snapped)
        }
    }
    
    private func snapFibonacci(_ value: Any) -> Any {
        let current = Double("\(value)") ?? 0.0
        let fibonacci = generateFibonacciSequence(upTo: current * 2)
        let snapped = fibonacci.min(by: { abs($0 - current) < abs($1 - current) }) ?? current
        
        if valueType == .integer {
            return Int(snapped)
        } else {
            return Float(snapped)
        }
    }
    
    private func snapGoldenRatio(_ value: Any) -> Any {
        let current = Double("\(value)") ?? 0.0
        let phi = (1.0 + sqrt(5.0)) / 2.0
        let snapped = round(current / phi) * phi
        
        if valueType == .integer {
            return Int(snapped)
        } else {
            return Float(snapped)
        }
    }
    
    private func customSnapInteger(_ value: Int) -> Int {
        let points = customSnapPoints.map { Int($0) }
        return points.min(by: { abs($0 - value) < abs($1 - value) }) ?? value
    }
    
    private func customSnapFloat(_ value: Float) -> Float {
        let points = customSnapPoints.map { Float($0) }
        return points.min(by: { abs($0 - value) < abs($1 - value) }) ?? value
    }
    
    private func customSnapVector2(_ value: SIMD2<Float>) -> SIMD2<Float> {
        // Custom vector snapping logic
        return value
    }
    
    private func customSnapVector3(_ value: SIMD3<Float>) -> SIMD3<Float> {
        // Custom vector snapping logic
        return value
    }
    
    private func customSnapVector4(_ value: SIMD4<Float>) -> SIMD4<Float> {
        // Custom vector snapping logic
        return value
    }
    
    private func generateFibonacciSequence(upTo limit: Double) -> [Double] {
        var sequence: [Double] = [0, 1]
        while sequence.last ?? 0 < limit {
            let next = sequence[sequence.count - 1] + sequence[sequence.count - 2]
            sequence.append(next)
        }
        return sequence
    }
    
    private func addToSnapHistory(_ snappedValue: Any) {
        let entry = SnapEntry(
            originalValue: value,
            snappedValue: snappedValue,
            snapInterval: snapInterval,
            snapMode: snapMode,
            timestamp: Date()
        )
        
        snapHistory.insert(entry, at: 0)
        
        if snapHistory.count > 20 {
            snapHistory = Array(snapHistory.prefix(20))
        }
        
        saveSnapHistory()
    }
    
    private func saveSnapHistory() {
        // Implementation for persisting snap history
    }
    
    private func loadSnapHistory() {
        // Implementation for loading snap history
    }
    
    private func formatSnapValue(_ value: Any) -> String {
        switch valueType {
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        }
    }
}

// MARK: - Snap History Row View
struct SnapHistoryRowView: View {
    let entry: PropertyValueSnapping.SnapEntry
    let valueType: PropertyValueSnapping.SnappingValueType
    
    var body: some View {
        HStack {
            // Snap indicator
            VStack(spacing: 2) {
                Circle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 8, height: 8)
                
                Text(entry.snapMode.rawValue)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            
            // Snap details
            VStack(alignment: .leading, spacing: 2) {
                Text("\(formatSnapValue(entry.originalValue)) → \(formatSnapValue(entry.snappedValue))")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                
                Text("Interval: \(String(format: "%.2f", entry.snapInterval))")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(entry.timestamp, style: .relative) ago)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            Spacer()
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
    
    private func formatSnapValue(_ value: Any) -> String {
        switch valueType {
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.2f", value as? Float ?? 0.0)
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.1f", vec.x)), \(String(format: "%.1f", vec.y)), \(String(format: "%.1f", vec.z)), \(String(format: "%.1f", vec.w)))"
        case .color:
            return "Color Value"
        }
    }
}

// MARK: - Snap Settings View
struct SnapSettingsView: View {
    @Binding var snapEnabled: Bool
    @Binding var snapInterval: Double
    @Binding var snapMode: PropertyValueSnapping.SnapMode
    @Binding var customSnapPoints: [Double]
    let valueType: PropertyValueSnapping.SnappingValueType
    let onSettingsChanged: () -> Void
    @Environment(\.dismiss) private var dismiss
    
    @State private var newSnapPoint: String = ""
    
    var body: some View {
        NavigationView {
            Form {
                Section("Snap Settings") {
                    Toggle("Enable Snapping", isOn: $snapEnabled)
                        .onChange(of: snapEnabled) { _ in
                            onSettingsChanged()
                        }
                    
                    Picker("Snap Mode", selection: $snapMode) {
                        ForEach(PropertyValueSnapping.SnapMode.allCases, id: \.self) { mode in
                            Text(mode.rawValue).tag(mode)
                        }
                    }
                    .pickerStyle(.menu)
                    .onChange(of: snapMode) { _ in
                        onSettingsChanged()
                    }
                    
                    HStack {
                        Text("Snap Interval:")
                            .font(DesignSystem.Typography.small)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        TextField("Interval", value: $snapInterval, format: .number)
                            .textFieldStyle(.roundedBorder)
                            .onChange(of: snapInterval) { _ in
                                onSettingsChanged()
                            }
                    }
                }
                
                if snapMode == .custom {
                    Section("Custom Snap Points") {
                        HStack {
                            TextField("New Snap Point", text: $newSnapPoint)
                                .textFieldStyle(.roundedBorder)
                            
                            Button("Add") {
                                if let value = Double(newSnapPoint) {
                                    customSnapPoints.append(value)
                                    newSnapPoint = ""
                                    onSettingsChanged()
                                }
                            }
                            .buttonStyle(.bordered)
                            .disabled(newSnapPoint.isEmpty)
                        }
                        
                        ForEach(Array(customSnapPoints.enumerated()), id: \.offset) { index, point in
                            HStack {
                                Text(String(format: "%.2f", point))
                                    .font(DesignSystem.Typography.body)
                                
                                Spacer()
                                
                                Button("Remove") {
                                    customSnapPoints.remove(at: index)
                                    onSettingsChanged()
                                }
                                .buttonStyle(.bordered)
                                .foregroundColor(DesignSystem.Colors.error)
                            }
                        }
                    }
                }
                
                Section("Advanced Options") {
                    Toggle("Auto-snap on value change", isOn: Binding(
                        get: { true },
                        set: { newValue in
                            // Auto-snap logic would go here
                        }
                    ))
                    
                    Toggle("Visual snap indicators", isOn: Binding(
                        get: { true },
                        set: { newValue in
                            // Visual indicators logic would go here
                        }
                    ))
                    
                    Text("Snap Sensitivity")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Slider(value: Binding(
                        get: { 0.5 },
                        set: { newValue in
                            // Sensitivity logic would go here
                        }
                    ), in: 0...1)
                }
            }
            .navigationTitle("Snap Settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") { dismiss() }
                }
            }
        }
        .frame(width: 400, height: 600)
    }
}

// MARK: - Property Value Rounding (TODO-1490)
struct PropertyValueRounding: View, PropertyEditor {
    let propertyName: String
    @Binding var value: Any
    let valueType: RoundingValueType
    
    // Configuration
    let config: PropertyEditorConfig<Any>
    
    // Protocol conformance
    var defaultValue: Any? { config.defaultValue }
    var isLocked: Bool { config.isLocked }
    var isOverridden: Bool { config.isOverridden }
    var tooltip: String? { config.tooltip }
    var unit: String? { config.unit }
    var propertyType: PropertyType { config.propertyType }
    var onValueChanged: ((Any) -> Void)? { config.onValueChanged }
    var onReset: (() -> Void)? { config.onReset }
    var onLockToggle: ((Bool) -> Void)? { config.onLockToggle }
    var onAnimationKeyframe: (() -> Void)? { config.onAnimationKeyframe }
    var onExpressionInput: (() -> Void)? { config.onExpressionInput }
    var onLinkProperty: (() -> Void)? { config.onLinkProperty }
    
    // State
    @State private var roundingEnabled: Bool = true
    @State private var roundingMode: RoundingMode = .nearest
    @State private var precision: Int = 2
    @State private var showSettings = false
    @State private var isRounding: Bool = false
    @State private var lastRoundedValue: Any?
    @State private var roundingHistory: [RoundingEntry] = []
    
    // Managers
    @StateObject private var undoManager = PropertyUndoManager()
    @EnvironmentObject private var clipboardManager: PropertyClipboardManager
    
    enum RoundingValueType {
        case integer
        case float
        case vector2
        case vector3
        case vector4
        case color
    }
    
    enum RoundingMode: String, CaseIterable, Codable {
        case nearest = "Nearest"
        case floor = "Floor"
        case ceil = "Ceiling"
        case truncate = "Truncate"
        case bankers = "Bankers"
        case awayFromZero = "Away From Zero"
        case towardZero = "Toward Zero"
        case custom = "Custom"
    }
    
    struct RoundingEntry: Identifiable, Codable {
        let id = UUID()
        let originalValue: Any
        let roundedValue: Any
        let roundingMode: RoundingMode
        let precision: Int
        let timestamp: Date
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with rounding status
            HStack {
                Text("Value Rounding")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                HStack(spacing: 4) {
                    Circle()
                        .fill(roundingEnabled ? DesignSystem.Colors.success : DesignSystem.Colors.textTertiary)
                        .frame(width: 8, height: 8)
                    
                    Text(roundingEnabled ? "Rounding Enabled" : "Rounding Disabled")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(roundingEnabled ? DesignSystem.Colors.success : DesignSystem.Colors.textTertiary)
                }
                
                Button(action: { showSettings = true }) {
                    Image(systemName: "gear")
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            
            // Current value display
            HStack {
                Text("Current Value:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Text(formatRoundingValue(value))
                    .font(DesignSystem.Typography.body.monospaced())
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundSecondary)
                    .cornerRadius(4)
                
                if let lastRounded = lastRoundedValue {
                    Text("→ \(formatRoundingValue(lastRounded))")
                        .font(DesignSystem.Typography.small.monospaced())
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
            }
            
            // Rounding controls
            HStack {
                Toggle("Enable Rounding", isOn: $roundingEnabled)
                    .toggleStyle(.switch)
                    .onChange(of: roundingEnabled) { _ in
                        if roundingEnabled {
                            applyRounding()
                        }
                    }
                
                Spacer()
                
                Button("Round Now") {
                    applyRounding()
                }
                .buttonStyle(.borderedProminent)
                .disabled(isLocked || !roundingEnabled)
            }
            
            // Rounding mode selection
            HStack {
                Text("Rounding Mode:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Picker("Rounding Mode", selection: $roundingMode) {
                    ForEach(RoundingMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.menu)
                .onChange(of: roundingMode) { _ in
                    if roundingEnabled {
                        applyRounding()
                    }
                }
            }
            
            // Precision control
            HStack {
                Text("Decimal Places:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Stepper("Precision: \(precision)", value: $precision, in: 0...10)
                    .onChange(of: precision) { _ in
                        if roundingEnabled {
                            applyRounding()
                        }
                    }
            }
            
            // Quick precision presets
            HStack {
                Text("Quick Presets:")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                HStack(spacing: 4) {
                    Button("0") {
                        precision = 0
                        if roundingEnabled { applyRounding() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("1") {
                        precision = 1
                        if roundingEnabled { applyRounding() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("2") {
                        precision = 2
                        if roundingEnabled { applyRounding() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("3") {
                        precision = 3
                        if roundingEnabled { applyRounding() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("4") {
                        precision = 4
                        if roundingEnabled { applyRounding() }
                    }
                    .buttonStyle(.bordered)
                    
                    Button("6") {
                        precision = 6
                        if roundingEnabled { applyRounding() }
                    }
                    .buttonStyle(.bordered)
                }
            }
            
            // Rounding history
            if !roundingHistory.isEmpty {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Rounding History")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    ScrollView {
                        LazyVStack(spacing: 2) {
                            ForEach(roundingHistory.reversed()) { entry in
                                RoundingHistoryRowView(entry: entry)
                            }
                        }
                    }
                    .frame(height: 100)
                }
            }
        }
        .sheet(isPresented: $showSettings) {
            RoundingSettingsView(
                roundingEnabled: $roundingEnabled,
                roundingMode: $roundingMode,
                precision: $precision,
                valueType: valueType,
                onSettingsChanged: { applyRounding() }
            )
        }
        .onAppear {
            loadRoundingHistory()
        }
        .onChange(of: value) { oldValue, newValue in
            if roundingEnabled && !isLocked {
                applyRounding()
            }
        }
    }
    
    private func applyRounding() {
        guard !isLocked else { return }
        
        isRounding = true
        
        DispatchQueue.global().asyncAfter(deadline: .now() + 0.1) {
            let roundedValue = performRounding()
            
            DispatchQueue.main.async {
                isRounding = false
                undoManager.recordState(value)
                lastRoundedValue = roundedValue
                value = roundedValue
                onValueChanged?(roundedValue)
                
                addToRoundingHistory(roundedValue)
            }
        }
    }
    
    private func performRounding() -> Any {
        switch valueType {
        case .integer:
            return roundInteger()
        case .float:
            return roundFloat()
        case .vector2:
            return roundVector2()
        case .vector3:
            return roundVector3()
        case .vector4:
            return roundVector4()
        case .color:
            return roundColor()
        }
    }
    
    private func roundInteger() -> Any {
        let current = value as? Int ?? 0
        
        switch roundingMode {
        case .nearest:
            return current
        case .floor:
            return current
        case .ceil:
            return current
        case .truncate:
            return current
        case .bankers:
            return current
        case .awayFromZero:
            return current
        case .towardZero:
            return current
        case .custom:
            return current
        }
    }
    
    private func roundFloat() -> Any {
        let current = value as? Float ?? 0.0
        let multiplier = Float(pow(10.0, Double(precision)))
        
        switch roundingMode {
        case .nearest:
            return round(current * multiplier) / multiplier
        case .floor:
            return floor(current * multiplier) / multiplier
        case .ceil:
            return ceil(current * multiplier) / multiplier
        case .truncate:
            return trunc(current * multiplier) / multiplier
        case .bankers:
            return bankersRound(current * multiplier) / multiplier
        case .awayFromZero:
            return current >= 0 ? ceil(current * multiplier) / multiplier : floor(current * multiplier) / multiplier
        case .towardZero:
            return current >= 0 ? floor(current * multiplier) / multiplier : ceil(current * multiplier) / multiplier
        case .custom:
            return customRound(current)
        }
    }
    
    private func roundVector2() -> Any {
        let current = value as? SIMD2<Float> ?? .zero
        
        return SIMD2<Float>(
            roundFloatValue(current.x),
            roundFloatValue(current.y)
        )
    }
    
    private func roundVector3() -> Any {
        let current = value as? SIMD3<Float> ?? .zero
        
        return SIMD3<Float>(
            roundFloatValue(current.x),
            roundFloatValue(current.y),
            roundFloatValue(current.z)
        )
    }
    
    private func roundVector4() -> Any {
        let current = value as? SIMD4<Float> ?? .zero
        
        return SIMD4<Float>(
            roundFloatValue(current.x),
            roundFloatValue(current.y),
            roundFloatValue(current.z),
            roundFloatValue(current.w)
        )
    }
    
    private func roundColor() -> Any {
        // Color rounding would require Color type conversion
        return value
    }
    
    private func roundFloatValue(_ value: Float) -> Float {
        let multiplier = Float(pow(10.0, Double(precision)))
        
        switch roundingMode {
        case .nearest:
            return round(value * multiplier) / multiplier
        case .floor:
            return floor(value * multiplier) / multiplier
        case .ceil:
            return ceil(value * multiplier) / multiplier
        case .truncate:
            return trunc(value * multiplier) / multiplier
        case .bankers:
            return bankersRound(value * multiplier) / multiplier
        case .awayFromZero:
            return value >= 0 ? ceil(value * multiplier) / multiplier : floor(value * multiplier) / multiplier
        case .towardZero:
            return value >= 0 ? floor(value * multiplier) / multiplier : ceil(value * multiplier) / multiplier
        case .custom:
            return customRound(value)
        }
    }
    
    private func bankersRound(_ value: Float) -> Float {
        let integer = Int(value)
        let fractional = value - Float(integer)
        
        if fractional < 0.5 {
            return Float(integer)
        } else if fractional > 0.5 {
            return Float(integer + 1)
        } else {
            // Exactly .5 - round to nearest even
            return integer % 2 == 0 ? Float(integer) : Float(integer + 1)
        }
    }
    
    private func customRound(_ value: Float) -> Float {
        // Custom rounding logic could be implemented here
        // For now, fall back to nearest
        return round(value)
    }
    
    private func addToRoundingHistory(_ roundedValue: Any) {
        let entry = RoundingEntry(
            originalValue: value,
            roundedValue: roundedValue,
            roundingMode: roundingMode,
            precision: precision,
            timestamp: Date()
        )
        
        roundingHistory.insert(entry, at: 0)
        
        if roundingHistory.count > 20 {
            roundingHistory = Array(roundingHistory.prefix(20))
        }
        
        saveRoundingHistory()
    }
    
    private func saveRoundingHistory() {
        // Implementation for persisting rounding history
    }
    
    private func loadRoundingHistory() {
        // Implementation for loading rounding history
    }
    
    private func formatRoundingValue(_ value: Any) -> String {
        switch valueType {
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.\(precision)f", value as? Float ?? 0.0)
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.\(precision)f", vec.x)), \(String(format: "%.\(precision)f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.\(precision)f", vec.x)), \(String(format: "%.\(precision)f", vec.y)), \(String(format: "%.\(precision)f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.\(precision)f", vec.x)), \(String(format: "%.\(precision)f", vec.y)), \(String(format: "%.\(precision)f", vec.z)), \(String(format: "%.\(precision)f", vec.w)))"
        case .color:
            return "Color Value"
        }
    }
}

// MARK: - Rounding History Row View
struct RoundingHistoryRowView: View {
    let entry: PropertyValueRounding.RoundingEntry
    let valueType: PropertyValueRounding.RoundingValueType
    
    var body: some View {
        HStack {
            // Rounding indicator
            VStack(spacing: 2) {
                Circle()
                    .fill(DesignSystem.Colors.accentPrimary)
                    .frame(width: 8, height: 8)
                
                Text(entry.roundingMode.rawValue)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            
            // Rounding details
            VStack(alignment: .leading, spacing: 2) {
                Text("\(formatRoundingValue(entry.originalValue)) → \(formatRoundingValue(entry.roundedValue))")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .lineLimit(1)
                
                Text("Precision: \(entry.precision) decimal places")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
                
                Text(entry.timestamp, style: .relative) ago)
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textTertiary)
            }
            
            Spacer()
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 4)
        .background(DesignSystem.Colors.backgroundPrimary)
        .cornerRadius(4)
    }
    
    private func formatRoundingValue(_ value: Any) -> String {
        switch valueType {
        case .integer:
            return String(describing: value as? Int ?? 0)
        case .float:
            return String(format: "%.\(entry.precision)f", value as? Float ?? 0.0)
        case .vector2:
            let vec = value as? SIMD2<Float> ?? .zero
            return "(\(String(format: "%.\(entry.precision)f", vec.x)), \(String(format: "%.\(entry.precision)f", vec.y)))"
        case .vector3:
            let vec = value as? SIMD3<Float> ?? .zero
            return "(\(String(format: "%.\(entry.precision)f", vec.x)), \(String(format: "%.\(entry.precision)f", vec.y)), \(String(format: "%.\(entry.precision)f", vec.z)))"
        case .vector4:
            let vec = value as? SIMD4<Float> ?? .zero
            return "(\(String(format: "%.\(entry.precision)f", vec.x)), \(String(format: "%.\(entry.precision)f", vec.y)), \(String(format: "%.\(entry.precision)f", vec.z)), \(String(format: "%.\(entry.precision)f", vec.w)))"
        case .color:
            return "Color Value"
        }
    }
}

// MARK: - Rounding Settings View
struct RoundingSettingsView: View {
    @Binding var roundingEnabled: Bool
    @Binding var roundingMode: PropertyValueRounding.RoundingMode
    @Binding var precision: Int
    let valueType: PropertyValueRounding.RoundingValueType
    let onSettingsChanged: () -> Void
    @Environment(\.dismiss) private var dismiss
    
    var body: some View {
        NavigationView {
            Form {
                Section("Rounding Settings") {
                    Toggle("Enable Rounding", isOn: $roundingEnabled)
                        .onChange(of: roundingEnabled) { _ in
                            onSettingsChanged()
                        }
                    
                    Picker("Rounding Mode", selection: $roundingMode) {
                        ForEach(PropertyValueRounding.RoundingMode.allCases, id: \.self) { mode in
                            Text(mode.rawValue).tag(mode)
                        }
                    }
                    .pickerStyle(.menu)
                    .onChange(of: roundingMode) { _ in
                        onSettingsChanged()
                    }
                    
                    Stepper("Decimal Places: \(precision)", value: $precision, in: 0...10)
                        .onChange(of: precision) { _ in
                            onSettingsChanged()
                        }
                }
                
                Section("Rounding Mode Descriptions") {
                    Text("Nearest: Standard rounding to nearest value")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Floor: Always round down")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Ceiling: Always round up")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Truncate: Remove decimal part")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Bankers: Round to nearest even number on .5")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Away From Zero: Always round away from zero")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Text("Toward Zero: Always round toward zero")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                
                Section("Advanced Options") {
                    Toggle("Auto-round on value change", isOn: Binding(
                        get: { true },
                        set: { newValue in
                            // Auto-round logic would go here
                        }
                    ))
                    
                    Toggle("Visual rounding indicators", isOn: Binding(
                        get: { true },
                        set: { newValue in
                            // Visual indicators logic would go here
                        }
                    ))
                    
                    Text("Rounding Sensitivity")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Slider(value: Binding(
                        get: { 0.5 },
                        set: { newValue in
                            // Sensitivity logic would go here
                        }
                    ), in: 0...1)
                }
            }
            .navigationTitle("Rounding Settings")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") { dismiss() }
                }
                
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") { dismiss() }
                }
            }
        }
        .frame(width: 400, height: 600)
    }
}

// MARK: - Curve Editor
struct CurveEditor: View {
    @Binding var points: [SIMD2<Float>]
    var rangeX: ClosedRange<Float> = 0...1
    var rangeY: ClosedRange<Float> = 0...1
    
    @State private var draggedPointIndex: Int?
    @State private var hoveredPointIndex: Int?
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background grid
                CurveGrid(rangeX: rangeX, rangeY: rangeY)
                
                // Curve Path
                Path { path in
                    guard points.count >= 2 else { return }
                    
                    let sortedPoints = points.sorted { $0.x < $1.x }
                    let start = pointToScreen(sortedPoints[0], size: geometry.size)
                    path.move(to: start)
                    
                    for i in 1..<sortedPoints.count {
                        let pt = pointToScreen(sortedPoints[i], size: geometry.size)
                        path.addLine(to: pt)
                    }
                }
                .stroke(DesignSystem.Colors.accentPrimary, lineWidth: 2)
                
                // Control Points
                ForEach(0..<points.count, id: \.self) { index in
                    let pt = pointToScreen(points[index], size: geometry.size)
                    Circle()
                        .fill(DesignSystem.Colors.textPrimary)
                        .frame(width: 8, height: 8)
                        .position(pt)
                        .gesture(
                            DragGesture()
                                .onChanged { value in
                                    draggedPointIndex = index
                                    let location = value.location
                                    let newPoint = screenToPoint(location, size: geometry.size)
                                    
                                    // Constrain to range
                                    let constrainedX = min(rangeX.upperBound, max(rangeX.lowerBound, newPoint.x))
                                    let constrainedY = min(rangeY.upperBound, max(rangeY.lowerBound, newPoint.y))
                                    
                                    points[index] = SIMD2<Float>(constrainedX, constrainedY)
                                }
                                .onEnded { _ in
                                    draggedPointIndex = nil
                                    points.sort { $0.x < $1.x }
                                }
                        )
                        .onHover { isHovering in
                            hoveredPointIndex = isHovering ? index : nil
                        }
                }
            }
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(DesignSystem.CornerRadius.md)
            .clipped()
        }
    }
    
    private func pointToScreen(_ point: SIMD2<Float>, size: CGSize) -> CGPoint {
        let normalizedX = (point.x - rangeX.lowerBound) / (rangeX.upperBound - rangeX.lowerBound)
        let normalizedY = (point.y - rangeY.lowerBound) / (rangeY.upperBound - rangeY.lowerBound)
        
        return CGPoint(
            x: CGFloat(normalizedX) * size.width,
            y: size.height - CGFloat(normalizedY) * size.height
        )
    }
    
    private func screenToPoint(_ point: CGPoint, size: CGSize) -> SIMD2<Float> {
        let normalizedX = Float(point.x / size.width)
        let normalizedY = Float(1.0 - (point.y / size.height))
        
        let x = normalizedX * (rangeX.upperBound - rangeX.lowerBound) + rangeX.lowerBound
        let y = normalizedY * (rangeY.upperBound - rangeY.lowerBound) + rangeY.lowerBound
        
        return SIMD2<Float>(x, y)
    }
}

struct CurveGrid: View {
    let rangeX: ClosedRange<Float>
    let rangeY: ClosedRange<Float>
    
    var body: some View {
        Canvas { context, size in
            let path = Path { p in
                for i in 0...10 {
                    let x = size.width * CGFloat(i) / 10.0
                    p.move(to: CGPoint(x: x, y: 0))
                    p.addLine(to: CGPoint(x: x, y: size.height))
                }
                for i in 0...10 {
                    let y = size.height * CGFloat(i) / 10.0
                    p.move(to: CGPoint(x: 0, y: y))
                    p.addLine(to: CGPoint(x: size.width, y: y))
                }
            }
            context.stroke(path, with: .color(DesignSystem.Colors.backgroundTertiary), lineWidth: 1)
        }
    }
}
