import SwiftUI
import simd

// MARK: - Component System
// This file defines the component architecture for the Inspector

// MARK: - Component Protocol
protocol Component: Identifiable {
    var id: UUID { get }
    var componentType: EditorComponentType { get }
    var isEnabled: Bool { get set }
    
    func toEngineBridge() -> ComponentBridgeData
    static func fromEngineBridge(_ data: ComponentBridgeData) -> Self?
}

// MARK: - Component Types
enum EditorComponentType: String, CaseIterable {
    case transform = "Transform"
    case meshRenderer = "Mesh Renderer"
    case light = "Light"
    case camera = "Camera"
    case audioSource = "Audio Source"
    case rigidbody = "Rigidbody"
    case collider = "Collider"
    case animator = "Animator"
    case script = "Script"
    case particleSystem = "Particle System"
    case boat = "Boat Component"

    
    var icon: String {
        switch self {
        case .transform: return "arrow.up.and.down.and.arrow.left.and.right"
        case .meshRenderer: return "cube"
        case .light: return "lightbulb"
        case .camera: return "camera"
        case .audioSource: return "speaker.wave.2"
        case .rigidbody: return "figure.walk"
        case .collider: return "shield"
        case .animator: return "figure.run"
        case .script: return "doc.text"
        case .particleSystem: return "sparkles"
        case .boat: return "sailboat.fill"

        }
    }
    
    var color: Color {
        switch self {
        case .transform: return .blue
        case .meshRenderer: return .purple
        case .light: return .yellow
        case .camera: return .cyan
        case .audioSource: return .green
        case .rigidbody: return .orange
        case .collider: return .red
        case .animator: return .pink
        case .script: return .gray
        case .particleSystem: return .mint
        case .boat: return .blue

        }
    }
    
    var canBeRemoved: Bool {
        return self != .transform // Transform is required
    }
}

// MARK: - Transform Component
struct TransformComponent: Component {
    let id: UUID
    let componentType: EditorComponentType = .transform
    var isEnabled: Bool = true
    
    var position: SIMD3<Float>
    var rotation: SIMD3<Float>
    var scale: SIMD3<Float>
    
