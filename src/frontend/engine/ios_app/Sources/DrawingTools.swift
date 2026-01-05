import SwiftUI

/// Drawing tool types
enum DrawingTool: String, CaseIterable {
    case brush = "Brush"
    case pencil = "Pencil"
    case eraser = "Eraser"
    case fill = "Fill Bucket"
    case eyedropper = "Eyedropper"
    case line = "Line"
    case rectangle = "Rectangle"
    case circle = "Circle"
    case select = "Select"
    
    var icon: String {
        switch self {
        case .brush: return "paintbrush.fill"
        case .pencil: return "pencil"
        case .eraser: return "eraser.fill"
        case .fill: return "paintbrush.pointed.fill"
        case .eyedropper: return "eyedropper"
        case .line: return "line.diagonal"
        case .rectangle: return "rectangle"
        case .circle: return "circle"
        case .select: return "lasso"
        }
    }
}

/// Tool settings manager
class ToolSettings: ObservableObject {
    @Published var currentTool: DrawingTool = .brush
    @Published var brushSize: Int = 1
    @Published var brushOpacity: Double = 1.0
    @Published var primaryColor: Color = .white
    @Published var secondaryColor: Color = .black
    
    // Shape tool settings
    @Published var fillShape: Bool = false
    @Published var shapeOutlineWidth: Int = 1
}

/// Drawing engine
class DrawingEngine {
    let layerManager: SpriteLayerManager
    let toolSettings: ToolSettings
    
    private var lastPoint: CGPoint?
    private var shapeStartPoint: CGPoint?
    
    init(layerManager: SpriteLayerManager, toolSettings: ToolSettings) {
        self.layerManager = layerManager
        self.toolSettings = toolSettings
    }
    
    // MARK: - Drawing Operations
    
    func beginStroke(at point: CGPoint) {
        lastPoint = point
        shapeStartPoint = point
        
        switch toolSettings.currentTool {
        case .brush, .pencil, .eraser:
            drawPoint(at: point)
        case .eyedropper:
            pickColor(at: point)
        default:
            break
        }
    }
    
    func continueStroke(at point: CGPoint) {
        guard let last = lastPoint else { return }
        
        switch toolSettings.currentTool {
        case .brush, .pencil, .eraser:
            drawLine(from: last, to: point)
        default:
            break
        }
        
        lastPoint = point
    }
    
    func endStroke(at point: CGPoint) {
        switch toolSettings.currentTool {
        case .fill:
            floodFill(at: point)
        case .line:
            if let start = shapeStartPoint {
                drawLine(from: start, to: point, final: true)
            }
        case .rectangle:
            if let start = shapeStartPoint {
                drawRectangle(from: start, to: point)
            }
        case .circle:
            if let start = shapeStartPoint {
                drawCircle(from: start, to: point)
            }
        case .select:
            if let start = shapeStartPoint {
                updateSelection(from: start, to: point)
            }
        default:
            break
        }
        
        lastPoint = nil
        shapeStartPoint = nil
    }
    
    // MARK: - Transform Operations
    
    func flipHorizontal() {
        guard let layer = layerManager.selectedLayer else { return }
        var newPixels = layer.pixels
        for y in 0..<layer.height {
            for x in 0..<layer.width {
                newPixels[y * layer.width + x] = layer.pixels[y * layer.width + (layer.width - 1 - x)]
            }
        }
        layer.pixels = newPixels
    }
    
    func flipVertical() {
        guard let layer = layerManager.selectedLayer else { return }
        var newPixels = layer.pixels
        for y in 0..<layer.height {
            for x in 0..<layer.width {
                newPixels[y * layer.width + x] = layer.pixels[(layer.height - 1 - y) * layer.width + x]
            }
        }
        layer.pixels = newPixels
    }
    
    func rotateClockwise() {
        guard let layer = layerManager.selectedLayer else { return }
        var newPixels = Array(repeating: Color.clear, count: layer.pixels.count)
        for y in 0..<layer.height {
            for x in 0..<layer.width {
                newPixels[x * layer.height + (layer.height - 1 - y)] = layer.pixels[y * layer.width + x]
            }
        }
        layer.pixels = newPixels
    }
    
    // MARK: - Tool Implementations
    
    private func drawPoint(at point: CGPoint) {
        guard let layer = layerManager.selectedLayer else { return }
        
        let x = Int(point.x)
        let y = Int(point.y)
        let color = toolSettings.currentTool == .eraser ? .clear : toolSettings.primaryColor
        
        // Apply brush size
        let radius = toolSettings.brushSize / 2
        for dy in -radius...radius {
            for dx in -radius...radius {
                if dx * dx + dy * dy <= radius * radius {
                    layer.setPixel(x: x + dx, y: y + dy, color: color)
                }
            }
        }
    }
    
