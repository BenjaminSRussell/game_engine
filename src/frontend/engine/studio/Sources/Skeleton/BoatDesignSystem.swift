import SwiftUI
import simd

// MARK: - Propeller Type Variants
/// Different propeller designs with unique performance characteristics

enum PropellerType: String, CaseIterable, Identifiable {
    case twoBladeFixed = "2-Blade Fixed Pitch"
    case threeBladeFixed = "3-Blade Fixed Pitch"
    case fourBlade = "4-Blade High Thrust"
    case ducted = "Ducted Propeller"
    case foldable = "Folding (Sailboat)"
    case jetDrive = "Jet Drive"
    
    var id: String { rawValue }
    
    var bladeCount: Int {
        switch self {
        case .twoBladeFixed: return 2
        case .threeBladeFixed: return 3
        case .fourBlade: return 4
        case .ducted: return 5
        case .foldable: return 2
        case .jetDrive: return 0  // Internal impeller
        }
    }
    
    /// Propeller efficiency (0-1)
    var efficiency: Float {
        switch self {
        case .twoBladeFixed: return 0.55
        case .threeBladeFixed: return 0.65
        case .fourBlade: return 0.60  // Slightly less efficient but more thrust
        case .ducted: return 0.75  // Most efficient due to duct
        case .foldable: return 0.50  // Reduced due to hinge mechanisms
        case .jetDrive: return 0.70
        }
    }
    
    /// Thrust coefficient (affects thrust generation)
    var thrustCoefficient: Float {
        switch self {
        case .twoBladeFixed: return 0.25
        case .threeBladeFixed: return 0.30
        case .fourBlade: return 0.40  // Highest thrust
        case .ducted: return 0.50  // Duct increases thrust significantly
        case .foldable: return 0.20
        case .jetDrive: return 0.35
        }
    }
    
    /// RPM threshold where cavitation begins (reduces thrust)
    var cavitationThreshold: Float {
        switch self {
        case .twoBladeFixed: return 1500
        case .threeBladeFixed: return 2000
        case .fourBlade: return 1800
        case .ducted: return 2500  // Best cavitation resistance
        case .foldable: return 1200  // Prone to cavitation
        case .jetDrive: return 999999  // No cavitation (internal)
        }
    }
    
    /// Drag coefficient when not rotating (important for sailing)
    var dragCoefficient: Float {
        switch self {
        case .foldable: return 0.01  // Folds flat against hull
        case .jetDrive: return 0.02  // Flush with hull
        case .twoBladeFixed: return 0.08
        case .threeBladeFixed: return 0.10
        case .fourBlade: return 0.12
        case .ducted: return 0.06  // Duct reduces parasitic drag
        }
    }
    
    /// Noise level (affects stealth/comfort)
    var noiseLevel: Float {
        switch self {
        case .jetDrive: return 0.3  // Quietest
        case .ducted: return 0.4
        case .threeBladeFixed: return 0.6
        case .fourBlade: return 0.7
        case .twoBladeFixed: return 0.8
        case .foldable: return 0.5
        }
    }
    
    /// Typical diameter range for this propeller type
    var typicalDiameter: Float {
        switch self {
        case .twoBladeFixed: return 0.35
        case .threeBladeFixed: return 0.30
        case .fourBlade: return 0.28  // Smaller diameter, more blades
        case .ducted: return 0.25  // Compact
        case .foldable: return 0.32
        case .jetDrive: return 0.20  // Internal
        }
    }
    
    var description: String {
        switch self {
        case .twoBladeFixed:
            return "Simple, low cost. Good efficiency. Prone to vibration. Best for low-power applications."
        case .threeBladeFixed:
            return "Standard choice. Balanced performance. Good for most boats. Smooth operation."
        case .fourBlade:
            return "Maximum thrust and acceleration. Best for heavy boats. Slightly lower top speed."
        case .ducted:
            return "Highest efficiency and thrust. Protected blades. Excellent for shallow water. Expensive."
        case .foldable:
            return "For sailboats. Folds to reduce drag under sail. Unfolds when motor is used."
        case .jetDrive:
            return "No external propeller. Best for very shallow water. Debris-resistant. Moderate efficiency."
        }
    }
    
    var icon: String {
        switch self {
        case .twoBladeFixed: return "fan.fill"
        case .threeBladeFixed: return "fan.and.light.ceiling.fill"
        case .fourBlade: return "tropicalstorm"
        case .ducted: return "circle.hexagongrid.fill"
        case .foldable: return "arrow.triangle.merge"
        case .jetDrive: return "tornado"
        }
    }
    
    /// 3D model asset path
    var modelAsset: String {
        "models/propellers/\(rawValue.lowercased().replacingOccurrences(of: " ", with: "_").replacingOccurrences(of: "-", with: ""))"
    }
}

