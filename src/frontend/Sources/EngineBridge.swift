import Foundation
import simd
import SwiftUI

// MARK: - Log Level

enum LogLevel: Int32 {
    case debug = 0
    case info = 1
    case warning = 2
    case error = 3
    
    var color: Color {
        switch self {
        case .debug: return .gray
        case .info: return .blue
        case .warning: return .orange
        case .error: return .red
        }
    }
    
    var icon: String {
        switch self {
        case .debug: return "ant"
        case .info: return "info.circle"
        case .warning: return "exclamationmark.triangle"
        case .error: return "xmark.octagon"
        }
    }
    
    var displayName: String {
        switch self {
        case .debug: return "Debug"
        case .info: return "Info"
        case .warning: return "Warning"
        case .error: return "Error"
        }
    }
}

struct LogEntry: Identifiable {
    let id = UUID()
    let timestamp: Date
    let level: LogLevel
    let source: String
    let message: String
    let stackTrace: String?
    var count: Int = 1
}

// MARK: - Engine Bridge

class EngineBridge: ObservableObject {
    static let shared = EngineBridge()
    
    @Published var entities: [UUID: EntityBridgeData] = [:]
    @Published var logs: [LogEntry] = []
    @Published var renderStats: RenderStats = RenderStats()
    
    private var callbacks: EngineCallbacks
    private var updateTimer: Timer?
    
    private init() {
        // Setup callbacks
        callbacks = EngineCallbacks(
            on_entity_created: { entityID in
                EngineBridge.shared.handleEntityCreated(entityID)
            },
            on_entity_deleted: { entityID in
                EngineBridge.shared.handleEntityDeleted(entityID)
            },
            on_entity_modified: { entityID in
                EngineBridge.shared.handleEntityModified(entityID)
            },
            on_log_message: { message, level, source in
                EngineBridge.shared.handleLogMessage(message, level: level, source: source)
            },
            on_scene_loaded: { scenePath in
                EngineBridge.shared.handleSceneLoaded(scenePath)
            }
        )
        
        // Initialize engine
        engine_init(callbacks)
        
        // Start update loop
        startUpdateLoop()
    }
    
    deinit {
        stopUpdateLoop()
        engine_shutdown()
    }
    
    // MARK: - Update Loop
    
