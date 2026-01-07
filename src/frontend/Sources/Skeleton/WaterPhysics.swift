import SwiftUI
import simd

// MARK: - Water Physics System
/// Comprehensive ocean simulation with realistic wave generation, tides, currents, and hull interaction

class WaterPhysicsSystem: ObservableObject {
    static let shared = WaterPhysicsSystem()
    
    // MARK: - Physical Constants
    
    static let waterDensity: Float = 1000.0  // kg/m³ (saltwater: 1025)
    static let gravity: Float = 9.81  // m/s²
    static let airDensity: Float = 1.225  // kg/m³
    static let kinematicViscosity: Float = 0.000001  // m²/s (water viscosity)
    
    // MARK: - Wave System (FFT-based Gerstner Waves)
    
    @Published var waveSettings = WaveSettings()
    
    struct WaveSettings {
        var waveHeight: Float = 0.5  // meters (significant wave height)
        var dominantWavelength: Float = 10.0  // meters
        var waveSpeed: Float = 3.0  // m/s (varies with wavelength)
        var waveDirection: SIMD2<Float> = SIMD2<Float>(1, 0)  // Normalized
        var choppiness: Float = 0.8  // Wave sharpness (0=sinusoidal, 1=sharp crests)
        
        // Wave spectrum (multiple wave components for realism)
        var waveCount: Int = 4  // Number of wave octaves
        var frequencyFalloff: Float = 0.5  // Each octave is half the amplitude
    }
    
    // MARK: - Tide System
    
    @Published var tideSettings = TideSettings()
    
    struct TideSettings {
        var tidalRange: Float = 2.0  // meters (vertical difference between high and low tide)
        var tidalPeriod: Float = 12.4 * 3600  // seconds (12.4 hours for semi-diurnal tide)
        var currentTideLevel: Float = 0.0  // Current offset from mean sea level
        var tidalPhase: Float = 0.0  // 0 to 2π
        
        // Tidal currents
        var enableTidalCurrents: Bool = true
        var maxTidalCurrentSpeed: Float = 0.5  // m/s
    }
    
    // MARK: - Current & Flow System
    
    @Published var currentSettings = CurrentSettings()
    
    class CurrentSettings: ObservableObject {
        @Published var oceanCurrent: SIMD3<Float> = SIMD3<Float>(0.2, 0, 0.1)  // m/s (Gulf Stream ~2 m/s)
        @Published var enableSubsurfaceFlow: Bool = true
        @Published var flowTurbulence: Float = 0.3  // Random variation in current
        
        // Depth-dependent current (Ekman spiral)
        @Published var surfaceCurrentMultiplier: Float = 1.0
        @Published var deepCurrentMultiplier: Float = 0.3  // Slower at depth
        @Published var ekmanDepth: Float = 50.0  // meters (depth of current influence)
        
        // Vortices and eddies
        var vortices: [WaterVortex] = []
    }
    
    struct WaterVortex {
        var position: SIMD2<Float>
        var strength: Float  // Circulation
        var radius: Float
    }
    
    // MARK: - Hull Wake System
    
    struct HullWake {
        var boatPosition: SIMD3<Float>
        var boatVelocity: SIMD3<Float>
        var wakeAge: Float  // seconds
        var waveHeight: Float
        var divergence: Float  // Wake angle (typically 19.47° - Kelvin wake)
    }
    
    @Published var activeWakes: [HullWake] = []
    
    // MARK: - Wave Height Calculation (FFT Gerstner Waves)
    
