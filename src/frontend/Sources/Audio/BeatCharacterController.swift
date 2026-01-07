import Foundation
import SwiftUI
import simd

// MARK: - Beat-Synced Character Controller
/// Controls character movement synced to music beats
class BeatCharacterController: ObservableObject {
    static let shared = BeatCharacterController()
    
    @Published var isEnabled: Bool = true
    @Published var currentAction: CharacterAction = .idle
    @Published var beatPhase: Double = 0.0
    
    // Movement parameters
    var walkSpeed: Float = 2.0  // units per second
    var runSpeed: Float = 5.0
    var jumpHeight: Float = 2.0
    
    // Animation blending
    private var currentAnimationWeight: Float = 1.0
    private var targetAnimationWeight: Float = 1.0
    private var blendSpeed: Float = 5.0
    
    // Footstep IK
    private var leftFootTarget: SIMD3<Float> = .zero
    private var rightFootTarget: SIMD3<Float> = .zero
    private var footStepPhase: Double = 0.0
    
    // MARK: - Public API
    
    /// Update character based on beat timing
    func update(timingSystem: AudioTimingSystem, skeleton: Skeleton, deltaTime: TimeInterval) {
        guard isEnabled else { return }
        
        beatPhase = timingSystem.beatPhase
        
        // Update animation blending
        updateAnimationBlending(deltaTime: deltaTime)
        
        // Execute current action
        executeAction(skeleton: skeleton, deltaTime: deltaTime)
        
        // Update footstep IK
        if currentAction == .walk || currentAction == .run {
            updateFootstepIK(skeleton: skeleton, timingSystem: timingSystem)
        }
    }
    
    /// Trigger action on specific beat
    func triggerAction(_ action: CharacterAction, skeleton: Skeleton) {
        // Smooth transition to new action
        currentAction = action
        targetAnimationWeight = 1.0
        
        print("🎵 Triggered: \(action.rawValue)")
    }
    
    /// Get ideal foot plant position based on beat
    func getFootPlantPosition(isLeftFoot: Bool, beatPhase: Double) -> SIMD3<Float> {
        // Foot plants at beat 0.0 and 0.5 (alternate feet)
        let phase = beatPhase * 2.0  // Two steps per beat
        let isPlantPhase = (isLeftFoot && phase.truncatingRemainder(dividingBy: 2.0) < 1.0) ||
                          (!isLeftFoot && phase.truncatingRemainder(dividingBy: 2.0) >= 1.0)
        
        if isPlantPhase {
            let stepDistance: Float = 0.5
            let lateralOffset: Float = isLeftFoot ? -0.3 : 0.3
            return SIMD3<Float>(lateralOffset, 0.0, stepDistance * Float(phase))
        }
        
        return SIMD3<Float>(isLeftFoot ? -0.3 : 0.3, 0.3, 0.0)  // Lift foot
    }
    
    // MARK: - Private Methods
    
    private func updateAnimationBlending(deltaTime: TimeInterval) {
        // Smooth blend animation weights
        let blendDelta = blendSpeed * Float(deltaTime)
        if currentAnimationWeight < targetAnimationWeight {
            currentAnimationWeight = min(currentAnimationWeight + blendDelta, targetAnimationWeight)
        } else if currentAnimationWeight > targetAnimationWeight {
            currentAnimationWeight = max(currentAnimationWeight - blendDelta, targetAnimationWeight)
        }
    }
    
    private func executeAction(skeleton: Skeleton, deltaTime: TimeInterval) {
        switch currentAction {
        case .walk:
            applyWalkAnimation(skeleton: skeleton, deltaTime: deltaTime)
        case .run:
            applyRunAnimation(skeleton: skeleton, deltaTime: deltaTime)
        case .jump:
            applyJumpAnimation(skeleton: skeleton)
        case .crouch:
            applyCrouchAnimation(skeleton: skeleton)
        case .dance:
            applyDanceAnimation(skeleton: skeleton)
        case .turnLeft, .turnRight:
            applyTurnAnimation(skeleton: skeleton, left: currentAction == .turnLeft)
        case .idle:
            applyIdleAnimation(skeleton: skeleton)
        }
    }
    
    // MARK: - Animation Applications
    
    private func applyWalkAnimation(skeleton: Skeleton, deltaTime: TimeInterval) {
        guard var hips = skeleton.bones.values.first(where: { $0.name.lowercased().contains("hips") }) else { return }
        
        // Bob hips based on beat phase
        let bobHeight = sin(beatPhase * .pi * 2) * 0.1
        hips.localPosition.y += Float(bobHeight) * currentAnimationWeight
        
        // Move forward
        hips.localPosition.z += walkSpeed * Float(deltaTime) * currentAnimationWeight
        
        skeleton.bones[hips.id] = hips
    }
    