    private func startUpdateLoop() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] _ in
            self?.update()
        }
    }
    
    private func stopUpdateLoop() {
        updateTimer?.invalidate()
        updateTimer = nil
    }
    
    private func update() {
        engine_update(1.0/60.0)
        
        // Update render stats
        var stats: RenderStats_C = (fps: 0, frame_time_ms: 0, draw_calls: 0, triangles: 0, vertices: 0, memory_used: 0)
        engine_get_render_stats(&stats)
        
        DispatchQueue.main.async {
            self.renderStats = RenderStats(
                fps: stats.fps,
                frameTimeMs: stats.frame_time_ms,
                drawCalls: Int(stats.draw_calls),
                triangles: Int(stats.triangles),
                vertices: Int(stats.vertices),
                memoryUsed: Int(stats.memory_used)
            )
        }
    }
    
    // MARK: - Entity Management
    
    func createEntity(name: String) -> UUID {
        let entityID = engine_create_entity(name)
        let uuid = UUID(uint64: entityID)
        
        // Fetch entity data
        syncEntity(uuid)
        
        return uuid
    }
    
    func deleteEntity(_ id: UUID) {
        engine_delete_entity(id.uint64Value)
        
        DispatchQueue.main.async {
            self.entities.removeValue(forKey: id)
        }
    }
    
    func entityExists(_ id: UUID) -> Bool {
        return engine_entity_exists(id.uint64Value)
    }
    
    func setEntityName(_ id: UUID, name: String) {
        engine_set_entity_name(id.uint64Value, name)
    }
    
    func getAllEntities() -> [UUID] {
        var entityIDs = [UInt64](repeating: 0, count: 10000)
        let count = engine_get_all_entities(&entityIDs, 10000)
        
        return (0..<Int(count)).map { UUID(uint64: entityIDs[$0]) }
    }
    
    // MARK: - Transform
    
    func setTransform(_ id: UUID, transform: TransformData) {
        var cTransform = Transform_C(
            position: (transform.position.x, transform.position.y, transform.position.z),
            rotation: (transform.rotation.x, transform.rotation.y, transform.rotation.z),
            scale: (transform.scale.x, transform.scale.y, transform.scale.z)
        )
        
        engine_set_transform(id.uint64Value, &cTransform)
    }
    
    /// Convenience method for setting transform with individual SIMD3 parameters
    func setTransform(_ id: UUID, position: SIMD3<Float>, rotation: SIMD3<Float>, scale: SIMD3<Float>) {
        let transform = TransformData(position: position, rotation: rotation, scale: scale)
        setTransform(id, transform: transform)
    }
    
    /// Create entity with a specific UUID (for undo/redo system)
    func createEntityWithID(_ id: UUID, name: String) {
        engine_create_entity_with_id(id.uint64Value, name)
        syncEntity(id)
    }
    
    func getTransform(_ id: UUID) -> TransformData {
        var cTransform: Transform_C = (position: (0,0,0), rotation: (0,0,0), scale: (1,1,1))
        engine_get_transform(id.uint64Value, &cTransform)
        
        return TransformData(
            position: SIMD3(cTransform.position.0, cTransform.position.1, cTransform.position.2),
            rotation: SIMD3(cTransform.rotation.0, cTransform.rotation.1, cTransform.rotation.2),
            scale: SIMD3(cTransform.scale.0, cTransform.scale.1, cTransform.scale.2)
        )
    }
    
    // MARK: - Components
    
    func hasComponent(_ id: UUID, type: ComponentType) -> Bool {
        return engine_has_component(id.uint64Value, type.rawValue)
    }
    
    // MARK: - Enhanced Component Management
    
    func getComponents(for entityID: UUID) -> [ComponentBridgeData]? {
        let id_u64 = entityID.uint64Value
        let count = engine_get_component_count(id_u64)
        
        if count == 0 { return [] }
        
        var types = [Int32](repeating: 0, count: Int(count))
        // We need to pass MutablePointer to the array
        engine_get_component_types(id_u64, &types, count)
        
        var components: [ComponentBridgeData] = []
        
        for typeVal in types {
            guard let type = ComponentType(rawValue: typeVal) else { continue }
            
            var data: [String: Any] = [:]
            
            switch type {
            case .transform:
                var transform: Transform_C = (position: (0,0,0), rotation: (0,0,0), scale: (1,1,1))
                engine_get_transform(id_u64, &transform)
                data = [
                    "position": ["x": transform.position.0, "y": transform.position.1, "z": transform.position.2],
                    "rotation": ["x": transform.rotation.0, "y": transform.rotation.1, "z": transform.rotation.2],
                    "scale": ["x": transform.scale.0, "y": transform.scale.1, "z": transform.scale.2]
                ]
                
            case .meshRenderer:
                var meshData: MeshRendererData_C = (mesh_id: 0, material_id: 0, cast_shadows: false, receive_shadows: false)
                engine_get_mesh_renderer(id_u64, &meshData)
                data = [
                    "meshID": Int(meshData.mesh_id),
                    "materialID": Int(meshData.material_id),
                    "castShadows": meshData.cast_shadows,
                    "receiveShadows": meshData.receive_shadows
                ]
                
            case .physics:
                var physData: PhysicsData_C = (mass: 1.0, drag: 0.0, angular_drag: 0.05, use_gravity: true, is_kinematic: false, velocity: (0,0,0), angular_velocity: (0,0,0))
                engine_get_physics(id_u64, &physData)
                data = [
                    "mass": physData.mass,
                    "drag": physData.drag,
                    "angularDrag": physData.angular_drag,
                    "useGravity": physData.use_gravity,
                    "isKinematic": physData.is_kinematic
                ]
                
            default:
                break
            }
            
            // Map ComponentType enum name to string key expected by UI
            let typeName: String
            switch type {
            case .transform: typeName = "Transform"
            case .meshRenderer: typeName = "Mesh Renderer"
            case .physics: typeName = "Physics"
            default: typeName = "Component"
            }
            
            components.append(ComponentBridgeData(type: typeName, data: data))
        }
        
        return components
    }
    
    func updateComponent(_ entityID: UUID, component: ComponentBridgeData) {
        let id_u64 = entityID.uint64Value
        
        switch component.type {
        case "Transform":
            if let posDict = component.data["position"] as? [String: Float],
               let rotDict = component.data["rotation"] as? [String: Float],
               let scaleDict = component.data["scale"] as? [String: Float] {
                var transform = Transform_C(
                    position: (posDict["x"] ?? 0, posDict["y"] ?? 0, posDict["z"] ?? 0),
                    rotation: (rotDict["x"] ?? 0, rotDict["y"] ?? 0, rotDict["z"] ?? 0),
                    scale: (scaleDict["x"] ?? 1, scaleDict["y"] ?? 1, scaleDict["z"] ?? 1)
                )
                engine_set_transform(id_u64, &transform)
            }
            
        case "Mesh Renderer":
            var meshData = MeshRendererData_C(
                mesh_id: UInt64(component.data["meshID"] as? Int ?? 0),
                material_id: UInt64(component.data["materialID"] as? Int ?? 0),
                cast_shadows: component.data["castShadows"] as? Bool ?? true,
                receive_shadows: component.data["receiveShadows"] as? Bool ?? true
            )
            engine_set_mesh_renderer(id_u64, &meshData)
            
        case "Physics":
            var physData = PhysicsData_C(
                mass: component.data["mass"] as? Float ?? 1.0,
                drag: component.data["drag"] as? Float ?? 0.0,
                angular_drag: component.data["angularDrag"] as? Float ?? 0.05,
                use_gravity: component.data["useGravity"] as? Bool ?? true,
                is_kinematic: component.data["isKinematic"] as? Bool ?? false,
                velocity: (0, 0, 0),
                angular_velocity: (0, 0, 0)
            )
            engine_set_physics(id_u64, &physData)
            
        default:
            log("Unknown component type: \(component.type)", level: .warning, source: "EngineBridge")
        }
        
        syncEntity(entityID)
    }
    
    func addComponent(_ entityID: UUID, component: ComponentBridgeData) {
        let id_u64 = entityID.uint64Value
        
        // Map component type string to ComponentType enum
        var componentType: Int32 = -1
        switch component.type {
        case "Transform": componentType = ComponentType.transform.rawValue
        case "Mesh Renderer": componentType = ComponentType.meshRenderer.rawValue
        case "Physics": componentType = ComponentType.physics.rawValue
        case "Camera": componentType = ComponentType.camera.rawValue
        case "Light": componentType = ComponentType.light.rawValue
        case "Audio Source": componentType = ComponentType.audioSource.rawValue
        case "Script": componentType = ComponentType.script.rawValue
        default:
            log("Unknown component type: \(component.type)", level: .warning, source: "EngineBridge")
            return
        }
        
        engine_add_component(id_u64, componentType)
        updateComponent(entityID, component: component)
    }
    
    func removeComponent(_ entityID: UUID, componentType: String) {
        let id_u64 = entityID.uint64Value
        
        var type: Int32 = -1
        switch componentType {
        case "Transform": type = ComponentType.transform.rawValue
        case "Mesh Renderer": type = ComponentType.meshRenderer.rawValue
        case "Physics": type = ComponentType.physics.rawValue
        case "Camera": type = ComponentType.camera.rawValue
        case "Light": type = ComponentType.light.rawValue
        case "Audio Source": type = ComponentType.audioSource.rawValue
        case "Script": type = ComponentType.script.rawValue
        default: return
        }
        
        engine_remove_component(id_u64, type)
        syncEntity(entityID)
    }
    
    // MARK: - Entity Data Management
    
    struct EntityData {
        var name: String
        var isActive: Bool
        var isStatic: Bool
        var tag: String
        var layer: String
    }
    
    func getEntityData(_ entityID: UUID) -> EntityData? {
        // Fetch real data from C engine
        let id_u64 = entityID.uint64Value
        
        // Name
        let namePtr = engine_get_entity_name(id_u64)
        let name = namePtr.map { String(cString: $0) } ?? "Entity"
        
        // Tag
        let tagPtr = engine_get_entity_tag(id_u64)
        let tag = tagPtr.map { String(cString: $0) } ?? "Untagged"
        
        // Layer
        let layerPtr = engine_get_entity_layer(id_u64)
        let layer = layerPtr.map { String(cString: $0) } ?? "Default"
        
        return EntityData(
            name: name,
            isActive: engine_get_entity_active(id_u64),
            isStatic: engine_get_entity_static(id_u64),
            tag: tag,
            layer: layer
        )
    }
    

    
    func setEntityActive(_ entityID: UUID, isActive: Bool) {
        engine_set_entity_active(entityID.uint64Value, isActive)
    }
    
    func setEntityStatic(_ entityID: UUID, isStatic: Bool) {
        engine_set_entity_static(entityID.uint64Value, isStatic)
    }
    
    func setEntityTag(_ entityID: UUID, tag: String) {
        engine_set_entity_tag(entityID.uint64Value, tag)
    }
    
    func setEntityLayer(_ entityID: UUID, layer: String) {
        engine_set_entity_layer(entityID.uint64Value, layer)
    }
    
    func addMeshRenderer(_ id: UUID, data: MeshRendererData) {
        var cData = MeshRendererData_C(
            mesh_id: 0,
            material_id: 0,
            cast_shadows: data.castShadows,
            receive_shadows: data.receiveShadows
        )
        
        engine_set_mesh_renderer(id.uint64Value, &cData)
    }
    
    func getMeshRenderer(_ id: UUID) -> MeshRendererData? {
        guard hasComponent(id, type: .meshRenderer) else { return nil }
        
        var cData: MeshRendererData_C = (mesh_id: 0, material_id: 0, cast_shadows: false, receive_shadows: false)
        engine_get_mesh_renderer(id.uint64Value, &cData)
        
        return MeshRendererData(
            material: "Material",
            castShadows: cData.cast_shadows,
            receiveShadows: cData.receive_shadows
        )
    }
    
    func addPhysics(_ id: UUID, data: PhysicsData) {
        var cData = PhysicsData_C(
            mass: data.mass,
            drag: data.drag,
            angular_drag: data.angularDrag,
            use_gravity: data.useGravity,
            is_kinematic: data.isKinematic,
            velocity: (data.velocity.x, data.velocity.y, data.velocity.z),
            angular_velocity: (data.angularVelocity.x, data.angularVelocity.y, data.angularVelocity.z)
        )
        
        engine_set_physics(id.uint64Value, &cData)
    }
    
    func getPhysics(_ id: UUID) -> PhysicsData? {
        guard hasComponent(id, type: .physics) else { return nil }
        
        var cData: PhysicsData_C = (mass: 0, drag: 0, angular_drag: 0, use_gravity: false, is_kinematic: false, velocity: (0,0,0), angular_velocity: (0,0,0))
        engine_get_physics(id.uint64Value, &cData)
        
        return PhysicsData(
            mass: cData.mass,
            useGravity: cData.use_gravity,
            isKinematic: cData.is_kinematic,
            drag: cData.drag,
            angularDrag: cData.angular_drag,
            velocity: SIMD3(cData.velocity.0, cData.velocity.1, cData.velocity.2),
            angularVelocity: SIMD3(cData.angular_velocity.0, cData.angular_velocity.1, cData.angular_velocity.2)
        )
    }
    
    /// Alias for addMeshRenderer (for consistency)
    func setMeshRenderer(_ id: UUID, data: MeshRendererData) {
        addMeshRenderer(id, data: data)
    }
    
    /// Alias for addPhysics (for consistency)
    func setPhysics(_ id: UUID, data: PhysicsData) {
        addPhysics(id, data: data)
    }
    
    // MARK: - Scene Management
    
    func loadScene(path: String) {
        engine_load_scene(path)
    }
    
    func saveScene(path: String) {
        engine_save_scene(path)
    }
    
    func newScene() {
        engine_new_scene()
        
        DispatchQueue.main.async {
            self.entities.removeAll()
        }
    }
    
    // MARK: - Logging
    
    func log(_ message: String, level: LogLevel = .info, source: String = "Swift") {
        engine_log(level.rawValue, source, message)
    }
    
    // MARK: - Callbacks
    
    private func handleEntityCreated(_ entityID: UInt64) {
        let uuid = UUID(uint64: entityID)
        syncEntity(uuid)
    }
    
    private func handleEntityDeleted(_ entityID: UInt64) {
        let uuid = UUID(uint64: entityID)
        
        DispatchQueue.main.async {
            self.entities.removeValue(forKey: uuid)
        }
    }
    
    private func handleEntityModified(_ entityID: UInt64) {
        let uuid = UUID(uint64: entityID)
        syncEntity(uuid)
    }
    
    private func handleLogMessage(_ message: UnsafePointer<CChar>?, level: Int32, source: UnsafePointer<CChar>?) {
        guard let message = message,
              let source = source else { return }
        
        let messageStr = String(cString: message)
        let sourceStr = String(cString: source)
        let logLevel = LogLevel(rawValue: level) ?? .info
        
        let entry = LogEntry(
            timestamp: Date(),
            level: logLevel,
            source: sourceStr,
            message: messageStr,
            stackTrace: nil
        )
        
        DispatchQueue.main.async {
            self.logs.append(entry)
            
            // Limit log size
            if self.logs.count > 1000 {
                self.logs.removeFirst()
            }
        }
    }
    
    private func handleSceneLoaded(_ scenePath: UnsafePointer<CChar>?) {
        guard let scenePath = scenePath else { return }
        
        let path = String(cString: scenePath)
        
        // Sync all entities
        let entityIDs = getAllEntities()
        for id in entityIDs {
            syncEntity(id)
        }
        
        log("Scene loaded: \(path)", level: .info, source: "SceneManager")
    }
    
    // MARK: - Synchronization
    
    private func syncEntity(_ id: UUID) {
        guard entityExists(id) else { return }
        
        let transform = getTransform(id)
        let meshRenderer = getMeshRenderer(id)
        let physics = getPhysics(id)
        
        let data = EntityBridgeData(
            id: id,
            name: engine_get_entity_name(id.uint64Value).map(String.init(cString:)) ?? "Unnamed",
            transform: transform,
            meshRenderer: meshRenderer,
            physics: physics
        )
        
        DispatchQueue.main.async {
            self.entities[id] = data
        }
    }
    
    // MARK: - Grid and Snapping Controls
    
    func setGridEnabled(_ enabled: Bool) {
        engine_set_grid_enabled(enabled)
    }
    
    func setGridSpacing(_ spacing: Float) {
        engine_set_grid_spacing(spacing)
    }
    
    func setPositionSnapEnabled(_ enabled: Bool) {
        engine_set_position_snap_enabled(enabled)
    }
    
    func setPositionSnapIncrement(_ increment: Float) {
        engine_set_position_snap_increment(increment)
    }
    
    func setRotationSnapEnabled(_ enabled: Bool) {
        engine_set_rotation_snap_enabled(enabled)
    }
    
    func setRotationSnapIncrement(_ increment: Float) {
        engine_set_rotation_snap_increment(increment)
    }
    
    func setScaleSnapEnabled(_ enabled: Bool) {
        engine_set_scale_snap_enabled(enabled)
    }
    
    func setScaleSnapIncrement(_ increment: Float) {
        engine_set_scale_snap_increment(increment)
    }
    
    // MARK: - Mesh Overlay & Visualization
    
    enum UVVisualizationMode: Int32 {
        case none = 0
        case uv0 = 1
        case uv1 = 2
        case checker = 3
    }
    
    enum RenderMode: Int32 {
        case shaded = 0
        case wireframe = 1
        case shadedWireframe = 2
        case unlit = 3
        case overdraw = 4
        case albedo = 5
        case normals = 6
        case smoothness = 7
        case metallic = 8
        case ambientOcclusion = 9
    }
    
    func setMeshOverlayColor(_ id: UUID, r: Float, g: Float, b: Float, a: Float = 1.0) {
        engine_set_mesh_overlay_color(id.uint64Value, r, g, b, a)
    }
    
    func setMeshWireframeEnabled(_ id: UUID, enabled: Bool) {
        engine_set_mesh_wireframe_enabled(id.uint64Value, enabled)
    }
    
    func setMeshVertexColorsEnabled(_ id: UUID, enabled: Bool) {
        engine_set_mesh_vertex_colors_enabled(id.uint64Value, enabled)
    }
    
    func setMeshUVVisualization(_ id: UUID, mode: UVVisualizationMode) {
        engine_set_mesh_uv_visualization(id.uint64Value, mode.rawValue)
    }
    
    func setMeshNormalsVisualization(_ id: UUID, enabled: Bool) {
        engine_set_mesh_normals_visualization(id.uint64Value, enabled)
    }
    
    func setMeshBoundsVisualization(_ id: UUID, enabled: Bool) {
        engine_set_mesh_bounds_visualization(id.uint64Value, enabled)
    }
    
    func setMaterialOverride(_ id: UUID, materialID: UInt64) {
        engine_set_material_override(id.uint64Value, materialID)
    }
    
    func clearMaterialOverride(_ id: UUID) {
        engine_clear_material_override(id.uint64Value)
    }
    
    // MARK: - Selection Visualization
    
    func setSelectionOutlineColor(r: Float, g: Float, b: Float) {
        engine_set_selection_outline_color(r, g, b)
    }
    
    func setSelectionOutlineWidth(_ width: Float) {
        engine_set_selection_outline_width(width)
    }
    
    func setHoverHighlightEnabled(_ enabled: Bool) {
        engine_set_hover_highlight_enabled(enabled)
    }
    
    // MARK: - Render Settings
    
    func setRenderMode(_ mode: RenderMode) {
        engine_set_render_mode(mode.rawValue)
    }
    
    func setLightingEnabled(_ enabled: Bool) {
        engine_set_lighting_enabled(enabled)
    }
    
    func setShadowsEnabled(_ enabled: Bool) {
        engine_set_shadows_enabled(enabled)
    }
    
    func setAmbientOcclusionEnabled(_ enabled: Bool) {
        engine_set_ambient_occlusion_enabled(enabled)
    }
}

