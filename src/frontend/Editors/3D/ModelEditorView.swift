import SwiftUI

struct ModelEditorView: View {
    var body: some View {
        HStack(spacing: 0) {
            // Toolbar
            VStack(spacing: 15) {
                ToolButton(icon: "cursorarrow", label: "Select")
                ToolButton(icon: "move.3d", label: "Translate")
                ToolButton(icon: "rotate.3d", label: "Rotate")
                ToolButton(icon: "scale.3d", label: "Scale")
                Divider()
                ToolButton(icon: "cube", label: "Add Cube")
                ToolButton(icon: "sphere", label: "Add Sphere")
                Spacer()
            }
            .frame(width: 60)
            .padding(.top)
            .background(Color(NSColor.windowBackgroundColor))
            
            Divider()
            
            // Canvas Area
            VStack {
                Text("3D Model Editor")
                    .font(.headline)
                    .padding()
                
                ZStack {
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.black.opacity(0.1))
                        .overlay(
                            Text("Metal/Vulkan Viewport")
                                .foregroundColor(.secondary)
                        )
                }
                .padding()
            }
            
            Divider()
            
            // Properties Sidebar
            VStack(alignment: .leading, spacing: 10) {
                Text("Properties")
                    .font(.subheadline)
                    .bold()
                
                PropertyRow(label: "Transform", value: "0, 0, 0")
                PropertyRow(label: "Material", value: "Default")
                PropertyRow(label: "Mesh", value: "Cube")
                
                Spacer()
            }
            .frame(width: 200)
            .padding()
            .background(Color(NSColor.windowBackgroundColor))
        }
        .frame(minWidth: 800, minHeight: 500)
    }
}

struct ToolButton: View {
    let icon: String
    let label: String
    
    var body: some View {
        Button(action: {}) {
            VStack {
                Image(systemName: icon)
                    .font(.title2)
                Text(label)
                    .font(.caption2)
            }
        }
        .buttonStyle(.plain)
    }
}

struct PropertyRow: View {
    let label: String
    let value: String
    
    var body: some View {
        VStack(alignment: .leading) {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            Text(value)
                .font(.body)
        }
    }
}