    init(id: UUID = UUID(), position: SIMD3<Float> = .zero, rotation: SIMD3<Float> = .zero, scale: SIMD3<Float> = SIMD3(1, 1, 1)) {
        self.id = id
        self.position = position
        self.rotation = rotation
        self.scale = scale
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        return ComponentBridgeData(
            type: componentType.rawValue,
            data: [
                "position": ["x": position.x, "y": position.y, "z": position.z],
                "rotation": ["x": rotation.x, "y": rotation.y, "z": rotation.z],
                "scale": ["x": scale.x, "y": scale.y, "z": scale.z]
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> TransformComponent? {
        guard let posData = data.data["position"] as? [String: Float],
              let rotData = data.data["rotation"] as? [String: Float],
              let scaleData = data.data["scale"] as? [String: Float] else {
            return nil
        }
        
        return TransformComponent(
            position: SIMD3(posData["x"] ?? 0, posData["y"] ?? 0, posData["z"] ?? 0),
            rotation: SIMD3(rotData["x"] ?? 0, rotData["y"] ?? 0, rotData["z"] ?? 0),
            scale: SIMD3(scaleData["x"] ?? 1, scaleData["y"] ?? 1, scaleData["z"] ?? 1)
        )
    }
}

// MARK: - Mesh Renderer Component
struct MeshRendererComponent: Component {
    let id: UUID
    let componentType: EditorComponentType = .meshRenderer
    var isEnabled: Bool = true
    
    var meshPath: String
    var materialPath: String
    var castShadows: Bool
    var receiveShadows: Bool
    
    init(id: UUID = UUID(), meshPath: String = "", materialPath: String = "", castShadows: Bool = true, receiveShadows: Bool = true) {
        self.id = id
        self.meshPath = meshPath
        self.materialPath = materialPath
        self.castShadows = castShadows
        self.receiveShadows = receiveShadows
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        return ComponentBridgeData(
            type: componentType.rawValue,
            data: [
                "meshPath": meshPath,
                "materialPath": materialPath,
                "castShadows": castShadows,
                "receiveShadows": receiveShadows
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> MeshRendererComponent? {
        return MeshRendererComponent(
            meshPath: data.data["meshPath"] as? String ?? "",
            materialPath: data.data["materialPath"] as? String ?? "",
            castShadows: data.data["castShadows"] as? Bool ?? true,
            receiveShadows: data.data["receiveShadows"] as? Bool ?? true
        )
    }
}

// MARK: - Light Component
struct LightComponent: Component {
    let id: UUID
    let componentType: EditorComponentType = .light
    var isEnabled: Bool = true
    
    var lightType: LightType
    var color: Color
    var intensity: Float
    var range: Float
    var spotAngle: Float
    var castShadows: Bool
    
    enum LightType: String, CaseIterable, Codable, CustomStringConvertible {
        case directional = "Directional"
        case point = "Point"
        case spot = "Spot"
        case area = "Area"
        
        var description: String { rawValue }
    }
    
    init(id: UUID = UUID(), lightType: LightType = .point, color: Color = .white, intensity: Float = 1.0, range: Float = 10.0, spotAngle: Float = 45.0, castShadows: Bool = true) {
        self.id = id
        self.lightType = lightType
        self.color = color
        self.intensity = intensity
        self.range = range
        self.spotAngle = spotAngle
        self.castShadows = castShadows
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        return ComponentBridgeData(
            type: componentType.rawValue,
            data: [
                "lightType": lightType.rawValue,
                "intensity": intensity,
                "range": range,
                "spotAngle": spotAngle,
                "castShadows": castShadows
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> LightComponent? {
        guard let typeStr = data.data["lightType"] as? String,
              let lightType = LightType(rawValue: typeStr) else {
            return nil
        }
        
        return LightComponent(
            lightType: lightType,
            intensity: data.data["intensity"] as? Float ?? 1.0,
            range: data.data["range"] as? Float ?? 10.0,
            spotAngle: data.data["spotAngle"] as? Float ?? 45.0,
            castShadows: data.data["castShadows"] as? Bool ?? true
        )
    }
}

// MARK: - Rigidbody Component
struct RigidbodyComponent: Component {
    let id: UUID
    let componentType: EditorComponentType = .rigidbody
    var isEnabled: Bool = true
    
    var mass: Float
    var drag: Float
    var angularDrag: Float
    var useGravity: Bool
    var isKinematic: Bool
    
    init(id: UUID = UUID(), mass: Float = 1.0, drag: Float = 0.0, angularDrag: Float = 0.05, useGravity: Bool = true, isKinematic: Bool = false) {
        self.id = id
        self.mass = mass
        self.drag = drag
        self.angularDrag = angularDrag
        self.useGravity = useGravity
        self.isKinematic = isKinematic
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        return ComponentBridgeData(
            type: componentType.rawValue,
            data: [
                "mass": mass,
                "drag": drag,
                "angularDrag": angularDrag,
                "useGravity": useGravity,
                "isKinematic": isKinematic
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> RigidbodyComponent? {
        return RigidbodyComponent(
            mass: data.data["mass"] as? Float ?? 1.0,
            drag: data.data["drag"] as? Float ?? 0.0,
            angularDrag: data.data["angularDrag"] as? Float ?? 0.05,
            useGravity: data.data["useGravity"] as? Bool ?? true,
            isKinematic: data.data["isKinematic"] as? Bool ?? false
        )
    }
}

// MARK: - Collider Component
struct ColliderComponent: Component {
    let id: UUID
    let componentType: EditorComponentType = .collider
    var isEnabled: Bool = true
    
    var colliderType: ColliderType
    var isTrigger: Bool
    var center: SIMD3<Float>
    var size: SIMD3<Float>
    var radius: Float
    
    enum ColliderType: String, CaseIterable, Codable, CustomStringConvertible {
        case box = "Box"
        case sphere = "Sphere"
        case capsule = "Capsule"
        case mesh = "Mesh"
        
        var description: String { rawValue }
    }
    
    init(id: UUID = UUID(), colliderType: ColliderType = .box, isTrigger: Bool = false, center: SIMD3<Float> = .zero, size: SIMD3<Float> = SIMD3(1, 1, 1), radius: Float = 0.5) {
        self.id = id
        self.colliderType = colliderType
        self.isTrigger = isTrigger
        self.center = center
        self.size = size
        self.radius = radius
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        return ComponentBridgeData(
            type: componentType.rawValue,
            data: [
                "colliderType": colliderType.rawValue,
                "isTrigger": isTrigger,
                "center": ["x": center.x, "y": center.y, "z": center.z],
                "size": ["x": size.x, "y": size.y, "z": size.z],
                "radius": radius
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> ColliderComponent? {
        guard let typeStr = data.data["colliderType"] as? String,
              let colliderType = ColliderType(rawValue: typeStr) else {
            return nil
        }
        
        let centerData = data.data["center"] as? [String: Float] ?? [:]
        let sizeData = data.data["size"] as? [String: Float] ?? [:]
        
        return ColliderComponent(
            colliderType: colliderType,
            isTrigger: data.data["isTrigger"] as? Bool ?? false,
            center: SIMD3(centerData["x"] ?? 0, centerData["y"] ?? 0, centerData["z"] ?? 0),
            size: SIMD3(sizeData["x"] ?? 1, sizeData["y"] ?? 1, sizeData["z"] ?? 1),
            radius: data.data["radius"] as? Float ?? 0.5
        )
    }
}

// MARK: - Component Bridge Data
struct ComponentBridgeData: Codable {
    let type: String
    let data: [String: Any]
    
    enum CodingKeys: String, CodingKey {
        case type, data
    }
    
    init(type: String, data: [String: Any]) {
        self.type = type
        self.data = data
    }
    
    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        type = try container.decode(String.self, forKey: .type)
        data = [:] // Simplified for now
    }
    
    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        try container.encode(type, forKey: .type)
    }
}
