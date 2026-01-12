import SwiftUI
import simd

// MARK: - AI Debug Data Structures

struct DecisionTreeNode {
    let id: String
    let type: NodeType
    let label: String
    let position: SIMD3<Float>
    let children: [String]
    
    enum NodeType {
        case selector, sequence, condition, action, decorator
    }
}

struct BlackboardVariable {
    let name: String
    let value: String
    let type: VariableType
    
    enum VariableType {
        case string, int, float, bool, entity, vector3
    }
}

struct LineOfSightRay {
    let start: SIMD3<Float>
    let end: SIMD3<Float>
    let hasLineOfSight: Bool
}

struct FOVCone {
    let position: SIMD3<Float>
    let direction: SIMD3<Float>
    let angle: Float
    let distance: Float
}

struct TrackingLine {
    let from: SIMD3<Float>
    let to: SIMD3<Float>
    let targetType: String
    let trackingStrength: Float
}

struct ThreatLevel {
    let level: Float
    let source: SIMD3<Float>
    let type: String
}

struct SquadFormation {
    let type: String
    let members: [SquadMember]
}

struct SquadMember {
    let position: SIMD3<Float>
    let role: String
}

struct CoverPoint {
    let position: SIMD3<Float>
    let quality: Float
    let occupied: Bool
}

struct TacticalPosition {
    let position: SIMD3<Float>
    let type: String
    let priority: String
}

struct CommunicationEvent {
    let type: String
    let from: UUID
    let to: UUID
    let message: String
    let timestamp: Float
}

struct PathfindingNode {
    let position: SIMD3<Float>
    let gCost: Int
    let hCost: Int
    let fCost: Int
    let status: NodeStatus
    
    enum NodeStatus {
        case start, open, closed, goal
    }
}

struct HeatMapPoint {
    let position: SIMD3<Float>
    let cost: Float
}

struct SteeringBehavior {
    let type: String
    let vector: SIMD3<Float>
    let weight: Float
}

struct FlockingData {
    let separationRadius: Float
    let alignmentRadius: Float
    let cohesionRadius: Float
    let neighbors: [SIMD3<Float>]
}

struct Obstacle {
    let position: SIMD3<Float>
    let radius: Float
}

struct ObstacleAvoidanceData {
    let agentPosition: SIMD3<Float>
    let agentVelocity: SIMD3<Float>
    let obstacles: [Obstacle]
    let avoidanceForces: [SIMD3<Float>]
}

struct AIGoal {
    let name: String
    let priority: Float
    let status: GoalStatus
    
    enum GoalStatus {
        case active, pending, inactive
    }
}

struct SensoryInput {
    let type: String
    let intensity: Float
    let source: SIMD3<Float>
    let description: String
}

// MARK: - Audio Debug Data Structures

struct AudioSource3D {
    let id: UUID
    let position: SIMD3<Float>
    let volume: Float
    let radius: Float
    let isPlaying: Bool
}

struct AttenuationSphere {
    let position: SIMD3<Float>
    let radius: Float
    let innerRadius: Float
    let volume: Float
}

struct AudioListener {
    let position: SIMD3<Float>
    let orientation: SIMD3<Float>
    let gain: Float
}

struct Waypoint {
    let position: SIMD3<Float>
    let id: String
    let connections: [String]
    let isOffMeshLink: Bool
    let linkType: String?
}

struct NavMeshTriangle {
    let vertices: [SIMD3<Float>]
    let isWalkable: Bool
    let area: Float
    let normal: SIMD3<Float>
}

struct OffMeshLink {
    let start: SIMD3<Float>
    let end: SIMD3<Float>
    let radius: Float
    let type: String
    let bidirectional: Bool
}

struct AIState {
    let currentState: String
    let previousState: String
    let stateTimer: Float
    let transitionReason: String
    let confidence: Float
}

// MARK: - AI & Pathfinding Debugger
// Visual debugging for AI systems and pathfinding

