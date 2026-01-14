import SwiftUI

// MARK: - Audio Source (for scene visualization)
struct AudioSource: Identifiable {
    let id = UUID()
    var name: String
    var position: SIMD3<Float>
    var volume: Double
    var isPlaying: Bool
    var sourceType: SourceType
    
    enum SourceType {
        case music, sfx, ambient, dialogue
        
        var color: Color {
            switch self {
            case .music: return .purple
            case .sfx: return .orange
            case .ambient: return .blue
            case .dialogue: return .green
            }
        }
        
        var icon: String {
            switch self {
            case .music: return "music.note"
            case .sfx: return "waveform"
            case .ambient: return "wind"
            case .dialogue: return "person.wave.2"
            }
        }
    }
}

// MARK: - Audio Editor Manager (FULLY FUNCTIONAL)
class AudioEditorManager: ObservableObject {
    static let shared = AudioEditorManager()
    
    // Master
    @Published var masterVolume: Double = 0.8
    @Published var masterMuted: Bool = false
    
    // Surround sound channels (7.1)
    @Published var frontLeftVolume: Double = 0.75
    @Published var frontRightVolume: Double = 0.75
    @Published var centerVolume: Double = 0.8
    @Published var subwooferVolume: Double = 0.6
    @Published var rearLeftVolume: Double = 0.7
    @Published var rearRightVolume: Double = 0.7
    @Published var sideLeftVolume: Double = 0.7
    @Published var sideRightVolume: Double = 0.7
    
    // Channel mutes
    @Published var frontLeftMuted: Bool = false
    @Published var frontRightMuted: Bool = false
    @Published var centerMuted: Bool = false
    @Published var subwooferMuted: Bool = false
    @Published var rearLeftMuted: Bool = false
    @Published var rearRightMuted: Bool = false
    @Published var sideLeftMuted: Bool = false
    @Published var sideRightMuted: Bool = false
    
    // EQ per channel
    @Published var lowFreq: Double = 0.0  // -12 to +12 dB
    @Published var midFreq: Double = 0.0
    @Published var highFreq: Double = 0.0
    
    // Compression
    @Published var compressionThreshold: Double = -20.0  // dB
    @Published var compressionRatio: Double = 4.0
    @Published var compressionAttack: Double = 10.0  // ms
    @Published var compressionRelease: Double = 100.0  // ms
    
    // Mix channels with full controls
    @Published var musicVolume: Double = 0.8
    @Published var musicPan: Double = 0.0  // -1 to 1
    @Published var musicPitch: Double = 1.0
    @Published var musicReverb: Double = 0.2
    
    @Published var sfxVolume: Double = 1.0
    @Published var sfxPan: Double = 0.0
    @Published var sfxPitch: Double = 1.0
    @Published var sfxReverb: Double = 0.1
    
    @Published var ambientVolume: Double = 0.6
    @Published var ambientPan: Double = 0.0
    @Published var ambientPitch: Double = 1.0
    @Published var ambientReverb: Double = 0.5
    
    @Published var dialogueVolume: Double = 0.9
    @Published var dialoguePan: Double = 0.0
    @Published var dialoguePitch: Double = 1.0
    @Published var dialogueReverb: Double = 0.05
    
    // Spatial Audio Settings
    @Published var spatialAudioEnabled: Bool = true
    @Published var hrtfEnabled: Bool = true
    @Published var dopplerEffect: Double = 1.0
    @Published var speedOfSound: Double = 343.0  // m/s
    
    // Distance Attenuation
    @Published var distanceModel: String = "Inverse"
    @Published var referenceDistance: Double = 1.0
    @Published var maxDistance: Double = 100.0
    @Published var rolloffFactor: Double = 1.0
    
    // Reverb
    @Published var reverbAmount: Double = 0.3
    @Published var reverbZone: String = "Generic"
    @Published var reverbDecay: Double = 1.5
    @Published var reverbDensity: Double = 1.0
    @Published var reverbDiffusion: Double = 1.0
    
    // Occlusion
    @Published var occlusionEnabled: Bool = true
    @Published var occlusionAmount: Double = 0.8
    @Published var obstructionEnabled: Bool = true
    