// MARK: - Hull Shape Variants
/// Different hull designs with unique hydrodynamic properties

enum HullShape: String, CaseIterable, Identifiable {
    case deepV = "Deep-V Hull"
    case flatBottom = "Flat Bottom"
    case catamaran = "Catamaran"
    case trimaran = "Trimaran"  
    case displacement = "Displacement Hull"
    case planing = "Planing Hull"
    case pontoon = "Pontoon Boat"
    
    var id: String { rawValue }
    
    /// Hydrodynamic drag coefficient
    var dragCoefficient: Float {
        switch self {
        case .planing: return 0.10  // Lowest drag at speed (lifts out of water)
        case .catamaran: return 0.12  // Narrow hulls
        case .trimaran: return 0.13
        case .deepV: return 0.15
        case .displacement: return 0.18
        case .flatBottom: return 0.25
        case .pontoon: return 0.30  // Highest drag (blunt fronts)
        }
    }
    
    /// Hydrodynamic lift coefficient (planing capability)
    var liftCoefficient: Float {
        switch self {
        case .planing: return 0.15  // Designed to lift
        case .flatBottom: return 0.08
        case .deepV: return 0.05
        default: return 0.0  // Displacement hulls don't plane
        }
    }
    
    /// Stability factor (metacentric height multiplier)
    var stabilityFactor: Float {
        switch self {
        case .catamaran: return 2.0  // Extremely stable (wide beam)
        case .trimaran: return 1.8
        case .pontoon: return 1.5
        case .flatBottom: return 1.2
        case .deepV: return 1.0  // Baseline
        case .planing: return 0.8  // Less stable at rest
        case .displacement: return 0.9
        }
    }
    
    /// Maximum theoretical speed (knots)
    var maxSpeed: Float {
        switch self {
        case .planing: return 50  // Can plane efficiently
        case .deepV: return 40
        case .catamaran: return 35
        case .trimaran: return 32
        case .flatBottom: return 25
        case .pontoon: return 15
        case .displacement: return 8  // Limited by hull speed: 1.34 * sqrt(waterline length)
        }
    }
    
    /// Wave handling capability (1-10)
    var roughWaterRating: Int {
        switch self {
        case .deepV: return 9  // Excellent
        case .catamaran: return 7
        case .trimaran: return 8
        case .displacement: return 6
        case .planing: return 5
        case .flatBottom: return 3
        case .pontoon: return 2  // Poor in waves
        }
    }
    
    /// Shallow water capability (minimum draft)
    var minDraft: Float {
        switch self {
        case .flatBottom: return 0.2  // Very shallow
        case .pontoon: return 0.3
        case .planing: return 0.4
        case .catamaran: return 0.5
        case .trimaran: return 0.5
        case .deepV: return 0.8
        case .displacement: return 1.2  // Deepest
        }
    }
    
    /// Interior space multiplier
    var spaceEfficiency: Float {
        switch self {
        case .pontoon: return 2.0  // Maximum deck space
        case .catamaran: return 1.6
        case .trimaran: return 1.4
        case .flatBottom: return 1.3
        case .planing: return 1.0
        case .deepV: return 0.9
        case .displacement: return 0.8
        }
    }
    
    var description: String {
        switch self {
        case .deepV:
            return "V-shaped bottom. Best for rough water. Cuts through waves. Smooth ride at speed. Most popular for offshore boats."
        case .flatBottom:
            return "Flat planing surface. Excellent for calm water and shallow areas. Fast acceleration. Rough in waves."
        case .catamaran:
            return "Twin hulls. Very stable platform. Low drag. Great interior space. Popular for sailing and power boats."
        case .trimaran:
            return "Three hulls. Balance of speed and stability. Efficient. Good in rough water. Maximum deck space."
        case .displacement:
            return "Traditional round hull. Smooth, comfortable ride. Limited by hull speed. Very fuel efficient. Best for trawlers."
        case .planing:
            return "Designed to rise up and skim water surface. Maximum speed. Fuel hungry. Needs power to perform."
        case .pontoon:
            return "Flat deck on cylindrical floats. Maximum stability and space. Perfect for calm water recreation. Slow but comfortable."
        }
    }
    
    var icon: String {
        switch self {
        case .deepV: return "triangle.fill"
        case .flatBottom: return "rectangle.fill"
        case .catamaran: return "equal.square.fill"
        case .trimaran: return "square.grid.3x1.fill"
        case .displacement: return "circle.fill"
        case .planing: return "arrow.up.right"
        case .pontoon: return "cylinder.fill"
        }
    }
    
