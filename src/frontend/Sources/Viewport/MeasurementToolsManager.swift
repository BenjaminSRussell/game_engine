import SwiftUI
import simd

// MARK: - Measurement & Annotation Tools
// Provides precision measurement and annotation capabilities in the viewport

class MeasurementToolsManager: ObservableObject {
    @Published var activeTool: MeasurementTool?
    @Published var measurements: [Measurement] = []
    @Published var annotations: [Annotation] = []
    @Published var snapToVertices = true
    @Published var snapToEdges = false
    @Published var snapToGrid = true
    @Published var measurementUnit: MeasurementUnit = .meters
    
    // MARK: - Measurement Tool
    enum MeasurementTool: String, CaseIterable, Identifiable {
        // TODO: Implement distance measurement tools
        case distance = "Distance"
        // TODO: Implement angle measurement tools
        case angle = "Angle"
        // TODO: Implement area calculator (2D)
        case area = "Area"
        // TODO: Implement volume calculator (3D)
        case volume = "Volume"
        // TODO: Implement radius measurement
        case radius = "Radius"
        // TODO: Implement arc length
        case arcLength = "Arc Length"
        // TODO: Implement perimeter
        case perimeter = "Perimeter"
        
        var id: String { rawValue }
        
        var icon: String {
            switch self {
            case .distance: return "ruler"
            case .angle: return "angle"
            case .area: return "square.dashed"
            case .volume: return "cube.transparent"
            case .radius: return "circle.dashed"
            case .arcLength: return "circle.lefthalf.filled"
            case .perimeter: return "hexagon"
            }
        }
    }
    
    // MARK: - Measurement Unit
    enum MeasurementUnit: String, CaseIterable {
        case meters = "Meters (m)"
        case centimeters = "Centimeters (cm)"
        case millimeters = "Millimeters (mm)"
        case kilometers = "Kilometers (km)"
        case feet = "Feet (ft)"
        case inches = "Inches (in)"
        case yards = "Yards (yd)"
        case miles = "Miles (mi)"
        
        // TODO: Implement unit conversion
        func convert(value: Float, to target: MeasurementUnit) -> Float {
            // Conversion logic placeholder
            return value
        }
        
        var abbreviation: String {
            switch self {
            case .meters: return "m"
            case .centimeters: return "cm"
            case .millimeters: return "mm"
            case .kilometers: return "km"
            case .feet: return "ft"
            case .inches: return "in"
            case .yards: return "yd"
            case .miles: return "mi"
            }
        }
    }
    
    // MARK: - Measurement
    struct Measurement: Identifiable {
        let id = UUID()
        let tool: MeasurementTool
        let points: [SIMD3<Float>]
        let value: Float
        let unit: MeasurementUnit
        var label: String
        var color: Color = .yellow
        var timestamp: Date = Date()
        
        var formattedValue: String {
            String(format: "%.2f %@", value, unit.abbreviation)
        }
        
        // TODO: Render measurement in viewport (line, arc, polygon outline)
        // TODO: Display measurement value as floating label
        // TODO: Highlight measurement on hover
    }
    
    // MARK: - Annotation
    struct Annotation: Identifiable {
        let id = UUID()
        var text: String
        var position: SIMD3<Float>
        var color: Color = .white
        var fontSize: Float = 14
        var backgroundColor: Color = Color.black.opacity(0.7)
        var isVisible: Bool = true
        var timestamp: Date = Date()
        
        // TODO: Render annotation as 3D text label
        // TODO: Support billboard-style annotations (always face camera)
        // TODO: Support multiple lines of text
        // TODO: Support rich text formatting
    }
    
    // MARK: - Functions
    
    func selectTool(_ tool: MeasurementTool?) {
        activeTool = tool
        if let tool = tool {
            print("[Measurements] Selected tool: \(tool.rawValue)")
        } else {
            print("[Measurements] Deselected tool")
        }
    }
    
    // TODO: Implement distance measurement between two points
    func measureDistance(from: SIMD3<Float>, to: SIMD3<Float>) -> Measurement {
        let distance = simd_distance(from, to)
        let measurement = Measurement(
            tool: .distance,
            points: [from, to],
            value: distance,
            unit: measurementUnit,
            label: "Distance"
        )
        measurements.append(measurement)
        print("[Measurements] Distance: \(measurement.formattedValue)")
        return measurement
    }
    
    // TODO: Implement angle measurement between three points
    func measureAngle(point1: SIMD3<Float>, vertex: SIMD3<Float>, point2: SIMD3<Float>) -> Measurement {
        let v1 = simd_normalize(point1 - vertex)
        let v2 = simd_normalize(point2 - vertex)
        let dot = simd_dot(v1, v2)
        let angle = acos(dot) * (180.0 / Float.pi) // Convert to degrees
        
        let measurement = Measurement(
            tool: .angle,
            points: [point1, vertex, point2],
            value: angle,
            unit: .meters, // Angles use degrees, not distance units
            label: "Angle"
        )
        measurements.append(measurement)
        print("[Measurements] Angle: \(String(format: "%.2f", angle))")
        return measurement
    }
    