class AIDebugger: ObservableObject {
    @Published var showNavMesh = false
    @Published var showPaths = false
    @Published var showPerceptionRadius = false
    @Published var showLineOfSight = false
    @Published var showHearingRadius = false
    @Published var showFOVCone = false
    @Published var showBehaviorTree = false
    @Published var showBlackboard = false
    @Published var showFormations = false
    @Published var showCoverPoints = false
    
    @Published var selectedAgent: UUID?
    @Published var agentCount: Int = 0
    
    // MARK: - Functions
    
    // NavMesh display with walkable areas
    func visualizeNavMesh() -> [NavMeshTriangle] {
        return [
            NavMeshTriangle(
                vertices: [
                    SIMD3<Float>(0, 0, 0),
                    SIMD3<Float>(10, 0, 0),
                    SIMD3<Float>(5, 0, 8.66)
                ],
                isWalkable: true,
                area: 43.3,
                normal: SIMD3<Float>(0, 1, 0)
            ),
            NavMeshTriangle(
                vertices: [
                    SIMD3<Float>(10, 0, 0),
                    SIMD3<Float>(15, 0, 5),
                    SIMD3<Float>(5, 0, 8.66)
                ],
                isWalkable: true,
                area: 32.5,
                normal: SIMD3<Float>(0, 1, 0)
            ),
            NavMeshTriangle(
                vertices: [
                    SIMD3<Float>(15, 0, 5),
                    SIMD3<Float>(20, 0, 0),
                    SIMD3<Float>(25, 0, 8.66)
                ],
                isWalkable: false,
                area: 21.6,
                normal: SIMD3<Float>(0, 1, 0)
            )
        ]
    }
    
    // Off-mesh link visualization
    func getOffMeshLinks() -> [OffMeshLink] {
        return [
            OffMeshLink(
                start: SIMD3<Float>(5, 0, 0),
                end: SIMD3<Float>(5, 3, 0),
                radius: 1.0,
                type: "jump",
                bidirectional: true
            ),
            OffMeshLink(
                start: SIMD3<Float>(15, 0, 5),
                end: SIMD3<Float>(18, 0, 8),
                radius: 0.8,
                type: "climb",
                bidirectional: false
            )
        ]
    }
    
    // AI agent path lines
    func getAgentPath(for agentID: UUID) -> [SIMD3<Float>] {
        return [
            SIMD3<Float>(0, 1.8, 0),
            SIMD3<Float>(2, 1.8, 1),
            SIMD3<Float>(4, 1.8, 2),
            SIMD3<Float>(6, 1.8, 3),
            SIMD3<Float>(8, 1.8, 4),
            SIMD3<Float>(10, 1.8, 5)
        ]
    }
    
    // Waypoint visualization
    func getWaypoints() -> [Waypoint] {
        return [
            Waypoint(
                position: SIMD3<Float>(0, 0, 0),
                id: "WP_001",
                connections: ["WP_002", "WP_003"],
                isOffMeshLink: false,
                linkType: nil
            ),
            Waypoint(
                position: SIMD3<Float>(5, 0, 3),
                id: "WP_002",
                connections: ["WP_001", "WP_004"],
                isOffMeshLink: false,
                linkType: nil
            ),
            Waypoint(
                position: SIMD3<Float>(10, 0, 6),
                id: "WP_003",
                connections: ["WP_001", "WP_005"],
                isOffMeshLink: true,
                linkType: "jump"
            ),
            Waypoint(
                position: SIMD3<Float>(15, 0, 9),
                id: "WP_004",
                connections: ["WP_002", "WP_005"],
                isOffMeshLink: false,
                linkType: nil
            ),
            Waypoint(
                position: SIMD3<Float>(20, 0, 12),
                id: "WP_005",
                connections: ["WP_003", "WP_004"],
                isOffMeshLink: false,
                linkType: nil
            )
        ]
    }
    
    // Current AI state display (per agent)
    func getAgentState(for agentID: UUID) -> AIState {
        return AIState(
            currentState: "Patrolling",
            previousState: "Idle",
            stateTimer: 12.5,
            transitionReason: "No threats detected",
            confidence: 0.85
        )
    }
    
