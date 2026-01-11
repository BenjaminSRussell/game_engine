// SwiftUI/Sequencer.swift
// Cinematic sequencer (timeline editor)
import SwiftUI

struct SequencerView: View {
    @State private var tracks: [SequencerTrack] = []
    @State private var currentTime: Double = 0
    @State private var duration: Double = 60
    @State private var isPlaying = false
    @State private var zoom: CGFloat = 1.0
    @State private var playbackTimer: Timer?
    @State private var lastUpdateTime: Date = Date()
    
    var body: some View {
        VStack(spacing: 0) {
            // Playback controls
            HStack {
                Button(action: { currentTime = 0 }) {
                    Image(systemName: "backward.end.fill")
                }
                Button(action: { togglePlayback() }) {
                    Image(systemName: isPlaying ? "pause.fill" : "play.fill")
                }
                Button(action: { currentTime = duration }) {
                    Image(systemName: "forward.end.fill")
                }
                
                Divider()
                
                Text(String(format: "%.2fs", currentTime))
                    .font(.system(.body, design: .monospaced))
                
                Spacer()
                
                Slider(value: $zoom, in: 0.5...5.0)
                    .frame(width: 200)
                
                Button("Add Track") {
                    addTrack()
                }
            }
            .padding()
            .background(Color(nsColor: .controlBackgroundColor))
            
            Divider()
            
            // Timeline
            GeometryReader { geometry in
                ScrollView([.horizontal, .vertical]) {
                    VStack(spacing: 0) {
                        // Time ruler
                        TimeRuler(duration: duration, zoom: zoom, width: geometry.size.width)
                            .frame(height: 30)
                        
                        // Tracks
                        ForEach(tracks) { track in
                            TrackView(track: track, currentTime: $currentTime, zoom: zoom)
                                .frame(height: 60)
                        }
                    }
                }
                .overlay(
                    // Playhead
                    Rectangle()
                        .fill(Color.red)
                        .frame(width: 2)
                        .offset(x: CGFloat(currentTime / duration) * geometry.size.width * zoom)
                    , alignment: .leading
                )
            }
        }
        .onAppear {
            // Add default tracks
            tracks = [
                SequencerTrack(name: "Camera", type: .camera),
                SequencerTrack(name: "Audio", type: .audio),
                SequencerTrack(name: "Animation", type: .animation)
            ]
        }
    }
    
    func togglePlayback() {
        isPlaying.toggle()
        
        if isPlaying {
            lastUpdateTime = Date()
            playbackTimer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { _ in
                let now = Date()
                let deltaTime = now.timeIntervalSince(lastUpdateTime)
                lastUpdateTime = now
                
                currentTime += deltaTime
                
                if currentTime >= duration {
                    currentTime = duration
                    isPlaying = false
                    playbackTimer?.invalidate()
                    playbackTimer = nil
                }
                
                // Update track animations
                updateTrackAnimations()
            }
        } else {
            playbackTimer?.invalidate()
            playbackTimer = nil
        }
    }
    
    private func updateTrackAnimations() {
        for track in tracks {
            for clip in track.clips {
                if currentTime >= clip.startTime && currentTime <= clip.endTime {
                    clip.isActive = true
                    // Trigger animation callbacks
                    if let animationClip = clip as? AnimationClip {
                        animationClip.updateAtTime(currentTime - clip.startTime)
                    }
                } else {
                    clip.isActive = false
                }
            }
        }
    }
    
    func addTrack() {
        tracks.append(SequencerTrack(name: "Track \\(tracks.count + 1)", type: .generic))
    }
}

struct TimeRuler: View {
    let duration: Double
    let zoom: CGFloat
    let width: CGFloat
    
    var body: some View {
        Canvas { context, size in
            let totalWidth = width * zoom
            let secondWidth = totalWidth / CGFloat(duration)
            
            for second in 0...Int(duration) {
                let x = CGFloat(second) * secondWidth
                let isMajor = second % 5 == 0
                
                // Tick
                var path = Path()
                path.move(to: CGPoint(x: x, y: size.height))
                path.addLine(to: CGPoint(x: x, y: size.height - (isMajor ? 15 : 10)))
                context.stroke(path, with: .color(.white), lineWidth: 1)
                
                // Label
                if isMajor {
                    let text = Text("\\(second)s").font(.caption2)
                    context.draw(text, at: CGPoint(x: x + 5, y: 5))
                }
            }
        }
        .frame(width: width * zoom)
        .background(Color.black.opacity(0.3))
    }
}

struct TrackView: View {
    let track: SequencerTrack
    @Binding var currentTime: Double
    let zoom: CGFloat
    
    var body: some View {
        HStack(spacing: 0) {
            // Track label
            Text(track.name)
                .frame(width: 120)
                .background(Color(nsColor: .controlBackgroundColor))
            
            // Track content
            ZStack(alignment: .leading) {
                Color.black.opacity(0.2)
                
                ForEach(track.clips) { clip in
                    ClipView(clip: clip, zoom: zoom)
                        .offset(x: CGFloat(clip.startTime) * 100 * zoom)
                }
            }
        }
    }
}

struct ClipView: View {
    let clip: SequencerClip
    let zoom: CGFloat
    
    var body: some View {
        RoundedRectangle(cornerRadius: 4)
            .fill(clip.color.opacity(0.7))
            .frame(width: CGFloat(clip.duration) * 100 * zoom, height: 40)
            .overlay(
                Text(clip.name)
                    .font(.caption)
                    .foregroundColor(.white)
                    .padding(4)
                , alignment: .leading
            )
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(Color.white.opacity(0.5), lineWidth: 1)
            )
    }
}

struct SequencerTrack: Identifiable {
    let id = UUID()
    var name: String
    var type: TrackType
    var clips: [SequencerClip] = []
    
    enum TrackType {
        case camera, audio, animation, event, generic
    }
}

struct SequencerClip: Identifiable {
    let id = UUID()
    var name: String
    var startTime: Double
    var duration: Double
    var color: Color = .blue
}