    /// Recommended use cases
    var bestFor: [String] {
        switch self {
        case .deepV: return ["Offshore fishing", "Rough water", "High speed cruising"]
        case .flatBottom: return ["Shallow rivers", "Calm lakes", "Speed on flat water"]
        case .catamaran: return ["Sailing", "Stable platform", "Long-distance cruising"]
        case .trimaran: return ["Speed sailing", "Expedition", "Live-aboard"]
        case .displacement: return ["Trawlers", "Long-range cruising", "Fuel efficiency"]
        case .planing: return ["High-speed transport", "Sport fishing", "Racing"]
        case .pontoon: return ["Leisure", "Parties", "Calm water recreation"]
        }
    }
}

// MARK: - Hull Parameters
/// Customizable parameters for hull generation

struct HullParameters {
    var shape: HullShape = .deepV
    
    // Dimensions
    var length: Float = 6.0  // meters (LOA - length overall)
    var beam: Float = 2.0    // meters (maximum width)
    var draft: Float = 0.8   // meters (depth below waterline)
    var freeboard: Float = 0.5  // meters (height above waterline)
    
    // Shape parameters
    var deadrise: Float = 20.0  // degrees (V-angle at transom for V-hulls)
    var bowDeadrise: Float = 45.0  // degrees (V-angle at bow)
    var chineAngle: Float = 10.0  // degrees (hard chine angle)
    
    // Hull form
    var entranceAngle: Float = 30.0  // degrees (bow entry angle)
    var transomWidth: Float = 1.6  // meters (stern width, usually < beam)
    
    /// Calculate displacement volume (how much water the hull displaces)
    var displacementVolume: Float {
        // Simplified prismatic coefficient model
        let prismCoeff: Float = {
            switch shape {
            case .displacement: return 0.56  // Traditional
            case .planing: return 0.48  // Fine ends
            case .flatBottom: return 0.70  // Boxy
            case .deepV: return 0.52
            case .catamaran: return 0.50  // Per hull
            case .trimaran: return 0.50
            case .pontoon: return 0.80  // Cylinders
            }
        }()
        
        let maxArea = beam * draft
        return length * maxArea * prismCoeff
    }
    
    /// Generate 3D mesh vertices for hull
    func generateMesh() -> ([SIMD3<Float>], [UInt32]) {
        var vertices: [SIMD3<Float>] = []
        var indices: [UInt32] = []
        
        let lengthSections = 30  // Longitudinal sections
        let circumferencePoints = 24  // Points around hull
        
        for i in 0...lengthSections {
            let z = (Float(i) / Float(lengthSections) - 0.5) * length
            let sectionPos = Float(i) / Float(lengthSections)  // 0 to 1
            
            // Width varies along length
            let widthScale = calculateWidthAt(position: sectionPos)
            let sectionBeam = beam * widthScale
            
            for j in 0..<circumferencePoints {
                let angle = Float(j) / Float(circumferencePoints) * 2 * .pi
                
                let (x, y) = calculateHullCrossSection(
                    angle: angle,
                    beam: sectionBeam,
                    position: sectionPos,
                    z: z
                )
                
                vertices.append(SIMD3<Float>(x, y, z))
            }
        }
        
        // Generate triangle indices
        for i in 0..<lengthSections {
            for j in 0..<circumferencePoints {
                let current = UInt32(i * circumferencePoints + j)
                let next = UInt32(i * circumferencePoints + (j + 1) % circumferencePoints)
                let belowCurrent = UInt32((i + 1) * circumferencePoints + j)
                let belowNext = UInt32((i + 1) * circumferencePoints + (j + 1) % circumferencePoints)
                
                // Two triangles per quad
                indices.append(contentsOf: [current, next, belowCurrent])
                indices.append(contentsOf: [next, belowNext, belowCurrent])
            }
        }
        
        return (vertices, indices)
    }
    
    /// Calculate hull width at longitudinal position (0-1)
    private func calculateWidthAt(position: Float) -> Float {
        // Parabolic taper (wider in middle, narrow at ends)
        let x = position * 2 - 1  // -1 (bow) to 1 (stern)
        
        // Bow taper is sharper than stern
        if x < 0 {
            // Bow: quartic curve
            return 1.0 - pow(-x, 3) * 0.8
        } else {
            // Stern: gentler taper
            let sternScale = transomWidth / beam
            return 1.0 - pow(x, 2) * (1.0 - sternScale)
        }
    }
    
