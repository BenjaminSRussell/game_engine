import SwiftUI
import simd

// MARK: - Skeleton Preset

enum SkeletonPresetCategory: String, CaseIterable {
    case character = "Characters"
    case vehicle = "Vehicles"
    case custom = "Custom"
    
    var icon: String {
        switch self {
        case .character: return "person.fill"
        case .vehicle: return "sailboat.fill"
        case .custom: return "cube"
        }
    }
}

enum SkeletonPreset: String, CaseIterable, Identifiable {
    case humanoid = "Humanoid Character"
    case sailboat = "Sailboat"
    case motorboat = "Motorboat"
    case custom = "Empty Skeleton"
    
    var id: String { rawValue }
    
    var category: SkeletonPresetCategory {
        switch self {
        case .humanoid: return .character
        case .sailboat, .motorboat: return .vehicle
        case .custom: return .custom
        }
    }
    
    var description: String {
        switch self {
        case .humanoid:
            return "19-bone bipedal character rig with arms, legs, spine, and head"
        case .sailboat:
            return "Sailing vessel with hull, mast, boom, sails, and rudder for wind-powered navigation"
        case .motorboat:
            return "Motorized boat with hull, propeller, rudder, and outboard motor for powered navigation"
        case .custom:
            return "Start from scratch with no bones"
        }
    }
    
    var icon: String {
        switch self {
        case .humanoid: return "figure.stand"
        case .sailboat: return "sailboat.fill"
        case .motorboat: return "ferry.fill"
        case .custom: return "cube.transparent"
        }
    }
    
    var boneCount: Int {
        switch self {
        case .humanoid: return 19
        case .sailboat: return 6
        case .motorboat: return 4
        case .custom: return 0
        }
    }
}

// MARK: - Skeleton System
/// Complete skeletal hierarchy for character rigging and animation

class Skeleton: ObservableObject, Identifiable {
    let id: UUID
    @Published var name: String
    @Published var bones: [UUID: BoneNode] = [:]
    @Published var rootBoneID: UUID?
    @Published var selectedBoneIDs: Set<UUID> = []
    
    // Bind pose (reference pose for skinning)
    private var bindPoseTransforms: [UUID: simd_float4x4] = [:]
    private var bindPoseInverses: [UUID: simd_float4x4] = [:]
    
    // Cached world transforms
    @Published private(set) var worldTransforms: [UUID: simd_float4x4] = [:]
    
    // MARK: - Initialization
    
    init(id: UUID = UUID(), name: String = "New Skeleton") {
        self.id = id
        self.name = name
    }
    
    // MARK: - Bone Management
    
    /// Add a new bone to the skeleton
    @discardableResult
    func addBone(
        name: String,
        parentID: UUID? = nil,
        position: SIMD3<Float> = .zero,
        length: Float = 1.0
    ) -> UUID {
        let bone = BoneNode(
            name: name,
            parentID: parentID,
            localPosition: position,
            length: length
        )
        
        bones[bone.id] = bone
        
        if let parentID = parentID, var parent = bones[parentID] {
            parent.childIDs.append(bone.id)
            bones[parentID] = parent
        } else if rootBoneID == nil {
            rootBoneID = bone.id
        }
        
        updateWorldTransforms()
        return bone.id
    }
    
    /// Remove a bone and its children
    func removeBone(_ boneID: UUID) {
        guard let bone = bones[boneID] else { return }
        
        // Remove children recursively
        for childID in bone.childIDs {
            removeBone(childID)
        }
        
        // Remove from parent's children list
        if let parentID = bone.parentID, var parent = bones[parentID] {
            parent.childIDs.removeAll { $0 == boneID }
            bones[parentID] = parent
        }
        
        // Update root if needed
        if rootBoneID == boneID {
            rootBoneID = nil
        }
        
        bones.removeValue(forKey: boneID)
        selectedBoneIDs.remove(boneID)
        updateWorldTransforms()
    }
    
    /// Reparent a bone to a new parent
    func reparentBone(_ boneID: UUID, newParentID: UUID?) {
        guard var bone = bones[boneID] else { return }
        
        // Remove from old parent
        if let oldParentID = bone.parentID, var oldParent = bones[oldParentID] {
            oldParent.childIDs.removeAll { $0 == boneID }
            bones[oldParentID] = oldParent
        }
        
        // Add to new parent
        bone.parentID = newParentID
        if let newParentID = newParentID, var newParent = bones[newParentID] {
            newParent.childIDs.append(boneID)
            bones[newParentID] = newParent
        }
        
        bones[boneID] = bone
        updateWorldTransforms()
    }
    
