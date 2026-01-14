import SwiftUI
import simd

// MARK: - Boat Physics Component
/// Core boat physics properties for realistic water interaction

struct BoatPhysicsComponent: Component {
    let id = UUID()
    var componentType: EditorComponentType { .boat }
    var isEnabled = true
    
    // Physical Properties
    var mass: Float = 500.0  // kg
    var waterlineHeight: Float = 0.0  // Height of waterline when at rest
    var hullDragCoefficient: Float = 0.15  // Water resistance
    var hullLiftCoefficient: Float = 0.08  // Hydrodynamic lift at speed
    
    // Buoyancy System (Archimedes Principle)
    var buoyancyPoints: [SIMD3<Float>] = []  // Sample points on hull for buoyancy calculation
    var displacementVolume: Float = 2.5  // m - Volume of water displaced
    var buoyancyDamping: Float = 0.3  // Damping for buoyancy oscillation
    
    // Hull Shape (for wave interaction)
    var hullLength: Float = 3.0  // meters
    var hullWidth: Float = 1.2   // meters (beam)
    var hullDepth: Float = 0.8   // meters (draft)
    
    // Stability
    var centerOfMass: SIMD3<Float> = .zero
    var centerOfBuoyancy: SIMD3<Float> = SIMD3<Float>(0, -0.5, 0)
    var metacentricHeight: Float = 0.8  // Stability measure (higher = more stable)
    
    // Wake Generation
    var generateWake: Bool = true
    var wakeIntensity: Float = 1.0
    var bowWaveHeight: Float = 0.3  // Height of bow wave at speed
    
    // Current/Tide Interaction
    var affectedByCurrents: Bool = true
    var currentResistance: Float = 1.0  // How much currents affect boat
    
