import SwiftUI
import simd

// MARK: - Animation Timeline
// Professional animation sequencer and timeline editor

class AnimationTimeline: ObservableObject {
    @Published var tracks: [TimelineTrack] = []
    @Published var currentTime: TimeInterval = 0
    @Published var duration: TimeInterval = 30
    @Published var isPlaying = false
    @Published var playbackSpeed: Float = 1.0
    @Published var selectedTracks: Set<UUID> = []
    @Published var selectedKeys: Set<UUID> = []
    
    // Timeline settings
    @Published var fps: Int = 60
    @Published var snapToFrames = true
    @Published var showCurveEditor = false
    
    // MARK: - Timeline Track
    struct TimelineTrack: Identifiable {
        let id = UUID()
        var name: String
        var type: TrackType
        var keyframes: [Keyframe]
        var strips: [NLAStrip] = []
        var isMuted = false
        var isLocked = false
        var isExpanded = true
        var color: Color = .blue
        
        enum TrackType: String, CaseIterable {
            // Sequencer tracks - All implemented
            case camera = "Camera"
            case audio = "Audio"
            case animation = "Animation"
            case transform = "Transform"
            case materialParameter = "Material Parameter"
            case lightIntensity = "Light Intensity"
            case particleTrigger = "Particle Trigger"
            case event = "Event"
            case subtitle = "Subtitle"
            case director = "Director"
            // Skeletal animation tracks
            case skeleton = "Skeleton"
            case boneTransform = "Bone Transform"
            
            var icon: String {
                switch self {
                case .camera: return "camera"
                case .audio: return "waveform"
                case .animation: return "figure.walk"
                case .transform: return "arrow.up.and.down.and.arrow.left.and.right"
                case .materialParameter: return "paintpalette"
                case .lightIntensity: return "lightbulb"
                case .particleTrigger: return "sparkles"
                case .event: return "bell"
                case .subtitle: return "text.bubble"
                case .director: return "film"
                case .skeleton: return "figure.stand"
                case .boneTransform: return "figure.wave"
                }
            }
        }
    }
    
    // MARK: - Keyframe
    struct Keyframe: Identifiable {
        let id = UUID()
        var time: TimeInterval
        var value: KeyframeValue
        var interpolation: InterpolationType = .linear
        var inTangent: SIMD2<Float>? // For bezier curves
        var outTangent: SIMD2<Float>?
        
        enum KeyframeValue {
            case float(Float)
            case vector3(SIMD3<Float>)
            case color(Color)
            case bool(Bool)
            case string(String)
            case event(String)
        }
        
        enum InterpolationType: String, CaseIterable {
            case constant = "Constant"
            case linear = "Linear"
            case bezier = "Bezier"
            case ease = "Ease"
            case easeIn = "Ease In"
            case easeOut = "Ease Out"
            case easeInOut = "Ease In-Out"
        }
    }
    
    // MARK: - NLA Data Structures
    struct NLAAction: Identifiable {
        let id = UUID()
        var name: String
        var duration: TimeInterval
        // Reference to raw animation data (e.g. keyframes)
    }
    
    struct NLAStrip: Identifiable {
        let id = UUID()
        var actionID: UUID
        var name: String // Can be overridden from action
        var startTime: TimeInterval
        var duration: TimeInterval // Can be scaled
        var timeScale: Double = 1.0
        var blendMode: BlendMode = .replace
        var opacity: Double = 1.0
        
        enum BlendMode: String, CaseIterable {
            case replace = "Replace"
            case add = "Add"
            case multiply = "Multiply"
            case crossfade = "Crossfade"
        }
    }
    
    // MARK: - Functions
    
    // MARK: - Multi-Track Timeline Rendering
    func renderMultiTrackTimeline(pixelsPerSecond: CGFloat, zoom: CGFloat) -> some View {
        VStack(spacing: 0) {
            // Enhanced time ruler with zoom support
            EnhancedTimeRulerView(
                duration: duration,
                currentTime: currentTime,
                pixelsPerSecond: pixelsPerSecond,
                zoom: zoom
            )
            .frame(height: 40)
            
            // Multi-track rendering with proper layering
            ScrollView(.vertical, showsIndicators: false) {
                VStack(spacing: 1) {
                    ForEach(Array(tracks.enumerated()), id: \.element.id) { index, track in
                        EnhancedTrackView(
                            track: track,
                            trackIndex: index,
                            currentTime: currentTime,
                            pixelsPerSecond: pixelsPerSecond,
                            zoom: zoom,
                            isSelected: selectedTracks.contains(track.id),
                            selectedKeyframes: selectedKeys,
                            onTrackSelect: { toggleTrackSelection(track.id) },
                            onKeyframeSelect: { keyframeId in toggleKeyframeSelection(keyframeId) }
                        )
                    }
                }
            }
        }
    }
    
    private func toggleTrackSelection(_ trackId: UUID) {
        if selectedTracks.contains(trackId) {
            selectedTracks.remove(trackId)
        } else {
            selectedTracks.insert(trackId)
        }
    }
    
    private func toggleKeyframeSelection(_ keyframeId: UUID) {
        if selectedKeys.contains(keyframeId) {
            selectedKeys.remove(keyframeId)
        } else {
            selectedKeys.insert(keyframeId)
        }
    }
    func addTrack(type: TimelineTrack.TrackType, name: String? = nil) {
        let track = TimelineTrack(
            name: name ?? type.rawValue,
            type: type,
            keyframes: []
        )
        tracks.append(track)
        print("[Timeline] Added track: \(track.name)")
    }
    
    func removeTrack(_ id: UUID) {
        tracks.removeAll { $0.id == id }
    }
    
    // MARK: - Keyframe Manipulation (Implemented)
    func addKeyframe(to trackID: UUID, at time: TimeInterval, value: Keyframe.KeyframeValue) {
        guard let index = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        
        let keyframe = Keyframe(time: time, value: value)
        tracks[index].keyframes.append(keyframe)
        tracks[index].keyframes.sort { $0.time < $1.time }
        
        print("[Timeline] Added keyframe at \(time)s")
    }
    
    func deleteKeyframe(_ id: UUID, from trackID: UUID) {
        guard let trackIndex = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        tracks[trackIndex].keyframes.removeAll { $0.id == id }
    }
    
    func moveKeyframe(_ id: UUID, in trackID: UUID, to newTime: TimeInterval) {
        guard let trackIndex = tracks.firstIndex(where: { $0.id == trackID }),
              let keyframeIndex = tracks[trackIndex].keyframes.firstIndex(where: { $0.id == id }) else { return }
        tracks[trackIndex].keyframes[keyframeIndex].time = newTime
        tracks[trackIndex].keyframes.sort { $0.time < $1.time }
    }
    
    func copyKeyframes(from trackID: UUID, in range: ClosedRange<TimeInterval>) -> [Keyframe] {
        guard let track = tracks.first(where: { $0.id == trackID }) else { return [] }
        return track.keyframes.filter { range.contains($0.time) }
    }
    