// MARK: - Data Types

struct EntityBridgeData {
    let id: UUID
    var name: String
    var transform: TransformData
    var meshRenderer: MeshRendererData?
    var physics: PhysicsData?
}

enum ComponentType: Int32 {
    case transform = 0
    case meshRenderer = 1
    case physics = 2
    case camera = 3
    case light = 4
    case audioSource = 5
    case script = 6
}

struct RenderStats {
    var fps: Float = 0
    var frameTimeMs: Float = 0
    var drawCalls: Int = 0
    var triangles: Int = 0
    var vertices: Int = 0
    var memoryUsed: Int = 0
}

// MARK: - C Type Definitions

typealias Transform_C = (
    position: (Float, Float, Float),
    rotation: (Float, Float, Float),
    scale: (Float, Float, Float)
)

typealias MeshRendererData_C = (
    mesh_id: UInt64,
    material_id: UInt64,
    cast_shadows: Bool,
    receive_shadows: Bool
)

typealias PhysicsData_C = (
    mass: Float,
    drag: Float,
    angular_drag: Float,
    use_gravity: Bool,
    is_kinematic: Bool,
    velocity: (Float, Float, Float),
    angular_velocity: (Float, Float, Float)
)

// MARK: - Component Data Structs

struct MeshRendererData {
    var material: String
    var castShadows: Bool
    var receiveShadows: Bool
}

