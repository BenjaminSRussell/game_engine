import SwiftUI
import simd

// MARK: - F-Curve Editor
// Advanced graph editor for manipulating animation curves with bezier handles

struct FCurveEditor: View {
    @ObservedObject var timeline: AnimationTimeline
    @Binding var selectedTracks: Set<UUID>
    
    @State private var zoom: SIMD2<Float> = SIMD2(1.0, 1.0)
    @State private var pan: SIMD2<Float> = SIMD2(0, 0)
    @State private var selection: Set<UUID> = []
    @State private var draggingID: UUID?
    @State private var dragOffset: SIMD2<Float> = .zero
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background Grid
                FCurveGrid(zoom: zoom, pan: pan, size: geometry.size)
                
                // Curves
                ForEach(filterSelectedTracks()) { track in
                    FCurveRenderer(
                        track: track,
                        zoom: zoom,
                        pan: pan,
                        size: geometry.size,
                        selection: $selection,
                        onDragStart: handleDragStart,
                        onDrag: { point in handleDrag(location: point, size: geometry.size) },
                        onDragEnd: handleDragEnd
                    )
                }
                
                // Selection overlay / controls
                if !selection.isEmpty {
                   // SelectionControls()
                }
            }
            .background(DesignSystem.Colors.backgroundPrimary)
            .clipped()
            .gesture(
                DragGesture()
                    .onChanged { value in
                        if draggingID == nil {
                            // Pan grid
                            pan.x += Float(value.translation.width) * 0.1
                            pan.y += Float(value.translation.height) * 0.1
                        }
                    }
            )
            .overlay(
                 // Zoom controls could go here or be handled by scroll/magnification gestures
                 ZoomControls(zoom: $zoom),
                 alignment: .topTrailing
            )
        }
    }
    
    private func filterSelectedTracks() -> [AnimationTimeline.TimelineTrack] {
        return timeline.tracks.filter { selectedTracks.contains($0.id) }
    }
    
    private func handleDragStart(id: UUID, location: CGPoint, type: FCurveRenderer.DragType) {
        // Logic handled in FCurveRenderer mostly, but we need to track state here
        draggingID = id
        // We'll calculate offset in handleDrag relative to start
         // For now just track which ID we are dragging
    }
    
    private func handleDrag(location: CGPoint, size: CGSize) {
        guard let id = draggingID else { return }
        
        // Find track containing this keyframe
        for trackIndex in timeline.tracks.indices {
            if let keyIndex = timeline.tracks[trackIndex].keyframes.firstIndex(where: { $0.id == id }) {
                // Convert screen location to data space
                let time = Double(location.x - CGFloat(pan.x)) / (100.0 * Double(zoom.x))
                let val = Float(size.height/2 + CGFloat(pan.y) - location.y) / (50.0 * zoom.y)
                
                // Update keyframe
                // Note: For handles we'd need to know if we are dragging the handle or the key
                // For this pass we'll assume keyframe dragging
                
                var key = timeline.tracks[trackIndex].keyframes[keyIndex]
                key.time = max(0, time)
                
                // Update value based on type
                switch key.value {
                case .float:
                    key.value = .float(val)
                case .vector3(let v):
                    key.value = .vector3(SIMD3<Float>(val, v.y, v.z))
                default:
                    break
                }
                
                timeline.tracks[trackIndex].keyframes[keyIndex] = key
                return
            }
        }
    }
    
    private func handleDragEnd() {
        draggingID = nil
        dragOffset = .zero
    }
}

// MARK: - Grid Renderer
struct FCurveGrid: View {
    let zoom: SIMD2<Float>
    let pan: SIMD2<Float>
    let size: CGSize
    