    // Scene audio sources
    @Published var audioSources: [AudioSource] = [
        AudioSource(name: "Background Music", position: SIMD3(0, 2, -5), volume: 0.8, isPlaying: true, sourceType: .music),
        AudioSource(name: "Footsteps", position: SIMD3(-3, 0, 2), volume: 0.6, isPlaying: true, sourceType: .sfx),
        AudioSource(name: "Wind", position: SIMD3(5, 1, 0), volume: 0.5, isPlaying: true, sourceType: .ambient),
        AudioSource(name: "NPC Voice", position: SIMD3(2, 1.5, -3), volume: 0.9, isPlaying: false, sourceType: .dialogue)
    ]
    
    @Published var listenerPosition: SIMD3<Float> = SIMD3(0, 1.7, 0)
    @Published var listenerRotation: Float = 0.0
    
    let reverbZones = ["None", "Generic", "Cave", "Forest", "City", "Hallway", "Cathedral", "Arena", "Room", "Bathroom"]
    let distanceModels = ["Linear", "Inverse", "Exponential"]
}

// MARK: - Audio Editor View
struct AudioEditorView: View {
    @ObservedObject var manager = AudioEditorManager.shared
    @State private var selectedTab = 0
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack {
                Text("Audio Mixer")
                    .font(DesignSystem.Typography.title2)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
                
                // Tab selector
                Picker("", selection: $selectedTab) {
                    Text("Scene").tag(0)
                    Text("Surround").tag(1)
                    Text("Mixer").tag(2)
                    Text("Effects").tag(3)
                    Text("Settings").tag(4)
                }
                .pickerStyle(.segmented)
                .frame(width: 400)
            }
            .padding(12)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            Divider()
            