struct PhysicsData {
    var mass: Float
    var useGravity: Bool
    var isKinematic: Bool
    var drag: Float
    var angularDrag: Float
    var velocity: SIMD3<Float>
    var angularVelocity: SIMD3<Float>
}

typealias RenderStats_C = (
    fps: Float,
    frame_time_ms: Float,
    draw_calls: UInt32,
    triangles: UInt32,
    vertices: UInt32,
    memory_used: UInt64
)

typealias EngineCallbacks = (
    on_entity_created: @convention(c) (UInt64) -> Void,
    on_entity_deleted: @convention(c) (UInt64) -> Void,
    on_entity_modified: @convention(c) (UInt64) -> Void,
    on_log_message: @convention(c) (UnsafePointer<CChar>?, Int32, UnsafePointer<CChar>?) -> Void,
    on_scene_loaded: @convention(c) (UnsafePointer<CChar>?) -> Void
)

// MARK: - UUID Extension

extension UUID {
    init(uint64: UInt64) {
        // Convert UInt64 to UUID (simplified)
        var bytes = [UInt8](repeating: 0, count: 16)
        withUnsafeBytes(of: uint64) { ptr in
            for i in 0..<min(8, ptr.count) {
                bytes[i] = ptr[i]
            }
        }
        self.init(uuid: (bytes[0], bytes[1], bytes[2], bytes[3],
                         bytes[4], bytes[5], bytes[6], bytes[7],
                         bytes[8], bytes[9], bytes[10], bytes[11],
                         bytes[12], bytes[13], bytes[14], bytes[15]))
    }
    
