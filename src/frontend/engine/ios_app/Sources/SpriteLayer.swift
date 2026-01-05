import SwiftUI

/// Layer system for sprite editor
class SpriteLayerManager: ObservableObject {
    @Published var layers: [SpriteLayer] = []
    @Published var frames: [SpriteFrame] = []
    @Published var currentFrameIndex: Int = 0
    @Published var selectedLayerIndex: Int = 0
    @Published var canvasSize: Int = 32
    @Published var selection: Set<Int> = [] // Indices of selected pixels
    
    var currentFrame: SpriteFrame {
        guard currentFrameIndex < frames.count else { return frames[0] }
        return frames[currentFrameIndex]
    }
    
    var selectedLayer: SpriteLayer? {
        guard selectedLayerIndex < currentFrame.layers.count else { return nil }
        return currentFrame.layers[selectedLayerIndex]
    }
    
    init(canvasSize: Int = 32) {
        self.canvasSize = canvasSize
        addFrame()
    }
    
    // MARK: - Frame Management
    
    func addFrame() {
        let newFrame = SpriteFrame(canvasSize: canvasSize)
        frames.append(newFrame)
        currentFrameIndex = frames.count - 1
    }
    
    func deleteFrame(at index: Int) {
        guard frames.count > 1 else { return }
        frames.remove(at: index)
        if currentFrameIndex >= frames.count {
            currentFrameIndex = frames.count - 1
        }
    }
    
    // MARK: - Layer Management
    
    func addLayer(name: String? = nil) {
        currentFrame.addLayer(name: name, canvasSize: canvasSize)
        selectedLayerIndex = 0
    }
    
    func deleteLayer(at index: Int) {
        guard currentFrame.layers.count > 1, index < currentFrame.layers.count else { return }
        currentFrame.layers.remove(at: index)
        if selectedLayerIndex >= currentFrame.layers.count {
            selectedLayerIndex = currentFrame.layers.count - 1
        }
    }
    
    func duplicateLayer(at index: Int) {
        guard index < currentFrame.layers.count else { return }
        let layer = currentFrame.layers[index]
        let duplicate = SpriteLayer(
            name: "\(layer.name) Copy",
            width: canvasSize,
            height: canvasSize,
            pixels: layer.pixels,
            opacity: layer.opacity,
            zDepth: layer.zDepth,
            blendMode: layer.blendMode
        )
        currentFrame.layers.insert(duplicate, at: index)
        selectedLayerIndex = index
    }
    
    func moveLayer(from source: Int, to destination: Int) {
        guard source < currentFrame.layers.count, destination < currentFrame.layers.count else { return }
        let layer = currentFrame.layers.remove(at: source)
        currentFrame.layers.insert(layer, at: destination)
        selectedLayerIndex = destination
    }
    
    func mergeDown(at index: Int) {
        guard index > 0, index < currentFrame.layers.count else { return }
        let upperLayer = currentFrame.layers[index]
        let lowerLayer = currentFrame.layers[index - 1]
        
        // Merge pixels
        for i in 0..<lowerLayer.pixels.count {
            if upperLayer.pixels[i] != .clear {
                lowerLayer.pixels[i] = upperLayer.pixels[i]
            }
        }
        
        currentFrame.layers.remove(at: index)
        selectedLayerIndex = index - 1
    }
    
    // MARK: - Composite Rendering
    
    func getCompositeImage(forFrame index: Int? = nil) -> [Color] {
        let frame = index != nil ? frames[index!] : currentFrame
        var composite = Array(repeating: Color.clear, count: canvasSize * canvasSize)
        
        // Render from bottom to top
        for layer in frame.layers.reversed() {
            guard layer.isVisible else { continue }
            
            for i in 0..<composite.count {
                if layer.pixels[i] != .clear {
                    composite[i] = blendColors(
                        bottom: composite[i],
                        top: layer.pixels[i],
                        opacity: layer.opacity,
                        mode: layer.blendMode
                    )
                }
            }
        }
        
        return composite
    }
    
    // MARK: - Blend Modes
    
