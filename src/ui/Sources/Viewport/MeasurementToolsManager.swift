import SwiftUI
import simd
import UniformTypeIdentifiers

// MARK: - Viewport Renderer Protocol
protocol ViewportRenderer {
    func drawLine(from: SIMD3<Float>, to: SIMD3<Float>, color: Color, thickness: Float)
    func renderSphere(center: SIMD3<Float>, radius: Float, color: Color)
    func renderCircle(center: SIMD3<Float>, radius: Float, color: Color, thickness: Float)
    func renderArc(center: SIMD3<Float>, radius: Float, startAngle: Float, endAngle: Float, color: Color, thickness: Float)
    func renderPolygon(points: [SIMD3<Float>], color: Color)
    func renderText3D(text: String, position: SIMD3<Float>, color: Color, fontSize: Float, backgroundColor: Color, isBillboard: Bool)
}

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
        
        // Unit conversion implementation
        func convert(value: Float, to target: MeasurementUnit) -> Float {
            return convertValue(value, from: self, to: target)
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
        var isHighlighted: Bool = false // For hover effects
        var showFloatingLabel: Bool = true // Display as floating label
        
        var formattedValue: String {
            if tool == .angle {
                return String(format: "%.2f°", value)
            } else {
                return String(format: "%.2f %@", value, unit.abbreviation)
            }
        }
        
        /// Returns measurement data for CSV export
        var csvData: [String] {
            return [
                tool.rawValue,
                String(value),
                unit.abbreviation,
                label,
                timestamp.ISO8601String(),
                formattedValue
            ]
        }
        
        /// Renders measurement visualization in viewport
        func renderInViewport(renderer: ViewportRenderer) {
            switch tool {
            case .distance:
                renderDistanceMeasurement(renderer: renderer)
            case .angle:
                renderAngleMeasurement(renderer: renderer)
            case .area:
                renderAreaMeasurement(renderer: renderer)
            case .volume:
                renderVolumeMeasurement(renderer: renderer)
            case .radius:
                renderRadiusMeasurement(renderer: renderer)
            case .arcLength:
                renderArcLengthMeasurement(renderer: renderer)
            case .perimeter:
                renderPerimeterMeasurement(renderer: renderer)
            }
        }
        
        /// Renders distance measurement as a line with endpoints
        private func renderDistanceMeasurement(renderer: ViewportRenderer) {
            guard points.count >= 2 else { return }
            
            let start = points[0]
            let end = points[1]
            let lineColor = isHighlighted ? Color.orange : color
            
            // Render main measurement line
            renderer.drawLine(
                from: start,
                to: end,
                color: lineColor,
                thickness: isHighlighted ? 3.0 : 2.0
            )
            
            // Render endpoint markers
            renderer.renderSphere(
                center: start,
                radius: 0.05,
                color: lineColor
            )
            renderer.renderSphere(
                center: end,
                radius: 0.05,
                color: lineColor
            )
            
            // Render floating label if enabled
            if showFloatingLabel {
                let midpoint = (start + end) * 0.5
                renderFloatingLabel(
                    renderer: renderer,
                    text: formattedValue,
                    position: midpoint,
                    color: lineColor
                )
            }
        }
        
        /// Renders angle measurement as an arc with lines
        private func renderAngleMeasurement(renderer: ViewportRenderer) {
            guard points.count >= 3 else { return }
            
            let point1 = points[0]
            let vertex = points[1]
            let point2 = points[2]
            let arcColor = isHighlighted ? Color.orange : color
            
            // Render lines from vertex to points
            renderer.drawLine(
                from: vertex,
                to: point1,
                color: arcColor,
                thickness: isHighlighted ? 3.0 : 2.0
            )
            renderer.drawLine(
                from: vertex,
                to: point2,
                color: arcColor,
                thickness: isHighlighted ? 3.0 : 2.0
            )
            
            // Render angle arc
            let v1 = simd_normalize(point1 - vertex)
            let v2 = simd_normalize(point2 - vertex)
            let angle = acos(simd_dot(v1, v2))
            let arcRadius: Float = 0.5
            
            renderer.renderArc(
                center: vertex,
                radius: arcRadius,
                startAngle: atan2(v1.z, v1.x),
                endAngle: atan2(v2.z, v2.x),
                color: arcColor,
                thickness: isHighlighted ? 3.0 : 2.0
            )
            
            // Render vertex marker
            renderer.renderSphere(
                center: vertex,
                radius: 0.05,
                color: arcColor
            )
            
            // Render floating label if enabled
            if showFloatingLabel {
                let labelDirection = simd_normalize((v1 + v2) * 0.5)
                let labelPosition = vertex + labelDirection * (arcRadius + 0.2)
                renderFloatingLabel(
                    renderer: renderer,
                    text: formattedValue,
                    position: labelPosition,
                    color: arcColor
                )
            }
        }
        
        /// Renders area measurement as a filled polygon
        private func renderAreaMeasurement(renderer: ViewportRenderer) {
            guard points.count >= 3 else { return }
            
            let polygonColor = isHighlighted ? Color.orange : color
            
            // Render polygon outline
            for i in 0..<points.count {
                let next = (i + 1) % points.count
                renderer.drawLine(
                    from: points[i],
                    to: points[next],
                    color: polygonColor,
                    thickness: isHighlighted ? 3.0 : 2.0
                )
            }
            
            // Render semi-transparent fill
            renderer.renderPolygon(
                points: points,
                color: polygonColor.opacity(0.2)
            )
            
            // Render floating label at centroid
            if showFloatingLabel {
                let centroid = points.reduce(SIMD3<Float>(0, 0, 0), +) / Float(points.count)
                renderFloatingLabel(
                    renderer: renderer,
                    text: formattedValue,
                    position: centroid,
                    color: polygonColor
                )
            }
        }
        
        /// Renders volume measurement as a bounding box
        private func renderVolumeMeasurement(renderer: ViewportRenderer) {
            guard points.count >= 2 else { return }
            
            let boxColor = isHighlighted ? Color.orange : color
            let minPoint = points[0]
            let maxPoint = points[1]
            
            // Render bounding box edges
            let corners = [
                SIMD3<Float>(minPoint.x, minPoint.y, minPoint.z),
                SIMD3<Float>(maxPoint.x, minPoint.y, minPoint.z),
                SIMD3<Float>(maxPoint.x, maxPoint.y, minPoint.z),
                SIMD3<Float>(minPoint.x, maxPoint.y, minPoint.z),
                SIMD3<Float>(minPoint.x, minPoint.y, maxPoint.z),
                SIMD3<Float>(maxPoint.x, minPoint.y, maxPoint.z),
                SIMD3<Float>(maxPoint.x, maxPoint.y, maxPoint.z),
                SIMD3<Float>(minPoint.x, maxPoint.y, maxPoint.z)
            ]
            
            let edges = [
                (0, 1), (1, 2), (2, 3), (3, 0), // Bottom face
                (4, 5), (5, 6), (6, 7), (7, 4), // Top face
                (0, 4), (1, 5), (2, 6), (3, 7)  // Vertical edges
            ]
            
            for edge in edges {
                renderer.drawLine(
                    from: corners[edge.0],
                    to: corners[edge.1],
                    color: boxColor,
                    thickness: isHighlighted ? 3.0 : 2.0
                )
            }
            
            // Render floating label at center
            if showFloatingLabel {
                let center = (minPoint + maxPoint) * 0.5
                renderFloatingLabel(
                    renderer: renderer,
                    text: formattedValue,
                    position: center,
                    color: boxColor
                )
            }
        }
        
        /// Renders radius measurement as a circle
        private func renderRadiusMeasurement(renderer: ViewportRenderer) {
            guard points.count >= 4 else { return }
            
            let circleColor = isHighlighted ? Color.orange : color
            let center = points[3]
            let radius = simd_distance(center, points[0])
            
            // Render circle
            renderer.renderCircle(
                center: center,
                radius: radius,
                color: circleColor,
                thickness: isHighlighted ? 3.0 : 2.0
            )
            
            // Render radius line
            renderer.drawLine(
                from: center,
                to: points[0],
                color: circleColor,
                thickness: isHighlighted ? 3.0 : 2.0
            )
            
            // Render center point
            renderer.renderSphere(
                center: center,
                radius: 0.05,
                color: circleColor
            )
            
            // Render floating label
            if showFloatingLabel {
                let labelPosition = center + SIMD3<Float>(radius + 0.2, 0, 0)
                renderFloatingLabel(
                    renderer: renderer,
                    text: formattedValue,
                    position: labelPosition,
                    color: circleColor
                )
            }
        }
        
        /// Renders arc length measurement
        private func renderArcLengthMeasurement(renderer: ViewportRenderer) {
            guard points.count >= 3 else { return }
            
            let arcColor = isHighlighted ? Color.orange : color
            let center = points[0]
            let start = points[1]
            let end = points[2]
            
            let radius = simd_distance(center, start)
            let startAngle = atan2(start.z - center.z, start.x - center.x)
            let endAngle = atan2(end.z - center.z, end.x - center.x)
            
            // Render arc
            renderer.renderArc(
                center: center,
                radius: radius,
                startAngle: startAngle,
                endAngle: endAngle,
                color: arcColor,
                thickness: isHighlighted ? 3.0 : 2.0
            )
            
            // Render radius lines
            renderer.drawLine(
                from: center,
                to: start,
                color: arcColor,
                thickness: 1.0
            )
            renderer.drawLine(
                from: center,
                to: end,
                color: arcColor,
                thickness: 1.0
            )
            
            // Render center point
            renderer.renderSphere(
                center: center,
                radius: 0.05,
                color: arcColor
            )
            
            // Render floating label at arc midpoint
            if showFloatingLabel {
                let midAngle = (startAngle + endAngle) * 0.5
                let labelPosition = center + SIMD3<Float>(
                    cos(midAngle) * (radius + 0.2),
                    0,
                    sin(midAngle) * (radius + 0.2)
                )
                renderFloatingLabel(
                    renderer: renderer,
                    text: formattedValue,
                    position: labelPosition,
                    color: arcColor
                )
            }
        }
        
        /// Renders perimeter measurement
        private func renderPerimeterMeasurement(renderer: ViewportRenderer) {
            guard points.count >= 3 else { return }
            
            let perimeterColor = isHighlighted ? Color.orange : color
            
            // Render polygon outline (similar to area but without fill)
            for i in 0..<points.count {
                let next = (i + 1) % points.count
                renderer.drawLine(
                    from: points[i],
                    to: points[next],
                    color: perimeterColor,
                    thickness: isHighlighted ? 3.0 : 2.0
                )
            }
            
            // Render vertex markers
            for point in points {
                renderer.renderSphere(
                    center: point,
                    radius: 0.03,
                    color: perimeterColor
                )
            }
            
            // Render floating label at centroid
            if showFloatingLabel {
                let centroid = points.reduce(SIMD3<Float>(0, 0, 0), +) / Float(points.count)
                renderFloatingLabel(
                    renderer: renderer,
                    text: formattedValue,
                    position: centroid,
                    color: perimeterColor
                )
            }
        }
        
        /// Renders 3D text label
        private func renderFloatingLabel(renderer: ViewportRenderer, text: String, position: SIMD3<Float>, color: Color) {
            renderer.renderText3D(
                text: text,
                position: position,
                color: color,
                fontSize: 14,
                backgroundColor: Color.black.opacity(0.7),
                isBillboard: true
            )
        }
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
        var isBillboard: Bool = true // Always face camera
        var isRichText: Bool = false // Support rich text formatting
        var textLines: [String] = [] // Support multiple lines
        
        init(text: String, position: SIMD3<Float>) {
            self.text = text
            self.position = position
            self.textLines = text.components(separatedBy: "\n")
        }
        
        /// Updates text and splits into lines
        mutating func updateText(_ newText: String) {
            self.text = newText
            self.textLines = newText.components(separatedBy: "\n")
        }
        
        /// Returns formatted text for display
        var displayText: String {
            return textLines.joined(separator: "\n")
        }
        
        /// Returns single-line text for CSV export
        var csvText: String {
            return text.replacingOccurrences(of: "\n", with: "\\n")
        }
        
        /// Renders annotation in 3D space
        func renderInViewport(renderer: ViewportRenderer) {
            if isVisible {
                renderer.renderText3D(
                    text: displayText,
                    position: position,
                    color: color,
                    fontSize: fontSize,
                    backgroundColor: backgroundColor,
                    isBillboard: isBillboard
                )
            }
        }
    }
    
    // MARK: - Functions
    
    // MARK: - 3D Math Utilities
    
    /// Calculates distance between two 3D points
    private func calculateDistance(from: SIMD3<Float>, to: SIMD3<Float>) -> Float {
        return simd_distance(from, to)
    }
    
    /// Calculates angle between three points in degrees
    private func calculateAngle(point1: SIMD3<Float>, vertex: SIMD3<Float>, point2: SIMD3<Float>) -> Float {
        let v1 = simd_normalize(point1 - vertex)
        let v2 = simd_normalize(point2 - vertex)
        let dot = simd_dot(v1, v2)
        // Clamp to avoid domain errors due to floating point precision
        let clampedDot = max(-1.0, min(1.0, dot))
        return acos(clampedDot) * (180.0 / Float.pi)
    }
    
    /// Calculates area of a 3D polygon using cross product method
    private func calculatePolygonArea(points: [SIMD3<Float>]) -> Float {
        guard points.count >= 3 else { return 0.0 }
        
        var area: Float = 0.0
        let normal = calculatePolygonNormal(points: points)
        
        // Project points onto plane perpendicular to normal
        for i in 0..<points.count {
            let j = (i + 1) % points.count
            let cross = simd_cross(points[i], points[j])
            area += simd_dot(cross, normal)
        }
        
        return abs(area) * 0.5
    }
    
    /// Calculates normal vector of a polygon
    private func calculatePolygonNormal(points: [SIMD3<Float>]) -> SIMD3<Float> {
        guard points.count >= 3 else { return SIMD3<Float>(0, 1, 0) }
        
        let v1 = points[1] - points[0]
        let v2 = points[2] - points[0]
        return simd_normalize(simd_cross(v1, v2))
    }
    
    /// Calculates volume of a closed mesh using signed tetrahedron method
    private func calculateMeshVolume(vertices: [SIMD3<Float>], indices: [Int]?) -> Float {
        guard vertices.count >= 3 else { return 0.0 }
        
        var volume: Float = 0.0
        
        if let indices = indices {
            // Use provided indices for triangles
            guard indices.count % 3 == 0 else { return 0.0 }
            
            for i in stride(from: 0, to: indices.count, by: 3) {
                let v0 = vertices[indices[i]]
                let v1 = vertices[indices[i + 1]]
                let v2 = vertices[indices[i + 2]]
                
                // Signed tetrahedron volume with origin
                volume += simd_dot(v0, simd_cross(v1, v2))
            }
        } else {
            // Assume vertices form triangles sequentially
            for i in stride(from: 0, to: vertices.count - 2, by: 3) {
                let v0 = vertices[i]
                let v1 = vertices[i + 1]
                let v2 = vertices[i + 2]
                
                volume += simd_dot(v0, simd_cross(v1, v2))
            }
        }
        
        return abs(volume) / 6.0
    }
    
    /// Calculates circle center and radius from 3 points
    private func calculateCircleFromPoints(p1: SIMD3<Float>, p2: SIMD3<Float>, p3: SIMD3<Float>) -> (center: SIMD3<Float>, radius: Float)? {
        // Project points onto 2D plane (use XY plane for simplicity)
        let a = SIMD2<Float>(p1.x, p1.y)
        let b = SIMD2<Float>(p2.x, p2.y)
        let c = SIMD2<Float>(p3.x, p3.y)
        
        // Calculate perpendicular bisectors
        let ab = b - a
        let bc = c - b
        
        // Check if points are collinear
        let cross = ab.x * bc.y - ab.y * bc.x
        if abs(cross) < 1e-6 {
            return nil // Points are collinear
        }
        
        // Calculate circle center using perpendicular bisector intersection
        let abMid = (a + b) * 0.5
        let bcMid = (b + c) * 0.5
        
        let abPerp = SIMD2<Float>(-ab.y, ab.x)
        let bcPerp = SIMD2<Float>(-bc.y, bc.x)
        
        // Solve for intersection
        let denominator = abPerp.x * bcPerp.y - abPerp.y * bcPerp.x
        if abs(denominator) < 1e-6 {
            return nil
        }
        
        let t = ((bcMid.x - abMid.x) * bcPerp.y - (bcMid.y - abMid.y) * bcPerp.x) / denominator
        let center2D = abMid + abPerp * t
        
        let center = SIMD3<Float>(center2D.x, center2D.y, (p1.z + p2.z + p3.z) / 3.0)
        let radius = simd_distance(center, p1)
        
        return (center: center, radius: radius)
    }
    
    // MARK: - Unit Conversion
    
    /// Converts value between measurement units
    func convertValue(_ value: Float, from: MeasurementUnit, to: MeasurementUnit) -> Float {
        if from == to { return value }
        
        // Convert to meters first (base unit)
        let inMeters: Float
        switch from {
        case .meters: inMeters = value
        case .centimeters: inMeters = value / 100.0
        case .millimeters: inMeters = value / 1000.0
        case .kilometers: inMeters = value * 1000.0
        case .feet: inMeters = value * 0.3048
        case .inches: inMeters = value * 0.0254
        case .yards: inMeters = value * 0.9144
        case .miles: inMeters = value * 1609.344
        }
        
        // Convert from meters to target unit
        switch to {
        case .meters: return inMeters
        case .centimeters: return inMeters * 100.0
        case .millimeters: return inMeters * 1000.0
        case .kilometers: return inMeters / 1000.0
        case .feet: return inMeters / 0.3048
        case .inches: return inMeters / 0.0254
        case .yards: return inMeters / 0.9144
        case .miles: return inMeters / 1609.344
        }
    }
    
    func selectTool(_ tool: MeasurementTool?) {
        activeTool = tool
        if let tool = tool {
            print("[Measurements] Selected tool: \(tool.rawValue)")
        } else {
            print("[Measurements] Deselected tool")
        }
    }
    
    /// Measures distance between two 3D points
    func measureDistance(from: SIMD3<Float>, to: SIMD3<Float>) -> Measurement {
        let distance = calculateDistance(from: from, to: to)
        let convertedDistance = convertValue(distance, from: .meters, to: measurementUnit)
        
        let measurement = Measurement(
            tool: .distance,
            points: [from, to],
            value: convertedDistance,
            unit: measurementUnit,
            label: "Distance"
        )
        measurements.append(measurement)
        print("[Measurements] Distance: \(measurement.formattedValue)")
        return measurement
    }
    
    /// Measures angle between three points in degrees
    func measureAngle(point1: SIMD3<Float>, vertex: SIMD3<Float>, point2: SIMD3<Float>) -> Measurement {
        let angle = calculateAngle(point1: point1, vertex: vertex, point2: point2)
        
        let measurement = Measurement(
            tool: .angle,
            points: [point1, vertex, point2],
            value: angle,
            unit: .meters, // Angles use degrees, not distance units
            label: "Angle"
        )
        measurements.append(measurement)
        print("[Measurements] Angle: \(String(format: "%.2f", angle))°")
        return measurement
    }
    
    /// Measures area of a polygon defined by points
    func measureArea(points: [SIMD3<Float>]) -> Measurement {
        let area = calculatePolygonArea(points: points)
        let convertedArea = convertValue(area, from: .meters, to: measurementUnit)
        
        let measurement = Measurement(
            tool: .area,
            points: points,
            value: convertedArea,
            unit: measurementUnit,
            label: "Area"
        )
        measurements.append(measurement)
        print("[Measurements] Area: \(measurement.formattedValue)")
        return measurement
    }
    
    /// Measures volume of a closed mesh
    func measureVolume(meshVertices: [SIMD3<Float>], meshIndices: [Int]? = nil) -> Measurement {
        let volume = calculateMeshVolume(vertices: meshVertices, indices: meshIndices)
        let convertedVolume = convertValue(volume, from: .meters, to: measurementUnit)
        
        let measurement = Measurement(
            tool: .volume,
            points: meshVertices,
            value: convertedVolume,
            unit: measurementUnit,
            label: "Volume"
        )
        measurements.append(measurement)
        print("[Measurements] Volume: \(measurement.formattedValue)")
        return measurement
    }
    
    /// Measures radius of circle defined by 3 points
    func measureRadius(p1: SIMD3<Float>, p2: SIMD3<Float>, p3: SIMD3<Float>) -> Measurement {
        guard let circleData = calculateCircleFromPoints(p1: p1, p2: p2, p3: p3) else {
            print("[Measurements] Failed to calculate circle: points are collinear")
            let measurement = Measurement(
                tool: .radius,
                points: [p1, p2, p3],
                value: 0.0,
                unit: measurementUnit,
                label: "Radius (Error)"
            )
            measurements.append(measurement)
            return measurement
        }
        
        let convertedRadius = convertValue(circleData.radius, from: .meters, to: measurementUnit)
        
        let measurement = Measurement(
            tool: .radius,
            points: [p1, p2, p3, circleData.center],
            value: convertedRadius,
            unit: measurementUnit,
            label: "Radius"
        )
        measurements.append(measurement)
        print("[Measurements] Radius: \(measurement.formattedValue)")
        return measurement
    }
    
    /// Measures arc length defined by center and two points
    func measureArcLength(center: SIMD3<Float>, startPoint: SIMD3<Float>, endPoint: SIMD3<Float>) -> Measurement {
        let radius = simd_distance(center, startPoint)
        let startVector = simd_normalize(startPoint - center)
        let endVector = simd_normalize(endPoint - center)
        let angle = acos(simd_dot(startVector, endVector))
        let arcLength = radius * angle
        let convertedLength = convertValue(arcLength, from: .meters, to: measurementUnit)
        
        let measurement = Measurement(
            tool: .arcLength,
            points: [center, startPoint, endPoint],
            value: convertedLength,
            unit: measurementUnit,
            label: "Arc Length"
        )
        measurements.append(measurement)
        print("[Measurements] Arc Length: \(measurement.formattedValue)")
        return measurement
    }
    
    /// Measures perimeter of a polygon defined by points
    func measurePerimeter(points: [SIMD3<Float>]) -> Measurement {
        guard points.count >= 3 else {
            let measurement = Measurement(
                tool: .perimeter,
                points: points,
                value: 0.0,
                unit: measurementUnit,
                label: "Perimeter (Error)"
            )
            measurements.append(measurement)
            return measurement
        }
        
        var perimeter: Float = 0.0
        for i in 0..<points.count {
            let next = (i + 1) % points.count
            perimeter += calculateDistance(from: points[i], to: points[next])
        }
        
        let convertedPerimeter = convertValue(perimeter, from: .meters, to: measurementUnit)
        
        let measurement = Measurement(
            tool: .perimeter,
            points: points,
            value: convertedPerimeter,
            unit: measurementUnit,
            label: "Perimeter"
        )
        measurements.append(measurement)
        print("[Measurements] Perimeter: \(measurement.formattedValue)")
        return measurement
    }
    func addAnnotation(text: String, position: SIMD3<Float>, color: Color = .white, fontSize: Float = 14, isBillboard: Bool = true) {
        var annotation = Annotation(text: text, position: position)
        annotation.color = color
        annotation.fontSize = fontSize
        annotation.isBillboard = isBillboard
        annotations.append(annotation)
        print("[Measurements] Added annotation at \(position): \(text)")
    }
    
    /// Updates existing annotation
    func updateAnnotation(_ id: UUID, text: String? = nil, position: SIMD3<Float>? = nil, color: Color? = nil, fontSize: Float? = nil) {
        guard let index = annotations.firstIndex(where: { $0.id == id }) else { return }
        
        if let text = text {
            annotations[index].updateText(text)
        }
        if let position = position {
            annotations[index].position = position
        }
        if let color = color {
            annotations[index].color = color
        }
        if let fontSize = fontSize {
            annotations[index].fontSize = fontSize
        }
    }
    
    /// Renders all measurements and annotations in the viewport
    func renderAllInViewport(renderer: ViewportRenderer, mousePosition: CGPoint? = nil) {
        // Update hover states based on mouse position
        if let mousePos = mousePosition {
            updateHoverStates(mousePosition: mousePos, renderer: renderer)
        }
        
        // Render all measurements
        for measurement in measurements {
            measurement.renderInViewport(renderer: renderer)
        }
        
        // Render all annotations
        for annotation in annotations {
            annotation.renderInViewport(renderer: renderer)
        }
    }
    
    /// Updates hover states for measurements based on mouse position
    private func updateHoverStates(mousePosition: CGPoint, renderer: ViewportRenderer) {
        // This would require viewport-specific ray casting to determine
        // which measurement is under the mouse cursor
        // For now, we'll implement a simple distance-based check
        
        for i in measurements.indices {
            measurements[i].isHighlighted = false
        }
        
        // TODO: Implement proper ray-casting hover detection
        // This would involve:
        // 1. Converting mouse position to 3D ray
        // 2. Testing intersection with measurement geometries
        // 3. Highlighting the closest measurement within threshold
    }
    
    /// Highlights a specific measurement by ID
    func highlightMeasurement(_ id: UUID, highlight: Bool = true) {
        if let index = measurements.firstIndex(where: { $0.id == id }) {
            measurements[index].isHighlighted = highlight
        }
    }
    
    /// Gets measurement under mouse cursor
    func getMeasurementUnderMouse(mousePosition: CGPoint, renderer: ViewportRenderer) -> Measurement? {
        // TODO: Implement ray-casting to find measurement under cursor
        // This would require access to the camera and viewport information
        return nil
    }
    func getCurrentCursor3DPosition(viewportSize: CGSize, mousePosition: CGPoint, cameraTransform: simd_float4x4, projectionMatrix: simd_float4x4) -> SIMD3<Float>? {
        // Convert mouse position to normalized device coordinates
        let ndcX = (2.0 * mousePosition.x / viewportSize.width) - 1.0
        let ndcY = 1.0 - (2.0 * mousePosition.y / viewportSize.height)
        
        // Create ray in clip space
        let rayClip = SIMD4<Float>(ndcX, ndcY, -1.0, 1.0)
        
        // Transform to eye space
        let invProjection = simd_inverse(projectionMatrix)
        let rayEye = invProjection * rayClip
        let rayEyeNormalized = SIMD4<Float>(rayEye.x, rayEye.y, -1.0, 0.0)
        
        // Transform to world space
        let invView = simd_inverse(cameraTransform)
        let rayWorld = invView * rayEyeNormalized
        
        let rayDirection = simd_normalize(SIMD3<Float>(rayWorld.x, rayWorld.y, rayWorld.z))
        let rayOrigin = SIMD3<Float>(cameraTransform.columns.3.x, cameraTransform.columns.3.y, cameraTransform.columns.3.z)
        
        // For now, return a point at a fixed distance along the ray
        // In a real implementation, this would intersect with the terrain or objects
        let distance: Float = 100.0
        return rayOrigin + rayDirection * distance
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
    
    /// Exports measurements to CSV format
    func exportMeasurements() -> String {
        var csv = "Tool,Value,Unit,Label,Timestamp,FormattedValue\n"
        for measurement in measurements {
            csv += measurement.csvData.joined(separator: ",") + "\n"
        }
        return csv
    }
    
    /// Imports measurements from CSV format
    func importMeasurements(csv: String) {
        print("[Measurements] Importing measurements from CSV")
        
        let lines = csv.components(separatedBy: .newlines)
        guard lines.count > 1 else {
            print("[Measurements] CSV file is empty or invalid")
            return
        }
        
        var importedCount = 0
        var errorCount = 0
        
        // Skip header line
        for line in lines.dropFirst() {
            let trimmedLine = line.trimmingCharacters(in: .whitespacesAndNewlines)
            guard !trimmedLine.isEmpty else { continue }
            
            let columns = trimmedLine.components(separatedBy: ",")
            guard columns.count >= 6 else {
                print("[Measurements] Invalid CSV line: \(trimmedLine)")
                errorCount += 1
                continue
            }
            
            // Parse CSV data
            guard let tool = MeasurementTool(rawValue: columns[0]),
                  let value = Float(columns[1]),
                  let unit = MeasurementUnit(rawValue: columns[2]) else {
                print("[Measurements] Failed to parse CSV line: \(trimmedLine)")
                errorCount += 1
                continue
            }
            
            let label = columns[3]
            let timestampString = columns[4]
            
            // Create measurement (points would need to be reconstructed from saved data)
            let measurement = Measurement(
                tool: tool,
                points: [], // Points not stored in CSV for simplicity
                value: value,
                unit: unit,
                label: label
            )
            
            measurements.append(measurement)
            successCount += 1
        }
        return csv
    }
    
    /// Exports measurements to CSV string
    func exportMeasurements() -> String {
        var csv = "ID,Tool,Value,Unit,Label,Timestamp\n"
        
        for measurement in measurements {
            csv += "\(measurement.id.uuidString),\(measurement.tool.rawValue),\(measurement.value),\(measurement.unit.rawValue),\(measurement.label),\(measurement.timestamp)\n"
        }
        
        return csv
    }
    
    /// Exports annotations to CSV string
    func exportAnnotations() -> String {
        var csv = "ID,Text,Position X,Position Y,Position Z,Color,FontSize,IsBillboard,IsVisible,Timestamp\n"
        
        for annotation in annotations {
            let position = annotation.position
            csv += "\(annotation.id.uuidString),\(annotation.csvText),\(position.x),\(position.y),\(position.z),\(annotation.color.description),\(annotation.fontSize),\(annotation.isBillboard),\(annotation.isVisible),\(annotation.timestamp)\n"
        }
        
        return csv
    }
    
    /// Exports measurements to CSV and saves to file
    func exportMeasurementsToFile() -> URL? {
        let csv = exportMeasurements()
        
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        let timestamp = dateFormatter.string(from: Date())
        let filename = "measurements_\(timestamp).csv"
        
        do {
            let documentsURL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
            let fileURL = documentsURL.appendingPathComponent(filename)
            try csv.write(to: fileURL, atomically: true, encoding: .utf8)
            print("[Measurements] Exported measurements to: \(fileURL.path)")
            return fileURL
        } catch {
            print("[Measurements] Error exporting measurements: \(error)")
            return nil
        }
    }
    
    /// Exports annotations to CSV and saves to file
    func exportAnnotationsToFile() -> URL? {
        let csv = exportAnnotations()
        
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        let timestamp = dateFormatter.string(from: Date())
        let filename = "annotations_\(timestamp).csv"
        
        do {
            let documentsURL = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
            let fileURL = documentsURL.appendingPathComponent(filename)
            try csv.write(to: fileURL, atomically: true, encoding: .utf8)
            print("[Measurements] Exported annotations to: \(fileURL.path)")
            return fileURL
        } catch {
            print("[Measurements] Error exporting annotations: \(error)")
            return nil
        }
    }
    
    /// Imports measurements from CSV file
    func importMeasurementsFromFile(url: URL) -> Int {
        do {
            let csv = try String(contentsOf: url, encoding: .utf8)
            let result = importMeasurements(csv: csv)
            print("[Measurements] Imported \(result.successCount) measurements from: \(url.path)")
            if result.errorCount > 0 {
                print("[Measurements] Import errors: \(result.errorCount)")
            }
            return result.successCount
        } catch {
            print("[Measurements] Error importing measurements: \(error)")
            return 0
        }
    }
    
    /// Imports annotations from CSV file
    func importAnnotationsFromFile(url: URL) -> Int {
        do {
            let csv = try String(contentsOf: url, encoding: .utf8)
            let result = importAnnotations(csv: csv)
            print("[Measurements] Imported \(result.successCount) annotations from: \(url.path)")
            if result.errorCount > 0 {
                print("[Measurements] Import errors: \(result.errorCount)")
            }
            return result.successCount
        } catch {
            print("[Measurements] Error importing annotations: \(error)")
            return 0
        }
    }
    
    /// Imports measurements from CSV format
    func importMeasurements(csv: String) -> (successCount: Int, errorCount: Int) {
        print("[Measurements] Importing measurements from CSV")
        
        let lines = csv.components(separatedBy: .newlines)
        guard lines.count > 1 else {
            print("[Measurements] CSV file is empty or invalid")
            return (0, 0)
        }
        
        var successCount = 0
        var errorCount = 0
        
        // Skip header line
        for line in lines.dropFirst() {
            let trimmedLine = line.trimmingCharacters(in: .whitespacesAndNewlines)
            guard !trimmedLine.isEmpty else { continue }
            
            let columns = trimmedLine.components(separatedBy: ",")
            guard columns.count >= 5 else {
                print("[Measurements] Invalid CSV line: \(trimmedLine)")
                errorCount += 1
                continue
            }
            
            // Parse CSV data
            guard let tool = MeasurementTool(rawValue: columns[1]),
                  let value = Float(columns[2]),
                  let unit = MeasurementUnit(rawValue: columns[3]) else {
                print("[Measurements] Failed to parse CSV line: \(trimmedLine)")
                errorCount += 1
                continue
            }
            
            let label = columns[4]
            
            // Create measurement (points would need to be reconstructed from saved data)
            let measurement = Measurement(
                tool: tool,
                points: [], // Points not stored in CSV for simplicity
                value: value,
                unit: unit,
                label: label
            )
            
            measurements.append(measurement)
            successCount += 1
        }
        
        return (successCount, errorCount)
    }
    
    /// Imports annotations from CSV format
    func importAnnotations(csv: String) -> (successCount: Int, errorCount: Int) {
        print("[Measurements] Importing annotations from CSV")
        
        let lines = csv.components(separatedBy: .newlines)
        guard lines.count > 1 else {
            print("[Measurements] CSV file is empty or invalid")
            return (0, 0)
        }
        
        var successCount = 0
        var errorCount = 0
        
        // Skip header line
        for line in lines.dropFirst() {
            let trimmedLine = line.trimmingCharacters(in: .whitespacesAndNewlines)
            guard !trimmedLine.isEmpty else { continue }
            
            let columns = trimmedLine.components(separatedBy: ",")
            guard columns.count >= 9 else {
                print("[Measurements] Invalid CSV line: \(trimmedLine)")
                errorCount += 1
                continue
            }
            
            // Parse CSV data
            let text = columns[1].replacingOccurrences(of: "\\n", with: "\n")
            guard let posX = Float(columns[2]),
                  let posY = Float(columns[3]),
                  let posZ = Float(columns[4]),
                  let fontSize = Float(columns[6]),
                  let isVisible = Bool(columns[7]),
                  let isBillboard = Bool(columns[8]) else {
                print("[Measurements] Failed to parse CSV line: \(trimmedLine)")
                errorCount += 1
                continue
            }
            
            // Parse color (RGBA)
            let colorComponents = columns[5].components(separatedBy: ",")
            var color: Color = .white
            if colorComponents.count == 4 {
                if let r = Double(colorComponents[0]),
                   let g = Double(colorComponents[1]),
                   let b = Double(colorComponents[2]) {
                    color = Color(red: r, green: g, blue: b)
                }
            }
            
            var annotation = Annotation(text: text, position: SIMD3<Float>(posX, posY, posZ))
            annotation.color = color
            annotation.fontSize = fontSize
            annotation.isVisible = isVisible
            annotation.isBillboard = isBillboard
            
            annotations.append(annotation)
            successCount += 1
        }
        
        return (successCount, errorCount)
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
                HStack {
                    Button(action: {
                        showingMeasurements.toggle()
                    }) {
                        HStack {
                            Text("Measurements (\(manager.measurements.count))")
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Spacer()
                            
                            Image(systemName: showingMeasurements ? "chevron.up" : "chevron.down")
                                .font(.system(size: 10))
                        }
                    }
                    .buttonStyle(.plain)
                    
                    Spacer()
                    
                    HStack(spacing: 8) {
                        if manager.measurements.count > 0 {
                            Button(action: {
                                if let fileURL = manager.exportMeasurementsToFile() {
                                    // Show success message or share sheet
                                    print("[Measurements] Measurements exported successfully")
                                }
                            }) {
                                Image(systemName: "square.and.arrow.up")
                                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                            }
                            .buttonStyle(.plain)
                            .help("Export measurements")
                            
                            Button(action: {
                                manager.clearAllMeasurements()
                            }) {
                                Image(systemName: "trash")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                            .help("Clear all measurements")
                        }
                    }
                }
                
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
                HStack {
                    Button(action: {
                        showingAnnotations.toggle()
                    }) {
                        HStack {
                            Text("Annotations (\(manager.annotations.count))")
                                .font(DesignSystem.Typography.bodyBold)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Spacer()
                            
                            Image(systemName: showingAnnotations ? "chevron.up" : "chevron.down")
                                .font(.system(size: 10))
                        }
                    }
                    .buttonStyle(.plain)
                    
                    Spacer()
                    
                    HStack(spacing: 8) {
                        if manager.annotations.count > 0 {
                            Button(action: {
                                if let fileURL = manager.exportAnnotationsToFile() {
                                    // Show success message or share sheet
                                    print("[Measurements] Annotations exported successfully")
                                }
                            }) {
                                Image(systemName: "square.and.arrow.up")
                                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                            }
                            .buttonStyle(.plain)
                            .help("Export annotations")
                            
                            Button(action: {
                                manager.clearAllAnnotations()
                            }) {
                                Image(systemName: "trash")
                                    .foregroundColor(.red)
                            }
                            .buttonStyle(.plain)
                            .help("Clear all annotations")
                        }
                    }
                }
                
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
                                // TODO: Get current cursor 3D position - using placeholder for now
                                let position = getCurrentCursor3DPosition(
                                    viewportSize: CGSize(width: 800, height: 600),
                                    mousePosition: CGPoint(x: 400, y: 300),
                                    cameraTransform: simd_float4x4(diagonal: SIMD4<Float>(1, 1, 1, 1)),
                                    projectionMatrix: simd_float4x4(diagonal: SIMD4<Float>(1, 1, 1, 1))
                                ) ?? SIMD3<Float>(0, 0, 0)
                                manager.addAnnotation(text: newAnnotationText, position: position)
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