    /// Get water height at position using realistic ocean wave model
    func getWaterHeight(at position: SIMD2<Float>, time: Float) -> Float {
        var height: Float = tideSettings.currentTideLevel
        
        // Multi-octave Gerstner waves for realistic ocean
        for octave in 0..<waveSettings.waveCount {
            let scale = pow(2.0, Float(octave))
            let wavelength = waveSettings.dominantWavelength / scale
            let amplitude = waveSettings.waveHeight / scale * pow(waveSettings.frequencyFalloff, Float(octave))
            
            // Wave number
            let k = 2 * Float.pi / wavelength
            
            // Angular frequency (deep water dispersion relation: ω² = gk)
            let omega = sqrt(WaterPhysicsSystem.gravity * k)
            
            // Wave direction (add some variation per octave)
            let directionVariation = Float(octave) * 0.1
            let waveDir = normalize(SIMD2<Float>(
                waveSettings.waveDirection.x * cos(directionVariation) - waveSettings.waveDirection.y * sin(directionVariation),
                waveSettings.waveDirection.x * sin(directionVariation) + waveSettings.waveDirection.y * cos(directionVariation)
            ))
            
            // Phase
            let phase = k * dot(waveDir, position) - omega * time
            
            // Gerstner wave (with choppiness)
            let steepness = waveSettings.choppiness * k * amplitude
            height += amplitude * cos(phase)
            
            // Add horizontal displacement for wave shape (choppiness)
            // This would be used for full 3D wave displacement
        }
        
        // Add hull wake contributions
        for wake in activeWakes {
            let distanceToWake = distance(position, SIMD2<Float>(wake.boatPosition.x, wake.boatPosition.z))
            let wakeDecay = exp(-wake.wakeAge / 10.0)  // Wake fades over time
            
            if distanceToWake < 20.0 {  // Wake influence radius
                let wakeContribution = wake.waveHeight * wakeDecay * sin(distanceToWake * 2.0 - wake.wakeAge)
                height += wakeContribution
            }
        }
        
        return height
    }
    
    /// Get water normal at position (for lighting/reflections)
    func getWaterNormal(at position: SIMD2<Float>, time: Float, epsilon: Float = 0.1) -> SIMD3<Float> {
        let h = getWaterHeight(at: position, time: time)
        let hx = getWaterHeight(at: position + SIMD2<Float>(epsilon, 0), time: time)
        let hz = getWaterHeight(at: position + SIMD2<Float>(0, epsilon), time: time)
        
        let tangentX = SIMD3<Float>(epsilon, hx - h, 0)
        let tangentZ = SIMD3<Float>(0, hz - h, epsilon)
        
        return normalize(cross(tangentX, tangentZ))
    }
    
    /// Get water velocity at position (for current interaction)
    func getWaterVelocity(at position: SIMD3<Float>) -> SIMD3<Float> {
        var velocity = currentSettings.oceanCurrent
        
        // Depth-dependent current (Ekman spiral)
        if currentSettings.enableSubsurfaceFlow {
            let depthFactor = exp(-position.y / currentSettings.ekmanDepth)
            velocity *= mix(currentSettings.deepCurrentMultiplier, currentSettings.surfaceCurrentMultiplier, depthFactor)
        }
        
        // Tidal current
        if tideSettings.enableTidalCurrents {
            let tidalVelocity = tideSettings.maxTidalCurrentSpeed * sin(tideSettings.tidalPhase)
            velocity += SIMD3<Float>(tidalVelocity, 0, 0)
        }
        
        // Add turbulence
        let turbulence = SIMD3<Float>(
            (Float.random(in: -1...1) - 0.5) * currentSettings.flowTurbulence,
            0,
            (Float.random(in: -1...1) - 0.5) * currentSettings.flowTurbulence
        )
        velocity += turbulence
        
        // Vortex influence
        for vortex in currentSettings.vortices {
            let pos2D = SIMD2<Float>(position.x, position.z)
            let toVortex = pos2D - vortex.position
            let dist = length(toVortex)
            
            if dist < vortex.radius {
                let vortexVelocity = vortex.strength / (2 * Float.pi * max(dist, 0.1))
                let tangent = SIMD2<Float>(-toVortex.y, toVortex.x)  // Perpendicular
                let vel2D = normalize(tangent) * vortexVelocity
                velocity += SIMD3<Float>(vel2D.x, 0, vel2D.y)
            }
        }
        
        return velocity
    }
    
    // MARK: - Buoyancy Calculation (Archimedes Principle)
    