    // MARK: - Transform Calculations
    
    /// Calculate world transform for a bone
    func worldTransform(for boneID: UUID) -> simd_float4x4 {
        guard let bone = bones[boneID] else {
            return matrix_identity_float4x4
        }
        
        let localMatrix = bone.localTransformMatrix
        
        if let parentID = bone.parentID {
            let parentWorld = worldTransform(for: parentID)
            return parentWorld * localMatrix
        }
        
        return localMatrix
    }
    
    /// Update all world transforms (call after bone changes)
    func updateWorldTransforms() {
        worldTransforms.removeAll()
        
        func updateRecursive(_ boneID: UUID, parentWorld: simd_float4x4) {
            guard let bone = bones[boneID] else { return }
            
            let worldMatrix = parentWorld * bone.localTransformMatrix
            worldTransforms[boneID] = worldMatrix
            
            for childID in bone.childIDs {
                updateRecursive(childID, parentWorld: worldMatrix)
            }
        }
        
        if let rootID = rootBoneID {
            updateRecursive(rootID, parentWorld: matrix_identity_float4x4)
        }
    }
    
    /// Get bone position in world space
    func boneWorldPosition(_ boneID: UUID) -> SIMD3<Float> {
        let world = worldTransforms[boneID] ?? matrix_identity_float4x4
        return SIMD3<Float>(world.columns.3.x, world.columns.3.y, world.columns.3.z)
    }
    
    /// Get bone tail (end) position in world space
    func boneTailWorldPosition(_ boneID: UUID) -> SIMD3<Float> {
        guard let bone = bones[boneID],
              let world = worldTransforms[boneID] else {
            return .zero
        }
        
        let tailLocal = SIMD4<Float>(0, bone.length, 0, 1)
        let tailWorld = world * tailLocal
        return SIMD3<Float>(tailWorld.x, tailWorld.y, tailWorld.z)
    }
    
    // MARK: - Bind Pose
    
    /// Store current pose as bind pose (for skinning)
    func captureBindPose() {
        updateWorldTransforms()
        bindPoseTransforms = worldTransforms
        
        // Calculate inverse bind pose matrices
        bindPoseInverses.removeAll()
        for (boneID, transform) in bindPoseTransforms {
            bindPoseInverses[boneID] = transform.inverse
        }
        
        print("[Skeleton] Captured bind pose for \(bones.count) bones")
    }
    
    /// Get inverse bind pose matrix for a bone
    func inverseBindPose(for boneID: UUID) -> simd_float4x4 {
        return bindPoseInverses[boneID] ?? matrix_identity_float4x4
    }
    
    // MARK: - Pose Capture
    
    /// Capture current skeleton pose
    func capturePose() -> SkeletonPose {
        return SkeletonPose(skeleton: self)
    }
    
    /// Apply a pose to the skeleton
    func applyPose(_ pose: SkeletonPose) {
        for snapshot in pose.boneTransforms {
            guard var bone = bones[snapshot.boneID] else { continue }
            bone.localPosition = snapshot.position
            bone.localRotation = snapshot.rotation
            bone.localScale = snapshot.scale
            bones[snapshot.boneID] = bone
        }
        updateWorldTransforms()
    }
    
    // MARK: - Traversal
    
    /// Get all bone IDs in depth-first order
    func allBoneIDsDepthFirst() -> [UUID] {
        var result: [UUID] = []
        
        func traverse(_ boneID: UUID) {
            result.append(boneID)
            if let bone = bones[boneID] {
                for childID in bone.childIDs {
                    traverse(childID)
                }
            }
        }
        
        if let rootID = rootBoneID {
            traverse(rootID)
        }
        
        return result
    }
    
    /// Get bone chain from root to specified bone
    func boneChainToRoot(_ boneID: UUID) -> [UUID] {
        var chain: [UUID] = []
        var currentID: UUID? = boneID
        
        while let id = currentID {
            chain.append(id)
            currentID = bones[id]?.parentID
        }
        
        return chain.reversed()
    }
    
    // MARK: - Selection
    
    func selectBone(_ boneID: UUID, additive: Bool = false) {
        if !additive {
            selectedBoneIDs.removeAll()
        }
        selectedBoneIDs.insert(boneID)
    }
    
    func deselectBone(_ boneID: UUID) {
        selectedBoneIDs.remove(boneID)
    }
    
    func clearSelection() {
        selectedBoneIDs.removeAll()
    }
    