    var uint64Value: UInt64 {
        var value: UInt64 = 0
        withUnsafePointer(to: self.uuid) { ptr in
            ptr.withMemoryRebound(to: UInt64.self, capacity: 1) { uintPtr in
                value = uintPtr.pointee
            }
        }
        return value
    }
}

// MARK: - C Function Declarations

@_silgen_name("swift_engine_init")
func engine_init(_ callbacks: EngineCallbacks)

@_silgen_name("engine_shutdown")
func engine_shutdown()

@_silgen_name("engine_update")
func engine_update(_ deltaTime: Float)

@_silgen_name("engine_create_entity")
func engine_create_entity(_ name: String) -> UInt64

@_silgen_name("engine_delete_entity")
func engine_delete_entity(_ entityID: UInt64)

@_silgen_name("engine_entity_exists")
func engine_entity_exists(_ entityID: UInt64) -> Bool

@_silgen_name("engine_get_entity_name")
func engine_get_entity_name(_ entityID: UInt64) -> UnsafePointer<CChar>?

@_silgen_name("engine_set_entity_name")
func engine_set_entity_name(_ entityID: UInt64, _ name: String)

@_silgen_name("engine_get_all_entities")
func engine_get_all_entities(_ outEntities: UnsafeMutablePointer<UInt64>, _ maxCount: UInt32) -> UInt32

