import SwiftUI
import simd

// MARK: - Skeleton Visualizer
struct SkeletonVisualizer: View {
    @ObservedObject var skeleton: Skeleton
    @ObservedObject var audioDriver: AudioAnimationDriver
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background grid
                GridBackground()
                
                // Skeleton rendering
                SkeletonRenderer(
                    skeleton: skeleton,
                    audioDriver: audioDriver,
                    width: geometry.size.width,
                    height: geometry.size.height
                )
            }
        }
        .background(Color.black.opacity(0.8))
        .drawingGroup()
    }
}

// MARK: - Skeleton Renderer
struct SkeletonRenderer: View {
    @ObservedObject var skeleton: Skeleton
    @ObservedObject var audioDriver: AudioAnimationDriver
    let width: CGFloat
    let height: CGFloat
    
    // Camera params
    @State private var rotation: SIMD2<Float> = .zero
    @State private var scale: Float = 50.0
    @State private var offset: SIMD2<Float> = .zero
    
    var body: some View {
        Canvas { context, size in
            let center = CGPoint(x: size.width / 2, y: size.height / 2)
            
            // Draw connections (bones)
            drawBones(context: context, center: center)
            
            // Draw joints (nodes)
            drawJoints(context: context, center: center)
        }
        .gesture(
            DragGesture()
                .onChanged { value in
                    rotation.x += Float(value.translation.height) * 0.01
                    rotation.y += Float(value.translation.width) * 0.01
                }
        )
    }
    
    private func drawBones(context: GraphicsContext, center: CGPoint) {
        for (_, bone) in skeleton.bones {
            guard let parentID = bone.parentID,
                  let parent = skeleton.bones[parentID] else { continue }
            
            let startPos = project(skeleton.boneWorldPosition(parent.id), center: center)
            let endPos = project(skeleton.boneWorldPosition(bone.id), center: center)
            
            // Influence visualization
            let influence = getAudioInfluence(bone.id)
            let color = Color(
                red: 0.5 + Double(influence) * 0.5,
                green: 0.5 - Double(influence) * 0.2,
                blue: 0.5 - Double(influence) * 0.2
            )
            
            var path = Path()
            path.move(to: startPos)
            path.addLine(to: endPos)
            
            context.stroke(path, with: .color(color), lineWidth: 2 + CGFloat(influence * 5))
        }
    }
    
    private func drawJoints(context: GraphicsContext, center: CGPoint) {
        for (_, bone) in skeleton.bones {
            let pos = project(skeleton.boneWorldPosition(bone.id), center: center)
            let influence = getAudioInfluence(bone.id)
            let radius: CGFloat = 4 + CGFloat(influence * 4)
            
            let rect = CGRect(
                x: pos.x - radius,
                y: pos.y - radius,
                width: radius * 2,
                height: radius * 2
            )
            
            context.fill(Path(ellipseIn: rect), with: .color(.white))
            
            if influence > 0.1 {
                context.stroke(
                    Path(ellipseIn: rect.insetBy(dx: -4, dy: -4)),
                    with: .color(.cyan.opacity(Double(influence))),
                    lineWidth: 2
                )
            }
        }
    }
    
    private func project(_ point: SIMD3<Float>, center: CGPoint) -> CGPoint {
        // Simple perspective projection
        var p = point
        
        // Rotate
        let rx = rotation.x
        let ry = rotation.y
        
        let y = p.y * cos(rx) - p.z * sin(rx)
        let z = p.y * sin(rx) + p.z * cos(rx)
        p.y = y
        p.z = z
        
        let x = p.x * cos(ry) + p.z * sin(ry)
        p.x = x
        
        // Project
        let scale = Double(self.scale)
        return CGPoint(
            x: center.x + CGFloat(p.x) * scale,
            y: center.y - CGFloat(p.y) * scale  // Flip Y for screen coords
        )
    }
    
    private func getAudioInfluence(_ boneID: UUID) -> Float {
        // Check if this bone has an active audio mapping
        guard let mapping = audioDriver.mappings.first(where: { $0.boneID == boneID }) else {
            return 0.0
        }
        
        // Return current normalized value for that audio channel
        return Float(AudioAnalyzer.shared.getNormalizedBand(mapping.audioChannel))
    }
}

// MARK: - Grid Background
struct GridBackground: View {
    var body: some View {
        Canvas { context, size in
            let spacing: CGFloat = 40
            let linesX = Int(size.width / spacing)
            let linesY = Int(size.height / spacing)
            
            var path = Path()
            
            for i in 0...linesX {
                let x = CGFloat(i) * spacing
                path.move(to: CGPoint(x: x, y: 0))
                path.addLine(to: CGPoint(x: x, y: size.height))
            }
            
            for i in 0...linesY {
                let y = CGFloat(i) * spacing
                path.move(to: CGPoint(x: 0, y: y))
                path.addLine(to: CGPoint(x: size.width, y: y))
            }
            
            context.stroke(path, with: .color(Color.gray.opacity(0.2)), lineWidth: 1)
        }
    }
}