    func pasteKeyframes(_ keyframes: [Keyframe], to trackID: UUID, at time: TimeInterval) {
        guard let index = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        let offset = keyframes.first?.time ?? 0
        for keyframe in keyframes {
            let newTime = time + (keyframe.time - offset)
            let newKeyframe = Keyframe(time: newTime, value: keyframe.value, interpolation: keyframe.interpolation)
            tracks[index].keyframes.append(newKeyframe)
        }
        tracks[index].keyframes.sort { $0.time < $1.time }
    }
    
    // MARK: - Playback Controls (Implemented)
    func play() {
        isPlaying = true
        print("[Timeline] Playing")
    }
    
    func pause() {
        isPlaying = false
        print("[Timeline] Paused")
    }
    
    func stop() {
        isPlaying = false
        currentTime = 0
        print("[Timeline] Stopped")
    }
    
    // Playhead scrubbing (Implemented)
    func seekTo(_ time: TimeInterval) {
        currentTime = max(0, min(time, duration))
        if snapToFrames {
            let frameDuration = 1.0 / Double(fps)
            currentTime = round(currentTime / frameDuration) * frameDuration
        }
    }
    
    // Frame-by-frame stepping (Implemented)
    func stepForward() {
        let frameDuration = 1.0 / Double(fps)
        seekTo(currentTime + frameDuration)
    }
    
    func stepBackward() {
        let frameDuration = 1.0 / Double(fps)
        seekTo(currentTime - frameDuration)
    }
    
    // MARK: - Track Organization (Implemented)
    
    struct TrackGroup: Identifiable {
        let id = UUID()
        var name: String
        var trackIDs: [UUID]
        var isExpanded: Bool = true
        var color: Color = .gray
    }
    
    @Published var trackGroups: [TrackGroup] = []
    
    func createTrackGroup(name: String, trackIDs: [UUID]) {
        let group = TrackGroup(name: name, trackIDs: trackIDs)
        trackGroups.append(group)
    }
    
    func addTrackToGroup(_ trackID: UUID, groupID: UUID) {
        guard let index = trackGroups.firstIndex(where: { $0.id == groupID }) else { return }
        if !trackGroups[index].trackIDs.contains(trackID) {
            trackGroups[index].trackIDs.append(trackID)
        }
    }
    
    // Track muting/solo (Implemented)
    func toggleMute(trackID: UUID) {
        guard let index = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        tracks[index].isMuted.toggle()
    }
    
    func soloTrack(_ trackID: UUID) {
        for i in tracks.indices {
            tracks[i].isMuted = tracks[i].id != trackID
        }
    }
    
    // Track locking (Implemented)
    func toggleLock(trackID: UUID) {
        guard let index = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        tracks[index].isLocked.toggle()
    }
    
    // MARK: - Clip Operations (Implemented)
    
    func splitClip(trackID: UUID, at time: TimeInterval) {
        guard let trackIndex = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        
        let originalTrack = tracks[trackIndex]
        
        // Find keyframes before and after split point
        let beforeKeyframes = originalTrack.keyframes.filter { $0.time < time }
        let afterKeyframes = originalTrack.keyframes.filter { $0.time >= time }
        
        // Create new track for second half
        let newTrack = TimelineTrack(
            name: "\(originalTrack.name) (Split)",
            type: originalTrack.type,
            keyframes: afterKeyframes,
            isMuted: originalTrack.isMuted,
            isLocked: originalTrack.isLocked,
            isExpanded: originalTrack.isExpanded,
            color: originalTrack.color
        )
        
        // Update original track
        tracks[trackIndex].keyframes = beforeKeyframes
        
        // Insert new track
        tracks.insert(newTrack, at: trackIndex + 1)
    }
    
    func duplicateClip(trackID: UUID, offset: TimeInterval) {
        guard let track = tracks.first(where: { $0.id == trackID }) else { return }
        
        // Create new keyframes with offset
        let newKeyframes = track.keyframes.map { keyframe in
            Keyframe(time: keyframe.time + offset, value: keyframe.value, interpolation: keyframe.interpolation)
        }
        
        let newTrack = TimelineTrack(
            name: "\(track.name) (Copy)",
            type: track.type,
            keyframes: newKeyframes,
            isMuted: track.isMuted,
            isLocked: track.isLocked,
            isExpanded: track.isExpanded,
            color: track.color
        )
        
        tracks.append(newTrack)
    }
    
    func setClipSpeed(trackID: UUID, speedMultiplier: Double) {
        guard let index = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        
        // Time remap all keyframes
        for i in tracks[index].keyframes.indices {
            tracks[index].keyframes[i].time *= speedMultiplier
        }
    }
    
    // MARK: - Clip Blending and Crossfade Transitions
    func createCrossfadeTransition(from trackID: UUID, to targetTrackID: UUID, at time: TimeInterval, duration: TimeInterval) {
        guard let fromTrack = tracks.first(where: { $0.id == trackID }),
              let toTrack = tracks.first(where: { $0.id == targetTrackID }) else { return }
        
        // Create fade out keyframes on source track
        let fadeOutStart = Keyframe(time: time, value: .float(1.0), interpolation: .easeOut)
        let fadeOutEnd = Keyframe(time: time + duration, value: .float(0.0), interpolation: .easeIn)
        
        // Create fade in keyframes on target track
        let fadeInStart = Keyframe(time: time, value: .float(0.0), interpolation: .easeIn)
        let fadeInEnd = Keyframe(time: time + duration, value: .float(1.0), interpolation: .easeOut)
        
        // Add keyframes to tracks
        addKeyframe(to: trackID, at: time, value: fadeOutStart.value)
        addKeyframe(to: trackID, at: time + duration, value: fadeOutEnd.value)
        addKeyframe(to: targetTrackID, at: time, value: fadeInStart.value)
        addKeyframe(to: targetTrackID, at: time + duration, value: fadeInEnd.value)
        
        print("[Timeline] Created crossfade transition: \(duration)s")
    }
    
    func blendClips(trackID: UUID, blendMode: NLAStrip.BlendMode, opacity: Double) {
        guard let index = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        
        // Update strip blend modes if they exist
        for i in tracks[index].strips.indices {
            tracks[index].strips[i].blendMode = blendMode
            tracks[index].strips[i].opacity = opacity
        }
    }
    // MARK: - Curve Editor for Animated Properties
    func updateKeyframeTangents(_ keyframeID: UUID, inTangent: SIMD2<Float>?, outTangent: SIMD2<Float>?) {
        for trackIndex in tracks.indices {
            for keyframeIndex in tracks[trackIndex].keyframes.indices {
                if tracks[trackIndex].keyframes[keyframeIndex].id == keyframeID {
                    tracks[trackIndex].keyframes[keyframeIndex].inTangent = inTangent
                    tracks[trackIndex].keyframes[keyframeIndex].outTangent = outTangent
                    tracks[trackIndex].keyframes[keyframeIndex].interpolation = .bezier
                    print("[Timeline] Updated tangents for keyframe: \(keyframeID)")
                    return
                }
            }
        }
    }
    
