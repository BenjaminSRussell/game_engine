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
    
    // TODO: Implement multi-track timeline rendering
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
    
    // TODO: Implement clip blending/crossfade transitions
    // TODO: Implement curve editor for animated properties
    // TODO: Implement bezier curve tangent editing
    
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
    
    // TODO: Implement post-process track
    // TODO: Implement text/title track
    
    // TODO: Implement timeline zoom levels
    func setZoom(_ zoom: CGFloat) {
        print("[Timeline] Set zoom: \(zoom)")
    }
    
    // TODO: Implement multi-camera editing
    // TODO: Implement preview render
    // TODO: Implement export to video
    func exportVideo(path: String, resolution: SIMD2<Int>, fps: Int) {
        print("[Timeline] Exporting video: \(path)")
    }
    
    func clear() {
        tracks.removeAll()
        currentTime = 0
        isPlaying = false
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