    var body: some View {
        Canvas { context, size in
            // Draw time lines (vertical)
            drawGridLines(
                context: context,
                size: size,
                step: 1.0 * CGFloat(zoom.x),
                offset: CGFloat(pan.x),
                isVertical: true,
                color: Color.gray.opacity(0.3)
            )
            
            // Draw value lines (horizontal)
            drawGridLines(
                context: context,
                size: size,
                step: 10.0 * CGFloat(zoom.y),
                offset: CGFloat(pan.y),
                isVertical: false,
                color: Color.gray.opacity(0.3)
            )
            
            // Draw axes
            let zeroX = (0 + CGFloat(pan.x)) * CGFloat(zoom.x)
            let zeroY = size.height / 2 + CGFloat(pan.y) * CGFloat(zoom.y)
            
            // X Axis (Time)
            context.stroke(
                Path { p in
                    p.move(to: CGPoint(x: 0, y: zeroY))
                    p.addLine(to: CGPoint(x: size.width, y: zeroY))
                },
                with: .color(.white.opacity(0.8))
            )
            
            // Y Axis (Value)
            context.stroke(
                Path { p in
                    p.move(to: CGPoint(x: zeroX, y: 0))
                    p.addLine(to: CGPoint(x: zeroX, y: size.height))
                },
                with: .color(.white.opacity(0.8))
            )
        }
    }
    
    private func drawGridLines(
        context: GraphicsContext,
        size: CGSize,
        step: CGFloat,
        offset: CGFloat,
        isVertical: Bool,
        color: Color
    ) {
        let count = isVertical ? Int(size.width / step) : Int(size.height / step)
        
        for i in 0...count {
            let pos = CGFloat(i) * step // + offset accounting needed
            
            context.stroke(
                Path { p in
                    if isVertical {
                        p.move(to: CGPoint(x: pos, y: 0))
                        p.addLine(to: CGPoint(x: pos, y: size.height))
                    } else {
                        p.move(to: CGPoint(x: 0, y: pos))
                        p.addLine(to: CGPoint(x: size.width, y: pos))
                    }
                },
                with: .color(color)
            )
        }
    }
}

// MARK: - Curve Renderer
struct FCurveRenderer: View {
    let track: AnimationTimeline.TimelineTrack
    let zoom: SIMD2<Float>
    let pan: SIMD2<Float>
    let size: CGSize
    @Binding var selection: Set<UUID>
    
    var onDragStart: (UUID, CGPoint, DragType) -> Void
    var onDrag: (CGPoint) -> Void
    var onDragEnd: () -> Void
    
    enum DragType {
        case keyframe
        case inTangent
        case outTangent
    }
    
    var body: some View {
        Canvas { context, size in
            let sortedKeys = track.keyframes.sorted { $0.time < $1.time }
            guard sortedKeys.count > 0 else { return }
            
            // Coordinate transformation helper
            let toScreen = { (time: Double, value: Float) -> CGPoint in
                let x = (CGFloat(time) * 100 * CGFloat(zoom.x)) + CGFloat(pan.x)
                let y = size.height/2 - (CGFloat(value) * 50 * CGFloat(zoom.y)) + CGFloat(pan.y)
                return CGPoint(x: x, y: y)
            }
            
            // Draw Curve
            if sortedKeys.count > 1 {
                var path = Path()
                
                if let first = sortedKeys.first {
                    let startVal = extractValue(first.value)
                    path.move(to: toScreen(first.time, startVal))
                }
                
                for i in 0..<sortedKeys.count - 1 {
                    let p0 = sortedKeys[i]
                    let p1 = sortedKeys[i+1]
                    
                    let v0 = extractValue(p0.value)
                    let v1 = extractValue(p1.value)
                    
                    let p0Screen = toScreen(p0.time, v0)
                    let p1Screen = toScreen(p1.time, v1)
                    
                    switch p0.interpolation {
                    case .linear:
                        path.addLine(to: p1Screen)
                        
                    case .bezier:
                        // Calculate control points based on time deltas
                        let dt = CGFloat(p1.time - p0.time) * 100 * CGFloat(zoom.x)
                        
                        let t1 = p0.outTangent ?? SIMD2<Float>(0.5, 0)
                        let t2 = p1.inTangent ?? SIMD2<Float>(-0.5, 0)
                        
                        // Scale handles by time delta for proper curvature
                        let cx1 = p0Screen.x + CGFloat(t1.x) * dt * 0.5
                        let cy1 = p0Screen.y - CGFloat(t1.y) * 50 * CGFloat(zoom.y)
                        let cx2 = p1Screen.x + CGFloat(t2.x) * dt * 0.5
                        let cy2 = p1Screen.y - CGFloat(t2.y) * 50 * CGFloat(zoom.y)
                        
                        path.addCurve(
                            to: p1Screen,
                            control1: CGPoint(x: cx1, y: cy1),
                            control2: CGPoint(x: cx2, y: cy2)
                        )
                        
                    case .constant:
                        path.addLine(to: CGPoint(x: p1Screen.x, y: p0Screen.y))
                        path.addLine(to: p1Screen)
                        
                    default:
                        path.addLine(to: p1Screen)
                    }
                }
                
                context.stroke(path, with: .color(track.color), lineWidth: 2)
            }
            
            // Draw Keyframes and Handles
            for key in sortedKeys {
                let val = extractValue(key.value)
                let pos = toScreen(key.time, val)
                let isSelected = selection.contains(key.id)
                
                // Keyframe diamond
                let rect = CGRect(x: pos.x - 5, y: pos.y - 5, width: 10, height: 10)
                let path = Path { p in
                    p.move(to: CGPoint(x: rect.midX, y: rect.minY))
                    p.addLine(to: CGPoint(x: rect.maxX, y: rect.midY))
                    p.addLine(to: CGPoint(x: rect.midX, y: rect.maxY))
                    p.addLine(to: CGPoint(x: rect.minX, y: rect.midY))
                    p.closeSubpath()
                }
                
                context.fill(path, with: isSelected ? .color(.white) : .color(track.color))
                context.stroke(path, with: .color(.black), lineWidth: 1.5)
                
                // Handles
                if isSelected && key.interpolation == .bezier {
                    drawHandle(context: context, pos: pos, tangent: key.inTangent, isIn: true, zoom: zoom)
                    drawHandle(context: context, pos: pos, tangent: key.outTangent, isIn: false, zoom: zoom)
                }
            }
        }
        .gesture(
            DragGesture(minimumDistance: 0)
                .onChanged { value in
                    onDrag(value.location)
                }
                .onEnded { _ in
                    onDragEnd()
                }
        )
    }
    