    private func blendColors(bottom: Color, top: Color, opacity: Double, mode: BlendMode) -> Color {
        // Simplified blending - in production would use proper color space math
        switch mode {
        case .normal:
            return top.opacity(opacity)
        case .multiply:
            // Simplified multiply blend
            return top.opacity(opacity * 0.7)
        case .screen:
            return top.opacity(opacity * 1.2)
        case .overlay:
            return top.opacity(opacity)
        case .add:
            return top.opacity(min(1.0, opacity * 1.5))
        }
    }
}

/// Individual sprite layer
class SpriteLayer: ObservableObject, Identifiable {
    let id = UUID()
    @Published var name: String
    @Published var isVisible: Bool = true
    @Published var isLocked: Bool = false
    @Published var opacity: Double = 1.0
    @Published var zDepth: Double = 0.0 // 2.5D depth
    @Published var blendMode: BlendMode = .normal
    @Published var pixels: [Color]
    
    let width: Int
    let height: Int
    
    init(name: String, width: Int, height: Int, pixels: [Color]? = nil, opacity: Double = 1.0, zDepth: Double = 0.0, blendMode: BlendMode = .normal) {
        self.name = name
        self.width = width
        self.height = height
        self.opacity = opacity
        self.zDepth = zDepth
        self.blendMode = blendMode
        
        if let pixels = pixels {
            self.pixels = pixels
        } else {
            self.pixels = Array(repeating: .clear, count: width * height)
        }
    }
    
    func setPixel(x: Int, y: Int, color: Color) {
        guard !isLocked, x >= 0, x < width, y >= 0, y < height else { return }
        let index = y * width + x
        pixels[index] = color
    }
    
    func getPixel(x: Int, y: Int) -> Color? {
        guard x >= 0, x < width, y >= 0, y < height else { return nil }
        let index = y * width + x
        return pixels[index]
    }
    
    func clear() {
        guard !isLocked else { return }
        pixels = Array(repeating: .clear, count: width * height)
    }
    
    func fill(with color: Color) {
        guard !isLocked else { return }
        pixels = Array(repeating: color, count: width * height)
    }
}

/// Blend modes for layers
enum BlendMode: String, CaseIterable {
    case normal = "Normal"
    case multiply = "Multiply"
    case screen = "Screen"
    case overlay = "Overlay"
    case add = "Add"
}

/// Layer panel view
struct LayerPanelView: View {
    @ObservedObject var layerManager: SpriteLayerManager
    
    var body: some View {
        VStack(spacing: 0) {
            // Layer Controls
            HStack(spacing: 8) {
                Button(action: { layerManager.addLayer() }) {
                    Image(systemName: "plus")
                }
                
                Button(action: {
                    if layerManager.selectedLayerIndex < layerManager.layers.count {
                        layerManager.deleteLayer(at: layerManager.selectedLayerIndex)
                    }
                }) {
                    Image(systemName: "trash")
                }
                .disabled(layerManager.layers.count <= 1)
                
                Button(action: {
                    if layerManager.selectedLayerIndex < layerManager.layers.count {
                        layerManager.duplicateLayer(at: layerManager.selectedLayerIndex)
                    }
                }) {
                    Image(systemName: "doc.on.doc")
                }
                
                Button(action: {
                    if layerManager.selectedLayerIndex < layerManager.layers.count {
                        layerManager.mergeDown(at: layerManager.selectedLayerIndex)
                    }
                }) {
                    Image(systemName: "arrow.down.to.line")
                }
                .disabled(layerManager.selectedLayerIndex == 0)
                
                Spacer()
            }
            .padding(8)
            .background(Color(white: 0.15))
            
            // Layer List
            ScrollView {
                VStack(spacing: 2) {
                    ForEach(Array(layerManager.currentFrame.layers.enumerated()), id: \.element.id) { index, layer in
                        LayerRow(
                            layer: layer,
                            isSelected: index == layerManager.selectedLayerIndex
                        )
                        .onTapGesture {
                            layerManager.selectedLayerIndex = index
                        }
                    }
                }
            }
        }
        .frame(width: 250)
        .background(Color(white: 0.12))
    }
}

