import SwiftUI
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
    @State private var textValue: String = ""
    @State private var isValid: Bool = true
    
    init(value: Binding<Float>) {
        self._value = value
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text("Scientific")
                    .font(.caption)
                    .foregroundColor(.secondary)
                
                Spacer()
                
                if !isValid {
                    Image(systemName: "exclamationmark.triangle")
                        .foregroundColor(.orange)
                }
                
                Text(value.formatted(.number.precision(.fractionLength(2)).notation(.scientific)))
                    .font(.caption.monospaced())
                    .foregroundColor(isFinite(value) ? .primary : .red)
            }
            
            TextField("Value", text: $textValue)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .keyboardType(.decimalPad)
                .onAppear {
                    updateTextFromValue()
                }
                .onChange(of: textValue) { newValue in
                    updateValueFromText()
                }
        }
    }
    
    private func updateTextFromValue() {
        if isFinite(value) {
            textValue = String(format: "%.6e", value)
        } else {
            textValue = ""
        }
        isValid = true
    }
    
    private func updateValueFromText() {
        guard !textValue.isEmpty else {
            value = 0
            isValid = false
            return
        }
        
        let formatter = NumberFormatter()
        formatter.numberStyle = .scientific
        formatter.locale = Locale.current
        
        if let number = formatter.number(from: textValue)?.floatValue {
            value = number
            isValid = true
        } else {
            isValid = false
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
    @State private var displayValue: String = ""
    @State private var isValid: Bool = true
    
    enum DistanceUnit: String, CaseIterable {
        case meters = "m"
        case kilometers = "km"
        case miles = "mi"
        case feet = "ft"
        case yards = "yd"
        case nauticalMiles = "nm"
        case centimeters = "cm"
        case millimeters = "mm"
        
        var conversionToMeters: Float {
            switch self {
            case .meters: return 1.0
            case .kilometers: return 1000.0
            case .miles: return 1609.34
            case .feet: return 0.3048
            case .yards: return 0.9144
            case .nauticalMiles: return 1852.0
            case .centimeters: return 0.01
            case .millimeters: return 0.001
            }
        }
        
        var symbol: String {
            return self.rawValue
        }
        
        var fullName: String {
            switch self {
            case .meters: return "Meters"
            case .kilometers: return "Kilometers"
            case .miles: return "Miles"
            case .feet: return "Feet"
            case .yards: return "Yards"
            case .nauticalMiles: return "Nautical Miles"
            case .centimeters: return "Centimeters"
            case .millimeters: return "Millimeters"
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
                
                if !isValid {
                    Image(systemName: "exclamationmark.triangle")
                        .foregroundColor(.orange)
                }
                
                Text(getFormattedDistance())
                    .font(.caption.monospaced())
                    .foregroundColor(.primary)
            }
            
            HStack {
                TextField("Value", text: $displayValue)
                    .textFieldStyle(RoundedBorderTextFieldStyle())
                    .keyboardType(.decimalPad)
                    .onAppear {
                        updateDisplayFromValue()
                    }
                    .onChange(of: displayValue) { newValue in
                        updateValueFromDisplay()
                    }
                
                Picker("Unit", selection: $unit) {
                    ForEach(DistanceUnit.allCases, id: \.self) { unit in
                        Text(unit.fullName).tag(unit)
                    }
                }
                .pickerStyle(MenuPickerStyle())
                .frame(width: 120)
                
                Text(unit.symbol)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            
            // Quick conversion buttons
            HStack {
                ForEach([DistanceUnit.meters, .kilometers, .miles, .feet], id: \.self) { quickUnit in
                    if quickUnit != unit {
                        Button(quickUnit.symbol) {
                            convertToUnit(quickUnit)
                        }
                        .buttonStyle(BorderlessButtonStyle())
                        .font(.caption2)
                        .foregroundColor(.blue)
                    }
                }
                
                Spacer()
            }
        }
    }
    
    private func updateDisplayFromValue() {
        let convertedValue = value / unit.conversionToMeters
        displayValue = String(format: "%.3f", convertedValue)
        isValid = true
    }
    
    private func updateValueFromDisplay() {
        guard !displayValue.isEmpty else {
            value = 0
            isValid = false
            return
        }
        
        if let number = Float(displayValue) {
            value = number * unit.conversionToMeters
            isValid = true
        } else {
            isValid = false
        }
    }
    
    private func convertToUnit(_ targetUnit: DistanceUnit) {
        let convertedValue = value / targetUnit.conversionToMeters
        value = convertedValue * targetUnit.conversionToMeters
        unit = targetUnit
        updateDisplayFromValue()
    }
    
    private func getFormattedDistance() -> String {
        let convertedValue = value / unit.conversionToMeters
        return String(format: "%.3f %@", convertedValue, unit.symbol)
    }
}
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