    // MARK: - Preset Factory Methods
    
    /// Create a skeleton from a preset template
    static func create(preset: SkeletonPreset, name: String? = nil) -> Skeleton {
        let skeleton = Skeleton(name: name ?? preset.rawValue)
        
        switch preset {
        case .humanoid:
            createHumanoidRig(skeleton)
        case .sailboat:
            createSailboatRig(skeleton)
        case .motorboat:
            createMotorboatRig(skeleton)
        case .custom:
            break // Empty skeleton
        }
        
        if preset != .custom {
            skeleton.captureBindPose()
        }
        
        return skeleton
    }
    
    /// Create a humanoid character rig (19 bones)
    private static func createHumanoidRig(_ skeleton: Skeleton) {
        // Torso
        let hips = skeleton.addBone(name: "Hips", position: SIMD3<Float>(0, 1, 0), length: 0.2)
        let spine = skeleton.addBone(name: "Spine", parentID: hips, length: 0.3)
        let chest = skeleton.addBone(name: "Chest", parentID: spine, length: 0.3)
        let neck = skeleton.addBone(name: "Neck", parentID: chest, length: 0.1)
        let _ = skeleton.addBone(name: "Head", parentID: neck, length: 0.25)
        
        // Left Arm
        let shoulderL = skeleton.addBone(name: "Shoulder.L", parentID: chest, position: SIMD3<Float>(0.15, 0, 0), length: 0.1)
        let upperArmL = skeleton.addBone(name: "UpperArm.L", parentID: shoulderL, length: 0.3)
        let forearmL = skeleton.addBone(name: "Forearm.L", parentID: upperArmL, length: 0.25)
        let _ = skeleton.addBone(name: "Hand.L", parentID: forearmL, length: 0.1)
        
        // Right Arm
        let shoulderR = skeleton.addBone(name: "Shoulder.R", parentID: chest, position: SIMD3<Float>(-0.15, 0, 0), length: 0.1)
        let upperArmR = skeleton.addBone(name: "UpperArm.R", parentID: shoulderR, length: 0.3)
        let forearmR = skeleton.addBone(name: "Forearm.R", parentID: upperArmR, length: 0.25)
        let _ = skeleton.addBone(name: "Hand.R", parentID: forearmR, length: 0.1)
        
        // Left Leg
        let upperLegL = skeleton.addBone(name: "UpperLeg.L", parentID: hips, position: SIMD3<Float>(0.1, 0, 0), length: 0.45)
        let lowerLegL = skeleton.addBone(name: "LowerLeg.L", parentID: upperLegL, length: 0.4)
        let _ = skeleton.addBone(name: "Foot.L", parentID: lowerLegL, length: 0.15)
        
        // Right Leg
        let upperLegR = skeleton.addBone(name: "UpperLeg.R", parentID: hips, position: SIMD3<Float>(-0.1, 0, 0), length: 0.45)
        let lowerLegR = skeleton.addBone(name: "LowerLeg.R", parentID: upperLegR, length: 0.4)
        let _ = skeleton.addBone(name: "Foot.R", parentID: lowerLegR, length: 0.15)
    }
    
    /// Create a sailboat rig (6 bones)
    private static func createSailboatRig(_ skeleton: Skeleton) {
        // Root: Hull center (waterline)
        let hull = skeleton.addBone(name: "Hull", position: SIMD3<Float>(0, 0, 0), length: 3.0)
        
        // Rudder (steerable, at stern)
        var rudder = BoneNode(name: "Rudder", parentID: hull, localPosition: SIMD3<Float>(0, -0.5, -1.5), length: 0.8)
        rudder.color = .blue
        skeleton.bones[rudder.id] = rudder
        if var hullBone = skeleton.bones[hull] {
            hullBone.childIDs.append(rudder.id)
            skeleton.bones[hull] = hullBone
        }
        
        // Mast (vertical, amidships)
        var mast = BoneNode(name: "Mast", parentID: hull, localPosition: SIMD3<Float>(0, 0.5, 0), length: 4.0)
        mast.color = .brown
        skeleton.bones[mast.id] = mast
        if var hullBone = skeleton.bones[hull] {
            hullBone.childIDs.append(mast.id)
            skeleton.bones[hull] = hullBone
        }
        
        // Boom (horizontal, attached to mast, rotates for sail control)
        var boom = BoneNode(name: "Boom", parentID: mast.id, localPosition: SIMD3<Float>(0, 2.0, 0), length: 2.5)
        boom.color = .brown
        skeleton.bones[boom.id] = boom
        if var mastBone = skeleton.bones[mast.id] {
            mastBone.childIDs.append(boom.id)
            skeleton.bones[mast.id] = mastBone
        }
        
        // Main Sail (controlled by boom rotation, catches wind)
        var mainSail = BoneNode(name: "MainSail", parentID: boom.id, localPosition: SIMD3<Float>(0, 0, 0), length: 3.0)
        mainSail.color = .white
        skeleton.bones[mainSail.id] = mainSail
        if var boomBone = skeleton.bones[boom.id] {
            boomBone.childIDs.append(mainSail.id)
            skeleton.bones[boom.id] = boomBone
        }
        
        // Jib Sail (front sail, attached to hull)
        var jibSail = BoneNode(name: "JibSail", parentID: hull, localPosition: SIMD3<Float>(0, 1.5, 1.0), length: 2.5)
        jibSail.color = .white
        skeleton.bones[jibSail.id] = jibSail
        if var hullBone = skeleton.bones[hull] {
            hullBone.childIDs.append(jibSail.id)
            skeleton.bones[hull] = hullBone
        }
        
        skeleton.updateWorldTransforms()
    }
    
