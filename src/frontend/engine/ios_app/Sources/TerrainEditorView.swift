import SwiftUI

struct TerrainEditorView: View {
    @StateObject private var terrainData = TerrainData(size: 64)
    @State private var selectedTool: TerrainTool = .sculpt
    @State private var brushSize: Float = 5.0
    @State private var brushIntensity: Float = 0.5
    
    enum TerrainTool {
        case sculpt, smooth, flatten, paint, foliage
    }
    
    var body: some View {
        HStack(spacing: 0) {
            // Left: Tool Palette
            VStack(spacing: 15) {
                TerrainToolButton(icon: "mountain.2.fill", label: "Sculpt", isSelected: selectedTool == .sculpt) { selectedTool = .sculpt }
                TerrainToolButton(icon: "radius", label: "Smooth", isSelected: selectedTool == .smooth) { selectedTool = .smooth }
                TerrainToolButton(icon: "framer", label: "Flatten", isSelected: selectedTool == .flatten) { selectedTool = .flatten }
                TerrainToolButton(icon: "paintpalette.fill", label: "Paint", isSelected: selectedTool == .paint) { selectedTool = .paint }
                TerrainToolButton(icon: "leaf.fill", label: "Foliage", isSelected: selectedTool == .foliage) { selectedTool = .foliage }
                
                Spacer()
                
                // Brush Settings
                VStack(alignment: .leading, spacing: 10) {
                    Text("BRUSH SETTINGS")
                        .font(.caption2.bold())
                        .foregroundColor(.gray)
                    
                    VStack(alignment: .leading) {
                        Text("Size: \(Int(brushSize))")
                            .font(.caption2.monospaced())
                        Slider(value: $brushSize, in: 1...20)
                            .accentColor(.cyan)
                    }
                    
                    VStack(alignment: .leading) {
                        Text("Intensity: \(Int(brushIntensity * 100))%")
                            .font(.caption2.monospaced())
                        Slider(value: $brushIntensity, in: 0...1)
                            .accentColor(.cyan)
                    }
                }
                .padding()
                .background(Color.black.opacity(0.3))
                .cornerRadius(8)
            }
            .padding()
            .frame(width: 120)
            .background(Color(white: 0.12))
            
            // Center: 3D Terrain Viewport
            ZStack {
                TerrainViewport(data: terrainData, tool: selectedTool, brushSize: brushSize, brushIntensity: brushIntensity)
                    .edgesIgnoringSafeArea(.all)
                
                // Overlay Info
                VStack {
                    HStack {
                        VStack(alignment: .leading) {
                            Text("WORLD EDITOR")
                                .font(.headline)
                                .foregroundColor(.cyan)
                            Text("Resolution: \(terrainData.size)x\(terrainData.size)")
                                .font(.caption.monospaced())
                                .foregroundColor(.white)
                        }
                        .padding()
                        .background(Color.black.opacity(0.5))
                        .cornerRadius(8)
                        
                        Spacer()
                    }
                    .padding()
                    
                    Spacer()
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Color.black)
            
            // Right: Layer/Biome Inspector
            VStack {
                Text("BIOMES & LAYERS")
                    .font(.caption.bold())
                    .padding()
                
                List {
                    BiomeRow(name: "Grasslands", color: .green, isSelected: true)
                    BiomeRow(name: "Mountainous", color: .gray, isSelected: false)
                    BiomeRow(name: "Desert", color: .yellow, isSelected: false)
                }
                .listStyle(PlainListStyle())
            }
            .frame(width: 200)
            .background(Color(white: 0.12))
        }
    }
}

struct TerrainToolButton: View {
    let icon: String
    let label: String
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            VStack(spacing: 4) {
                Image(systemName: icon)
                    .font(.system(size: 20))
                Text(label)
                    .font(.system(size: 10, weight: .bold))
            }
            .foregroundColor(isSelected ? .cyan : .gray)
            .frame(width: 70, height: 60)
            .background(isSelected ? Color.cyan.opacity(0.1) : Color.clear)
            .cornerRadius(8)
            .overlay(
                RoundedRectangle(cornerRadius: 8)
                    .stroke(isSelected ? Color.cyan.opacity(0.5) : Color.clear, lineWidth: 1)
            )
        }
    }
}