    // MARK: - AI Visualization Functions
    
    // AI decision tree visualization
    func visualizeDecisionTree(for agentID: UUID) -> [DecisionTreeNode] {
        // Return mock decision tree data for visualization
        return [
            DecisionTreeNode(id: "root", type: .selector, label: "Main Strategy", position: SIMD3<Float>(0, 2, 0), children: ["combat", "patrol"]),
            DecisionTreeNode(id: "combat", type: .sequence, label: "Combat", position: SIMD3<Float>(-2, 1, 0), children: ["target_visible", "attack"]),
            DecisionTreeNode(id: "patrol", type: .sequence, label: "Patrol", position: SIMD3<Float>(2, 1, 0), children: ["waypoint_reached", "move_to_waypoint"]),
            DecisionTreeNode(id: "target_visible", type: .condition, label: "Target Visible?", position: SIMD3<Float>(-3, 0, 0), children: []),
            DecisionTreeNode(id: "attack", type: .action, label: "Attack", position: SIMD3<Float>(-1, 0, 0), children: []),
            DecisionTreeNode(id: "waypoint_reached", type: .condition, label: "Waypoint Reached?", position: SIMD3<Float>(1, 0, 0), children: []),
            DecisionTreeNode(id: "move_to_waypoint", type: .action, label: "Move to Waypoint", position: SIMD3<Float>(3, 0, 0), children: [])
        ]
    }
    
    // Behavior tree node highlighting
    func getActiveBehaviorNodes(for agentID: UUID) -> [String] {
        // Return IDs of currently active behavior tree nodes
        return ["root", "patrol", "move_to_waypoint"]
    }
    
    // Blackboard variable display
    func getBlackboardVariables(for agentID: UUID) -> [BlackboardVariable] {
        return [
            BlackboardVariable(name: "target", value: "Player_001", type: .entity),
            BlackboardVariable(name: "health", value: "85.0", type: .float),
            BlackboardVariable(name: "ammo", value: "30", type: .int),
            BlackboardVariable(name: "state", value: "patrolling", type: .string),
            BlackboardVariable(name: "last_seen_time", value: "12.5", type: .float),
            BlackboardVariable(name: "cover_point", value: "Cover_A3", type: .vector3)
        ]
    }
    
    // Perception radius indicators
    func getPerceptionRadius(for agentID: UUID) -> Float {
        return 15.0 // meters
    }
    
    // Line of sight visualization
    func getLineOfSightRays(for agentID: UUID) -> [LineOfSightRay] {
        return [
            LineOfSightRay(start: SIMD3<Float>(0, 1.8, 0), end: SIMD3<Float>(5, 1.8, 0), hasLineOfSight: true),
            LineOfSightRay(start: SIMD3<Float>(0, 1.8, 0), end: SIMD3<Float>(-3, 1.8, 2), hasLineOfSight: false),
            LineOfSightRay(start: SIMD3<Float>(0, 1.8, 0), end: SIMD3<Float>(2, 1.8, -4), hasLineOfSight: true)
        ]
    }
    
    // Hearing radius visualization
    func getHearingRadius(for agentID: UUID) -> Float {
        return 25.0 // meters
    }
    
    // FOV cone display
    func getFOVCone(for agentID: UUID) -> FOVCone {
        return FOVCone(
            position: SIMD3<Float>(0, 1.8, 0),
            direction: SIMD3<Float>(1, 0, 0),
            angle: 90.0, // degrees
            distance: 20.0 // meters
        )
    }
    
    // Target tracking lines
    func getTargetTrackingLines(for agentID: UUID) -> [TrackingLine] {
        return [
            TrackingLine(
                from: SIMD3<Float>(0, 1.8, 0),
                to: SIMD3<Float>(10, 1.8, 5),
                targetType: "player",
                trackingStrength: 0.8
            )
        ]
    }
    