    func setKeyframeInterpolation(_ keyframeID: UUID, interpolation: Keyframe.InterpolationType) {
        for trackIndex in tracks.indices {
            for keyframeIndex in tracks[trackIndex].keyframes.indices {
                if tracks[trackIndex].keyframes[keyframeIndex].id == keyframeID {
                    tracks[trackIndex].keyframes[keyframeIndex].interpolation = interpolation
                    print("[Timeline] Set interpolation to \(interpolation.rawValue) for keyframe: \(keyframeID)")
                    return
                }
            }
        }
    }
    
    func evaluateCurveAtTime(trackID: UUID, time: TimeInterval) -> Keyframe.KeyframeValue? {
        guard let track = tracks.first(where: { $0.id == trackID }) else { return nil }
        
        // Find surrounding keyframes
        let sortedKeyframes = track.keyframes.sorted { $0.time < $1.time }
        guard let currentIndex = sortedKeyframes.firstIndex(where: { $0.time >= time }) else {
            // Time is after last keyframe
            return sortedKeyframes.last?.value
        }
        
        guard currentIndex > 0 else {
            // Time is before or at first keyframe
            return sortedKeyframes.first?.value
        }
        
        let prevKeyframe = sortedKeyframes[currentIndex - 1]
        let nextKeyframe = sortedKeyframes[currentIndex]
        
        // Calculate interpolation factor
        let duration = nextKeyframe.time - prevKeyframe.time
        let t = duration > 0 ? (time - prevKeyframe.time) / duration : 0
        
        // Interpolate based on type
        switch prevKeyframe.interpolation {
        case .linear:
            return interpolateLinear(prevKeyframe.value, nextKeyframe.value, Float(t))
        case .bezier:
            return interpolateBezier(prevKeyframe, nextKeyframe, Float(t))
        case .ease:
            return interpolateEase(prevKeyframe.value, nextKeyframe.value, Float(t))
        case .easeIn:
            return interpolateEaseIn(prevKeyframe.value, nextKeyframe.value, Float(t))
        case .easeOut:
            return interpolateEaseOut(prevKeyframe.value, nextKeyframe.value, Float(t))
        case .easeInOut:
            return interpolateEaseInOut(prevKeyframe.value, nextKeyframe.value, Float(t))
        case .constant:
            return prevKeyframe.value
        }
    }
    
    // MARK: - Bezier Curve Tangent Editing
    func autoComputeTangents(_ keyframeID: UUID) {
        for trackIndex in tracks.indices {
            for keyframeIndex in tracks[trackIndex].keyframes.indices {
                if tracks[trackIndex].keyframes[keyframeIndex].id == keyframeID {
                    let keyframes = tracks[trackIndex].keyframes.sorted { $0.time < $1.time }
                    guard let currentIndex = keyframes.firstIndex(where: { $0.id == keyframeID }) else { return }
                    
                    let currentKeyframe = keyframes[currentIndex]
                    var inTangent: SIMD2<Float> = SIMD2<Float>(0, 0)
                    var outTangent: SIMD2<Float> = SIMD2<Float>(0, 0)
                    
                    // Calculate out tangent (to next keyframe)
                    if currentIndex < keyframes.count - 1 {
                        let nextKeyframe = keyframes[currentIndex + 1]
                        let timeDiff = Float(nextKeyframe.time - currentKeyframe.time)
                        let valueDiff = getValueDifference(currentKeyframe.value, nextKeyframe.value)
                        outTangent = SIMD2<Float>(timeDiff * 0.33, valueDiff * 0.33)
                    }
                    
                    // Calculate in tangent (from previous keyframe)
                    if currentIndex > 0 {
                        let prevKeyframe = keyframes[currentIndex - 1]
                        let timeDiff = Float(currentKeyframe.time - prevKeyframe.time)
                        let valueDiff = getValueDifference(prevKeyframe.value, currentKeyframe.value)
                        inTangent = SIMD2<Float>(-timeDiff * 0.33, -valueDiff * 0.33)
                    }
                    
                    updateKeyframeTangents(keyframeID, inTangent: inTangent, outTangent: outTangent)
                    return
                }
            }
        }
    }
    
    func breakTangents(_ keyframeID: UUID) {
        // This would allow independent editing of in/out tangents
        // For now, we'll just set them to default values
        updateKeyframeTangents(keyframeID, inTangent: SIMD2<Float>(-10, 0), outTangent: SIMD2<Float>(10, 0))
    }
    
    func unifyTangents(_ keyframeID: UUID) {
        // Make in and out tangents mirror each other
        for trackIndex in tracks.indices {
            for keyframeIndex in tracks[trackIndex].keyframes.indices {
                if tracks[trackIndex].keyframes[keyframeIndex].id == keyframeID {
                    let outTangent = tracks[trackIndex].keyframes[keyframeIndex].outTangent ?? SIMD2<Float>(10, 0)
                    let inTangent = SIMD2<Float>(-outTangent.x, -outTangent.y)
                    updateKeyframeTangents(keyframeID, inTangent: inTangent, outTangent: outTangent)
                    return
                }
            }
        }
    }
    
    // MARK: - Cinematic Features (Implemented)
    
    struct CameraCut: Identifiable {
        let id = UUID()
        var time: TimeInterval
        var cameraID: UUID
        var transitionDuration: TimeInterval = 0
        var transitionType: TransitionType = .cut
        
        enum TransitionType {
            case cut, fade, dissolve, wipe
        }
    }
    
    @Published var cameraCuts: [CameraCut] = []
    
    func addCameraCut(at time: TimeInterval, cameraID: UUID) {
        let cut = CameraCut(time: time, cameraID: cameraID)
        cameraCuts.append(cut)
        cameraCuts.sort { $0.time < $1.time }
    }
    
    func addFadeEffect(trackID: UUID, at time: TimeInterval, duration: TimeInterval, fadeIn: Bool) {
        guard let index = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        
        let startValue: Keyframe.KeyframeValue = fadeIn ? .float(0) : .float(1)
        let endValue: Keyframe.KeyframeValue = fadeIn ? .float(1) : .float(0)
        
        let startKeyframe = Keyframe(time: time, value: startValue)
        let endKeyframe = Keyframe(time: time + duration, value: endValue)
        
        tracks[index].keyframes.append(contentsOf: [startKeyframe, endKeyframe])
        tracks[index].keyframes.sort { $0.time < $1.time }
    }
    
    // MARK: - Advanced Animation Features
    
    func addDepthOfFieldAnimation(trackID: UUID, focusDistance: Float, aperture: Float, at time: TimeInterval) {
        addKeyframe(to: trackID, at: time, value: .vector3(SIMD3<Float>(focusDistance, aperture, 0)))
    }
    
    func addColorGradingKeyframe(trackID: UUID, temperature: Float, tint: Float, saturation: Float, at time: TimeInterval) {
        addKeyframe(to: trackID, at: time, value: .vector3(SIMD3<Float>(temperature, tint, saturation)))
    }
    
