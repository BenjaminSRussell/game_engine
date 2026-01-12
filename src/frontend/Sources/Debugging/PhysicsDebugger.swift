import SwiftUI
import simd

// MARK: - Physics Debugger
// Visual debugging for physics simulation

class PhysicsDebugger: ObservableObject {
    @Published var debugMode: DebugMode = .none
    @Published var showCollisionShapes = false
    @Published var showContactPoints = false
    @Published var showContactNormals = false
    @Published var showVelocityVectors = false
    @Published var showAngularVelocity = false
    @Published var showCenterOfMass = false
    @Published var showInertia = false
    @Published var showConstraints = false
    @Published var showJointLimits = false
    @Published var showSleepingBodies = false
    @Published var showForceVectors = false
    @Published var showBroadphaseAABB = false
    @Published var showPhysicsIslands = false
    
    // Collision filtering
    @Published var collisionFilter: CollisionFilter = .all
    @Published var selectedBody: UUID?
    
    // Performance metrics
    @Published var activeBodyCount: Int = 0
    @Published var sleepingBodyCount: Int = 0
    @Published var collisionPairCount: Int = 0
    @Published var physicsStepTimeMs: Float = 0
    
    // MARK: - Debug Mode
    enum DebugMode: String, CaseIterable, Identifiable {
        case none = "None"
        case basic = "Basic"
        case detailed = "Detailed"
        case advanced = "Advanced"
        
        var id: String { rawValue }
    }
    
    // MARK: - Collision Filter
    enum CollisionFilter: String, CaseIterable {
        case all = "All Collisions"
        case activeOnly = "Active Only"
        case contacts = "Contact Points Only"
        case triggers = "Triggers Only"
    }
    
    // MARK: - Collision Shape Data
    struct CollisionShapeDebugData: Identifiable {
        let id = UUID()
        let entityID: UUID
        let shapeType: ShapeType
        let position: SIMD3<Float>
        let rotation: SIMD3<Float>
        let scale: SIMD3<Float>
        let isActive: Bool
        let isSleeping: Bool
        
        enum ShapeType: String {
            case box = "Box"
            case sphere = "Sphere"
            case capsule = "Capsule"
            case mesh = "Mesh"
            case convex = "Convex"
            case heightField = "Height Field"
        }
    }
    
    // MARK: - Contact Point Data
    struct ContactPointData: Identifiable {
        let id = UUID()
        let position: SIMD3<Float>
        let normal: SIMD3<Float>
        let penetration: Float
        let impulse: Float
        let entity1: UUID
        let entity2: UUID
    }
    
    // MARK: - Constraint Data
    struct ConstraintDebugData: Identifiable {
        let id = UUID()
        let constraintType: ConstraintType
        let entity1: UUID
        let entity2: UUID
        let anchor1: SIMD3<Float>
        let anchor2: SIMD3<Float>
        let limits: (min: Float, max: Float)?
        let currentAngle: Float?
        
        enum ConstraintType: String {
            case hinge = "Hinge"
            case slider = "Slider"
            case fixed = "Fixed"
            case spring = "Spring"
            case ballSocket = "Ball Socket"
            case cone = "Cone"
            case universal = "Universal"
        }
    }
    
    // MARK: - Physics Data Structures
    
    struct BodyProperties: Identifiable {
        let id = UUID()
        let entityID: UUID
        let friction: Float
        let restitution: Float
        let linearDamping: Float
        let angularDamping: Float
        let mass: Float
        let isSleeping: Bool
    }
    
    struct SpringForce: Identifiable {
        let id = UUID()
        let entityID: UUID
        let anchorPoint: SIMD3<Float>
        let restLength: Float
        let stiffness: Float
        let damping: Float
        let currentForce: SIMD3<Float>
    }
    
    struct JointLimitData: Identifiable {
        let id = UUID()
        let constraintID: UUID
        let entity1: UUID
        let entity2: UUID
        let currentAngle: Float
        let minAngle: Float
        let maxAngle: Float
        let limitType: String
    }
    
