import SwiftUI
import simd

// MARK: - Bone Node
/// Represents a single bone in a skeletal hierarchy

struct BoneNode: Identifiable, Hashable {
    let id: UUID
    var name: String
    var parentID: UUID?
    var childIDs: [UUID] = []
    
    // Transform data
    var localPosition: SIMD3<Float>
    var localRotation: simd_quatf
    var localScale: SIMD3<Float>
    
    // Bone properties
    var length: Float
    var roll: Float = 0  // Twist around bone axis
    
    // Visualization
    var color: Color = .cyan
    var isVisible: Bool = true
    var isLocked: Bool = false
    var isSelected: Bool = false
    
    // IK hints (for future inverse kinematics)
    var ikPoleVector: SIMD3<Float>?
    var ikConstraints: IKConstraints?
    
    // MARK: - Initialization
    
    init(
        id: UUID = UUID(),
        name: String,
        parentID: UUID? = nil,
        localPosition: SIMD3<Float> = .zero,
        localRotation: simd_quatf = simd_quatf(ix: 0, iy: 0, iz: 0, r: 1),
        localScale: SIMD3<Float> = SIMD3<Float>(1, 1, 1),
        length: Float = 1.0
    ) {
        self.id = id
        self.name = name
        self.parentID = parentID
        self.localPosition = localPosition
        self.localRotation = localRotation
        self.localScale = localScale
        self.length = length
    }
    
    // MARK: - Transform Helpers
    
    /// Local transform matrix
    var localTransformMatrix: simd_float4x4 {
        let translation = simd_float4x4(translation: localPosition)
        let rotation = simd_float4x4(localRotation)
        let scale = simd_float4x4(scale: localScale)
        return translation * rotation * scale
    }
    
    /// Bone end position in local space (tip of bone)
    var tailPositionLocal: SIMD3<Float> {
        return SIMD3<Float>(0, length, 0)  // Bones extend along Y axis
    }
    
    // MARK: - Hashable
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
    
    static func == (lhs: BoneNode, rhs: BoneNode) -> Bool {
        lhs.id == rhs.id
    }
}

// MARK: - IK Constraints

struct IKConstraints {
    var minAngle: SIMD3<Float> = SIMD3<Float>(-Float.pi, -Float.pi, -Float.pi)
    var maxAngle: SIMD3<Float> = SIMD3<Float>(Float.pi, Float.pi, Float.pi)
    var stiffness: Float = 1.0
    var dampening: Float = 0.5
    var isLimited: Bool = false
}

/// Captures the transform state of a bone at a moment in time
struct BoneTransformSnapshot: Identifiable {
    var id: UUID { boneID }
    let boneID: UUID
    var position: SIMD3<Float>
    var rotationX: Float
    var rotationY: Float
    var rotationZ: Float
    var rotationW: Float
    var scale: SIMD3<Float>
    
    var rotation: simd_quatf {
        simd_quatf(ix: rotationX, iy: rotationY, iz: rotationZ, r: rotationW)
    }
    
    init(from bone: BoneNode) {
        self.boneID = bone.id
        self.position = bone.localPosition
        self.rotationX = bone.localRotation.imag.x
        self.rotationY = bone.localRotation.imag.y
        self.rotationZ = bone.localRotation.imag.z
        self.rotationW = bone.localRotation.real
        self.scale = bone.localScale
    }
}

// MARK: - Matrix Extensions

extension simd_float4x4 {
    init(translation: SIMD3<Float>) {
        self = matrix_identity_float4x4
        self.columns.3 = SIMD4<Float>(translation.x, translation.y, translation.z, 1)
    }
    
    init(scale: SIMD3<Float>) {
        self = matrix_identity_float4x4
        self.columns.0.x = scale.x
        self.columns.1.y = scale.y
        self.columns.2.z = scale.z
    }
    
    init(_ quaternion: simd_quatf) {
        self = simd_float4x4(quaternion)
    }
}