    // AI threat level indicators
    func getThreatLevel(for agentID: UUID) -> ThreatLevel {
        return ThreatLevel(
            level: 0.7, // 0.0 to 1.0
            source: SIMD3<Float>(10, 1.8, 5),
            type: "hostile_player"
        )
    }
    
    // Squad formation visualization
    func getSquadFormation(for squadID: UUID) -> SquadFormation {
        return SquadFormation(
            type: "wedge",
            members: [
                SquadMember(position: SIMD3<Float>(0, 0, 0), role: "leader"),
                SquadMember(position: SIMD3<Float>(-3, 0, 2), role: "flanker_left"),
                SquadMember(position: SIMD3<Float>(3, 0, 2), role: "flanker_right"),
                SquadMember(position: SIMD3<Float>(0, 0, 4), role: "rear_guard")
            ]
        )
    }
    
    // Cover point visualization
    func getCoverPoints(in region: SIMD3<Float>) -> [CoverPoint] {
        return [
            CoverPoint(position: SIMD3<Float>(5, 0, 3), quality: 0.9, occupied: false),
            CoverPoint(position: SIMD3<Float>(-4, 0, 2), quality: 0.7, occupied: true),
            CoverPoint(position: SIMD3<Float>(2, 0, -5), quality: 0.8, occupied: false)
        ]
    }
    
    // Tactical position markers
    func getTacticalPositions(for agentID: UUID) -> [TacticalPosition] {
        return [
            TacticalPosition(position: SIMD3<Float>(8, 0, 4), type: "sniper_spot", priority: "high"),
            TacticalPosition(position: SIMD3<Float>(-6, 0, -2), type: "ambush_point", priority: "medium"),
            TacticalPosition(position: SIMD3<Float>(0, 0, 8), type: "flanking_route", priority: "low")
        ]
    }
    
    // AI communication events
    func getCommunicationEvents(for agentID: UUID) -> [CommunicationEvent] {
        return [
            CommunicationEvent(
                type: "target_spotted",
                from: agentID,
                to: UUID(),
                message: "Target at position (10, 1.8, 5)",
                timestamp: 12.5
            ),
            CommunicationEvent(
                type: "request_backup",
                from: agentID,
                to: UUID(),
                message: "Need assistance at cover point A3",
                timestamp: 15.2
            )
        ]
    }
    
    // Pathfinding A* node exploration
    func getPathfindingExploration(for agentID: UUID) -> [PathfindingNode] {
        return [
            PathfindingNode(position: SIMD3<Float>(0, 0, 0), gCost: 0, hCost: 10, fCost: 10, status: .start),
            PathfindingNode(position: SIMD3<Float>(2, 0, 0), gCost: 2, hCost: 8, fCost: 10, status: .open),
            PathfindingNode(position: SIMD3<Float>(4, 0, 0), gCost: 4, hCost: 6, fCost: 10, status: .open),
            PathfindingNode(position: SIMD3<Float>(6, 0, 0), gCost: 6, hCost: 4, fCost: 10, status: .closed),
            PathfindingNode(position: SIMD3<Float>(8, 0, 0), gCost: 8, hCost: 2, fCost: 10, status: .closed),
            PathfindingNode(position: SIMD3<Float>(10, 0, 0), gCost: 10, hCost: 0, fCost: 10, status: .goal)
        ]
    }
    
    // Path cost heat map
    func getPathCostHeatMap(in region: SIMD3<Float>) -> [HeatMapPoint] {
        return [
            HeatMapPoint(position: SIMD3<Float>(0, 0, 0), cost: 0.1),
            HeatMapPoint(position: SIMD3<Float>(2, 0, 0), cost: 0.3),
            HeatMapPoint(position: SIMD3<Float>(4, 0, 0), cost: 0.5),
            HeatMapPoint(position: SIMD3<Float>(6, 0, 0), cost: 0.8),
            HeatMapPoint(position: SIMD3<Float>(8, 0, 0), cost: 0.6),
            HeatMapPoint(position: SIMD3<Float>(10, 0, 0), cost: 0.2)
        ]
    }
    