    func toEngineBridge() -> ComponentBridgeData {
        ComponentBridgeData(
            type: "BoatPhysicsComponent",
            data: [
                "mass": String(mass),
                "waterlineHeight": String(waterlineHeight),
                "dragCoefficient": String(hullDragCoefficient),
                "displacementVolume": String(displacementVolume)
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> BoatPhysicsComponent? {
        var component = BoatPhysicsComponent()
        if let massStr = data.data["mass"] as? String, let mass = Float(massStr) {
            component.mass = mass
        }
        return component
    }
}

// MARK: - Propeller Component
/// Propeller thrust generation with blade element theory

struct PropellerComponent: Component {
    let id = UUID()
    var componentType: EditorComponentType { .boat }
    var isEnabled = true
    
    var boneName: String = "Propeller"
    var type: PropellerType = .threeBladeFixed  // NEW: Propeller type selection
    
    // Propeller Geometry (from type)
    var diameter: Float {
        type.typicalDiameter
    }
    var pitch: Float = 0.25  // meters (distance traveled per revolution)
    var bladeCount: Int {
        type.bladeCount
    }
    var bladeArea: Float = 0.05  // m per blade
    
    // Performance (from type)
    var efficiency: Float {
        type.efficiency
    }
    var cavitationThreshold: Float {
        type.cavitationThreshold
    }
    
    // State (updated by physics system)
    var rpm: Float = 0.0  // Current rotation speed
    var thrust: Float = 0.0  // Newtons
    var torque: Float = 0.0  // Nm
    var powerOutput: Float = 0.0  // Watts
    
    // Animation
    var rotationAngle: Float = 0.0  // Current rotation for rendering
    
    /// Calculate thrust using blade element momentum theory with propeller type characteristics
    /// F_thrust =     n  D  K_t
    func calculateThrust(waterDensity: Float = 1000.0, advanceRatio: Float) -> Float {
        let rps = rpm / 60.0  // Revolutions per second
        
        // Thrust coefficient (varies with advance ratio and propeller type)
        let kt = type.thrustCoefficient * (1.0 - 0.5 * advanceRatio)
        
        // Check for cavitation (reduces thrust)
        var cavitationFactor: Float = 1.0
        if rpm > cavitationThreshold {
            cavitationFactor = cavitationThreshold / rpm
            print("[Propeller] Cavitation occurring at \(Int(rpm)) RPM (threshold: \(Int(cavitationThreshold)))")
        }
        
        return efficiency * waterDensity * pow(rps, 2) * pow(diameter, 4) * kt * cavitationFactor
    }
    
    /// Calculate propeller wash (affects rudder and surrounding water)
    func calculatePropwash() -> SIMD3<Float> {
        let washVelocity = rpm * pitch / 60.0  // m/s
        return SIMD3<Float>(0, 0, -washVelocity)  // Backward flow
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        ComponentBridgeData(
            type: "PropellerComponent",
            data: [
                "boneName": boneName,
                "diameter": String(diameter),
                "rpm": String(rpm)
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> PropellerComponent? {
        var component = PropellerComponent()
        if let boneName = data.data["boneName"] as? String {
            component.boneName = boneName
        }
        return component
    }
}

// MARK: - Rudder Component
/// Rudder steering with hydrodynamic forces

struct RudderComponent: Component {
    let id = UUID()
    var componentType: EditorComponentType { .boat }
    var isEnabled = true
    
    var boneName: String = "Rudder"
    
    // Geometry
    var area: Float = 0.15  // m - Rudder surface area
    var chordLength: Float = 0.4  // meters
    var span: Float = 0.6  // meters
    var aspectRatio: Float { span / chordLength }
    
    // Control
    var angle: Float = 0.0  // radians, current rudder angle
    var maxAngle: Float = .pi / 4  // 45 degrees
    var turnRate: Float = .pi / 2  // rad/s - How fast rudder can turn
    var targetAngle: Float = 0.0  // Desired angle (for smooth control)
    
    // Hydrodynamics
    var liftCoefficient: Float = 1.2  // Varies with angle of attack
    var dragCoefficient: Float = 0.05  // Parasitic drag
    var stallAngle: Float = 0.35  // radians (~20)
    
    /// Calculate lateral force from rudder deflection
    /// F = 0.5    v  A  C_l  sin()
    func calculateLateralForce(velocity: Float, waterDensity: Float = 1000.0, propwashBoost: Float = 1.0) -> Float {
        // Effective velocity includes propeller wash
        let effectiveVelocity = velocity * propwashBoost
        
        // Check for stall
        var effectiveLift = liftCoefficient
        if abs(angle) > stallAngle {
            effectiveLift *= (1.0 - (abs(angle) - stallAngle) / stallAngle)  // Reduce lift after stall
        }
        
        // Lateral force
        let force = 0.5 * waterDensity * pow(effectiveVelocity, 2) * area * effectiveLift * sin(angle)
        return force
    }
    
    /// Calculate drag from rudder
    func calculateDrag(velocity: Float, waterDensity: Float = 1000.0) -> Float {
        let dragArea = area * abs(sin(angle))  // Projected area
        return 0.5 * waterDensity * pow(velocity, 2) * dragArea * dragCoefficient
    }
    
    /// Update rudder angle smoothly toward target
    mutating func updateAngle(deltaTime: Float) {
        let delta = targetAngle - angle
        let maxChange = turnRate * deltaTime
        
        if abs(delta) < maxChange {
            angle = targetAngle
        } else {
            angle += sign(delta) * maxChange
        }
        
        // Clamp to limits
        angle = max(-maxAngle, min(maxAngle, angle))
    }
    
    private func sign(_ value: Float) -> Float {
        value >= 0 ? 1.0 : -1.0
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        ComponentBridgeData(
            type: "RudderComponent",
            data: [
                "boneName": boneName,
                "angle": String(angle)
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> RudderComponent? {
        var component = RudderComponent()
        if let boneName = data.data["boneName"] as? String {
            component.boneName = boneName
        }
        return component
    }
}

// MARK: - Sail Component
/// Sail aerodynamics with wind interaction

struct SailComponent: Component {
    let id = UUID()
    var componentType: EditorComponentType { .boat }
    var isEnabled = true
    
    var boneName: String = "MainSail"
    
    // Geometry
    var area: Float = 8.0  // m - Sail surface area
    var aspectRatio: Float = 2.5  // Height / Width
    var camber: Float = 0.1  // Sail curvature (0-1)
    
    // Control
    var sheeting: Float = 0.0  // 0-1, how tight the sail is (0=luffing, 1=close-hauled)
    var reefing: Float = 1.0   // 0-1, how much sail is deployed (1=full, 0=furled)
    var targetSheeting: Float = 0.5
    
    // Aerodynamics
    var liftCoefficient: Float = 1.5
    var dragCoefficient: Float = 0.05
    var centerOfEffort: SIMD3<Float> = SIMD3<Float>(0, 2.0, 0)  // Point where force acts
    
    // State
    var isLuffing: Bool = false  // Sail flapping in wind
    
    /// Calculate force from wind
    /// Apparent wind = true wind - boat velocity
    mutating func calculateForce(
        windSpeed: SIMD3<Float>,
        boatVelocity: SIMD3<Float>,
        boatHeading: Float,
        airDensity: Float = 1.225
    ) -> SIMD3<Float> {
        // Apparent wind
        let apparentWind = windSpeed - boatVelocity
        let windMagnitude = length(apparentWind)
        
        guard windMagnitude > 0.1 else { return .zero }
        
        let windDirection = normalize(apparentWind)
        
        // Angle of attack (angle between wind and sail)
        let sailAngle = boatHeading + (sheeting * .pi / 2)  // Simplified
        let windAngle = atan2(windDirection.x, windDirection.z)
        var angleOfAttack = windAngle - sailAngle
        
        // Normalize to - to 
        while angleOfAttack > .pi { angleOfAttack -= 2 * .pi }
        while angleOfAttack < -.pi { angleOfAttack += 2 * .pi }
        
        // Check for luffing (sail not catching wind)
        if abs(angleOfAttack) < 0.2 {  // ~11 degrees
            isLuffing = true
            return .zero
        } else {
            isLuffing = false
        }
        
        // Lift and drag coefficients vary with angle of attack
        let effectiveLift = liftCoefficient * sin(2 * angleOfAttack) * (1.0 + camber)
        let effectiveDrag = dragCoefficient + liftCoefficient * pow(sin(angleOfAttack), 2)
        
        // Effective area with reefing
        let effectiveArea = area * reefing
        
        // Dynamic pressure
        let dynamicPressure = 0.5 * airDensity * pow(windMagnitude, 2)
        
        // Lift (perpendicular to apparent wind)
        let liftMagnitude = dynamicPressure * effectiveArea * effectiveLift
        
        // Drag (parallel to apparent wind)
        let dragMagnitude = dynamicPressure * effectiveArea * effectiveDrag
        
        // Combine into total force
        let perpendicular = SIMD3<Float>(-windDirection.z, 0, windDirection.x)  // 90 to wind
        let totalForce = windDirection * dragMagnitude + perpendicular * liftMagnitude
        
        return totalForce
    }
    
    /// Calculate heeling moment (boat tipping from sail force)
    func calculateHeelingMoment(force: SIMD3<Float>) -> Float {
        // Torque = force  distance (center of effort height)
        return length(force) * centerOfEffort.y
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        ComponentBridgeData(
            type: "SailComponent",
            data: [
                "boneName": boneName,
                "area": String(area),
                "sheeting": String(sheeting)
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> SailComponent? {
        var component = SailComponent()
        if let boneName = data.data["boneName"] as? String {
            component.boneName = boneName
        }
        return component
    }
}
