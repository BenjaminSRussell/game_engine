import SwiftUI

/// Animated "Vitality" mesh background
/// Creates subtle living gradient animation
@available(macOS 14.0, *)
public struct VitalityMesh: View {
    @State private var phase: CGFloat = 0
    
    let colors: [Color]
    let speed: Double
    
    public init(
        colors: [Color] = [.blue, .purple, .pink, .orange],
        speed: Double = 8.0
    ) {
        self.colors = colors
        self.speed = speed
    }
    
    public var body: some View {
        Group {
            if #available(macOS 15.0, *) {
                let points: [SIMD2<Float>] = [
                    // Top row
                    .init(0, 0),
                    .init(0.5 + Float(sin(phase)) * 0.1, 0),
                    .init(1, 0),
                    // Middle row
                    .init(0, 0.5 + Float(cos(phase * 1.2)) * 0.1),
                    .init(0.5 + Float(sin(phase * 0.8)) * 0.15, 0.5),
                    .init(1, 0.5 + Float(cos(phase * 0.9)) * 0.1),
                    // Bottom row
                    .init(0, 1),
                    .init(0.5 + Float(sin(phase * 1.1)) * 0.1, 1),
                    .init(1, 1)
                ]
                
                let meshColors = colors + colors.reversed() + [colors.first ?? .blue]
                
                MeshGradient(
                    width: 3,
                    height: 3,
                    points: points,
                    colors: meshColors
                )
                .onAppear {
                    withAnimation(.linear(duration: speed).repeatForever(autoreverses: false)) {
                        phase = .pi * 2
                    }
                }
            } else {
                // Fallback for macOS 14
                ZStack {
                    LinearGradient(colors: colors, startPoint: .topLeading, endPoint: .bottomTrailing)
                    Color.black.opacity(0.1)
                }
            }
        }
        .ignoresSafeArea()
    }
}

// MARK: - Preview
@available(macOS 14.0, *)
struct VitalityMesh_Previews: PreviewProvider {
    static var previews: some View {
        VitalityMesh()
    }
}