    /// Create a motorboat rig (4 bones)
    private static func createMotorboatRig(_ skeleton: Skeleton) {
        // Root: Hull center (waterline)
        let hull = skeleton.addBone(name: "Hull", position: SIMD3<Float>(0, 0, 0), length: 2.5)
        
        // Rudder (steerable)
        var rudder = BoneNode(name: "Rudder", parentID: hull, localPosition: SIMD3<Float>(0, -0.3, -1.2), length: 0.6)
        rudder.color = .blue
        skeleton.bones[rudder.id] = rudder
        if var hullBone = skeleton.bones[hull] {
            hullBone.childIDs.append(rudder.id)
            skeleton.bones[hull] = hullBone
        }
        
        // Propeller (rotates continuously for thrust)
        var propeller = BoneNode(name: "Propeller", parentID: hull, localPosition: SIMD3<Float>(0, -0.4, -1.5), length: 0.3)
        propeller.color = .orange
        skeleton.bones[propeller.id] = propeller
        if var hullBone = skeleton.bones[hull] {
            hullBone.childIDs.append(propeller.id)
            skeleton.bones[hull] = hullBone
        }
        
        // Outboard motor mount (tiltable)
        var motor = BoneNode(name: "OutboardMotor", parentID: hull, localPosition: SIMD3<Float>(0, 0, -1.3), length: 0.8)
        motor.color = .gray
        skeleton.bones[motor.id] = motor
        if var hullBone = skeleton.bones[hull] {
            hullBone.childIDs.append(motor.id)
            skeleton.bones[hull] = hullBone
        }
        
        skeleton.updateWorldTransforms()
    }
}

// MARK: - Skeleton Pose

/// Captures the complete pose of a skeleton at a moment in time
struct SkeletonPose: Identifiable {
    let id: UUID
    var name: String
    let boneTransforms: [BoneTransformSnapshot]
    
    init(skeleton: Skeleton, name: String = "Pose") {
        self.id = UUID()
        self.name = name
        self.boneTransforms = skeleton.bones.values.map { BoneTransformSnapshot(from: $0) }
    }
}

// MARK: - Skeleton Manager

class SkeletonManager: ObservableObject {
    static let shared = SkeletonManager()
    
    @Published var skeletons: [UUID: Skeleton] = [:]
    @Published var activeSkeletonID: UUID?
    
    var activeSkeleton: Skeleton? {
        guard let id = activeSkeletonID else { return nil }
        return skeletons[id]
    }
    
    func createSkeleton(name: String = "New Skeleton") -> Skeleton {
        let skeleton = Skeleton(name: name)
        skeletons[skeleton.id] = skeleton
        activeSkeletonID = skeleton.id
        return skeleton
    }
    
    /// Create a skeleton from a preset template
    func createSkeleton(preset: SkeletonPreset, name: String? = nil) -> Skeleton {
        let skeleton = Skeleton.create(preset: preset, name: name)
        skeletons[skeleton.id] = skeleton
        activeSkeletonID = skeleton.id
        print("[SkeletonManager] Created \(preset.rawValue) skeleton with \(skeleton.bones.count) bones")
        return skeleton
    }
    
    func deleteSkeleton(_ id: UUID) {
        skeletons.removeValue(forKey: id)
        if activeSkeletonID == id {
            activeSkeletonID = skeletons.keys.first
        }
    }
}