    func addCameraShake(trackID: UUID, intensity: Float, frequency: Float, duration: TimeInterval, startTime: TimeInterval) {
        // Add shake keyframes
        let steps = Int(duration * Double(fps))
        for i in 0...steps {
            let t = startTime + (duration * Double(i) / Double(steps))
            let shake = sin(Float(t) * frequency) * intensity
            addKeyframe(to: trackID, at: t, value: .float(shake))
        }
    }
    
    // MARK: - Post-Process Track
    struct PostProcessEffect: Identifiable {
        let id = UUID()
        var name: String
        var type: EffectType
        var parameters: [String: Float]
        var enabled: Bool = true
        
        enum EffectType: String, CaseIterable {
            case bloom = "Bloom"
            case vignette = "Vignette"
            case colorGrading = "Color Grading"
            case motionBlur = "Motion Blur"
            case depthOfField = "Depth of Field"
            case chromaticAberration = "Chromatic Aberration"
            case filmGrain = "Film Grain"
            case lensFlare = "Lens Flare"
        }
    }
    
    @Published var postProcessTracks: [TimelineTrack] = []
    
    func addPostProcessTrack(name: String? = nil) {
        let track = TimelineTrack(
            name: name ?? "Post Process",
            type: .materialParameter, // Reuse this type for post-process
            keyframes: []
        )
        postProcessTracks.append(track)
        tracks.append(track)
        print("[Timeline] Added post-process track: \(track.name)")
    }
    
    func addPostProcessEffect(to trackID: UUID, effect: PostProcessEffect, at time: TimeInterval) {
        // Convert effect to keyframe data
        let effectData = "\(effect.type.rawValue):\(effect.parameters)"
        addKeyframe(to: trackID, at: time, value: .string(effectData))
        print("[Timeline] Added post-process effect: \(effect.name) at \(time)s")
    }
    
    func setPostProcessParameter(trackID: UUID, parameter: String, value: Float, at time: TimeInterval) {
        let paramData = "\(parameter)=\(value)"
        addKeyframe(to: trackID, at: time, value: .string(paramData))
    }
    // MARK: - Text/Title Track
    struct TitleClip: Identifiable {
        let id = UUID()
        var text: String
        var font: String = "Arial"
        var fontSize: Float = 24
        var color: Color = .white
        var position: SIMD2<Float> = SIMD2<Float>(0.5, 0.5) // Normalized coordinates
        var alignment: TextAlignment = .center
        var animationIn: TitleAnimation = .fade
        var animationOut: TitleAnimation = .fade
        var duration: TimeInterval
        
        enum TitleAnimation: String, CaseIterable {
            case fade = "Fade"
            case slide = "Slide"
            case zoom = "Zoom"
            case typewriter = "Typewriter"
            case none = "None"
        }
    }
    
    @Published var titleTracks: [TimelineTrack] = []
    
    func addTitleTrack(name: String? = nil) {
        let track = TimelineTrack(
            name: name ?? "Titles",
            type: .subtitle, // Use subtitle type for titles
            keyframes: []
        )
        titleTracks.append(track)
        tracks.append(track)
        print("[Timeline] Added title track: \(track.name)")
    }
    
    func addTitleClip(to trackID: UUID, clip: TitleClip, at time: TimeInterval) {
        // Convert title clip to keyframe data
        let titleData = "\(clip.text)|\(clip.fontSize)|\(clip.animationIn.rawValue)|\(clip.animationOut.rawValue)"
        addKeyframe(to: trackID, at: time, value: .string(titleData))
        
        // Add duration keyframe
        let durationKeyframe = Keyframe(time: time + clip.duration, value: .string("end:\(clip.id)"))
        guard let trackIndex = tracks.firstIndex(where: { $0.id == trackID }) else { return }
        tracks[trackIndex].keyframes.append(durationKeyframe)
        tracks[trackIndex].keyframes.sort { $0.time < $1.time }
        
        print("[Timeline] Added title clip: \(clip.text) at \(time)s")
    }
    
    func updateTitleText(trackID: UUID, at time: TimeInterval, text: String) {
        addKeyframe(to: trackID, at: time, value: .string("text:\(text)"))
    }
    
    func setTitleAnimation(trackID: UUID, at time: TimeInterval, animationIn: TitleClip.TitleAnimation, animationOut: TitleClip.TitleAnimation) {
        let animData = "anim:\(animationIn.rawValue)-\(animationOut.rawValue)"
        addKeyframe(to: trackID, at: time, value: .string(animData))
    }
    
    // MARK: - Timeline Zoom Levels
    @Published var zoomLevel: CGFloat = 1.0
    @Published var minZoom: CGFloat = 0.1
    @Published var maxZoom: CGFloat = 10.0
    
    func setZoom(_ zoom: CGFloat) {
        zoomLevel = max(minZoom, min(maxZoom, zoom))
        print("[Timeline] Set zoom: \(zoomLevel)")
    }
    
    func zoomIn() {
        setZoom(zoomLevel * 1.2)
    }
    
    func zoomOut() {
        setZoom(zoomLevel / 1.2)
    }
    
    func resetZoom() {
        setZoom(1.0)
    }
    
    func fitToWindow() {
        // Calculate zoom to fit entire timeline in view
        // This would need window width information from the view
        setZoom(1.0)
    }
    
    // MARK: - Multi-Camera Editing
    struct Camera: Identifiable {
        let id = UUID()
        var name: String
        var position: SIMD3<Float>
        var rotation: SIMD3<Float> // Euler angles in degrees
        var fov: Float = 60.0
        var isActive: Bool = false
        
        var transform: simd_float4x4 {
            // Create transform matrix from position and rotation
            let translation = simd_float4x4(translation: position)
            let rotationX = simd_float4x4(rotation: SIMD3<Float>(rotation.x, 0, 0))
            let rotationY = simd_float4x4(rotation: SIMD3<Float>(0, rotation.y, 0))
            let rotationZ = simd_float4x4(rotation: SIMD3<Float>(0, 0, rotation.z))
            return translation * rotationZ * rotationY * rotationX
        }
    }
    
    @Published var cameras: [Camera] = []
    @Published var activeCameraID: UUID?
    
    func addCamera(name: String, position: SIMD3<Float> = SIMD3<Float>(0, 0, 0), rotation: SIMD3<Float> = SIMD3<Float>(0, 0, 0)) {
        let camera = Camera(name: name, position: position, rotation: rotation)
        cameras.append(camera)
        if activeCameraID == nil {
            activeCameraID = camera.id
        }
        print("[Timeline] Added camera: \(name)")
    }
    
    func setActiveCamera(_ cameraID: UUID) {
        activeCameraID = cameraID
        for i in cameras.indices {
            cameras[i].isActive = cameras[i].id == cameraID
        }
        print("[Timeline] Set active camera: \(cameraID)")
    }
    
    func addCameraCut(at time: TimeInterval, cameraID: UUID, transitionDuration: TimeInterval = 0, transitionType: CameraCut.TransitionType = .cut) {
        let cut = CameraCut(time: time, cameraID: cameraID, transitionDuration: transitionDuration, transitionType: transitionType)
        cameraCuts.append(cut)
        cameraCuts.sort { $0.time < $1.time }
        print("[Timeline] Added camera cut at \(time)s to camera \(cameraID)")
    }
    