    struct CollisionPair: Identifiable {
        let id = UUID()
        let entity1: UUID
        let entity2: UUID
        let contactCount: Int
        let totalImpulse: Float
        let maxPenetration: Float
        let isPersistent: Bool
    }
    
    struct BroadphaseAABB: Identifiable {
        let id = UUID()
        let entityID: UUID
        let min: SIMD3<Float>
        let max: SIMD3<Float>
        let isActive: Bool
    }
    
    struct PhysicsIsland: Identifiable {
        let id = UUID()
        let bodies: [UUID]
        let constraints: [UUID]
        let isActive: Bool
        let sleepTime: Float
    }
    
    struct RagdollBone: Identifiable {
        let id = UUID()
        let entityID: UUID
        let boneName: String
        let position: SIMD3<Float>
        let rotation: SIMD3<Float>
        let velocity: SIMD3<Float>
        let isColliding: Bool
    }
    
    struct SoftBodyStress: Identifiable {
        let id = UUID()
        let entityID: UUID
        let vertexIndex: Int
        let stressValue: Float
        let stressDirection: SIMD3<Float>
        let maxStress: Float
    }
    
    struct ClothMesh: Identifiable {
        let id = UUID()
        let entityID: UUID
        let vertices: [SIMD3<Float>]
        let indices: [Int]
        let constraints: [Int]
        let windForce: SIMD3<Float>
    }
    
    struct FluidParticle: Identifiable {
        let id = UUID()
        let position: SIMD3<Float>
        let velocity: SIMD3<Float>
        let pressure: Float
        let density: Float
        let radius: Float
    }
    
    struct BuoyancyForce: Identifiable {
        let id = UUID()
        let entityID: UUID
        let centerOfBuoyancy: SIMD3<Float>
        let buoyancyForce: SIMD3<Float>
        let submergedVolume: Float
        let waterLevel: Float
    }
    
    struct WindForceField: Identifiable {
        let id = UUID()
        let position: SIMD3<Float>
        let force: SIMD3<Float>
        let radius: Float
        let strength: Float
    }
    
    struct GravityVector: Identifiable {
        let id = UUID()
        let position: SIMD3<Float>
        let gravity: SIMD3<Float>
        let magnitude: Float
    }
    
    struct CustomForce: Identifiable {
        let id = UUID()
        let entityID: UUID
        let force: SIMD3<Float>
        let position: SIMD3<Float>
        let forceType: String
        let duration: Float
    }
    
    struct BodyStepTime: Identifiable {
        let id = UUID()
        let entityID: UUID
        let stepTimeMs: Float
        let constraintSolveTime: Float
        let collisionTime: Float
        let integrationTime: Float
    }
    
    struct CollisionMatrixEntry: Identifiable {
        let id = UUID()
        let group1: Int
        let group2: Int
        let shouldCollide: Bool
        let group1Name: String
        let group2Name: String
    }
    
    // MARK: - Functions
    
    // TODO: Implement collision shape visualization (boxes, spheres, capsules, meshes)
    func getCollisionShapes() -> [CollisionShapeDebugData] {
        // Fetch from engine
        return []
    }
    
    // TODO: Implement contact point indicators
    func getContactPoints() -> [ContactPointData] {
        // Fetch from physics engine
        return []
    }
    
    // TODO: Implement contact normal vectors
    func visualizeContactNormals() {
        print("[PhysicsDebug] Visualizing contact normals")
    }
    
    // TODO: Implement collision impulse magnitude display
    // TODO: Implement rigidbody velocity vectors
    func getVelocityVector(for entityID: UUID) -> SIMD3<Float> {
        // Fetch from physics engine
        return SIMD3(0, 0, 0)
    }
    
    // TODO: Implement angular velocity indicators
    func getAngularVelocity(for entityID: UUID) -> SIMD3<Float> {
        return SIMD3(0, 0, 0)
    }
    
    // TODO: Implement center of mass visualization
    func getCenterOfMass(for entityID: UUID) -> SIMD3<Float> {
        return SIMD3(0, 0, 0)
    }
    