    // Steering behavior vectors
    func getSteeringBehaviors(for agentID: UUID) -> [SteeringBehavior] {
        return [
            SteeringBehavior(type: "seek", vector: SIMD3<Float>(0.8, 0, 0.6), weight: 0.7),
            SteeringBehavior(type: "avoidance", vector: SIMD3<Float>(-0.3, 0, -0.2), weight: 0.5),
            SteeringBehavior(type: "alignment", vector: SIMD3<Float>(0.1, 0, 0.1), weight: 0.3),
            SteeringBehavior(type: "cohesion", vector: SIMD3<Float>(0.2, 0, 0.1), weight: 0.4)
        ]
    }
    
    // Flocking behavior debug
    func getFlockingData(for agentID: UUID) -> FlockingData {
        return FlockingData(
            separationRadius: 2.0,
            alignmentRadius: 5.0,
            cohesionRadius: 8.0,
            neighbors: [
                SIMD3<Float>(3, 0, 1),
                SIMD3<Float>(-2, 0, 3),
                SIMD3<Float>(1, 0, -4)
            ]
        )
    }
    
    // Obstacle avoidance visualization
    func getObstacleAvoidanceData(for agentID: UUID) -> ObstacleAvoidanceData {
        return ObstacleAvoidanceData(
            agentPosition: SIMD3<Float>(0, 1.8, 0),
            agentVelocity: SIMD3<Float>(2, 0, 1),
            obstacles: [
                Obstacle(position: SIMD3<Float>(5, 0, 0), radius: 1.5),
                Obstacle(position: SIMD3<Float>(-3, 0, 2), radius: 1.0),
                Obstacle(position: SIMD3<Float>(2, 0, -3), radius: 2.0)
            ],
            avoidanceForces: [
                SIMD3<Float>(-0.5, 0, 0),
                SIMD3<Float>(0.3, 0, -0.2),
                SIMD3<Float>(0, 0, 0.4)
            ]
        )
    }
    
    // Agent velocity and acceleration vectors
    func getAgentVelocityAcceleration(for agentID: UUID) -> (velocity: SIMD3<Float>, acceleration: SIMD3<Float>) {
        return (
            velocity: SIMD3<Float>(2.5, 0, 1.8),
            acceleration: SIMD3<Float>(0.3, 0, 0.1)
        )
    }
    
    // AI goal stack display
    func getGoalStack(for agentID: UUID) -> [AIGoal] {
        return [
            AIGoal(name: "Survive", priority: 1.0, status: .active),
            AIGoal(name: "Eliminate Target", priority: 0.8, status: .active),
            AIGoal(name: "Maintain Cover", priority: 0.6, status: .pending),
            AIGoal(name: "Reload Weapon", priority: 0.4, status: .inactive),
            AIGoal(name: "Regroup with Squad", priority: 0.3, status: .inactive)
        ]
    }
    
    // Sensory input visualization
    func getSensoryInputs(for agentID: UUID) -> [SensoryInput] {
        return [
            SensoryInput(type: "visual", intensity: 0.9, source: SIMD3<Float>(10, 1.8, 5), description: "Player sighted"),
            SensoryInput(type: "audio", intensity: 0.6, source: SIMD3<Float>(-5, 0, 3), description: "Footsteps heard"),
            SensoryInput(type: "damage", intensity: 0.3, source: SIMD3<Float>(0, 1.8, 0), description: "Minor impact"),
            SensoryInput(type: "proximity", intensity: 0.4, source: SIMD3<Float>(3, 0, 2), description: "Nearby ally")
        ]
    }
    
    func selectAgent(_ id: UUID?) {
        selectedAgent = id
    }
}

// MARK: - Audio Debugger
// Visual debugging for 3D audio systems

class AudioDebugger: ObservableObject {
    @Published var show3DPositions = false
    @Published var showAttenuationSpheres = false
    @Published var showOcclusionRays = false
    @Published var showReverbZones = false  
    @Published var showSpectrumAnalyzer = false
    @Published var showWaveform = false
    @Published var showBusRouting = false
    
