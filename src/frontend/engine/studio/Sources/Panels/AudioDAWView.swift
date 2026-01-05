import SwiftUI
import AVFoundation
import Accelerate

// MARK: - Audio DAW View (Logic Pro Style)
struct AudioDAWView: View {
    @ObservedObject var dawManager = AudioDAWManager.shared
    @ObservedObject var timingSystem = AudioTimingSystem.shared
    
    @State private var zoom: CGFloat = 1.0
    @State private var selectedTrack: AudioTrack? = nil
    @State private var showingAudioImporter = false
    
    var body: some View {
        VStack(spacing: 0) {
            // Top toolbar
            DAWToolbar(zoom: $zoom, showingImporter: $showingAudioImporter)
            
            Divider()
            
            // Main content
            HSplitView {
                // Track list (left sidebar)
                TrackListSidebar(
                    tracks: dawManager.tracks,
                    selectedTrack: $selectedTrack,
                    onAddTrack: { dawManager.addTrack(name: "Track \(dawManager.tracks.count + 1)") }
                )
                .frame(minWidth: 200, maxWidth: 300)
                
                // Timeline/arrangement view
                GeometryReader { geometry in
                    ScrollView([.horizontal, .vertical], showsIndicators: true) {
                        ZStack(alignment: .topLeading) {
                            // Grid background
                            TimelineGrid(
                                width: geometry.size.width,
                                height: CGFloat(dawManager.tracks.count) * 120,
                                zoom: zoom,
                                bpm: timingSystem.bpm
                            )
                            
                            // Tracks
                            VStack(spacing: 0) {
                                ForEach(dawManager.tracks) { track in
                                    TrackLane(
                                        track: track,
                                        zoom: zoom,
                                        bpm: timingSystem.bpm,
                                        isSelected: selectedTrack?.id == track.id
                                    )
                                    .frame(height: 120)
                                    .onTapGesture {
                                        selectedTrack = track
                                    }
                                }
                            }
                            
                            // Playhead
                            if timingSystem.isPlaying {
                                Rectangle()
                                    .fill(DesignSystem.Colors.accentDanger)
                                    .frame(width: 2)
                                    .offset(x: playheadPosition(zoom: zoom))
                            }
                        }
                        .frame(
                            minWidth: max(geometry.size.width, 5000 * zoom),
                            minHeight: max(geometry.size.height, CGFloat(dawManager.tracks.count) * 120)
                        )
                    }
                }
            }
            
            // Bottom transport controls
            TransportControls(timingSystem: timingSystem, dawManager: dawManager)
                .frame(height: 60)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .fileImporter(
            isPresented: $showingAudioImporter,
            allowedContentTypes: [.audio],
            allowsMultipleSelection: true
        ) { result in
            switch result {
            case .success(let urls):
                for url in urls {
                    _ = url.startAccessingSecurityScopedResource()
                    dawManager.importAudioClip(from: url, toTrack: selectedTrack ?? dawManager.tracks.first)
                    url.stopAccessingSecurityScopedResource()
                }
            case .failure(let error):
                print("Import failed: \(error)")
            }
        }
    }
    
    private func playheadPosition(zoom: CGFloat) -> CGFloat {
        let beatWidth = 100.0 * zoom
        return CGFloat(timingSystem.currentBeat) * beatWidth + CGFloat(timingSystem.beatPhase) * beatWidth
    }
}

// MARK: - DAW Toolbar
struct DAWToolbar: View {
    @Binding var zoom: CGFloat
    @Binding var showingImporter: Bool
    
