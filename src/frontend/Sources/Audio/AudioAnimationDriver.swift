import Foundation
import SwiftUI
import simd

// MARK: - Audio Animation Driver
/// Links audio analysis to skeleton bone transforms in real-time
class AudioAnimationDriver: ObservableObject {
    static let shared = AudioAnimationDriver()
    
    @Published var mappings: [BoneAudioMapping] = []
    @Published var isEnabled: Bool = true
    
    private var previousValues: [UUID: SIMD3<Float>] = [:]
    
    // MARK: - Public API
    
    /// Update skeleton bones based on audio analysis
    func update(skeletonManager: SkeletonManager, analyzer: AudioAnalyzer, deltaTime: TimeInterval) {
        guard isEnabled, let activeSkeleton = skeletonManager.activeSkeleton else { return }
        
        for mapping in mappings {
            guard var bone = activeSkeleton.bones[mapping.boneID] else {
                continue
            }
            
            // Get audio value
            let audioValue = Float(analyzer.getNormalizedBand(mapping.audioChannel)) * mapping.multiplier
            
            // Apply smoothing
            let smoothedValue = applySmoothing(
                current: audioValue,
                boneID: mapping.boneID,
                deltaTime: deltaTime,
                smoothing: mapping.smoothing
            )
            
            // Apply to bone transform
            applyTransform(
                to: &bone,
                value: smoothedValue,
                mapping: mapping
            )
            
            // Update bone back to skeleton
            activeSkeleton.bones[mapping.boneID] = bone
        }
    }
    
    /// Add a new mapping
    func addMapping(_ mapping: BoneAudioMapping) {
        mappings.append(mapping)
    }
    
    /// Remove mapping by ID
    func removeMapping(id: UUID) {
        mappings.removeAll { $0.id == id }
    }
    
    /// Clear all mappings
    func clearMappings() {
        mappings.removeAll()
        previousValues.removeAll()
    }
    
    /// Load preset mapping (e.g., "dance", "idle sway")
    func loadPreset(_ preset: MappingPreset, skeletonManager: SkeletonManager) {
        clearMappings()
        
        switch preset {
        case .dance:
            loadDancePreset(skeletonManager: skeletonManager)
        case .idleSway:
            loadIdleSwayPreset(skeletonManager: skeletonManager)
        case .headBob:
            loadHeadBobPreset(skeletonManager: skeletonManager)
        }
    }
    
    // MARK: - Private Methods
    
    private func applySmoothing(current: Float, boneID: UUID, deltaTime: TimeInterval, smoothing: Float) -> Float {
        let key = boneID
        let previous = previousValues[key] ?? SIMD3<Float>(current, current, current)
        
        // Exponential moving average
        let smoothed = previous.x * smoothing + current * (1.0 - smoothing)
        
        previousValues[key] = SIMD3<Float>(smoothed, smoothed, smoothed)
        return smoothed
    }
    
    private func applyTransform(to bone: inout BoneNode, value: Float, mapping: BoneAudioMapping) {
        switch mapping.influenceType {
        case .position:
            applyPositionInfluence(to: &bone, value: value, axis: mapping.axis)
        case .rotation:
            applyRotationInfluence(to: &bone, value: value, axis: mapping.axis)
        case .scale:
            applyScaleInfluence(to: &bone, value: value, axis: mapping.axis)
        }
    }
    
    private func applyPositionInfluence(to bone: inout BoneNode, value: Float, axis: BoneAxis?) {
        if let axis = axis {
            switch axis {
            case .x: bone.localPosition.x += value
            case .y: bone.localPosition.y += value
            case .z: bone.localPosition.z += value
            }
        } else {
            bone.localPosition += SIMD3<Float>(value, value, value)
        }
    }
    
    private func applyRotationInfluence(to bone: inout BoneNode, value: Float, axis: BoneAxis?) {
        let angle = value * .pi  // Convert to radians
        
        if let axis = axis {
            let rotationAxis: SIMD3<Float>
            switch axis {
            case .x: rotationAxis = SIMD3<Float>(1, 0, 0)
            case .y: rotationAxis = SIMD3<Float>(0, 1, 0)
            case .z: rotationAxis = SIMD3<Float>(0, 0, 1)
            }
            
            let rotation = simd_quatf(angle: angle, axis: rotationAxis)
            bone.localRotation = rotation * bone.localRotation
        } else {
            // Rotate around all axes equally
            let rotation = simd_quatf(angle: angle, axis: normalize(SIMD3<Float>(1, 1, 1)))
            bone.localRotation = rotation * bone.localRotation
        }
    }
    
