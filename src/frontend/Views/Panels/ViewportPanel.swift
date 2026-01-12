import SwiftUI
import MetalKit

// MARK: - Viewport Panel

struct ViewportPanel: View {
    @ObservedObject var selectionManager: SelectionManager
    @ObservedObject private var profiler = Profiler.shared
    
    let rendererBridge: UnsafeMutablePointer<RendererBridge>?
    
    @State private var gizmoMode: GizmoMode = .translate
    @State private var showGrid = true
    @State private var showStats = true
    @State private var cameraMode: CameraMode = .perspective
    
    enum GizmoMode {
        case translate, rotate, scale
        
        var icon: String {
            switch self {
            case .translate: return "arrow.up.and.down.and.arrow.left.and.right"
            case .rotate: return "arrow.triangle.2.circlepath"
            case .scale: return "arrow.up.left.and.arrow.down.right"
            }
        }
    }
    
    enum CameraMode {
        case perspective, orthographic, top, front, right
        
        var displayName: String {
            switch self {
            case .perspective: return "Perspective"
            case .orthographic: return "Orthographic"
            case .top: return "Top"
            case .front: return "Front"
            case .right: return "Right"
            }
        }
    }
    
    var body: some View {
        ZStack {
            // Metal viewport
            if let bridge = rendererBridge {
                EngineViewport(rendererBridge: bridge)
                    .edgesIgnoringSafeArea(.all)
            } else {
                // Placeholder when no bridge is available
                DesignSystem.Colors.backgroundPrimary
                    .overlay(
                        VStack(spacing: DesignSystem.Spacing.md) {
                            Image(systemName: "cube.transparent")
                                .font(.system(size: 64, weight: .thin))
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                            
                            Text("No Engine Connection")
                                .font(DesignSystem.Typography.headline)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                            
                            Text("The rendering engine is not initialized")
                                .font(DesignSystem.Typography.body)
                                .foregroundColor(DesignSystem.Colors.textTertiary)
                        }
                    )
            }
            
            // Viewport overlay
            VStack {
                // Toolbar
                ViewportToolbar(
                    gizmoMode: $gizmoMode,
                    showGrid: $showGrid,
                    showStats: $showStats,
                    cameraMode: $cameraMode
                )
                
                Spacer()
                
                // Stats overlay
                if showStats {
                    ViewportStatsOverlay(profiler: profiler)
                        .padding(DesignSystem.Spacing.sm)
                }
            }
        }
    }
}

// MARK: - Viewport Toolbar

struct ViewportToolbar: View {
    @Binding var gizmoMode: ViewportPanel.GizmoMode
    @Binding var showGrid: Bool
    @Binding var showStats: Bool
    @Binding var cameraMode: ViewportPanel.CameraMode
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            // Camera mode dropdown
            Menu {
                ForEach([ViewportPanel.CameraMode.perspective, .orthographic, .top, .front, .right], id: \.self) { mode in
                    Button(mode.displayName) {
                        cameraMode = mode
                    }
                }
            } label: {
                HStack(spacing: DesignSystem.Spacing.xs) {
                    Image(systemName: "camera")
                        .font(.system(size: 12, weight: .medium))
                    Text(cameraMode.displayName)
                        .font(DesignSystem.Typography.small)
                }
                .foregroundColor(DesignSystem.Colors.textSecondary)
                .padding(.horizontal, DesignSystem.Spacing.sm)
                .padding(.vertical, DesignSystem.Spacing.xs)
                .background(DesignSystem.Colors.backgroundTertiary)
                .cornerRadius(DesignSystem.CornerRadius.tight)
            }
            .menuStyle(.borderlessButton)
            
            Spacer()
            
            // Gizmo modes
            HStack(spacing: 2) {
                ForEach([ViewportPanel.GizmoMode.translate, .rotate, .scale], id: \.icon) { mode in
                    Button(action: { gizmoMode = mode }) {
                        Image(systemName: mode.icon)
                            .font(.system(size: 14, weight: .medium))
                            .foregroundColor(gizmoMode == mode ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                            .frame(width: 28, height: 28)
                            .background(gizmoMode == mode ? DesignSystem.Colors.selection : Color.clear)
                            .cornerRadius(4)
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.regular)
            
            // View options
            HStack(spacing: 2) {
                EditorIconButton(icon: showGrid ? "square.grid.3x3" : "square.grid.3x3.fill", tooltip: "Toggle Grid") {
                    showGrid.toggle()
                    EngineBridge.shared.setGridEnabled(showGrid)
                }
                
                EditorIconButton(icon: showStats ? "chart.bar.fill" : "chart.bar", tooltip: "Toggle Stats") {
                    showStats.toggle()
                }
            }
            .padding(4)
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.regular)
        }
        .padding(DesignSystem.Spacing.sm)
        .background(
            VisualEffectView(material: .hudWindow)
                .opacity(0.8)
        )
        .cornerRadius(DesignSystem.CornerRadius.medium)
        .padding(DesignSystem.Spacing.sm)
    }
}