    var body: some View {
        HStack(spacing: 16) {
            Text("Audio DAW")
                .font(DesignSystem.Typography.title2)
            
            Spacer()
            
            // Zoom control
            HStack(spacing: 8) {
                Button(action: { zoom = max(0.1, zoom - 0.2) }) {
                    Image(systemName: "minus.magnifyingglass")
                }
                .buttonStyle(.plain)
                
                Text("\(Int(zoom * 100))%")
                    .font(DesignSystem.Typography.mono)
                    .frame(width: 50)
                
                Button(action: { zoom = min(5.0, zoom + 0.2) }) {
                    Image(systemName: "plus.magnifyingglass")
                }
                .buttonStyle(.plain)
            }
            
            // Import button
            Button(action: { showingImporter = true }) {
                HStack(spacing: 6) {
                    Image(systemName: "plus.circle.fill")
                    Text("Import Audio")
                }
            }
            .buttonStyle(.plain)
            .padding(.horizontal, 12)
            .padding(.vertical, 6)
            .background(DesignSystem.Colors.accentPrimary.opacity(0.1))
            .clipShape(Capsule())
        }
        .padding(12)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Track List Sidebar
struct TrackListSidebar: View {
    let tracks: [AudioTrack]
    @Binding var selectedTrack: AudioTrack?
    let onAddTrack: () -> Void
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack {
                Text("Tracks")
                    .font(DesignSystem.Typography.bodyBold)
                Spacer()
                Button(action: onAddTrack) {
                    Image(systemName: "plus.circle.fill")
                }
                .buttonStyle(.plain)
            }
            .padding(12)
            
            Divider()
            
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach(tracks) { track in
                        TrackSidebarRow(
                            track: track,
                            isSelected: selectedTrack?.id == track.id
                        )
                        .onTapGesture {
                            selectedTrack = track
                        }
                    }
                }
                .padding(8)
            }
        }
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

struct TrackSidebarRow: View {
    @ObservedObject var track: AudioTrack
    let isSelected: Bool
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: track.isMuted ? "speaker.slash.fill" : "speaker.wave.2.fill")
                    .foregroundColor(track.isMuted ? .red : DesignSystem.Colors.accentPrimary)
                
                Text(track.name)
                    .font(DesignSystem.Typography.body)
                    .lineLimit(1)
                
                Spacer()
            }
            
            // Volume slider
            HStack {
                Text("Vol")
                    .font(DesignSystem.Typography.micro)
                    .frame(width: 30)
                Slider(value: $track.volume, in: 0...1)
                Text("\(Int(track.volume * 100))")
                    .font(DesignSystem.Typography.mono)
                    .frame(width: 30)
            }
            
            // Pan slider
            HStack {
                Text("Pan")
                    .font(DesignSystem.Typography.micro)
                    .frame(width: 30)
                Slider(value: $track.pan, in: -1...1)
                Text(panText)
                    .font(DesignSystem.Typography.mono)
                    .frame(width: 30)
            }
        }
        .padding(8)
        .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
        .cornerRadius(6)
    }
    
    private var panText: String {
        if track.pan < -0.05 { return "L\(Int(abs(track.pan) * 100))" }
        if track.pan > 0.05 { return "R\(Int(track.pan * 100))" }
        return "C"
    }
}

// MARK: - Timeline Grid
struct TimelineGrid: View {
    let width: CGFloat
    let height: CGFloat
    let zoom: CGFloat
    let bpm: Double
    
    var body: some View {
        Canvas { context, size in
            let beatWidth = 100.0 * zoom
            let beatsToShow = Int(size.width / beatWidth) + 2
            
            // Draw beat lines
            for beat in 0...beatsToShow {
                let x = CGFloat(beat) * beatWidth
                let isBar = beat % 4 == 0
                
                var path = Path()
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
                
                context.stroke(
                    path,
                    with: .color(isBar ? DesignSystem.Colors.border.opacity(0.5) : DesignSystem.Colors.border.opacity(0.2)),
                    lineWidth: isBar ? 2 : 1
                )
                
                // Bar numbers
                if isBar {
                    let barNumber = beat / 4 + 1
                    let text = Text("\(barNumber)")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    context.draw(text, at: CGPoint(x: x + 4, y: 8))
                }
            }
            
            // Draw horizontal track separators
            let trackHeight: CGFloat = 120
            let trackCount = Int(size.height / trackHeight) + 1
            for track in 1...trackCount {
                let y = CGFloat(track) * trackHeight
                var path = Path()
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
                context.stroke(path, with: .color(DesignSystem.Colors.border.opacity(0.3)), lineWidth: 1)
            }
        }
        .frame(width: width, height: height)
    }
}