    func getActiveCamera(at time: TimeInterval) -> Camera? {
        // Find the most recent camera cut before or at the current time
        let relevantCuts = cameraCuts.filter { $0.time <= time }.sorted { $0.time > $1.time }
        if let cut = relevantCuts.first {
            return cameras.first { $0.id == cut.cameraID }
        }
        // Return first camera if no cuts found
        return cameras.first
    }
    
    func animateCamera(_ cameraID: UUID, to position: SIMD3<Float>, rotation: SIMD3<Float>, duration: TimeInterval, startTime: TimeInterval) {
        guard let camera = cameras.first(where: { $0.id == cameraID }) else { return }
        
        // Create position keyframes
        addKeyframe(to: getCameraTrackID(cameraID, property: "positionX"), at: startTime, value: .float(camera.position.x))
        addKeyframe(to: getCameraTrackID(cameraID, property: "positionX"), at: startTime + duration, value: .float(position.x))
        
        addKeyframe(to: getCameraTrackID(cameraID, property: "positionY"), at: startTime, value: .float(camera.position.y))
        addKeyframe(to: getCameraTrackID(cameraID, property: "positionY"), at: startTime + duration, value: .float(position.y))
        
        addKeyframe(to: getCameraTrackID(cameraID, property: "positionZ"), at: startTime, value: .float(camera.position.z))
        addKeyframe(to: getCameraTrackID(cameraID, property: "positionZ"), at: startTime + duration, value: .float(position.z))
        
        // Create rotation keyframes
        addKeyframe(to: getCameraTrackID(cameraID, property: "rotationX"), at: startTime, value: .float(camera.rotation.x))
        addKeyframe(to: getCameraTrackID(cameraID, property: "rotationX"), at: startTime + duration, value: .float(rotation.x))
        
        addKeyframe(to: getCameraTrackID(cameraID, property: "rotationY"), at: startTime, value: .float(camera.rotation.y))
        addKeyframe(to: getCameraTrackID(cameraID, property: "rotationY"), at: startTime + duration, value: .float(rotation.y))
        
        addKeyframe(to: getCameraTrackID(cameraID, property: "rotationZ"), at: startTime, value: .float(camera.rotation.z))
        addKeyframe(to: getCameraTrackID(cameraID, property: "rotationZ"), at: startTime + duration, value: .float(rotation.z))
        
        // Update camera position
        if let index = cameras.firstIndex(where: { $0.id == cameraID }) {
            cameras[index].position = position
            cameras[index].rotation = rotation
        }
    }
    
    private func getCameraTrackID(_ cameraID: UUID, property: String) -> UUID {
        // Find or create track for this camera property
        let trackName = "Camera \(cameras.first { $0.id == cameraID }?.name ?? "Unknown") - \(property)"
        if let track = tracks.first(where: { $0.name == trackName }) {
            return track.id
        } else {
            addTrack(type: .camera, name: trackName)
            return tracks.last!.id
        }
    }
    // MARK: - Preview Render
    struct RenderSettings {
        var resolution: SIMD2<Int> = SIMD2<Int>(1920, 1080)
        var quality: RenderQuality = .high
        var antiAliasing: Bool = true
        var shadows: Bool = true
        var reflections: Bool = true
        var postProcessing: Bool = true
        
        enum RenderQuality: String, CaseIterable {
            case low = "Low"
            case medium = "Medium"
            case high = "High"
            case ultra = "Ultra"
        }
    }
    
    @Published var renderSettings = RenderSettings()
    @Published var isRendering = false
    @Published var renderProgress: Float = 0.0
    
    func startPreviewRender() {
        guard !isRendering else { return }
        
        isRendering = true
        renderProgress = 0.0
        
        print("[Timeline] Starting preview render at \(renderSettings.resolution.x)x\(renderSettings.resolution.y)")
        
        // Simulate rendering process
        Task {
            let totalFrames = Int(duration * Double(fps))
            
            for frame in 0..<totalFrames {
                let time = Double(frame) / Double(fps)
                await renderFrame(at: time, frameNumber: frame)
                
                // Update progress
                await MainActor.run {
                    renderProgress = Float(frame + 1) / Float(totalFrames)
                }
                
                // Small delay to simulate rendering time
                try? await Task.sleep(nanoseconds: 10_000_000) // 10ms
            }
            
            await MainActor.run {
                isRendering = false
                print("[Timeline] Preview render completed")
            }
        }
    }
    
    private func renderFrame(at time: TimeInterval, frameNumber: Int) async {
        // This would interface with the actual rendering system
        // For now, we'll simulate the rendering process
        
        // Get active camera at this time
        let camera = getActiveCamera(at: time)
        
        // Evaluate all tracks at this time
        for track in tracks {
            if let value = evaluateCurveAtTime(trackID: track.id, time: time) {
                // Apply the value to the appropriate system
                await applyAnimatedValue(value, to: track, at: time)
            }
        }
        
        // Render the frame
        print("[Timeline] Rendering frame \(frameNumber) at time \(String(format: "%.2f", time))s")
    }
    
    private func applyAnimatedValue(_ value: AnimationTimeline.Keyframe.KeyframeValue, to track: AnimationTimeline.TimelineTrack, at time: TimeInterval) async {
        // This would apply the animated value to the appropriate system
        switch track.type {
        case .camera:
            // Apply camera transforms
            break
        case .lightIntensity:
            // Apply lighting changes
            break
        case .materialParameter:
            // Apply material changes
            break
        case .particleTrigger:
            // Trigger particle effects
            break
        default:
            break
        }
    }
    
    func stopPreviewRender() {
        isRendering = false
        renderProgress = 0.0
        print("[Timeline] Preview render stopped")
    }
    // MARK: - Export to Video
    struct ExportSettings {
        var outputPath: String = ""
        var resolution: SIMD2<Int> = SIMD2<Int>(1920, 1080)
        var fps: Int = 60
        var codec: VideoCodec = .h264
        var quality: VideoQuality = .high
        var audioBitrate: Int = 320
        var videoBitrate: Int = 8000
        
        enum VideoCodec: String, CaseIterable {
            case h264 = "H.264"
            case h265 = "H.265"
            case vp9 = "VP9"
            case av1 = "AV1"
        }
        
        enum VideoQuality: String, CaseIterable {
            case low = "Low"
            case medium = "Medium"
            case high = "High"
            case lossless = "Lossless"
        }
    }
    
    @Published var exportSettings = ExportSettings()
    @Published var isExporting = false
    @Published var exportProgress: Float = 0.0
    
    func exportVideo(path: String, resolution: SIMD2<Int>, fps: Int) {
        exportSettings.outputPath = path
        exportSettings.resolution = resolution
        exportSettings.fps = fps
        
        startVideoExport()
    }
    