/// Individual layer row
struct LayerRow: View {
    @ObservedObject var layer: SpriteLayer
    let isSelected: Bool
    
    var body: some View {
        HStack(spacing: 8) {
            // Visibility toggle
            Button(action: { layer.isVisible.toggle() }) {
                Image(systemName: layer.isVisible ? "eye" : "eye.slash")
                    .foregroundColor(layer.isVisible ? .white : .gray)
            }
            .buttonStyle(PlainButtonStyle())
            
            // Layer thumbnail (simplified)
            Rectangle()
                .fill(Color.white.opacity(0.1))
                .frame(width: 32, height: 32)
                .cornerRadius(4)
                .overlay(
                    Text("L")
                        .font(.caption2)
                        .foregroundColor(.gray)
                )
            
            // Layer name
            TextField("Layer name", text: $layer.name)
                .textFieldStyle(PlainTextFieldStyle())
                .font(.caption)
                .foregroundColor(.white)
            
            // Lock toggle
            Button(action: { layer.isLocked.toggle() }) {
                Image(systemName: layer.isLocked ? "lock.fill" : "lock.open")
                    .foregroundColor(layer.isLocked ? .yellow : .gray)
            }
            .buttonStyle(PlainButtonStyle())
        }
        .padding(.horizontal, 8)
        .padding(.vertical, 6)
        .background(isSelected ? Color.cyan.opacity(0.2) : Color.clear)
        .cornerRadius(4)
        
        // Layer properties (when selected)
        if isSelected {
            VStack(spacing: 8) {
                // Opacity slider
                HStack {
                    Text("Opacity")
                        .font(.caption2)
                        .foregroundColor(.gray)
                    Slider(value: $layer.opacity, in: 0...1)
                        .accentColor(.cyan)
                    Text("\(Int(layer.opacity * 100))%")
                        .font(.caption2.monospaced())
                        .foregroundColor(.gray)
                        .frame(width: 40)
                }
                
                // Blend mode picker
                HStack {
                    Text("Blend")
                        .font(.caption2)
                        .foregroundColor(.gray)
                    Picker("", selection: $layer.blendMode) {
                        ForEach(BlendMode.allCases, id: \.self) { mode in
                            Text(mode.rawValue).tag(mode)
                        }
                    }
                    .pickerStyle(MenuPickerStyle())
                    .labelsHidden()
                }
            }
            .padding(.horizontal, 8)
            .padding(.bottom, 8)
            
            // Depth Slider
            HStack {
                Text("Depth")
                    .font(.caption2)
                    .foregroundColor(.gray)
                Slider(value: $layer.zDepth, in: -1...1)
                    .accentColor(.purple)
                Text(String(format: "%.1f", layer.zDepth))
                    .font(.caption2.monospaced())
                    .foregroundColor(.gray)
                    .frame(width: 40)
            }
            .padding(.horizontal, 8)
            .padding(.bottom, 8)
        }
    }
}

/// Parallax Preview for 2.5D effects
struct ParallaxPreviewView: View {
    @ObservedObject var layerManager: SpriteLayerManager
    @State private var offset: CGFloat = 0
    
    var body: some View {
        VStack {
            Text("2.5D PARALLAX PREVIEW")
                .font(.caption.bold())
                .foregroundColor(.cyan)
                .padding(.top, 4)
            
            GeometryReader { geo in
                ZStack {
                    ForEach(Array(layerManager.currentFrame.layers.enumerated()), id: \.element.id) { index, layer in
                        if layer.isVisible {
                            CanvasView(
                                pixels: layer.pixels,
                                gridSize: layer.width,
                                cellSize: 2, // Small preview size
                                showGrid: false,
                                isInteractive: false
                            )
                            .offset(x: offset * CGFloat(layer.zDepth))
                        }
                    }
                }
                .position(x: geo.size.width / 2, y: geo.size.height / 2)
                .gesture(
                    DragGesture()
                        .onChanged { value in
                            offset = value.translation.width
                        }
                        .onEnded { _ in 
                            withAnimation(.spring()) { offset = 0 }
                        }
                )
            }
        }
    }
}