struct BiomeRow: View {
    let name: String
    let color: Color
    let isSelected: Bool
    
    var body: some View {
        HStack {
            Circle()
                .fill(color)
                .frame(width: 12, height: 12)
            Text(name)
                .font(.caption)
            Spacer()
            if isSelected {
                Image(systemName: "checkmark")
                    .foregroundColor(.cyan)
                    .font(.caption2)
            }
        }
        .padding(.vertical, 4)
        .listRowBackground(isSelected ? Color.white.opacity(0.05) : Color.clear)
    }
}

struct TerrainViewport: UIViewRepresentable {
    @ObservedObject var data: TerrainData
    var tool: TerrainEditorView.TerrainTool
    var brushSize: Float
    var brushIntensity: Float
    
    func makeUIView(context: Context) -> MTKView {
        let mtkView = MTKView()
        mtkView.device = MTLCreateSystemDefaultDevice()
        mtkView.clearColor = MTLClearColor(red: 0.1, green: 0.12, blue: 0.15, alpha: 1.0)
        mtkView.depthStencilPixelFormat = .depth32Float
        
        let renderer = TerrainRenderer(device: mtkView.device!, terrain: data)
        context.coordinator.renderer = renderer
        mtkView.delegate = renderer
        
        // Add gesture recognizers
        let pan = UIPanGestureRecognizer(target: context.coordinator, action: #selector(Coordinator.handlePan(_:)))
        mtkView.addGestureRecognizer(pan)
        
        let longPress = UILongPressGestureRecognizer(target: context.coordinator, action: #selector(Coordinator.handleLongPress(_:)))
        longPress.minimumPressDuration = 0
        mtkView.addGestureRecognizer(longPress)
        
        return mtkView
    }
    
    func updateUIView(_ uiView: MTKView, context: Context) {
        context.coordinator.tool = tool
        context.coordinator.brushSize = brushSize
        context.coordinator.brushIntensity = brushIntensity
        context.coordinator.renderer?.updateMesh()
    }
    
    func makeCoordinator() -> Coordinator {
        Coordinator(data: data)
    }
    
    class Coordinator {
        var data: TerrainData
        var renderer: TerrainRenderer?
        var tool: TerrainEditorView.TerrainTool = .sculpt
        var brushSize: Float = 5.0
        var brushIntensity: Float = 0.5
        
        init(data: TerrainData) {
            self.data = data
        }
        
        @objc func handlePan(_ gesture: UIPanGestureRecognizer) {
            guard let renderer = renderer else { return }
            let translation = gesture.translation(in: gesture.view)
            renderer.rotation.y += Float(translation.x) * 0.01
            renderer.rotation.x += Float(translation.y) * 0.01
            gesture.setTranslation(.zero, in: gesture.view)
        }
        
        @objc func handleLongPress(_ gesture: UILongPressGestureRecognizer) {
            guard let view = gesture.view as? MTKView, let renderer = renderer else { return }
            let point = gesture.location(in: view)
            
            // Map screen point to terrain coordinates (Simplified)
            // In a full implementation, we'd use a raycaster for the terrain mesh
            let x = Int(point.x / view.bounds.width * CGFloat(data.size))
            let y = Int(point.y / view.bounds.height * CGFloat(data.size))
            
            if tool == .foliage {
                // Procedural foliage scatter
                let offset = Float(data.size) / 2.0
                for _ in 0...2 {
                    let rx = Float.random(in: -brushSize...brushSize)
                    let rz = Float.random(in: -brushSize...brushSize)
                    let tx = x + Int(rx)
                    let tz = y + Int(rz)
                    
                    if tx >= 0 && tx < data.size && tz >= 0 && tz < data.size {
                        let h = data.getHeight(at: tx, tz)
                        let pos = SIMD3<Float>(Float(tx) - offset, h, Float(tz) - offset)
                        let instance = FoliageInstance(position: pos, type: "tree", rotation: Float.random(in: 0...(.pi * 2)), scale: Float.random(in: 0.8...1.2))
                        data.foliage.append(instance)
                    }
                }
                data.objectWillChange.send()
            } else {
                data.sculpt(at: x, y: y, radius: brushSize, intensity: brushIntensity, tool: tool)
            }
        }
    }
}