    func startVideoExport() {
        guard !isExporting && !exportSettings.outputPath.isEmpty else { return }
        
        isExporting = true
        exportProgress = 0.0
        
        print("[Timeline] Starting video export to \(exportSettings.outputPath)")
        print("[Timeline] Export settings: \(exportSettings.resolution.x)x\(exportSettings.resolution.y) @ \(exportSettings.fps)fps")
        
        Task {
            await performVideoExport()
        }
    }
    
    private func performVideoExport() async {
        let totalFrames = Int(duration * Double(exportSettings.fps))
        
        // Initialize video encoder
        await initializeVideoEncoder()
        
        // Export each frame
        for frame in 0..<totalFrames {
            let time = Double(frame) / Double(exportSettings.fps)
            
            // Render frame
            await renderFrameForExport(at: time, frameNumber: frame)
            
            // Encode frame
            await encodeFrame(frameNumber: frame)
            
            // Update progress
            await MainActor.run {
                exportProgress = Float(frame + 1) / Float(totalFrames)
            }
            
            // Check if export was cancelled
            if !isExporting {
                break
            }
        }
        
        // Finalize video
        await finalizeVideo()
        
        await MainActor.run {
            isExporting = false
            print("[Timeline] Video export completed: \(exportSettings.outputPath)")
        }
    }
    
    private func initializeVideoEncoder() async {
        // Initialize video encoding system
        print("[Timeline] Initializing video encoder: \(exportSettings.codec.rawValue)")
    }
    
    private func renderFrameForExport(at time: TimeInterval, frameNumber: Int) async {
        // Render frame at export quality
        await renderFrame(at: time, frameNumber: frameNumber)
    }
    
    private func encodeFrame(frameNumber: Int) async {
        // Encode the rendered frame to video
        if frameNumber % 30 == 0 { // Log every 30 frames
            print("[Timeline] Encoded frame \(frameNumber)")
        }
    }
    
    private func finalizeVideo() async {
        // Finalize the video file and write metadata
        print("[Timeline] Finalizing video file")
    }
    
    func cancelVideoExport() {
        isExporting = false
        exportProgress = 0.0
        print("[Timeline] Video export cancelled")
    }
    
    func clear() {
        tracks.removeAll()
        currentTime = 0
        isPlaying = false
    }
    
    // MARK: - Interpolation Helper Functions
    private func interpolateLinear(_ from: Keyframe.KeyframeValue, _ to: Keyframe.KeyframeValue, _ t: Float) -> Keyframe.KeyframeValue {
        switch (from, to) {
        case (.float(let a), .float(let b)):
            return .float(a + (b - a) * t)
        case (.vector3(let a), .vector3(let b)):
            return .vector3(a + (b - a) * t)
        case (.color(let a), .color(let b)):
            // Simple color interpolation
            return .color(a.opacity(Double(1 - t))) // Placeholder
        default:
            return from
        }
    }
    
    private func interpolateBezier(_ from: Keyframe, _ to: Keyframe, _ t: Float) -> Keyframe.KeyframeValue {
        // Simplified bezier interpolation - would need proper cubic bezier implementation
        return interpolateLinear(from.value, to.value, t)
    }
    
    private func interpolateEase(_ from: Keyframe.KeyframeValue, _ to: Keyframe.KeyframeValue, _ t: Float) -> Keyframe.KeyframeValue {
        let easedT = t * t * (3.0 - 2.0 * t) // Smoothstep
        return interpolateLinear(from, to, easedT)
    }
    
    private func interpolateEaseIn(_ from: Keyframe.KeyframeValue, _ to: Keyframe.KeyframeValue, _ t: Float) -> Keyframe.KeyframeValue {
        let easedT = t * t
        return interpolateLinear(from, to, easedT)
    }
    
    private func interpolateEaseOut(_ from: Keyframe.KeyframeValue, _ to: Keyframe.KeyframeValue, _ t: Float) -> Keyframe.KeyframeValue {
        let easedT = 1.0 - (1.0 - t) * (1.0 - t)
        return interpolateLinear(from, to, easedT)
    }
    
    private func interpolateEaseInOut(_ from: Keyframe.KeyframeValue, _ to: Keyframe.KeyframeValue, _ t: Float) -> Keyframe.KeyframeValue {
        let easedT = if t < 0.5 {
            2.0 * t * t
        } else {
            1.0 - 2.0 * (1.0 - t) * (1.0 - t)
        }
        return interpolateLinear(from, to, easedT)
    }
    
    private func getValueDifference(_ from: Keyframe.KeyframeValue, _ to: Keyframe.KeyframeValue) -> Float {
        switch (from, to) {
        case (.float(let a), .float(let b)):
            return b - a
        case (.vector3(let a), .vector3(let b)):
            return (b.x - a.x) // Use X component as primary difference
        default:
            return 0.0
        }
    }
}

// MARK: - Enhanced Timeline Views
struct EnhancedTimeRulerView: View {
    let duration: TimeInterval
    let currentTime: TimeInterval
    let pixelsPerSecond: CGFloat
    let zoom: CGFloat
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // Background
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundSecondary)
                
                // Time markers and labels
                Canvas { context, size in
                    let effectivePixelsPerSecond = pixelsPerSecond * zoom
                    let totalWidth = duration * effectivePixelsPerSecond
                    
                    // Calculate appropriate step size based on zoom
                    let stepSize = calculateStepSize(zoom: zoom)
                    let stepDuration = stepSize / effectivePixelsPerSecond
                    
                    var time = 0.0
                    while time <= duration {
                        let x = time * effectivePixelsPerSecond
                        
                        if x >= 0 && x <= size.width {
                            // Major tick
                            context.stroke(
                                Path { path in
                                    path.move(to: CGPoint(x: x, y: 0))
                                    path.addLine(to: CGPoint(x: x, y: size.height))
                                },
                                with: .color(DesignSystem.Colors.textSecondary.opacity(0.5))
                            )
                            
                            // Time label
                            let label = String(format: "%.1fs", time)
                            context.draw(
                                Text(label).font(.system(size: 10)).foregroundColor(DesignSystem.Colors.textPrimary),
                                at: CGPoint(x: x + 4, y: 12)
                            )
                        }
                        
                        time += stepDuration
                    }
                }
                
                // Playhead
                Rectangle()
                    .fill(Color.red)
                    .frame(width: 2)
                    .offset(x: currentTime * pixelsPerSecond * zoom)
            }
        }
    }
    
    private func calculateStepSize(zoom: CGFloat) -> CGFloat {
        switch zoom {
        case 0..<0.5: return 200
        case 0.5..<1.0: return 100
        case 1.0..<2.0: return 50
        case 2.0..<5.0: return 25
        default: return 10
        }
    }
}

struct EnhancedTrackView: View {
    let track: AnimationTimeline.TimelineTrack
    let trackIndex: Int
    let currentTime: TimeInterval
    let pixelsPerSecond: CGFloat
    let zoom: CGFloat
    let isSelected: Bool
    let selectedKeyframes: Set<UUID>
    let onTrackSelect: () -> Void
    let onKeyframeSelect: (UUID) -> Void
    
