import SwiftUI

struct AudioMetersGridView: View {
    let speakers: [SpeakerPosition]
    let levels: [String: Double]
    
    private let columns = [
        GridItem(.adaptive(minimum: 180), spacing: 16)
    ]
    
    var body: some View {
        ScrollView {
            LazyVGrid(columns: columns, spacing: 16) {
                ForEach(speakers) { speaker in
                    PremiumAudioMeter(
                        name: speaker.name,
                        channel: speaker.channel,
                        level: levels[speaker.channel] ?? 0
                    )
                }
            }
            .padding(20)
        }
        .background(
            LinearGradient(
                colors: [
                    Color(nsColor: .controlBackgroundColor),
                    Color(nsColor: .controlBackgroundColor).opacity(0.8)
                ],
                startPoint: .top,
                endPoint: .bottom
            )
        )
    }
}

struct PremiumAudioMeter: View {
    let name: String
    let channel: String
    let level: Double
    
    @State private var peakLevel: Double = 0
    @State private var peakHoldTime: Date = Date()
    
    private var dbLevel: Double {
        level > 0 ? 20 * log10(level) : -60
    }
    
    private var meterColor: Color {
        if dbLevel > -3 {
            return .red
        } else if dbLevel > -6 {
            return .orange
        } else if dbLevel > -12 {
            return .yellow
        } else {
            return .green
        }
    }
    
    private var gradientColors: [Color] {
        [.green, .green, .yellow, .orange, .red]
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            // Header with glow effect
            HStack {
                ZStack(alignment: .leading) {
                    Text(name)
                        .font(.headline.bold())
                        .foregroundStyle(.white)
                        .shadow(color: meterColor.opacity(0.5), radius: 4)
                    
                    if level > 0.01 {
                        Text(name)
                            .font(.headline.bold())
                            .foregroundStyle(meterColor.opacity(0.3))
                            .blur(radius: 8)
                    }
                }
                
                Spacer()
                
                Text("\(Int(dbLevel)) dB")
                    .font(.system(.body, design: .monospaced).weight(.semibold))
                    .foregroundStyle(meterColor)
                    .padding(.horizontal, 10)
                    .padding(.vertical, 4)
                    .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 8))
            }
            
            // Premium meter with gradient
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    // Background with depth
                    RoundedRectangle(cornerRadius: 6)
                        .fill(
                            LinearGradient(
                                colors: [.black.opacity(0.6), .black.opacity(0.8)],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                        )
                        .overlay(
                            RoundedRectangle(cornerRadius: 6)
                                .stroke(.white.opacity(0.1), lineWidth: 1)
                        )
                    
                    // Level gradient bar
                    RoundedRectangle(cornerRadius: 4)
                        .fill(
                            LinearGradient(
                                colors: gradientColors,
                                startPoint: .leading,
                                endPoint: .trailing
                            )
                        )
                        .frame(width: max(4, geometry.size.width * CGFloat(normalizedLevel)))
                        .padding(2)
                        .animation(.linear(duration: 0.05), value: level)
                        .overlay(
                            // Shine effect
                            LinearGradient(
                                colors: [.white.opacity(0.3), .clear],
                                startPoint: .top,
                                endPoint: .bottom
                            )
                            .frame(width: max(4, geometry.size.width * CGFloat(normalizedLevel)))
                            .padding(2)
                            .clipShape(RoundedRectangle(cornerRadius: 4))
                        )
                    
                    // Peak hold line with glow
                    if peakLevel > 0.01 {
                        Rectangle()
                            .fill(meterColor)
                            .frame(width: 3, height: geometry.size.height)
                            .offset(x: geometry.size.width * CGFloat(normalizedPeak))
                            .shadow(color: meterColor, radius: 6)
                    }
                    
                    // Scale markers
                    HStack(spacing: 0) {
                        ForEach([0.0, 0.25, 0.5, 0.75, 1.0], id: \.self) { position in
                            Rectangle()
                                .fill(.white.opacity(0.2))
                                .frame(width: 1, height: geometry.size.height)
                            if position < 1.0 {
                                Spacer()
                            }
                        }
                    }
                }
            }
            .frame(height: 28)
            
            // dB scale with improved typography
            HStack {
                ForEach([-60, -30, -12, -6, 0], id: \.self) { db in
                    Text("\(db)")
                        .font(.system(size: 9, design: .monospaced))
                        .foregroundStyle(db == 0 ? .red.opacity(0.8) : .secondary)
                    if db < 0 {
                        Spacer()
                    }
                }
            }
        }
        .padding(12)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(.ultraThinMaterial)
                .shadow(color: .black.opacity(0.15), radius: 6)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(
                    LinearGradient(
                        colors: [.white.opacity(0.2), .white.opacity(0.05)],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    ),
                    lineWidth: 1
                )
        )
        .onChange(of: level) { newLevel in
            updatePeak(newLevel)
        }
    }
    
    private var normalizedLevel: Double {
        max(0, min(1, (dbLevel + 60) / 60))
    }
    
    private var normalizedPeak: Double {
        max(0, min(1, (20 * log10(peakLevel) + 60) / 60))
    }
    
    private func updatePeak(_ newLevel: Double) {
        if newLevel > peakLevel {
            peakLevel = newLevel
            peakHoldTime = Date()
        } else if Date().timeIntervalSince(peakHoldTime) > 2.0 {
            peakLevel *= 0.95
        }
    }
}

#Preview {
    AudioMetersGridView(
        speakers: SpeakerPosition.atmos714Layout,
        levels: [
            "front_left": 0.7,
            "front_right": 0.8,
            "center": 0.5,
            "lfe": 0.95
        ]
    )
    .frame(height: 300)
}