    private func drawLine(from start: CGPoint, to end: CGPoint, final: Bool = false) {
        // Bresenham's line algorithm
        let x0 = Int(start.x)
        let y0 = Int(start.y)
        let x1 = Int(end.x)
        let y1 = Int(end.y)
        
        let dx = abs(x1 - x0)
        let dy = abs(y1 - y0)
        let sx = x0 < x1 ? 1 : -1
        let sy = y0 < y1 ? 1 : -1
        var err = dx - dy
        
        var x = x0
        var y = y0
        
        while true {
            drawPoint(at: CGPoint(x: x, y: y))
            
            if x == x1 && y == y1 { break }
            
            let e2 = 2 * err
            if e2 > -dy {
                err -= dy
                x += sx
            }
            if e2 < dx {
                err += dx
                y += sy
            }
        }
    }
    
    private func drawRectangle(from start: CGPoint, to end: CGPoint) {
        guard let layer = layerManager.selectedLayer else { return }
        
        let x1 = Int(min(start.x, end.x))
        let y1 = Int(min(start.y, end.y))
        let x2 = Int(max(start.x, end.x))
        let y2 = Int(max(start.y, end.y))
        
        if toolSettings.fillShape {
            // Fill rectangle
            for y in y1...y2 {
                for x in x1...x2 {
                    layer.setPixel(x: x, y: y, color: toolSettings.primaryColor)
                }
            }
        } else {
            // Draw outline
            for x in x1...x2 {
                layer.setPixel(x: x, y: y1, color: toolSettings.primaryColor)
                layer.setPixel(x: x, y: y2, color: toolSettings.primaryColor)
            }
            for y in y1...y2 {
                layer.setPixel(x: x1, y: y, color: toolSettings.primaryColor)
                layer.setPixel(x: x2, y: y, color: toolSettings.primaryColor)
            }
        }
    }
    
    private func drawCircle(from start: CGPoint, to end: CGPoint) {
        guard let layer = layerManager.selectedLayer else { return }
        
        let centerX = Int(start.x)
        let centerY = Int(start.y)
        let radius = Int(sqrt(pow(end.x - start.x, 2) + pow(end.y - start.y, 2)))
        
        if toolSettings.fillShape {
            // Fill circle
            for y in (centerY - radius)...(centerY + radius) {
                for x in (centerX - radius)...(centerX + radius) {
                    let dx = x - centerX
                    let dy = y - centerY
                    if dx * dx + dy * dy <= radius * radius {
                        layer.setPixel(x: x, y: y, color: toolSettings.primaryColor)
                    }
                }
            }
        } else {
            // Midpoint circle algorithm (outline)
            var x = radius
            var y = 0
            var err = 0
            
            while x >= y {
                layer.setPixel(x: centerX + x, y: centerY + y, color: toolSettings.primaryColor)
                layer.setPixel(x: centerX + y, y: centerY + x, color: toolSettings.primaryColor)
                layer.setPixel(x: centerX - y, y: centerY + x, color: toolSettings.primaryColor)
                layer.setPixel(x: centerX - x, y: centerY + y, color: toolSettings.primaryColor)
                layer.setPixel(x: centerX - x, y: centerY - y, color: toolSettings.primaryColor)
                layer.setPixel(x: centerX - y, y: centerY - x, color: toolSettings.primaryColor)
                layer.setPixel(x: centerX + y, y: centerY - x, color: toolSettings.primaryColor)
                layer.setPixel(x: centerX + x, y: centerY - y, color: toolSettings.primaryColor)
                
                if err <= 0 {
                    y += 1
                    err += 2 * y + 1
                }
                if err > 0 {
                    x -= 1
                    err -= 2 * x + 1
                }
            }
        }
    }
    
    private func floodFill(at point: CGPoint) {
        guard let layer = layerManager.selectedLayer else { return }
        
        let x = Int(point.x)
        let y = Int(point.y)
        guard let targetColor = layer.getPixel(x: x, y: y) else { return }
        
        let fillColor = toolSettings.primaryColor
        guard targetColor != fillColor else { return }
        
        // Simple flood fill (stack-based)
        var stack: [(Int, Int)] = [(x, y)]
        
        while !stack.isEmpty {
            let (px, py) = stack.removeLast()
            
            guard let currentColor = layer.getPixel(x: px, y: py),
                  currentColor == targetColor else { continue }
            
            layer.setPixel(x: px, y: py, color: fillColor)
            
            // Add neighbors
            stack.append((px + 1, py))
            stack.append((px - 1, py))
            stack.append((px, py + 1))
            stack.append((px, py - 1))
        }
    }
    