// MARK: - Track Lane
struct TrackLane: View {
    @ObservedObject var track: AudioTrack
    let zoom: CGFloat
    let bpm: Double
    let isSelected: Bool
    
    var body: some View {
        ZStack(alignment: .topLeading) {
            // Background
            Rectangle()
                .fill(isSelected ? DesignSystem.Colors.selection.opacity(0.1) : Color.clear)
            
            // Audio clips
            ForEach(track.clips) { clip in
                AudioClipView(clip: clip, zoom: zoom, bpm: bpm)
                    .offset(x: clipPosition(clip))
            }
        }
    }
    
    private func clipPosition(_ clip: AudioClip) -> CGFloat {
        let beatWidth = 100.0 * zoom
        return CGFloat(clip.startBeat) * beatWidth
    }
}

// MARK: - Audio Clip View (Block)
struct AudioClipView: View {
    @ObservedObject var clip: AudioClip
    let zoom: CGFloat
    let bpm: Double
    
    @State private var isHovered = false
    
    var body: some View {
        ZStack(alignment: .leading) {
            // Background
            RoundedRectangle(cornerRadius: 6)
                .fill(clip.color.opacity(0.3))
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(clip.color, lineWidth: isHovered ? 2 : 1)
                )
            
            // Waveform
            if let waveform = clip.waveformData {
                WaveformView(waveform: waveform, color: clip.color)
                    .padding(4)
            }
            
            // Label
            Text(clip.name)
                .font(DesignSystem.Typography.micro)
                .foregroundColor(.white)
                .padding(6)
                .background(Color.black.opacity(0.5))
                .cornerRadius(4)
                .padding(4)
                .allowsHitTesting(false)
        }
        .frame(width: clipWidth, height: 100)
        .shadow(color: .black.opacity(0.3), radius: isHovered ? 6 : 2, x: 0, y: 2)
        .scaleEffect(isHovered ? 1.02 : 1.0)
        .animation(.spring(response: 0.2, dampingFraction: 0.7), value: isHovered)
        .onHover { isHovered = $0 }
    }
    
    private var clipWidth: CGFloat {
        let beatWidth = 100.0 * zoom
        let secondsPerBeat = 60.0 / bpm
        let durationInBeats = clip.duration / secondsPerBeat
        return CGFloat(durationInBeats) * beatWidth
    }
}

// MARK: - Waveform View
struct WaveformView: View {
    let waveform: [Float]
    let color: Color
    
    var body: some View {
        Canvas { context, size in
            let count = waveform.count
            guard count > 0 else { return }
            
            let barWidth = size.width / CGFloat(count)
            let centerY = size.height / 2
            
            for (index, amplitude) in waveform.enumerated() {
                let x = CGFloat(index) * barWidth
                let barHeight = CGFloat(amplitude) * size.height * 0.8
                
                var path = Path()
                path.move(to: CGPoint(x: x, y: centerY - barHeight / 2))
                path.addLine(to: CGPoint(x: x, y: centerY + barHeight / 2))
                
                context.stroke(path, with: .color(color), lineWidth: barWidth * 0.8)
            }
        }
    }
}

// MARK: - Transport Controls
struct TransportControls: View {
    @ObservedObject var timingSystem: AudioTimingSystem
    @ObservedObject var dawManager: AudioDAWManager
    