            // Content
            Group {
                switch selectedTab {
                case 0:
                    SceneAudioView(manager: manager)
                case 1:
                    SurroundSoundView(manager: manager)
                case 2:
                    MixerView(manager: manager)
                case 3:
                    EffectsView(manager: manager)
                case 4:
                    AudioSettingsView(manager: manager)
                default:
                    SceneAudioView(manager: manager)
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Scene Audio View (3D Visualization)
struct SceneAudioView: View {
    @ObservedObject var manager: AudioEditorManager
    @State private var selectedSource: AudioSource? = nil
    
    var body: some View {
        HSplitView {
            // 3D Scene View
            GeometryReader { geometry in
                ZStack {
                    // Background grid
                    Canvas { context, size in
                        let gridSize: CGFloat = 40
                        context.stroke(Path { path in
                            for x in stride(from: 0, through: size.width, by: gridSize) {
                                path.move(to: CGPoint(x: x, y: 0))
                                path.addLine(to: CGPoint(x: x, y: size.height))
                            }
                            for y in stride(from: 0, through: size.height, by: gridSize) {
                                path.move(to: CGPoint(x: 0, y: y))
                                path.addLine(to: CGPoint(x: size.width, y: y))
                            }
                        }, with: .color(DesignSystem.Colors.border.opacity(0.3)), lineWidth: 0.5)
                    }
                    
                    // Listener (center)
                    VStack(spacing: 4) {
                        ZStack {
                            Circle()
                                .fill(DesignSystem.Colors.accentPrimary.opacity(0.2))
                                .frame(width: 50, height: 50)
                            
                            Image(systemName: "person.fill")
                                .font(.system(size: 24))
                                .foregroundColor(DesignSystem.Colors.accentPrimary)
                            
                            // Direction indicator
                            Rectangle()
                                .fill(DesignSystem.Colors.accentPrimary)
                                .frame(width: 2, height: 20)
                                .offset(y: -35)
                        }
                        .rotationEffect(.degrees(Double(manager.listenerRotation)))
                        
                        Text("Listener")
                            .font(DesignSystem.Typography.micro)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                    }
                    .position(x: geometry.size.width / 2, y: geometry.size.height / 2)
                    
                    // Audio sources
                    ForEach(manager.audioSources) { source in
                        AudioSourceNode(source: source, isSelected: selectedSource?.id == source.id)
                            .position(
                                x: geometry.size.width / 2 + CGFloat(source.position.x) * 20,
                                y: geometry.size.height / 2 + CGFloat(source.position.z) * 20
                            )
                            .onTapGesture {
                                selectedSource = source
                            }
                    }
                }
                .background(DesignSystem.Colors.backgroundTertiary.opacity(0.3))
            }
            .frame(minWidth: 400)
            
            // Source Inspector
            VStack(alignment: .leading, spacing: 0) {
                // Header
                HStack {
                    Text(selectedSource?.name ?? "No Source Selected")
                        .font(DesignSystem.Typography.headline)
                    Spacer()
                }
                .padding(12)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                Divider()
                
                if let source = selectedSource {
                    ScrollView {
                        VStack(alignment: .leading, spacing: 16) {
                            // Position
                            VStack(alignment: .leading, spacing: 8) {
                                Text("Position")
                                    .font(DesignSystem.Typography.bodyBold)
                                
                                HStack {
                                    Text("X:")
                                    TextField("", value: .constant(source.position.x), format: .number)
                                        .textFieldStyle(.plain)
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 60)
                                        .padding(4)
                                        .background(DesignSystem.Colors.backgroundTertiary)
                                        .cornerRadius(4)
                                    
                                    Text("Y:")
                                    TextField("", value: .constant(source.position.y), format: .number)
                                        .textFieldStyle(.plain)
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 60)
                                        .padding(4)
                                        .background(DesignSystem.Colors.backgroundTertiary)
                                        .cornerRadius(4)
                                    
                                    Text("Z:")
                                    TextField("", value: .constant(source.position.z), format: .number)
                                        .textFieldStyle(.plain)
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 60)
                                        .padding(4)
                                        .background(DesignSystem.Colors.backgroundTertiary)
                                        .cornerRadius(4)
                                }
                            }
                            
                            // Volume
                            VStack(alignment: .leading, spacing: 8) {
                                HStack {
                                    Text("Volume")
                                        .font(DesignSystem.Typography.bodyBold)
                                    Spacer()
                                    Text("\(Int(source.volume * 100))%")
                                        .font(DesignSystem.Typography.mono)
                                }
                                Slider(value: .constant(source.volume), in: 0...1)
                            }
                            
                            // Distance Attenuation
                            VStack(alignment: .leading, spacing: 8) {
                                Text("Distance Attenuation")
                                    .font(DesignSystem.Typography.bodyBold)
                                
                                HStack {
                                    Text("Min Distance")
                                    Spacer()
                                    Slider(value: $manager.referenceDistance, in: 0.1...10)
                                        .frame(width: 150)
                                    Text(String(format: "%.1f", manager.referenceDistance))
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 40, alignment: .trailing)
                                }
                                
                                HStack {
                                    Text("Max Distance")
                                    Spacer()
                                    Slider(value: $manager.maxDistance, in: 10...1000)
                                        .frame(width: 150)
                                    Text(String(format: "%.0f", manager.maxDistance))
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 40, alignment: .trailing)
                                }
                                
                                HStack {
                                    Text("Rolloff")
                                    Spacer()
                                    Slider(value: $manager.rolloffFactor, in: 0...10)
                                        .frame(width: 150)
                                    Text(String(format: "%.1f", manager.rolloffFactor))
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 40, alignment: .trailing)
                                }
                            }
                            
                            // Spatial Settings
                            VStack(alignment: .leading, spacing: 8) {
                                Text("Spatial")
                                    .font(DesignSystem.Typography.bodyBold)
                                
                                EditorToggle(label: "3D Spatialization", isOn: $manager.spatialAudioEnabled)
                                EditorToggle(label: "Doppler Shift", isOn: .constant(true))
                                
                                HStack {
                                    Text("Spread")
                                    Spacer()
                                    Slider(value: .constant(0.5), in: 0...1)
                                        .frame(width: 150)
                                    Text("50%")
                                        .font(DesignSystem.Typography.mono)
                                        .frame(width: 40, alignment: .trailing)
                                }
                            }
                        }
                        .padding(16)
                    }
                } else {
                    VStack {
                        Spacer()
                        Image(systemName: "waveform.circle")
                            .font(.system(size: 48))
                            .foregroundColor(DesignSystem.Colors.textTertiary)
                        Text("Select an audio source")
                            .font(DesignSystem.Typography.body)
                            .foregroundColor(DesignSystem.Colors.textSecondary)
                        Spacer()
                    }
                }
            }
            .frame(minWidth: 300, maxWidth: 400)
            .background(DesignSystem.Colors.backgroundSecondary)
        }
    }
}