    private func applyRunAnimation(skeleton: Skeleton, deltaTime: TimeInterval) {
        guard var hips = skeleton.bones.values.first(where: { $0.name.lowercased().contains("hips") }) else { return }
        
        let bobHeight = sin(beatPhase * .pi * 4) * 0.15  // Faster bob
        hips.localPosition.y += Float(bobHeight) * currentAnimationWeight
        hips.localPosition.z += runSpeed * Float(deltaTime) * currentAnimationWeight
        
        skeleton.bones[hips.id] = hips
    }
    
    private func applyJumpAnimation(skeleton: Skeleton) {
        guard var hips = skeleton.bones.values.first(where: { $0.name.lowercased().contains("hips") }) else { return }
        
        // Jump curve: starts at beat, peaks at 0.5, lands at 1.0
        let jumpCurve = sin(beatPhase * .pi)
        let jumpOffset = Float(jumpCurve) * jumpHeight
        
        hips.localPosition.y += jumpOffset * currentAnimationWeight
        
        skeleton.bones[hips.id] = hips
    }
    
    private func applyCrouchAnimation(skeleton: Skeleton) {
        guard var hips = skeleton.bones.values.first(where: { $0.name.lowercased().contains("hips") }) else { return }
        
        hips.localPosition.y -= 0.5 * currentAnimationWeight
        
        skeleton.bones[hips.id] = hips
    }
    
    private func applyDanceAnimation(skeleton: Skeleton) {
        // Already handled by AudioAnimationDriver, but we can enhance here
        guard var spine = skeleton.bones.values.first(where: { $0.name.lowercased().contains("spine") }) else { return }
        
        let rotationAngle = sin(beatPhase * .pi * 2) * 0.3
        let rotation = simd_quatf(angle: Float(rotationAngle), axis: SIMD3<Float>(0, 1, 0))
        
        spine.localRotation = rotation * spine.localRotation
        
        skeleton.bones[spine.id] = spine
    }
    
    private func applyTurnAnimation(skeleton: Skeleton, left: Bool) {
        guard var hips = skeleton.bones.values.first(where: { $0.name.lowercased().contains("hips") }) else { return }
        
        let turnAngle = Float(beatPhase * .pi / 2) * (left ? 1.0 : -1.0)
        let rotation = simd_quatf(angle: turnAngle, axis: SIMD3<Float>(0, 1, 0))
        
        hips.localRotation = rotation
        
        skeleton.bones[hips.id] = hips
    }
    
    private func applyIdleAnimation(skeleton: Skeleton) {
        guard var spine = skeleton.bones.values.first(where: { $0.name.lowercased().contains("spine") }) else { return }
        
        // Gentle breathing
        let breathPhase = beatPhase * .pi * 2
        let breathAmount = sin(breathPhase) * 0.05
        
        spine.localScale.y = 1.0 + Float(breathAmount) * currentAnimationWeight
        
        skeleton.bones[spine.id] = spine
    }
    
    // MARK: - Footstep IK
    
    private func updateFootstepIK(skeleton: Skeleton, timingSystem: AudioTimingSystem) {
        // Get foot bones
        guard let leftFoot = skeleton.bones.values.first(where: { $0.name.lowercased().contains("left") && $0.name.lowercased().contains("foot") }),
              let rightFoot = skeleton.bones.values.first(where: { $0.name.lowercased().contains("right") && $0.name.lowercased().contains("foot") }) else {
            return
        }
        
        // Calculate target positions based on beat phase
        leftFootTarget = getFootPlantPosition(isLeftFoot: true, beatPhase: timingSystem.beatPhase)
        rightFootTarget = getFootPlantPosition(isLeftFoot: false, beatPhase: timingSystem.beatPhase)
        
        // Apply IK (simplified 2-bone IK)
        applyFootIK(foot: leftFoot, target: leftFootTarget, skeleton: skeleton)
        applyFootIK(foot: rightFoot, target: rightFootTarget, skeleton: skeleton)
    }
    
    private func applyFootIK(foot: BoneNode, target: SIMD3<Float>, skeleton: Skeleton) {
        // Simplified IK: just move foot to target
        // In production, this would use proper 2-bone IK solver (FABRIK or analytic)
        
        guard var footCopy = skeleton.bones[foot.id] else { return }
        
        // Lerp to target position
        let lerpSpeed: Float = 0.2
        footCopy.localPosition = footCopy.localPosition * (1.0 - lerpSpeed) + target * lerpSpeed
        
        skeleton.bones[footCopy.id] = footCopy
    }
}

// MARK: - Beat Trigger Bridge
/// Connects timing system triggers to character controller
class BeatTriggerBridge: ObservableObject {
    static let shared = BeatTriggerBridge()
    
    func setupTriggers(timingSystem: AudioTimingSystem, characterController: BeatCharacterController, skeleton: Skeleton) {
        // Listen for action triggers
        for trigger in timingSystem.actionTriggers {
            trigger.callback = { [weak characterController] in
                guard let controller = characterController else { return }
                controller.triggerAction(trigger.actionType, skeleton: skeleton)
            }
        }
    }
}