// MARK: - Viewport Stats Overlay

struct ViewportStatsOverlay: View {
    let profiler: Profiler
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                StatLine(label: "FPS", value: String(format: "%.1f", profiler.stats.fps), color: fpsColor)
                StatLine(label: "Frame", value: String(format: "%.2f ms", profiler.stats.frameTimeMs), color: frameTimeColor)
                StatLine(label: "Draw Calls", value: "\(profiler.stats.drawCalls)", color: .white)
                StatLine(label: "Triangles", value: formatNumber(profiler.stats.triangles), color: .white)
                StatLine(label: "Memory", value: formatMemory(profiler.stats.memoryUsed), color: .white)
            }
            .font(DesignSystem.Typography.mono)
            .foregroundColor(.white)
            .padding(DesignSystem.Spacing.sm)
            .background(
                VisualEffectView(material: .hudWindow)
                    .opacity(0.9)
            )
            .cornerRadius(DesignSystem.CornerRadius.tight)
            
            Spacer()
        }
    }
    
    private var fpsColor: Color {
        if profiler.stats.fps >= 50 {
            return .green
        } else if profiler.stats.fps >= 30 {
            return .yellow
        } else {
            return .red
        }
    }
    
    private var frameTimeColor: Color {
        if profiler.stats.frameTimeMs <= 16.67 {
            return .green
        } else if profiler.stats.frameTimeMs <= 33.33 {
            return .yellow
        } else {
            return .red
        }
    }
    
    private func formatNumber(_ number: Int) -> String {
        if number >= 1000000 {
            return String(format: "%.1fM", Double(number) / 1000000)
        } else if number >= 1000 {
            return String(format: "%.1fK", Double(number) / 1000)
        } else {
            return "\(number)"
        }
    }
    
    private func formatMemory(_ bytes: Int) -> String {
        let mb = Double(bytes) / (1024 * 1024)
        if mb >= 1024 {
            return String(format: "%.2f GB", mb / 1024)
        } else {
            return String(format: "%.1f MB", mb)
        }
    }
}

struct StatLine: View {
    let label: String
    let value: String
    let color: Color
    
    var body: some View {
        HStack {
            Text(label + ":")
                .foregroundColor(.white.opacity(0.8))
                .frame(width: 60, alignment: .leading)
            
            Text(value)
                .foregroundColor(color)
                .frame(width: 60, alignment: .trailing)
        }
    }
}

// MARK: - Engine Viewport

struct EngineViewport: NSViewRepresentable {
    let rendererBridge: UnsafeMutablePointer<RendererBridge>
    
    func makeNSView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.preferredFramesPerSecond = 60
        mtkView.enableSetNeedsDisplay = true
        mtkView.isPaused = false
        mtkView.clearColor = MTLClearColor(red: 0.05, green: 0.05, blue: 0.08, alpha: 1.0)
        
        // Setup the renderer bridge
        if let bridge = rendererBridge {
            // This would connect the Metal view to the C engine
            // Implementation depends on the specific engine architecture
        }
        
        return mtkView
    }
    
    func updateNSView(_ nsView: MTKView, context: Context) {
        // Update view properties if needed
    }
}

// MARK: - Visual Effect View

struct VisualEffectView: NSViewRepresentable {
    let material: NSVisualEffectView.Material
    
    func makeNSView(context: Context) -> NSVisualEffectView {
        let visualEffectView = NSVisualEffectView()
        visualEffectView.material = material
        visualEffectView.state = .active
        visualEffectView.isEmphasized = false
        return visualEffectView
    }
    
    func updateNSView(_ nsView: NSVisualEffectView, context: Context) {
        nsView.material = material
    }
}

// MARK: - Renderer Bridge Placeholder

struct RendererBridge {
    // This would contain the actual bridge to the C rendering engine
    // Implementation depends on the specific engine architecture
}