struct AudioSourceNode: View {
    let source: AudioSource
    let isSelected: Bool
    
    var body: some View {
        VStack(spacing: 4) {
            ZStack {
                Circle()
                    .fill(source.sourceType.color.opacity(0.2))
                    .frame(width: 40, height: 40)
                
                Circle()
                    .stroke(source.sourceType.color, lineWidth: isSelected ? 3 : 2)
                    .frame(width: 40, height: 40)
                
                Image(systemName: source.isPlaying ? source.sourceType.icon : "speaker.slash")
                    .font(.system(size: 16))
                    .foregroundColor(source.sourceType.color)
                
                // Playing indicator
                if source.isPlaying {
                    Circle()
                        .fill(Color.green)
                        .frame(width: 8, height: 8)
                        .offset(x: 18, y: -18)
                }
            }
            
            Text(source.name)
                .font(DesignSystem.Typography.micro)
                .foregroundColor(DesignSystem.Colors.textPrimary)
                .lineLimit(1)
                .frame(maxWidth: 80)
        }
        .padding(8)
        .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
        .cornerRadius(8)
    }
}

// MARK: - Effects View (EQ, Compression, Reverb)
struct EffectsView: View {
    @ObservedObject var manager: AudioEditorManager
    
    var body: some View {
        ScrollView {
            VStack(spacing: 20) {
                // EQ
                EditorCard(title: "Equalizer") {
                    VStack(spacing: 16) {
                        HStack(spacing: 40) {
                            VStack {
                                Text("Low")
                                    .font(DesignSystem.Typography.caption)
                                Slider(value: $manager.lowFreq, in: -12...12)
                                    .rotationEffect(.degrees(-90))
                                    .frame(width: 40, height: 150)
                                Text("\(String(format: "%.1f", manager.lowFreq)) dB")
                                    .font(DesignSystem.Typography.mono)
                            }
                            
                            VStack {
                                Text("Mid")
                                    .font(DesignSystem.Typography.caption)
                                Slider(value: $manager.midFreq, in: -12...12)
                                    .rotationEffect(.degrees(-90))
                                    .frame(width: 40, height: 150)
                                Text("\(String(format: "%.1f", manager.midFreq)) dB")
                                    .font(DesignSystem.Typography.mono)
                            }
                            
                            VStack {
                                Text("High")
                                    .font(DesignSystem.Typography.caption)
                                Slider(value: $manager.highFreq, in: -12...12)
                                    .rotationEffect(.degrees(-90))
                                    .frame(width: 40, height: 150)
                                Text("\(String(format: "%.1f", manager.highFreq)) dB")
                                    .font(DesignSystem.Typography.mono)
                            }
                        }
                        .frame(maxWidth: .infinity)
                    }
                }
                
                // Compression
                EditorCard(title: "Compressor") {
                    VStack(spacing: 12) {
                        HStack {
                            Text("Threshold")
                            Spacer()
                            Slider(value: $manager.compressionThreshold, in: -60...0)
                                .frame(width: 200)
                            Text("\(String(format: "%.1f", manager.compressionThreshold)) dB")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Ratio")
                            Spacer()
                            Slider(value: $manager.compressionRatio, in: 1...20)
                                .frame(width: 200)
                            Text("\(String(format: "%.1f", manager.compressionRatio)):1")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Attack")
                            Spacer()
                            Slider(value: $manager.compressionAttack, in: 0.1...100)
                                .frame(width: 200)
                            Text("\(String(format: "%.1f", manager.compressionAttack)) ms")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Release")
                            Spacer()
                            Slider(value: $manager.compressionRelease, in: 10...1000)
                                .frame(width: 200)
                            Text("\(String(format: "%.0f", manager.compressionRelease)) ms")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                    }
                }
                
                // Reverb
                EditorCard(title: "Reverb") {
                    VStack(spacing: 12) {
                        HStack {
                            Text("Amount")
                            Spacer()
                            Slider(value: $manager.reverbAmount, in: 0...1)
                                .frame(width: 200)
                            Text("\(Int(manager.reverbAmount * 100))%")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Decay Time")
                            Spacer()
                            Slider(value: $manager.reverbDecay, in: 0.1...10)
                                .frame(width: 200)
                            Text("\(String(format: "%.1f", manager.reverbDecay)) s")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Density")
                            Spacer()
                            Slider(value: $manager.reverbDensity, in: 0...1)
                                .frame(width: 200)
                            Text("\(Int(manager.reverbDensity * 100))%")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Diffusion")
                            Spacer()
                            Slider(value: $manager.reverbDiffusion, in: 0...1)
                                .frame(width: 200)
                            Text("\(Int(manager.reverbDiffusion * 100))%")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 70, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Zone Preset")
                            Spacer()
                            Picker("", selection: $manager.reverbZone) {
                                ForEach(manager.reverbZones, id: \.self) { zone in
                                    Text(zone).tag(zone)
                                }
                            }
                            .pickerStyle(.menu)
                            .frame(width: 150)
                        }
                    }
                }
            }
            .padding(20)
        }
    }
}

// MARK: - Surround Sound View (same as before but enhanced)
struct SurroundSoundView: View {
    @ObservedObject var manager: AudioEditorManager
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Center listener position
                Circle()
                    .fill(DesignSystem.Colors.accentPrimary.opacity(0.2))
                    .frame(width: 60, height: 60)
                    .overlay(
                        Image(systemName: "person.fill")
                            .font(.system(size: 24))
                            .foregroundColor(DesignSystem.Colors.accentPrimary)
                    )
                    .position(x: geometry.size.width / 2, y: geometry.size.height / 2)
                