    // TODO: Implement inertia tensor visualization
    func getInertiaTensor(for entityID: UUID) -> (SIMD3<Float>, SIMD3<Float>) {
        return (SIMD3(1, 1, 1), SIMD3(0, 0, 0))
    }
    
    // TODO: Implement con straint visualization (hinges, sliders, fixed)
    func getConstraints() -> [ConstraintDebugData] {
        return []
    }
    
    // Joint limits and angles display
    func getJointLimits() -> [JointLimitData] {
        return [
            JointLimitData(
                constraintID: UUID(),
                entity1: UUID(),
                entity2: UUID(),
                currentAngle: 45.0,
                minAngle: -90.0,
                maxAngle: 90.0,
                limitType: "hinge"
            ),
            JointLimitData(
                constraintID: UUID(),
                entity1: UUID(),
                entity2: UUID(),
                currentAngle: 30.0,
                minAngle: 0.0,
                maxAngle: 120.0,
                limitType: "cone"
            )
        ]
    }
    
    // Spring forces visualization
    func getSpringForces() -> [SpringForce] {
        return [
            SpringForce(
                entityID: UUID(),
                anchorPoint: SIMD3<Float>(0, 5, 0),
                restLength: 2.0,
                stiffness: 100.0,
                damping: 5.0,
                currentForce: SIMD3<Float>(0, -50, 0)
            ),
            SpringForce(
                entityID: UUID(),
                anchorPoint: SIMD3<Float>(3, 2, 3),
                restLength: 1.5,
                stiffness: 150.0,
                damping: 8.0,
                currentForce: SIMD3<Float>(-25, 0, -25)
            )
        ]
    }
    
    // Damping visualization
    func getBodyProperties() -> [BodyProperties] {
        return [
            BodyProperties(
                entityID: UUID(),
                friction: 0.7,
                restitution: 0.3,
                linearDamping: 0.05,
                angularDamping: 0.1,
                mass: 10.0,
                isSleeping: false
            ),
            BodyProperties(
                entityID: UUID(),
                friction: 0.5,
                restitution: 0.8,
                linearDamping: 0.02,
                angularDamping: 0.05,
                mass: 5.0,
                isSleeping: true
            )
        ]
    }
    
    // Friction coefficient display
    func getFrictionCoefficients() -> [(UUID, Float)] {
        return [
            (UUID(), 0.7),
            (UUID(), 0.5),
            (UUID(), 0.9),
            (UUID(), 0.3)
        ]
    }
    
    // Restitution (bounciness) display
    func getRestitutionValues() -> [(UUID, Float)] {
        return [
            (UUID(), 0.3),
            (UUID(), 0.8),
            (UUID(), 0.1),
            (UUID(), 0.6)
        ]
    }
    
    // Sleeping bodies highlight
    func getSleepingBodies() -> Set<UUID> {
        return Set([
            UUID(),
            UUID(),
            UUID()
        ])
    }
    
    // Active body count tracking
    func updatePhysicsMetrics(active: Int, sleeping: Int, pairs: Int, stepTime: Float) {
        activeBodyCount = active
        sleepingBodyCount = sleeping
        collisionPairCount = pairs
        physicsStepTimeMs = stepTime
    }
    
    // Collision pair tracking
    func getCollisionPairs() -> [CollisionPair] {
        return [
            CollisionPair(
                entity1: UUID(),
                entity2: UUID(),
                contactCount: 2,
                totalImpulse: 15.5,
                maxPenetration: 0.02,
                isPersistent: true
            ),
            CollisionPair(
                entity1: UUID(),
                entity2: UUID(),
                contactCount: 1,
                totalImpulse: 8.3,
                maxPenetration: 0.01,
                isPersistent: false
            )
        ]
    }
    