    @Published var selectedSource: UUID?
    @Published var activeVoiceCount: Int = 0
    
    // MARK: - Functions
    
    // MARK: - Audio Visualization Functions
    
    // Audio source 3D position indicators
    func getAudioSources3D() -> [AudioSource3D] {
        return [
            AudioSource3D(
                id: UUID(),
                position: SIMD3<Float>(5, 2, 3),
                volume: 0.8,
                radius: 10.0,
                isPlaying: true
            ),
            AudioSource3D(
                id: UUID(),
                position: SIMD3<Float>(-3, 1.5, -2),
                volume: 0.6,
                radius: 8.0,
                isPlaying: true
            ),
            AudioSource3D(
                id: UUID(),
                position: SIMD3<Float>(0, 0, 5),
                volume: 0.4,
                radius: 6.0,
                isPlaying: false
            )
        ]
    }
    
    // Audio attenuation sphere/cone visualization
    func getAttenuationSpheres() -> [AttenuationSphere] {
        return [
            AttenuationSphere(
                position: SIMD3<Float>(5, 2, 3),
                radius: 10.0,
                innerRadius: 2.0,
                volume: 0.8
            ),
            AttenuationSphere(
                position: SIMD3<Float>(-3, 1.5, -2),
                radius: 8.0,
                innerRadius: 1.5,
                volume: 0.6
            )
        ]
    }
    
    // Audio listener position
    func getAudioListener() -> AudioListener {
        return AudioListener(
            position: SIMD3<Float>(0, 1.8, 0),
            orientation: SIMD3<Float>(0, 0, 1),
            gain: 1.0
        )
    }
    // TODO: Implement sound occlusion rays
    // TODO: Implement reverb zone boundaries
    // TODO: Implement audio ducking visualization
    // TODO: Implement audio level meters (per source)
    // TODO: Implement spectrum analyzer
    // TODO: Implement waveform display
    // TODO: Implement audio bus routing diagram
    // TODO: Implement DSP effect chain visualization
    // TODO: Implement audio memory usage tracking
    // TODO: Implement active voice count display
    // TODO: Implement audio streaming buffer status
    // TODO: Implement HRTF visualization
    // TODO: Implement Doppler effect indicator
    // TODO: Implement audio priority visualization
    // TODO: Implement audio pooling status
    // TODO: Implement audio spatializer debug
    // TODO: Implement ambisonics visualization
    
    func selectSource(_ id: UUID?) {
        selectedSource = id
    }
}

// MARK: - Network Debugger
// Network traffic and replication debugging

class NetworkDebugger: ObservableObject {
    @Published var showPacketVisualization = false
    @Published var showBandwidthGraph = true
    @Published var showLatencyGraph = true
    @Published var showReplicationStatus = false
    @Published var enableNetworkSimulation = false
    
    // Network metrics
    @Published var currentBandwidthUp: Float = 0 // KB/s
    @Published var currentBandwidthDown: Float = 0 // KB/s
    @Published var currentLatency: Float = 0 // ms  
    @Published var packetLoss: Float = 0 // percentage
    @Published var jitter: Float = 0 // ms
    
    // Simulation settings
    @Published var simulatedLatency: Float = 0
    @Published var simulatedPacketLoss: Float = 0
    @Published var simulatedJitter: Float = 0
    
    // MARK: - Functions
    
    // TODO: Implement network packet visualization
    // TODO: Implement bandwidth usage graph (up/down)
    // TODO: Implement packet loss percentage tracking
    // TODO: Implement ping/latency graph
    // TODO: Implement server tick rate indicator
    // TODO: Implement client update rate
    // TODO: Implement entity replication status
    // TODO: Implement RPC call tracking
    // TODO: Implement network authority indicators
    // TODO: Implement prediction error visualization
    // TODO: Implement lag compensation visualization
    // TODO: Implement network snapshot history
    // TODO: Implement bandwidth breakdown per entity
    // TODO: Implement network stats per player
    // TODO: Implement connection quality indicators
    // TODO: Implement jitter visualization
    // TODO: Implement packet prioritization view
    // TODO: Implement network relevancy debug  
    // TODO: Implement ownership chain visualization
    // TODO: Implement network simulation controls (artificial lag, packet loss)
    
