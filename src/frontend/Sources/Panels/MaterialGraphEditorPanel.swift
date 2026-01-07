import SwiftUI

struct MaterialGraphEditorPanel: View {
    @StateObject private var viewModel = MaterialGraphViewModel()
    @State private var showNodeMenu = false
    @State private var nodeMenuPosition: CGPoint = .zero
    
    var body: some View {
        VStack(spacing: 0) {
            // Toolbar
            HStack(spacing: 8) {
                Text("Material Graph Editor")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                // Add Node Menu
                Menu {
                    Button("Principled BSDF") {
                        viewModel.addNode(type: .principledBSDF, at: CGPoint(x: 200, y: 200))
                    }
                    
                    Divider()
                    
                    Button("Image Texture") {
                        viewModel.addNode(type: .imageTexture, at: CGPoint(x: 200, y: 200))
                    }
                    
                    Button("Mix RGB") {
                        viewModel.addNode(type: .mixRGB, at: CGPoint(x: 200, y: 200))
                    }
                    
                    Button("Color Ramp") {
                        viewModel.addNode(type: .colorRamp, at: CGPoint(x: 200, y: 200))
                    }
                    
                    Divider()
                    
                    Button("Math") {
                        viewModel.addNode(type: .math(operation: .add), at: CGPoint(x: 200, y: 200))
                    }
                } label: {
                    HStack {
                        Image(systemName: "plus.circle.fill")
                        Text("Add Node")
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
                }
                .menuStyle(.borderlessButton)
                
                Button(action: {
                    viewModel.resetView()
                }) {
                    Image(systemName: "scope")
                        .font(.system(size: 16))
                }
                .buttonStyle(.plain)
                .help("Reset View")
                
                Button(action: {
                    viewModel.deleteSelectedNode()
                }) {
                    Image(systemName: "trash")
                        .font(.system(size: 16))
                        .foregroundColor(.red)
                }
                .buttonStyle(.plain)
                .disabled(viewModel.selectedNodeID == nil)
                .help("Delete Selected Node")
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Canvas
            MaterialNodeCanvas(viewModel: viewModel)
        }
        .background(DesignSystem.Colors.backgroundPrimary)
    }
}

#Preview {
    MaterialGraphEditorPanel()
        .frame(width: 1200, height: 800)
}
