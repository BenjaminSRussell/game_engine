import SwiftUI
import simd

// MARK: - Motion Importer
/// Import motion capture and animation data from various formats

class MotionImporter: ObservableObject {
    static let shared = MotionImporter()
    
    @Published var importProgress: Double = 0
    @Published var lastImportError: String?
    @Published var importedClips: [ImportedAnimationClip] = []
    
    // MARK: - BVH Import
    
    /// Parse BVH (Biovision Hierarchy) motion capture file
    func importBVH(from url: URL, targetSkeleton: Skeleton) throws -> ImportedAnimationClip {
        let content = try String(contentsOf: url, encoding: .utf8)
        return try parseBVH(content: content, fileName: url.lastPathComponent, targetSkeleton: targetSkeleton)
    }
    
    func importBVH(content: String, fileName: String, targetSkeleton: Skeleton) throws -> ImportedAnimationClip {
        return try parseBVH(content: content, fileName: fileName, targetSkeleton: targetSkeleton)
    }
    
    private func parseBVH(content: String, fileName: String, targetSkeleton: Skeleton) throws -> ImportedAnimationClip {
        let lines = content.components(separatedBy: .newlines).map { $0.trimmingCharacters(in: .whitespaces) }
        var lineIndex = 0
        
        var bvhJoints: [BVHJoint] = []
        var jointStack: [BVHJoint] = []
        var frameTime: Float = 1.0 / 30.0
        var frameCount = 0
        var motionData: [[Float]] = []
        
        // Parse HIERARCHY section
        while lineIndex < lines.count {
            let line = lines[lineIndex]
            
            if line == "HIERARCHY" {
                lineIndex += 1
                continue
            }
            
            if line.hasPrefix("ROOT") || line.hasPrefix("JOINT") {
                let name = line.components(separatedBy: .whitespaces).last ?? "Joint"
                let parentName = jointStack.last?.name
                let joint = BVHJoint(name: name, parentName: parentName)
                bvhJoints.append(joint)
                jointStack.append(joint)
            }
            
            if line.hasPrefix("OFFSET") {
                let values = line.replacingOccurrences(of: "OFFSET", with: "")
                    .trimmingCharacters(in: .whitespaces)
                    .components(separatedBy: .whitespaces)
                    .compactMap { Float($0) }
                
                if values.count >= 3, let joint = jointStack.last {
                    var updatedJoint = joint
                    updatedJoint.offset = SIMD3<Float>(values[0], values[1], values[2])
                    if let index = bvhJoints.firstIndex(where: { $0.name == joint.name }) {
                        bvhJoints[index] = updatedJoint
                    }
                }
            }
            
            if line.hasPrefix("CHANNELS") {
                let parts = line.components(separatedBy: .whitespaces)
                if parts.count >= 2, let channelCount = Int(parts[1]), let joint = jointStack.last {
                    var updatedJoint = joint
                    updatedJoint.channelCount = channelCount
                    updatedJoint.channels = Array(parts.dropFirst(2))
                    if let index = bvhJoints.firstIndex(where: { $0.name == joint.name }) {
                        bvhJoints[index] = updatedJoint
                    }
                }
            }
            
            if line == "End Site" {
                // Skip end site
                while lineIndex < lines.count && !lines[lineIndex].contains("}") {
                    lineIndex += 1
                }
            }
            
            if line == "}" {
                jointStack.removeLast()
            }
            
            if line == "MOTION" {
                lineIndex += 1
                break
            }
            
            lineIndex += 1
        }
        
        // Parse MOTION section
        while lineIndex < lines.count {
            let line = lines[lineIndex]
            
            if line.hasPrefix("Frames:") {
                let value = line.replacingOccurrences(of: "Frames:", with: "").trimmingCharacters(in: .whitespaces)
                frameCount = Int(value) ?? 0
            }
            
            if line.hasPrefix("Frame Time:") {
                let value = line.replacingOccurrences(of: "Frame Time:", with: "").trimmingCharacters(in: .whitespaces)
                frameTime = Float(value) ?? (1.0 / 30.0)
            }
            
            // Motion data lines
            let values = line.components(separatedBy: .whitespaces).compactMap { Float($0) }
            if !values.isEmpty && values.count > 3 {
                motionData.append(values)
            }
            
            lineIndex += 1
        }
        
        // Create animation clip
        let duration = Float(frameCount) * frameTime
        var clip = ImportedAnimationClip(
            name: fileName.replacingOccurrences(of: ".bvh", with: ""),
            duration: duration,
            frameRate: 1.0 / frameTime
        )
        
        // Convert motion data to keyframes
        clip.boneKeyframes = convertBVHMotionToKeyframes(
            joints: bvhJoints,
            motionData: motionData,
            frameTime: frameTime,
            targetSkeleton: targetSkeleton
        )
        
        importedClips.append(clip)
        print("[MotionImporter] Imported BVH: \(fileName), \(frameCount) frames, \(bvhJoints.count) joints")
        
        return clip
    }
    
