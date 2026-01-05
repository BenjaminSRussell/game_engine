import SwiftUI

struct SpeakerVisualizerView: View {
    let format: AudioFormat
    let speakerLevels: [String: Double]
    @State private var viewMode: ViewMode = .topDown
    
    enum ViewMode {
        case topDown, sideView
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Professional header
            HStack {
                Text("Speaker Layout")
                    .font(.title2.bold())
                    .foregroundStyle(.primary)
                
                Spacer()
                
                // View mode toggle
                Picker("View", selection: $viewMode) {
                    Label("Top", systemImage: "arrow.down.circle").tag(ViewMode.topDown)
                    Label("Side", systemImage: "arrow.right.circle").tag(ViewMode.sideView)
                }
                .pickerStyle(.segmented)
                .frame(width: 180)
            }
            .padding()
            .background(.ultraThinMaterial)
            
            Divider()
            
            // Visualizer
            GeometryReader { geometry in
                ZStack {
                    // Premium gradient background
                    LinearGradient(
                        colors: [
                            Color(nsColor: .controlBackgroundColor),
                            Color(nsColor: .controlBackgroundColor).opacity(0.7)
                        ],
                        startPoint: .top,
                        endPoint: .bottom
                    )
                    
                    // Subtle grid
                    Canvas { context, size in
                        let gridSpacing: CGFloat = 50
                        context.stroke(
                            Path { path in
                                for x in stride(from: 0, to: size.width, by: gridSpacing) {
                                    path.move(to: CGPoint(x: x, y: 0))
                                    path.addLine(to: CGPoint(x: x, y: size.height))
                                }
                                for y in stride(from: 0, to: size.height, by: gridSpacing) {
                                    path.move(to: CGPoint(x: 0, y: y))
                                    path.addLine(to: CGPoint(x: size.width, y: y))
                                }
                            },
                            with: .color(.white.opacity(0.05)),
                            lineWidth: 1
                        )
                    }
                    
                    // Room outline with premium styling
                    RoundedRectangle(cornerRadius: 16)
                        .stroke(
                            LinearGradient(
                                colors: [.blue.opacity(0.3), .purple.opacity(0.3)],
                                startPoint: .topLeading,
                                endPoint: .bottomTrailing
                            ),
                            lineWidth: 3
                        )
                        .padding(50)
                        .shadow(color: .blue.opacity(0.2), radius: 10)
                    
                    // Listener position with glow
                    ZStack {
                        Circle()
                            .fill(.blue.opacity(0.2))
                            .frame(width: 40, height: 40)
                            .blur(radius: 8)
                        
                        Circle()
                            .fill(
                                LinearGradient(
                                    colors: [.blue, .cyan],
                                    startPoint: .topLeading,
                                    endPoint: .bottomTrailing
                                )
                            )
                            .frame(width: 24, height: 24)
                            .overlay(
                                Circle()
                                    .stroke(.white.opacity(0.5), lineWidth: 2)
                            )
                        
                        Image(systemName: "person.fill")
                            .font(.system(size: 12))
                            .foregroundStyle(.white)
                    }
                    .position(x: geometry.size.width / 2, y: geometry.size.height / 2)
                    
                    // Speakers
                    ForEach(format.speakers.filter { viewMode == .topDown || $0.height == 0 }) { speaker in
                        PremiumSpeakerIndicator(
                            speaker: speaker,
                            level: speakerLevels[speaker.channel] ?? 0,
                            geometry: geometry
                        )
                    }
                    
                    // Height indicators for Atmos
                    if format == .dolbyAtmos && viewMode == .topDown {
                        ForEach(format.speakers.filter { $0.height > 0 }) { speaker in
                            PremiumSpeakerIndicator(
                                speaker: speaker,
                                level: speakerLevels[speaker.channel] ?? 0,
                                geometry: geometry,
                                isHeight: true
                            )
                        }
                    }
                }
            }
        }
        .background(Color(nsColor: .windowBackgroundColor))
        .clipShape(RoundedRectangle(cornerRadius: 16))
        .shadow(color: .black.opacity(0.1), radius: 10)
    }
}

struct PremiumSpeakerIndicator: View {
    let speaker: SpeakerPosition
    let level: Double
    let geometry: GeometryProxy
    var isHeight: Bool = false
    
    private var speakerPosition: CGPoint {
        let centerX = geometry.size.width / 2
        let centerY = geometry.size.height / 2
        let scale = min(geometry.size.width, geometry.size.height) * 0.35
        
        return CGPoint(
            x: centerX + speaker.position.x * scale,
            y: centerY - speaker.position.y * scale
        )
    }
    
    private var color: Color {
        if speaker.channel == "lfe" {
            return .purple
        } else if isHeight {
            return .cyan
        } else {
            return .green
        }
    }
    
    private var gradient: LinearGradient {
        LinearGradient(
            colors: [color, color.opacity(0.6)],
            startPoint: .topLeading,
            endPoint: .bottomTrailing
        )
    }
    
    var body: some View {
        ZStack {
            // Outer activity glow
            if level > 0.01 {
                Circle()
                    .fill(color.opacity(0.15))
                    .frame(width: 60 + CGFloat(level * 40), height: 60 + CGFloat(level * 40))
                    .blur(radius: 12)
                    .animation(.easeOut(duration: 0.15), value: level)
                
                Circle()
                    .stroke(color.opacity(0.4), lineWidth: 2)
                    .frame(width: 50 + CGFloat(level * 30), height: 50 + CGFloat(level * 30))
                    .animation(.easeInOut(duration: 0.1), value: level)
            }
            
            // Speaker body with premium gradient
            Circle()
                .fill(gradient)
                .frame(width: 36, height: 36)
                .overlay(
                    Circle()
                        .stroke(.white.opacity(0.3), lineWidth: 2)
                )
                .overlay(
                    Circle()
                        .fill(color.opacity(level * 0.6))
                        .scaleEffect(0.7 + CGFloat(level) * 0.3)
                        .blur(radius: 4)
                )
                .shadow(color: color.opacity(0.5), radius: 8)
            
            // Label with backdrop
            Text(speaker.name)
                .font(.caption.bold().monospacedDigit())
                .foregroundStyle(.white)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 6))
                .shadow(color: .black.opacity(0.3), radius: 4)
                .offset(y: 30)
            
            // Height indicator with glow
            if isHeight {
                ZStack {
                    Circle()
                        .fill(.cyan.opacity(0.3))
                        .frame(width: 20, height: 20)
                        .blur(radius: 4)
                    
                    Image(systemName: "arrow.up.circle.fill")
                        .font(.callout)
                        .foregroundStyle(
                            LinearGradient(
                                colors: [.cyan, .blue],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                        )
                        .shadow(color: .cyan, radius: 4)
                }
                .offset(x: 20, y: -20)
            }
            
            // Peak indicator
            if level > 0.9 {
                Image(systemName: "exclamationmark.triangle.fill")
                    .font(.caption2)
                    .foregroundStyle(.red)
                    .offset(x: -20, y: -20)
                    .shadow(color: .red, radius: 4)
            }
        }
        .position(speakerPosition)
    }
}

#Preview {
    SpeakerVisualizerView(
        format: .dolbyAtmos,
        speakerLevels: [
            "front_left": 0.7,
            "front_right": 0.8,
            "center": 0.5,
            "height_front_left": 0.4
        ]
    )
    .frame(width: 700, height: 550)
}
