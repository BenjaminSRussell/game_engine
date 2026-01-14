import SwiftUI

struct SpriteEditorView: View {
    var body: some View {
        VStack(spacing: 0) {
            // Drawing Toolbar
            HStack(spacing: 20) {
                ToolButton(icon: "pencil", label: "Pencil")
                ToolButton(icon: "eraser", label: "Eraser")
                ToolButton(icon: "paintbrush", label: "Brush")
                ToolButton(icon: "eyedropper", label: "Picker")
                ToolButton(icon: "fill.and.line.vertical", label: "Fill")
                Spacer()
                Text("2.5D Sprite Editor")
                    .font(.headline)
                Spacer()
                Button("Export...") {}
            }
            .padding()
            .background(Color(NSColor.windowBackgroundColor))
            
            Divider()
            
            HStack(spacing: 0) {
                // Main Canvas
                ZStack {
                    CheckerboardView()
                        .opacity(0.1)
                    
                    Text("Pixel Grid Area")
                        .foregroundColor(.secondary)
                }
                .padding()
                
                Divider()
                
                // Color Palette & Layers
                VStack(alignment: .leading, spacing: 15) {
                    Text("Palette")
                        .font(.subheadline).bold()
                    
                    LazyVGrid(columns: [GridItem(.adaptive(minimum: 20))]) {
                        ForEach(0..<16) { i in
                            Color.random(seed: i)
                                .frame(width: 20, height: 20)
                                .cornerRadius(2)
                        }
                    }
                    
                    Divider()
                    
                    Text("Layers")
                        .font(.subheadline).bold()
                    Text("Layer 1")
                    Text("Background")
                    
                    Spacer()
                }
                .frame(width: 150)
                .padding()
                .background(Color(NSColor.windowBackgroundColor))
            }
        }
        .frame(minWidth: 800, minHeight: 500)
    }
}

struct CheckerboardView: View {
    var body: some View {
        GeometryReader { _ in
            Path { path in
                // Dummy checkerboard path
            }
            .fill(Color.gray)
        }
    }
}

extension Color {
    static func random(seed: Int) -> Color {
        let colors: [Color] = [.red, .blue, .green, .yellow, .orange, .purple, .pink, .cyan, .brown, .gray]
        return colors[seed % colors.count]
    }
}
