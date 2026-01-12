import SwiftUI

struct TimelineEditorPanel: View {
    @StateObject private var timeline = AnimationTimeline()
    @ObservedObject var selectionManager: SelectionManager
    
    @State private var zoomLevel: CGFloat = 1.0
    @State private var scrollOffset: CGFloat = 0
    @State private var selectedKeyframes: Set<UUID> = []
    @State private var recordingMode: Bool = false
    @State private var isShiftPressed = false
    
    private let frameWidth: CGFloat = 8
    private let minZoomLevel: CGFloat = 0.1
    private let maxZoomLevel: CGFloat = 5.0
    
    var body: some View {
        VStack(spacing: 0) {
            // Transport Controls
            TimelineTransportBar(timeline: timeline, recordingMode: $recordingMode, zoomLevel: $zoomLevel, scrollOffset: $scrollOffset)
            
            EditorDivider()
            
            // Timeline View
            GeometryReader { proxy in
                VStack(spacing: 0) {
                    // Time Ruler
                    TimelineRulerView(
                        timeline: timeline,
                        width: proxy.size.width,
                        zoomLevel: zoomLevel,
                        scrollOffset: scrollOffset
                    )
                    
                    EditorDivider()
                    
                    // Tracks + Playhead
                    ZStack(alignment: .topLeading) {
                        // Track Lanes
                        ScrollView([.horizontal, .vertical]) {
                            VStack(alignment: .leading, spacing: 0) {
                                ForEach(timeline.tracks) { track in
                                    TrackLaneView(
                                        track: track,
                                        timeline: timeline,
                                        zoomLevel: zoomLevel,
                                        selectedKeyframes: $selectedKeyframes,
                                        recordingMode: recordingMode
                                    )
                                    
                                    Divider()
                                }
                                
                                if timeline.tracks.isEmpty {
                                    EmptyTimelineView()
                                        .frame(height: 200)
                                }
                            }
                            .offset(x: scrollOffset)
                        }
                        .onScrollWheel { event in
                            // Handle different scroll modes based on modifier keys
                            if event.modifierFlags.contains(.shift) {
                                // Vertical scrolling when Shift is pressed
                                let scrollDelta = event.scrollingDeltaY
                                let scrollSpeed: CGFloat = 2.0
                                
                                withAnimation(.easeOut(duration: 0.1)) {
                                    // Note: ScrollView handles vertical scrolling automatically
                                    // This is for any custom vertical behavior if needed
                                }
                            } else if event.modifierFlags.contains(.command) || event.modifierFlags.contains(.control) {
                                // Zoom with Cmd/Ctrl + scroll
                                let scrollDelta = event.scrollingDeltaY
                                let zoomSpeed: CGFloat = 0.001
                                
                                withAnimation(.easeOut(duration: 0.1)) {
                                    let newZoom = zoomLevel + scrollDelta * zoomSpeed
                                    zoomLevel = max(minZoomLevel, min(maxZoomLevel, newZoom))
                                }
                            } else {
                                // Horizontal scrolling with mouse wheel
                                let scrollDelta = event.scrollingDeltaX != 0 ? event.scrollingDeltaX : event.scrollingDeltaY
                                let scrollSpeed: CGFloat = 2.0
                                
                                withAnimation(.easeOut(duration: 0.1)) {
                                    scrollOffset += scrollDelta * scrollSpeed
                                    
                                    // Clamp scroll offset to reasonable bounds
                                    let maxScrollOffset: CGFloat = 0
                                    let minScrollOffset: CGFloat = -CGFloat(timeline.totalFrames) * frameWidth * zoomLevel + proxy.size.width - 200 // Account for track names
                                    scrollOffset = max(minScrollOffset, min(maxScrollOffset, scrollOffset))
                                }
                            }
                        }
                        .onKeyPress(.shift) { key in
                            isShiftPressed = true
                            return .ignored
                        }
                        .onKeyUp(.shift) { key in
                            isShiftPressed = false
                            return .ignored
                        }
                        
                        // Playhead
                        PlayheadView(
                            timeline: timeline,
                            zoomLevel: zoomLevel,
                            scrollOffset: scrollOffset,
                            geometry: proxy
                        )
                    }
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Transport Bar

struct TimelineTransportBar: View {
    @ObservedObject var timeline: AnimationTimeline
    @Binding var recordingMode: Bool
    @Binding var zoomLevel: CGFloat
    @Binding var scrollOffset: CGFloat
    
    private let minZoomLevel: CGFloat = 0.1
    private let maxZoomLevel: CGFloat = 5.0
    
    var body: some View {
        HStack(spacing: 12) {
            // Play/Pause
            Button(action: {
                if timeline.isPlaying {
                    timeline.pause()
                } else {
                    timeline.play()
                }
            }) {
                Image(systemName: timeline.isPlaying ? "pause.fill" : "play.fill")
                    .font(.system(size: 20))
                    .foregroundColor(.white)
                    .frame(width: 40, height: 40)
                    .background(timeline.isPlaying ? DesignSystem.Colors.accentWarning : DesignSystem.Colors.accentPrimary)
                    .cornerRadius(4)
            }
            .buttonStyle(.plain)
            
            // Stop
            Button(action: { timeline.stop() }) {
                Image(systemName: "stop.fill")
                    .font(.system(size: 16))
            }
            .buttonStyle(.plain)
            .disabled(!timeline.isPlaying && timeline.currentFrame == timeline.startFrame)
            
            // Step Backward
            Button(action: { timeline.stepBackward() }) {
                Image(systemName: "backward.frame.fill")
                    .font(.system(size: 16))
            }
            .buttonStyle(.plain)
            
            // Step Forward
            Button(action: { timeline.stepForward() }) {
                Image(systemName: "forward.frame.fill")
                    .font(.system(size: 16))
            }
            .buttonStyle(.plain)
            
            EditorDivider()
                .frame(height: 30)
            
            // Frame Display
            VStack(alignment: .leading, spacing: 2) {
                Text("Frame: \(timeline.currentFrame)")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Text(String(format: "Time: %.2fs", timeline.currentTime))
                    .font(DesignSystem.Typography.micro)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            EditorDivider()
                .frame(height: 30)
            
            // Frame Rate
            Menu {
                ForEach([24, 30, 60], id: \.self) { fps in
                    Button("\(fps) FPS") {
                        timeline.frameRate = fps
                    }
                }
            } label: {
                HStack {
                    Text("\(timeline.frameRate) FPS")
                        .font(DesignSystem.Typography.small)
                    Image(systemName: "chevron.down")
                        .font(.system(size: 8))
                }
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(4)
            }
            .menuStyle(.borderlessButton)
            
            EditorDivider()
                .frame(height: 30)
            
            // Zoom Controls
            HStack(spacing: 4) {
                Button(action: {
                    withAnimation(.easeOut(duration: 0.1)) {
                        zoomLevel = max(minZoomLevel, zoomLevel * 0.8)
                    }
                }) {
                    Image(systemName: "minus.magnifyingglass")
                        .font(.system(size: 14))
                }
                .buttonStyle(.plain)
                .help("Zoom Out (Cmd/Ctrl + Scroll)")
                
                Text("\(Int(zoomLevel * 100))%")
                    .font(DesignSystem.Typography.smallMono)
                    .frame(width: 40)
                
                Button(action: {
                    withAnimation(.easeOut(duration: 0.1)) {
                        zoomLevel = min(maxZoomLevel, zoomLevel * 1.25)
                    }
                }) {
                    Image(systemName: "plus.magnifyingglass")
                        .font(.system(size: 14))
                }
                .buttonStyle(.plain)
                .help("Zoom In (Cmd/Ctrl + Scroll)")
                
                Button(action: {
                    withAnimation(.easeOut(duration: 0.2)) {
                        zoomLevel = 1.0
                        scrollOffset = 0
                    }
                }) {
                    Image(systemName: "1.magnifyingglass")
                        .font(.system(size: 14))
                }
                .buttonStyle(.plain)
                .help("Reset Zoom")
            }
            
            EditorDivider()
                .frame(height: 30)
            
            // Recording Toggle
            Toggle(isOn: $recordingMode) {
                HStack(spacing: 4) {
                    Circle()
                        .fill(recordingMode ? Color.red : Color.gray)
                        .frame(width: 8, height: 8)
                    Text("Auto Key")
                        .font(DesignSystem.Typography.small)
                }
            }
            .toggleStyle(.button)
            .help("Automatically insert keyframes when changing values")
            
            Spacer()
            
            // Range
            HStack(spacing: 4) {
                TextField("Start", value: $timeline.startFrame, format: .number)
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 50)
                
                Text("to")
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                TextField("End", value: $timeline.endFrame, format: .number)
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 50)
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Time Ruler

struct TimelineRulerView: View {
    let timeline: AnimationTimeline
    let width: CGFloat
    let zoomLevel: CGFloat
    let scrollOffset: CGFloat
    
    private let majorTickInterval = 10
    private let minorTickInterval = 1
    private let frameWidth: CGFloat = 8
    
    var body: some View {
        Canvas { context, size in
            let scaledFrameWidth = frameWidth * zoomLevel
            
            // Draw ticks
            for frame in timeline.startFrame...timeline.endFrame {
                let x = CGFloat(frame - timeline.startFrame) * scaledFrameWidth + scrollOffset
                
                guard x >= 0 && x <= size.width else { continue }
                
                let isMajor = frame % majorTickInterval == 0
                let tickHeight: CGFloat = isMajor ? 20 : 10
                
                var path = Path()
                path.move(to: CGPoint(x: x, y: size.height - tickHeight))
                path.addLine(to: CGPoint(x: x, y: size.height))
                
                context.stroke(path, with: .color(DesignSystem.Colors.border), lineWidth: 1)
                
                // Draw frame number for major ticks
                if isMajor {
                    let text = Text("\(frame)")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    context.draw(text, at: CGPoint(x: x + 2, y: 4))
                }
            }
        }
        .frame(height: 30)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Track Lane

struct TrackLaneView: View {
    let track: AnimationTimeline.TimelineTrack
    @ObservedObject var timeline: AnimationTimeline
    let zoomLevel: CGFloat
    @Binding var selectedKeyframes: Set<UUID>
    let recordingMode: Bool
    
    private let frameWidth: CGFloat = 8
    private let laneHeight: CGFloat = 40
    
    var body: some View {
        HStack(spacing: 0) {
            // Track Name/Controls
            HStack(spacing: 8) {
                Button(action: { timeline.toggleMute(trackID: track.id) }) {
                    Image(systemName: track.isMuted ? "speaker.slash.fill" : "speaker.wave.2.fill")
                        .font(.system(size: 12))
                        .foregroundColor(track.isMuted ? DesignSystem.Colors.textTertiary : DesignSystem.Colors.accentPrimary)
                }
                .buttonStyle(.plain)
                
                Text(track.name)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                Button(action: { timeline.toggleLock(trackID: track.id) }) {
                    Image(systemName: track.isLocked ? "lock.fill" : "lock.open")
                        .font(.system(size: 12))
                        .foregroundColor(track.isLocked ? DesignSystem.Colors.textTertiary : DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
            }
            .frame(width: 200)
            .padding(.horizontal, 8)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            // Keyframe Lane
            ZStack(alignment: .leading) {
                // Background
                Rectangle()
                    .fill(DesignSystem.Colors.backgroundPrimary)
                
                // Keyframes
                ForEach(track.keyframes) { keyframe in
                    KeyframeMarker(
                        keyframe: keyframe,
                        track: track,
                        timeline: timeline,
                        isSelected: selectedKeyframes.contains(keyframe.id)
                    )
                    .position(
                        x: CGFloat(Int(keyframe.time * Double(timeline.fps)) - timeline.startFrame) * frameWidth * zoomLevel,
                        y: laneHeight / 2
                    )
                    .onTapGesture {
                        if selectedKeyframes.contains(keyframe.id) {
                            selectedKeyframes.remove(keyframe.id)
                        } else {
                            selectedKeyframes.insert(keyframe.id)
                        }
                    }
                }
            }
            .frame(height: laneHeight)
            .frame(minWidth: CGFloat(timeline.totalFrames) * frameWidth * zoomLevel)
        }
    }
}

// MARK: - Keyframe Marker

struct KeyframeMarker: View {
    let keyframe: AnimationTimeline.Keyframe
    let track: AnimationTimeline.TimelineTrack
    let timeline: AnimationTimeline
    let isSelected: Bool
    
    var body: some View {
        ZStack {
            // Diamond shape
            Diamond()
                .fill(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.accentSuccess)
                .frame(width: 12, height: 12)
            
            Diamond()
                .stroke(Color.white, lineWidth: isSelected ? 2 : 1)
                .frame(width: 12, height: 12)
        }
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

// MARK: - Playhead

struct PlayheadView: View {
    @ObservedObject var timeline: AnimationTimeline
    let zoomLevel: CGFloat
    let scrollOffset: CGFloat
    let geometry: GeometryProxy
    
    private let frameWidth: CGFloat = 8
    
    var playheadX: CGFloat {
        CGFloat(timeline.currentFrame - timeline.startFrame) * frameWidth * zoomLevel + scrollOffset
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Playhead line
            Rectangle()
                .fill(DesignSystem.Colors.accentWarning)
                .frame(width: 2)
            
            // Playhead handle
            Triangle()
                .fill(DesignSystem.Colors.accentWarning)
                .frame(width: 12, height: 8)
                .offset(y: -8)
        }
        .offset(x: playheadX)
        .gesture(
            DragGesture()
                .onChanged { value in
                    let frameWidth = self.frameWidth * zoomLevel
                    let frame = Int((value.location.x - scrollOffset) / frameWidth) + timeline.startFrame
                    timeline.goToFrame(frame)
                }
        )
    }
}

struct Triangle: Shape {
    func path(in rect: CGRect) -> Path {
        var path = Path()
        path.move(to: CGPoint(x: rect.midX, y: rect.maxY))
        path.addLine(to: CGPoint(x: rect.minX, y: rect.minY))
        path.addLine(to: CGPoint(x: rect.maxX, y: rect.minY))
        path.closeSubpath()
        return path
    }
}

// MARK: - Empty State

struct EmptyTimelineView: View {
    var body: some View {
        VStack(spacing: 12) {
            Image(systemName: "waveform.path.ecg")
                .font(.system(size: 40))
                .foregroundColor(DesignSystem.Colors.textTertiary)
            
            Text("No Animation Tracks")
                .font(DesignSystem.Typography.body)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            Text("Select an object and press 'I' to insert a keyframe")
                .font(DesignSystem.Typography.small)
                .foregroundColor(DesignSystem.Colors.textTertiary)
        }
        .frame(maxWidth: .infinity)
    }
}

#Preview {
    TimelineEditorPanel(selectionManager: SelectionManager())
        .frame(height: 300)
}