    private func convertBVHMotionToKeyframes(
        joints: [BVHJoint],
        motionData: [[Float]],
        frameTime: Float,
        targetSkeleton: Skeleton
    ) -> [UUID: [BoneKeyframe]] {
        var result: [UUID: [BoneKeyframe]] = [:]
        
        // Try to match BVH joints to skeleton bones by name
        var jointToBoneMapping: [String: UUID] = [:]
        for joint in joints {
            // Try exact match first
            if let bone = targetSkeleton.bones.values.first(where: { $0.name.lowercased() == joint.name.lowercased() }) {
                jointToBoneMapping[joint.name] = bone.id
            }
            // Try partial match
            else if let bone = targetSkeleton.bones.values.first(where: { $0.name.lowercased().contains(joint.name.lowercased()) || joint.name.lowercased().contains($0.name.lowercased()) }) {
                jointToBoneMapping[joint.name] = bone.id
            }
        }
        
        // Process each frame
        for (frameIndex, frameData) in motionData.enumerated() {
            let time = Float(frameIndex) * frameTime
            var dataIndex = 0
            
            for joint in joints {
                guard dataIndex + joint.channelCount <= frameData.count else { break }
                
                var position = joint.offset
                var rotation = SIMD3<Float>.zero
                
                for i in 0..<joint.channelCount {
                    let value = frameData[dataIndex + i]
                    let channel = joint.channels[i]
                    
                    switch channel {
                    case "Xposition": position.x += value
                    case "Yposition": position.y += value
                    case "Zposition": position.z += value
                    case "Xrotation": rotation.x = value * Float.pi / 180
                    case "Yrotation": rotation.y = value * Float.pi / 180
                    case "Zrotation": rotation.z = value * Float.pi / 180
                    default: break
                    }
                }
                
                dataIndex += joint.channelCount
                
                // Add keyframe for mapped bone
                if let boneID = jointToBoneMapping[joint.name] {
                    let quaternion = simd_quatf(
                        angle: rotation.z, axis: SIMD3<Float>(0, 0, 1)
                    ) * simd_quatf(
                        angle: rotation.y, axis: SIMD3<Float>(0, 1, 0)
                    ) * simd_quatf(
                        angle: rotation.x, axis: SIMD3<Float>(1, 0, 0)
                    )
                    
                    let keyframe = BoneKeyframe(
                        time: time,
                        position: position,
                        rotation: quaternion,
                        scale: SIMD3<Float>(1, 1, 1)
                    )
                    
                    if result[boneID] == nil {
                        result[boneID] = []
                    }
                    result[boneID]?.append(keyframe)
                }
            }
        }
        
        return result
    }
    
    // MARK: - Animation Application
    
    /// Apply imported animation clip to skeleton at given time
    func applyAnimation(_ clip: ImportedAnimationClip, to skeleton: Skeleton, at time: Float) {
        let loopedTime = time.truncatingRemainder(dividingBy: clip.duration)
        
        for (boneID, keyframes) in clip.boneKeyframes {
            guard var bone = skeleton.bones[boneID], !keyframes.isEmpty else { continue }
            
            // Find surrounding keyframes
            let sortedKeyframes = keyframes.sorted { $0.time < $1.time }
            
            var prevKeyframe = sortedKeyframes.last!
            var nextKeyframe = sortedKeyframes.first!
            
            for i in 0..<sortedKeyframes.count {
                if sortedKeyframes[i].time <= loopedTime {
                    prevKeyframe = sortedKeyframes[i]
                    nextKeyframe = sortedKeyframes[(i + 1) % sortedKeyframes.count]
                }
            }
            
            // Interpolate
            let t: Float
            if nextKeyframe.time > prevKeyframe.time {
                t = (loopedTime - prevKeyframe.time) / (nextKeyframe.time - prevKeyframe.time)
            } else {
                t = 0
            }
            
            bone.localPosition = simd_mix(prevKeyframe.position, nextKeyframe.position, SIMD3<Float>(repeating: t))
            bone.localRotation = simd_slerp(prevKeyframe.rotation, nextKeyframe.rotation, t)
            bone.localScale = simd_mix(prevKeyframe.scale, nextKeyframe.scale, SIMD3<Float>(repeating: t))
            
            skeleton.bones[boneID] = bone
        }
        
        skeleton.updateWorldTransforms()
    }
}

// MARK: - BVH Joint