@_silgen_name("engine_set_transform")
func engine_set_transform(_ entityID: UInt64, _ transform: UnsafePointer<Transform_C>)

@_silgen_name("engine_get_transform")
func engine_get_transform(_ entityID: UInt64, _ outTransform: UnsafeMutablePointer<Transform_C>)

@_silgen_name("engine_has_component")
func engine_has_component(_ entityID: UInt64, _ type: Int32) -> Bool

@_silgen_name("engine_set_mesh_renderer")
func engine_set_mesh_renderer(_ entityID: UInt64, _ data: UnsafePointer<MeshRendererData_C>)

@_silgen_name("engine_get_mesh_renderer")
func engine_get_mesh_renderer(_ entityID: UInt64, _ outData: UnsafeMutablePointer<MeshRendererData_C>)

@_silgen_name("engine_set_physics")
func engine_set_physics(_ entityID: UInt64, _ data: UnsafePointer<PhysicsData_C>)

@_silgen_name("engine_get_physics")
func engine_get_physics(_ entityID: UInt64, _ outData: UnsafeMutablePointer<PhysicsData_C>)

@_silgen_name("engine_load_scene")
func engine_load_scene(_ path: String)

@_silgen_name("engine_save_scene")
func engine_save_scene(_ path: String)

@_silgen_name("engine_new_scene")
func engine_new_scene()