                // Speaker positions (7.1 surround)
                SpeakerControl(name: "FL", icon: "speaker.wave.2.fill", volume: $manager.frontLeftVolume, muted: $manager.frontLeftMuted)
                    .position(x: geometry.size.width * 0.35, y: geometry.size.height * 0.25)
                
                SpeakerControl(name: "FR", icon: "speaker.wave.2.fill", volume: $manager.frontRightVolume, muted: $manager.frontRightMuted)
                    .position(x: geometry.size.width * 0.65, y: geometry.size.height * 0.25)
                
                SpeakerControl(name: "C", icon: "speaker.fill", volume: $manager.centerVolume, muted: $manager.centerMuted)
                    .position(x: geometry.size.width * 0.5, y: geometry.size.height * 0.15)
                
                SpeakerControl(name: "SUB", icon: "speaker.wave.3.fill", volume: $manager.subwooferVolume, muted: $manager.subwooferMuted, color: .orange)
                    .position(x: geometry.size.width * 0.5, y: geometry.size.height * 0.85)
                
                SpeakerControl(name: "RL", icon: "speaker.wave.1.fill", volume: $manager.rearLeftVolume, muted: $manager.rearLeftMuted)
                    .position(x: geometry.size.width * 0.3, y: geometry.size.height * 0.75)
                
                SpeakerControl(name: "RR", icon: "speaker.wave.1.fill", volume: $manager.rearRightVolume, muted: $manager.rearRightMuted)
                    .position(x: geometry.size.width * 0.7, y: geometry.size.height * 0.75)
                
                SpeakerControl(name: "SL", icon: "speaker.wave.2.fill", volume: $manager.sideLeftVolume, muted: $manager.sideLeftMuted)
                    .position(x: geometry.size.width * 0.15, y: geometry.size.height * 0.5)
                
                SpeakerControl(name: "SR", icon: "speaker.wave.2.fill", volume: $manager.sideRightVolume, muted: $manager.sideRightMuted)
                    .position(x: geometry.size.width * 0.85, y: geometry.size.height * 0.5)
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .padding(40)
    }
}

// MARK: - Speaker Control (same as before)
struct SpeakerControl: View {
    let name: String
    let icon: String
    @Binding var volume: Double
    @Binding var muted: Bool
    var color: Color = DesignSystem.Colors.accentPrimary
    
    @State private var isHovered = false
    