    func setNetworkSimulation(latency: Float, packetLoss: Float, jitter: Float) {
        simulatedLatency = latency
        simulatedPacketLoss = packetLoss
        simulatedJitter = jitter
        print("[NetworkDebug] Set simulation: \(latency)ms latency, \(packetLoss)% loss, \(jitter)ms jitter")
    }
}

// MARK: - Combined Debuggers Panel
struct AllDebuggersPanel: View {
    @StateObject var aiDebugger = AIDebugger()
    @StateObject var audioDebugger = AudioDebugger()
    @StateObject var networkDebugger = NetworkDebugger()
    
    @State private var selectedDebugger: DebuggerType = .ai
    
    enum DebuggerType: String, CaseIterable {
        case ai = "AI & Pathfinding"
        case audio = "Audio"
        case network = "Network"
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Tabs
            HStack(spacing: 0) {
                ForEach(DebuggerType.allCases, id: \.self) { type in
                    Button(action: {
                        selectedDebugger = type
                    }) {
                        Text(type.rawValue)
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(selectedDebugger == type ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .padding(.vertical, 8)
                            .padding(.horizontal, 12)
                            .background(selectedDebugger == type ? DesignSystem.Colors.selection : Color.clear)
                    }
                    .buttonStyle(.plain)
                }
            }
            
            EditorDivider()
            
            // Content
            ScrollView {
                switch selectedDebugger {
                case .ai:
                    AIDebuggerView(debugger: aiDebugger)
                case .audio:
                    AudioDebuggerView(debugger: audioDebugger)
                case .network:
                    NetworkDebuggerView(debugger: networkDebugger)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - AI Debugger View
private struct AIDebuggerView: View {
    @ObservedObject var debugger: AIDebugger
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("AI Visualization")
                .font(DesignSystem.Typography.bodyBold)
            
            Toggle("NavMesh", isOn: $debugger.showNavMesh)
                .toggleStyle(.checkbox)
            Toggle("Agent Paths", isOn: $debugger.showPaths)
                .toggleStyle(.checkbox)
            Toggle("Perception Radius", isOn: $debugger.showPerceptionRadius)
                .toggleStyle(.checkbox)
            Toggle("Line of Sight", isOn: $debugger.showLineOfSight)
                .toggleStyle(.checkbox)
            Toggle("Hearing Radius", isOn: $debugger.showHearingRadius)
                .toggleStyle(.checkbox)
            Toggle("FOV Cone", isOn: $debugger.showFOVCone)
                .toggleStyle(.checkbox)
            Toggle("Behavior Tree", isOn: $debugger.showBehaviorTree)
                .toggleStyle(.checkbox)
            Toggle("Blackboard", isOn: $debugger.showBlackboard)
                .toggleStyle(.checkbox)
            Toggle("Formations", isOn: $debugger.showFormations)
                .toggleStyle(.checkbox)
            Toggle("Cover Points", isOn: $debugger.showCoverPoints)
                .toggleStyle(.checkbox)
            
            EditorDivider()
            
            Text("Agent Count: \(debugger.agentCount)")
                .font(DesignSystem.Typography.body)
        }
        .padding(8)
    }
}

// MARK: - Audio Debugger View
private struct AudioDebuggerView: View {
    @ObservedObject var debugger: AudioDebugger
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Audio Visualization")
                .font(DesignSystem.Typography.bodyBold)
            
            Toggle("3D Positions", isOn: $debugger.show3DPositions)
                .toggleStyle(.checkbox)
            Toggle("Attenuation Spheres", isOn: $debugger.showAttenuationSpheres)
                .toggleStyle(.checkbox)
            Toggle("Occlusion Rays", isOn: $debugger.showOcclusionRays)
                .toggleStyle(.checkbox)
            Toggle("Reverb Zones", isOn: $debugger.showReverbZones)
                .toggleStyle(.checkbox)
            Toggle("Spectrum Analyzer", isOn: $debugger.showSpectrumAnalyzer)
                .toggleStyle(.checkbox)
            Toggle("Waveform", isOn: $debugger.showWaveform)
                .toggleStyle(.checkbox)
            Toggle("Bus Routing", isOn: $debugger.showBusRouting)
                .toggleStyle(.checkbox)
            
            EditorDivider()
            
            Text("Active Voices: \(debugger.activeVoiceCount)")
                .font(DesignSystem.Typography.body)
        }
        .padding(8)
    }
}

// MARK: - Network Debugger View
private struct NetworkDebuggerView: View {
    @ObservedObject var debugger: NetworkDebugger
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Network Metrics")
                .font(DesignSystem.Typography.bodyBold)
            
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Bandwidth")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Text(" \(String(format: "%.1f", debugger.currentBandwidthDown)) KB/s")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.green)
                    Text(" \(String(format: "%.1f", debugger.currentBandwidthUp)) KB/s")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(.blue)
                }
                