struct BVHJoint: Equatable {
    var name: String
    var parentName: String?
    var offset: SIMD3<Float> = .zero
    var channelCount: Int = 0
    var channels: [String] = []
    
    init(name: String, parentName: String? = nil) {
        self.name = name
        self.parentName = parentName
    }
}

// MARK: - Imported Animation Clip

struct ImportedAnimationClip: Identifiable {
    let id = UUID()
    var name: String
    var duration: Float
    var frameRate: Float
    var boneKeyframes: [UUID: [BoneKeyframe]] = [:]
}

// MARK: - Bone Keyframe

struct BoneKeyframe {
    var time: Float
    var position: SIMD3<Float>
    var rotation: simd_quatf
    var scale: SIMD3<Float>
    var interpolation: InterpolationType = .linear
    
    enum InterpolationType {
        case linear
        case bezier
        case step
    }
}

// MARK: - Animation Retargeter

class AnimationRetargeter {
    
    /// Retarget animation from source skeleton proportions to target skeleton
    static func retarget(
        animation: ImportedAnimationClip,
        fromSkeleton source: Skeleton,
        toSkeleton target: Skeleton,
        boneMapping: [UUID: UUID]
    ) -> ImportedAnimationClip {
        var retargetedClip = ImportedAnimationClip(
            name: animation.name + " (Retargeted)",
            duration: animation.duration,
            frameRate: animation.frameRate
        )
        
        for (sourceBoneID, keyframes) in animation.boneKeyframes {
            guard let targetBoneID = boneMapping[sourceBoneID],
                  let sourceBone = source.bones[sourceBoneID],
                  let targetBone = target.bones[targetBoneID] else {
                continue
            }
            
            // Calculate scale factor between bones
            let scaleFactor = targetBone.length / max(sourceBone.length, 0.001)
            
            // Retarget keyframes
            let retargetedKeyframes = keyframes.map { keyframe -> BoneKeyframe in
                var newKeyframe = keyframe
                newKeyframe.position = keyframe.position * scaleFactor
                // Rotation is preserved (assuming same joint orientations)
                return newKeyframe
            }
            
            retargetedClip.boneKeyframes[targetBoneID] = retargetedKeyframes
        }
        
        return retargetedClip
    }
}

// MARK: - Motion Import Panel View

struct MotionImportPanel: View {
    @ObservedObject var importer = MotionImporter.shared
    @ObservedObject var skeletonManager = SkeletonManager.shared
    @State private var showFilePicker = false
    @State private var selectedClipID: UUID?
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Motion Library")
                    .font(DesignSystem.Typography.headline)
                Spacer()
                Button {
                    showFilePicker = true
                } label: {
                    Label("Import", systemImage: "square.and.arrow.down")
                }
                .buttonStyle(.bordered)
            }
            .padding(DesignSystem.Spacing.sm)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Clip list
            if importer.importedClips.isEmpty {
                VStack(spacing: DesignSystem.Spacing.md) {
                    Image(systemName: "figure.walk.motion")
                        .font(.system(size: 48))
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                    Text("No animations imported")
                        .font(DesignSystem.Typography.body)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    Text("Import BVH or FBX files")
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textTertiary)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                List(importer.importedClips, selection: $selectedClipID) { clip in
                    HStack {
                        Image(systemName: "waveform.path")
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                        VStack(alignment: .leading) {
                            Text(clip.name)
                                .font(DesignSystem.Typography.body)
                            Text("\(String(format: "%.1f", clip.duration))s  \(Int(clip.frameRate)) fps")
                                .font(DesignSystem.Typography.caption)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        Spacer()
                        Text("\(clip.boneKeyframes.count) bones")
                            .font(DesignSystem.Typography.caption)
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                    }
                    .padding(.vertical, DesignSystem.Spacing.xs)
                }
            }
            
            // Error display
            if let error = importer.lastImportError {
                HStack {
                    Image(systemName: "exclamationmark.triangle")
                        .foregroundColor(DesignSystem.Colors.accentDanger)
                    Text(error)
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.accentDanger)
                }
                .padding(DesignSystem.Spacing.sm)
                .background(DesignSystem.Colors.accentDanger.opacity(0.1))
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .fileImporter(
            isPresented: $showFilePicker,
            allowedContentTypes: [.data],
            allowsMultipleSelection: false
        ) { result in
            switch result {
            case .success(let urls):
                if let url = urls.first, let skeleton = skeletonManager.activeSkeleton {
                    do {
                        _ = try importer.importBVH(from: url, targetSkeleton: skeleton)
                        importer.lastImportError = nil
                    } catch {
                        importer.lastImportError = error.localizedDescription
                    }
                }
            case .failure(let error):
                importer.lastImportError = error.localizedDescription
            }
        }
    }
}
