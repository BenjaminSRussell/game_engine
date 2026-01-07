import SwiftUI

// MARK: - NLA Editor
// Non-Linear Animation editor for composing action strips

struct NLAEditor: View {
    @ObservedObject var timeline: AnimationTimeline
    @Binding var selectedTracks: Set<UUID>
    
    @State private var zoom: CGFloat = 1.0
    @State private var pan: CGFloat = 0.0
    @State private var pixelsPerSecond: CGFloat = 50.0
    
    var body: some View {
        VStack(spacing: 0) {
            // NLA Header / Controls
            HStack {
                Text("NLA Editor")
                    .font(DesignSystem.Typography.headline)
                
                Spacer()
                
                Button("Add Strip") {
                   // Placeholder for adding a test strip
                   addTestStrip()
                }
                .buttonStyle(NLAEditorButtonStyle())
            }
            .padding(8)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            // Tracks Area
            ScrollView(.vertical) {
                VStack(spacing: 1) {
                    ForEach(timeline.tracks) { track in
                        NLATrackRow(
                            track: track,
                            zoom: zoom,
                            pan: pan,
                            pixelsPerSecond: pixelsPerSecond,
                            onUpdateStrip: updateStrip
                        )
                    }
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
        }
    }
    
    private func addTestStrip() {
        if let trackIndex = timeline.tracks.firstIndex(where: { selectedTracks.contains($0.id) }) ?? timeline.tracks.indices.first {
            let strip = AnimationTimeline.NLAStrip(
                actionID: UUID(),
                name: "Action Clip",
                startTime: timeline.currentTime,
                duration: 2.0
            )
            timeline.tracks[trackIndex].strips.append(strip)
        }
    }
    
    private func updateStrip(trackID: UUID, strip: AnimationTimeline.NLAStrip) {
        // Find track and update strip
        if let trackIndex = timeline.tracks.firstIndex(where: { $0.id == trackID }) {
            if let stripIndex = timeline.tracks[trackIndex].strips.firstIndex(where: { $0.id == strip.id }) {
                timeline.tracks[trackIndex].strips[stripIndex] = strip
            }
        }
    }
}

// MARK: - NLA Track Row
struct NLATrackRow: View {
    let track: AnimationTimeline.TimelineTrack
    let zoom: CGFloat
    let pan: CGFloat
    let pixelsPerSecond: CGFloat
    var onUpdateStrip: (UUID, AnimationTimeline.NLAStrip) -> Void
    
    var body: some View {
        HStack(spacing: 0) {
            // Header
            HStack {
                Text(track.name)
                    .font(.caption)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                Spacer()
            }
            .padding(.horizontal, 8)
            .frame(width: 150)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            // Strip Area
            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    Rectangle()
                        .fill(DesignSystem.Colors.backgroundTertiary.opacity(0.3))
                        .frame(height: 1)
                        .padding(.vertical, 15)
                    
                    ForEach(track.strips) { strip in
                        NLAStripView(
                            strip: strip,
                            zoom: zoom,
                            pixelsPerSecond: pixelsPerSecond,
                            pan: pan,
                            maxHeight: 24
                        )
                        .gesture(
                            DragGesture()
                                .onChanged { value in
                                    var newStrip = strip
                                    let dt = Double(value.translation.width / (pixelsPerSecond * zoom))
                                    newStrip.startTime += dt
                                    // Snap or clamp logic here
                                    onUpdateStrip(track.id, newStrip)
                                }
                        )
                    }
                }
            }
            .frame(height: 30)
            .background(DesignSystem.Colors.backgroundPrimary)
            .clipped()
        }
    }
}

// MARK: - NLA Strip View
struct NLAStripView: View {
    let strip: AnimationTimeline.NLAStrip
    let zoom: CGFloat
    let pixelsPerSecond: CGFloat
    let pan: CGFloat
    let maxHeight: CGFloat
    
    var body: some View {
        let width = CGFloat(strip.duration) * pixelsPerSecond * zoom
        let x = (CGFloat(strip.startTime) * pixelsPerSecond * zoom) + pan
        
        RoundedRectangle(cornerRadius: 4)
            .fill(Color.orange.opacity(0.7))
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(Color.black, lineWidth: 1)
            )
            .overlay(
                Text(strip.name)
                    .font(.caption2)
                    .foregroundColor(.black)
                    .lineLimit(1)
                    .padding(.horizontal, 4),
                alignment: .leading
            )
            .frame(width: max(1, width), height: maxHeight)
            .position(x: x + width/2, y: 15) // Centered position fix
            .overlay(
                // Resize Handles (visual only for now)
                HStack {
                    Rectangle().fill(Color.black.opacity(0.3)).frame(width: 4)
                    Spacer()
                    Rectangle().fill(Color.black.opacity(0.3)).frame(width: 4)
                }
                .frame(width: max(1, width), height: maxHeight)
                .position(x: x + width/2, y: 15)
            )
    }
}

struct NLAEditorButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .padding(.horizontal, 8)
            .padding(.vertical, 4)
            .background(DesignSystem.Colors.accentPrimary)
            .foregroundColor(.white)
            .cornerRadius(4)
            .opacity(configuration.isPressed ? 0.8 : 1.0)
    }
}