    // Broadphase AABB visualization
    func getBroadphaseAABBs() -> [BroadphaseAABB] {
        return [
            BroadphaseAABB(
                entityID: UUID(),
                min: SIMD3<Float>(-1, -1, -1),
                max: SIMD3<Float>(1, 1, 1),
                isActive: true
            ),
            BroadphaseAABB(
                entityID: UUID(),
                min: SIMD3<Float>(4, 0, 4),
                max: SIMD3<Float>(6, 2, 6),
                isActive: false
            ),
            BroadphaseAABB(
                entityID: UUID(),
                min: SIMD3<Float>(-3, 0, -3),
                max: SIMD3<Float>(-1, 3, -1),
                isActive: true
            )
        ]
    }
    
    // Physics island visualization
    func getPhysicsIslands() -> [PhysicsIsland] {
        return [
            PhysicsIsland(
                bodies: [UUID(), UUID(), UUID()],
                constraints: [UUID(), UUID()],
                isActive: true,
                sleepTime: 0.0
            ),
            PhysicsIsland(
                bodies: [UUID(), UUID()],
                constraints: [UUID()],
                isActive: false,
                sleepTime: 2.5
            )
        ]
    }
    
    // Ragdoll debug mode
    func getRagdollBones(for entityID: UUID) -> [RagdollBone] {
        return [
            RagdollBone(
                entityID: entityID,
                boneName: "spine",
                position: SIMD3<Float>(0, 1.2, 0),
                rotation: SIMD3<Float>(0, 0, 0),
                velocity: SIMD3<Float>(0, 0, 0),
                isColliding: false
            ),
            RagdollBone(
                entityID: entityID,
                boneName: "head",
                position: SIMD3<Float>(0, 1.8, 0),
                rotation: SIMD3<Float>(10, 0, 5),
                velocity: SIMD3<Float>(0.5, -0.2, 0.1),
                isColliding: true
            ),
            RagdollBone(
                entityID: entityID,
                boneName: "left_arm",
                position: SIMD3<Float>(-0.8, 1.3, 0),
                rotation: SIMD3<Float>(-20, 30, 0),
                velocity: SIMD3<Float>(-0.3, 0.1, 0.2),
                isColliding: false
            )
        ]
    }
    
    // Soft body stress visualization
    func getSoftBodyStress(for entityID: UUID) -> [SoftBodyStress] {
        return [
            SoftBodyStress(
                entityID: entityID,
                vertexIndex: 0,
                stressValue: 0.3,
                stressDirection: SIMD3<Float>(1, 0, 0),
                maxStress: 1.0
            ),
            SoftBodyStress(
                entityID: entityID,
                vertexIndex: 5,
                stressValue: 0.7,
                stressDirection: SIMD3<Float>(0, 1, 0),
                maxStress: 1.0
            ),
            SoftBodyStress(
                entityID: entityID,
                vertexIndex: 12,
                stressValue: 0.9,
                stressDirection: SIMD3<Float>(0, 0, 1),
                maxStress: 1.0
            )
        ]
    }
    
    // Cloth simulation mesh display
    func getClothMeshes() -> [ClothMesh] {
        return [
            ClothMesh(
                entityID: UUID(),
                vertices: [
                    SIMD3<Float>(0, 2, 0),
                    SIMD3<Float>(1, 2, 0),
                    SIMD3<Float>(0, 2, 1),
                    SIMD3<Float>(1, 2, 1)
                ],
                indices: [0, 1, 2, 1, 3, 2],
                constraints: [0, 1],
                windForce: SIMD3<Float>(0.5, 0, 0.2)
            ),
            ClothMesh(
                entityID: UUID(),
                vertices: [
                    SIMD3<Float>(2, 3, 2),
                    SIMD3<Float>(3, 3, 2),
                    SIMD3<Float>(2, 3, 3),
                    SIMD3<Float>(3, 3, 3)
                ],
                indices: [0, 1, 2, 1, 3, 2],
                constraints: [0, 2],
                windForce: SIMD3<Float>(-0.3, 0, 0.1)
            )
        ]
    }
    