    var body: some View {
        HStack(spacing: 20) {
            // Play/Pause/Stop
            HStack(spacing: 12) {
                Button(action: { timingSystem.start(); dawManager.play() }) {
                    Image(systemName: "play.fill")
                        .font(.system(size: 20))
                }
                .buttonStyle(.plain)
                .disabled(timingSystem.isPlaying)
                
                Button(action: { timingSystem.stop(); dawManager.stop() }) {
                    Image(systemName: "stop.fill")
                        .font(.system(size: 20))
                }
                .buttonStyle(.plain)
            }
            
            Divider()
                .frame(height: 30)
            
            // Time display
            VStack(alignment: .leading, spacing: 2) {
                Text("Bar \(timingSystem.currentBeat / 4 + 1) | Beat \(timingSystem.currentBeat % 4 + 1)")
                    .font(DesignSystem.Typography.mono)
                Text("\(timingSystem.bpm, specifier: "%.1f") BPM")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            // Master volume
            HStack {
                Image(systemName: "speaker.wave.2.fill")
                Slider(value: $dawManager.masterVolume, in: 0...1)
                    .frame(width: 150)
                Text("\(Int(dawManager.masterVolume * 100))%")
                    .font(DesignSystem.Typography.mono)
                    .frame(width: 40)
            }
        }
        .padding(.horizontal, 16)
        .background(DesignSystem.Colors.backgroundSecondary)
    }
}

// MARK: - Audio DAW Manager
class AudioDAWManager: ObservableObject {
    static let shared = AudioDAWManager()
    
    @Published var tracks: [AudioTrack] = []
    @Published var masterVolume: Double = 0.8
    
    init() {
        // Create default tracks
        addTrack(name: "Music")
        addTrack(name: "Sound Effects")
        addTrack(name: "Dialogue")
    }
    
    func addTrack(name: String) {
        let track = AudioTrack(name: name)
        tracks.append(track)
    }
    
    func importAudioClip(from url: URL, toTrack track: AudioTrack?) {
        guard let targetTrack = track ?? tracks.first else { return }
        
        do {
            let audioFile = try AVAudioFile(forReading: url)
            let duration = Double(audioFile.length) / audioFile.fileFormat.sampleRate
            
            let clip = AudioClip(
                name: url.deletingPathExtension().lastPathComponent,
                url: url,
                duration: duration,
                startBeat: 0
            )
            
            // Generate waveform
            generateWaveform(for: clip, audioFile: audioFile)
            
            targetTrack.clips.append(clip)
            
        } catch {
            print("Failed to import clip: \(error)")
        }
    }
    
    private func generateWaveform(for clip: AudioClip, audioFile: AVAudioFile) {
        let targetSamples = 200  // Number of bars in waveform
        
        guard let buffer = AVAudioPCMBuffer(
            pcmFormat: audioFile.processingFormat,
            frameCapacity: AVAudioFrameCount(audioFile.length)
        ) else { return }
        
        do {
            try audioFile.read(into: buffer)
            
            guard let channelData = buffer.floatChannelData else { return }
            let frameLength = Int(buffer.frameLength)
            let samplesPerBar = frameLength / targetSamples
            
            var waveform: [Float] = []
            
            for i in 0..<targetSamples {
                let startIndex = i * samplesPerBar
                let endIndex = min(startIndex + samplesPerBar, frameLength)
                
                var maxAmplitude: Float = 0
                for j in startIndex..<endIndex {
                    let sample = abs(channelData[0][j])
                    maxAmplitude = max(maxAmplitude, sample)
                }
                
                waveform.append(maxAmplitude)
            }
            
            clip.waveformData = waveform
            
        } catch {
            print("Waveform generation failed: \(error)")
        }
    }
    
    func play() {
        // Play all non-muted tracks
        for track in tracks where !track.isMuted {
            // Would trigger AVAudioEngine playback here
        }
    }
    
    func stop() {
        // Stop all tracks
    }
}

// MARK: - Data Models
class AudioTrack: Identifiable, ObservableObject {
    let id = UUID()
    @Published var name: String
    @Published var volume: Double = 0.8
    @Published var pan: Double = 0.0
    @Published var isMuted: Bool = false
    @Published var clips: [AudioClip] = []
    
    init(name: String) {
        self.name = name
    }
}

class AudioClip: Identifiable, ObservableObject {
    let id = UUID()
    let name: String
    let url: URL
    let duration: TimeInterval
    @Published var startBeat: Int
    @Published var color: Color
    @Published var waveformData: [Float]? = nil
    
    init(name: String, url: URL, duration: TimeInterval, startBeat: Int) {
        self.name = name
        self.url = url
        self.duration = duration
        self.startBeat = startBeat
        
        // Random color for clip
        self.color = [Color.blue, .purple, .green, .orange, .pink, .cyan].randomElement() ?? .blue
    }
}