                Spacer()
                
                VStack(alignment: .trailing, spacing: 4) {
                    Text("Latency")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Text("\(Int(debugger.currentLatency)) ms")
                        .font(DesignSystem.Typography.body)
                    Text("\(String(format: "%.1f", debugger.packetLoss))% loss")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(.red)
                }
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(4)
            
            EditorDivider()
            
            Text("Visualization")
                .font(DesignSystem.Typography.bodyBold)
            
            Toggle("Packet Visualization", isOn: $debugger.showPacketVisualization)
                .toggleStyle(.checkbox)
            Toggle("Bandwidth Graph", isOn: $debugger.showBandwidthGraph)
                .toggleStyle(.checkbox)
            Toggle("Latency Graph", isOn: $debugger.showLatencyGraph)
                .toggleStyle(.checkbox)
            Toggle("Replication Status", isOn: $debugger.showReplicationStatus)
                .toggleStyle(.checkbox)
            
            EditorDivider()
            
            VStack(alignment: .leading, spacing: 8) {
                Toggle("Network Simulation", isOn: $debugger.enableNetworkSimulation)
                    .toggleStyle(.checkbox)
                
                if debugger.enableNetworkSimulation {
                    VStack(spacing: 8) {
                        HStack {
                            Text("Latency")
                                .frame(width: 80, alignment: .leading)
                            Slider(value: $debugger.simulatedLatency, in: 0...500)
                            Text("\(Int(debugger.simulatedLatency)) ms")
                                .frame(width: 60, alignment: .trailing)
                        }
                        .font(DesignSystem.Typography.small)
                        
                        HStack {
                            Text("Packet Loss")
                                .frame(width: 80, alignment: .leading)
                            Slider(value: $debugger.simulatedPacketLoss, in: 0...50)
                            Text("\(Int(debugger.simulatedPacketLoss))%")
                                .frame(width: 60, alignment: .trailing)
                        }
                        .font(DesignSystem.Typography.small)
                        
                        HStack {
                            Text("Jitter")
                                .frame(width: 80, alignment: .leading)
                            Slider(value: $debugger.simulatedJitter, in: 0...100)
                            Text("\(Int(debugger.simulatedJitter)) ms")
                                .frame(width: 60, alignment: .trailing)
                        }
                        .font(DesignSystem.Typography.small)
                        
                        EditorButton("Apply Simulation", icon: "network", style: .primary) {
                            debugger.setNetworkSimulation(
                                latency: debugger.simulatedLatency,
                                packetLoss: debugger.simulatedPacketLoss,
                                jitter: debugger.simulatedJitter
                            )
                        }
                    }
                }
            }
        }
        .padding(8)
    }
}