@_silgen_name("engine_get_render_stats")
func engine_get_render_stats(_ outStats: UnsafeMutablePointer<RenderStats_C>)

@_silgen_name("engine_log")
func engine_log(_ level: Int32, _ source: String, _ message: String)

// Grid and Snapping Controls
@_silgen_name("engine_set_grid_enabled")
func engine_set_grid_enabled(_ enabled: Bool)

@_silgen_name("engine_set_grid_spacing")
func engine_set_grid_spacing(_ spacing: Float)

@_silgen_name("engine_set_position_snap_enabled")
func engine_set_position_snap_enabled(_ enabled: Bool)

@_silgen_name("engine_set_position_snap_increment")
func engine_set_position_snap_increment(_ increment: Float)

@_silgen_name("engine_set_rotation_snap_enabled")
func engine_set_rotation_snap_enabled(_ enabled: Bool)

@_silgen_name("engine_set_rotation_snap_increment")
func engine_set_rotation_snap_increment(_ increment: Float)

@_silgen_name("engine_set_scale_snap_enabled")
func engine_set_scale_snap_enabled(_ enabled: Bool)

@_silgen_name("engine_set_scale_snap_increment")
func engine_set_scale_snap_increment(_ increment: Float)

// MARK: - Enhanced Inspector C Functions

@_silgen_name("engine_get_entity_active")
func engine_get_entity_active(_ entityID: UInt64) -> Bool