    // TODO: Implement area calculation for polygon
    func measureArea(points: [SIMD3<Float>]) -> Measurement {
        // Placeholder: simplified 2D area calculation
        var area: Float = 0.0
        // TODO: Implement proper polygon area calculation
        
        let measurement = Measurement(
            tool: .area,
            points: points,
            value: area,
            unit: measurementUnit,
            label: "Area"
        )
        measurements.append(measurement)
        print("[Measurements] Area: \(measurement.formattedValue)")
        return measurement
    }
    
    // TODO: Implement volume calculation for closed mesh
    func measureVolume(meshVertices: [SIMD3<Float>]) -> Measurement {
        // Placeholder
        let volume: Float = 0.0
        // TODO: Implement mesh volume calculation (signed tetrahedron method)
        
        let measurement = Measurement(
            tool: .volume,
            points: meshVertices,
            value: volume,
            unit: measurementUnit,
            label: "Volume"
        )
        measurements.append(measurement)
        print("[Measurements] Volume: \(measurement.formattedValue)")
        return measurement
    }
    
    // TODO: Implement radius measurement (circle from 3 points)
    func measureRadius(p1: SIMD3<Float>, p2: SIMD3<Float>, p3: SIMD3<Float>) -> Measurement {
        // TODO: Calculate circle center and radius from 3 points
        let radius: Float = 0.0
        
        let measurement = Measurement(
            tool: .radius,
            points: [p1, p2, p3],
            value: radius,
            unit: measurementUnit,
            label: "Radius"
        )
        measurements.append(measurement)
        return measurement
    }
    
    func addAnnotation(text: String, position: SIMD3<Float>) {
        let annotation = Annotation(text: text, position: position)
        annotations.append(annotation)
        print("[Measurements] Added annotation at \(position): \(text)")
    }
    
    func deleteMeasurement(_ id: UUID) {
        measurements.removeAll { $0.id == id }
    }
    
    func deleteAnnotation(_ id: UUID) {
        annotations.removeAll { $0.id == id }
    }
    
    func clearAllMeasurements() {
        measurements.removeAll()
        print("[Measurements] Cleared all measurements")
    }
    
    func clearAllAnnotations() {
        annotations.removeAll()
        print("[Measurements] Cleared all annotations")
    }
    
    // TODO: Export measurements to CSV
    func exportMeasurements() -> String {
        var csv = "Tool,Value,Unit,Timestamp\n"
        for measurement in measurements {
            csv += "\(measurement.tool.rawValue),\(measurement.value),\(measurement.unit.abbreviation),\(measurement.timestamp)\n"
        }
        return csv
    }
    
    //TODO: Import measurements from CSV
    func importMeasurements(csv: String) {
        print("[Measurements] Importing measurements from CSV")
        // TODO: Parse CSV and create measurements
    }
}

// MARK: - Measurement Tools Panel
struct MeasurementToolsPanel: View {
    @ObservedObject var manager: MeasurementToolsManager
    @State private var showingMeasurements = true
    @State private var showingAnnotations = true
    @State private var newAnnotationText = ""
    
