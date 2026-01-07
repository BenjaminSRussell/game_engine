import SwiftUI
import UniformTypeIdentifiers

// MARK: - Music Import & Timeline Panel
struct MusicTimelinePanel: View {
    @ObservedObject var importManager = AudioImportManager.shared
    @ObservedObject var timingSystem = AudioTimingSystem.shared
    @ObservedObject var backgroundMusic = BackgroundMusicSystem.shared
    
    @State private var selectedTrack: ImportedAudioTrack?
    @State private var showingImporter = false
    @State private var zoom: CGFloat = 1.0
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Music Timeline")
                    .font(DesignSystem.Typography.title2)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Import button
                Button(action: { showingImporter = true }) {
                    HStack(spacing: 6) {
                        Image(systemName: "plus.circle.fill")
                        Text("Import Audio")
                    }
                    .font(DesignSystem.Typography.body)
                }
                .buttonStyle(.plain)
                .foregroundColor(DesignSystem.Colors.accentPrimary)
                .padding(.horizontal, 12)
                .padding(.vertical, 6)
                .background(DesignSystem.Colors.accentPrimary.opacity(0.1))
                .clipShape(Capsule())
                
                // Playback controls
                HStack(spacing: 8) {
                    Button(action: { timingSystem.start() }) {
                        Image(systemName: "play.fill")
                            .font(.system(size: 14))
                    }
                    .buttonStyle(.plain)
                    .disabled(selectedTrack == nil)
                    
                    Button(action: { timingSystem.stop() }) {
                        Image(systemName: "stop.fill")
                            .font(.system(size: 14))
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(12)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            Divider()
            
            HSplitView {
                // Track list
                VStack(alignment: .leading, spacing: 0) {
                    Text("Imported Tracks")
                        .font(DesignSystem.Typography.bodyBold)
                        .padding(12)
                    
                    Divider()
                    
                    ScrollView {
                        LazyVStack(spacing: 4) {
                            ForEach(importManager.importedTracks) { track in
                                TrackRow(track: track, isSelected: selectedTrack?.id == track.id)
                                    .onTapGesture {
                                        selectedTrack = track
                                        loadTrack(track)
                                    }
                            }
                        }
                        .padding(8)
                    }
                }
                .frame(minWidth: 250, maxWidth: 350)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                // Timeline view
                VStack(spacing: 0) {
                    if let track = selectedTrack {
                        // Track info
                        HStack {
                            VStack(alignment: .leading, spacing: 4) {
                                Text(track.name)
                                    .font(DesignSystem.Typography.headline)
                                Text("BPM: \(Int(track.estimatedBPM)) | Duration: \(formatDuration(track.duration))")
                                    .font(DesignSystem.Typography.caption)
                                    .foregroundColor(DesignSystem.Colors.textSecondary)
                            }
                            
                            Spacer()
                            
                            // BPM adjustment
                            HStack(spacing: 8) {
                                Text("BPM:")
                                    .font(DesignSystem.Typography.body)
                                TextField("", value: Binding(
                                    get: { track.estimatedBPM },
                                    set: { newBPM in
                                        track.estimatedBPM = newBPM
                                        timingSystem.setBPM(newBPM)
                                    }
                                ), format: .number)
                                .textFieldStyle(.plain)
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 50)
                                .padding(4)
                                .background(DesignSystem.Colors.backgroundTertiary)
                                .cornerRadius(4)
                            }
                        }
                        .padding(12)
                        .background(DesignSystem.Colors.backgroundSecondary)
                        
                        Divider()
                        
                        // Timeline
                        GeometryReader { geometry in
                            TimelineView(
                                track: track,
                                timingSystem: timingSystem,
                                width: geometry.size.width,
                                zoom: zoom
                            )
                        }
                        
                        // Zoom controls
                        HStack {
                            Text("Zoom:")
                                .font(DesignSystem.Typography.caption)
                            Slider(value: $zoom, in: 0.1...5.0)
                                .frame(width: 150)
                            Text("\(Int(zoom * 100))%")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 50)
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundSecondary)
                        
                    } else {
                        // Empty state
                        VStack(spacing: 16) {
                            Image(systemName: "music.note.list")
                                .font(.system(size: 64))
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                            Text("Select a track or import one")
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                    }
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .fileImporter(
            isPresented: $showingImporter,
            allowedContentTypes: [.audio],
            allowsMultipleSelection: false
        ) { result in
            switch result {
            case .success(let urls):
                guard let url = urls.first else { return }
                _ = url.startAccessingSecurityScopedResource()
                importManager.importAudio(from: url) { track in
                    if let track = track {
                        selectedTrack = track
                    }
                }
                url.stopAccessingSecurityScopedResource()
            case .failure(let error):
                print("Import failed: \(error)")
            }
        }
    }
    
    private func loadTrack(_ track: ImportedAudioTrack) {
        timingSystem.setBPM(track.estimatedBPM)
        timingSystem.beatMarkers = track.beatMarkers
        backgroundMusic.loadTrack(url: track.url)
    }
    
    private func formatDuration(_ duration: TimeInterval) -> String {
        let minutes = Int(duration) / 60
        let seconds = Int(duration) % 60
        return String(format: "%d:%02d", minutes, seconds)
    }
}

// MARK: - Track Row
struct TrackRow: View {
    @ObservedObject var track: ImportedAudioTrack
    let isSelected: Bool
    
    var body: some View {
        HStack(spacing: 12) {
            Image(systemName: "waveform")
                .font(.system(size: 16))
                .foregroundColor(DesignSystem.Colors.accentPrimary)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(track.name)
                    .font(DesignSystem.Typography.body)
                    .lineLimit(1)
                
                Text("\(Int(track.estimatedBPM)) BPM")
                    .font(DesignSystem.Typography.caption)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            if track.isAnalyzed {
                Image(systemName: "checkmark.circle.fill")
                    .foregroundColor(DesignSystem.Colors.accentSuccess)
                    .font(.system(size: 14))
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
        .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
        .cornerRadius(6)
    }
}

// MARK: - Timeline View
struct TimelineView: View {
    @ObservedObject var track: ImportedAudioTrack
    @ObservedObject var timingSystem: AudioTimingSystem
    let width: CGFloat
    let zoom: CGFloat
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: true) {
            ZStack(alignment: .topLeading) {
                // Background grid
                Canvas { context, size in
                    let beatWidth = 50.0 * zoom
                    let totalBeats = Int(track.duration / (60.0 / track.estimatedBPM))
                    
                    // Draw beat lines
                    for beat in 0...totalBeats {
                        let x = CGFloat(beat) * beatWidth
                        let isDownbeat = beat % 4 == 0
                        
                        context.stroke(
                            Path { path in
                                path.move(to: CGPoint(x: x, y: 0))
                                path.addLine(to: CGPoint(x: x, y: size.height))
                            },
                            with: .color(isDownbeat ? DesignSystem.Colors.border.opacity(0.8) : DesignSystem.Colors.border.opacity(0.3)),
                            lineWidth: isDownbeat ? 2 : 1
                        )
                        
                        // Beat numbers
                        if isDownbeat {
                            let text = Text("\(beat / 4 + 1)")
                                .font(DesignSystem.Typography.micro)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            context.draw(text, at: CGPoint(x: x + 4, y: 8))
                        }
                    }
                }
                .frame(width: max(width, CGFloat(track.duration / (60.0 / track.estimatedBPM)) * 50 * zoom), height: 200)
                
                // Beat markers
                ForEach(track.beatMarkers) { marker in
                    BeatMarkerView(marker: marker, zoom: zoom)
                        .position(x: CGFloat(marker.beat) * 50 * zoom, y: 100)
                }
                
                // Playhead
                if timingSystem.isPlaying {
                    Rectangle()
                        .fill(DesignSystem.Colors.accentDanger)
                        .frame(width: 2)
                        .position(x: CGFloat(timingSystem.currentBeat) * 50 * zoom + (timingSystem.beatPhase * 50 * zoom), y: 100)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundTertiary.opacity(0.3))
    }
}

// MARK: - Beat Marker View
struct BeatMarkerView: View {
    @ObservedObject var marker: BeatMarker
    let zoom: CGFloat
    
    var body: some View {
        VStack(spacing: 4) {
            Image(systemName: marker.type.icon)
                .font(.system(size: 18 * zoom))
                .foregroundColor(color(for: marker.type))
            
            Text(marker.type.rawValue)
                .font(DesignSystem.Typography.micro)
                .lineLimit(1)
        }
        .opacity(marker.intensity)
    }
    
    private func color(for type: BeatMarkerType) -> Color {
        switch type {
        case .downbeat: return DesignSystem.Colors.accentDanger
        case .beat: return DesignSystem.Colors.accentPrimary
        case .offbeat: return DesignSystem.Colors.textSecondary
        case .accent: return DesignSystem.Colors.accentWarning
        case .step: return DesignSystem.Colors.accentSuccess
        case .jump: return .purple
        case .crouch: return .orange
        }
    }
}

// MARK: - Character Action Panel
struct CharacterActionPanel: View {
    @ObservedObject var timingSystem = AudioTimingSystem.shared
    @State private var selectedAction: CharacterAction = .walk
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Character Actions")
                .font(DesignSystem.Typography.headline)
            
            Text("Add action triggers to specific beats:")
                .font(DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textSecondary)
            
            // Action selector
            HStack {
                Picker("Action", selection: $selectedAction) {
                    ForEach(CharacterAction.allCases, id: \.self) { action in
                        HStack {
                            Image(systemName: action.icon)
                            Text(action.rawValue)
                        }
                        .tag(action)
                    }
                }
                .pickerStyle(.menu)
                
                Button("Add at Current Beat") {
                    timingSystem.addActionTrigger(
                        ActionTrigger(beat: timingSystem.currentBeat, actionType: selectedAction)
                    )
                }
            }
            
            Divider()
            
            // Trigger list
            ScrollView {
                LazyVStack(spacing: 4) {
                    ForEach(timingSystem.actionTriggers) { trigger in
                        HStack {
                            Image(systemName: trigger.actionType.icon)
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                            Text("Beat \(trigger.beat): \(trigger.actionType.rawValue)")
                                .font(DesignSystem.Typography.body)
                            Spacer()
                            Button(action: {
                                timingSystem.actionTriggers.removeAll { $0.id == trigger.id }
                            }) {
                                Image(systemName: "trash")
                                    .foregroundColor(DesignSystem.Colors.accentDanger)
                            }
                            .buttonStyle(.plain)
                        }
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundSecondary)
                        .cornerRadius(6)
                    }
                }
            }
        }
        .padding(16)
    }
}