@_silgen_name("engine_get_entity_static")
func engine_get_entity_static(_ entityID: UInt64) -> Bool

@_silgen_name("engine_get_entity_tag")
func engine_get_entity_tag(_ entityID: UInt64) -> UnsafePointer<CChar>?

@_silgen_name("engine_get_entity_layer")
func engine_get_entity_layer(_ entityID: UInt64) -> UnsafePointer<CChar>?

@_silgen_name("engine_set_entity_active")
func engine_set_entity_active(_ entityID: UInt64, _ active: Bool)

@_silgen_name("engine_set_entity_static")
func engine_set_entity_static(_ entityID: UInt64, _ isStatic: Bool)

@_silgen_name("engine_set_entity_tag")
func engine_set_entity_tag(_ entityID: UInt64, _ tag: String)

@_silgen_name("engine_set_entity_layer")
func engine_set_entity_layer(_ entityID: UInt64, _ layer: String)

@_silgen_name("engine_get_component_count")
func engine_get_component_count(_ entityID: UInt64) -> Int32

@_silgen_name("engine_get_component_types")
func engine_get_component_types(_ entityID: UInt64, _ outTypes: UnsafeMutablePointer<Int32>, _ maxCount: Int32)

// MARK: - Component Management C Functions

@_silgen_name("engine_add_component")
func engine_add_component(_ entityID: UInt64, _ componentType: Int32)

@_silgen_name("engine_remove_component")
func engine_remove_component(_ entityID: UInt64, _ componentType: Int32)

@_silgen_name("engine_create_entity_with_id")
func engine_create_entity_with_id(_ entityID: UInt64, _ name: String)

// MARK: - Mesh Overlay & Visualization C Functions

@_silgen_name("engine_set_mesh_overlay_color")
func engine_set_mesh_overlay_color(_ entityID: UInt64, _ r: Float, _ g: Float, _ b: Float, _ a: Float)

@_silgen_name("engine_set_mesh_wireframe_enabled")
func engine_set_mesh_wireframe_enabled(_ entityID: UInt64, _ enabled: Bool)

@_silgen_name("engine_set_mesh_vertex_colors_enabled")
func engine_set_mesh_vertex_colors_enabled(_ entityID: UInt64, _ enabled: Bool)

@_silgen_name("engine_set_mesh_uv_visualization")
func engine_set_mesh_uv_visualization(_ entityID: UInt64, _ mode: Int32)

@_silgen_name("engine_set_mesh_normals_visualization")
func engine_set_mesh_normals_visualization(_ entityID: UInt64, _ enabled: Bool)

@_silgen_name("engine_set_mesh_bounds_visualization")
func engine_set_mesh_bounds_visualization(_ entityID: UInt64, _ enabled: Bool)

@_silgen_name("engine_set_material_override")
func engine_set_material_override(_ entityID: UInt64, _ materialID: UInt64)

@_silgen_name("engine_clear_material_override")
func engine_clear_material_override(_ entityID: UInt64)

@_silgen_name("engine_set_selection_outline_color")
func engine_set_selection_outline_color(_ r: Float, _ g: Float, _ b: Float)

@_silgen_name("engine_set_selection_outline_width")
func engine_set_selection_outline_width(_ width: Float)

@_silgen_name("engine_set_hover_highlight_enabled")
func engine_set_hover_highlight_enabled(_ enabled: Bool)

// MARK: - Render Mode C Functions

@_silgen_name("engine_set_ambient_occlusion_enabled")
func engine_set_ambient_occlusion_enabled(_ enabled: Bool)

@_silgen_name("engine_set_debug_rendering_enabled")
func engine_set_debug_rendering_enabled(_ enabled: Bool)

@_silgen_name("engine_set_wireframe_overlay_enabled")
func engine_set_wireframe_overlay_enabled(_ enabled: Bool)

@_silgen_name("engine_set_bounding_boxes_enabled")
func engine_set_bounding_boxes_enabled(_ enabled: Bool)

@_silgen_name("engine_set_performance_profiling_enabled")
func engine_set_performance_profiling_enabled(_ enabled: Bool)
func engine_set_post_processing_enabled(_ enabled: Bool)

@_silgen_name("engine_set_vsync_enabled")
func engine_set_vsync_enabled(_ enabled: Bool)