    /// Calculate hull cross-section at given angle and position
    private func calculateHullCrossSection(angle: Float, beam: Float, position: Float, z: Float) -> (x: Float, y: Float) {
        let halfBeam = beam / 2
        
        switch shape {
        case .deepV:
            return calculateVHull(angle: angle, halfBeam: halfBeam, position: position)
            
        case .flatBottom:
            return calculateFlatHull(angle: angle, halfBeam: halfBeam)
            
        case .catamaran:
            return calculateCatamaranHull(angle: angle, halfBeam: halfBeam)
            
        case .planing:
            return calculatePlaningHull(angle: angle, halfBeam: halfBeam, position: position)
            
        case .displacement:
            return calculateRoundHull(angle: angle, halfBeam: halfBeam)
            
        case .pontoon:
            return calculatePontoon(angle: angle, halfBeam: halfBeam)
            
        case .trimaran:
            return calculateTrimaranHull(angle: angle, halfBeam: halfBeam)
        }
    }
    
    private func calculateVHull(angle: Float, halfBeam: Float, position: Float) -> (Float, Float) {
        // V-shaped hull with chines
        let x = cos(angle) * halfBeam
        let absAngle = abs(angle - .pi)  // 0 at bottom, pi at sides
        
        // Deadrise varies from bow to stern
        let localDeadrise = mix(bowDeadrise, deadrise, position)
        let deadriseRad = localDeadrise * .pi / 180
        
        let y = -abs(x) * tan(deadriseRad) - draft
        
        return (x, y)
    }
    
    private func calculateFlatHull(angle: Float, halfBeam: Float) -> (Float, Float) {
        let x = cos(angle) * halfBeam
        let y: Float
        
        if abs(angle - .pi) < .pi / 2 {
            // Bottom is flat
            y = -draft
        } else {
            // Sides curve up
            let sideAngle = abs(angle - .pi) - .pi / 2
            y = -draft + abs(sin(sideAngle)) * freeboard
        }
        
        return (x, y)
    }
    
    private func calculateCatamaranHull(angle: Float, halfBeam: Float) -> (Float, Float) {
        // Two separate hulls
        let hullWidth = halfBeam * 0.3  // Each hull is 30% of total beam
        let separation = halfBeam * 0.7  // Gap between hulls
        
        let x = cos(angle) * halfBeam
        
        // Which hull are we on?
        if x > separation / 2 {
            // Right hull
            let localX = (x - separation / 2) / hullWidth
            let y = -sqrt(max(0, 1 - pow(localX * 2, 2))) * draft
            return (x, y)
        } else if x < -separation / 2 {
            // Left hull
            let localX = (x + separation / 2) / hullWidth
            let y = -sqrt(max(0, 1 - pow(localX * 2, 2))) * draft
            return (x, y)
        } else {
            // Deck between hulls
            return (x, 0)
        }
    }
    
    private func calculatePlaningHull(angle: Float, halfBeam: Float, position: Float) -> (Float, Float) {
        // Similar to V-hull but shallower deadrise
        let x = cos(angle) * halfBeam
        let planingDeadrise: Float = 12.0  // Shallower for planing
        let deadriseRad = planingDeadrise * .pi / 180
        
        let y = -abs(x) * tan(deadriseRad) - draft
        return (x, y)
    }
    
    private func calculateRoundHull(angle: Float, halfBeam: Float) -> (Float, Float) {
        // Traditional rounded hull (semi-ellipse)
        let x = cos(angle) * halfBeam
        let y = -sqrt(max(0, 1 - pow(x / halfBeam, 2))) * draft
        return (x, y)
    }
    
    private func calculatePontoon(angle: Float, halfBeam: Float) -> (Float, Float) {
        // Cylindrical pontoons
        let pontoonRadius = draft / 2
        let pontoonSeparation = halfBeam - pontoonRadius
        
        let x = cos(angle) * halfBeam
        
        // Which pontoon?
        if x > 0 {
            let localX = x - pontoonSeparation
            let y = sin(angle) * pontoonRadius - pontoonRadius
            return (pontoonSeparation + cos(angle) * pontoonRadius, y)
        } else {
            let localX = x + pontoonSeparation
            let y = sin(angle) * pontoonRadius - pontoonRadius
            return (-pontoonSeparation + cos(angle) * pontoonRadius, y)
        }
    }
    
    private func calculateTrimaranHull(angle: Float, halfBeam: Float) -> (Float, Float) {
        // Three hulls: one center, two outriggers
        // Simplified version - similar to catamaran but with center hull
        let x = cos(angle) * halfBeam
        
        if abs(x) < halfBeam * 0.3 {
            // Center hull
            let y = -sqrt(max(0, 1 - pow(x / (halfBeam * 0.3), 2))) * draft
            return (x, y)
        } else {
            // Outrigger (smaller)
            let outriggerDraft = draft * 0.6
            let localX = abs(x) - halfBeam * 0.7
            let y = -sqrt(max(0, 1 - pow(localX / (halfBeam * 0.2), 2))) * outriggerDraft
            return (x, y)
        }
    }
    
    private func mix(_ a: Float, _ b: Float, _ t: Float) -> Float {
        a * (1 - t) + b * t
    }
}
