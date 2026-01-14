import SwiftUI

// MARK: - Splat Map Painter
struct SplatMapPainter: View {
    @ObservedObject var manager = TerrainEditorManager.shared
    @State private var paintMode: PaintMode = .paint
    @State private var selectedSplatLayer: Int = 0
    
    enum PaintMode: String, CaseIterable {
        case paint = "Paint"
        case erase = "Erase"
        case blend = "Blend"
        case smooth = "Smooth"
        
        var icon: String {
            switch self {
            case .paint: return "paintbrush"
            case .erase: return "eraser"
            case .blend: return "drop"
            case .smooth: return "waveform.path"
            }
        }
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: DesignSystem.Spacing.md) {
            Text("Splat Map Painting")
                .font(DesignSystem.Typography.bodyBold)
            
            // Paint mode selector
            Picker("Mode", selection: $paintMode) {
                ForEach(PaintMode.allCases, id: \.self) { mode in
                    Label(mode.rawValue, systemImage: mode.icon)
                        .tag(mode)
                }
            }
            .pickerStyle(.segmented)
            
            // Layer selector for splat painting
            VStack(alignment: .leading, spacing: 4) {
                Text("Target Layer")
                    .font(DesignSystem.Typography.small)
                Picker("Layer", selection: $selectedSplatLayer) {
                    ForEach(Array(manager.layers.enumerated()), id: \.offset) { index, layer in
                        Text(layer.name).tag(index)
                    }
                }
                .pickerStyle(.menu)
            }
            
            // Splat brush settings
            VStack(alignment: .leading, spacing: 4) {
                Text("Brush Settings")
                    .font(DesignSystem.Typography.small)
                
                FloatPropertyEditor(label: "Size", value: $manager.brushSize, range: 1...100, step: 1)
                FloatPropertyEditor(label: "Opacity", value: $manager.brushOpacity, range: 0...1, step: 0.05)
                FloatPropertyEditor(label: "Falloff", value: $manager.brushFalloff, range: 0...1, step: 0.05)
            }
            
            // Splat map operations
            VStack(alignment: .leading, spacing: 4) {
                Text("Operations")
                    .font(DesignSystem.Typography.small)
                
                HStack {
                    Button("Clear Layer") {
                        clearSplatLayer(selectedSplatLayer)
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Fill Layer") {
                        fillSplatLayer(selectedSplatLayer)
                    }
                    .buttonStyle(.bordered)
                }
                
                HStack {
                    Button("Invert Layer") {
                        invertSplatLayer(selectedSplatLayer)
                    }
                    .buttonStyle(.bordered)
                    
                    Button("Normalize All") {
                        normalizeSplatMaps()
                    }
                    .buttonStyle(.bordered)
                }
            }
            
            // Preview
            VStack(alignment: .leading, spacing: 4) {
                Text("Splat Map Preview")
                    .font(DesignSystem.Typography.small)
                
                RoundedRectangle(cornerRadius: 4)
                    .fill(Color.gray.opacity(0.3))
                    .frame(height: 100)
                    .overlay(
                        VStack {
                            Text("Layer \(selectedSplatLayer)")
                                .font(DesignSystem.Typography.small)
                            Text("Preview")
                                .font(DesignSystem.Typography.small)
                                .foregroundColor(DesignSystem.Colors.textSecondary)
                        }
                    )
            }
        }
        .padding(DesignSystem.Spacing.sm)
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(8)
    }
    
    private func clearSplatLayer(_ layerIndex: Int) {
        // TODO: Implement splat layer clearing
        print("Clear splat layer \(layerIndex)")
    }
    
    private func fillSplatLayer(_ layerIndex: Int) {
        // TODO: Implement splat layer filling
        print("Fill splat layer \(layerIndex)")
    }
    
    private func invertSplatLayer(_ layerIndex: Int) {
        // TODO: Implement splat layer inversion
        print("Invert splat layer \(layerIndex)")
    }
    
    private func normalizeSplatMaps() {
        // TODO: Implement splat map normalization
        print("Normalize all splat maps")
    }
}