    // Fluid particle visualization
    func getFluidParticles() -> [FluidParticle] {
        return [
            FluidParticle(
                position: SIMD3<Float>(0, 1, 0),
                velocity: SIMD3<Float>(0.5, 0.2, 0.1),
                pressure: 101325.0,
                density: 1000.0,
                radius: 0.1
            ),
            FluidParticle(
                position: SIMD3<Float>(0.2, 1.1, 0),
                velocity: SIMD3<Float>(0.3, 0.1, 0.2),
                pressure: 101320.0,
                density: 998.0,
                radius: 0.1
            ),
            FluidParticle(
                position: SIMD3<Float>(-0.1, 0.9, 0.1),
                velocity: SIMD3<Float>(0.4, 0.3, 0.0),
                pressure: 101330.0,
                density: 1002.0,
                radius: 0.1
            )
        ]
    }
    
    // Buoyancy force vectors
    func getBuoyancyForces() -> [BuoyancyForce] {
        return [
            BuoyancyForce(
                entityID: UUID(),
                centerOfBuoyancy: SIMD3<Float>(0, 0.5, 0),
                buoyancyForce: SIMD3<Float>(0, 98.1, 0),
                submergedVolume: 0.01,
                waterLevel: 1.0
            ),
            BuoyancyForce(
                entityID: UUID(),
                centerOfBuoyancy: SIMD3<Float>(2, 0.3, 1),
                buoyancyForce: SIMD3<Float>(0, 49.05, 0),
                submergedVolume: 0.005,
                waterLevel: 1.0
            )
        ]
    }
    
    // Wind force field visualization
    func getWindForceFields() -> [WindForceField] {
        return [
            WindForceField(
                position: SIMD3<Float>(5, 2, 5),
                force: SIMD3<Float>(2, 0, 1),
                radius: 3.0,
                strength: 0.8
            ),
            WindForceField(
                position: SIMD3<Float>(-3, 3, -2),
                force: SIMD3<Float>(-1, 0.5, -0.5),
                radius: 2.0,
                strength: 0.6
            )
        ]
    }
    
    // Gravity visualization
    func getGravityVectors() -> [GravityVector] {
        return [
            GravityVector(
                position: SIMD3<Float>(0, 5, 0),
                gravity: SIMD3<Float>(0, -9.81, 0),
                magnitude: 9.81
            ),
            GravityVector(
                position: SIMD3<Float>(10, 5, 10),
                gravity: SIMD3<Float>(0, -9.81, 0),
                magnitude: 9.81
            ),
            GravityVector(
                position: SIMD3<Float>(-5, 5, -5),
                gravity: SIMD3<Float>(0, -9.81, 0),
                magnitude: 9.81
            )
        ]
    }
    
    // Custom force visualization
    func getCustomForces() -> [CustomForce] {
        return [
            CustomForce(
                entityID: UUID(),
                force: SIMD3<Float>(5, 0, 0),
                position: SIMD3<Float>(0, 1, 0),
                forceType: "explosion",
                duration: 0.5
            ),
            CustomForce(
                entityID: UUID(),
                force: SIMD3<Float>(0, 10, 0),
                position: SIMD3<Float>(2, 0, 2),
                forceType: "impulse",
                duration: 0.1
            ),
            CustomForce(
                entityID: UUID(),
                force: SIMD3<Float>(-3, 0, -2),
                position: SIMD3<Float>(-1, 1, -1),
                forceType: "magnetic",
                duration: 2.0
            )
        ]
    }
    
    // Physics step time per body
    func getBodyStepTimes() -> [BodyStepTime] {
        return [
            BodyStepTime(
                entityID: UUID(),
                stepTimeMs: 0.15,
                constraintSolveTime: 0.08,
                collisionTime: 0.04,
                integrationTime: 0.03
            ),
            BodyStepTime(
                entityID: UUID(),
                stepTimeMs: 0.22,
                constraintSolveTime: 0.12,
                collisionTime: 0.06,
                integrationTime: 0.04
            ),
            BodyStepTime(
                entityID: UUID(),
                stepTimeMs: 0.09,
                constraintSolveTime: 0.05,
                collisionTime: 0.02,
                integrationTime: 0.02
            )
        ]
    }
    