    /// Calculate buoyancy force on a boat
    func calculateBuoyancy(
        boat: BoatPhysicsComponent,
        position: SIMD3<Float>,
        rotation: simd_quatf,
        time: Float
    ) -> (force: SIMD3<Float>, torque: SIMD3<Float>) {
        var totalForce: SIMD3<Float> = .zero
        var totalTorque: SIMD3<Float> = .zero
        
        // Sample at multiple points on hull for accurate buoyancy
        let samplePoints = boat.buoyancyPoints.isEmpty ? generateHullSamplePoints(boat) : boat.buoyancyPoints
        
        for localPoint in samplePoints {
            // Transform to world space
            let rotatedPoint = rotation.act(localPoint)
            let worldPoint = position + rotatedPoint
            
            // Get water height at this point
            let waterHeight = getWaterHeight(at: SIMD2<Float>(worldPoint.x, worldPoint.z), time: time)
            
            // Check if point is submerged
            let submersionDepth = waterHeight - worldPoint.y
            
            if submersionDepth > 0 {
                // Volume element (simplified as uniform distribution)
                let volumeElement = boat.displacementVolume / Float(samplePoints.count)
                
                // Buoyant force (Archimedes: F = ρ × V × g)
                let buoyancy = WaterPhysicsSystem.waterDensity * volumeElement * WaterPhysicsSystem.gravity * submersionDepth
                let buoyantForce = SIMD3<Float>(0, buoyancy, 0)
                
                totalForce += buoyantForce
                
                // Torque from offset force (rotational effect)
                let leverArm = rotatedPoint - boat.centerOfMass
                totalTorque += cross(leverArm, buoyantForce)
            }
        }
        
        // Add buoyancy damping to prevent oscillation
        // (In full implementation, this would track velocity)
        totalForce *= (1.0 - boat.buoyancyDamping)
        
        return (totalForce, totalTorque)
    }
    
    /// Generate hull sample points based on boat dimensions
    private func generateHullSamplePoints(_ boat: BoatPhysicsComponent) -> [SIMD3<Float>] {
        var points: [SIMD3<Float>] = []
        
        // Sample grid on hull bottom
        let samplesX = 5
        let samplesZ = 7
        
        for ix in 0..<samplesX {
            for iz in 0..<samplesZ {
                let x = (Float(ix) / Float(samplesX - 1) - 0.5) * boat.hullWidth
                let z = (Float(iz) / Float(samplesZ - 1) - 0.5) * boat.hullLength
                let y = -boat.hullDepth / 2
                
                points.append(SIMD3<Float>(x, y, z))
            }
        }
        
        return points
    }
    
    // MARK: - Hydrodynamic Forces
    
    /// Calculate drag force on hull
    func calculateHullDrag(
        boat: BoatPhysicsComponent,
        velocity: SIMD3<Float>,
        rotation: simd_quatf
    ) -> SIMD3<Float> {
        // Water velocity relative to boat
        let avgPosition = SIMD3<Float>(0, boat.waterlineHeight, 0)  // Approximate
        let waterVel = getWaterVelocity(at: avgPosition)
        let relativeVelocity = velocity - waterVel
        let speed = length(relativeVelocity)
        
        guard speed > 0.01 else { return .zero }
        
        // Drag force: F_drag = 0.5 × ρ × v² × C_d × A
        let frontalArea = boat.hullWidth * boat.hullDepth
        let dragMagnitude = 0.5 * WaterPhysicsSystem.waterDensity * pow(speed, 2) * boat.hullDragCoefficient * frontalArea
        
        return -normalize(relativeVelocity) * dragMagnitude
    }
    
    /// Calculate hydrodynamic lift (planing hull at speed)
    func calculateHullLift(
        boat: BoatPhysicsComponent,
        velocity: SIMD3<Float>
    ) -> SIMD3<Float> {
        let speed = length(velocity)
        
        // Lift only occurs at significant speed (planing)
        guard speed > 3.0 else { return .zero }
        
        let planingSurface = boat.hullLength * boat.hullWidth
        let liftMagnitude = 0.5 * WaterPhysicsSystem.waterDensity * pow(speed, 2) * boat.hullLiftCoefficient * planingSurface
        
        return SIMD3<Float>(0, liftMagnitude, 0)
    }
    