    var body: some View {
        VStack(spacing: 0) {
            // Tool selector
            HStack {
                Text("Measurement Tool")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if let tool = manager.activeTool {
                    Button(action: {
                        manager.selectTool(nil)
                    }) {
                        HStack {
                            Image(systemName: tool.icon)
                            Text(tool.rawValue)
                            Image(systemName: "xmark.circle.fill")
                                .font(.system(size: 12))
                        }
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.accentPrimary)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(DesignSystem.Colors.selection)
                        .cornerRadius(4)
                    }
                    .buttonStyle(.plain)
                } else {
                    Menu {
                        ForEach(MeasurementToolsManager.MeasurementTool.allCases) { tool in
                            Button(action: {
                                manager.selectTool(tool)
                            }) {
                                HStack {
                                    Image(systemName: tool.icon)
                                    Text(tool.rawValue)
                                }
                            }
                        }
                    } label: {
                        HStack {
                            Text("Select Tool")
                            Image(systemName: "chevron.down")
                                .font(.system(size: 10))
                        }
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Settings
            VStack(spacing: 8) {
                // Unit selection
                HStack {
                    Text("Unit")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 60, alignment: .leading)
                    
                    Menu {
                        ForEach(MeasurementToolsManager.MeasurementUnit.allCases, id: \.self) { unit in
                            Button(action: {
                                manager.measurementUnit = unit
                            }) {
                                Text(unit.rawValue)
                            }
                        }
                    } label: {
                        HStack {
                            Text(manager.measurementUnit.rawValue)
                            Spacer()
                            Image(systemName: "chevron.down")
                                .font(.system(size: 10))
                        }
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 4)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                    }
                }
                
                // Snap settings
                Toggle("Snap to Vertices", isOn: $manager.snapToVertices)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
                
                Toggle("Snap to Edges", isOn: $manager.snapToEdges)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
                
                Toggle("Snap to Grid", isOn: $manager.snapToGrid)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
            }
            .padding(8)
            
            EditorDivider()
            
            // Measurements list
            VStack(alignment: .leading, spacing: 8) {
                Button(action: {
                    showingMeasurements.toggle()
                }) {
                    HStack {
                        Text("Measurements (\(manager.measurements.count))")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        if manager.measurements.count > 0 {
                            Button(action: {
                                manager.clearAllMeasurements()
                            }) {
                                Image(systemName: "trash")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                        }
                        
                        Image(systemName: showingMeasurements ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                    }
                }
                .buttonStyle(.plain)
                
                if showingMeasurements {
                    ScrollView {
                        VStack(spacing: 4) {
                            ForEach(manager.measurements) { measurement in
                                MeasurementRow(
                                    measurement: measurement,
                                    onDelete: {
                                        manager.deleteMeasurement(measurement.id)
                                    }
                                )
                            }
                        }
                    }
                    .frame(maxHeight: 150)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Annotations list
            VStack(alignment: .leading, spacing: 8) {
                Button(action: {
                    showingAnnotations.toggle()
                }) {
                    HStack {
                        Text("Annotations (\(manager.annotations.count))")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        Spacer()
                        
                        if manager.annotations.count > 0 {
                            Button(action: {
                                manager.clearAllAnnotations()
                            }) {
                                Image(systemName: "trash")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                        }
                        
                        Image(systemName: showingAnnotations ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                    }
                }
                .buttonStyle(.plain)
                
                if showingAnnotations {
                    // Add annotation input
                    HStack {
                        TextField("Add annotation...", text: $newAnnotationText)
                            .textFieldStyle(.plain)
                            .font(DesignSystem.Typography.small)
                            .padding(4)
                            .background(DesignSystem.Colors.backgroundPrimary)
                            .cornerRadius(4)
                        
                        Button(action: {
                            if !newAnnotationText.isEmpty {
                                // TODO: Get current cursor 3D position
                                manager.addAnnotation(text: newAnnotationText, position: SIMD3(0, 0, 0))
                                newAnnotationText = ""
                            }
                        }) {
                            Image(systemName: "plus.circle.fill")
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                        }
                        .buttonStyle(.plain)
                        .disabled(newAnnotationText.isEmpty)
                    }
                    
                    ScrollView {
                        VStack(spacing: 4) {
                            ForEach(manager.annotations) { annotation in
                                AnnotationRow(
                                    annotation: annotation,
                                    onDelete: {
                                        manager.deleteAnnotation(annotation.id)
                                    },
                                    onToggleVisibility: {
                                        if let index = manager.annotations.firstIndex(where: { $0.id == annotation.id }) {
                                            manager.annotations[index].isVisible.toggle()
                                        }
                                    }
                                )
                            }
                        }
                    }
                    .frame(maxHeight: 150)
                }
            }
            .padding(8)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

// MARK: - Measurement Row
private struct MeasurementRow: View {
    let measurement: MeasurementToolsManager.Measurement
    let onDelete: () -> Void
    
    var body: some View {
        HStack {
            Image(systemName: measurement.tool.icon)
                .foregroundColor(measurement.color)
                .frame(width: 20)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(measurement.label)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(measurement.formattedValue)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            
            Spacer()
            
            Button(action: onDelete) {
                Image(systemName: "trash")
                    .foregroundColor(.red)
            }
            .buttonStyle(.plain)
        }
        .padding(4)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}

// MARK: - Annotation Row
private struct AnnotationRow: View {
    let annotation: MeasurementToolsManager.Annotation
    let onDelete: () -> Void
    let onToggleVisibility: () -> Void
    
    var body: some View {
        HStack {
            Button(action: onToggleVisibility) {
                Image(systemName: annotation.isVisible ? "eye" : "eye.slash")
                    .foregroundColor(annotation.isVisible ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textDisabled)
            }
            .buttonStyle(.plain)
            
            Text(annotation.text)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .lineLimit(2)
            
            Spacer()
            
            Button(action: onDelete) {
                Image(systemName: "trash")
                    .foregroundColor(.red)
            }
            .buttonStyle(.plain)
        }
        .padding(4)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
        .opacity(annotation.isVisible ? 1.0 : 0.5)
    }
}
