import SwiftUI

/// Context-aware status glow
/// Emits ambient color based on system state
@available(macOS 14.0, *)
public struct StatusGlow: View {
    let status: SystemStatus
    let intensity: Double
    
    public enum SystemStatus {
        case idle
        case active
        case warning
        case error
        
        var color: Color {
            switch self {
            case .idle: return .blue
            case .active: return .green
            case .warning: return .orange
            case .error: return .red
            }
        }
    }
    
    public init(status: SystemStatus, intensity: Double = 0.5) {
        self.status = status
        self.intensity = intensity
    }
    
    public var body: some View {
        Circle()
            .fill(
                RadialGradient(
                    colors: [
                        status.color.opacity(intensity),
                        status.color.opacity(intensity * 0.5),
                        .clear
                    ],
                    center: .center,
                    startRadius: 0,
                    endRadius: 100
                )
            )
            .frame(width: 200, height: 200)
            .blur(radius: 50)
            .animation(.easeInOut(duration: 2).repeatForever(autoreverses: true), value: status)
    }
}

// MARK: - Preview
struct StatusGlow_Previews: PreviewProvider {
    static var previews: some View {
        ZStack {
            Color.black.ignoresSafeArea()
            
            HStack(spacing: 50) {
                StatusGlow(status: .idle)
                StatusGlow(status: .active)
                StatusGlow(status: .warning)
                StatusGlow(status: .error)
            }
        }
    }
}