    private func applyScaleInfluence(to bone: inout BoneNode, value: Float, axis: BoneAxis?) {
        let scaleValue = 1.0 + value * 0.1  // Small scale changes
        
        if let axis = axis {
            switch axis {
            case .x: bone.localScale.x *= scaleValue
            case .y: bone.localScale.y *= scaleValue
            case .z: bone.localScale.z *= scaleValue
            }
        } else {
            bone.localScale *= scaleValue
        }
    }
    
    // MARK: - Presets
    
    private func loadDancePreset(skeletonManager: SkeletonManager) {
        guard let activeSkeleton = skeletonManager.activeSkeleton else { return }
        
        // Find common bones
        let hipsBone = activeSkeleton.bones.values.first { $0.name.lowercased().contains("hips") || $0.name.lowercased().contains("pelvis") }
        let leftArmBone = activeSkeleton.bones.values.first { $0.name.lowercased().contains("left") && $0.name.lowercased().contains("arm") }
        let rightArmBone = activeSkeleton.bones.values.first { $0.name.lowercased().contains("right") && $0.name.lowercased().contains("arm") }
        let spineBone = activeSkeleton.bones.values.first { $0.name.lowercased().contains("spine") }
        
        // Hips bounce with bass
        if let hips = hipsBone {
            addMapping(BoneAudioMapping(
                boneID: hips.id,
                audioChannel: .bass,
                influenceType: .position,
                axis: .y,
                multiplier: 0.5,
                smoothing: 0.6
            ))
        }
        
        // Arms sway with mids
        if let leftArm = leftArmBone {
            addMapping(BoneAudioMapping(
                boneID: leftArm.id,
                audioChannel: .mid,
                influenceType: .rotation,
                axis: .z,
                multiplier: 1.5,
                smoothing: 0.7
            ))
        }
        
        if let rightArm = rightArmBone {
            addMapping(BoneAudioMapping(
                boneID: rightArm.id,
                audioChannel: .treble,
                influenceType: .rotation,
                axis: .z,
                multiplier: -1.5,
                smoothing: 0.7
            ))
        }
        
        // Spine rotation with beat
        if let spine = spineBone {
            addMapping(BoneAudioMapping(
                boneID: spine.id,
                audioChannel: .beatStrength,
                influenceType: .rotation,
                axis: .y,
                multiplier: 2.0,
                smoothing: 0.5
            ))
        }
    }
    
    private func loadIdleSwayPreset(skeletonManager: SkeletonManager) {
        guard let activeSkeleton = skeletonManager.activeSkeleton else { return }
        let spineBone = activeSkeleton.bones.values.first { $0.name.lowercased().contains("spine") }
        
        if let spine = spineBone {
            // Gentle breathing effect
            addMapping(BoneAudioMapping(
                boneID: spine.id,
                audioChannel: .amplitude,
                influenceType: .scale,
                axis: .y,
                multiplier: 0.1,
                smoothing: 0.9
            ))
        }
    }
    
    private func loadHeadBobPreset(skeletonManager: SkeletonManager) {
        guard let activeSkeleton = skeletonManager.activeSkeleton else { return }
        let headBone = activeSkeleton.bones.values.first { $0.name.lowercased().contains("head") }
        
        if let head = headBone {
            // Head bobs to beat
            addMapping(BoneAudioMapping(
                boneID: head.id,
                audioChannel: .beatStrength,
                influenceType: .position,
                axis: .y,
                multiplier: 0.3,
                smoothing: 0.4
            ))
        }
    }
}

// MARK: - Supporting Types

struct BoneAudioMapping: Identifiable {
    let id: UUID
    let boneID: UUID
    let audioChannel: AudioChannel
    let influenceType: InfluenceType
    let axis: BoneAxis?
    let multiplier: Float
    let smoothing: Float
    
    init(
        id: UUID = UUID(),
        boneID: UUID,
        audioChannel: AudioChannel,
        influenceType: InfluenceType,
        axis: BoneAxis?,
        multiplier: Float,
        smoothing: Float
    ) {
        self.id = id
        self.boneID = boneID
        self.audioChannel = audioChannel
        self.influenceType = influenceType
        self.axis = axis
        self.multiplier = multiplier
        self.smoothing = smoothing
    }
}

enum InfluenceType: String, CaseIterable, Identifiable {
    case position = "Position"
    case rotation = "Rotation"
    case scale = "Scale"
    
    var id: String { rawValue }
}

enum BoneAxis: String, CaseIterable, Identifiable {
    case x = "X"
    case y = "Y"
    case z = "Z"
    
    var id: String { rawValue }
}

enum MappingPreset: String, CaseIterable, Identifiable {
    case dance = "Dance"
    case idleSway = "Idle Sway"
    case headBob = "Head Bob"
    
    var id: String { rawValue }
}
