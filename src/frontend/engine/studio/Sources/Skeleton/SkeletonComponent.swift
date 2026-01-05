import SwiftUI
import simd

// MARK: - Skeleton Component
/// Component for attaching a skeleton to an entity

struct SkeletonComponent: Component {
    let id: UUID
    let componentType: EditorComponentType = .animator  // Using animator for now
    var isEnabled: Bool = true
    
    var skeletonID: UUID?
    var currentPoseName: String = "Bind Pose"
    var showBones: Bool = true
    var boneDisplaySize: Float = 1.0
    
    init(id: UUID = UUID(), skeletonID: UUID? = nil) {
        self.id = id
        self.skeletonID = skeletonID
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        return ComponentBridgeData(
            type: "Skeleton",
            data: [
                "skeletonID": skeletonID?.uuidString ?? "",
                "currentPose": currentPoseName,
                "showBones": showBones,
                "boneDisplaySize": boneDisplaySize
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> SkeletonComponent? {
        let skeletonIDString = data.data["skeletonID"] as? String ?? ""
        let skeletonID = UUID(uuidString: skeletonIDString)
        
        var component = SkeletonComponent(skeletonID: skeletonID)
        component.currentPoseName = data.data["currentPose"] as? String ?? "Bind Pose"
        component.showBones = data.data["showBones"] as? Bool ?? true
        component.boneDisplaySize = data.data["boneDisplaySize"] as? Float ?? 1.0
        
        return component
    }
}

// MARK: - Skinned Mesh Renderer Component

struct SkinnedMeshRendererComponent: Component {
    let id: UUID
    let componentType: EditorComponentType = .meshRenderer
    var isEnabled: Bool = true
    
    var meshWrapperID: UUID?
    var skeletonID: UUID?
    var materialPath: String = ""
    var castShadows: Bool = true
    var receiveShadows: Bool = true
    var updateMode: UpdateMode = .auto
    
    enum UpdateMode: String, CaseIterable {
        case auto = "Auto"
        case manual = "Manual"
        case whenVisible = "When Visible"
    }
    
    init(id: UUID = UUID()) {
        self.id = id
    }
    
    func toEngineBridge() -> ComponentBridgeData {
        return ComponentBridgeData(
            type: "SkinnedMeshRenderer",
            data: [
                "meshWrapperID": meshWrapperID?.uuidString ?? "",
                "skeletonID": skeletonID?.uuidString ?? "",
                "materialPath": materialPath,
                "castShadows": castShadows,
                "receiveShadows": receiveShadows,
                "updateMode": updateMode.rawValue
            ]
        )
    }
    
    static func fromEngineBridge(_ data: ComponentBridgeData) -> SkinnedMeshRendererComponent? {
        var component = SkinnedMeshRendererComponent()
        
        if let idStr = data.data["meshWrapperID"] as? String {
            component.meshWrapperID = UUID(uuidString: idStr)
        }
        if let idStr = data.data["skeletonID"] as? String {
            component.skeletonID = UUID(uuidString: idStr)
        }
        component.materialPath = data.data["materialPath"] as? String ?? ""
        component.castShadows = data.data["castShadows"] as? Bool ?? true
        component.receiveShadows = data.data["receiveShadows"] as? Bool ?? true
        
        if let modeStr = data.data["updateMode"] as? String {
            component.updateMode = UpdateMode(rawValue: modeStr) ?? .auto
        }
        
        return component
    }
}