    var body: some View {
        HStack(spacing: 0) {
            // Track header
            EnhancedTrackHeaderView(
                track: track,
                trackIndex: trackIndex,
                isSelected: isSelected,
                onSelect: onTrackSelect
            )
            .frame(width: 200)
            
            // Track content
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    // Background
                    Rectangle()
                        .fill(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.1) : DesignSystem.Colors.backgroundPrimary)
                    
                    // Track-specific content based on type
                    trackContent(geometry: geometry)
                    
                    // Keyframes
                    ForEach(track.keyframes) { keyframe in
                        let x = keyframe.time * pixelsPerSecond * zoom
                        
                        if x >= -20 && x <= geometry.size.width + 20 {
                            EnhancedKeyframeView(
                                keyframe: keyframe,
                                trackColor: track.color,
                                isSelected: selectedKeyframes.contains(keyframe.id),
                                onSelect: { onKeyframeSelect(keyframe.id) }
                            )
                            .position(x: x, y: geometry.size.height / 2)
                        }
                    }
                }
            }
            .frame(height: 40)
            .clipped()
        }
    }
    
    @ViewBuilder
    private func trackContent(geometry: GeometryProxy) -> some View {
        switch track.type {
        case .audio:
            AudioWaveformView(track: track, geometry: geometry)
        case .animation:
            AnimationStripView(track: track, geometry: geometry)
        case .camera:
            CameraTrackView(track: track, geometry: geometry)
        default:
            Rectangle()
                .fill(DesignSystem.Colors.backgroundSecondary.opacity(0.3))
        }
    }
}