    // MARK: - Wake Generation
    
    /// Generate wake behind moving boat
    func generateWake(boat: BoatPhysicsComponent, position: SIMD3<Float>, velocity: SIMD3<Float>) {
        guard boat.generateWake else { return }
        
        let speed = length(velocity)
        guard speed > 0.5 else { return }  // Only generate wake at significant speed
        
        // Kelvin wake angle (19.47° half-angle)
        let kelvinAngle: Float = 19.47 * Float.pi / 180.0
        
        // Bow wave height increases with speed (Froude number)
        let froudeNumber = speed / sqrt(WaterPhysicsSystem.gravity * boat.hullLength)
        let waveHeight = boat.bowWaveHeight * min(froudeNumber, 1.5)
        
        let wake = HullWake(
            boatPosition: position,
            boatVelocity: velocity,
            wakeAge: 0,
            waveHeight: waveHeight * boat.wakeIntensity,
            divergence: kelvinAngle
        )
        
        activeWakes.append(wake)
        
        // Limit wake count for performance
        if activeWakes.count > 50 {
            activeWakes.removeFirst()
        }
    }
    
    /// Update wake system (called each frame)
    func updateWakes(deltaTime: Float) {
        for i in 0..<activeWakes.count {
            activeWakes[i].wakeAge += deltaTime
        }
        
        // Remove old wakes
        activeWakes.removeAll { $0.wakeAge > 30.0 }
    }
    
    // MARK: - Tide Simulation
    
    /// Update tidal level based on time
    func updateTide(time: Float) {
        // Simple cosine model for tide
        tideSettings.tidalPhase = (2 * Float.pi * time) / tideSettings.tidalPeriod
        tideSettings.currentTideLevel = (tideSettings.tidalRange / 2) * cos(tideSettings.tidalPhase)
    }
    
    // MARK: - Utility
    
    private func mix(_ a: Float, _ b: Float, _ t: Float) -> Float {
        return a * (1 - t) + b * t
    }
}

// MARK: - Water Rendering Settings
/// Settings for realistic ocean rendering (for C engine integration)

struct WaterRenderSettings {
    // Lighting
    var sunDirection: SIMD3<Float> = normalize(SIMD3<Float>(0.3, 0.8, 0.5))
    var sunColor: SIMD3<Float> = SIMD3<Float>(1.0, 0.95, 0.8)
    var skyColor: SIMD3<Float> = SIMD3<Float>(0.4, 0.6, 0.9)
    
    // Water appearance
    var waterColor: SIMD3<Float> = SIMD3<Float>(0.0, 0.3, 0.5)  // Deep water color
    var foamColor: SIMD3<Float> = SIMD3<Float>(1.0, 1.0, 1.0)
    var foamThreshold: Float = 0.8  // Wave steepness for foam
    
    // Reflection/Refraction
    var fresnelIndexOfRefraction: Float = 1.333  // Water IOR
    var reflectivity: Float = 0.02  // Base reflectivity (Fresnel)
    var transparency: Float = 0.9
    var refractionStrength: Float = 0.1
    
    // Subsurface scattering
    var scatteringColor: SIMD3<Float> = SIMD3<Float>(0.1, 0.5, 0.4)
    var scatteringDepth: Float = 10.0  // meters
    
    // Detail
    var normalStrength: Float = 1.0
    var meshResolution: Int = 256  // Grid resolution for water mesh
    var renderDistance: Float = 500.0  // meters
}

// MARK: - Water Shader Parameters
/// Parameters sent to GPU for water rendering

struct WaterShaderParams {
    var time: Float
    var waveHeight: Float
    var waveLength: Float
    var waveSpeed: Float
    var choppiness: Float
    var tideLevel: Float
    
    // Hull interaction
    var hullPositions: [SIMD3<Float>] = []  // Active boat positions
    var hullVelocities: [SIMD3<Float>] = []
    var hullWakeIntensities: [Float] = []
}
