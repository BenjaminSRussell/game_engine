import SwiftUI

struct AtmosControlsView: View {
    @State private var binauralMode = false
    @State private var objectCount = 32
    @State private var heightChannelGain: Double = 1.0
    @State private var overheadSpeakerAngle: Double = 45
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Dolby Atmos Settings")
                .font(.headline)
            
            // Binaural mode toggle
            Toggle("Binaural Rendering (Headphones)", isOn: $binauralMode)
                .help("Enable HRTF-based binaural rendering for headphones")
            
            if !binauralMode {
                // Speaker configuration
                VStack(alignment: .leading, spacing: 8) {
                    Text("Height Channel Gain")
                        .font(.subheadline)
                    
                    HStack {
                        Slider(value: $heightChannelGain, in: 0...2)
                        Text("\(Int(heightChannelGain * 100))%")
                            .frame(width: 50)
                            .monospacedDigit()
                    }
                }
                
                VStack(alignment: .leading, spacing: 8) {
                    Text("Overhead Speaker Angle")
                        .font(.subheadline)
                    
                    HStack {
                        Slider(value: $overheadSpeakerAngle, in: 30...60)
                        Text("\(Int(overheadSpeakerAngle))°")
                            .frame(width: 50)
                            .monospacedDigit()
                    }
                }
            }
            
            Divider()
            
            // Object count
            VStack(alignment: .leading, spacing: 8) {
                Text("Max Audio Objects")
                    .font(.subheadline)
                
                Stepper(value: $objectCount, in: 8...128, step: 8) {
                    Text("\(objectCount) objects")
                        .monospacedDigit()
                }
            }
            
            // Preset layouts
            VStack(alignment: .leading, spacing: 8) {
                Text("Channel Layout")
                    .font(.subheadline)
                
                VStack(spacing: 6) {
                    LayoutButton(title: "7.1.4", description: "7 base + 4 height") {
                        // Apply 7.1.4 layout
                    }
                    LayoutButton(title: "5.1.2", description: "5 base + 2 height") {
                        // Apply 5.1.2 layout
                    }
                    LayoutButton(title: "9.1.6", description: "9 base + 6 height") {
                        // Apply 9.1.6 layout
                    }
                }
            }
        }
    }
}

struct DTSXControlsView: View {
    @State private var neuralXEnabled = true
    @State private var objectPriority: Double = 0.7
    @State private var dialogEnhancement: Double = 0
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("DTS:X Settings")
                .font(.headline)
            
            Toggle("Neural:X Upmixing", isOn: $neuralXEnabled)
                .help("Enable intelligent upmixing for non-DTS:X content")
            
            VStack(alignment: .leading, spacing: 8) {
                Text("Object Rendering Priority")
                    .font(.subheadline)
                
                HStack {
                    Slider(value: $objectPriority, in: 0...1)
                    Text("\(Int(objectPriority * 100))%")
                        .frame(width: 50)
                        .monospacedDigit()
                }
                .help("Balance between object precision and channel compatibility")
            }
            
            VStack(alignment: .leading, spacing: 8) {
                Text("Dialog Enhancement")
                    .font(.subheadline)
                
                HStack {
                    Slider(value: $dialogEnhancement, in: 0...1)
                    Text("\(Int(dialogEnhancement * 100))%")
                        .frame(width: 50)
                        .monospacedDigit()
                }
            }
            
            Divider()
            
            VStack(alignment: .leading, spacing: 8) {
                Text("Speaker Configuration")
                    .font(.subheadline)
                
                VStack(spacing: 6) {
                    LayoutButton(title: "7.1", description: "Standard surround") {
                        // Apply 7.1 layout
                    }
                    LayoutButton(title: "5.1", description: "Traditional surround") {
                        // Apply 5.1 layout
                    }
                }
            }
        }
    }
}

struct CustomSurroundControlsView: View {
    @State private var vbapSpread: Double = 15
    @State private var distanceAttenuation = true
    @State private var maxDistance: Double = 100
    @State private var selectedSpeaker: String?
    
    var body: some View {
        VStack(alignment: .leading, spacing: 16) {
            Text("Custom Surround Settings")
                .font(.headline)
            
            VStack(alignment: .leading, spacing: 8) {
                Text("VBAP Spread Angle")
                    .font(.subheadline)
                
                HStack {
                    Slider(value: $vbapSpread, in: 0...45)
                    Text("\(Int(vbapSpread))°")
                        .frame(width: 50)
                        .monospacedDigit()
                }
                .help("Controls how sound spreads between adjacent speakers")
            }
            
            Toggle("Distance Attenuation", isOn: $distanceAttenuation)
            
            if distanceAttenuation {
                VStack(alignment: .leading, spacing: 8) {
                    Text("Max Audible Distance")
                        .font(.subheadline)
                    
                    HStack {
                        Slider(value: $maxDistance, in: 10...200)
                        Text("\(Int(maxDistance))m")
                            .frame(width: 50)
                            .monospacedDigit()
                    }
                }
            }
            
            Divider()
            
            Button(action: {
                // Open speaker position editor
            }) {
                Label("Edit Speaker Positions", systemImage: "slider.horizontal.3")
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.bordered)
            
            Button(action: {
                // Save custom layout
            }) {
                Label("Save Layout Preset", systemImage: "square.and.arrow.down")
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.borderedProminent)
        }
    }
}

struct LayoutButton: View {
    let title: String
    let description: String
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text(title)
                        .font(.body.bold())
                    Text(description)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
                Spacer()
                Image(systemName: "chevron.right")
                    .font(.caption)
            }
            .padding(.vertical, 6)
            .padding(.horizontal, 8)
            .frame(maxWidth: .infinity)
        }
        .buttonStyle(.bordered)
    }
}

#Preview("Atmos") {
    AtmosControlsView()
        .frame(width: 300)
        .padding()
}

#Preview("DTS:X") {
    DTSXControlsView()
        .frame(width: 300)
        .padding()
}

#Preview("Custom") {
    CustomSurroundControlsView()
        .frame(width: 300)
        .padding()
}
