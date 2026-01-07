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
    
    // TODO: Implement joint limits and angles display
    // TODO: Implement spring forces visualization
    // TODO: Implement damping visualization
    // TODO: Implement friction coefficient display
    // TODO: Implement restitution (bounciness) display
    // TODO: Implement sleeping bodies highlight
    func getSleepingBodies() -> Set<UUID> {
        return []
    }
    
    // TODO: Implement active body count tracking
    func updatePhysicsMetrics(active: Int, sleeping: Int, pairs: Int, stepTime: Float) {
        activeBodyCount = active
        sleepingBodyCount = sleeping
        collisionPairCount = pairs
        physicsStepTimeMs = stepTime
    }
    
    // TODO: Implement collision pair tracking
    // TODO: Implement broadphase AABB visualization
    // TODO: Implement physics island visualization
    // TODO: Implement ragdoll debug mode
    // TODO: Implement soft body stress visualization
    // TODO: Implement cloth simulation mesh display
    // TODO: Implement fluid particle visualization
    // TODO: Implement buoyancy force vectors
    // TODO: Implement wind force field visualization
    // TODO: Implement gravity visualization
    // TODO: Implement custom force visualization
    // TODO: Implement physics step time per body
    // TODO: Implement collision matrix editor
    
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
