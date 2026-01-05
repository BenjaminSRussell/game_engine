import SwiftUI

/// Liquid Glass UI Component - Translucent Plate
/// Implements 2026 "Liquid Glass" design language
@available(macOS 14.0, *)
public struct GlassPlate<Content: View>: View {
    let content: Content
    let material: Material
    let blurRadius: CGFloat
    let cornerRadius: CGFloat
    
    public enum Material {
        case ultraThin
        case thin
        case regular
        case thick
        
        var opacity: Double {
            switch self {
            case .ultraThin: return 0.3
            case .thin: return 0.5
            case .regular: return 0.7
            case .thick: return 0.85
            }
        }
    }
    
    public init(
        material: Material = .regular,
        blurRadius: CGFloat = 20,
        cornerRadius: CGFloat = 12,
        @ViewBuilder content: () -> Content
    ) {
        self.material = material
        self.blurRadius = blurRadius
        self.cornerRadius = cornerRadius
        self.content = content()
    }
    
    public var body: some View {
        ZStack {
            // Background blur
            RoundedRectangle(cornerRadius: cornerRadius)
                .fill(.ultraThinMaterial)
                .opacity(material.opacity)
            
            // Luminance border
            RoundedRectangle(cornerRadius: cornerRadius)
                .strokeBorder(
                    LinearGradient(
                        colors: [
                            .white.opacity(0.3),
                            .white.opacity(0.1),
                            .clear
                        ],
                        startPoint: .topLeading,
                        endPoint: .bottomTrailing
                    ),
                    lineWidth: 1
                )
            
            // Content
            content
                .padding()
        }
    }
}

// MARK: - Preview
struct GlassPlate_Previews: PreviewProvider {
    static var previews: some View {
        ZStack {
            Color.blue.ignoresSafeArea()
            
            GlassPlate {
                VStack(spacing: 12) {
                    Text("Liquid Glass")
                        .font(.headline)
                    Text("Premium macOS Design")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
            }
            .frame(width: 300, height: 200)
        }
    }
}