    private func drawHandle(context: GraphicsContext, pos: CGPoint, tangent: SIMD2<Float>?, isIn: Bool, zoom: SIMD2<Float>) {
        guard let tan = tangent else { return }
        
        let handleLen: CGFloat = 40.0
        // Tangent x is usually normalized time (0-1), y is value
        // We need to project this to screen space
        let dx = CGFloat(tan.x) * handleLen * (isIn ? -1 : 1)
        let dy = CGFloat(-tan.y) * handleLen // Flip Y for screen
        
        let handlePos = CGPoint(x: pos.x + dx, y: pos.y + dy)
        
        context.stroke(
            Path { p in p.move(to: pos); p.addLine(to: handlePos) },
            with: .color(.gray)
        )
        
        context.fill(
            Path(ellipseIn: CGRect(x: handlePos.x - 3, y: handlePos.y - 3, width: 6, height: 6)),
            with: .color(.pink)
        )
    }
    
    private func extractValue(_ value: AnimationTimeline.Keyframe.KeyframeValue) -> Float {
        switch value {
        case .float(let f): return f
        case .vector3(let v): return v.x // Currently uses X component. Future: UI for selecting which component (x/y/z)
        case .bool(let b): return b ? 1.0 : 0.0
        default: return 0
        }
    }
}

// MARK: - Zoom Controls
struct ZoomControls: View {
    @Binding var zoom: SIMD2<Float>
    
    var body: some View {
        VStack {
            Button(action: { zoom *= 1.2 }) {
                Image(systemName: "plus.magnifyingglass")
            }
            .buttonStyle(EditorIconButtonStyle())
            
            Button(action: { zoom *= 0.8 }) {
                Image(systemName: "minus.magnifyingglass")
            }
            .buttonStyle(EditorIconButtonStyle())
            
            Button(action: { zoom = SIMD2(1,1) }) {
                Image(systemName: "arrow.up.left.and.arrow.down.right.magnifyingglass")
            }
            .buttonStyle(EditorIconButtonStyle())
        }
        .padding()
        .background(DesignSystem.Colors.backgroundSecondary.opacity(0.8))
        .cornerRadius(8)
        .padding()
    }
}

struct EditorIconButtonStyle: ButtonStyle {
    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .foregroundColor(.white)
            .padding(8)
            .background(configuration.isPressed ? Color.gray : Color.clear)
            .cornerRadius(4)
    }
}