    // Collision matrix editor
    func getCollisionMatrix() -> [CollisionMatrixEntry] {
        return [
            CollisionMatrixEntry(
                group1: 0,
                group2: 1,
                shouldCollide: true,
                group1Name: "Player",
                group2Name: "Environment"
            ),
            CollisionMatrixEntry(
                group1: 0,
                group2: 2,
                shouldCollide: true,
                group1Name: "Player",
                group2Name: "Enemies"
            ),
            CollisionMatrixEntry(
                group1: 1,
                group2: 2,
                shouldCollide: false,
                group1Name: "Environment",
                group2Name: "Enemies"
            ),
            CollisionMatrixEntry(
                group1: 2,
                group2: 3,
                shouldCollide: true,
                group1Name: "Enemies",
                group2Name: "Projectiles"
            ),
            CollisionMatrixEntry(
                group1: 3,
                group2: 4,
                shouldCollide: false,
                group1Name: "Projectiles",
                group2Name: "Allies"
            )
        ]
    }
    
    func setCollisionMatrixEntry(group1: Int, group2: Int, shouldCollide: Bool) {
        print("[PhysicsDebug] Set collision: group \(group1) with group \(group2) = \(shouldCollide)")
    }
    
    func setDebugMode(_ mode: DebugMode) {
        debugMode = mode
        
        // Update flags based on mode
        switch mode {
        case .none:
            disableAllVisualizations()
        case .basic:
            showCollisionShapes = true
            showContactPoints = true
        case .detailed:
            showCollisionShapes = true
            showContactPoints = true
            showContactNormals = true
            showVelocityVectors = true
            showCenterOfMass = true
        case .advanced:
            enableAllVisualizations()
        }
        
        print("[PhysicsDebug] Set mode to: \(mode.rawValue)")
    }
    
    private func disableAllVisualizations() {
        showCollisionShapes = false
        showContactPoints = false
        showContactNormals = false
        showVelocityVectors = false
        showAngularVelocity = false
        showCenterOfMass = false
        showInertia = false
        showConstraints = false
        showJointLimits = false
        showSleepingBodies = false
        showForceVectors = false
        showBroadphaseAABB = false
        showPhysicsIslands = false
    }
    
    private func enableAllVisualizations() {
        showCollisionShapes = true
        showContactPoints = true
        showContactNormals = true
        showVelocityVectors = true
        showAngularVelocity = true
        showCenterOfMass = true
        showInertia = true
        showConstraints = true
        showJointLimits = true
        showSleepingBodies = true
        showForceVectors = true
        showBroadphaseAABB = true
        showPhysicsIslands = true
    }
    
    func selectBody(_ id: UUID?) {
        selectedBody = id
    }
}

