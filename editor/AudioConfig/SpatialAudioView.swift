import SwiftUI

enum AudioFormat: String, CaseIterable {
    case dolbyAtmos = "Dolby Atmos"
    case dtsX = "DTS:X"
    case customSurround = "Custom Surround"
}

struct SpatialAudioView: View {
    @State private var selectedFormat: AudioFormat = .dolbyAtmos
    @State private var masterVolume: Double = 0.75
    @State private var speakerLevels: [String: Double] = [:]
    @StateObject private var audioEngine = SpatialAudioEngine()
    
    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Format Selector
                FormatSelectorView(selectedFormat: $selectedFormat)
                    .padding()
                    .background(Color(.windowBackgroundColor))
                
                Divider()
                
                HStack(spacing: 20) {
                    // Left Panel - Controls
                    VStack(alignment: .leading, spacing: 20) {
                        // Master Volume
                        VStack(alignment: .leading) {
                            Text("Master Volume")
                                .font(.headline)
                            
                            HStack {
                                Slider(value: $masterVolume, in: 0...1)
                                Text("\(Int(masterVolume * 100))%")
                                    .frame(width: 50)
                                    .monospacedDigit()
                            }
                        }
                        
                        Divider()
                        
                        // Format-specific controls
                        formatSpecificControls
                        
                        Spacer()
                    }
                    .frame(width: 300)
                    .padding()
                    .background(Color(.controlBackgroundColor))
                    
                    // Center - Speaker Visualization
                    VStack {
                        SpeakerVisualizerView(
                            format: selectedFormat,
                            speakerLevels: audioEngine.speakerLevels
                        )
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                        
                        // Audio Meters
                        AudioMetersGridView(
                            speakers: selectedFormat.speakers,
                            levels: audioEngine.speakerLevels
                        )
                        .frame(height: 150)
                        .padding(.horizontal)
                    }
                }
            }
            .navigationTitle("Spatial Audio Configuration")
            .toolbar {
                ToolbarItem(placement: .primaryAction) {
                    Button("Test Audio") {
                        audioEngine.playTestTone()
                    }
                }
            }
        }
        .frame(minWidth: 900, minHeight: 600)
        .onChange(of: selectedFormat) { newFormat in
            audioEngine.setFormat(newFormat)
        }
    }
    
    @ViewBuilder
    private var formatSpecificControls: some View {
        switch selectedFormat {
        case .dolbyAtmos:
            AtmosControlsView()
        case .dtsX:
            DTSXControlsView()
        case .customSurround:
            CustomSurroundControlsView()
        }
    }
}

struct FormatSelectorView: View {
    @Binding var selectedFormat: AudioFormat
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Audio Format")
                .font(.headline)
            
            Picker("", selection: $selectedFormat) {
                ForEach(AudioFormat.allCases, id: \.self) { format in
                    HStack {
                        Image(systemName: iconForFormat(format))
                        Text(format.rawValue)
                    }
                    .tag(format)
                }
            }
            .pickerStyle(.segmented)
            
            Text(descriptionForFormat(selectedFormat))
                .font(.caption)
                .foregroundColor(.secondary)
        }
    }
    
    private func iconForFormat(_ format: AudioFormat) -> String {
        switch format {
        case .dolbyAtmos: return "waveform.circle.fill"
        case .dtsX: return "hifispeaker.2.fill"
        case .customSurround: return "slider.horizontal.3"
        }
    }
    
    private func descriptionForFormat(_ format: AudioFormat) -> String {
        switch format {
        case .dolbyAtmos:
            return "Object-based immersive audio with height channels"
        case .dtsX:
            return "Flexible object-based surround with Neural:X upmixing"
        case .customSurround:
            return "User-configurable speaker layout with VBAP panning"
        }
    }
}

extension AudioFormat {
    var speakers: [SpeakerPosition] {
        switch self {
        case .dolbyAtmos:
            return SpeakerPosition.atmos714Layout
        case .dtsX:
            return SpeakerPosition.dtsx71Layout
        case .customSurround:
            return SpeakerPosition.custom51Layout
        }
    }
}

#Preview {
    SpatialAudioView()
}