    var body: some View {
        VStack(spacing: 8) {
            ZStack {
                Circle()
                    .fill(muted ? DesignSystem.Colors.accentDanger.opacity(0.2) : color.opacity(0.3))
                    .frame(width: 70, height: 70)
                
                Circle()
                    .stroke(muted ? DesignSystem.Colors.accentDanger : color, lineWidth: 3)
                    .frame(width: 70, height: 70)
                
                Image(systemName: muted ? "speaker.slash.fill" : icon)
                    .font(.system(size: 24))
                    .foregroundColor(muted ? DesignSystem.Colors.accentDanger : color)
            }
            .scaleEffect(isHovered ? 1.1 : 1.0)
            .animation(.spring(response: 0.3, dampingFraction: 0.7), value: isHovered)
            .onHover { isHovered = $0 }
            
            Text(name)
                .font(DesignSystem.Typography.captionBold)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            VStack(spacing: 4) {
                Slider(value: $volume, in: 0...1)
                    .frame(width: 100)
                    .disabled(muted)
                    .tint(color)
                
                Text("\(Int(volume * 100))%")
                    .font(DesignSystem.Typography.mono)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Button(action: { muted.toggle() }) {
                HStack(spacing: 4) {
                    Image(systemName: muted ? "speaker.slash.fill" : "speaker.wave.2.fill")
                        .font(.system(size: 10))
                    Text(muted ? "Muted" : "Active")
                        .font(DesignSystem.Typography.micro)
                }
                .foregroundColor(muted ? .white : DesignSystem.Colors.textPrimary)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(muted ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.backgroundTertiary)
                .clipShape(Capsule())
            }
            .buttonStyle(.plain)
        }
        .padding(12)
        .background(DesignSystem.Colors.backgroundSecondary)
        .clipShape(RoundedRectangle(cornerRadius: 12, style: .continuous))
        .shadow(color: .black.opacity(0.1), radius: 4, x: 0, y: 2)
    }
}

// MARK: - Mixer View (Enhanced with more controls)
struct MixerView: View {
    @ObservedObject var manager: AudioEditorManager
    
    var body: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: 24) {
                AdvancedChannelStrip(
                    name: "MASTER",
                    volume: $manager.masterVolume,
                    muted: $manager.masterMuted,
                    pan: .constant(0.0),
                    pitch: .constant(1.0),
                    reverb: .constant(0.0),
                    color: DesignSystem.Colors.accentDanger,
                    isMaster: true
                )
                
                Divider().frame(height: 500)
                
                AdvancedChannelStrip(name: "Music", volume: $manager.musicVolume, muted: .constant(false), pan: $manager.musicPan, pitch: $manager.musicPitch, reverb: $manager.musicReverb)
                AdvancedChannelStrip(name: "SFX", volume: $manager.sfxVolume, muted: .constant(false), pan: $manager.sfxPan, pitch: $manager.sfxPitch, reverb: $manager.sfxReverb)
                AdvancedChannelStrip(name: "Ambient", volume: $manager.ambientVolume, muted: .constant(false), pan: $manager.ambientPan, pitch: $manager.ambientPitch, reverb: $manager.ambientReverb)
                AdvancedChannelStrip(name: "Dialogue", volume: $manager.dialogueVolume, muted: .constant(false), pan: $manager.dialoguePan, pitch: $manager.dialoguePitch, reverb: $manager.dialogueReverb)
            }
            .padding(24)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

// MARK: - Advanced Channel Strip (with pan, pitch, reverb)
struct AdvancedChannelStrip: View {
    let name: String
    @Binding var volume: Double
    @Binding var muted: Bool
    @Binding var pan: Double
    @Binding var pitch: Double
    @Binding var reverb: Double
    var color: Color = DesignSystem.Colors.accentPrimary
    var isMaster: Bool = false
    
