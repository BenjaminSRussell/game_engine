import SwiftUI

@available(macOS 14.0, *)
struct ViewportWithTransformControls: View {
    @StateObject private var transformController = TransformController()
    @ObservedObject var selectionManager: SelectionManager
    
    @State private var isDragging = false
    @State private var dragStart: CGPoint = .zero
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Main viewport content (placeholder for now)
                Color.black
                    .overlay(
                        Text("3D Viewport")
                            .font(DesignSystem.Typography.h2)
                            .foregroundColor(.white.opacity(0.2))
                    )
                
                // Transform Gizmo Overlay
                TransformGizmo(
                    controller: transformController,
                    selectionManager: selectionManager,
                    viewportSize: geometry.size
                )
                
                // Transform Toolbar
                VStack {
                    TransformToolbar(controller: transformController)
                        .padding(8)
                        .background(DesignSystem.Colors.backgroundSecondary.opacity(0.95))
                        .cornerRadius(8)
                        .padding()
                    
                    Spacer()
                }
            }
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { value in
                        if !isDragging {
                            isDragging = true
                            dragStart = value.startLocation
                        }
                        
                        if transformController.mode.isActive {
                            let delta = CGPoint(
                                x: value.location.x - dragStart.x,
                                y: value.location.y - dragStart.y
                            )
                            transformController.updateTransform(mouseDelta: delta)
                        }
                    }
                    .onEnded { _ in
                        isDragging = false
                    }
            )
            .onKeyPress(.return) {
                if transformController.mode.isActive {
                    transformController.commitTransform()
                    return .handled
                }
                return .ignored
            }
            .onKeyPress(.escape) {
                if transformController.mode.isActive {
                    transformController.cancelTransform()
                    return .handled
                }
                return .ignored
            }
            .onKeyPress(KeyEquivalent("g")) {
                transformController.beginTranslate()
                return .handled
            }
            .onKeyPress(KeyEquivalent("r")) {
                transformController.beginRotate()
                return .handled
            }
            .onKeyPress(KeyEquivalent("s")) {
                transformController.beginScale()
                return .handled
            }
            .onKeyPress(KeyEquivalent("x")) {
                if transformController.mode.isActive {
                    transformController.setConstraint(.x)
                    return .handled
                }
                return .ignored
            }
            .onKeyPress(KeyEquivalent("y")) {
                if transformController.mode.isActive {
                    transformController.setConstraint(.y)
                    return .handled
                }
                return .ignored
            }
            .onKeyPress(KeyEquivalent("z")) {
                if transformController.mode.isActive {
                    transformController.setConstraint(.z)
                    return .handled
                }
                return .ignored
            }
            .onAppear {
                transformController.selectionManager = selectionManager
            }
        }
    }
}

@available(macOS 14.0, *)
#Preview {
    ViewportWithTransformControls(selectionManager: SelectionManager())
        .frame(width: 800, height: 600)
}