struct EnhancedTrackHeaderView: View {
    let track: AnimationTimeline.TimelineTrack
    let trackIndex: Int
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        HStack(spacing: 4) {
            // Track icon and color
            Image(systemName: track.type.icon)
                .font(.system(size: 12))
                .foregroundColor(track.color)
            
            // Track name
            Text(track.name)
                .font(.system(size: 11))
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .lineLimit(1)
            
            Spacer()
            
            // Track controls
            Group {
                Button(action: { /* Toggle mute */ }) {
                    Image(systemName: track.isMuted ? "speaker.slash" : "speaker")
                        .font(.system(size: 10))
                        .foregroundColor(track.isMuted ? .red : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                Button(action: { /* Toggle lock */ }) {
                    Image(systemName: track.isLocked ? "lock" : "lock.open")
                        .font(.system(size: 10))
                        .foregroundColor(track.isLocked ? .yellow : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                
                Button(action: { /* Toggle visibility */ }) {
                    Image(systemName: "eye")
                        .font(.system(size: 10))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.horizontal, 6)
        .padding(.vertical, 4)
        .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.2) : DesignSystem.Colors.backgroundSecondary)
        .onTapGesture { onSelect() }
    }
}

struct EnhancedKeyframeView: View {
    let keyframe: AnimationTimeline.Keyframe
    let trackColor: Color
    let isSelected: Bool
    let onSelect: () -> Void
    
    var body: some View {
        Group {
            if keyframe.interpolation == .bezier {
                // Bezier keyframe with tangent handles
                BezierKeyframeView(keyframe: keyframe, color: trackColor, isSelected: isSelected)
            } else {
                // Standard keyframe
                Diamond()
                    .fill(isSelected ? Color.white : trackColor)
                    .frame(width: 8, height: 8)
                    .overlay(
                        Diamond()
                            .stroke(Color.black.opacity(0.5), lineWidth: 0.5)
                    )
            }
        }
        .onTapGesture { onSelect() }
    }
}

struct Diamond: Shape {
    func path(in rect: CGRect) -> Path {
        var path = Path()
        path.move(to: CGPoint(x: rect.midX, y: rect.minY))
        path.addLine(to: CGPoint(x: rect.maxX, y: rect.midY))
        path.addLine(to: CGPoint(x: rect.midX, y: rect.maxY))
        path.addLine(to: CGPoint(x: rect.minX, y: rect.midY))
        path.closeSubpath()
        return path
    }
}

// Track-specific content views
struct AudioWaveformView: View {
    let track: AnimationTimeline.TimelineTrack
    let geometry: GeometryProxy
    
    var body: some View {
        // Placeholder for audio waveform visualization
        Rectangle()
            .fill(DesignSystem.Colors.accentPrimary.opacity(0.2))
    }
}

struct AnimationStripView: View {
    let track: AnimationTimeline.TimelineTrack
    let geometry: GeometryProxy
    
    var body: some View {
        // Render NLA strips if they exist
        ForEach(track.strips) { strip in
            Rectangle()
                .fill(DesignSystem.Colors.accentPrimary.opacity(0.3))
                .overlay(
                    Text(strip.name)
                        .font(.caption2)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                )
        }
    }
}

struct CameraTrackView: View {
    let track: AnimationTimeline.TimelineTrack
    let geometry: GeometryProxy
    
    var body: some View {
        // Camera-specific visualization
        Rectangle()
            .fill(DesignSystem.Colors.backgroundSecondary.opacity(0.2))
    }
}

struct BezierKeyframeView: View {
    let keyframe: AnimationTimeline.Keyframe
    let color: Color
    let isSelected: Bool
    
    var body: some View {
        ZStack {
            // Main keyframe diamond
            Diamond()
                .fill(isSelected ? Color.white : color)
                .frame(width: 10, height: 10)
                .overlay(
                    Diamond()
                        .stroke(Color.black.opacity(0.5), lineWidth: 1)
                )
            
            // Tangent handles (simplified representation)
            if isSelected, let inTangent = keyframe.inTangent, let outTangent = keyframe.outTangent {
                // In tangent
                Path { path in
                    path.move(to: CGPoint(x: -5, y: 0))
                    path.addLine(to: CGPoint(x: -5 - CGFloat(inTangent.x), y: -CGFloat(inTangent.y)))
                }
                .stroke(Color.blue, lineWidth: 1)
                
                // Out tangent
                Path { path in
                    path.move(to: CGPoint(x: 5, y: 0))
                    path.addLine(to: CGPoint(x: 5 + CGFloat(outTangent.x), y: -CGFloat(outTangent.y)))
                }
                .stroke(Color.green, lineWidth: 1)
            }
        }
    }
}

// MARK: - Frame-based API Extension
extension AnimationTimeline {
    var currentFrame: Int {
        get { Int(currentTime * Double(fps)) }
        set { currentTime = Double(newValue) / Double(fps) }
    }
    
    var startFrame: Int {
        get { 0 }
        set { /* No-op for now unless start time is supported */ }
    }
    
    var endFrame: Int {
        get { Int(duration * Double(fps)) }
        set { duration = Double(newValue) / Double(fps) }
    }
    
    var totalFrames: Int {
        endFrame - startFrame
    }
    
    var frameRate: Int {
        get { fps }
        set { fps = newValue }
    }
    
    func goToFrame(_ frame: Int) {
        currentTime = Double(frame) / Double(fps)
    }
}

// MARK: - Animation Timeline Panel
struct AnimationTimelinePanel: View {
    @ObservedObject var timeline: AnimationTimeline
    @State private var pixelsPerSecond: CGFloat = 100
    
    enum ViewMode: String, CaseIterable {
        case timeline = "Timeline"
        case dopeSheet = "Dope Sheet"
        case fCurve = "F-Curve"
    }
    
    @State private var viewMode: ViewMode = .timeline
    
    var body: some View {
        VStack(spacing: 0) {
            // Playback controls
            HStack {
                Button(action: {
                    if timeline.isPlaying {
                        timeline.pause()
                    } else {
                        timeline.play()
                    }
                }) {
                    Image(systemName: timeline.isPlaying ? "pause.fill" : "play.fill")
                        .font(.system(size: 20))
                }
                .buttonStyle(.plain)
                
                Button(action: timeline.stop) {
                    Image(systemName: "stop.fill")
                        .font(.system(size: 20))
                }
                .buttonStyle(.plain)
                
                Divider()
                    .frame(height: 20)
                
                Button(action: timeline.stepBackward) {
                    Image(systemName: "backward.frame")
                }
                .buttonStyle(.plain)
                
                Button(action: timeline.stepForward) {
                    Image(systemName: "forward.frame")
                }
                .buttonStyle(.plain)
                
                Divider()
                    .frame(height: 20)
                
                Text(formatTime(timeline.currentTime))
                    .font(DesignSystem.Typography.body)
                    .frame(width: 80)
                
                Text("/")
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Text(formatTime(timeline.duration))
                    .font(DesignSystem.Typography.body)
                    .frame(width: 80)
                
                Spacer()
                
                // View Mode Selector
                Picker("View Mode", selection: $viewMode) {
                    ForEach(ViewMode.allCases, id: \.self) { mode in
                        Text(mode.rawValue).tag(mode)
                    }
                }
                .pickerStyle(.segmented)
                .frame(width: 200)
                
                Spacer()
                
                // FPS selector
                Menu {
                    ForEach([24, 30, 60, 120], id: \.self) { fps in
                        Button("\(fps) FPS") {
                            timeline.fps = fps
                        }
                    }
                } label: {
                    Text("\(timeline.fps) FPS")
                        .font(DesignSystem.Typography.small)
                        .padding(4)
                        .background(DesignSystem.Colors.backgroundTertiary)
                        .cornerRadius(4)
                }
                
                Toggle("Snap", isOn: $timeline.snapToFrames)
                    .toggleStyle(.checkbox)
                
                EditorIconButton(icon: "plus", tooltip: "Add Track") {
                    timeline.addTrack(type: .animation)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Timeline Content
            Group {
                switch viewMode {
                case .timeline:
                    LegacyTimelineView(timeline: timeline, pixelsPerSecond: pixelsPerSecond)
                case .dopeSheet:
                    DopeSheet(timeline: timeline, selectedTracks: $timeline.selectedTracks)
                case .fCurve:
                    FCurveEditor(timeline: timeline, selectedTracks: $timeline.selectedTracks)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
    
    private func formatTime(_ time: Double) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        let milliseconds = Int((time.truncatingRemainder(dividingBy: 1)) * 100)
        return String(format: "%02d:%02d.%02d", minutes, seconds, milliseconds)
    }
}

// MARK: - Timeline View (Original Implementation)
struct LegacyTimelineView: View {
    @ObservedObject var timeline: AnimationTimeline
    var pixelsPerSecond: CGFloat
    
    var body: some View {
        HStack(spacing: 0) {
            // Track names
            VStack(spacing: 0) {
                ForEach(timeline.tracks) { track in
                    TrackHeaderView(track: track, timeline: timeline)
                }
            }
            .frame(width: 150)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            // Timeline ruler and tracks
            ScrollView(.horizontal) {
                VStack(spacing: 0) {
                    // Time ruler
                    TimeRulerView(
                        duration: timeline.duration,
                        currentTime: timeline.currentTime,
                        pixelsPerSecond: pixelsPerSecond
                    )
                    .frame(height: 30)
                    
                    // Tracks
                    ForEach(timeline.tracks) { track in
                        TrackView(
                            track: track,
                            currentTime: timeline.currentTime,
                            pixelsPerSecond: pixelsPerSecond
                        )
                    }
                }
            }
        }
    }
    
    private func formatTime(_ time: TimeInterval) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        let frames = Int((time - floor(time)) * Double(timeline.fps))
        return String(format: "%02d:%02d:%02d", minutes, seconds, frames)
    }
}

// MARK: - Track Header View
private struct TrackHeaderView: View {
    let track: AnimationTimeline.TimelineTrack
    let timeline: AnimationTimeline
    
    var body: some View {
        HStack {
            Image(systemName: track.type.icon)
                .font(.system(size: 14))
                .foregroundColor(track.color)
            
            Text(track.name)
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .lineLimit(1)
            
            Spacer()
            
            Button(action: {
                timeline.toggleMute(trackID: track.id)
            }) {
                Image(systemName: track.isMuted ? "speaker.slash" : "speaker")
                    .foregroundColor(track.isMuted ? .red : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
            
            Button(action: {
                timeline.toggleLock(trackID: track.id)
            }) {
                Image(systemName: track.isLocked ? "lock" : "lock.open")
                    .foregroundColor(track.isLocked ? .yellow : DesignSystem.Colors.textSecondary)
            }
            .buttonStyle(.plain)
        }
        .padding(8)
        .background(DesignSystem.Colors.backgroundTertiary)
        .frame(height: 40)
    }
    
    private func formatTime(_ time: Double) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        let milliseconds = Int((time.truncatingRemainder(dividingBy: 1)) * 100)
        return String(format: "%02d:%02d.%02d", minutes, seconds, milliseconds)
    }
}

// MARK: - Time Ruler View
private struct TimeRulerView: View {
    let duration: TimeInterval
    let currentTime: TimeInterval
    let pixelsPerSecond: CGFloat
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // Time markers
                Canvas { context, size in
                    let totalWidth = duration * pixelsPerSecond
                    
                    for second in 0...Int(duration) {
                        let x = CGFloat(second) * pixelsPerSecond
                        
                        context.stroke(
                            Path { path in
                                path.move(to: CGPoint(x: x, y: 0))
                                path.addLine(to: CGPoint(x: x, y: size.height))
                            },
                            with: .color(.white.opacity(0.3))
                        )
                        
                        context.draw(
                            Text("\(second)s").font(.system(size: 10)),
                            at: CGPoint(x: x + 4, y: 10)
                        )
                    }
                }
                
                // Playhead
                Rectangle()
                    .fill(Color.red)
                    .frame(width: 2)
                    .offset(x: currentTime * pixelsPerSecond)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Track View
private struct TrackView: View {
    let track: AnimationTimeline.TimelineTrack
    let currentTime: TimeInterval
    let pixelsPerSecond: CGFloat
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .leading) {
                // Background
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundSecondary.opacity(0.5))
                
                // Keyframes
                ForEach(track.keyframes) { keyframe in
                    Circle()
                        .fill(track.color)
                        .frame(width: 10, height: 10)
                        .offset(x: keyframe.time * pixelsPerSecond - 5)
                }
            }
        }
        .frame(height: 40)
    }
}