// MARK: - Physics Debugger Panel
struct PhysicsDebuggerPanel: View {
    @ObservedObject var debugger: PhysicsDebugger
    @State private var showingMetrics = true
    @State private var showingVisualizations = true
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Physics Debugger")
                    .font(DesignSystem.Typography.h3)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Menu {
                    ForEach(PhysicsDebugger.DebugMode.allCases) { mode in
                        Button(action: {
                            debugger.setDebugMode(mode)
                        }) {
                            Text(mode.rawValue)
                        }
                    }
                } label: {
                    HStack {
                        Text(debugger.debugMode.rawValue)
                        Image(systemName: "chevron.down")
                            .font(.system(size: 10))
                    }
                    .font(DesignSystem.Typography.body)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            ScrollView {
                VStack(spacing: 8) {
                    // Metrics section
                    VStack(alignment: .leading, spacing: 8) {
                        Button(action: {
                            showingMetrics.toggle()
                        }) {
                            HStack {
                                Text("Physics Metrics")
                                    .font(DesignSystem.Typography.bodyBold)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                
                                Spacer()
                                
                                Image(systemName: showingMetrics ? "chevron.up" : "chevron.down")
                                    .font(.system(size: 10))
                            }
                        }
                        .buttonStyle(.plain)
                        
                        if showingMetrics {
                            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
                                MetricCard(title: "Active Bodies", value: "\(debugger.activeBodyCount)", unit: "", color: .green)
                                MetricCard(title: "Sleeping Bodies", value: "\(debugger.sleepingBodyCount)", unit: "", color: .blue)
                                MetricCard(title: "Collision Pairs", value: "\(debugger.collisionPairCount)", unit: "", color: .orange)
                                MetricCard(title: "Step Time", value: String(format: "%.2f", debugger.physicsStepTimeMs), unit: "ms", color: .purple)
                            }
                        }
                    }
                    .padding(8)
                    
                    EditorDivider()
                    
                    // Visualizations section
                    VStack(alignment: .leading, spacing: 8) {
                        Button(action: {
                            showingVisualizations.toggle()
                        }) {
                            HStack {
                                Text("Visualizations")
                                    .font(DesignSystem.Typography.bodyBold)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                                
                                Spacer()
                                
                                Image(systemName: showingVisualizations ? "chevron.up" : "chevron.down")
                                    .font(.system(size: 10))
                            }
                        }
                        .buttonStyle(.plain)
                        
                        if showingVisualizations {
                            VStack(alignment: .leading, spacing: 4) {
                                Toggle("Collision Shapes", isOn: $debugger.showCollisionShapes)
                                    .toggleStyle(.checkbox)
                                Toggle("Contact Points", isOn: $debugger.showContactPoints)
                                    .toggleStyle(.checkbox)
                                Toggle("Contact Normals", isOn: $debugger.showContactNormals)
                                    .toggleStyle(.checkbox)
                                Toggle("Velocity Vectors", isOn: $debugger.showVelocityVectors)
                                    .toggleStyle(.checkbox)
                                Toggle("Angular Velocity", isOn: $debugger.showAngularVelocity)
                                    .toggleStyle(.checkbox)
                                Toggle("Center of Mass", isOn: $debugger.showCenterOfMass)
                                    .toggleStyle(.checkbox)
                                Toggle("Inertia Tensor", isOn: $debugger.showInertia)
                                    .toggleStyle(.checkbox)
                                Toggle("Constraints", isOn: $debugger.showConstraints)
                                    .toggleStyle(.checkbox)
                                Toggle("Joint Limits", isOn: $debugger.showJointLimits)
                                    .toggleStyle(.checkbox)
                                Toggle("Sleeping Bodies", isOn: $debugger.showSleepingBodies)
                                    .toggleStyle(.checkbox)
                                Toggle("Force Vectors", isOn: $debugger.showForceVectors)
                                    .toggleStyle(.checkbox)
                                Toggle("Broadphase AABB", isOn: $debugger.showBroadphaseAABB)
                                    .toggleStyle(.checkbox)
                                Toggle("Physics Islands", isOn: $debugger.showPhysicsIslands)
                                    .toggleStyle(.checkbox)
                            }
                            .font(DesignSystem.Typography.body)
                        }
                    }
                    .padding(8)
                    
                    EditorDivider()
                    
                    // Collision filter
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Collision Filter")
                            .font(DesignSystem.Typography.bodyBold)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        
                        ForEach(PhysicsDebugger.CollisionFilter.allCases, id: \.self) { filter in
                            Button(action: {
                                debugger.collisionFilter = filter
                            }) {
                                HStack {
                                    Image(systemName: debugger.collisionFilter == filter ? "checkmark.circle.fill" : "circle")
                                        .foregroundColor(debugger.collisionFilter == filter ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textDisabled)
                                    
                                    Text(filter.rawValue)
                                        .font(DesignSystem.Typography.body)
                                        .foregroundColor(DesignSystem.Colors.textPrimary)
                                    
                                    Spacer()
                                }
                                .padding(4)
                            }
                            .buttonStyle(.plain)
                        }
                    }
                    .padding(8)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}