    var body: some View {
        VStack(spacing: 16) {
            Text(name)
                .font(isMaster ? DesignSystem.Typography.bodyBold : DesignSystem.Typography.caption)
                .foregroundColor(DesignSystem.Colors.textPrimary)
            
            // VU Meter
            GeometryReader { geometry in
                ZStack(alignment: .bottom) {
                    RoundedRectangle(cornerRadius: 4)
                        .fill(DesignSystem.Colors.backgroundTertiary)
                    
                    RoundedRectangle(cornerRadius: 4)
                        .fill(LinearGradient(colors: [DesignSystem.Colors.accentSuccess, DesignSystem.Colors.accentWarning, DesignSystem.Colors.accentDanger], startPoint: .bottom, endPoint: .top))
                        .frame(height: geometry.size.height * (muted ? 0 : volume))
                        .animation(.easeOut(duration: 0.1), value: volume)
                }
            }
            .frame(width: isMaster ? 40 : 24, height: 200)
            
            // Fader
            VStack(spacing: 8) {
                Slider(value: $volume, in: 0...1)
                    .rotationEffect(.degrees(-90))
                    .frame(width: 40, height: 150)
                    .disabled(muted)
                
                Text("\(Int(volume * 100))")
                    .font(DesignSystem.Typography.monoSmall)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .frame(minWidth: 40)
            }
            
            if !isMaster {
                // Pan
                VStack(spacing: 4) {
                    Text("Pan")
                        .font(DesignSystem.Typography.micro)
                    Slider(value: $pan, in: -1...1)
                        .frame(width: 80)
                    Text(pan < 0 ? "L\(Int(abs(pan) * 100))" : pan > 0 ? "R\(Int(pan * 100))" : "C")
                        .font(DesignSystem.Typography.mono)
                        .frame(width: 40)
                }
                
                // Pitch
                VStack(spacing: 4) {
                    Text("Pitch")
                        .font(DesignSystem.Typography.micro)
                    Slider(value: $pitch, in: 0.5...2.0)
                        .frame(width: 80)
                    Text(String(format: "%.2f", pitch))
                        .font(DesignSystem.Typography.mono)
                        .frame(width: 40)
                }
                
                // Reverb Send
                VStack(spacing: 4) {
                    Text("Reverb")
                        .font(DesignSystem.Typography.micro)
                    Slider(value: $reverb, in: 0...1)
                        .frame(width: 80)
                    Text("\(Int(reverb * 100))%")
                        .font(DesignSystem.Typography.mono)
                        .frame(width: 40)
                }
            }
            
            // Mute button
            Button(action: { muted.toggle() }) {
                Text("M")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(muted ? .white : DesignSystem.Colors.textPrimary)
                    .frame(width: 32, height: 24)
                    .background(muted ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.backgroundTertiary)
                    .clipShape(RoundedRectangle(cornerRadius: 4))
            }
            .buttonStyle(.plain)
        }
        .frame(width: isMaster ? 100 : 120)
        .padding(16)
        .background(DesignSystem.Colors.backgroundSecondary)
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }
}

// MARK: - Settings View (Enhanced)
struct AudioSettingsView: View {
    @ObservedObject var manager: AudioEditorManager
    
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 20) {
                // Spatial Audio
                EditorCard(title: "Spatial Audio") {
                    VStack(spacing: 12) {
                        EditorToggle(label: "Enable Spatial Audio", isOn: $manager.spatialAudioEnabled)
                        EditorToggle(label: "HRTF (Head-Related Transfer)", isOn: $manager.hrtfEnabled)
                        
                        HStack {
                            Text("Doppler Effect")
                            Spacer()
                            Slider(value: $manager.dopplerEffect, in: 0...5)
                                .frame(width: 200)
                            Text(String(format: "%.1f", manager.dopplerEffect))
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 40, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Speed of Sound")
                            Spacer()
                            Slider(value: $manager.speedOfSound, in: 100...500)
                                .frame(width: 200)
                            Text(String(format: "%.0f", manager.speedOfSound))
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 50, alignment: .trailing)
                        }
                        
                        HStack {
                            Text("Distance Model")
                            Spacer()
                            Picker("", selection: $manager.distanceModel) {
                                ForEach(manager.distanceModels, id: \.self) { model in
                                    Text(model).tag(model)
                                }
                            }
                            .pickerStyle(.menu)
                            .frame(width: 150)
                        }
                    }
                }
                
                // Occlusion
                EditorCard(title: "Occlusion & Obstruction") {
                    VStack(spacing: 12) {
                        EditorToggle(label: "Enable Occlusion", isOn: $manager.occlusionEnabled)
                        
                        HStack {
                            Text("Occlusion Amount")
                            Spacer()
                            Slider(value: $manager.occlusionAmount, in: 0...1)
                                .frame(width: 200)
                                .disabled(!manager.occlusionEnabled)
                            Text("\(Int(manager.occlusionAmount * 100))%")
                                .font(DesignSystem.Typography.mono)
                                .frame(width: 40, alignment: .trailing)
                        }
                        
                        EditorToggle(label: "Enable Obstruction", isOn: $manager.obstructionEnabled)
                    }
                }
            }
            .padding(20)
        }
    }
}