    private func pickColor(at point: CGPoint) {
        guard let layer = layerManager.selectedLayer else { return }
        
        let x = Int(point.x)
        let y = Int(point.y)
        if let color = layer.getPixel(x: x, y: y) {
            toolSettings.primaryColor = color
        }
    }
    
    private func updateSelection(from start: CGPoint, to end: CGPoint) {
        let x1 = Int(min(start.x, end.x))
        let y1 = Int(min(start.y, end.y))
        let x2 = Int(max(start.x, end.x))
        let y2 = Int(max(start.y, end.y))
        
        var newSelection = Set<Int>()
        for y in y1...y2 {
            for x in x1...x2 {
                if x >= 0 && x < layerManager.canvasSize && y >= 0 && y < layerManager.canvasSize {
                    newSelection.insert(y * layerManager.canvasSize + x)
                }
            }
        }
        layerManager.selection = newSelection
    }
}

/// Tool palette view
struct ToolPaletteView: View {
    @ObservedObject var toolSettings: ToolSettings
    
    var body: some View {
        VStack(spacing: 12) {
            // Tool buttons
            ForEach(DrawingTool.allCases, id: \.self) { tool in
                Button(action: { toolSettings.currentTool = tool }) {
                    VStack(spacing: 4) {
                        Image(systemName: tool.icon)
                            .font(.title3)
                        Text(tool.rawValue)
                            .font(.caption2)
                    }
                    .foregroundColor(toolSettings.currentTool == tool ? .cyan : .gray)
                    .frame(width: 60, height: 60)
                    .background(toolSettings.currentTool == tool ? Color.cyan.opacity(0.1) : Color.clear)
                    .cornerRadius(8)
                }
                .buttonStyle(PlainButtonStyle())
            }
            
            Divider().background(Color.gray)
            
            // Transform actions (Quick access)
            HStack(spacing: 8) {
                ActionButton(icon: "arrow.left.and.right.righttriangle.left.righttriangle.right", label: "Flip H") {
                    DrawingEngine(layerManager: layerManager, toolSettings: toolSettings).flipHorizontal()
                }
                ActionButton(icon: "arrow.up.and.down.righttriangle.up.righttriangle.down", label: "Flip V") {
                    DrawingEngine(layerManager: layerManager, toolSettings: toolSettings).flipVertical()
                }
            }
            .padding(.horizontal, 4)
            
            Divider().background(Color.gray)
            
            // Tool settings
            VStack(alignment: .leading, spacing: 8) {
                Text("Size")
                    .font(.caption2)
                    .foregroundColor(.gray)
                
                Slider(value: Binding(
                    get: { Double(toolSettings.brushSize) },
                    set: { toolSettings.brushSize = Int($0) }
                ), in: 1...10, step: 1)
                    .accentColor(.cyan)
                
                Text("\(toolSettings.brushSize)px")
                    .font(.caption2.monospaced())
                    .foregroundColor(.white)
                
                if toolSettings.currentTool == .rectangle || toolSettings.currentTool == .circle {
                    Toggle("Fill", isOn: $toolSettings.fillShape)
                        .font(.caption2)
                        .toggleStyle(SwitchToggleStyle(tint: .cyan))
                }
            }
            .padding(.horizontal, 8)
            
            Spacer()
            
            // Color swatches
            VStack(spacing: 8) {
                ColorSwatch(color: $toolSettings.primaryColor, label: "Primary")
                ColorSwatch(color: $toolSettings.secondaryColor, label: "Secondary")
            }
            .padding(.horizontal, 8)
        }
        .frame(width: 80)
        .padding(.vertical, 12)
        .background(Color.black.opacity(0.3))
        .cornerRadius(12)
    }
}

/// Color swatch component
struct ColorSwatch: View {
    @Binding var color: Color
    let label: String
    @State private var showPicker = false
    
    var body: some View {
        VStack(spacing: 4) {
            Button(action: { showPicker.toggle() }) {
                RoundedRectangle(cornerRadius: 6)
                    .fill(color)
                    .frame(width: 40, height: 40)
                    .overlay(
                        RoundedRectangle(cornerRadius: 6)
                            .stroke(Color.white.opacity(0.3), lineWidth: 1)
                    )
            }
            .buttonStyle(PlainButtonStyle())
            .popover(isPresented: $showPicker) {
                ColorPicker("", selection: $color)
                    .labelsHidden()
                    .padding()
            }
            
            Text(label)
                .font(.caption2)
                .foregroundColor(.gray)
        }
    }
}
