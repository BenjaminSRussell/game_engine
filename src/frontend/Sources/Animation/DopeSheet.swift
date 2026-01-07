import SwiftUI

// MARK: - Dope Sheet
// High-level timeline view for adjusting keyframe timing

struct DopeSheet: View {
    @ObservedObject var timeline: AnimationTimeline
    @Binding var selectedTracks: Set<UUID>
    
    @State private var zoom: CGFloat = 1.0
    @State private var scrollOffset: CGFloat = 0.0
    @State private var pixelsPerSecond: CGFloat = 100.0
    
    var body: some View {
        VStack(spacing: 0) {
            // Header / Ruler
            DopeSheetRuler(
                duration: timeline.duration,
                zoom: zoom,
                pixelsPerSecond: pixelsPerSecond,
                scrollOffset: $scrollOffset
            )
            .frame(height: 30)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            // Tracks
            ScrollView(.vertical) {
                VStack(spacing: 1) {
                    ForEach(timeline.tracks) { track in
                        DopeSheetTrackRow(
                            track: track,
                            zoom: zoom,
                            pixelsPerSecond: pixelsPerSecond,
                            scrollOffset: scrollOffset,
                            isSelected: selectedTracks.contains(track.id),
                            onSelect: { selectedTracks.insert(track.id) }
                        )
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
        .overlay(
            // Playhead
            Rectangle()
                .fill(Color.yellow)
                .frame(width: 2)
                .offset(x: (CGFloat(timeline.currentTime) * pixelsPerSecond * zoom) - scrollOffset)
                .allowsHitTesting(false),
            alignment: .leading
        )
    }
}

// MARK: - Ruler
struct DopeSheetRuler: View {
    let duration: TimeInterval
    let zoom: CGFloat
    let pixelsPerSecond: CGFloat
    @Binding var scrollOffset: CGFloat
    
    var body: some View {
        GeometryReader { geometry in
            Canvas { context, size in
                let step = 1.0 * zoom * pixelsPerSecond
                let count = Int(duration + 1)
                
                for i in 0...count {
                    let x = (CGFloat(i) * step) - scrollOffset
                    if x >= 0 && x <= size.width {
                        // Major tick
                        context.stroke(
                            Path { p in
                                p.move(to: CGPoint(x: x, y: 15))
                                p.addLine(to: CGPoint(x: x, y: 30))
                            },
                            with: .color(.gray)
                        )
                        // Label
                        context.draw(
                            Text("\(i)s").font(.caption2),
                            at: CGPoint(x: x + 2, y: 10)
                        )
                    }
                    
                    // Minor ticks
                    for j in 1...4 {
                        let mx = x + (CGFloat(j) * step / 5.0)
                        if mx >= 0 && mx <= size.width {
                            context.stroke(
                                Path { p in
                                    p.move(to: CGPoint(x: mx, y: 22))
                                    p.addLine(to: CGPoint(x: mx, y: 30))
                                },
                                with: .color(.gray.opacity(0.5))
                            )
                        }
                    }
                }
            }
            .gesture(
                DragGesture()
                    .onChanged { value in
                        scrollOffset -= value.translation.width
                    }
            )
        }
    }
}

// MARK: - Track Row
struct DopeSheetTrackRow: View {
    let track: AnimationTimeline.TimelineTrack
    let zoom: CGFloat
    let pixelsPerSecond: CGFloat
    let scrollOffset: CGFloat
    let isSelected: Bool
    var onSelect: () -> Void
    
    var body: some View {
        HStack(spacing: 0) {
            // Track Header (Name, Controls)
            HStack {
                Image(systemName: track.type.icon)
                    .foregroundColor(track.color)
                Text(track.name)
                    .font(.caption)
                    .foregroundColor(.white)
                Spacer()
                
                Button(action: { /* Toggle Mute */ }) {
                    Image(systemName: track.isMuted ? "speaker.slash" : "speaker")
                        .font(.caption2)
                }
                
                Button(action: { /* Toggle Lock */ }) {
                    Image(systemName: track.isLocked ? "lock" : "lock.open")
                        .font(.caption2)
                }
            }
            .padding(.horizontal, 8)
            .frame(width: 200)
            .frame(height: 32)
            .background(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.2) : DesignSystem.Colors.backgroundSecondary)
            .onTapGesture { onSelect() }
            
            // Timeline Content
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    Rectangle()
                        .fill(isSelected ? DesignSystem.Colors.backgroundTertiary : DesignSystem.Colors.backgroundPrimary)
                    
                    // Keyframes
                    ForEach(track.keyframes) { keyframe in
                        let x = (CGFloat(keyframe.time) * pixelsPerSecond * zoom) - scrollOffset
                        
                        if x >= -10 && x <= geometry.size.width + 10 {
                            DopeSheetKeyframeMarker(color: track.color, isSelected: false) // TODO: selection state
                                .position(x: x, y: 16)
                        }
                    }
                }
            }
            .frame(height: 32)
            .clipped()
        }
    }
}

struct DopeSheetKeyframeMarker: View {
    let color: Color
    let isSelected: Bool
    
    var body: some View {
        Rhombus()
            .fill(isSelected ? .white : color)
            .frame(width: 8, height: 8)
            .overlay(
                Rhombus()
                    .stroke(Color.black, lineWidth: 1)
            )
    }
}

struct Rhombus: Shape {
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
